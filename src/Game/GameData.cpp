#include "GameData.h"

#include <windows.h>
#include <cstring>

namespace nier::game
{
    // ============================================================
    // 边界层：SEH 保护的内存读取
    // 注意：本函数内不得使用带析构的 C++ 对象（C2712）
    // ============================================================
    bool SafeLoad(void* dst, const void* src, std::size_t size) noexcept
    {
        __try
        {
            const auto* srcBytes = static_cast<const unsigned char*>(src);
            // 首尾字节探针：触发跨页无效访问，交给 SEH 兜底
            volatile unsigned char probe = srcBytes[0];
            probe = srcBytes[size - 1];
            (void)probe;
            memcpy(dst, src, size);
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                      ? EXCEPTION_EXECUTE_HANDLER
                      : EXCEPTION_CONTINUE_SEARCH)
        {
            return false;
        }
    }

    u8* BaseAddress() noexcept
    {
        static u8* cached = []() -> u8* {
            HMODULE mod = ::GetModuleHandleA(nullptr);
            return reinterpret_cast<u8*>(mod);
        }();
        return cached;
    }

    // ============================================================
    // 句柄解析（证据：docs/ESP静态分析报告.md 第 4 节）
    // ============================================================
    EntityBase* Resolve(EntityHandle handle) noexcept
    {
        if (!handle.IsValid())
            return nullptr;

        const u8* base = BaseAddress();
        if (!base)
            return nullptr;

        const auto boundOpt = TryLoad<u64>(base + offsets::kEntityTableBound);
        if (!boundOpt || handle.Index() >= boundOpt.value())
            return nullptr;

        const auto entryOpt = TryLoad<EntityTableEntry>(
            base + offsets::kEntityTable + std::ptrdiff_t(handle.Index()) * sizeof(EntityTableEntry));
        if (!entryOpt)
            return nullptr;

        const EntityTableEntry& entry = entryOpt.value();
        if (((handle.bits ^ entry.check) & 0xFFFFFF00u) != 0)
            return nullptr;

        auto* entity = static_cast<EntityBase*>(entry.entity);
        if (!entity)
            return nullptr;

        // 存活判定走 SafeLoad，避免裸解引用悬垂指针
        const auto flagsOpt = TryLoad<u32>(reinterpret_cast<const u8*>(entity) + offsets::kEntityFlags);
        if (!flagsOpt || (flagsOpt.value() & 3) != 0)
            return nullptr;
        return entity;
    }

    // ============================================================
    // 实体分类：比对 vftable
    // ============================================================
    EntityKind EntityCache::Classify(const EntityBase* entity) const noexcept
    {
        const u8* base = BaseAddress();
        if (!base || !entity)
            return EntityKind::Unknown;

        const auto vftableOpt = TryLoad<void*>(entity);
        if (!vftableOpt)
            return EntityKind::Unknown;

        const auto vftable = reinterpret_cast<std::uintptr_t>(vftableOpt.value());
        if (vftable == reinterpret_cast<std::uintptr_t>(base + offsets::kVftablePl0000))
            return EntityKind::Player;
        if (vftable == reinterpret_cast<std::uintptr_t>(base + offsets::kVftableEmBase))
            return EntityKind::Enemy;
        return EntityKind::Unknown;
    }

    // ============================================================
    // 快照收集：直接扫全局实体表（0x4000 条，每帧一遍开销可控）
    // ============================================================
    std::span<const EntitySnapshot> EntityCache::Refresh()
    {
        m_entries.clear();

        const u8* base = BaseAddress();
        if (!base)
            return {};

        const auto tableOpt = TryLoad<EntityTableEntry*>(base + offsets::kEntityTable);
        const auto boundOpt = TryLoad<u64>(base + offsets::kEntityTableBound);
        if (!tableOpt || !boundOpt)
            return {};

        const EntityTableEntry* table = tableOpt.value();
        const u64 bound = boundOpt.value();
        if (!table || bound == 0)
            return {};

        const u32 tableSize = std::min<u64>(bound, offsets::kEntityTableCount);
        m_entries.reserve(std::min<u64>(tableSize, 1024));

        for (u32 i = 0; i < tableSize; ++i)
        {
            const auto entryOpt = TryLoad<EntityTableEntry>(&table[i]);
            if (!entryOpt)
                continue;

            const EntityTableEntry& entry = entryOpt.value();
            auto* entity = static_cast<EntityBase*>(entry.entity);
            if (!entity)
                continue;

            EntitySnapshot snapshot;
            snapshot.handle.bits = entry.check; // bit8~31 与表内一致，重建 handle
            snapshot.tableIndex = i;
            snapshot.kind = Classify(entity);

            const auto worldOpt = TryLoad<Matrix44>(reinterpret_cast<const u8*>(entity) + offsets::kEntityWorldMatrix);
            if (!worldOpt || !worldOpt->AllFinite())
                continue;

            snapshot.pos = { worldOpt->m[3][0], worldOpt->m[3][1], worldOpt->m[3][2] };

            if (snapshot.kind == EntityKind::Unknown)
                continue; // 未分类实体先不上屏，减少噪声

            m_entries.push_back(std::move(snapshot));
        }

        return m_entries;
    }

    // ============================================================
    // 相机矩阵候选：[base + kCameraGame] + kCameraMatrix
    // ⚠ 语义（视图 or 相机世界矩阵）待运行时确认，见报告第 6 节
    // ============================================================
    std::optional<Matrix44> EntityCache::TryFetchCameraMatrix()
    {
        const u8* base = BaseAddress();
        if (!base)
            return std::nullopt;

        const auto cameraOpt = TryLoad<CameraGame*>(base + offsets::kCameraGame);
        if (!cameraOpt || !cameraOpt.value())
            return std::nullopt;

        const auto* camera = cameraOpt.value();
        const auto matrixOpt = TryLoad<Matrix44>(
            reinterpret_cast<const u8*>(camera) + offsets::kCameraMatrix);
        if (!matrixOpt || !matrixOpt->AllFinite())
            return std::nullopt;

        return matrixOpt;
    }
}
