#include "Esp.h"

#include "Capture.h"

#include <imgui.h>

#include <cmath>
#include <cstdio>
#include <algorithm>

namespace nier::esp
{
    using game::f32;

    namespace
    {
        // 视图空间坐标（世界 → 视图，行向量约定：v' = v * M）
        struct ViewState
        {
            f32 x{ 0.0f };
            f32 y{ 0.0f };
            f32 z{ 0.0f };
        };

        ViewState ToViewSpace(const game::Vec3& world, const game::Matrix44& mat)
        {
            const auto& m = mat.m;
            return {
                world.x * m[0][0] + world.y * m[1][0] + world.z * m[2][0] + m[3][0],
                world.x * m[0][1] + world.y * m[1][1] + world.z * m[2][1] + m[3][1],
                world.x * m[0][2] + world.y * m[1][2] + world.z * m[2][2] + m[3][2],
            };
        }

        ImU32 KindColor(game::EntityKind kind)
        {
            switch (kind)
            {
            case game::EntityKind::Player: return IM_COL32( 90, 200, 255, 255); // 蓝
            case game::EntityKind::Enemy:  return IM_COL32(255,  80,  80, 255); // 红
            default:                       return IM_COL32(200, 200, 200, 255); // 灰
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

        f32 KindHeight(game::EntityKind kind)
        {
            switch (kind)
            {
            case game::EntityKind::Player: return 1.6f;
            case game::EntityKind::Enemy:  return 1.6f;
            default:                       return 1.0f;
            }
        }

        // 游戏真实投影矩阵（行向量约定）：clip = view * Proj
        // 透视矩阵只在 x/y/z 对角线和 m[2][3]、m[3][2] 上有值，
        // clip.w = vz * P[2][3]，可见目标恒为正（LH/RH 自动适配）
        void ProjectToScreen(const ViewState& view, const game::Matrix44& proj,
                             ImVec2 displaySize, f32& outDepth, ImVec2& outScreen)
        {
            const auto& p = proj.m;
            outDepth = 0.0f;

            const f32 clipW = view.z * p[2][3];
            if (clipW < 0.05f)
                return; // 相机身后或退化：outDepth=0 作为失败标记

            const f32 ndcX = view.x * p[0][0] / clipW;
            const f32 ndcY = view.y * p[1][1] / clipW;
            if (ndcX < -1.05f || ndcX > 1.05f || ndcY < -1.05f || ndcY > 1.05f)
                return;

            outDepth = clipW;
            outScreen = ImVec2{ (ndcX * 0.5f + 0.5f) * displaySize.x,
                                (1.0f - (ndcY * 0.5f + 0.5f)) * displaySize.y };
        }

        void DrawEntity(ImDrawList* drawList, const game::EntitySnapshot& snapshot,
                        ImVec2 screen, f32 depth, f32 boxHeightPx)
        {
            const ImU32 color = KindColor(snapshot.kind);

            const f32 boxH = std::clamp(boxHeightPx, 8.0f, 400.0f);
            const f32 boxW = boxH * 0.45f;
            const ImVec2 minS{ screen.x - boxW * 0.5f, screen.y - boxH };
            const ImVec2 maxS{ screen.x + boxW * 0.5f, screen.y };

            if (config.showBox)
                drawList->AddRect(minS, maxS, color, 2.0f, 0, 1.4f);

            char label[32];
            if (config.showDistance)
                std::snprintf(label, sizeof(label), "%s %.0fm",
                              KindName(snapshot.kind), depth);
            else
                std::snprintf(label, sizeof(label), "%s", KindName(snapshot.kind));
            drawList->AddText(ImVec2(minS.x, minS.y - ImGui::GetTextLineHeight() - 2.0f),
                              color, label);

            if (config.showSnapline)
            {
                const ImVec2& display = ImGui::GetIO().DisplaySize;
                drawList->AddLine(ImVec2(display.x * 0.5f, display.y), screen, color, 1.0f);
            }
        }
    } // namespace

    void Render(game::EntityCache& cache)
    {
        if (!config.enabled)
            return;

        const auto view = game::GetCapturedView();
        const auto proj = game::GetCapturedProj();
        const auto counters = game::GetCaptureCounters();

        const auto entities = cache.Refresh();

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
            return;

        if (config.showDebug)
        {
            // 调试行按枚举顺序如实显示：U=Unknown P=Player E=Enemy
            int perKind[3]{};
            for (const auto& s : entities)
                ++perKind[static_cast<int>(s.kind)];

            char debug[128];
            std::snprintf(debug, sizeof(debug),
                          "ESP: U:%d P:%d E:%d | hook V:%u Pr:%u bufs:%u",
                          perKind[0], perKind[1], perKind[2],
                          counters.views, counters.projs, counters.buffers);
            drawList->AddText(ImVec2(12.0f, 40.0f), IM_COL32(120, 255, 120, 255), debug);
        }

        if (!view || !proj)
            return; // 尚未截获视图/投影矩阵，宁可不画

        const f32 maxDistanceSq = config.maxDistance * config.maxDistance;

        for (const auto& snapshot : entities)
        {
            const ViewState vs = ToViewSpace(snapshot.pos, view.value());

            const f32 distSq = vs.x * vs.x + vs.y * vs.y + vs.z * vs.z;
            if (distSq > maxDistanceSq)
                continue;

            ImVec2 screen{};
            f32 depth = 0.0f;
            ProjectToScreen(vs, proj.value(), displaySize, depth, screen);
            if (depth <= 0.0f)
                continue;

            // 方框高度 = 实体高度 × y缩放 ÷ 深度 × 半屏
            const f32 boxHeightPx = KindHeight(snapshot.kind)
                                    * proj->m[1][1] / depth * 0.5f * displaySize.y;
            DrawEntity(drawList, snapshot, screen, std::sqrt(distSq), boxHeightPx);
        }
    }
}
