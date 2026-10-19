"""The game's models (geo layouts) as the calculator draws them: a flat list of nodes, parents first, each with its
kind, transform and mesh (src/data.h, MNode). The meshes keep their normals: the renderer lights them as it draws.

Switches and the game's drawing callbacks (GEO_ASM) keep the name of their function, as a number the
renderer knows (src/port/scene.c)."""
import struct

import decomp

N_PART, N_TRANSLATE, N_ROTATE, N_SCALE, N_SWITCH, N_BILLBOARD, N_SHADOW, N_HELD, N_MESH, N_GROUP, N_RANGE, N_ASM = range(12)

# the callbacks the renderer reproduces (src/port/scene.c's SW_* and ASM_*, numbered from 1 in this order)
SWITCHES = ["geo_switch_anim_state", "geo_switch_mario_hand", "geo_switch_mario_eyes", "geo_switch_mario_cap_on_off",
            "geo_switch_mario_cap_effect", "geo_switch_mario_stand_run", "geo_switch_bowser_eyes",
            "geo_switch_peach_eyes", "geo_switch_tuxie_mother_eyes", "geo_switch_area", "geo_switch_mario_hand_grab_pos"]
ASMS = ["geo_mario_head_rotation", "geo_mario_tilt_torso", "geo_mario_hand_foot_scaler",
        "geo_move_mario_part_from_parent", "geo_mario_rotate_wing_cap_wings", "geo_update_layer_transparency",
        "geo_bits_bowser_coloring", "geo_update_body_rot_from_parent", "geo_offset_klepto_held_object",
        "geo_update_projectile_pos_from_parent", "geo_update_held_mario_pos", "geo_mirror_mario_backface_culling"]

NODE_FMT = "<BBBBBBH3h3hI"   # kind, parent, sw, arg, case, pad, mesh, t[3], r[3], scale
MODEL_FMT = "<IHBBhh3h3h"    # nodes, nnodes, shadow type, pad, shadow size, shadow solidity, lo[3], hi[3]


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

    def finish(self, nodes, shadow):
        data = b""
        lo, hi = [32767] * 3, [-32768] * 3
        for nd in nodes:
            data += struct.pack(NODE_FMT, nd["kind"], nd["parent"] & 0xFF, nd["sw"] & 0xFF, nd["arg"] & 0xFF,
                                nd["case"] & 0xFF, 0, nd["mesh"], *nd["t"], *nd["r"], nd["scale"] & 0xFFFFFFFF)
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
        self.models.append(struct.pack(MODEL_FMT, off, len(nodes), shadow[0] & 0xFF, 0, shadow[2], shadow[1], *lo, *hi))
        return len(self.models) - 1

    def from_geo(self, geo):
        if geo in self.index:
            return self.index[geo]
        tree = decomp.geo_tree(self.src, geo)
        nodes = []
        shadow = [0, 0, 0]

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
                    me = add(N_TRANSLATE, parent, case, sw=1, arg=L(0), t=V(1, 4), r=V(4, 7),
                             mesh=self.dl(a[7].strip(), L(0)) if len(a) > 7 else 0xFFFF)
                elif k in ("GEO_SCALE", "GEO_SCALE_WITH_DL"):
                    me = add(N_SCALE, parent, case, arg=L(0), scale=decomp.cval(a[1]),
                             mesh=self.dl(a[2].strip(), L(0)) if len(a) > 2 else 0xFFFF)
                elif k == "GEO_SWITCH_CASE":
                    fn = a[1].strip()
                    me = add(N_SWITCH, parent, case, sw=SWITCHES.index(fn) + 1 if fn in SWITCHES else 0,
                             arg=decomp.cval(a[0]))
                elif k in ("GEO_BILLBOARD", "GEO_BILLBOARD_WITH_PARAMS", "GEO_BILLBOARD_WITH_PARAMS_AND_DL"):
                    me = add(N_BILLBOARD, parent, case, t=V(1, 4) if len(a) >= 4 else (0, 0, 0))
                elif k == "GEO_SHADOW":
                    shadow = [decomp.cval(a[0]), decomp.cval(a[1]), decomp.cval(a[2])]
                    me = add(N_GROUP, parent, case)
                elif k == "GEO_HELD_OBJECT":
                    me = add(N_HELD, parent, case, arg=decomp.cval(a[0]), t=V(1, 4))
                elif k == "GEO_RENDER_RANGE":
                    me = add(N_RANGE, parent, case, t=(decomp.cval(a[0]), decomp.cval(a[1]), 0))
                elif k == "GEO_ASM":
                    fn = a[1].strip()
                    p = decomp.cval(a[0])
                    me = add(N_ASM if fn in ASMS else N_GROUP, parent, case, sw=ASMS.index(fn) + 1 if fn in ASMS else 0,
                             arg=p & 0xFF, t=(p >> 8, 0, 0))
                else:
                    me = add(N_GROUP, parent, case)
                walk(c, me)

        walk(tree, 0xFF)
        self.index[geo] = self.finish(nodes, shadow)
        return self.index[geo]

    def from_dl(self, dl, layer):
        key = ("dl", dl, layer)
        if key not in self.index:
            nodes = [{"kind": N_MESH, "parent": 0xFF, "case": 0, "sw": 0, "arg": layer, "t": [0, 0, 0], "r": [0, 0, 0],
                      "mesh": self.dl(dl, layer), "scale": 0}]
            self.index[key] = self.finish(nodes, [0, 0, 0])
        return self.index[key]
