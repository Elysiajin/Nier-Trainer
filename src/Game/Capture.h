#pragma once
// ============================================================
// DX11 常量缓冲截获：hook ID3D11DeviceContext::Map/Unmap，
// 对 CPU→GPU 上传的数据做矩阵指纹扫描，捕获每帧最新的
// 视图矩阵与投影矩阵（游戏不把它们放在静态全局区，
// 只能从上传通道截获）。
// ============================================================

#include "Structs.h"

#include <optional>

struct ID3D11DeviceContext;

namespace nier::game
{
    struct CaptureCounters
    {
        std::uint32_t buffers{ 0 }; // 已扫描的上传缓冲数
        std::uint32_t views{ 0 };   // 视图矩阵命中次数
        std::uint32_t projs{ 0 };   // 投影矩阵命中次数
    };

    [[nodiscard]] CaptureCounters GetCaptureCounters() noexcept;
    [[nodiscard]] std::optional<Matrix44> GetCapturedView() noexcept; // 统一为行向量约定
    [[nodiscard]] std::optional<Matrix44> GetCapturedProj() noexcept; // 统一为行向量约定

    // 对 immediate context 的 Map/Unmap 挂 MinHook（vtable[14]/[15]）。
    // ⚠ 实验功能，默认不启用；运行时由菜单开关触发。
    [[nodiscard]] bool InstallContextHooks(ID3D11DeviceContext* context) noexcept;
    void RemoveContextHooks() noexcept;
}
