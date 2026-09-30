cmake_minimum_required(VERSION 3.20)

# 注入 DLL 名称
project(NieRBase VERSION 1.0 LANGUAGES C CXX)

# 目标进程 NieRAutomata.exe 为 64 位 DX11 游戏（如需注入 32 位游戏改用 cmake -A Win32）
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "目标进程 NieRAutomata.exe 为 64 位，请使用 x64 工具链 (cmake -A x64)")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(MSVC)
    add_compile_options(/utf-8)
    # Release 优化与体积
    add_compile_options($<$<CONFIG:Release>:/O2>)
    add_compile_options($<$<CONFIG:Release>:/Ob2>)
    # 关闭调试信息（减小体积）
    string(REPLACE "/Zi" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/ZI" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    add_compile_options($<$<CONFIG:Release>:/Gy>)
endif()

# ============================================================
# ImGui (DX11 + Win32 后端)
# ============================================================
add_library(ImGui STATIC
    third_party/ImGui/imgui.cpp
    third_party/ImGui/imgui_demo.cpp
    third_party/ImGui/imgui_draw.cpp
    third_party/ImGui/imgui_tables.cpp
    third_party/ImGui/imgui_widgets.cpp
    third_party/ImGui/imgui_impl_dx11.cpp
    third_party/ImGui/imgui_impl_win32.cpp
)
# 启用 ImVec2/ImVec4 数学运算符（ImGui 默认关闭，需显式定义）
target_compile_definitions(ImGui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
target_include_directories(ImGui PUBLIC third_party/ImGui)

# ============================================================
# MinHook
# ============================================================
add_library(MinHook STATIC
    third_party/MinHook/buffer.c
    third_party/MinHook/hde32.c
    third_party/MinHook/hde64.c
    third_party/MinHook/hook.c
    third_party/MinHook/trampoline.c
)
target_include_directories(MinHook PUBLIC third_party/MinHook)

# ============================================================
# NieRBase (注入 DLL)
# ============================================================
add_library(NieRBase SHARED
    src/dllmain.cpp
    src/Source.cpp
    src/Engine.cpp
    src/Menu.cpp
    src/imgui_style.cpp
    src/Game/GameData.cpp
    src/Game/Capture.cpp
    src/Game/Esp.cpp
)

target_include_directories(NieRBase PRIVATE src)

target_link_libraries(NieRBase PRIVATE
    ImGui
    MinHook
    d3d11
    dxgi
    d3dcompiler
    winmm
    xinput
    dxguid
)

set_target_properties(NieRBase PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
)
