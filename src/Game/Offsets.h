#pragma once
// ============================================================
// NieR:Automata 静态偏移（RVA）
// 来源：docs/ESP静态分析报告.md，仅适用于
//   MD5 ed6c35a8d8083c142eec09c59c4ec1bd 的 NieRAutomata.exe
// 游戏更新后此文件整体重验。
// ============================================================

#include <cstddef>

namespace nier::offsets
{
    // ---- 全局实体表 ----
    inline constexpr std::ptrdiff_t kEntityTable       = 0x1029520; // EntityTableEntry[0x4000]
    inline constexpr std::ptrdiff_t kEntityTableBound  = 0x1029510; // 数量上界
    inline constexpr std::ptrdiff_t kEntityTableCount  = 0x4000;    // 容量

    // ---- 对象管理器 ----
    inline constexpr std::ptrdiff_t kEntityManager     = 0x10177E8; // EmBaseManagerImplement*

    // ---- 相机 ----
    inline constexpr std::ptrdiff_t kCameraGame        = 0x1020870; // cCameraGame（内嵌对象）
    inline constexpr std::ptrdiff_t kCameraMatrix      = 0x10;      // 矩阵候选，语义待运行时确认

    // ---- vftable（用于实体分类）----
    inline constexpr std::ptrdiff_t kVftablePl0000     = 0xC5EBF0;  // 玩家
    inline constexpr std::ptrdiff_t kVftableEmBase     = 0xC77340;  // 敌人/NPC 基类

    // ---- EntityBase 内部偏移 ----
    inline constexpr std::ptrdiff_t kEntityWorldMatrix = 0x10;      // 4x4 世界矩阵
    inline constexpr std::ptrdiff_t kEntityPos         = 0x40;      // 平移 x,y,z（矩阵 row3）
    inline constexpr std::ptrdiff_t kEntityFlags       = 0x2C;      // 低 2 位 = 存活标志
}
