#include "Esp.h"

#include <imgui.h>

#include <cmath>
#include <cstdio>
#include <algorithm>

namespace nier::esp
{
    using game::f32;
    namespace
    {
        struct ScreenPos
        {
            ImVec2 pos;
            f32 w{ 0.0f }; // 裁剪空间 w，用于按距离调透明度
        };

        // ----------------------------------------------------
        // 世界到屏幕（行向量约定：clip = pos * M）
        // ⚠ 当前矩阵源是 cCameraGame+0x10 候选（可能是视图矩阵而非
        //   ViewProj），语义待运行时确认；确认后仅需替换矩阵来源。
        // ----------------------------------------------------
        std::optional<ScreenPos> WorldToScreen(const game::Vec3& world, const game::Matrix44& mat,
                                               ImVec2 displaySize)
        {
            const auto& m = mat.m;
            const f32 clipX = world.x * m[0][0] + world.y * m[1][0] + world.z * m[2][0] + m[3][0];
            const f32 clipY = world.x * m[0][1] + world.y * m[1][1] + world.z * m[2][1] + m[3][1];
            const f32 clipW = world.x * m[0][3] + world.y * m[1][3] + world.z * m[2][3] + m[3][3];

            if (clipW < 0.001f || !std::isfinite(clipW))
                return std::nullopt; // 在相机身后或退化

            const ImVec2 ndc{ clipX / clipW, clipY / clipW };
            if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f)
                return std::nullopt; // 屏幕外留 20% 余量避免边框抖动

            ScreenPos out;
            out.pos.x = (ndc.x * 0.5f + 0.5f) * displaySize.x;
            out.pos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;
            out.w = clipW;
            return out;
        }

        ImU32 KindColor(game::EntityKind kind)
        {
            switch (kind)
            {
            case game::EntityKind::Player: return IM_COL32( 90, 200, 255, 255); // 蓝
            case game::EntityKind::Enemy:  return IM_COL32(255,  80,  80, 255); // 红
            default:                       return IM_COL32(200, 200, 200, 255);
            }
        }

        const char* KindName(game::EntityKind kind)
        {
            switch (kind)
            {
            case game::EntityKind::Player: return "PL";
            case game::EntityKind::Enemy:  return "EM";
            default:                       return "??";
            }
        }

        void DrawEntity(ImDrawList* drawList, const game::EntitySnapshot& snapshot,
                        const ScreenPos& screen)
        {
            const ImU32 color = KindColor(snapshot.kind);

            // 以裁剪 w 估算投影半径（经验系数，运行时标定）
            const f32 boxH = std::clamp(2200.0f / screen.w, 10.0f, 240.0f);
            const f32 boxW = boxH * 0.45f;
            const ImVec2 minS{ screen.pos.x - boxW * 0.5f, screen.pos.y - boxH };
            const ImVec2 maxS{ screen.pos.x + boxW * 0.5f, screen.pos.y };

            if (config.showBox)
                drawList->AddRect(minS, maxS, color, 2.0f, 0, 1.4f);

            char label[32];
            if (config.showDistance)
            {
                const f32 distance = screen.w; // 裁剪 w 单调于深度，先作距离近似
                std::snprintf(label, sizeof(label), "%s %.0fm", KindName(snapshot.kind), distance);
            }
            else
            {
                std::snprintf(label, sizeof(label), "%s", KindName(snapshot.kind));
            }
            drawList->AddText(ImVec2(minS.x, minS.y - ImGui::GetTextLineHeight() - 2.0f),
                              color, label);

            if (config.showSnapline)
            {
                const ImVec2& display = ImGui::GetIO().DisplaySize;
                drawList->AddLine(ImVec2(display.x * 0.5f, display.y), screen.pos, color, 1.0f);
            }
        }
    } // namespace

    void Render(game::EntityCache& cache)
    {
        if (!config.enabled)
            return;

        const auto matrix = cache.TryFetchCameraMatrix();
        if (!matrix)
            return;

        const auto entities = cache.Refresh();
        if (entities.empty())
            return;

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        const f32 maxDistanceSq = config.maxDistance * config.maxDistance;

        for (const auto& snapshot : entities)
        {
            // 距离粗筛：裁剪前用相机矩阵 row3 近似相机位置
            const auto& m = matrix->m;
            const f32 dx = snapshot.pos.x - m[3][0];
            const f32 dy = snapshot.pos.y - m[3][1];
            const f32 dz = snapshot.pos.z - m[3][2];
            if (dx * dx + dy * dy + dz * dz > maxDistanceSq)
                continue;

            const auto screen = WorldToScreen(snapshot.pos, matrix.value(), displaySize);
            if (screen)
                DrawEntity(drawList, snapshot, screen.value());
        }
    }
}
