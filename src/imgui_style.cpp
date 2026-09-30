// 深色科技风主题 + 通用视觉特效组件（移植自 PalworldInternal）
#include <cmath>
#include <vector>
#include <random>
#include <string>
#include <unordered_map>
#include <cstdio>
#include <imgui_internal.h>
#include "imgui_style.h"
#include "fonts.h"

void DrawRadialGlow(ImDrawList* dl, const ImVec2& center, float radius, ImU32 color);

// ============================================================================
// 带右侧延伸线的分隔标题
// ============================================================================
void ColoredSeparatorText(const char* text, ImVec4 textColor, float thickness, float padding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return;

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	float textWidth = ImGui::CalcTextSize(text).x;
	float textHeight = ImGui::GetTextLineHeight();

	ImVec2 textPos = cursorPos;
	textPos.y += -2.0f;

	float lineStartX = textPos.x + textWidth + 8.0f;
	float lineEndX = cursorPos.x + ImGui::GetContentRegionAvail().x;
	float lineY = textPos.y + textHeight * 0.5f;

	ImGui::Dummy(ImVec2(0.0f, textHeight + -2.0f + 2.0f));

	window->DrawList->AddLine(
		ImVec2(lineStartX, lineY),
		ImVec2(lineEndX, lineY),
		ImGui::GetColorU32(ImGuiCol_Separator),
		thickness
	);

	ImGui::SetCursorScreenPos(textPos);
	ImGui::PushStyleColor(ImGuiCol_Text, textColor);
	ImGui::TextUnformatted(text);
	ImGui::PopStyleColor();
}

// ============================================================================
// 基础主题（深色科技风）
// ============================================================================
void SetupImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.45f;
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.WindowRounding = 16.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(320, 240);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding = 10.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 10.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(12.0f, 6.0f);
    style.FrameRounding = 8.0f;
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.CellPadding = ImVec2(10.0f, 6.0f);
    style.IndentSpacing = 16.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabMinSize = 6.0f;
    style.GrabRounding = 10.0f;
    style.TabRounding = 8.0f;
    style.TabBorderSize = 0.0f;
    style.TabBarBorderSize = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    using namespace SciFi;
    ImGuiStyle& s = style;
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = TextMain;
    c[ImGuiCol_TextDisabled]         = TextDim;
    c[ImGuiCol_WindowBg]             = BgBase;
    c[ImGuiCol_ChildBg]              = CardBg;
    c[ImGuiCol_PopupBg]              = PanelBg;
    c[ImGuiCol_Border]               = Border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TitleBg]              = PanelBg;
    c[ImGuiCol_TitleBgActive]        = PanelBg;
    c[ImGuiCol_TitleBgCollapsed]     = PanelBg;
    c[ImGuiCol_MenuBarBg]            = PanelBg;
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.03f, 0.05f, 0.08f, 1.0f);
    c[ImGuiCol_ScrollbarGrab]        = WithA(Accent, 0.35f);
    c[ImGuiCol_ScrollbarGrabHovered] = WithA(Accent, 0.65f);
    c[ImGuiCol_ScrollbarGrabActive]  = Accent;
    c[ImGuiCol_CheckMark]            = Accent;
    c[ImGuiCol_SliderGrab]           = Accent;
    c[ImGuiCol_SliderGrabActive]     = WithA(Accent, 0.8f);
    c[ImGuiCol_Button]               = CardBg;
    c[ImGuiCol_ButtonHovered]        = WithA(Accent, 0.22f);
    c[ImGuiCol_ButtonActive]         = WithA(Accent, 0.40f);
    c[ImGuiCol_Header]               = WithA(Accent, 0.15f);
    c[ImGuiCol_HeaderHovered]        = WithA(Accent, 0.30f);
    c[ImGuiCol_HeaderActive]         = WithA(Accent, 0.45f);
    c[ImGuiCol_Separator]            = Border;
    c[ImGuiCol_SeparatorHovered]     = Accent;
    c[ImGuiCol_SeparatorActive]      = Accent;
    c[ImGuiCol_ResizeGrip]           = WithA(Accent, 0.25f);
    c[ImGuiCol_ResizeGripHovered]    = WithA(Accent, 0.55f);
    c[ImGuiCol_ResizeGripActive]     = Accent;
    c[ImGuiCol_Tab]                  = CardBg;
    c[ImGuiCol_TabHovered]           = WithA(Accent, 0.25f);
    c[ImGuiCol_TabActive]            = WithA(Accent, 0.45f);
    c[ImGuiCol_TabUnfocused]         = CardBg;
    c[ImGuiCol_TabUnfocusedActive]   = WithA(Accent, 0.35f);
    c[ImGuiCol_PlotLines]            = Accent;
    c[ImGuiCol_PlotLinesHovered]     = WithA(Accent, 0.8f);
    c[ImGuiCol_PlotHistogram]        = Accent;
    c[ImGuiCol_PlotHistogramHovered] = WithA(Accent, 0.8f);
    c[ImGuiCol_TableHeaderBg]        = WithA(Accent, 0.12f);
    c[ImGuiCol_TableBorderStrong]    = Border;
    c[ImGuiCol_TableBorderLight]     = WithA(Border, 0.6f);
    c[ImGuiCol_TableRowBg]           = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt]        = WithA(Accent, 0.04f);
    c[ImGuiCol_TextSelectedBg]       = WithA(Accent, 0.35f);
    c[ImGuiCol_DragDropTarget]       = Accent;
    c[ImGuiCol_NavHighlight]         = Accent;
    c[ImGuiCol_NavWindowingHighlight]= WithA(Accent, 0.7f);
    c[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);
    c[ImGuiCol_FrameBg]              = CardBg;
    c[ImGuiCol_FrameBgHovered]       = WithA(Accent, 0.15f);
    c[ImGuiCol_FrameBgActive]        = WithA(Accent, 0.30f);
}

// ============================================================================
// 工具：水平渐变圆角矩形
// ============================================================================
void SciFiDrawGradientRect(ImDrawList* dl, const ImVec2& p_min, const ImVec2& p_max,
                           const ImVec4& cA, const ImVec4& cB, float rounding)
{
    rounding = (rounding > (p_max.y - p_min.y) * 0.5f) ? (p_max.y - p_min.y) * 0.5f : rounding;
    dl->AddRectFilledMultiColor(p_min, p_max,
        SciFi::C(cA), SciFi::C(cB), SciFi::C(cB), SciFi::C(cA));
    dl->AddRectFilled(ImVec2(p_min.x + rounding, p_min.y), ImVec2(p_max.x, p_min.y + rounding), SciFi::C(cB));
    dl->AddRectFilled(ImVec2(p_min.x + rounding, p_max.y - rounding), ImVec2(p_max.x, p_max.y), SciFi::C(cB));
    dl->AddRectFilled(ImVec2(p_min.x, p_min.y + rounding), ImVec2(p_min.x + rounding, p_max.y - rounding), SciFi::C(cA));
    dl->AddRectFilled(ImVec2(p_max.x - rounding, p_min.y + rounding), ImVec2(p_max.x, p_max.y - rounding), SciFi::C(cB));
}

// ============================================================================
// 面板卡片
// ============================================================================
bool SciFiPanel(const char* id, const ImVec2& size, bool hoverable)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const float rounding = 12.0f;
    ImDrawList* dl = window->DrawList;

    bool hovered = hoverable && ImGui::IsMouseHoveringRect(bb.Min, bb.Max);

    ImVec4 borderCol = hovered ? SciFi::WithA(SciFi::Accent, 0.7f) : SciFi::Border;
    if (hovered)
    {
        ImU32 glow = SciFi::Alpha(SciFi::Accent, 0.12f);
        dl->AddRect(bb.Min - ImVec2(4, 4), bb.Max + ImVec2(4, 4), glow, rounding + 4.0f, 0, 6.0f);
    }

    dl->AddRectFilled(bb.Min, bb.Max, SciFi::C(SciFi::CardBg), rounding);
    dl->AddRect(bb.Min, bb.Max, SciFi::C(borderCol), rounding, 0, 1.0f);

    ImU32 topLine = SciFi::Alpha(SciFi::Accent, hovered ? 0.35f : 0.15f);
    dl->AddLine(ImVec2(bb.Min.x + 14, bb.Min.y + 1), ImVec2(bb.Max.x - 14, bb.Min.y + 1), topLine, 1.0f);

    ImGui::ItemSize(bb.GetSize());
    ImGui::ItemAdd(bb, ImGui::GetID(id));
    return hovered;
}

// ============================================================================
// 面板标题（状态圆点 + 文字 + 延伸线）
// ============================================================================
void SciFiPanelTitle(const char* text, const ImVec4& dotColor)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float lineHeight = ImGui::GetTextLineHeight();

    SciFiBreathingDot(cursor.x + 5.0f, cursor.y + lineHeight * 0.5f, 3.0f, dotColor, 2.4f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, SciFi::TextMain);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();

    float textWidth = ImGui::CalcTextSize(text).x;
    float lineStartX = cursor.x + 16.0f + textWidth + 10.0f;
    float lineEndX = cursor.x + ImGui::GetContentRegionAvail().x;
    float lineY = cursor.y + lineHeight * 0.5f;
    ImU32 lineCol = SciFi::Alpha(SciFi::Accent, 0.25f);
    dl->AddLine(ImVec2(lineStartX, lineY), ImVec2(lineEndX, lineY), lineCol, 1.0f);
}

// ============================================================================
// 渐变文字（逐字渐变，使用 clip rect）
// ============================================================================
void SciFiGradientText(const char* text, float size, const ImVec4& from, const ImVec4& to)
{
    ImFont* font = size >= 20.0f ? HeaderFont : (size > 0 ? HeaderFont : TextFont);
    if (!font) font = ImGui::GetFont();

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float x = cursor.x;
    const char* p = text;
    while (*p)
    {
        unsigned int codepoint = 0;
        int n = ImTextCharFromUtf8(&codepoint, p, p + 4);
        if (n == 0) break;
        ImFontGlyph const* glyph = font->FindGlyph((ImWchar)codepoint);
        if (glyph && glyph->Visible)
        {
            float gw = glyph->X1 - glyph->X0;
            float t = (x - cursor.x) / 160.0f;
            t = (t > 1.0f) ? 1.0f : t;
            ImVec4 col(from.x + (to.x - from.x) * t,
                       from.y + (to.y - from.y) * t,
                       from.z + (to.z - from.z) * t, 1.0f);
            dl->AddText(font, size, ImVec2(x, cursor.y), SciFi::C(col), p, p + n);
            x += gw * size;
        }
        p += n;
    }
    ImGui::Dummy(ImVec2(x - cursor.x, size + 4.0f));
}

// ============================================================================
// 徽章（胶囊）
// ============================================================================
void SciFiBadge(const char* text, const ImVec4& bg, const ImVec4& fg)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float padX = 10.0f, padY = 3.0f;
    ImVec2 bbMin = cursor;
    ImVec2 bbMax = ImVec2(cursor.x + textSize.x + padX * 2, cursor.y + textSize.y + padY * 2);
    float radius = (bbMax.y - bbMin.y) * 0.5f;
    dl->AddRectFilled(bbMin, bbMax, SciFi::C(bg), radius);
    dl->AddRect(bbMin, bbMax, SciFi::C(SciFi::WithA(fg, 0.5f)), radius);
    ImGui::SetCursorScreenPos(ImVec2(cursor.x + padX, cursor.y + padY));
    ImGui::PushStyleColor(ImGuiCol_Text, fg);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(bbMax.x, bbMax.y));
}

// ============================================================================
// 呼吸灯圆点
// ============================================================================
float SciFiBreathingDot(float cx, float cy, float radius, const ImVec4& color, float period)
{
    float t = (float)fmod(ImGui::GetTime(), period) / period;
    float alpha = 0.5f + 0.5f * sinf(t * 3.14159265f * 2.0f);
    float glowAlpha = 0.25f + 0.25f * alpha;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(ImVec2(cx, cy), radius * 2.2f, SciFi::Alpha(color, glowAlpha));
    dl->AddCircleFilled(ImVec2(cx, cy), radius, SciFi::Alpha(color, 0.4f + 0.6f * alpha));
    dl->AddCircle(ImVec2(cx, cy), radius, SciFi::Alpha(color, 0.9f), 32, 1.5f);
    return alpha;
}

// ============================================================================
// 导航项（渐变背景 + 左侧指示线 scaleY 动画 + 图标）
// ============================================================================
bool SciFiNavItem(int idx, const char* icon, const char* label, bool selected, float width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    const float itemHeight = 40.0f;
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImRect bb(cursor, ImVec2(cursor.x + width, cursor.y + itemHeight));
    const float rounding = 8.0f;

    bool hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    if (selected)
    {
        SciFiDrawGradientRect(dl, bb.Min, bb.Max, SciFi::WithA(SciFi::Accent, 0.22f), SciFi::WithA(SciFi::Purple, 0.12f), rounding);
    }
    else if (hovered)
    {
        dl->AddRectFilled(bb.Min, bb.Max, SciFi::Alpha(SciFi::Accent, 0.10f), rounding);
    }

    if (selected)
    {
        static float animProgress[32] = {};
        float target = 1.0f;
        float current = animProgress[idx % 32];
        animProgress[idx % 32] = current + (target - current) * (1.0f - expf(-8.0f * ImGui::GetIO().DeltaTime));
        float scaleY = animProgress[idx % 32];
        float lineH = itemHeight * 0.6f * scaleY;
        float lineY = bb.Min.y + (itemHeight - lineH) * 0.5f;
        ImU32 lineCol = SciFi::Alpha(SciFi::Accent, 0.5f + 0.5f * scaleY);
        dl->AddRectFilled(ImVec2(bb.Min.x + 2, lineY), ImVec2(bb.Min.x + 4, lineY + lineH), lineCol, 1.5f);
    }

    ImU32 textColor = selected ? SciFi::C(SciFi::Accent)
                    : (hovered ? SciFi::C(SciFi::TextMain) : SciFi::C(SciFi::TextDim));
    ImU32 iconColor  = selected ? SciFi::C(SciFi::Accent) : SciFi::C(SciFi::TextDim);

    if (icon && icon[0])
    {
        dl->AddRectFilled(ImVec2(bb.Min.x + 8, bb.Min.y + (itemHeight - 26) * 0.5f),
                          ImVec2(bb.Min.x + 34, bb.Min.y + (itemHeight - 26) * 0.5f + 26),
                          SciFi::Alpha(SciFi::Accent, selected ? 0.18f : 0.08f), 6.0f);
        dl->AddText(ImVec2(bb.Min.x + 16, bb.Min.y + (itemHeight - ImGui::GetFontSize()) * 0.5f),
                    iconColor, icon);
    }
    float textX = bb.Min.x + 44.0f;
    dl->AddText(ImVec2(textX, bb.Min.y + (itemHeight - ImGui::GetFontSize()) * 0.5f), textColor, label);

    if (selected || hovered)
    {
        ImVec2 arrowPos(bb.Max.x - 12, bb.Min.y + itemHeight * 0.5f);
        dl->AddTriangleFilled(
            ImVec2(arrowPos.x - 3, arrowPos.y - 3),
            ImVec2(arrowPos.x - 3, arrowPos.y + 3),
            ImVec2(arrowPos.x + 2, arrowPos.y),
            selected ? SciFi::C(SciFi::Accent) : SciFi::Alpha(SciFi::Accent, 0.4f));
    }

    ImGui::ItemSize(bb.GetSize());
    ImGui::ItemAdd(bb, ImGui::GetID(("##nav_" + std::string(label) + std::to_string(idx)).c_str()));
    return clicked;
}

// ============================================================================
// 渐变背景按钮
// ============================================================================
bool SciFiGradientButton(const char* label, const ImVec2& size, const ImVec4& from, const ImVec4& to)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImDrawList* dl = window->DrawList;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    bool clicked = false;
    bool hovered = false;

    ImGuiID id = ImGui::GetID(label);
    ImGui::PushID(label);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 actualSize = ImGui::GetContentRegionAvail();
    if (size.x > 0) actualSize.x = size.x;
    if (size.y > 0) actualSize.y = size.y;
    const ImRect bb(pos, pos + actualSize);

    hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool held = hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left);

    static std::unordered_map<ImGuiID, float> liftAnim;
    float targetLift = (hovered || held) ? 2.0f : 0.0f;
    float& lift = liftAnim[id];
    lift += (targetLift - lift) * (1.0f - expf(-15.0f * ImGui::GetIO().DeltaTime));
    ImVec2 drawMin = pos + ImVec2(0, -lift);

    const float rounding = 8.0f;
    ImVec2 p_min = drawMin;
    ImVec2 p_max = drawMin + actualSize;

    if (hovered)
    {
        ImU32 glow = SciFi::Alpha(from, 0.25f);
        dl->AddRectFilled(p_min - ImVec2(3, 3), p_max + ImVec2(3, 3), glow, rounding + 3.0f);
    }

    SciFiDrawGradientRect(dl, p_min, p_max,
        held ? SciFi::WithA(to, 0.9f) : from,
        held ? SciFi::WithA(from, 0.9f) : to, rounding);
    dl->AddRect(p_min, p_max, SciFi::Alpha(SciFi::Accent, 0.35f), rounding);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = p_min + (actualSize - textSize) * 0.5f;
    dl->AddText(textPos, SciFi::C(SciFi::TextMain), label);

    ImGui::ItemSize(actualSize);
    ImGui::ItemAdd(bb, id);
    ImGui::PopID();
    ImGui::PopStyleVar(2);
    return clicked;
}

// ============================================================================
// 带 shimmer 扫光的渐变按钮（3 秒循环）
// ============================================================================
bool SciFiShimmerButton(const char* label, const ImVec2& size, const ImVec4& from, const ImVec4& to)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImDrawList* dl = window->DrawList;

    ImGuiID id = ImGui::GetID(label);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushID(label);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 actualSize = ImGui::GetContentRegionAvail();
    if (size.x > 0) actualSize.x = size.x;
    if (size.y > 0) actualSize.y = size.y;
    const ImRect bb(pos, pos + actualSize);

    bool hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool held = hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left);

    static std::unordered_map<ImGuiID, float> liftAnim;
    float targetLift = (hovered || held) ? 2.0f : 0.0f;
    float& lift = liftAnim[id];
    lift += (targetLift - lift) * (1.0f - expf(-15.0f * ImGui::GetIO().DeltaTime));

    ImVec2 p_min = pos + ImVec2(0, -lift);
    ImVec2 p_max = p_min + actualSize;
    const float rounding = 8.0f;

    if (hovered)
    {
        dl->AddRectFilled(p_min - ImVec2(3, 3), p_max + ImVec2(3, 3), SciFi::Alpha(from, 0.25f), rounding + 3.0f);
    }

    SciFiDrawGradientRect(dl, p_min, p_max,
        held ? SciFi::WithA(to, 0.9f) : from,
        held ? SciFi::WithA(from, 0.9f) : to, rounding);
    dl->AddRect(p_min, p_max, SciFi::Alpha(SciFi::Accent, 0.4f), rounding);

    float t = fmodf(ImGui::GetTime(), 3.0f) / 3.0f;
    float bandWidth = actualSize.x * 0.45f;
    float bandX = p_min.x - bandWidth + (actualSize.x + bandWidth * 2.0f) * t;
    ImRect bandRect(bandX, p_min.y, bandX + bandWidth, p_max.y);
    float intensity = 0.10f + 0.35f * sinf(t * 3.14159f);
    dl->PushClipRect(p_min + ImVec2(rounding, 0), p_max - ImVec2(rounding, 0), true);
    dl->AddRectFilled(bandRect.Min, bandRect.Max, SciFi::RGBA(255, 255, 255, (int)(255 * intensity)));
    dl->PopClipRect();

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = p_min + (actualSize - textSize) * 0.5f;
    dl->AddText(textPos, SciFi::C(SciFi::TextMain), label);

    ImGui::ItemSize(actualSize);
    ImGui::ItemAdd(bb, id);
    ImGui::PopID();
    ImGui::PopStyleVar(1);
    return clicked;
}

// ============================================================================
// 描边按钮（hover 光晕扩散）
// ============================================================================
bool SciFiOutlineButton(const char* label, const ImVec2& size, const ImVec4& color)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImDrawList* dl = window->DrawList;

    ImGuiID id = ImGui::GetID(label);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushID(label);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 actualSize = ImGui::GetContentRegionAvail();
    if (size.x > 0) actualSize.x = size.x;
    if (size.y > 0) actualSize.y = size.y;
    const ImRect bb(pos, pos + actualSize);

    bool hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const float rounding = 8.0f;

    static std::unordered_map<ImGuiID, float> glowAnim;
    float target = hovered ? 1.0f : 0.0f;
    float& glow = glowAnim[id];
    glow += (target - glow) * (1.0f - expf(-12.0f * ImGui::GetIO().DeltaTime));

    dl->AddRectFilled(bb.Min, bb.Max, SciFi::Alpha(color, 0.06f + 0.08f * glow), rounding);
    dl->AddRect(bb.Min, bb.Max, SciFi::Alpha(color, 0.4f + 0.5f * glow), rounding, 0, 1.0f);
    if (glow > 0.02f)
        dl->AddRect(bb.Min - ImVec2(2 + glow * 2, 2 + glow * 2), bb.Max + ImVec2(2 + glow * 2, 2 + glow * 2),
                    SciFi::Alpha(color, 0.12f * glow), rounding + glow * 2, 0, 2.0f);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = bb.Min + (actualSize - textSize) * 0.5f;
    dl->AddText(textPos, SciFi::C(color), label);

    ImGui::ItemSize(actualSize);
    ImGui::ItemAdd(bb, id);
    ImGui::PopID();
    ImGui::PopStyleVar(1);
    return clicked;
}

// ============================================================================
// 数量步进器（带数字弹跳动画）
// ============================================================================
bool SciFiQuantityStepper(const char* id, int& value, int min, int max, int step)
{
    bool changed = false;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushID(id);

    static std::unordered_map<ImGuiID, float> bounce;
    ImGuiID numId = ImGui::GetID("num");

    float btnSize = 34.0f;
    ImFont* numFont = NumberFont ? NumberFont : ImGui::GetFont();
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    float numW = numFont->CalcTextSizeA(numFont->FontSize, FLT_MAX, 0.0f, buf).x + 30.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    if (ImGui::Button("-", ImVec2(btnSize, btnSize)))
    {
        if (value - step >= min) { value -= step; changed = true; bounce[numId] = 1.0f; }
    }
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 sz(numW, btnSize);
    const ImRect bb(pos, pos + sz);
    float& b = bounce[numId];
    b += (0.0f - b) * (1.0f - expf(-10.0f * ImGui::GetIO().DeltaTime));
    float scale = 1.0f + b * 0.18f;

    dl->AddRectFilled(bb.Min, bb.Max, SciFi::C(SciFi::CardBg), 8.0f);
    dl->AddRect(bb.Min, bb.Max, SciFi::C(SciFi::WithA(SciFi::Accent, 0.4f)), 8.0f);
    ImVec2 textSize = numFont->CalcTextSizeA(numFont->FontSize * scale, FLT_MAX, 0.0f, buf);
    ImVec2 textPos = bb.Min + (sz - textSize) * 0.5f;
    dl->PushTextureID(numFont->ContainerAtlas->TexID);
    dl->AddText(numFont, numFont->FontSize * scale, textPos, SciFi::C(SciFi::TextMain), buf);
    dl->PopTextureID();
    ImGui::Dummy(sz);

    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    if (ImGui::Button("+", ImVec2(btnSize, btnSize)))
    {
        if (value + step <= max) { value += step; changed = true; bounce[numId] = 1.0f; }
    }
    ImGui::PopStyleVar();

    ImGui::PopID();
    return changed;
}

// ============================================================================
// 类型标签
// ============================================================================
void SciFiTypeTag(const char* text, const ImVec4& color)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* f = TextFont ? TextFont : ImGui::GetFont();
    float fs = ImGui::GetFontSize() * 0.78f;
    ImVec2 textSize = f->CalcTextSizeA(fs, FLT_MAX, 0.0f, text);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float padX = 8.0f, padY = 2.0f;
    ImVec2 bbMin = cursor;
    ImVec2 bbMax = ImVec2(cursor.x + textSize.x + padX * 2, cursor.y + textSize.y + padY * 2);
    float radius = (bbMax.y - bbMin.y) * 0.5f;
    dl->AddRectFilled(bbMin, bbMax, SciFi::Alpha(color, 0.16f), radius);
    dl->AddRect(bbMin, bbMax, SciFi::Alpha(color, 0.55f), radius);
    ImGui::SetCursorScreenPos(ImVec2(cursor.x + padX, cursor.y + padY));
    ImGui::PushStyleColor(ImGuiCol_Text, SciFi::WithA(color, 0.95f));
    ImGui::PushFont(f);
    ImGui::SetWindowFontScale(fs / ImGui::GetFontSize());
    ImGui::TextUnformatted(text);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(bbMax.x, bbMax.y));
    ImGui::NewLine();
}

// ============================================================================
// 背景粒子系统
// ============================================================================
void SciFiRenderParticles(ImDrawList* dl, const ImVec2& origin, const ImVec2& size)
{
    const int ParticleCount = 26;
    static struct Particle {
        float x, y;
        float speed;
        float phase;
        float radius;
        ImVec4 color;
        float maxAlpha;
    } particles[ParticleCount];
    static bool initialized = false;

    if (!initialized)
    {
        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        ImVec4 palette[] = { SciFi::Accent, SciFi::Energy, SciFi::Success, SciFi::Purple };
        for (auto& p : particles)
        {
            p.x = dist01(rng);
            p.y = dist01(rng);
            p.speed = 0.02f + dist01(rng) * 0.04f;
            p.phase = dist01(rng) * 6.283f;
            p.radius = 1.2f + dist01(rng) * 2.2f;
            p.color = palette[(int)(dist01(rng) * 4) % 4];
            p.maxAlpha = 0.15f + dist01(rng) * 0.3f;
        }
        initialized = true;
    }

    float t = (float)ImGui::GetTime();
    float breathPhase = fmodf(t, 8.0f) / 8.0f;

    for (auto& p : particles)
    {
        float yOff = sinf(t * 0.6f * p.speed * 10.0f + p.phase) * 0.12f;
        float breath = 0.6f + 0.4f * sinf(breathPhase * 6.283f + p.phase);
        ImVec2 center(origin.x + p.x * size.x, origin.y + (p.y + yOff) * size.y);
        float alpha = p.maxAlpha * breath;
        dl->AddCircleFilled(center, p.radius, SciFi::Alpha(p.color, alpha));
        dl->AddCircleFilled(center, p.radius * 2.5f, SciFi::Alpha(p.color, alpha * 0.25f));
    }

    float cx = origin.x + size.x * (0.18f + 0.05f * sinf(t * 0.2f));
    float cy = origin.y + size.y * 0.3f;
    DrawRadialGlow(dl, ImVec2(cx, cy), 220.0f, SciFi::Alpha(SciFi::Accent, 0.06f));

    float cx2 = origin.x + size.x * 0.85f;
    float cy2 = origin.y + size.y * 0.75f;
    DrawRadialGlow(dl, ImVec2(cx2, cy2), 200.0f, SciFi::Alpha(SciFi::Energy, 0.05f));

    float cx3 = origin.x + size.x * 0.5f;
    float cy3 = origin.y + size.y * 1.02f;
    DrawRadialGlow(dl, ImVec2(cx3, cy3), 260.0f, SciFi::Alpha(SciFi::Success, 0.04f));
}

// 辅助：径向渐变光斑（多层同心圆）
void DrawRadialGlow(ImDrawList* dl, const ImVec2& center, float radius, ImU32 color)
{
    const int layers = 12;
    for (int i = layers; i > 0; --i)
    {
        float r = radius * (float)i / (float)layers;
        ImVec4 col = ImGui::ColorConvertU32ToFloat4(color);
        col.w *= (float)i / (float)layers;
        dl->AddCircleFilled(center, r, ImGui::GetColorU32(col), 48);
    }
}

// ============================================================================
// 通用样式辅助
// ============================================================================
void SciFiPushFrame(const ImVec4& bg, const ImVec4& border, float rounding)
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, bg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, bg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, bg);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
}

void SciFiPopFrame()
{
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}
