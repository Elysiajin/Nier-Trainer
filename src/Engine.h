#pragma once
#include <windows.h>
#include <cstdio>
#include <d3d11.h>
#include <dxgi.h>
#include <memory>
#include <imgui.h>

namespace NieRBase
{
    // 简单的文件日志，用于排查注入后的问题。
    // 写入 DLL 所在目录下的 NieR_debug.log（避免 C 盘权限问题）
    inline void LogFile(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        char path[MAX_PATH];
        GetModuleFileNameA(GetModuleHandleA("NieRBase.dll"), path, MAX_PATH);
        // 截取目录部分
        char* slash = strrchr(path, '\\');
        if (slash) slash[1] = '\0'; else lstrcpyA(path, "C:\\");

        lstrcatA(path, "NieR_debug.log");

        FILE* f = nullptr;
        fopen_s(&f, path, "a");
        if (f)
        {
            fprintf(f, "%s\n", buf);
            fclose(f);
        }
    }

    // 全局运行状态
    inline bool g_Running = false;
    inline bool g_KillSwitch = false;

    class Engine;
    class Console;
    class D3D11Window;
    inline std::unique_ptr<Engine> g_Engine;
    inline std::unique_ptr<D3D11Window> g_D3D11Window;

    class Engine
    {
    public:
        // 进程 & 窗口信息
        int     mGamePID{ 0 };
        HMODULE pGameModule{ 0 };
        HWND    pGameWindow{ 0 };
        int     mGameWidth{ 0 };
        int     mGameHeight{ 0 };
        ImVec2  mWindowSize{ 0.f, 0.f };

        // 菜单状态
        bool    bShowMenu{ false };
        bool    bShowDemo{ false };

    public:
        void Init();
        static bool GetKeyState(WORD vKey, SHORT delta);

        Engine() = default;
        ~Engine() = default;
    };

    class Console
    {
    public:
        enum EColors : int
        {
            EColor_dark_blue = 1,
            EColor_dark_green,
            EColor_dark_teal,
            EColor_dark_red,
            EColor_dark_pink,
            EColor_dark_yellow,
            EColor_dark_white,
            EColor_dark_gray,
            EColor_blue,
            EColor_green,
            EColor_teal,
            EColor_red,
            EColor_pink,
            EColor_yellow,
            EColor_white,
            EColor_DEFAULT = EColor_white
        };

    public:
        void InitializeConsole(const char* ConsoleName, bool bShowWindow = true);
        void cLog(const char* fmt, const EColors Color = EColor_DEFAULT, ...);

    public:
        explicit Console() = default;
        ~Console() = default;
    };
    inline std::unique_ptr<Console> g_Console;

    class D3D11Window
    {
    public:
        enum DXGI : int
        {
            IDXGI_PRESENT = 8,          // IDXGISwapChain::Present
            IDXGI_RESIZE_BUFFERS = 13,  // IDXGISwapChain::ResizeBuffers
        };

    public:
        bool    bInit{ false };
        bool    bInitImGui{ false };
        WNDPROC m_OldWndProc{};

    public:
        bool GetD3DContext();
        bool HookD3D();
        void UnhookD3D();
        bool InitWindow();
        bool DeleteWindow();
        bool InitImGui(IDXGISwapChain* swapChain);
        void Overlay(IDXGISwapChain* pSwapChain);

        D3D11Window() = default;
        ~D3D11Window() noexcept;

    private:
        typedef HRESULT(WINAPI* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        typedef HRESULT(WINAPI* IDXGISwapChainResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

        IDXGISwapChainPresent          IDXGISwapChain_Present_stub = nullptr;
        IDXGISwapChainResizeBuffers    IDXGISwapChain_ResizeBuffers_stub = nullptr;

        // 虚表项为指针宽度，用 uintptr_t* 读取（x86 4 字节 / x64 8 字节均正确）
        uintptr_t* SwapChainVtbl{ nullptr };

        WNDCLASSEX   WindowClass;
        HWND         WindowHwnd;
        ID3D11Device*        m_Device{};
        ID3D11DeviceContext* m_DeviceContext{};
        ID3D11RenderTargetView* m_RenderTargetView{};
        IDXGISwapChain*      m_pSwapChain{};

    private:
        static LRESULT APIENTRY  WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static HRESULT APIENTRY  SwapChain_Present_hook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static HRESULT APIENTRY  SwapChain_ResizeBuffers_hook(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    };
}
