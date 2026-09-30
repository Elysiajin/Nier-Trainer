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
        f32 maxDistance{ 500.0f };   // 米（按引擎单位≈米假设，待运行时标定）
    };

    // 全局配置（菜单直接绑定）
    inline Config config;

    // 在 ImGui 帧内调用（NewFrame 之后、Render 之前）
    void Render(game::EntityCache& cache);
}
