# NieR-Trainer 编码约束

> 适用范围：`src/` 下所有新增与重构代码。`third_party/` 不受本约束限制。
> 存量代码按第 8 节的迁移策略逐步向约束靠拢，不做一次性大改名。

## 1. 语言标准

- 统一 **C++20**（`CMAKE_CXX_STANDARD 20`），可用：`std::span`、`std::optional`、`std::string_view`、`std::byte`、指定初始化器、`std::jthread`。
- 禁止降级到 C 风格实现"图省事"的写法（见第 3、5 节）。

## 2. 命名

| 对象 | 规则 | 示例 |
|---|---|---|
| 类型 / 枚举值 | PascalCase | `EntityTableEntry`、`EntityKind::Enemy` |
| 函数 | PascalCase | `Refresh()`、`WorldToScreen()` |
| 局部变量 / 参数 | lowerCamelCase | `tableBase`、`maxDistance` |
| 成员变量 | `m_` + lowerCamelCase | `m_baseAddress` |
| 常量 / RVA | `k` + PascalCase | `kEntityTable` |
| 命名空间 | 游戏数据 `nier::game`，渲染 `nier::esp`，RVA 常量 `nier::offsets` | |

- 逆向出的结构体占位字段统一叫 `un_pad<十六进制偏移>`，如 `un_pad28`、`un_pad448`，一眼可对 IDA。
- 禁止新增匈牙利前缀（`b`、`p`、`lp`）；存量不改名，但新代码不扩散。

## 3. 类型转换

- **禁止 C 风格转换** `(T)x`，一律 `static_cast` / `reinterpret_cast` / `std::bit_cast`。
- `reinterpret_cast` 只允许出现在"内存边界"处：读游戏结构、虚表、指针位运算。业务逻辑里不得出现。
- COM 接口取址一律 `IID_PPV_ARGS(&obj)`，禁止 `(void**)&obj`。
- `__uuidof` 传类型本身，禁止传 `Type*`。

## 4. 枚举

- 一律 `enum class`，并显式指定底层类型：
  `enum class EntityKind : std::uint8_t { Unknown, Player, Enemy, Npc };`
- 禁止裸 `enum`。虚表槽位等"整型语义"的常量用 `constexpr` 整数而不是枚举。

## 5. STL 优先

| 场景 | 用 | 不用 |
|---|---|---|
| 动态数组 | `std::vector` + `std::span` 出参 | 裸指针+长度 |
| 可失败返回 | `std::optional` / `bool` + 出参 | 指针出参约定俗成 |
| 字符串传参 | `std::string_view` | `const char*`（仅 Win32 API 边界可用） |
| 格式化 | `std::format` | `sprintf` / `snprintf` |
| 时间 | `std::chrono::steady_clock` | `GetTickCount` 毫秒算术 |
| 线程 | `std::jthread` | 裸 `CreateThread`（`DllMain` 内除外，见 9） |
| 内存 | `std::make_unique` / 容器 | `new` / `delete` / `malloc` |
| 校验和断言 | `assert` / `static_assert`（结构体尺寸必须断言） | 注释"这里应该是 xx 字节" |

- Win32 / COM API 属于边界，允许直接调用，但封装在"边界层"（如 `Engine`、`safe_load`），不渗入业务逻辑。

## 6. 错误处理

- Hook 回调与每帧渲染路径**禁止抛异常、禁止分配可能失败的重量级资源**；失败路径静默降级（跳过本帧）。
- 可预期失败用 `std::optional` / `bool` 返回；不可恢复错误记日志后停用功能。
- 游戏内存读取必须经 `game::SafeLoad`（SEH 保护），禁止裸解引用未验证的指针。

## 7. 头文件与组织

- `#pragma once`；include 按"本文件对应头 → 项目头 → 第三方 → 标准库 → 系统头"分组。
- 结构体布局只声明在 `Game/Structs.h`，RVA 只声明在 `Game/Offsets.h`；其他文件不得出现魔法数字偏移。
- 每个结构体 `static_assert(sizeof(...))` 锁尺寸。

## 8. 存量代码迁移策略

- 改哪个文件就顺手把该文件迁到约束（enum class、cast、命名），不做全量重命名 commit。
- `Engine.h` 的 `LogFile` 暂保留 printf 风格（边界层），后续统一 `std::format`。

## 9. 已知例外

- `DllMain` 中创建初始化线程用 `CreateThread`（loader lock 下 `std::thread` 不可靠），属平台例外。
- MinHook 的 `LPVOID` 接口在 `Hooking.h` 边界内转换。
