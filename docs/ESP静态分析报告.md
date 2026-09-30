# NieR:Automata ESP 静态分析报告

> 目标模块：`NieRAutomata.exe`（Steam 版）
> 基址：`0x140000000`，镜像大小 `0x18F7000`
> SHA256：`76a6924cc7dce7b34d2f2745f9ad96c97dcb87d146d31874a58fd3f53355f95d`
> MD5：`ed6c35a8d8083c142eec09c59c4ec1bd`
> 引擎：Platinum Games 自研 + hap 脚本系统（字符串哈希查找，内嵌 mruby）
> 分析方式：IDA 静态分析（RTTI + 脚本导出器注册链回溯）。所有偏移均为 **RVA**（VA − 0x140000000）。
> ⚠️ 本报告只覆盖这一个版本的二进制，游戏更新后偏移会变。

---

## 1. 总览：ESP 数据链

```
全局实体表 (base+0x1029520)                ← 最快路径，直接遍历
        │  16字节/条目 {校验, Entity*}
        ▼
Entity (cModel/cObj 派生, Behavior 基类)
        ├─ +0x10  4×4 世界矩阵（+0x40 = 平移 x,y,z）← ESP 画框坐标
        └─ +0x2C  低2位 = 存活标志

对象管理器 [base+0x10177E8]                ← 备选路径，按类别分组
        └─ 5 × StaticArray<EntityHandle,256>
                └─ EntityHandle(u32) → 全局实体表解析 → Entity*

相机 [base+0x1020870]                      ← W2S
        └─ +0x10 起 4×4 矩阵（视图/相机变换候选）
```

---

## 2. 结构体定义（可直接用于工程）

```cpp
#include <cstdint>

#if defined(_MSC_VER)
#define RVA(ptr, off) (reinterpret_cast<uint8_t*>(ptr) + (off))
#endif

namespace nier
{
using u8  = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;

// ============================================================
// EntityHandle：4 字节句柄（不是裸指针！）
// 布局（见解析函数 0x140745C50）：
//   bit 0~7   : 类型标记（解析时不参与校验）
//   bit 8~23  : 全局实体表索引
//   bit 24~31 : 代数/校验位
// ============================================================
struct EntityHandle
{
    u32 bits;

    u32 index() const       { return (bits >> 8) & 0xFFFF; }
    bool is_valid() const   { return bits != 0; }
};

// ============================================================
// 全局实体表条目（16 字节）
// 表基址：base + 0x1029520，容量 0x4000 (16384) 条
// 初始化函数：0x140746210（分配 0x40000 字节清零）
// ============================================================
struct EntityTableEntry
{
    u32      check;      // +0x00 与 handle 的 bit8~31 异或比对
    u32      un_pad04;   // +0x04
    void*    entity;     // +0x08 Entity*（Behavior 派生对象）
    u64      un_pad10;   // +0x0C 尚未分析
};
static_assert(sizeof(EntityTableEntry) == 16);

// ============================================================
// lib::StaticArray<EntityHandle, 256, 4>（32 字节）
// ============================================================
struct EntityHandleArray
{
    void*           vftable;   // +0x00 lib::StaticArray<EntityHandle,256,4>
    EntityHandle*   data;      // +0x08
    u64             count;     // +0x10 当前数量
    u64             capacity;  // +0x18 恒为 256
};
static_assert(sizeof(EntityHandleArray) == 32);

// ============================================================
// EmBaseManagerImplement（0x1520 = 5408 字节）
// 单例指针：[base + 0x10177E8]
// 构造函数：0x140669FF0（从内存池分配）
// 5 个列表的语义（玩家/敌人/NPC/友军/飞行单位）未经运行时确认，
// 先按 list0~list4 命名。
// ============================================================
struct EmBaseManagerImplement
{
    u8                 un_pad00[0x08];  // 0x000
    EntityHandleArray  list0;           // 0x008
    u8                 un_pad28[0x400]; // 0x028
    EntityHandleArray  list1;           // 0x428
    u8                 un_pad448[0x410];// 0x448
    EntityHandleArray  list2;           // 0x858
    u8                 un_pad878[0x400];// 0x878
    EntityHandleArray  list3;           // 0xC78
    u8                 un_padC98[0x400];// 0xC98
    EntityHandleArray  list4;           // 0x1098
    u8                 un_pad10B8[0x468];// 0x10B8（尾部含临界区 +0x14D8 等）
};
// 大小 0x1520 = 5408，与构造函数分配值一致

// ============================================================
// 4×4 行主序矩阵（实体的矩阵是"行向量"布局，
// 最后一行 = 平移，即 m[3][0..2] = x,y,z）
// ============================================================
struct Matrix44
{
    f32 m[4][4];
    // 平移分量快捷访问
    f32& x() { return m[3][0]; }
    f32& y() { return m[3][1]; }
    f32& z() { return m[3][2]; }
};

// ============================================================
// 实体根部（cModel → cObj → Behavior → EmBase/Pl0000）
// 世界矩阵由 cModel 构造（0x14084A280 → 0x140844E40 → 0x140192650）
// 以单位阵初始化，运行时每帧更新。
// ============================================================
struct EntityBase
{
    void*     vftable;      // 0x000 cModel/Behavior/EmBase::vftable（随派生层级变化）
    Matrix44  world;        // 0x010 世界矩阵；平移 = 0x40(x,y,z)，0x4C = 1.0f
                            //       ⚠ +0x2C 位于矩阵 row1.w 槽位，
                            //         同时被句柄解析当作存活标志读（&3），
                            //         运行时务必验证
    Matrix44  dest;         // 0x050 目标/覆盖矩阵（setPosRotResetHap 等脚本写这里，
                            //       初值除 +0x6C=1.0f 外全 0）
    u8        un_pad90[0x560]; // 0x090 起：碰撞(ExCollision@0x320 等)……

    // 便捷取坐标
    void get_pos(f32 out[3]) const
    {
        out[0] = *reinterpret_cast<const f32*>(RVA(this, 0x40));
        out[1] = *reinterpret_cast<const f32*>(RVA(this, 0x44));
        out[2] = *reinterpret_cast<const f32*>(RVA(this, 0x48));
    }
    bool is_alive() const
    {
        return (*reinterpret_cast<const u32*>(RVA(this, 0x2C)) & 3) == 0;
    }
};

// 以下为 Behavior 层及更深的已确认字段（不单独建结构体，按偏移取）
//   0x0670  状态字段（setPosRotResetHap 检查 != 211 时才写坐标）
//   0x17810 / 0x17820  历史位置镜像（cVec4）
//   0x2BE20 附近：计时/浮点字段（0x2BDA8~0x2BE24，浮点衰减）
//   EmBase::setGoPoint 写 0x2C030(u32 标志) / 0x2C038(cVec4)
//   EmBase::setSlideEndPos 写 0x27600(cVec4)

// ============================================================
// cCameraGame（0x8C20 = 35872 字节）
// 单例：[base + 0x1020870]
// 静态初始化：0x140101CC0（确认单例地址）
// 每帧注册监听：0x140101E50 → update = 0x14073D900
// ============================================================
struct CameraGame
{
    void*     vftable;        // 0x000 Fw::cCameraBase
    Matrix44  xfm;            // 0x010 相机变换矩阵候选（视图矩阵需运行时确认）
    f32       un_pad50;       // 0x050 恒 1.0f
    u8        un_pad54[0x7CC]; // 0x054（0x820 CameraParam、0x8E0 CameraParamA~Z 等）
    // 0x834（a1+2100）相机参数对象，0x998（a1+2456）当前参数块
};

} // namespace nier
```

---

## 3. 关键全局变量速查表

| RVA | 类型 | 含义 | 置信度 |
|---|---|---|---|
| `0x1029520` | `EntityTableEntry*` | 全局实体表基址（0x4000 条） | ✅ 静态实锤（初始化函数确认） |
| `0x1029510` | `u64` | 实体表数量上界 | ✅ |
| `0x1029518` | `u64` | 实体表已用计数 | ✅ |
| `0x10177E8` | `EmBaseManagerImplement*` | 对象管理器单例 | ✅（构造函数尾部写入） |
| `0x1020870` | `CameraGame` | 相机单例（内嵌，非指针） | ✅（静态初始化确认） |
| `0x13DF814` | `f32` | 世界速度（`-fabs(spd)`，脚本 setWorldSpd） | ✅ |
| `0x13DF818` | `f32` | 世界速度第二参数 | ✅ |

### 关键函数（需要调用/参考时）

| VA | 原型 | 作用 |
|---|---|---|
| `0x140745C50` | `Entity* resolve(EntityHandle* h)` | 句柄→实体完整解析（含校验） |
| `0x1405E86F0` | `EmBaseManagerImplement* getMgr()` | 管理器单例 getter |
| `0x140266760` | `f32 dist2(cVec4* a, cVec4* b)` | 三维距离平方 |
| `0x1404F0EA0` | `EmBase::setPosRotResetHap` | 坐标写入路径（证明 +0x50 链路） |
| `0x140746210` | `bool initTable()` | 实体表初始化 |
| `0x1406E53A0` | `EmBase::searchTarget` | 官方目标搜索（遍历+过滤的参考实现） |

---

## 4. 句柄解析算法（逆向自 0x140745C50）

```cpp
EntityTableEntry* g_table = *(EntityTableEntry**)(base + 0x1029520);
u64               g_bound = *(u64*)(base + 0x1029510);

nier::EntityBase* resolve(EntityHandle h)
{
    if (!h.is_valid())                     return nullptr;
    if (h.index() >= g_bound)              return nullptr;

    EntityTableEntry* e = &g_table[h.index()];
    if (((h.bits ^ e->check) & 0xFFFFFF00) != 0)  // bit8~31 校验
        return nullptr;
    if (!e->entity || !is_alive(e->entity))
        return nullptr;
    return static_cast<nier::EntityBase*>(e->entity);
}
```

## 5. ESP 主循环参考

```cpp
void esp_iterate()
{
    // 路径 A：直接扫全局表（推荐，最简单）
    auto table = *(nier::EntityTableEntry**)(base + 0x1029520);
    for (u32 i = 0; i < 0x4000; ++i)
    {
        auto* ent = static_cast<nier::EntityBase*>(table[i].entity);
        if (!ent || !ent->is_alive()) continue;      // ⚠ +0x2C 标志需运行时验证

        f32 pos[3]; ent->get_pos(pos);               // +0x40 x,y,z
        // → 用 ViewProj 做 W2S → 画框
    }

    // 路径 B：走管理器（需要区分实体类别时用）
    auto* mgr = *(nier::EmBaseManagerImplement**)(base + 0x10177E8);
    for (auto* arr : { &mgr->list0, &mgr->list1, &mgr->list2,
                       &mgr->list3, &mgr->list4 })
    {
        for (u64 k = 0; k < arr->count; ++k)
        {
            auto* ent = resolve(arr->data[k]);
            if (ent) { /* 同上 */ }
        }
    }
}
```

---

## 6. 尚未确认 / 风险点

| 事项 | 状态 | 建议验证方法 |
|---|---|---|
| **ViewProj 矩阵** | ❌ 未静态锁定（运行时生成） | DX11 hook `VSSetConstantBuffers`/`Map` 截 WVP 常量缓冲（最稳）；或 CE 监视 `[base+0x1020870]+0x10` 的 64 字节随视角变化 |
| 相机 +0x10 矩阵语义（视图 or 相机世界） | ⚠ 候选 | 移动视角观察变化；与世界矩阵对比行向量方向 |
| 5 个 EntityHandle 列表各自的类别 | ⚠ 推测 | 进游戏后逐列表 dump `count` 并对照场上实体 |
| `+0x2C` 存活标志与矩阵 row1.w 共位 | ⚠ 共位确认过、语义需验证 | 死亡/销毁实体时观察该字节低 2 位 |
| 实体类型区分（玩家 vs 敌人 vs NPC） | ⚠ 未做 | handle 低 8 位类型标记；或比对 vftable（EmBase `0x140C77340`、Pl0000 `0x140C5EBF0`） |
| Pl0000/EmBase +0x50（dest 矩阵）与 +0x40（world 平移） | ✅ 双重复证 | — |

## 7. 运行时验证清单（写代码前先过一遍）

1. CE 附加游戏，读 `[base+0x1029520]`，遍历非空条目，确认指针有效且 `+0x40` 处浮点 = 玩家/敌人坐标（走动时变化）。
2. 读 `[base+0x10177E8]` → `list*.count`，对照场上可见实体数量。
3. 移动视角，确认 `[base+0x1020870]+0x10` 矩阵变化规律；同时 hook DX11 找 WVP。
4. 杀一个怪，确认其 `+0x2C` 低 2 位翻转 / 条目被清。

---
*报告生成：IDA 静态分析会话 2026-09-30。所有"✅"结论均有反编译证据，"⚠"项请在运行时过一遍清单第 7 节。*
