# -*- coding: utf-8 -*-
"""
ESP 数据链在线验证脚本（只读游戏内存）
用法：
  python tools/verify_esp.py            # 控制台验证（扫矩阵/实体/配对评分）
  python tools/verify_esp.py --overlay  # pygame 透明叠加层可视化验证
"""
import argparse
import struct
import time

import numpy as np
import pymem
import pymem.process

# ---- RVA（与 src/Game/Offsets.h 一致）----
K_CAMERA_GAME = 0x1020870
K_CAMERA_MATRIX = 0x10
K_ENTITY_TABLE = 0x1029520
K_ENTITY_TABLE_BOUND = 0x1029510
K_ENTITY_TABLE_COUNT = 0x4000
K_ENTITY_POS = 0x40
K_ENTITY_FLAGS = 0x2C

PROJ_SCAN_BEFORE = 0x4000
PROJ_SCAN_AFTER = 0x8C20 + 0x10000
VIEW_SCAN_BEFORE = 0x4000
VIEW_SCAN_AFTER = 0x8C20 + 0x40000


def log(msg):
    print(msg, flush=True)


def looks_like_projection(m):
    """严格透视指纹：排除单位阵变体"""
    return (
        np.isfinite(m[0, 0]) and 0.2 < m[0, 0] < 8.0
        and np.isfinite(m[1, 1]) and 0.2 < m[1, 1] < 8.0
        and m[0, 1] == 0 and m[0, 2] == 0 and m[0, 3] == 0
        and m[1, 0] == 0 and m[1, 2] == 0 and m[1, 3] == 0
        and m[3, 0] == 0 and m[3, 1] == 0 and m[3, 3] == 0
        and (m[2, 3] == 1.0 or m[2, 3] == -1.0)
        and np.isfinite(m[2, 2]) and abs(m[2, 2] - 1.0) > 1e-4
        and np.isfinite(m[3, 2]) and abs(m[3, 2]) > 1e-3
    )


def looks_like_view(m):
    """视图矩阵指纹：rows0-2 正交单位基，col3 平移，row3=(0,0,0,1)"""
    r = m[:3, :3]
    lens = np.linalg.norm(r, axis=1)
    dots = np.abs([r[0] @ r[1], r[0] @ r[2], r[1] @ r[2]])
    return (bool(np.all(np.abs(lens - 1.0) < 0.02))
            and bool(np.all(dots < 0.02))
            and abs(m[0, 3]) < 1e-5 and abs(m[1, 3]) < 1e-5 and abs(m[2, 3]) < 1e-5
            and abs(m[3, 3] - 1.0) < 1e-5
            and bool(np.all(np.isfinite(m))))


def scan_windows(pm, addr, size):
    """读内存并返回 sliding_window_view(16 floats) 及首地址数组"""
    try:
        buf = pm.read_bytes(addr, size)
    except Exception as ex:
        log(f"  (读取失败 {ex})")
        return None, None, 0
    arr = np.frombuffer(buf, dtype=np.float32)
    n = len(arr) - 16
    if n <= 0:
        return None, None, 0
    win = np.lib.stride_tricks.sliding_window_view(arr, 16)
    return win, arr, n


def scan_proj_vec(pm, addr, size, max_hits=32):
    """向量化透视矩阵指纹"""
    win, arr, n = scan_windows(pm, addr, size)
    if win is None:
        return []
    c = lambda i: win[:, i]
    mask = (
        (c(0) > 0.2) & (c(0) < 8.0) & (c(1) == 0) & (c(2) == 0) & (c(3) == 0)
        & (c(4) == 0) & (c(5) > 0.2) & (c(5) < 8.0) & (c(6) == 0) & (c(7) == 0)
        & (c(8) == 0) & (c(9) == 0)
        & ((c(11) == 1.0) | (c(11) == -1.0))
        & (c(12) == 0) & (c(13) == 0) & (c(15) == 0)
        & (np.abs(c(10) - 1.0) > 1e-4) & (np.abs(c(14)) > 1e-3)
        & np.isfinite(c(0)) & np.isfinite(c(10)) & np.isfinite(c(14))
    )
    idx = np.nonzero(mask)[0][:max_hits]
    return [(int(i) * 4, win[i].reshape(4, 4).copy()) for i in idx]


def scan_view_vec(pm, addr, size, max_hits=32):
    """向量化正交单位基指纹（行向量视图矩阵）"""
    win, arr, n = scan_windows(pm, addr, size)
    if win is None:
        return []
    c = lambda i: win[:, i].astype(np.float64)
    with np.errstate(over="ignore", invalid="ignore"):
        len0 = np.sqrt(c(0)**2 + c(1)**2 + c(2)**2)
        len1 = np.sqrt(c(4)**2 + c(5)**2 + c(6)**2)
        len2 = np.sqrt(c(8)**2 + c(9)**2 + c(10)**2)
        d01 = c(0)*c(4) + c(1)*c(5) + c(2)*c(6)
        d02 = c(0)*c(8) + c(1)*c(9) + c(2)*c(10)
        d12 = c(4)*c(8) + c(5)*c(9) + c(6)*c(10)
        mask = (
            (np.abs(len0 - 1) < 0.02) & (np.abs(len1 - 1) < 0.02) & (np.abs(len2 - 1) < 0.02)
            & (np.abs(d01) < 0.02) & (np.abs(d02) < 0.02) & (np.abs(d12) < 0.02)
            & (np.abs(c(3)) < 1e-5) & (np.abs(c(7)) < 1e-5) & (np.abs(c(11)) < 1e-5)
            & (np.abs(c(15) - 1.0) < 1e-5)
        )
    idx = np.nonzero(mask)[0][:max_hits]
    return [(int(i) * 4, win[i].reshape(4, 4).copy()) for i in idx]


def rtti_name(pm, base, entity):
    try:
        vf = struct.unpack("<Q", pm.read_bytes(entity, 8))[0]
        col = struct.unpack("<Q", pm.read_bytes(vf - 8, 8))[0]
        td_rva = struct.unpack("<I", pm.read_bytes(col + 0x0C, 4))[0]
        raw = pm.read_bytes(base + td_rva + 0x10, 48)
        return raw.split(b"\x00")[0].decode("ascii", errors="replace")
    except Exception as ex:
        return f"<fail:{type(ex).__name__}>"


def collect_entities(pm, base):
    table = struct.unpack("<Q", pm.read_bytes(base + K_ENTITY_TABLE, 8))[0]
    bound = struct.unpack("<Q", pm.read_bytes(base + K_ENTITY_TABLE_BOUND, 8))[0]
    n = min(bound, K_ENTITY_TABLE_COUNT)
    ents = []
    for i in range(n):
        try:
            check, _pad, ptr = struct.unpack("<IIQ", pm.read_bytes(table + i * 16, 16))
            flags = struct.unpack("<I", pm.read_bytes(ptr + K_ENTITY_FLAGS, 4))[0]
            pos = np.frombuffer(pm.read_bytes(ptr + K_ENTITY_POS, 16), dtype=np.float32)
        except Exception:
            continue
        if not ptr or (flags & 3):
            continue
        if not np.all(np.isfinite(pos)) or np.linalg.norm(pos[:3]) > 1e6:
            continue
        ents.append({"index": i, "ptr": ptr, "check": check,
                     "pos": pos[:3].copy(), "w": float(pos[3])})
    return ents


def verify_console(pm, base):
    cam_addr = base + K_CAMERA_GAME

    ents = collect_entities(pm, base)
    log(f"[ents] 有效实体: {len(ents)}")
    if not ents:
        return
    positions = np.array([e["pos"] for e in ents], dtype=np.float32)

    names = {}
    for e in ents[:300]:
        nm = rtti_name(pm, base, e["ptr"])
        names[nm] = names.get(nm, 0) + 1
    top = sorted(names.items(), key=lambda kv: -kv[1])[:12]
    log("[rtti] 类型Top12: " + ", ".join(f"{(k or '?')[:32]}×{v}" for k, v in top))

    # ---- 全 .data 段向量化扫描（RVA 0xF04000 ~ 0x17AF000）----
    DATA_START = 0xF04000
    DATA_SIZE = 0x17AF000 - 0xF04000
    log(f"[scan] 扫描 .data: {DATA_SIZE/1024/1024:.1f} MB ...")
    vhits = scan_view_vec(pm, base + DATA_START, DATA_SIZE)
    log(f"[view] 正交基指纹命中 {len(vhits)} 处: " +
        ", ".join(f"rva{o + DATA_START:#x}" for o, _ in vhits[:16]))

    phits = scan_proj_vec(pm, base + DATA_START, DATA_SIZE)
    log(f"[proj] 严格指纹命中 {len(phits)} 处: " +
        ", ".join(f"rva{o + DATA_START:#x}" for o, _ in phits[:16]))

    if not phits or not vhits:
        log("[pair] 缺少候选，无法配对")
        return

    # ---- 配对评分：正确的视图矩阵应让屏幕中心附近存在实体（相机注视玩家）----
    best = []
    for vo, V in vhits:
        for po, P in phits:
            vh = np.hstack([positions, np.ones((len(positions), 1), np.float32)]) @ V
            w = vh[:, 2] * P[2, 3]
            ok = w > 0.05
            if not ok.any():
                continue
            ndc_x = vh[:, 0] * P[0, 0] / np.where(ok, w, 1)
            ndc_y = vh[:, 1] * P[1, 1] / np.where(ok, w, 1)
            onscreen = ok & (np.abs(ndc_x) <= 1.0) & (np.abs(ndc_y) <= 1.0)
            n_on = int(onscreen.sum())
            center = int((onscreen & (np.abs(ndc_x) < 0.2) & (np.abs(ndc_y) < 0.2)).sum())
            best.append((n_on + center * 10, vo, po, n_on, center))
    best.sort(reverse=True)
    for score, vo, po, n_on, center in best[:5]:
        log(f"[pair] score={score:4d}  view@rva{vo + DATA_START:#x}  "
            f"proj@rva{po + DATA_START:#x}  onscreen={n_on}  center={center}")

    # ---- 展示最优配对下的投影明细 ----
    if best:
        _, vo, po, _, _ = best[0]
        V = dict(vhits)[vo]
        P = dict(phits)[po]
        log(f"[view*] @ cam{vo - VIEW_SCAN_BEFORE:+#x}:\n"
            f"{np.array2string(V, precision=3, suppress_small=True)}")
        log(f"[proj*] @ cam{po - PROJ_SCAN_BEFORE:+#x}:\n"
            f"{np.array2string(P, precision=4, suppress_small=True)}")
        vh = np.hstack([positions, np.ones((len(positions), 1), np.float32)]) @ V
        w = vh[:, 2] * P[2, 3]
        depth = np.linalg.norm(vh[:, :3], axis=1)
        order = np.argsort(depth)
        log("[near] 最近 8 个实体（应为玩家/视角附近目标）:")
        for i in order[:8]:
            ndx = vh[i, 0] * P[0, 0] / w[i] if w[i] > 0 else float("nan")
            ndy = vh[i, 1] * P[1, 1] / w[i] if w[i] > 0 else float("nan")
            nm = rtti_name(pm, base, ents[i]["ptr"]) or "?"
            log(f"    pos=({positions[i][0]:8.1f},{positions[i][1]:7.1f},{positions[i][2]:8.1f}) "
                f"depth={depth[i]:8.1f} ndc=({ndx:6.2f},{ndy:6.2f}) {nm[:28]}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--overlay", action="store_true")
    args = ap.parse_args()

    pm = pymem.Pymem("NieRAutomata.exe")
    mod = pymem.process.module_from_name(pm.process_handle, "NieRAutomata.exe")
    base = mod.lpBaseOfDll
    log(f"[attach] base = {base:#x}")

    if args.overlay:
        import verify_overlay
        verify_overlay.run(pm, base)
    else:
        verify_console(pm, base)


if __name__ == "__main__":
    main()
