#include "Capture.h"

#include <MinHook.h>
#include <d3d11.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <array>
#include <unordered_map>

#include "GameData.h"

namespace nier::game
{
    namespace
    {
        // ---- vtable 槽位（依据 Windows SDK d3d11.h 中 ID3D11DeviceContext 声明顺序）----
        constexpr int kVtMap = 14;
        constexpr int kVtUnmap = 15;

        using MapFn = HRESULT(STDMETHODCALLTYPE*)(
            ID3D11DeviceContext*, ID3D11Resource*, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE*);
        using UnmapFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*);

        MapFn RealMap = nullptr;
        UnmapFn RealUnmap = nullptr;

        // ---- 捕获状态（Present/Unmap 同在渲染线程，互斥量开销可忽略）----
        std::mutex g_mutex;
        Matrix44 g_view{};
        Matrix44 g_proj{};
        bool g_hasView = false;
        bool g_hasProj = false;
        std::uint32_t g_countViews = 0;
        std::uint32_t g_countProjs = 0;
        std::uint32_t g_countBuffers = 0;

        constexpr std::size_t kMaxScanBytes = 8192; // 只扫小的常量缓冲

        // Map→Unmap 的配对记录（同线程串行，thread_local 单条足够）
        struct Pending
        {
            const void* data{ nullptr };
            std::size_t bytes{ 0 };
        };
        thread_local Pending t_pending;

        // ---- 指纹工具（m[row][col] = f[row*4+col]）----
        bool AllFinite(const float* f) noexcept
        {
            for (int i = 0; i < 16; ++i)
                if (!std::isfinite(f[i])) return false;
            return true;
        }

        bool RowsOrthonormal(const float* f) noexcept
        {
            for (int r = 0; r < 3; ++r)
            {
                const float len = std::sqrt(f[r * 4] * f[r * 4] + f[r * 4 + 1] * f[r * 4 + 1]
                                            + f[r * 4 + 2] * f[r * 4 + 2]);
                if (std::fabs(len - 1.0f) > 0.02f) return false;
            }
            const float d01 = f[0] * f[4] + f[1] * f[5] + f[2] * f[6];
            const float d02 = f[0] * f[8] + f[1] * f[9] + f[2] * f[10];
            const float d12 = f[4] * f[8] + f[5] * f[9] + f[6] * f[10];
            return std::fabs(d01) < 0.02f && std::fabs(d02) < 0.02f && std::fabs(d12) < 0.02f;
        }

        // 行向量约定投影：xs/ys 在对角线，m[2][3]=±1，m[3][2]≠0
        bool LooksLikeProjRow(const float* f) noexcept
        {
            return AllFinite(f)
                && f[0] > 0.2f && f[0] < 8.0f
                && f[5] > 0.2f && f[5] < 8.0f
                && f[1] == 0.0f && f[2] == 0.0f && f[3] == 0.0f
                && f[4] == 0.0f && f[6] == 0.0f && f[7] == 0.0f
                && f[8] == 0.0f && f[9] == 0.0f
                && (f[11] == 1.0f || f[11] == -1.0f)
                && f[12] == 0.0f && f[13] == 0.0f && f[15] == 0.0f
                && std::fabs(f[10] - 1.0f) > 1e-4f
                && std::fabs(f[14]) > 1e-3f;
        }

        // 列向量约定的投影（即行向量形式的转置）：m[3][2]=±1，m[2][3]≠0
        bool LooksLikeProjCol(const float* f) noexcept
        {
            return AllFinite(f)
                && f[0] > 0.2f && f[0] < 8.0f
                && f[5] > 0.2f && f[5] < 8.0f
                && f[1] == 0.0f && f[2] == 0.0f && f[3] == 0.0f
                && f[4] == 0.0f && f[6] == 0.0f && f[7] == 0.0f
                && f[8] == 0.0f && f[9] == 0.0f
                && (f[14] == 1.0f || f[14] == -1.0f)
                && f[12] == 0.0f && f[13] == 0.0f && f[15] == 0.0f
                && std::fabs(f[10] - 1.0f) > 1e-4f
                && std::fabs(f[11]) > 1e-3f;
        }

        void StoreProjection(const float* f) noexcept
        {
            std::lock_guard lock(g_mutex);
            bool col = !(f[11] == 1.0f || f[11] == -1.0f); // f[14]==±1 → 列形式
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    g_proj.m[r][c] = col ? f[c * 4 + r] : f[r * 4 + c];
            g_hasProj = true;
            ++g_countProjs;
        }

        void StoreView(const float* f) noexcept
        {
            // 行向量形式：col3 全 0（f[3]=f[7]=f[11]=0），row3 = 平移
            // 列向量形式：row3 = (0,0,0,1)，平移在 col3 → 转置存储
            bool col = (f[12] == 0.0f && f[13] == 0.0f && f[14] == 0.0f)
                       && !(f[3] == 0.0f && f[7] == 0.0f && f[11] == 0.0f);
            std::lock_guard lock(g_mutex);
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    g_view.m[r][c] = col ? f[c * 4 + r] : f[r * 4 + c];
            g_hasView = true;
            ++g_countViews;
        }

        // 甄别"逐帧变化的世界矩阵"：同一缓冲地址上内容发生变化的正交矩阵
        // 才可能是相机视图（静态物体的世界矩阵不变）
        std::unordered_map<const void*, std::array<float, 16>> g_lastSeen;

        bool AcceptDynamicView(const float* m, const void* key) noexcept
        {
            const float translation[3] = { m[12], m[13], m[14] };
            if (g_entityCache.IsNearAnyEntity(translation, 1.0f))
                return false; // 平移恰在实体坐标上 → 物体世界矩阵

            auto& slot = g_lastSeen[key];
            if (slot.empty())
            {
                std::copy(m, m + 16, slot.begin());
                return false; // 首见不判定
            }
            const bool changed = std::memcmp(slot.data(), m, 64) != 0;
            std::copy(m, m + 16, slot.begin());
            if (g_lastSeen.size() > 256)
                g_lastSeen.clear();
            return changed;
        }

        void ScanBuffer(const void* data, std::size_t bytes, const void* baseAddr) noexcept
        {
            const auto* f = static_cast<const float*>(data);
            const std::size_t nf = (std::min)(bytes, kMaxScanBytes) / sizeof(float);
            if (nf < 16)
                return;

            {
                std::lock_guard lock(g_mutex);
                ++g_countBuffers;
            }

            // 第一遍：投影
            bool projFound = false;
            for (std::size_t i = 0; i + 16 <= nf; i += 4)
            {
                const float* m = f + i;
                if (!AllFinite(m))
                    continue;
                if (LooksLikeProjRow(m) || LooksLikeProjCol(m))
                {
                    StoreProjection(m);
                    projFound = true;
                    break;
                }
            }

            // 第二遍：视图。与投影同缓冲的正交矩阵几乎必是相机视图；
            // 否则要求"逐帧变化且平移不在实体坐标上"
            for (std::size_t i = 0; i + 16 <= nf; i += 4)
            {
                const float* m = f + i;
                if (!AllFinite(m) || !RowsOrthonormal(m) || m[15] != 1.0f)
                    continue;
                constexpr float kIdentity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
                if (std::memcmp(m, kIdentity, sizeof(kIdentity)) == 0)
                    continue;
                if (projFound || AcceptDynamicView(m, static_cast<const char*>(baseAddr) + i * 4))
                {
                    StoreView(m);
                    break;
                }
            }
        }

        HRESULT STDMETHODCALLTYPE HookMap(ID3D11DeviceContext* ctx, ID3D11Resource* res,
                                          D3D11_MAP type, UINT flags,
                                          D3D11_MAPPED_SUBRESOURCE* out)
        {
            const HRESULT hr = RealMap(ctx, res, type, flags, out);
            if (SUCCEEDED(hr) && out && out->pData
                && (type == D3D11_MAP_WRITE_DISCARD || type == D3D11_MAP_WRITE))
            {
                D3D11_BUFFER_DESC desc{};
                ID3D11Buffer* buf = nullptr;
                if (SUCCEEDED(res->QueryInterface(__uuidof(ID3D11Buffer),
                                                  reinterpret_cast<void**>(&buf))))
                {
                    buf->GetDesc(&desc);
                    buf->Release();
                    t_pending = Pending{ out->pData, desc.ByteWidth };
                }
            }
            return hr;
        }

        void STDMETHODCALLTYPE HookUnmap(ID3D11DeviceContext* ctx, ID3D11Resource* res)
        {
            if (t_pending.data)
            {
                ScanBuffer(t_pending.data, t_pending.bytes, t_pending.data);
                t_pending = Pending{};
            }
            RealUnmap(ctx, res);
        }
    } // namespace

    CaptureCounters GetCaptureCounters() noexcept
    {
        std::lock_guard lock(g_mutex);
        return CaptureCounters{ g_countBuffers, g_countViews, g_countProjs };
    }

    std::optional<Matrix44> GetCapturedView() noexcept
    {
        std::lock_guard lock(g_mutex);
        if (!g_hasView)
            return std::nullopt;
        return g_view;
    }

    std::optional<Matrix44> GetCapturedProj() noexcept
    {
        std::lock_guard lock(g_mutex);
        if (!g_hasProj)
            return std::nullopt;
        return g_proj;
    }

    bool InstallContextHooks(ID3D11DeviceContext* context) noexcept
    {
        if (!context)
            return false;
        void** vtbl = *reinterpret_cast<void***>(context);
        if (MH_CreateHook(vtbl[kVtMap], reinterpret_cast<void*>(&HookMap),
                          reinterpret_cast<void**>(&RealMap)) != MH_OK)
            return false;
        if (MH_CreateHook(vtbl[kVtUnmap], reinterpret_cast<void*>(&HookUnmap),
                          reinterpret_cast<void**>(&RealUnmap)) != MH_OK)
            return false;
        return MH_EnableHook(vtbl[kVtMap]) == MH_OK
            && MH_EnableHook(vtbl[kVtUnmap]) == MH_OK;
    }
}
