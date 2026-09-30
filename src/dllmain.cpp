#include <windows.h>
#include "Engine.h"

// MainThread_Initialize 定义于 Source.cpp
extern DWORD WINAPI MainThread_Initialize(LPVOID dwModule);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwCallReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwCallReason == DLL_PROCESS_ATTACH)
    {
        NieRBase::LogFile("[0] DllMain DLL_PROCESS_ATTACH, hModule=%p", hModule);

        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, MainThread_Initialize, hModule, 0, nullptr);
        NieRBase::LogFile("[0] CreateThread returned %p (err=%u)", hThread, GetLastError());
        if (hThread)
            CloseHandle(hThread);
    }

    return TRUE;
}
