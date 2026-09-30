#include "Engine.h"
#include "Menu.h"
#include <MinHook.h>
#include <thread>
#include <chrono>

using namespace NieRBase;

// 后台线程：处理退出逻辑
void ClientBGThread()
{
    while (g_Running)
    {
        if (g_KillSwitch)
        {
            g_D3D11Window->UnhookD3D();
            // 还原 MinHook 所有 hook
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            g_Running = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
}

// DLL 加载后的主初始化线程
DWORD WINAPI MainThread_Initialize(LPVOID dwModule)
{
    HMODULE hSelf = static_cast<HMODULE>(dwModule);

    g_Console = std::make_unique<Console>();
    g_Console->InitializeConsole("NieR Console", true);

    g_D3D11Window = std::make_unique<D3D11Window>();

    g_Engine = std::make_unique<Engine>();
    g_Engine->Init();
    LogFile("[1] Engine init ok, PID=%d, hwnd=%p", g_Engine->mGamePID, g_Engine->pGameWindow);

    // 建立 D3D11 Present hook
    bool hooked = g_D3D11Window->HookD3D();
    LogFile("[2] HookD3D returned %d", hooked ? 1 : 0);
    if (!hooked)
    {
        FreeLibraryAndExitThread(hSelf, 1);
        return 1;
    }

    // 后台线程
    std::thread bgThread(ClientBGThread);

    // 主循环：菜单开关
    g_Running = true;
    while (g_Running)
    {
        // 菜单开关
        if (Engine::GetKeyState(VK_INSERT, 500))
        {
            g_Engine->bShowMenu = !g_Engine->bShowMenu;
            LogFile("[3] INSERT pressed, bShowMenu=%d", g_Engine->bShowMenu ? 1 : 0);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }

    bgThread.join();

    FreeLibraryAndExitThread(hSelf, 0);
    return 0;
}
