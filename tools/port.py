#!/usr/bin/env python3
"""port.py REF: the decompilation's C sources the calculator build compiles, and the glue they need.

- copies from REF (the decompilation, see tools/fetch.py) into decomp/: its headers, the game's code
  (src/game, src/engine), the behaviors, the level scripts, and the data the game reads as C: collision,
  object placements, trajectories, paintings, animations. Not its display lists, models or textures:
  the calculator draws with its own (tools/pack.py);
- writes src/port/gen/symbols.c: every model, display list and segment the scripts name, as a small stub
  holding the index of the calculator's own model (src/data.bin), and the levels that are not in the game
  as a script that goes back to the castle;
- writes src/port/gen/terrain.c: each level area's collision cut into the game's 16 x 16 cells, sorted as
  the game sorts them, ready in flash (the game builds this in RAM when a level loads: 100 KB and more).

The calculator's own changes to the decompilation's files are marked "(calculator)" in them."""
import glob
import math
import os
import re
import shutil
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.join(HERE, "..")
DEC = os.path.join(GAME, "decomp")
GEN = os.path.join(GAME, "src", "port", "gen")

# the levels in the game (the others lead back to the castle)
LEVELS = ["castle_grounds", "castle_inside", "castle_courtyard", "bob"]


def copy(ref, rel):
    src = os.path.join(ref, rel)
    dst = os.path.join(DEC, rel)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if not os.path.exists(dst):
        shutil.copy(src, dst)


def copy_glob(ref, pattern):
    for f in sorted(glob.glob(os.path.join(ref, pattern), recursive=True)):
        copy(ref, os.path.relpath(f, ref))


# ------------------------------------------------------------------------------------------------ symbols
DECL = re.compile(r"extern\s+(const\s+)?(struct\s+)?(\w+)\s*(\*\s*const\s+)?(\w+)\s*(\[[^\]]*\])?\s*;")


def header_symbols(ref):
    """(type, name, pointer-array?) of everything the actor and level headers declare."""
    out = []
    for h in sorted(glob.glob(os.path.join(ref, "actors", "*.h")) + glob.glob(os.path.join(ref, "levels", "*", "header.h"))):
        for m in DECL.finditer(open(h).read()):
            out.append((m.group(3), m.group(5), bool(m.group(4)), os.path.relpath(h, ref)))
    return out


def level_data_files(ref, level):
    """The data .inc.c files of a level the game's code reads (not its models)."""
    keep = []
    for f in sorted(glob.glob(os.path.join(ref, "levels", level, "**", "*.inc.c"), recursive=True)):
        name = os.path.basename(f)
        if name in ("collision.inc.c", "macro.inc.c", "trajectory.inc.c", "room.inc.c") or name.startswith("trajectory"):
            keep.append(os.path.relpath(f, ref))
    return keep


def used_actors(ref):
    """The actors whose models the game's levels load (src/port/gen/models.txt, from tools/pack.py): only their
    animations and collision are kept, the others' are empty."""
    names = set()
    for line in open(os.path.join(GEN, "models.txt")):
        p = line.split()
        if p and p[0] in ("geo", "dl"):
            names.add(p[1])
    used = set()
    for d in sorted(glob.glob(os.path.join(ref, "actors", "*"))):
        if not os.path.isdir(d):
            continue
        for f in glob.glob(os.path.join(d, "*.inc.c")):
            if set(re.findall(r"(?:GeoLayout|Gfx)\s+(\w+)\s*\[", open(f).read())) & names:
                used.add(os.path.basename(d))
                break
    return used


def actor_data_files(ref):
    keep = []
    used = used_actors(ref)
    for f in sorted(glob.glob(os.path.join(ref, "actors", "*", "collision.inc.c"))):
        if os.path.basename(os.path.dirname(f)) in used:
            keep.append(os.path.relpath(f, ref))
    for d in sorted(glob.glob(os.path.join(ref, "actors", "*", "anims"))):
        if os.path.basename(os.path.dirname(d)) not in used:
            continue
        # data.inc.c includes the animations, table.inc.c lists them
        for f in ("data.inc.c", "table.inc.c"):
            if os.path.exists(os.path.join(d, f)):
                keep.append(os.path.relpath(os.path.join(d, f), ref))
    return keep


def actor_anim_files(ref):
    """Every file of the actors' animations (data.inc.c includes them by name)."""
    return [os.path.relpath(f, ref) for f in sorted(glob.glob(os.path.join(ref, "actors", "*", "anims", "*.inc.c")))]


def defined_in(path):
    """The array symbols a data .inc.c file defines."""
    txt = open(path).read()
    return set(re.findall(r"\b(?:Collision|MacroObject|Trajectory|RoomData|struct Painting|struct Animation|Movtex|"
                          r"struct MovtexQuadCollection|struct MovtexQuad)\s+(?:\*const\s+)?(\w+)\s*\[?", txt))


def write_symbols(ref, model_of):
    syms = header_symbols(ref)
    real = {}
    data_files = []
    for lv in LEVELS:
        data_files += level_data_files(ref, lv)
    data_files += actor_data_files(ref)
    for f in data_files:
        for s in defined_in(os.path.join(ref, f)):
            real[s] = f
    lines = ["/* Made by tools/port.py: what the decompilation's scripts and behaviors name, for the calculator. */",
             "#include <ultra64.h>", '#include "sm64.h"', '#include "types.h"', '#include "macros.h"',
             '#include "surface_terrains.h"', '#include "moving_texture_macros.h"', '#include "level_misc_macros.h"',
             '#include "macro_presets.h"', '#include "special_presets.h"', '#include "dialog_ids.h"',
             '#include "level_commands.h"', '#include "game/paintings.h"', '#include "game/moving_texture.h"',
             '#include "make_const_nonconst.h"', '#include "port.h"', ""]
    # the real data, file by file (each defines its own symbols)
    for f in data_files:
        lines.append(f'#include "{f}"')
    lines.append("")
    lines.append("/* a display list nobody draws: the calculator draws its own models */")
    lines.append("const Gfx port_dummy_dl[1] = {{{0, 0}}};")
    done = set(real)
    levels_scripts = set()
    for typ, name, ptrarr, hdr in syms:
        if name in done:
            continue
        done.add(name)
        if typ == "GeoLayout":
            m = model_of.get(name, 0xFFFF)
            if isinstance(m, tuple):
                v = m[1]
                lines.append(f"const GeoLayout {name}[] = {{ PORT_AREA({', '.join(map(str, v))}) }};")
            else:
                lines.append(f"const GeoLayout {name}[] = {{ PORT_MODEL({m}) }};")
        elif typ == "Gfx":
            if name in model_of:
                lines.append(f"const Gfx {name}[1] = {{{{{{PORT_MODEL_TAG, {model_of[name]}}}}}}};")
            else:
                lines.append(f'extern const Gfx {name}[] __attribute__((alias("port_dummy_dl")));')
        elif typ == "Collision":
            lines.append(f"const Collision {name}[] = {{ COL_INIT(), COL_VERTEX_INIT(0), COL_TRI_STOP(), COL_END() }};")
        elif typ == "Animation":
            lines.append(f"const struct Animation *const {name}[] = {{ NULL }};")
        elif typ == "MacroObject":
            lines.append(f"const MacroObject {name}[] = {{ MACRO_OBJECT_END() }};")
        elif typ == "Trajectory":
            lines.append(f"const Trajectory {name}[] = {{ TRAJECTORY_END() }};")
        elif typ == "LevelScript":
            levels_scripts.add(name)
            lv = hdr.split(os.sep)[1]
            if lv in LEVELS or lv in ("menu", "intro"):
                continue   # (its own script.c)
            lines.append(f"const LevelScript {name}[] = {{ PORT_LEVEL_MISSING() }};")
        elif typ in ("Movtex", "MovtexQuadCollection"):
            st = "struct " if typ != "Movtex" else ""
            lines.append(f"const {st}{typ} {name}[1];")
        elif typ == "Painting":
            lines.append(f"struct Painting {name};")
        elif typ == "RoomData":
            lines.append(f"const RoomData {name}[1];")
        elif typ in ("u8", "s16", "f32", "Vtx"):
            lines.append(f"{typ} {name}[1];")
    os.makedirs(GEN, exist_ok=True)
    open(os.path.join(GEN, "symbols.c"), "w").write("\n".join(lines) + "\n")
    return data_files


# ------------------------------------------------------------------------------------------------ mario's anims
def write_mario_anims(ref):
    files = sorted(glob.glob(os.path.join(ref, "assets", "anims", "*.inc.c")))
    names = []
    for f in files:
        copy(ref, os.path.relpath(f, ref))
        for m in re.finditer(r"struct Animation\s+(anim_([0-9A-Fa-f]+))\s*\[\]", open(f).read()):
            names.append((int(m.group(2), 16), m.group(1)))
    names.sort()
    lines = ["/* Made by tools/port.py: Mario's animations (the decompilation's assets/anims), by number: each",
             " * file's values and indices first, then the animations that name them. */",
             "#include <ultra64.h>", '#include "types.h"', '#include "port.h"', ""]
    for f in files:
        txt = open(f).read()
        blocks = re.findall(r"static const[^;{]*\{.*?\};", txt, re.S)
        lines += [b for b in blocks if "struct Animation" not in b]
        lines += [b for b in blocks if "struct Animation" in b]
    lines.append("")
    lines.append(f"const struct Animation *const port_mario_anims[{len(names)}] = {{")
    for i, (n, nm) in enumerate(names):
        assert n == i, (n, i)
        lines.append(f"    {nm},")
    lines.append("};")
    lines.append(f"const int port_mario_anim_count = {len(names)};")
    open(os.path.join(GEN, "mario_anims.c"), "w").write("\n".join(lines) + "\n")


# ------------------------------------------------------------------------------------------------ terrain
SURFACE_FORCE = {0x0004, 0x000E, 0x0024, 0x0025, 0x0027, 0x002C, 0x002D}   # see surface_has_force
NO_CAM = {0x0076, 0x0077, 0x0078, 0x007A}
CELL, BOUND = 0x400, 0x2000


def cell_lo(c):
    c += BOUND
    c = max(c, 0)
    i = c // CELL
    if c % CELL < 50:
        i -= 1
    return max(i, 0)


def cell_hi(c):
    c += BOUND
    c = max(c, 0)
    i = c // CELL
    if c % CELL > CELL - 50:
        i += 1
    return min(i, 15)


def terrain_surfaces(data):
    """The static surfaces of terrain data (a list of s16), as surface_load.c makes them."""
    out = []
    i = 0
    verts = None
    while True:
        cmd = data[i]
        i += 1
        if cmd == 0x40:   # TERRAIN_LOAD_VERTICES
            n = data[i]
            i += 1
            verts = data[i:i + 3 * n]
            i += 3 * n
        elif cmd == 0x43:  # TERRAIN_LOAD_OBJECTS (special objects): read by the game
            break
        elif cmd == 0x44:  # TERRAIN_LOAD_ENVIRONMENT
            break
        elif cmd == 0x41:  # CONTINUE
            continue
        elif cmd == 0x42:  # END
            break
        else:
            typ = cmd
            n = data[i]
            i += 1
            force = typ in SURFACE_FORCE
            for _ in range(n):
                a, b, c = data[i], data[i + 1], data[i + 2]
                fc = data[i + 3] if force else 0
                i += 4 if force else 3
                p = [verts[3 * a:3 * a + 3], verts[3 * b:3 * b + 3], verts[3 * c:3 * c + 3]]
                (x1, y1, z1), (x2, y2, z2), (x3, y3, z3) = p
                nx = float((y2 - y1) * (z3 - z2) - (z2 - z1) * (y3 - y2))
                ny = float((z2 - z1) * (x3 - x2) - (x2 - x1) * (z3 - z2))
                nz = float((x2 - x1) * (y3 - y2) - (y2 - y1) * (x3 - x2))
                mag = np.float32(math.sqrt(nx * nx + ny * ny + nz * nz))
                if mag < 0.0001:
                    continue
                inv = np.float32(1.0 / float(mag))
                nx, ny, nz = np.float32(nx) * inv, np.float32(ny) * inv, np.float32(nz) * inv
                off = -(nx * np.float32(x1) + ny * np.float32(y1) + nz * np.float32(z1))
                out.append({"type": typ, "force": fc, "flags": (0x02 if typ in NO_CAM else 0), "v": p,
                            "n": (float(nx), float(ny), float(nz)), "off": float(off),
                            "lo": min(y1, y2, y3) - 5, "hi": max(y1, y2, y3) + 5})
    return out


def partition(surfs):
    cells = [[[[], [], []] for _ in range(16)] for _ in range(16)]
    for s in surfs:
        ny = s["n"][1]
        if ny > 0.01:
            k, sd = 0, 1
        elif ny < -0.01:
            k, sd = 1, -1
        else:
            k, sd = 2, 0
            if s["n"][0] < -0.707 or s["n"][0] > 0.707:
                s["flags"] |= 0x08   # SURFACE_FLAG_X_PROJECTION
        xs = [v[0] for v in s["v"]]
        zs = [v[2] for v in s["v"]]
        for cz in range(cell_lo(min(zs)), cell_hi(max(zs)) + 1):
            for cx in range(cell_lo(min(xs)), cell_hi(max(xs)) + 1):
                lst = cells[cz][cx][k]
                pr = s["v"][0][1] * sd
                at = 0
                while at < len(lst) and not (pr > lst[at]["v"][0][1] * sd):
                    at += 1
                lst.insert(at, s)
    return cells


def parse_collision_arrays(ref, files):
    """name -> list of s16 of every Collision array in the files."""
    sys.path.insert(0, HERE)
    import decomp
    out = {}
    consts = surface_constants(ref)
    for f in files:
        txt = decomp.strip_comments(open(os.path.join(ref, f)).read())
        for name, body in decomp.arrays(txt, "Collision").items():
            vals = []
            for cmd, a in decomp.calls(body):
                av = [decomp.cval(x, consts) for x in a]
                if cmd == "COL_INIT":
                    vals.append(0x40)
                elif cmd == "COL_VERTEX_INIT":
                    vals.append(av[0])
                elif cmd == "COL_VERTEX":
                    vals += av
                elif cmd == "COL_TRI_INIT":
                    vals += av
                elif cmd == "COL_TRI":
                    vals += av
                elif cmd == "COL_TRI_SPECIAL":
                    vals += av
                elif cmd == "COL_TRI_STOP":
                    vals.append(0x41)
                elif cmd == "COL_END":
                    vals.append(0x42)
                elif cmd == "COL_SPECIAL_INIT":
                    vals += [0x43]
                    break
                elif cmd == "COL_WATER_BOX_INIT":
                    vals += [0x44]
                    break
            out[name] = vals
    return out


def surface_constants(ref):
    c = {}
    for line in open(os.path.join(ref, "include", "surface_terrains.h")):
        m = re.match(r"#define\s+(SURFACE_\w+)\s+(0x[0-9A-Fa-f]+|\d+)", line)
        if m:
            c[m.group(1)] = int(m.group(2), 0)
    return c


def write_terrain(ref, data_files):
    cols = parse_collision_arrays(ref, [f for f in data_files if f.startswith("levels") and f.endswith("collision.inc.c")
                                        and "/areas/" in f])
    lines = ["/* Made by tools/port.py: each level area's collision, in the game's 16 x 16 cells, in flash. */",
             "#include <ultra64.h>", '#include "types.h"', '#include "surface_terrains.h"',
             '#include "engine/surface_collision.h"', '#include "engine/surface_load.h"', '#include "port.h"', ""]
    table = []
    for name, data in sorted(cols.items()):
        surfs = terrain_surfaces(data)
        cells = partition(surfs)
        idx = {id(s): i for i, s in enumerate(surfs)}
        lines.append(f"static const struct Surface {name}_s[{max(1, len(surfs))}] = {{")
        for s in surfs:
            v = s["v"]
            lines.append("    {%d, %d, %d, 0, %d, %d, {%d, %d, %d}, {%d, %d, %d}, {%d, %d, %d}, {%r, %r, %r}, %r, NULL}," % (
                s["type"], s["force"], s["flags"], s["lo"], s["hi"], *v[0], *v[1], *v[2],
                np.float32(s["n"][0]).item(), np.float32(s["n"][1]).item(), np.float32(s["n"][2]).item(),
                np.float32(s["off"]).item()))
        lines.append("};")
        nodes = []
        heads = []
        for cz in range(16):
            for cx in range(16):
                for k in range(3):
                    lst = cells[cz][cx][k]
                    first = len(nodes) if lst else -1
                    for j, s in enumerate(lst):
                        nxt = len(nodes) + 1 if j + 1 < len(lst) else -1
                        nodes.append((nxt, idx[id(s)]))
                    heads.append(first)
        lines.append(f"static const struct SurfaceNode {name}_n[{max(1, len(nodes))}] = {{")
        for nxt, si in nodes:
            nx = f"(struct SurfaceNode *) &{name}_n[{nxt}]" if nxt >= 0 else "NULL"
            lines.append(f"    {{{nx}, (struct Surface *) &{name}_s[{si}]}},")
        lines.append("};")
        lines.append(f"static const SpatialPartitionCell {name}_p[16][16] = {{")
        for cz in range(16):
            row = []
            for cx in range(16):
                cell = []
                for k in range(3):
                    h = heads[(cz * 16 + cx) * 3 + k]
                    cell.append(f"{{(struct SurfaceNode *) &{name}_n[{h}], NULL}}" if h >= 0 else "{NULL, NULL}")
                row.append("{" + ", ".join(cell) + "}")
            lines.append("    {" + ", ".join(row) + "},")
        lines.append("};")
        table.append(name)
    lines.append("")
    for name in table:
        lines.append(f"extern const Collision {name}[];")
    lines.append("const struct PortTerrain port_terrains[] = {")
    for name in table:
        lines.append(f"    {{{name}, {name}_p}},")
    lines.append("    {NULL, NULL},")
    lines.append("};")
    open(os.path.join(GEN, "terrain.c"), "w").write("\n".join(lines) + "\n")
    print(f"terrain: {len(table)} areas")


def main():
    ref = sys.argv[1]
    model_of = {}
    mp = os.path.join(GEN, "models.txt")
    if os.path.exists(mp):
        for line in open(mp):
            w = line.split()
            if not w:
                continue
            if w[0] in ("geo", "dl"):
                model_of[w[1]] = int(w[2])
            elif w[0] == "area":
                model_of[w[1]] = ("area", [int(x) for x in w[2:]])
    for lv in LEVELS:
        copy(ref, f"levels/{lv}/script.c")
        copy(ref, f"levels/{lv}/header.h")
        for f in glob.glob(os.path.join(ref, "levels", lv, "*.h")):
            copy(ref, os.path.relpath(f, ref))
    data_files = write_symbols(ref, model_of)
    for f in data_files + actor_anim_files(ref):
        copy(ref, f)
        # (a texture some of them include: the calculator has its own, these stay empty)
        for inc in re.findall(r'#include\s+"([^"]+\.(?:rgba16|rgba32|ia16|ia8|ia4|i8|i4|ci4|ci8)\.inc\.c)"', open(os.path.join(ref, f)).read()):
            p = os.path.join(DEC, inc)
            if not os.path.exists(p):
                os.makedirs(os.path.dirname(p), exist_ok=True)
                open(p, "w").write("0\n")
    write_mario_anims(ref)
    write_terrain(ref, data_files)
    hdrs = sorted(glob.glob(os.path.join(ref, "levels", "*", "header.h")))
    open(os.path.join(GEN, "level_headers.h"), "w").write(
        "/* Made by tools/port.py: every level's header. */\n" +
        "".join(f'#include "{os.path.relpath(h, ref)}"\n' for h in hdrs))


if __name__ == "__main__":
    main()
