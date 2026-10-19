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

# the most vertices a batch of triangles has (the renderer transforms them at once: src/render.c)
BATCH_VERTS = 128

# the world light of the levels (the game's 0x28, 0x28, 0x28): baked into their vertices
WORLD_LIGHT = (0.57735, 0.57735, 0.57735)


class Blob:
    """The file's bytes; the same data added twice is kept once (Mario's heads differ only by their eyes)."""
    def __init__(self):
        self.b = bytearray(64)   # (the header)
        self.seen = {}

    def add(self, data, align=4):
        data = bytes(data)
        if (data, align) in self.seen:
            return self.seen[(data, align)]
        while len(self.b) % align:
            self.b.append(0)
        off = len(self.b)
        self.b += data
        self.seen[(data, align)] = off
        return off


def quantize(img, fmt, ncol):
    """A palette of at most ncol texel values (fmt's: RGB565, RGB565 with TEX_KEY first, or intensity << 8 | alpha)
    and the image's indices into it."""
    h, w = img.shape[:2]
    if fmt == 2:
        inten = img[..., 0].astype(np.int32)
        vals = (inten << 8) | img[..., 3].astype(np.int32)
        pts = np.stack([inten, img[..., 3].astype(np.int32)], axis=-1).reshape(-1, 2).astype(np.float64)
        pal, idx = kmeans_palette(pts, vals.reshape(-1), ncol, lambda c: (int(c[0]) << 8) | int(c[1]), h, w)
        return pal, idx.reshape(h, w)
    key = fmt == 1
    opaque = img[..., 3] >= 128 if key else np.ones((h, w), bool)
    r, g, b = (img[..., i].astype(np.int32) for i in range(3))
    vals = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    pts = np.stack([r, g, b], axis=-1).reshape(-1, 3).astype(np.float64)
    flat_opaque = opaque.reshape(-1)
    pal, idx = kmeans_palette(pts[flat_opaque], vals.reshape(-1)[flat_opaque], ncol - (1 if key else 0),
                              lambda c: ((int(c[0]) >> 3) << 11) | ((int(c[1]) >> 2) << 5) | (int(c[2]) >> 3), None, None)
    if key:
        pal = [0x0020] + [v if v != 0x0020 else 0x0040 for v in pal]
        full = np.zeros(h * w, np.int32)
        full[flat_opaque] = idx + 1
        return pal, full.reshape(h, w)
    return pal, idx.reshape(h, w)


def kmeans_palette(pts, vals, ncol, encode, h, w):
    """Exact when the image has few enough values, else k-means (k-means++ start, a few rounds)."""
    if len(vals) == 0:
        return [0], np.zeros(0, np.int32)
    uniq, inv = np.unique(vals, return_inverse=True)
    if len(uniq) <= ncol:
        return [int(v) for v in uniq], inv.astype(np.int32)
    rng = np.random.default_rng(7)
    cent = [pts[rng.integers(len(pts))]]
    for _ in range(1, ncol):
        d = np.min([((pts - c) ** 2).sum(1) for c in cent], axis=0)
        cent.append(pts[rng.choice(len(pts), p=d / d.sum())] if d.sum() > 0 else pts[0])
    cent = np.array(cent)
    for _ in range(12):
        lab = np.argmin(((pts[:, None, :] - cent[None]) ** 2).sum(2), axis=1)
        for k in range(ncol):
            m = lab == k
            if m.any():
                cent[k] = pts[m].mean(0)
    lab = np.argmin(((pts[:, None, :] - cent[None]) ** 2).sum(2), axis=1)
    return [encode(np.clip(np.round(c), 0, 255)) for c in cent], lab.astype(np.int32)


def texel_error(img, fmt, pal, idx):
    """The mean distance (0..255 a channel) of the paletted image from the painted one."""
    p = np.array(pal)
    v = p[idx]
    if fmt == 2:
        a = np.stack([v >> 8, v & 255], -1).astype(np.float64)
        b = np.stack([img[..., 0], img[..., 3]], -1).astype(np.float64)
        return np.abs(a - b).mean()
    a = np.stack([(v >> 11) * 255 / 31, ((v >> 5) & 63) * 255 / 63, (v & 31) * 255 / 31], -1)
    m = img[..., 3] >= 128 if fmt == 1 else np.ones(idx.shape, bool)
    return np.abs(a - img[..., :3])[m].mean() if m.any() else 0.0


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
        img = textures.paint(path, w, h).copy()
        if fmt == 2 and ".ia" not in path:
            img[..., 0] = img[..., :3].max(axis=2)
        # 16 colours a texture (4 bits a texel), 256 when 16 lose too much
        pal, idx = quantize(img, fmt, 16)
        depth = 1
        if texel_error(img, fmt, pal, idx) > 6:
            pal, idx = quantize(img, fmt, 256)
            depth = 2
        pal = pal + [pal[0]] * ((16 if depth == 1 else 256) - len(pal))
        if depth == 1:
            flat = idx.reshape(-1).astype(np.uint8)
            if len(flat) % 2:
                flat = np.append(flat, 0)
            texels = (flat[0::2] | (flat[1::2] << 4)).astype(np.uint8)
        else:
            texels = idx.reshape(-1).astype(np.uint8)
        off = self.blob.add(struct.pack(f"<{len(pal)}H", *pal) + texels.tobytes())
        self.tex_bytes = getattr(self, "tex_bytes", 0) + len(pal) * 2 + len(texels)
        wl, hl = int(round(np.log2(w))), int(round(np.log2(h)))
        self.tex_entries.append(struct.pack("<IBBBB", off, wl, hl, fmt | (depth << 4), 0))
        self.tex_index[key] = len(self.tex_entries) - 1
        return self.tex_index[key]

    def ui_texture(self, name, img, kind):
        """A texture of the screens (tools/glyphs.py), already painted: kind 'ia' or 'rgba' (a transparent key)."""
        fmt = 2 if kind == "ia" else 1
        h, w = img.shape[:2]
        pal, idx = quantize(img, fmt, 16)
        depth = 1
        if texel_error(img, fmt, pal, idx) > 6:
            pal, idx = quantize(img, fmt, 256)
            depth = 2
        pal = pal + [pal[0]] * ((16 if depth == 1 else 256) - len(pal))
        flat = idx.reshape(-1).astype(np.uint8)
        if depth == 1:
            if len(flat) % 2:
                flat = np.append(flat, 0)
            texels = (flat[0::2] | (flat[1::2] << 4)).astype(np.uint8)
        else:
            texels = flat
        off = self.blob.add(struct.pack(f"<{len(pal)}H", *pal) + texels.tobytes())
        wl, hl = int(round(np.log2(w))), int(round(np.log2(h)))
        self.tex_entries.append(struct.pack("<IBBBB", off, wl, hl, fmt | (depth << 4), 0))
        return len(self.tex_entries) - 1

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
            # split into runs of at most BATCH_VERTS vertices
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
                    if len(vmap) + len(set(new)) > BATCH_VERTS:
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
                tri_idx += bt
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
        toff = self.blob.add(bytes(i for t in tri_idx for i in t), 1)
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
    # the screens' textures: the HUD, the fonts, the power meter, the transitions (tools/glyphs.py)
    import glyphs
    ui = []
    for name, path in port.ui_textures(ref):
        if any(k in name for k in ("_cn_", "_jp_", "_eu_", "EU_", "beta")) or name in dict(ui):
            continue
        w, h, kind = glyphs.ui_size(name, path)
        ui.append((name, p.ui_texture(name, glyphs.paint_ui(name, w, h), kind)))
    open(os.path.join(port.GEN, "uitex.txt"), "w").write("".join(f"{n} {i}\n" for n, i in ui))
    p.write(mb)
    M.write_funcs(os.path.join(port.GEN, "geo_funcs.c"))
    print(f"{len(mb.models)} models, {len(mb.dl_mesh)} display lists, {len(M.FUNCS)} functions")


if __name__ == "__main__":
    main()
