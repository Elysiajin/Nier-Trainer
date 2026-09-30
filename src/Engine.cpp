#include "Engine.h"
#include "Menu.h"
#include "Hooking.h"
#include "fonts.h"
#include "imgui_style.h"
#include "Game/Esp.h"
#include "Game/GameData.h"
#include "Game/Capture.h"
#include <cstdio>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace NieRBase
{
    // ============================================================
    //                       ENGINE
    // ============================================================
    void Engine::Init()
    {
        mGamePID = GetCurrentProcessId();
        pGameModule = GetModuleHandle(nullptr);
        pGameWindow = GetForegroundWindow();

        RECT tempRECT;
        GetWindowRect(pGameWindow, &tempRECT);
        mGameWidth = tempRECT.right - tempRECT.left;
        mGameHeight = tempRECT.bottom - tempRECT.top;
    }

    bool Engine::GetKeyState(WORD vKey, SHORT delta)
    {
        static int lastTick = 0;
        bool result = ((GetAsyncKeyState(vKey) & 0x8000) && (GetTickCount64() - lastTick) > delta);
        if (result)
            lastTick = static_cast<int>(GetTickCount64());
        return result;
    }

    // ============================================================
    //                       CONSOLE
    // ============================================================
    void Console::InitializeConsole(const char* ConsoleName, bool bShowWindow)
    {
        AllocConsole();
        FILE* pOut = nullptr;
        freopen_s(&pOut, "CONOUT$", "w", stdout);
        SetConsoleTitleA(ConsoleName);
        if (!bShowWindow)
            ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    void Console::cLog(const char* fmt, EColors color, ...)
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
        va_list arg;
        va_start(arg, color);
        vfprintf(stdout, fmt, arg);
        va_end(arg);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), EColor_DEFAULT);
    }

    // ============================================================
    //                      D3D11 WINDOW
    // ============================================================
    D3D11Window::~D3D11Window() noexcept
    {
        bInit = false;
    }

    LRESULT D3D11Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // 菜单打开时：吞掉所有输入消息，避免穿透到游戏
        if (g_Engine->bShowMenu)
        {
            switch (msg)
            {
            case WM_MOUSEACTIVATE:
                // 防止点击激活游戏窗口，避免抢焦点
                return MA_NOACTIVATE;
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEMOVE:
            case WM_MOUSEWHEEL:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_CHAR:
                // 交给 ImGui，且不转发给游戏
                ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
                return true;
            default:
                ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
                return true;
            }
        }
        return CallWindowProc(g_D3D11Window->m_OldWndProc, hWnd, msg, wParam, lParam);
    }

    HRESULT APIENTRY D3D11Window::SwapChain_Present_hook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
    {
        g_D3D11Window->Overlay(pSwapChain);
        if (!g_D3D11Window->IDXGISwapChain_Present_stub)
            return E_FAIL;
        return g_D3D11Window->IDXGISwapChain_Present_stub(pSwapChain, SyncInterval, Flags);
    }

    HRESULT APIENTRY D3D11Window::SwapChain_ResizeBuffers_hook(
        IDXGISwapChain* p, UINT bufferCount, UINT Width, UINT Height,
        DXGI_FORMAT fmt, UINT scFlags)
    {
        g_D3D11Window->m_pSwapChain = p;
        if (g_D3D11Window->m_RenderTargetView)
        {
            g_D3D11Window->m_RenderTargetView->Release();
            g_D3D11Window->m_RenderTargetView = nullptr;
        }

        HRESULT result = g_D3D11Window->IDXGISwapChain_ResizeBuffers_stub(p, bufferCount, Width, Height, fmt, scFlags);

        // 重新获取后备缓冲并重建 RenderTarget
        ID3D11Texture2D* backBuffer = nullptr;
        p->GetBuffer(0, __uuidof(ID3D11Texture2D*), (LPVOID*)&backBuffer);
        if (backBuffer)
        {
            g_D3D11Window->m_Device->CreateRenderTargetView(backBuffer, nullptr, &g_D3D11Window->m_RenderTargetView);
            backBuffer->Release();
        }

        if (g_D3D11Window->bInitImGui)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));
        }

        return result;
    }

    bool D3D11Window::HookD3D()
    {
        if (GetD3DContext())
        {
            // MinHook 必须先初始化
            if (MH_Initialize() != MH_OK)
                LogFile("[4] MH_Initialize returned non-OK");

            LogFile("[4] GetD3DContext ok, Present=%p ResizeBuffers=%p",
                (void*)SwapChainVtbl[IDXGI_PRESENT], (void*)SwapChainVtbl[IDXGI_RESIZE_BUFFERS]);

            // 用正确的 vtable 槽位建立 hook
            bool b1 = CreateHook((LPVOID)SwapChainVtbl[IDXGI_PRESENT], &SwapChain_Present_hook, (LPVOID*)&IDXGISwapChain_Present_stub);
            bool b2 = CreateHook((LPVOID)SwapChainVtbl[IDXGI_RESIZE_BUFFERS], &SwapChain_ResizeBuffers_hook, (LPVOID*)&IDXGISwapChain_ResizeBuffers_stub);
            LogFile("[5] CreateHook Present=%d ResizeBuffers=%d", b1 ? 1 : 0, b2 ? 1 : 0);
            bInit = true;
            return true;
        }
        LogFile("[4] GetD3DContext failed");
        return false;
    }

    bool D3D11Window::GetD3DContext()
    {
        if (!InitWindow())
            return false;

        HMODULE D3D11Module = GetModuleHandleA("d3d11.dll");
        if (!D3D11Module)
        {
            DeleteWindow();
            return false;
        }

        D3D_FEATURE_LEVEL FeatureLevel;
        const D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

        DXGI_RATIONAL RefreshRate;
        RefreshRate.Numerator = 60;
        RefreshRate.Denominator = 1;

        DXGI_MODE_DESC BufferDesc{};
        BufferDesc.Width = 100;
        BufferDesc.Height = 100;
        BufferDesc.RefreshRate = RefreshRate;
        BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        DXGI_SAMPLE_DESC SampleDesc{};
        SampleDesc.Count = 1;
        SampleDesc.Quality = 0;

        DXGI_SWAP_CHAIN_DESC SwapChainDesc{};
        SwapChainDesc.BufferDesc = BufferDesc;
        SwapChainDesc.SampleDesc = SampleDesc;
        SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.BufferCount = 1;
        SwapChainDesc.OutputWindow = WindowHwnd;
        SwapChainDesc.Windowed = 1;
        SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        IDXGISwapChain* SwapChain = nullptr;
        ID3D11Device* Device = nullptr;
        ID3D11DeviceContext* Context = nullptr;
        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                FeatureLevels, 1, D3D11_SDK_VERSION, &SwapChainDesc, &SwapChain,
                &Device, &FeatureLevel, &Context) < 0)
        {
            DeleteWindow();
            return false;
        }

        // 缓存 swapchain 虚表（Present=8, ResizeBuffers=13）
        // 虚表项为指针宽度，必须用 uintptr_t* 读取
        SwapChainVtbl = *(uintptr_t**)SwapChain;

        // 释放临时对象
        SwapChain->Release();
        Device->Release();
        Context->Release();

        DeleteWindow();
        return true;
    }

    void D3D11Window::UnhookD3D()
    {
        if (m_OldWndProc)
            SetWindowLongPtr(g_Engine->pGameWindow, GWLP_WNDPROC, (LONG_PTR)m_OldWndProc);
    }

    bool D3D11Window::InitWindow()
    {
        WindowClass.cbSize = sizeof(WNDCLASSEX);
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        WindowClass.lpfnWndProc = DefWindowProc;
        WindowClass.cbClsExtra = 0;
        WindowClass.cbWndExtra = 0;
        WindowClass.hInstance = GetModuleHandle(nullptr);
        WindowClass.hIcon = nullptr;
        WindowClass.hCursor = nullptr;
        WindowClass.hbrBackground = nullptr;
        WindowClass.lpszMenuName = nullptr;
        WindowClass.lpszClassName = "NieROverlay";
        WindowClass.hIconSm = nullptr;
        RegisterClassEx(&WindowClass);
        WindowHwnd = CreateWindow(WindowClass.lpszClassName, "DX11 Window",
            WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, WindowClass.hInstance, nullptr);
        if (WindowHwnd == nullptr)
            return false;
        return true;
    }

    bool D3D11Window::DeleteWindow()
    {
        DestroyWindow(WindowHwnd);
        UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
        WindowHwnd = nullptr;
        return true;
    }

    bool D3D11Window::InitImGui(IDXGISwapChain* swapChain)
    {
        if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&m_Device)))
        {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.IniFilename = nullptr;

            m_Device->GetImmediateContext(&m_DeviceContext);

            DXGI_SWAP_CHAIN_DESC Desc;
            swapChain->GetDesc(&Desc);
            g_Engine->pGameWindow = Desc.OutputWindow;
            LogFile("[6] InitImGui: outputWnd=%p size=%dx%d", Desc.OutputWindow, Desc.BufferDesc.Width, Desc.BufferDesc.Height);

            ID3D11Texture2D* BackBuffer = nullptr;
            swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
            m_Device->CreateRenderTargetView(BackBuffer, nullptr, &m_RenderTargetView);
            BackBuffer->Release();

            ImGui_ImplWin32_Init(g_Engine->pGameWindow);
            ImGui_ImplDX11_Init(m_Device, m_DeviceContext);

            // 截获游戏上传 GPU 的常量缓冲（视图/投影矩阵）
            if (!nier::game::InstallContextHooks(m_DeviceContext))
                LogFile("[6b] Map/Unmap hook install FAILED");

            // ===== 合并几何符号字形区间（● ◆ ■），否则这些图标会渲染成 '?' =====
            // BuildRanges 的结果必须存活到字体图集构建（首次 NewFrame），所以用 static
            static ImVector<ImWchar> glyphRanges;
            if (glyphRanges.empty())
            {
                ImFontGlyphRangesBuilder rangeBuilder;
                rangeBuilder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
                rangeBuilder.AddText("\xE2\x97\x8F\xE2\x97\x86\xE2\x96\xA0"); // ● ◆ ■
                rangeBuilder.BuildRanges(&glyphRanges);
            }

            // ===== 加载中文字体，解决中文显示问号 =====
            // 正文（微软雅黑，含中文全字集 + 几何图标）
            TextFont = io.Fonts->AddFontFromFileTTF(
                "C:/Windows/Fonts/msyh.ttc", 18.0f, nullptr, glyphRanges.Data);
            if (!TextFont)
                TextFont = io.Fonts->AddFontFromFileTTF(
                    "C:/Windows/Fonts/segoeui.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
            io.FontDefault = TextFont;

            // 标题（微软雅黑粗体，稍大）
            HeaderFont = io.Fonts->AddFontFromFileTTF(
                "C:/Windows/Fonts/msyhbd.ttc", 22.0f, nullptr, glyphRanges.Data);
            if (!HeaderFont)
                HeaderFont = TextFont;

            // 等宽数字（用于 ID / 数值）
            NumberFont = io.Fonts->AddFontFromFileTTF(
                "C:/Windows/Fonts/consola.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
            if (!NumberFont)
                NumberFont = TextFont;

            LogFile("[6] Fonts loaded: TextFont=%p HeaderFont=%p NumberFont=%p", TextFont, HeaderFont, NumberFont);

            // 应用深色科技风主题
            SetupImGuiStyle();

            ImGui_ImplDX11_CreateDeviceObjects();

            // 替换 WndProc 以捕获键盘/鼠标输入
            m_OldWndProc = (WNDPROC)SetWindowLongPtr(g_Engine->pGameWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);
            LogFile("[7] WndProc replaced, old=%p", m_OldWndProc);

            bInitImGui = true;
            m_pSwapChain = swapChain;
            return true;
        }
        bInitImGui = false;
        LogFile("[6] InitImGui failed: GetDevice failed");
        return false;
    }

    void D3D11Window::Overlay(IDXGISwapChain* pSwapChain)
    {
        static int overlayCount = 0;
        if (overlayCount < 5)
        {
            LogFile("[8] Overlay called #%d, bInitImGui=%d", overlayCount, bInitImGui ? 1 : 0);
            overlayCount++;
        }

        if (!bInitImGui)
            InitImGui(pSwapChain);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_Engine->bShowMenu;

        if (g_Engine->bShowMenu)
        {
            // 菜单打开：强制 ImGui 捕获鼠标，并释放游戏窗口捕获，抑制穿透
            io.WantCaptureMouse = true;
            io.WantCaptureKeyboard = true;
            ReleaseCapture();
            if (GetCapture() == g_Engine->pGameWindow)
                ReleaseCapture();
        }

        // 渲染 ESP（开关由 nier::esp::config.enabled 控制）
        nier::esp::Render(nier::game::g_entityCache);

        // 渲染菜单
        Menu::Draw();

        ImGui::EndFrame();
        ImGui::Render();
        m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}
