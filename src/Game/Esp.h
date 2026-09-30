#pragma once
// ============================================================
// ESP 渲染层：实体快照 → 世界到屏幕 → ImGui 背景层绘制
// 每帧由 D3D11Window::Overlay 在 NewFrame 之后调用 Render()。
// ============================================================

#include "GameData.h"

namespace nier::esp
{
    using game::f32;

    struct Config
    {
        bool enabled{ false };
        bool showBox{ true };
        bool showDistance{ true };
        bool showSnapline{ false };
        bool showDebug{ true };      // 左上角调试行（验证完关掉）
        f32 fovDegrees{ 55.0f };     // 垂直 FOV，调到方框贴合实体为准
        f32 maxDistance{ 500.0f };   // 米（真实距离，视图空间求模）
    };

    // 全局配置（菜单直接绑定）
    inline Config config;

    // 在 ImGui 帧内调用（NewFrame 之后、Render 之前）
    void Render(game::EntityCache& cache);
}
