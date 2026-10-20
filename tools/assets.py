#!/usr/bin/env python3
"""assets.py FOLDER: the game's own textures, as PNG files in the decompilation's layout (actors/..., levels/...,
textures/...), for make data ASSETS=FOLDER.

The textures are Nintendo's, in Super Mario 64's ROM, which isn't public. They are taken from an extraction of the
US version's shared on the Internet Archive ("SM64 US Assets": its res/base.zip), and each one is kept only when
its bytes, as the N64 reads them, hash as the ROM's do: rom_textures.txt has the xxHash64 of each texture this
port uses: the list teamsalta/sm64nx made of every texture of the US ROM, and for the skyboxes' pictures (248 x
248, their 64 tiles put together as tools/skyconv.c does) the hash of the picture two extractions agree on.

    python3 tools/assets.py ~/sm64-assets && make data REF=build/ref/decomp ASSETS=~/sm64-assets

(Python 3, Pillow, NumPy; nothing in the download is run, only its pictures are read.)"""
import io
import os
import sys
import tarfile
import urllib.request
import zipfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
URL = "https://archive.org/download/am64-US-assets.tar/sm64-US-assets.tar.xz"
LIST = os.path.join(HERE, "rom_textures.txt")

# ------------------------------------------------------------------------------------------------ xxHash64
M64 = (1 << 64) - 1
P1, P2, P3, P4, P5 = 0x9E3779B185EBCA87, 0xC2B2AE3D27D4EB4F, 0x165667B19E3779F9, 0x85EBCA77C2B2AE63, 0x27D4EB2F165667C5


def _rotl(x, r):
    return ((x << r) | (x >> (64 - r))) & M64


def _round(acc, v):
    return _rotl((acc + v * P2) & M64, 31) * P1 & M64


def xxh64(data, seed=0):
    n, i = len(data), 0
    if n >= 32:
        v = [(seed + P1 + P2) & M64, (seed + P2) & M64, seed, (seed - P1) & M64]
        while i + 32 <= n:
            for k in range(4):
                v[k] = _round(v[k], int.from_bytes(data[i + 8 * k:i + 8 * k + 8], "little"))
            i += 32
        h = (_rotl(v[0], 1) + _rotl(v[1], 7) + _rotl(v[2], 12) + _rotl(v[3], 18)) & M64
        for k in range(4):
            h = ((h ^ _round(0, v[k])) * P1 + P4) & M64
    else:
        h = (seed + P5) & M64
    h = (h + n) & M64
    while i + 8 <= n:
        h = (_rotl(h ^ _round(0, int.from_bytes(data[i:i + 8], "little")), 27) * P1 + P4) & M64
        i += 8
    if i + 4 <= n:
        h = (_rotl(h ^ (int.from_bytes(data[i:i + 4], "little") * P1 & M64), 23) * P2 + P3) & M64
        i += 4
    while i < n:
        h = _rotl(h ^ (data[i] * P5 & M64), 11) * P1 & M64
        i += 1
    h ^= h >> 33
    h = h * P2 & M64
    h ^= h >> 29
    h = h * P3 & M64
    return h ^ (h >> 32)


# ------------------------------------------------------------------------------------------------ N64 formats
def fmt_of(path):
    for f in ("rgba16", "rgba32", "ia16", "ia8", "ia4", "i8", "i4"):
        if "." + f + "." in path:
            return f
    return "rgba16"


def to_raw(img, f):
    """An RGBA picture's bytes as the N64 keeps them (the decompilation's n64graphics, backwards)."""
    r, g, b, a = (img[..., i].astype(np.uint32) for i in range(4))
    if f == "rgba16":
        return ((r >> 3) << 11 | (g >> 3) << 6 | (b >> 3) << 1 | (a > 127)).astype(">u2").tobytes()
    if f == "rgba32":
        return img.astype(np.uint8).tobytes()
    if f == "ia16":
        return np.stack([r, a], -1).astype(np.uint8).tobytes()
    if f == "ia8":
        return ((r >> 4) << 4 | (a >> 4)).astype(np.uint8).tobytes()
    if f == "i8":
        return r.astype(np.uint8).tobytes()
    n = (((r >> 5) << 1 | (a > 127)) if f == "ia4" else r >> 4).reshape(-1)
    return ((n[0::2] << 4) | n[1::2]).astype(np.uint8).tobytes()


def from_raw(data, f, w, h):
    """The N64's bytes as an RGBA picture (as n64graphics shows them)."""
    d = np.frombuffer(data, np.uint8)
    if f == "rgba16":
        v = d.view(">u2")[:w * h].astype(np.uint32).reshape(h, w)
        c = lambda x: (x << 3) | (x >> 2)  # noqa: E731
        return np.stack([c(v >> 11 & 31), c(v >> 6 & 31), c(v >> 1 & 31), (v & 1) * 255], -1).astype(np.uint8)
    if f == "rgba32":
        return d[:w * h * 4].reshape(h, w, 4).copy()
    if f == "ia16":
        x = d[:w * h * 2].reshape(h, w, 2)
        return np.stack([x[..., 0]] * 3 + [x[..., 1]], -1)
    if f == "ia8":
        x = d[:w * h].reshape(h, w).astype(np.uint32)
        i, a = (x >> 4) * 17, (x & 15) * 17
        return np.stack([i, i, i, a], -1).astype(np.uint8)
    if f == "i8":
        x = d[:w * h].reshape(h, w)
        return np.stack([x] * 4, -1)
    n = np.stack([d >> 4, d & 15], -1).reshape(-1)[:w * h].reshape(h, w).astype(np.uint32)
    if f == "ia4":
        i3 = n >> 1
        i = (i3 << 5) | (i3 << 2) | (i3 >> 1)
        return np.stack([i, i, i, (n & 1) * 255], -1).astype(np.uint8)
    return np.stack([n * 17] * 4, -1).astype(np.uint8)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "build/assets"
    os.makedirs(out, exist_ok=True)
    want = {}
    for line in open(LIST):
        w = line.split()
        if len(w) == 2 and not line.startswith("#"):
            want[w[0]] = int(w[1], 16)
    archive = os.path.join(out, "sm64-US-assets.tar.xz")
    if not os.path.exists(archive):
        print("downloading", URL)
        with urllib.request.urlopen(URL, timeout=600) as r:
            data = r.read()
        open(archive, "wb").write(data)
    with tarfile.open(archive, "r:xz") as tar:
        member = next(m for m in tar.getmembers() if m.name.endswith("res/base.zip"))
        z = zipfile.ZipFile(io.BytesIO(tar.extractfile(member).read()))
    names = set(z.namelist())
    good = bad = 0
    for path, h in sorted(want.items()):
        name = "gfx/" + path
        if name not in names:
            print("missing:", path)
            bad += 1
            continue
        img = np.array(Image.open(io.BytesIO(z.read(name))).convert("RGBA"))
        f = fmt_of(path)
        raw = to_raw(img, f)
        if xxh64(raw) != h:
            print("not the ROM's:", path)
            bad += 1
            continue
        dst = os.path.join(out, path)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        Image.fromarray(from_raw(raw, f, img.shape[1], img.shape[0]), "RGBA").save(dst)
        good += 1
    print(f"{out}: {good} pictures checked, {bad} left out")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
