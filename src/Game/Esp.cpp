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
        // 视图空间坐标（世界 → 视图，行向量约定：v' = v * M）
        struct ViewState
        {
            f32 x{ 0.0f };
            f32 y{ 0.0f };
            f32 z{ 0.0f };   // 前向深度（符号取决于引擎左右手约定）
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

        f32 FovScale(f32 fovDegrees)
        {
            return 1.0f / std::tan(fovDegrees * 3.14159265f / 360.0f); // 1/tan(fov/2)
        }

        // 用自建透视投影把视图空间投到屏幕。
        // 引擎左右手约定未知：以"多数实体 z<0 则右手"逐帧自动判定，
        // 判错时表现为画面左右镜像，调 rhOverride 可强制。
        std::optional<ImVec2> Project(const ViewState& view, bool rhMode,
                                      f32 fovScale, f32 aspect, ImVec2 displaySize)
        {
            const f32 depth = rhMode ? -view.z : view.z;
            if (depth < 0.05f)
                return std::nullopt; // 相机身后

            const f32 xScale = fovScale / aspect; // 水平
            const f32 yScale = fovScale;          // 垂直

            const f32 ndcX = view.x * xScale / depth;
            const f32 ndcY = view.y * yScale / depth;
            if (ndcX < -1.05f || ndcX > 1.05f || ndcY < -1.05f || ndcY > 1.05f)
                return std::nullopt;

            return ImVec2{ (ndcX * 0.5f + 0.5f) * displaySize.x,
                           (1.0f - (ndcY * 0.5f + 0.5f)) * displaySize.y };
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

        const auto matrix = cache.TryFetchCameraMatrix();
        if (!matrix)
            return;

        const auto entities = cache.Refresh();

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
            return;

        // ---- 逐帧自动判定左右手约定：多数实体 z<0 → 右手 ----
        int zNegative = 0;
        for (const auto& snapshot : entities)
        {
            const f32 z = ToViewSpace(snapshot.pos, matrix.value()).z;
            (z < 0.0f) ? ++zNegative : --zNegative;
        }
        const bool rhMode = zNegative > 0;

        const f32 fovScale = FovScale(config.fovDegrees);
        const f32 aspect = displaySize.x / displaySize.y;
        const f32 maxDistanceSq = config.maxDistance * config.maxDistance;

        if (config.showDebug)
        {
            int perKind[3]{};
            for (const auto& s : entities)
                ++perKind[static_cast<int>(s.kind)];
            char debug[128];
            std::snprintf(debug, sizeof(debug),
                          "ESP: P:%d E:%d U:%d | %s | fov %.0f",
                          perKind[0], perKind[1], perKind[2],
                          rhMode ? "RH" : "LH", config.fovDegrees);
            drawList->AddText(ImVec2(12.0f, 40.0f), IM_COL32(120, 255, 120, 255), debug);
        }

        for (const auto& snapshot : entities)
        {
            const ViewState view = ToViewSpace(snapshot.pos, matrix.value());

            const f32 distSq = view.x * view.x + view.y * view.y + view.z * view.z;
            if (distSq > maxDistanceSq)
                continue;

            // 方框高度：实体真实高度 × 投影 ÷ 深度 × 半屏
            const f32 depth = std::fabs(view.z);
            const f32 boxHeightPx = KindHeight(snapshot.kind) * fovScale / depth
                                    * 0.5f * displaySize.y;

            auto screen = Project(view, rhMode, fovScale, aspect, displaySize);
            if (screen)
                DrawEntity(drawList, snapshot, screen.value(), std::sqrt(distSq), boxHeightPx);
        }
    }
}
