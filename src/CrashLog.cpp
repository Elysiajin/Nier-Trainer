#include "CrashLog.h"
#include "Engine.h"
#include <windows.h>
#include <psapi.h>
#include <cstdio>

namespace NieRBase
{
    namespace
    {
        struct ModuleInfo
        {
            HMODULE base{ nullptr };
            char name[MAX_PATH]{};
        };

        ModuleInfo FindModule(const void* addr) noexcept
        {
            ModuleInfo info;
            HMODULE mods[512]{};
            DWORD needed = 0;
            HANDLE proc = GetCurrentProcess();
            if (!EnumProcessModules(proc, mods, sizeof(mods), &needed))
                return info;
            const int count = static_cast<int>(needed / sizeof(HMODULE));
            for (int i = 0; i < count && i < 512; ++i)
            {
                MODULEINFO mi{};
                if (!GetModuleInformation(proc, mods[i], &mi, sizeof(mi)))
                    continue;
                const auto* a = static_cast<const BYTE*>(addr);
                const auto* lo = static_cast<const BYTE*>(mi.lpBaseOfDll);
                if (a >= lo && a < lo + mi.SizeOfImage)
                {
                    info.base = mods[i];
                    GetModuleFileNameA(mods[i], info.name, MAX_PATH);
                    // 只留文件名
                    if (const char* slash = strrchr(info.name, '\\'))
                        memmove(info.name, slash + 1, strlen(slash));
                    break;
                }
            }
            return info;
        }

        LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* ep) noexcept
        {
            if (ep && ep->ExceptionRecord)
            {
                const void* addr = ep->ExceptionRecord->ExceptionAddress;
                ModuleInfo mod = FindModule(addr);
                LONG offset = reinterpret_cast<const BYTE*>(mod.base)
                    ? static_cast<LONG>(reinterpret_cast<const BYTE*>(addr)
                                        - reinterpret_cast<const BYTE*>(mod.base))
                    : 0;
                LogFile("[CRASH] code=0x%08X addr=%p module=%s+%#lx",
                        ep->ExceptionRecord->ExceptionCode, addr,
                        mod.name[0] ? mod.name : "?", offset);
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    void InstallCrashLogger()
    {
        SetUnhandledExceptionFilter(&UnhandledFilter);
    }
}
