#pragma once
// 未处理异常记录器：崩溃时把异常代码/地址/模块偏移写入 NieR_debug.log，
// 用于事后用 IDA 定位崩溃点。在 DLL 注入初始化时安装。
namespace NieRBase
{
    void InstallCrashLogger();
}
