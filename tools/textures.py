"""The game's textures, drawn for the calculator: the decompilation has none (they are in the game's ROM), so each
one the levels and models use is painted here, after the original, from its name, size and where it is used.

paint(path, w, h) gives an RGBA image (numpy, h x w x 4) for a texture of the decompilation, e.g.
"textures/generic/bob_textures.05800.rgba16.png". Textures nobody painted yet come out as a plain grey
with a little noise, so they can be spotted in screenshots."""
import math

import numpy as np

import art

RNG = np.random.default_rng(64)


def rgb(h):
    return np.array([(h >> 16) & 255, (h >> 8) & 255, h & 255], dtype=np.float32)


def noise(h, w, scale, seed):
    """Smooth value noise, tiling, in 0..1."""
    r = np.random.default_rng(seed)
    gh, gw = max(1, h // scale), max(1, w // scale)
    g = r.random((gh, gw))
    ys, xs = np.mgrid[0:h, 0:w]
    fy, fx = ys / scale, xs / scale
    y0, x0 = np.floor(fy).astype(int), np.floor(fx).astype(int)
    ty, tx = fy - y0, fx - x0
    ty, tx = ty * ty * (3 - 2 * ty), tx * tx * (3 - 2 * tx)
    a = g[y0 % gh, x0 % gw]
    b = g[y0 % gh, (x0 + 1) % gw]
    c = g[(y0 + 1) % gh, x0 % gw]
    d = g[(y0 + 1) % gh, (x0 + 1) % gw]
    return (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty


def speckle(h, w, seed):
    return np.random.default_rng(seed).random((h, w))


def mix(c1, c2, t):
    t = t[..., None]
    return c1 * (1 - t) + c2 * t


def finish(img, alpha=None):
    h, w = img.shape[:2]
    out = np.zeros((h, w, 4), dtype=np.uint8)
    out[..., :3] = np.clip(img, 0, 255).astype(np.uint8)
    out[..., 3] = 255 if alpha is None else alpha
    return out


# ------------------------------------------------------------------------------------------------ materials

def grass(w, h, seed=1, base=0x3CA828, dark=0x2A7A1C, light=0x6CD040):
    """Bob-omb Battlefield's grass: bright green, darker blades in diagonal strokes, a few light ones."""
    n = noise(h, w, 8, seed) * 0.6 + noise(h, w, 4, seed + 1) * 0.4
    img = mix(rgb(base), rgb(dark), np.clip((n - 0.45) * 2.2, 0, 1))
    s = speckle(h, w, seed + 2)
    ys, xs = np.mgrid[0:h, 0:w]
    blades = ((xs + ys * 2) % 7 == 0) & (s > 0.55)
    img[blades] = rgb(dark) * 0.85
    lights = ((xs * 3 + ys) % 11 == 0) & (s < 0.3)
    img[lights] = rgb(light)
    return finish(img)


def dirt(w, h, seed=3, base=0xB07838, dark=0x7A5020, light=0xD09858):
    n = noise(h, w, 6, seed) * 0.5 + noise(h, w, 3, seed + 1) * 0.5
    img = mix(rgb(base), rgb(dark), np.clip((n - 0.5) * 2, 0, 1))
    s = speckle(h, w, seed + 2)
    img[s > 0.93] = rgb(light)
    img[s < 0.05] = rgb(dark) * 0.8
    return finish(img)


def rock(w, h, seed=5, base=0xC8A070, dark=0x8A6840, light=0xE0C090, strata=True):
    """Layered rock: the cliffs of the grass levels, in horizontal bands with cracks."""
    n = noise(h, w, 8, seed)
    ys, xs = np.mgrid[0:h, 0:w]
    band = (np.sin((ys + n * 6) * (2 * math.pi / (h / 3))) * 0.5 + 0.5) if strata else n
    img = mix(rgb(light), rgb(base), band * 0.8 + n * 0.2)
    crack = (np.abs(((ys + (n * 5).astype(int)) % (h // 4 if h >= 16 else 4))) == 0)
    img[crack] = rgb(dark)
    s = speckle(h, w, seed + 1)
    img[s > 0.95] = rgb(dark) * 1.05
    return finish(img)


def bricks(w, h, seed=7, base=0xB89070, mortar=0x786048, rows=4, cols=2, dark=None):
    ys, xs = np.mgrid[0:h, 0:w]
    rh = h // rows
    row = ys // rh
    off = (row % 2) * (w // cols // 2)
    bw = w // cols
    n = noise(h, w, 4, seed)
    img = mix(rgb(base), rgb(base) * 0.8, n)
    mortar_mask = ((ys % rh) == rh - 1) | (((xs + off) % bw) == bw - 1)
    img[mortar_mask] = rgb(mortar)
    top = (ys % rh) == 0
    img[top & ~mortar_mask] = np.minimum(rgb(base) * 1.15, 255)
    return finish(img)


def planks(w, h, seed=9, base=0xA06830, dark=0x603818, vertical=False, n_planks=4):
    ys, xs = np.mgrid[0:h, 0:w]
    a, b = (xs, ys) if vertical else (ys, xs)
    size = (w if vertical else h)
    ph = max(2, size // n_planks)
    n = noise(h, w, 3, seed)
    grain = np.sin((b * 0.9 + n * 8 + (a // ph) * 13) * 0.8) * 0.5 + 0.5
    img = mix(rgb(base), rgb(dark), grain * 0.35)
    img[(a % ph) == ph - 1] = rgb(dark) * 0.8
    img[(a % ph) == 0] = np.minimum(rgb(base) * 1.15, 255)
    return finish(img)


def metal(w, h, seed=11, base=0x9098A8, dark=0x585E70):
    ys, xs = np.mgrid[0:h, 0:w]
    n = noise(h, w, 8, seed)
    img = mix(rgb(base), rgb(dark), n * 0.5 + (ys / h) * 0.3)
    rivet = ((xs % 16) == 3) & ((ys % 16) == 3)
    img[rivet] = rgb(0xE0E8F0)
    return finish(img)


def plain(w, h, color, seed=13, amount=0.12):
    n = noise(h, w, 4, seed)
    img = mix(rgb(color), rgb(color) * (1 - amount * 2), n)
    return finish(img)


def fence(w, h, seed=15, base=0xC08048, dark=0x704020):
    """A wooden fence with a gap: posts at the sides, two rails, transparent elsewhere."""
    ys, xs = np.mgrid[0:h, 0:w]
    img = planks(w, h, seed, base, dark, vertical=True, n_planks=4)[..., :3].astype(np.float32)
    alpha = np.zeros((h, w), dtype=np.uint8)
    post = (xs < w // 6) | (xs >= w - w // 6)
    rail = ((ys >= h // 6) & (ys < h // 6 + h // 6)) | ((ys >= h // 2) & (ys < h // 2 + h // 6))
    alpha[post | rail] = 255
    return finish(img, alpha)


def flowers(w, h, seed=17):
    """Tufts of grass seen from above, for the IA decals of the grass levels."""
    ys, xs = np.mgrid[0:h, 0:w]
    cx, cy = w / 2, h / 2
    d = np.sqrt((xs - cx) ** 2 + (ys - cy) ** 2) / (w / 2)
    n = noise(h, w, 3, seed)
    a = np.clip((1 - d) * 2.2 - n * 0.8, 0, 1)
    out = np.zeros((h, w, 4), dtype=np.uint8)
    out[..., :3] = 60
    out[..., 3] = (a * 160).astype(np.uint8)
    return out


def circle_shadow(w, h):
    ys, xs = np.mgrid[0:h, 0:w]
    d = np.sqrt((xs - w / 2 + 0.5) ** 2 + (ys - h / 2 + 0.5) ** 2) / (w / 2)
    out = np.zeros((h, w, 4), dtype=np.uint8)
    out[..., 3] = (np.clip(1 - d, 0, 1) * 255).astype(np.uint8)
    return out


def unknown(w, h, name):
    seed = sum(map(ord, name))
    return plain(w, h, 0x909090, seed, 0.2)


# ------------------------------------------------------------------------------------------------ the table
# path (in the decompilation) -> painter
BOB = "textures/generic/bob_textures."
TABLE = {
    BOB + "05800.rgba16.png": lambda w, h: grass(w, h),
    BOB + "09000.rgba16.png": lambda w, h: rock(w, h, 21),
    BOB + "09800.rgba16.png": lambda w, h: dirt(w, h, 23),
    BOB + "08800.rgba16.png": lambda w, h: fence(w, h),
    BOB + "04800.rgba16.png": lambda w, h: rock(w, h, 25, 0xB89068, 0x7A5A38, 0xD8B888, strata=False),
    BOB + "01800.rgba16.png": lambda w, h: dirt(w, h, 27, 0x8A6A40, 0x5A4020, 0xA88858),
    BOB + "02000.rgba16.png": lambda w, h: planks(w, h, 29, 0x906030, 0x502C10, vertical=True),
    BOB + "03000.rgba16.png": lambda w, h: planks(w, h, 31),
    BOB + "03800.rgba16.png": lambda w, h: bricks(w, h, 33, 0xA0A0A8, 0x606068),
    BOB + "05000.rgba16.png": lambda w, h: bricks(w, h, 35, 0xB89878, 0x705840),
    BOB + "06000.rgba16.png": lambda w, h: planks(w, h, 37, 0xB07840, 0x683C18),
    BOB + "0A000.rgba16.png": lambda w, h: metal(w, h),
    BOB + "0B000.ia16.png": lambda w, h: flowers(w, h),
    "levels/bob/0.rgba16.png": lambda w, h: fence(w, h, 39),
    "levels/bob/1.rgba16.png": lambda w, h: planks(w, h, 41, 0x9A6A38, 0x5A3418),
    "levels/bob/2.rgba16.png": lambda w, h: plain(w, h, 0x707070, 43),
    "levels/bob/3.rgba16.png": lambda w, h: plain(w, h, 0x806040, 45),
    "levels/bob/4.rgba16.png": lambda w, h: rock(w, h, 47),
}


def paint(path, w, h):
    f = TABLE.get(path) or (art.ACTORS.get(path[len("actors/"):]) if path.startswith("actors/") else None)
    img = f(w, h) if f else unknown(w, h, path)
    if img.shape[0] != h or img.shape[1] != w:
        img = np.resize(img, (h, w, 4))
    return img
