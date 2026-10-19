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

    # ------------------------------------------------------------------------------------------ output
    def write(self):
        b = self.blob
        tex_off = b.add(b"".join(self.tex_entries))
        mesh_off = b.add(b"".join(self.meshes))
        hdr = struct.pack("<8I6H", 0x34364D53, 0, tex_off, mesh_off, 0, 0, 0, 0,
                          len(self.tex_entries), len(self.meshes), 0, 0, 0, 0)
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


def main():
    ref = sys.argv[1]
    p = Pack(ref)
    p.read("bin/*.c")
    p.read("levels/bob/**/*.inc.c")
    p.geo_mesh("bob_area1", "bob_geo_000488")
    p.write()


if __name__ == "__main__":
    main()
