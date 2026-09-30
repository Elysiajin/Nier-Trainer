#pragma once
// ============================================================
// 游戏数据层：从 NieRAutomata.exe 内存中收集实体快照
// 每帧 Refresh() 重建快照缓冲，渲染层只消费 std::span。
// ============================================================

#include "Offsets.h"
#include "Structs.h"

#include <optional>
#include <span>
#include <vector>

namespace nier::game
{
    struct Vec3
    {
        f32 x{ 0.0f };
        f32 y{ 0.0f };
        f32 z{ 0.0f };
    };

    enum class EntityKind : u8
    {
        Unknown,
        Player,   // Pl0000
        Enemy,    // EmBase 派生
    };

    struct EntitySnapshot
    {
        EntityHandle handle;
        EntityKind kind{ EntityKind::Unknown };
        u32 tableIndex{ 0 };
        Vec3 pos;
    };

    // 主模块基址（NieRAutomata.exe），失败返回 nullptr
    [[nodiscard]] u8* BaseAddress() noexcept;

    // SEH 保护的内存读取：任何指针在解引用前都必须经过这里
    [[nodiscard]] bool SafeLoad(void* dst, const void* src, std::size_t size) noexcept;

    template <typename T>
    [[nodiscard]] std::optional<T> TryLoad(const void* src) noexcept
    {
        T value{};
        if (!SafeLoad(&value, src, sizeof(T)))
            return std::nullopt;
        return value;
    }

    // 按句柄解析实体指针（逆向自 0x140745C50，含校验与存活判定）
    [[nodiscard]] EntityBase* Resolve(EntityHandle handle) noexcept;

    class EntityCache
    {
    public:
        // 重新扫描全局实体表，返回本帧快照。
        // 快照缓冲内部复用，调用方只在同帧内持有 span。
        [[nodiscard]] std::span<const EntitySnapshot> Refresh();

        // 相机矩阵候选（cCameraGame +0x10），语义待运行时确认；
        // 读取失败或含 NaN/Inf 返回 nullopt。
        [[nodiscard]] std::optional<Matrix44> TryFetchCameraMatrix();

    private:
        [[nodiscard]] EntityKind Classify(const EntityBase* entity) const noexcept;

        std::vector<EntitySnapshot> m_entries;
    };

    // 全局缓存实例（每帧仅 Overlay 线程访问，无需加锁）
    inline EntityCache g_entityCache;
}
