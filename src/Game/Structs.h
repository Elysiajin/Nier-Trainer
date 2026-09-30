#pragma once
// ============================================================
// NieR:Automata 游戏结构体（逆向自 IDA，证据见 docs/ESP静态分析报告.md）
// 占位字段统一命名 un_pad<起始偏移>，与 IDA 内地址直接对应。
// 所有尺寸用 static_assert 锁死，游戏更新后编译期即报错。
// ============================================================

#include <cstddef>
#include <cstdint>
#include <array>
#include <cmath>

namespace nier::game
{
    using u8 = std::uint8_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using f32 = float;

    // --------------------------------------------------------
    // EntityHandle：4 字节句柄，不是裸指针。
    // bit 0~7   类型标记（解析时不参与校验）
    // bit 8~23  全局实体表索引
    // bit 24~31 代数/校验位
    // 解析算法见 GameData.cpp 的 Resolve()
    // --------------------------------------------------------
    struct EntityHandle
    {
        u32 bits{ 0 };

        [[nodiscard]] constexpr u32 Index() const noexcept { return (bits >> 8) & 0xFFFF; }
        [[nodiscard]] constexpr bool IsValid() const noexcept { return bits != 0; }
    };

    // --------------------------------------------------------
    // 全局实体表条目（16 字节）
    // --------------------------------------------------------
    struct EntityTableEntry
    {
        u32 check{ 0 };       // +0x00 与 handle 的 bit8~31 异或比对
        u32 un_pad04{ 0 };    // +0x04
        void* entity{ nullptr }; // +0x08 Entity*
    };
    static_assert(sizeof(EntityTableEntry) == 16);
    static_assert(offsetof(EntityTableEntry, entity) == 0x08);

    // --------------------------------------------------------
    // lib::StaticArray<EntityHandle, 256, 4>（32 字节）
    // --------------------------------------------------------
    struct EntityHandleArray
    {
        void* vftable{ nullptr };     // +0x00
        EntityHandle* data{ nullptr };// +0x08
        u64 count{ 0 };               // +0x10 当前数量
        u64 capacity{ 0 };            // +0x18 恒为 256
    };
    static_assert(sizeof(EntityHandleArray) == 32);

    // --------------------------------------------------------
    // 4x4 行主序矩阵，row3 = 平移（引擎为行向量约定）
    // --------------------------------------------------------
    struct Matrix44
    {
        f32 m[4][4]{};

        [[nodiscard]] f32& X() noexcept { return m[3][0]; }
        [[nodiscard]] f32& Y() noexcept { return m[3][1]; }
        [[nodiscard]] f32& Z() noexcept { return m[3][2]; }
        [[nodiscard]] constexpr bool AllFinite() const noexcept
        {
            for (auto& row : m)
                for (f32 v : row)
                    if (!std::isfinite(v)) return false;
            return true;
        }
    };

    // --------------------------------------------------------
    // 实体根部（cModel → cObj → Behavior → EmBase/Pl0000）
    // 只声明 ESP 需要的头部，+0x90 之后不建模。
    // --------------------------------------------------------
    struct EntityBase
    {
        void* vftable{ nullptr }; // +0x000
        std::array<std::byte, 8> un_pad08; // +0x008 用途未知
        Matrix44 world;           // +0x010 世界矩阵；+0x2C 恰为其 row1.w 槽位
        Matrix44 dest;            // +0x050 目标/覆盖矩阵（脚本 setPos* 写入）

        [[nodiscard]] bool IsAlive() const noexcept
        {
            // +0x2C 低 2 位 = 0 视为存活（静态证据，待运行时复核）
            const auto bits = *reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(this) + 0x2C);
            return (bits & 3) == 0;
        }
    };
    static_assert(sizeof(EntityBase) == 0x90);
    static_assert(offsetof(EntityBase, world) == 0x10);
    static_assert(offsetof(EntityBase, dest) == 0x50);

    // --------------------------------------------------------
    // EmBaseManagerImplement（0x1520 = 5408 字节）
    // 5 个列表的类别语义待运行时确认，暂以 list0~list4 命名。
    // --------------------------------------------------------
    struct EmBaseManagerImplement
    {
        std::array<std::byte, 0x08> un_pad00;
        EntityHandleArray list0;                    // 0x008
        std::array<std::byte, 0x400> un_pad28;      // 0x028
        EntityHandleArray list1;                    // 0x428
        std::array<std::byte, 0x410> un_pad448;     // 0x448
        EntityHandleArray list2;                    // 0x858
        std::array<std::byte, 0x400> un_pad878;     // 0x878
        EntityHandleArray list3;                    // 0xC78
        std::array<std::byte, 0x400> un_padC98;     // 0xC98
        EntityHandleArray list4;                    // 0x1098
        std::array<std::byte, 0x468> un_pad10B8;    // 0x10B8（尾部含临界区 0x14D8）
    };
    static_assert(sizeof(EmBaseManagerImplement) == 0x1520);
    static_assert(offsetof(EmBaseManagerImplement, list0) == 0x08);
    static_assert(offsetof(EmBaseManagerImplement, list4) == 0x1098);

    // --------------------------------------------------------
    // 相机（cCameraGame，0x8C20 字节，内嵌全局对象）
    // --------------------------------------------------------
    struct CameraGame
    {
        std::array<std::byte, 0x10> un_pad00;   // vftable 区
        Matrix44 xfm;                           // +0x010 视图/相机矩阵候选
        std::array<std::byte, 0x8C20 - 0x50> un_pad50;
    };
    static_assert(sizeof(CameraGame) == 0x8C20);
    static_assert(offsetof(CameraGame, xfm) == 0x10);
}
