// imgui_style.h
// 深色科技风主题 + 通用视觉特效组件（移植自 PalworldInternal）
#pragma once
#include <imgui.h>

// ============================================================================
// 主题色板（Dark Sci-Fi）
// ============================================================================
namespace SciFi
{
    inline const ImVec4 BgBase     = ImVec4(0.04f, 0.055f, 0.09f, 1.0f); // #0a0e17
    inline const ImVec4 PanelBg    = ImVec4(0.067f, 0.094f, 0.153f, 1.0f); // #111827
    inline const ImVec4 CardBg     = ImVec4(0.102f, 0.137f, 0.196f, 1.0f); // #1a2332
    inline const ImVec4 Accent     = ImVec4(0.00f, 0.83f, 1.00f, 1.0f);    // #00d4ff 科技蓝
    inline const ImVec4 AccentDim  = ImVec4(0.00f, 0.83f, 1.00f, 0.18f);
    inline const ImVec4 Energy     = ImVec4(1.00f, 0.42f, 0.21f, 1.0f);    // #ff6b35 能量橙
    inline const ImVec4 Success    = ImVec4(0.06f, 0.72f, 0.51f, 1.0f);    // #10b981 翠绿
    inline const ImVec4 Purple     = ImVec4(0.62f, 0.35f, 1.00f, 1.0f);    // 紫（次要高亮）
    inline const ImVec4 TextMain   = ImVec4(0.89f, 0.91f, 0.94f, 1.0f);    // #e2e8f0
    inline const ImVec4 TextDim    = ImVec4(0.58f, 0.64f, 0.72f, 1.0f);    // #94a3b8
    inline const ImVec4 Border     = ImVec4(0.165f, 0.227f, 0.29f, 1.0f);  // #2a3a4a
    inline const ImVec4 Danger     = ImVec4(0.90f, 0.24f, 0.26f, 1.0f);    // 退出红色

    // 颜色助手
    inline ImU32 C(const ImVec4& c)        { return ImGui::GetColorU32(c); }
    inline ImVec4 WithA(const ImVec4& c, float a) { return ImVec4(c.x, c.y, c.z, a); }
    inline ImU32 Alpha(const ImVec4& c, float a)  { return ImGui::GetColorU32(WithA(c, a)); }
    inline ImU32 RGBA(int r, int g, int b, int a) { return IM_COL32(r, g, b, a); }
}

// ============================================================================
// 基础主题
// ============================================================================
void SetupImGuiStyle();

// 带右侧延伸线的分隔标题（供各 Tab 复用）
void ColoredSeparatorText(const char* text, ImVec4 textColor, float thickness = 1.5f, float padding = 6.0f);

// ============================================================================
// 特效 / 组件
// ============================================================================
void SciFiDrawGradientRect(ImDrawList* dl, const ImVec2& p_min, const ImVec2& p_max,
                           const ImVec4& cA, const ImVec4& cB, float rounding);
bool SciFiPanel(const char* id, const ImVec2& size, bool hoverable = false);
void SciFiPanelTitle(const char* text, const ImVec4& dotColor);
void SciFiGradientText(const char* text, float size, const ImVec4& from, const ImVec4& to);
void SciFiBadge(const char* text, const ImVec4& bg, const ImVec4& fg);
float SciFiBreathingDot(float cx, float cy, float radius, const ImVec4& color, float period = 2.0f);
bool SciFiNavItem(int idx, const char* icon, const char* label, bool selected, float width);
bool SciFiGradientButton(const char* label, const ImVec2& size, const ImVec4& from, const ImVec4& to);
bool SciFiShimmerButton(const char* label, const ImVec2& size, const ImVec4& from, const ImVec4& to);
bool SciFiOutlineButton(const char* label, const ImVec2& size, const ImVec4& color);
bool SciFiQuantityStepper(const char* id, int& value, int min, int max, int step = 1);
void SciFiTypeTag(const char* text, const ImVec4& color);
void SciFiRenderParticles(ImDrawList* dl, const ImVec2& origin, const ImVec2& size);
void SciFiPushFrame(const ImVec4& bg, const ImVec4& border, float rounding);
void SciFiPopFrame();
