"""The game's models (geo layouts) as the calculator draws them: a flat list of nodes, parents first, each with its
kind, transform and mesh (src/data.h, MNode). The meshes keep their normals: the renderer lights them as it draws.

Switches, held objects and the game's drawing callbacks (GEO_ASM) keep their function as a number: its place in
src/port/gen/geo_funcs.c, the table of the game's own functions src/port/scene.c calls (FUNCS)."""
import struct

import decomp

(N_PART, N_TRANSLATE, N_ROTATE, N_SCALE, N_SWITCH, N_BILLBOARD, N_SHADOW, N_HELD, N_MESH, N_GROUP, N_RANGE, N_ASM,
 N_TRANSROT) = range(13)

# the game's functions the models name, numbered from 1 (src/port/gen/geo_funcs.c)
FUNCS = []


def func(name):
    if name in ("NULL", "0", ""):
        return 0
    if name not in FUNCS:
        FUNCS.append(name)
    return FUNCS.index(name) + 1


NODE_FMT = "<BBBBHHHH3h3hI"  # kind, fn, arg, case, parent, end, mesh, pad, t[3], r[3], scale (or a callback's parameter)
MODEL_FMT = "<IHBBhhhh3h3h"  # nodes, nnodes, pad, pad, culling radius, pad, pad, pad, lo[3], hi[3]


class ModelBuilder:
    def __init__(self, pack):
        self.pack = pack
        self.src = pack.src
        self.dl_mesh = {}      # (display list, layer) -> mesh index
        self.models = []       # packed MModel
        self.index = {}        # geo symbol, or ("dl", symbol, layer) -> model index

    def dl(self, name, layer):
        if name in ("NULL", "0", ""):
            return 0xFFFF
        key = (name, layer)
        if key not in self.dl_mesh:
            tris = []
            decomp.run_dl(self.src, name, layer, tris)
            idx = self.pack.mesh(f"dl_{name}_{layer}", tris, bake=False, cell=0) if tris else None
            self.dl_mesh[key] = 0xFFFF if idx is None else idx
        return self.dl_mesh[key]

    def finish(self, nodes, cull):
        data = b""
        lo, hi = [32767] * 3, [-32768] * 3
        for nd in nodes:
            data += struct.pack(NODE_FMT, nd["kind"], nd["sw"] & 0xFF, nd["arg"] & 0xFF, nd["case"] & 0xFF,
                                nd["parent"] & 0xFFFF, nd.get("end", len(nodes)), nd["mesh"], 0, *nd["t"], *nd["r"],
                                nd["scale"] & 0xFFFFFFFF)
            if nd["mesh"] != 0xFFFF:
                m = struct.unpack("<IIIHHHH6h", self.pack.meshes[nd["mesh"]])
                for i in range(3):
                    lo[i], hi[i] = min(lo[i], m[7 + i]), max(hi[i], m[10 + i])
        if lo[0] > hi[0]:
            lo, hi = [-50] * 3, [50] * 3
        # (parts move with the animation: a generous box)
        pad = [max(abs(lo[i]), abs(hi[i])) for i in range(3)]
        r = max(pad)
        lo, hi = [max(-32768, -2 * r)] * 3, [min(32767, 2 * r)] * 3
        off = self.pack.blob.add(data)
        self.models.append(struct.pack(MODEL_FMT, off, len(nodes), 0, 0, cull, 0, 0, 0, *lo, *hi))
        return len(self.models) - 1

    def from_geo(self, geo):
        if geo in self.index:
            return self.index[geo]
        tree = decomp.geo_tree(self.src, geo)
        nodes = []
        # (a culling radius first: the object is culled with it, else with 300)
        first = tree.children[0] if tree.children else None
        cull = decomp.cval(first.args[0]) if first is not None and first.kind == "GEO_CULLING_RADIUS" else 300

        def add(kind, parent, case, sw=0, arg=0, t=(0, 0, 0), r=(0, 0, 0), mesh=0xFFFF, scale=0):
            nodes.append({"kind": kind, "parent": parent, "case": case, "sw": sw, "arg": arg, "t": list(t), "r": list(r),
                          "mesh": mesh, "scale": scale})
            return len(nodes) - 1

        def walk(n, parent):
            for case, c in enumerate(n.children):
                a, k = c.args, c.kind
                L = lambda i: decomp.layer_of(a[i])
                V = lambda i, j: [decomp.cval(x) for x in a[i:j]]
                if k == "GEO_DISPLAY_LIST":
                    me = add(N_MESH, parent, case, arg=L(0), mesh=self.dl(a[1].strip(), L(0)))
                elif k == "GEO_ANIMATED_PART":
                    me = add(N_PART, parent, case, arg=L(0), t=V(1, 4), mesh=self.dl(a[4].strip(), L(0)))
                elif k in ("GEO_TRANSLATE_NODE", "GEO_TRANSLATE_NODE_WITH_DL", "GEO_TRANSLATE", "GEO_TRANSLATE_WITH_DL"):
                    me = add(N_TRANSLATE, parent, case, arg=L(0), t=V(1, 4),
                             mesh=self.dl(a[4].strip(), L(0)) if len(a) > 4 else 0xFFFF)
                elif k in ("GEO_ROTATION_NODE", "GEO_ROTATION_NODE_WITH_DL", "GEO_ROTATE", "GEO_ROTATE_WITH_DL"):
                    me = add(N_ROTATE, parent, case, arg=L(0), r=V(1, 4),
                             mesh=self.dl(a[4].strip(), L(0)) if len(a) > 4 else 0xFFFF)
                elif k in ("GEO_TRANSLATE_ROTATE", "GEO_TRANSLATE_ROTATE_WITH_DL"):
                    me = add(N_TRANSROT, parent, case, arg=L(0), t=V(1, 4), r=V(4, 7),
                             mesh=self.dl(a[7].strip(), L(0)) if len(a) > 7 else 0xFFFF)
                elif k in ("GEO_SCALE", "GEO_SCALE_WITH_DL"):
                    me = add(N_SCALE, parent, case, arg=L(0), scale=decomp.cval(a[1]),
                             mesh=self.dl(a[2].strip(), L(0)) if len(a) > 2 else 0xFFFF)
                elif k == "GEO_SWITCH_CASE":
                    me = add(N_SWITCH, parent, case, sw=func(a[1].strip()), arg=decomp.cval(a[0]))
                elif k in ("GEO_BILLBOARD", "GEO_BILLBOARD_WITH_PARAMS"):
                    me = add(N_BILLBOARD, parent, case, arg=L(0) if len(a) >= 4 else 0,
                             t=V(1, 4) if len(a) >= 4 else (0, 0, 0))
                elif k == "GEO_BILLBOARD_WITH_PARAMS_AND_DL":
                    me = add(N_BILLBOARD, parent, case, arg=L(0), t=V(1, 4), mesh=self.dl(a[4].strip(), L(0)))
                elif k == "GEO_SHADOW":
                    # type, solidity, scale
                    me = add(N_SHADOW, parent, case, arg=decomp.cval(a[0]), t=(decomp.cval(a[1]), decomp.cval(a[2]), 0))
                elif k == "GEO_HELD_OBJECT":
                    me = add(N_HELD, parent, case, sw=func(a[4].strip()), arg=decomp.cval(a[0]), t=V(1, 4))
                elif k == "GEO_RENDER_RANGE":
                    me = add(N_RANGE, parent, case, t=(decomp.cval(a[0]), decomp.cval(a[1]), 0))
                elif k == "GEO_ASM":
                    me = add(N_ASM, parent, case, sw=func(a[1].strip()), scale=decomp.cval(a[0]))
                else:
                    me = add(N_GROUP, parent, case)
                walk(c, me)
                # (where its subtree ends: the renderer skips there)
                nodes[me]["end"] = len(nodes)

        walk(tree, 0xFFFF)
        assert len(nodes) < 0xFFFF, (geo, len(nodes))
        self.index[geo] = self.finish(nodes, cull)
        return self.index[geo]

    def from_dl(self, dl, layer):
        key = ("dl", dl, layer)
        if key not in self.index:
            nodes = [{"kind": N_MESH, "parent": 0xFFFF, "case": 0, "end": 1, "sw": 0, "arg": layer, "t": [0, 0, 0], "r": [0, 0, 0],
                      "mesh": self.dl(dl, layer), "scale": 0}]
            self.index[key] = self.finish(nodes, 300)
        return self.index[key]


def write_funcs(path):
    """src/port/gen/geo_funcs.c: the functions the models name, by number."""
    lines = ["/* Made by tools/pack.py: the game's functions its models name (geo layouts' switches, held objects and",
             " * GEO_ASM), by the number tools/models.py gave them. */",
             '#include <ultra64.h>', '#include "types.h"', '#include "engine/graph_node.h"', ""]
    for f in FUNCS:
        lines.append(f"Gfx *{f}();")
    lines.append("")
    lines.append(f"const GraphNodeFunc port_geo_funcs[{len(FUNCS) + 1}] = {{")
    lines.append("    NULL,")
    for f in FUNCS:
        lines.append(f"    (GraphNodeFunc) {f},")
    lines.append("};")
    open(path, "w").write("\n".join(lines) + "\n")
