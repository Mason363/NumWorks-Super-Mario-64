#!/usr/bin/env python3
"""pack.py REF: src/data.bin and src/data_ids.h from the decompilation in REF (tools/fetch.py fills it).

Levels, models, animations, collision and objects come from the decompilation's sources; the textures are
painted by tools/textures.py. See src/data.h for the format."""
import glob
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decomp  # noqa: E402
import textures  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_BIN = os.path.join(HERE, "..", "src", "data.bin")
OUT_IDS = os.path.join(HERE, "..", "src", "data_ids.h")

# the world light of the levels (the game's 0x28, 0x28, 0x28): baked into their vertices
WORLD_LIGHT = (0.57735, 0.57735, 0.57735)


class Blob:
    def __init__(self):
        self.b = bytearray(64)   # (the header)

    def add(self, data, align=4):
        while len(self.b) % align:
            self.b.append(0)
        off = len(self.b)
        self.b += data
        return off


class Pack:
    def __init__(self, ref):
        self.src = decomp.Source(ref)
        decomp.load_constants(ref)
        self.ref = ref
        self.blob = Blob()
        self.tex_index = {}      # (path, w, h, fmt) -> index
        self.tex_entries = []
        self.meshes = []         # packed Mesh structs
        self.ids = []            # (name, value) for data_ids.h

    def read(self, pattern):
        for f in sorted(glob.glob(os.path.join(self.ref, pattern), recursive=True)):
            self.src.read(os.path.relpath(f, self.ref))

    # ------------------------------------------------------------------------------------------ textures
    def texture(self, sym, w, h, cutout, ia):
        path = self.src.tex.get(sym, sym)
        fmt = 2 if ia or ".ia" in path or ".i4" in path or ".i8" in path else (1 if cutout or "rgba16" in path and cutout else 0)
        key = (path, w, h, fmt)
        if key in self.tex_index:
            return self.tex_index[key]
        img = textures.paint(path, w, h)
        if fmt == 2:
            inten = img[..., :3].max(axis=2).astype(np.uint16) if ".ia" not in path else img[..., 0].astype(np.uint16)
            data = ((inten << 8) | img[..., 3].astype(np.uint16)).astype("<u2")
        else:
            r, g, b = (img[..., i].astype(np.uint16) for i in range(3))
            data = (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)).astype("<u2")
            if fmt == 1:
                data[img[..., 3] < 128] = 0x0020
                data[(img[..., 3] >= 128) & (data == 0x0020)] = 0x0040
        off = self.blob.add(data.tobytes())
        wl, hl = int(round(np.log2(w))), int(round(np.log2(h)))
        self.tex_entries.append(struct.pack("<IBBBB", off, wl, hl, fmt, 0))
        self.tex_index[key] = len(self.tex_entries) - 1
        return self.tex_index[key]

    # ------------------------------------------------------------------------------------------ meshes
    def mesh(self, name, tris, bake=True, cell=2048):
        """Packs triangles (decomp.Tri) as a mesh: batches per material and, for big meshes, per cell of the
        ground, each with its own vertices. bake: lit vertices get their light now (levels); else they keep
        their normals and are lit as they are drawn (models)."""
        groups = {}
        for t in tris:
            m = t.mat
            if bake or not m.lit:
                mkey = m.key() + (False,)
            else:
                L = t.v[0][5][3]
                mkey = m.key() + (tuple(L[0]) + tuple(L[1]),)
            if cell:
                cx = sum(v[0] for v in t.v) / 3 // cell
                cz = sum(v[2] for v in t.v) / 3 // cell
                key = (cx, cz) + mkey
            else:
                key = mkey
            groups.setdefault(key, (m, []))[1].append(t)
        verts, tri_idx, batches = [], [], []
        lo, hi = [32767] * 3, [-32768] * 3
        for key, (m, ts) in sorted(groups.items(), key=lambda kv: (kv[1][0].layer, str(kv[0]))):
            lit = (not bake) and m.lit and key[-1] is not False
            mode = 0
            tex = 0
            if m.tex:
                mode |= 1
                ia = m.fmt in (3, 4)
                tex = self.texture(m.tex, m.w, m.h, m.cutout, ia)
                if m.combine == "mod":
                    mode |= 2
            if m.cutout:
                mode |= 4
            if m.xlu:
                mode |= 8
            if m.decal:
                mode |= 16
            if lit:
                mode |= 32
            if m.fog:
                mode |= 64
            if m.texgen:
                mode |= 128
            wrap = (m.cms & 3) | ((m.cmt & 3) << 2)
            # split into runs of at most 255 vertices
            pending = list(ts)
            while pending:
                vmap, bt, bverts = {}, [], []
                rest = []
                for t in pending:
                    keys = []
                    for v in t.v:
                        enc = self.encode_vertex(v, bake, lit, m)
                        keys.append(enc)
                    new = [k for k in keys if k not in vmap]
                    if len(vmap) + len(set(new)) > 255:
                        rest.append(t)
                        continue
                    idx = []
                    for k in keys:
                        if k not in vmap:
                            vmap[k] = len(bverts)
                            bverts.append(k)
                        idx.append(vmap[k])
                    bt.append(idx)
                pending = rest
                v0 = len(verts)
                verts += bverts
                tri0 = len(tri_idx)
                tri_idx += [[v0 + i for i in t] for t in bt]
                xs = [v[0] for v in bverts]
                ys = [v[1] for v in bverts]
                zs = [v[2] for v in bverts]
                blo, bhi = [min(xs), min(ys), min(zs)], [max(xs), max(ys), max(zs)]
                for i in range(3):
                    lo[i], hi[i] = min(lo[i], blo[i]), max(hi[i], bhi[i])
                light = bytes(6)
                if lit:
                    L = key[-1]
                    light = bytes(min(255, x) for x in L)
                batches.append(struct.pack("<HBBHHHH6h6s", tex, mode, wrap, tri0, len(bt), v0, len(bverts), *blo, *bhi, light))
        if not verts:
            return None
        vdata = b"".join(struct.pack("<5h4B", *v) for v in verts)
        voff = self.blob.add(vdata)
        toff = self.blob.add(b"".join(struct.pack("<3H", *t) for t in tri_idx))
        boff = self.blob.add(b"".join(batches))
        self.meshes.append(struct.pack("<IIIHHHH6h", voff, toff, boff, len(verts), len(tri_idx), len(batches), 0, *lo, *hi))
        idx = len(self.meshes) - 1
        self.ids.append(("MESH_" + name.upper(), idx))
        return idx

    @staticmethod
    def encode_vertex(v, bake, lit, m):
        x, y, z, s, t, col = v
        kind, c, al, L = col
        if kind == "c" or not lit:
            if kind == "n":
                r, g, b, a = decomp.shade(v, WORLD_LIGHT)
            else:
                r, g, b, a = c[0], c[1], c[2], al
        else:
            r, g, b, a = c[0] & 255, c[1] & 255, c[2] & 255, al
        si = max(-32768, min(32767, int(round(s))))
        ti = max(-32768, min(32767, int(round(t))))
        return (int(x), int(y), int(z), si, ti, r, g, b, a)

    # ------------------------------------------------------------------------------------------ geo
    def geo_mesh(self, name, geo, bake=True):
        tree = decomp.geo_tree(self.src, geo)
        tris = []

        def walk(n):
            for c in n.children:
                if c.kind == "GEO_DISPLAY_LIST":
                    decomp.run_dl(self.src, c.args[1].strip(), decomp.layer_of(c.args[0]), tris)
                walk(c)
        walk(tree)
        return self.mesh(name, tris, bake)

    # ------------------------------------------------------------------------------------------ areas
    def area(self, geo):
        """An area's geo layout: its mesh (the display lists, lit now), and what its camera, frustum, background
        and fog say."""
        tree = decomp.geo_tree(self.src, geo)
        tris, info = [], {"mode": 1, "pos": (0, 0, 0), "focus": (0, 0, 0), "fov": 45, "near": 100, "far": 20000,
                          "bg": 0, "fog": None}

        def walk(n):
            for c in n.children:
                a = c.args
                if c.kind == "GEO_DISPLAY_LIST":
                    decomp.run_dl(self.src, a[1].strip(), decomp.layer_of(a[0]), tris)
                    self.find_fog(a[1].strip(), info)
                elif c.kind == "GEO_CAMERA":
                    v = [decomp.cval(x) for x in a[:7]]
                    info["mode"], info["pos"], info["focus"] = v[0], v[1:4], v[4:7]
                elif c.kind in ("GEO_CAMERA_FRUSTUM", "GEO_CAMERA_FRUSTUM_WITH_FUNC"):
                    info["fov"], info["near"], info["far"] = (decomp.cval(x) for x in a[:3])
                elif c.kind == "GEO_BACKGROUND":
                    info["bg"] = decomp.cval(a[0], BACKGROUNDS)
                elif c.kind == "GEO_BACKGROUND_COLOR":
                    info["bg"] = 0x8000 | (decomp.cval(a[0]) & 0x7FFF)
                walk(c)
        walk(tree)
        info["mesh"] = self.mesh("area_" + geo, tris, bake=True) if tris else 0xFFFF
        if info["mesh"] is None:
            info["mesh"] = 0xFFFF
        return info

    def find_fog(self, dl, info):
        for cmd, a in self.src.gfx.get(dl, []):
            if cmd == "gsDPSetFogColor":
                info.setdefault("fogc", tuple(decomp.cval(x) for x in a[:3]))
            elif cmd == "gsSPFogPosition":
                info["fog"] = (decomp.cval(a[0]), decomp.cval(a[1]))
            elif cmd == "gsSPDisplayList":
                self.find_fog(a[0].strip(), info)

    # ------------------------------------------------------------------------------------------ output
    def write(self, models=None):
        b = self.blob
        tex_off = b.add(b"".join(self.tex_entries))
        mesh_off = b.add(b"".join(self.meshes))
        model_off = b.add(b"".join(models.models)) if models else 0
        hdr = struct.pack("<8I6H", 0x34364D53, 0, tex_off, mesh_off, model_off, 0, 0, 0,
                          len(self.tex_entries), len(self.meshes), len(models.models) if models else 0, 0, 0, 0)
        b.b[0:len(hdr)] = hdr
        struct.pack_into("<I", b.b, 4, len(b.b))
        open(OUT_BIN, "wb").write(bytes(b.b))
        with open(OUT_IDS, "w") as f:
            f.write("/* Made by tools/pack.py: the indices of what src/data.bin holds. */\n")
            f.write("#ifndef SM_DATA_IDS_H\n#define SM_DATA_IDS_H\n")
            for n, v in self.ids:
                f.write(f"#define {n} {v}\n")
            f.write("#endif\n")
        print(f"data.bin: {len(b.b)} bytes, {len(self.tex_entries)} textures, {len(self.meshes)} meshes")


BACKGROUNDS = {"BACKGROUND_OCEAN_SKY": 0, "BACKGROUND_FLAMING_SKY": 1, "BACKGROUND_UNDERWATER_CITY": 2,
               "BACKGROUND_BELOW_CLOUDS": 3, "BACKGROUND_SNOW_MOUNTAINS": 4, "BACKGROUND_DESERT": 5,
               "BACKGROUND_HAUNTED": 6, "BACKGROUND_GREEN_SKY": 7, "BACKGROUND_ABOVE_CLOUDS": 8,
               "BACKGROUND_PURPLE_SKY": 9}


def fog_distance(frac, near, far):
    """The distance where the N64's depth reaches frac (0..1000) of its range, for a frustum near..far."""
    z = 2 * frac / 1000 - 1
    d = (far + near) - z * (far - near)
    return int(2 * far * near / d) if d > 0 else far


def main():
    import models as M
    import port
    ref = sys.argv[1]
    p = Pack(ref)
    p.read("bin/*.c")
    p.read("actors/**/*.inc.c")
    for lv in port.LEVELS:
        p.read(f"levels/{lv}/**/*.inc.c")
        p.read(f"levels/{lv}/*.c")
    mb = M.ModelBuilder(p)
    lines = []
    # the scripts the levels run: their own, the common one, and the groups of models they link to
    bodies = []
    for lv in port.LEVELS:
        bodies += list(decomp.arrays(decomp.strip_comments(open(os.path.join(ref, "levels", lv, "script.c")).read()),
                                     "LevelScript").values())
    common = decomp.arrays(decomp.strip_comments(open(os.path.join(ref, "levels", "scripts.c")).read()), "LevelScript")
    used = {"level_main_scripts_entry"}
    for body in bodies:
        for cmd, a in decomp.calls(body):
            if cmd == "JUMP_LINK" and a[0].strip() in common:
                used.add(a[0].strip())
    bodies += [common[n] for n in sorted(used)]
    for body in bodies:
        for cmd, a in decomp.calls(body):
            if cmd == "LOAD_MODEL_FROM_GEO":
                geo = a[1].strip()
                if geo in p.src.geo:
                    lines.append(f"geo {geo} {mb.from_geo(geo)}")
            elif cmd == "LOAD_MODEL_FROM_DL":
                dl, layer = a[1].strip(), decomp.layer_of(a[2])
                if dl in p.src.gfx:
                    lines.append(f"dl {dl} {mb.from_dl(dl, layer)}")
            elif cmd == "AREA":
                geo = a[1].strip()
                if geo in p.src.geo:
                    info = p.area(geo)
                    fogc, fn, ff = 0, 0, 0
                    if info["fog"] and "fogc" in info:
                        r, g, bb = info["fogc"]
                        fogc = ((r >> 3) << 11) | ((g >> 2) << 5) | (bb >> 3) or 1
                        fn = fog_distance(info["fog"][0], info["near"], info["far"])
                        ff = fog_distance(info["fog"][1], info["near"], info["far"])
                    lines.append("area {} {} {} {} {} {} {} {} {} {} {} {}".format(
                        geo, info["mesh"], info["mode"], " ".join(map(str, info["pos"])), " ".join(map(str, info["focus"])),
                        info["fov"], info["near"], info["far"], info["bg"], fogc, fn, ff))
    os.makedirs(port.GEN, exist_ok=True)
    open(os.path.join(port.GEN, "models.txt"), "w").write("\n".join(sorted(set(lines))) + "\n")
    p.write(mb)
    print(f"{len(mb.models)} models, {len(mb.dl_mesh)} display lists")


if __name__ == "__main__":
    main()
