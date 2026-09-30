#pragma once
#include <windows.h>
#include <MinHook.h>

// MinHook 的轻量封装
namespace NieRBase
{
    inline bool CreateHook(LPVOID lpTarget, LPVOID pDetour, LPVOID* pOrig)
    {
        if (MH_CreateHook(lpTarget, pDetour, pOrig) != MH_OK)
            return false;
        if (MH_EnableHook(lpTarget) != MH_OK)
            return false;
        return true;
    }
}
