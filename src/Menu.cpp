#include "Menu.h"
#include "Engine.h"
#include "fonts.h"
#include "imgui_style.h"
#include "Game/Esp.h"
#include <string>
#include <imgui.h>

namespace NieRBase
{
    void Menu::Draw()
    {
        if (g_Engine->bShowMenu)
            MainMenu();

        // 演示窗口开关
        if (g_Engine->bShowDemo)
            ImGui::ShowDemoWindow();
    }

    void Menu::MainMenu()
    {
        static int selectedMenu = 0;

        struct NavEntry { const char* icon; const char* label; };
        static const NavEntry menuItems[] = {
            { "\xE2\x97\x8F", "功能" }, // ●
            { "\xE2\x97\x86", "设置" }, // ◆
            { "\xE2\x96\xA0", "关于" }, // ■
        };

        const float WindowW = 780.0f;
        const float WindowH = 520.0f;
        const float SidebarW = 180.0f;
        const float HeaderH = 60.0f;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 winPos = ImVec2((io.DisplaySize.x - WindowW) * 0.5f,
                               (io.DisplaySize.y - WindowH) * 0.5f);

        ImGui::SetNextWindowSize(ImVec2(WindowW, WindowH));
        ImGui::SetNextWindowPos(winPos, ImGuiCond_Once);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

        if (ImGui::Begin("##NieRMain", nullptr,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
        {
            ImDrawList* bg = ImGui::GetBackgroundDrawList();
            ImVec2 winMin = ImGui::GetWindowPos();
            ImVec2 winMax = winMin + ImGui::GetWindowSize();

            // 背景粒子 + 径向光斑
            SciFiRenderParticles(bg, ImVec2(winMin.x, winMin.y), ImVec2(winMax.x - winMin.x, winMax.y - winMin.y));

            // ==================== 顶部栏 ====================
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::BeginChild("Header", ImVec2(WindowW, HeaderH), false, ImGuiWindowFlags_NoScrollbar);
            {
                ImVec2 hMin = ImGui::GetWindowPos();
                ImVec2 hMax = hMin + ImGui::GetWindowSize();
                ImDrawList* hd = ImGui::GetWindowDrawList();

                // 顶部栏背景渐变
                hd->AddRectFilledMultiColor(hMin, hMax,
                    SciFi::C(SciFi::WithA(SciFi::PanelBg, 0.95f)),
                    SciFi::C(SciFi::WithA(SciFi::CardBg, 0.90f)),
                    SciFi::C(SciFi::WithA(SciFi::CardBg, 0.90f)),
                    SciFi::C(SciFi::WithA(SciFi::PanelBg, 0.95f)));
                hd->AddRectFilled(ImVec2(hMin.x + 24, hMax.y - 1), ImVec2(hMax.x - 24, hMax.y),
                    SciFi::Alpha(SciFi::Accent, 0.25f));

                // Logo 图标
                ImGui::SetCursorPos(ImVec2(24, 10));
                ImVec2 iconPos = ImGui::GetCursorScreenPos();
                ImVec2 iconSize(40, 40);
                ImU32 iconGlow = SciFi::Alpha(SciFi::Accent, 0.18f);
                hd->AddCircleFilled(ImVec2(iconPos.x + 20, iconPos.y + 20), 26, iconGlow, 40);
                SciFiDrawGradientRect(hd, iconPos, iconPos + iconSize, SciFi::WithA(SciFi::Accent, 0.35f), SciFi::WithA(SciFi::Purple, 0.28f), 10.0f);
                ImVec2 iconTextSize = ImGui::CalcTextSize(menuItems[selectedMenu].icon);
                hd->AddText(iconPos + (iconSize - iconTextSize) * 0.5f, SciFi::C(SciFi::TextMain), menuItems[selectedMenu].icon);

                // 标题
                ImGui::SetCursorPos(ImVec2(76, 16));
                ImGui::PushFont(HeaderFont ? HeaderFont : ImGui::GetFont());
                ImGui::TextUnformatted(menuItems[selectedMenu].label);
                ImGui::PopFont();
            }
            ImGui::EndChild();

            // ==================== 主体两列 ====================
            ImGui::SetCursorPos(ImVec2(0, HeaderH));
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, SidebarW);

            // ---- 左侧边栏 ----
            ImGui::BeginChild("Sidebar", ImVec2(SidebarW, WindowH - HeaderH), false, ImGuiWindowFlags_NoScrollbar);
            {
                ImDrawList* sd = ImGui::GetWindowDrawList();
                ImVec2 sMin = ImGui::GetWindowPos();
                ImVec2 sMax = sMin + ImGui::GetWindowSize();

                sd->AddRectFilled(sMin, sMax, SciFi::Alpha(SciFi::PanelBg, 0.92f), 12.0f, ImDrawFlags_RoundCornersBottomLeft);
                sd->AddRectFilled(ImVec2(sMax.x - 1, sMin.y), ImVec2(sMax.x, sMax.y), SciFi::Alpha(SciFi::Accent, 0.15f));

                // Logo 渐变文字
                ImGui::SetCursorPos(ImVec2(20, 14));
                ImGui::PushFont(HeaderFont ? HeaderFont : ImGui::GetFont());
                ImGui::SetWindowFontScale(1.15f);
                SciFiGradientText("NieR", 22.0f, SciFi::Accent, SciFi::Energy);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();

                // 版本徽章 + Ready 呼吸灯（同一行，垂直居中对齐）
                ImGui::SetCursorPos(ImVec2(20, 58));
                SciFiBadge("v1.0", SciFi::WithA(SciFi::Accent, 0.12f), SciFi::Accent);
                ImGui::SameLine(0, 10.0f);
                {
                    // 呼吸灯只画图不占布局宽度，必须用 Dummy 占位，否则后面的
                    // SameLine 会让 "Ready" 压在灯上；圆心对齐文字行垂直中心
                    const float dotR = 4.0f;
                    const float lineH = ImGui::GetTextLineHeight();
                    ImVec2 p = ImGui::GetCursorScreenPos();
                    SciFiBreathingDot(p.x + dotR + 1.0f, p.y + lineH * 0.5f, dotR, SciFi::Success, 2.0f);
                    ImGui::Dummy(ImVec2(dotR * 2.0f + 2.0f, lineH));
                }
                ImGui::SameLine(0, 6.0f);
                ImGui::TextColored(SciFi::Success, "Ready");

                // 分隔线
                ImGui::SetCursorPos(ImVec2(16, 92));
                ImGui::GetWindowDrawList()->AddLine(
                    ImGui::GetCursorScreenPos() + ImVec2(0, 6),
                    ImGui::GetCursorScreenPos() + ImVec2(SidebarW - 32, 6),
                    SciFi::Alpha(SciFi::Accent, 0.18f), 1.0f);
                ImGui::Dummy(ImVec2(0, 18));

                // 导航项
                float navWidth = SidebarW - 28.0f;
                for (int i = 0; i < IM_ARRAYSIZE(menuItems); ++i)
                {
                    ImGui::SetCursorPos(ImVec2(14, ImGui::GetCursorPosY()));
                    if (SciFiNavItem(i, menuItems[i].icon, menuItems[i].label, selectedMenu == i, navWidth))
                        selectedMenu = i;
                    ImGui::Spacing();
                }

                // 退出按钮
                ImGui::Spacing();
                ImGui::SetCursorPos(ImVec2(14, WindowH - HeaderH - 60));
                if (SciFiGradientButton("退出", ImVec2(navWidth, 40), SciFi::WithA(SciFi::Danger, 0.8f), SciFi::WithA(SciFi::Danger, 0.45f)))
                    g_KillSwitch = true;
            }
            ImGui::EndChild();
            ImGui::NextColumn();

            // ---- 右侧内容区 ----
            ImGui::PushStyleColor(ImGuiCol_ChildBg, SciFi::WithA(SciFi::BgBase, 0.0f));
            ImGui::BeginChild("Content", ImVec2(WindowW - SidebarW, WindowH - HeaderH), false);
            {
                ImGui::SetCursorPos(ImVec2(20, 18));
                ImGui::BeginChild("ContentPad", ImVec2(ImGui::GetContentRegionAvail().x - 40, ImGui::GetContentRegionAvail().y - 36));
                {
                    switch (selectedMenu)
                    {
                    case 0: // 功能
                    {
                        ColoredSeparatorText("ESP 透视", SciFi::Accent);
                        ImGui::Spacing();
                        ImGui::Checkbox("启用 ESP ", &nier::esp::config.enabled);
                        ImGui::BeginDisabled(!nier::esp::config.enabled);
                        ImGui::Checkbox("显示方框 ", &nier::esp::config.showBox);
                        ImGui::Checkbox("显示距离 ", &nier::esp::config.showDistance);
                        ImGui::Checkbox("显示连线 ", &nier::esp::config.showSnapline);
                        ImGui::SliderFloat("最大距离 ", &nier::esp::config.maxDistance, 50.0f, 1000.0f, "%.0f m");
                        ImGui::EndDisabled();

                        ImGui::Spacing();
                        ColoredSeparatorText("说明", SciFi::TextDim);
                        ImGui::TextColored(SciFi::TextDim, "相机矩阵语义未最终确认，若画面异常请先关 ESP。");
                        ImGui::TextColored(SciFi::TextDim, "按 INSERT 开关菜单");
                        break;
                    }
                    case 1: // 设置
                    {
                        ColoredSeparatorText("显示设置", SciFi::Accent);
                        ImGui::Spacing();
                        static bool bFps = false;
                        ImGui::Checkbox("FPS 显示 ", &bFps);
                        if(bFps){
                            ImDrawList* dl = ImGui::GetBackgroundDrawList();
                            char buffer[32];
                            snprintf(buffer, sizeof(buffer), "FPS: %.0f", ImGui::GetIO().Framerate);
                            dl->AddText(ImVec2(10, 20), IM_COL32(255, 255, 255, 255), buffer);
                        }
                        break;
                    }
                    case 2: // 关于
                    {
                        ColoredSeparatorText("关于", SciFi::Accent);
                        ImGui::Spacing();
                        ImGui::TextColored(SciFi::TextMain, "NieR DX11 内部注入菜单");
                        ImGui::TextColored(SciFi::TextDim, "基于 ImGui + MinHook，hook IDXGISwapChain::Present");
                        ImGui::TextColored(SciFi::TextDim, "按 INSERT 开关菜单");
                        break;
                    }
                    }
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Columns(1);
        }

        ImGui::End();
        ImGui::PopStyleVar(4);
    }
}
