"""The actors' textures, drawn for the calculator after the game's (the decompilation has none: they are in the ROM):
Mario's eyes and emblem, coins, stars, the enemies' faces, trees, doors, boxes and the little clouds of dust.

Each painter takes the texture's size and gives an RGBA image (numpy, h x w x 4); the textures the game blends over
the vertex colours (Mario's eyes on his skin) or cuts out (trees) are clear where those show. A picture is drawn 4
times bigger with Pillow and shrunk, for smooth edges. ACTORS maps a texture's path in the decompilation (after
"actors/") to its painter."""
import math

import numpy as np
from PIL import Image, ImageDraw

import glyphs

K = 4   # (the drawing's scale)


class Pic:
    """A texture being drawn: shapes in texel coordinates (x right, y down, from the top left corner)."""

    def __init__(self, w, h, bg=(0, 0, 0, 0)):
        self.w, self.h = w, h
        self.im = Image.new("RGBA", (w * K, h * K), tuple(bg))
        self.d = ImageDraw.Draw(self.im)

    @staticmethod
    def _p(pts):
        return [(x * K, y * K) for x, y in pts]

    def ellipse(self, cx, cy, rx, ry, fill):
        self.d.ellipse([(cx - rx) * K, (cy - ry) * K, (cx + rx) * K - 1, (cy + ry) * K - 1], fill=fill)
        return self

    def ring(self, cx, cy, rx, ry, fill, width):
        self.d.ellipse([(cx - rx) * K, (cy - ry) * K, (cx + rx) * K - 1, (cy + ry) * K - 1], outline=fill,
                       width=max(1, int(width * K)))
        return self

    def poly(self, pts, fill):
        self.d.polygon(self._p(pts), fill=fill)
        return self

    def rect(self, x0, y0, x1, y1, fill):
        self.d.rectangle([x0 * K, y0 * K, x1 * K - 1, y1 * K - 1], fill=fill)
        return self

    def line(self, pts, fill, width):
        self.d.line(self._p(pts), fill=fill, width=max(1, int(width * K)), joint="curve")
        for x, y in pts:   # (round ends)
            r = width * K / 2
            self.d.ellipse([x * K - r, y * K - r, x * K + r, y * K + r], fill=fill)
        return self

    def arc(self, cx, cy, rx, ry, a0, a1, fill, width):
        """An arc from angle a0 to a1 (degrees, 0 to the right, clockwise)."""
        self.d.arc([(cx - rx) * K, (cy - ry) * K, (cx + rx) * K, (cy + ry) * K], a0, a1, fill=fill,
                   width=max(1, int(width * K)))
        return self

    def done(self):
        im = self.im.convert("RGBa").resize((self.w, self.h), Image.BOX).convert("RGBA")
        return np.array(im)


def rgba(h, a=255):
    return ((h >> 16) & 255, (h >> 8) & 255, h & 255, a)


def grid(w, h):
    ys, xs = np.mgrid[0:h, 0:w].astype(np.float32)
    return xs + 0.5, ys + 0.5


def noise(w, h, scale, seed):
    """Smooth value noise in 0..1, tiling."""
    r = np.random.default_rng(seed)
    gh, gw = max(1, h // scale), max(1, w // scale)
    g = r.random((gh, gw))
    ys, xs = np.mgrid[0:h, 0:w]
    fy, fx = ys / scale, xs / scale
    y0, x0 = np.floor(fy).astype(int), np.floor(fx).astype(int)
    ty, tx = fy - y0, fx - x0
    ty, tx = ty * ty * (3 - 2 * ty), tx * tx * (3 - 2 * tx)
    a, b = g[y0 % gh, x0 % gw], g[y0 % gh, (x0 + 1) % gw]
    c, d = g[(y0 + 1) % gh, x0 % gw], g[(y0 + 1) % gh, (x0 + 1) % gw]
    return (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty


def solid(w, h, color, alpha=255):
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = color[:3]
    out[..., 3] = alpha
    return out


def over(dst, src):
    """src (RGBA) over dst (RGBA)."""
    a = src[..., 3:4].astype(np.float32) / 255
    out = dst.astype(np.float32)
    out[..., :3] = src[..., :3] * a + out[..., :3] * (1 - a)
    out[..., 3:4] = np.maximum(out[..., 3:4], src[..., 3:4])
    return np.clip(out, 0, 255).astype(np.uint8)


def shaded_ball(w, h, cx, cy, r, base, light=(-0.5, -0.6), spec=0.6, rim=0.0):
    """A ball (or a disc's worth of one) lit from the upper left: RGBA, clear outside."""
    xs, ys = grid(w, h)
    dx, dy = (xs - cx) / r, (ys - cy) / r
    d2 = dx * dx + dy * dy
    inside = d2 <= 1
    nz = np.sqrt(np.clip(1 - d2, 0, 1))
    lx, ly = light
    lz = math.sqrt(max(0.0, 1 - lx * lx - ly * ly))
    lam = np.clip(dx * lx + dy * ly + nz * lz, 0, 1)
    col = np.array(base[:3], np.float32) * (0.35 + 0.75 * lam)[..., None]
    hl = np.clip(lam, 0, 1) ** 24 * 255 * spec
    col = col + hl[..., None]
    if rim:
        col = col * (1 - rim * (np.sqrt(d2) > 0.9)[..., None])
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = np.clip(col, 0, 255)
    edge = np.clip((1 - np.sqrt(d2)) * r + 0.5, 0, 1)
    out[..., 3] = (edge * 255 * inside + 0).astype(np.uint8)
    return out


# ------------------------------------------------------------------------------------------------ Mario
SKIN = rgba(0xFEC179)
BROWN = rgba(0x3A2010)
BLACK = rgba(0x101010)
WHITE = rgba(0xFFFFFF)
MARIO_BLUE = rgba(0x2050E0)
RED = rgba(0xFF0000)


def mario_eyes(kind):
    """Both of Mario's eyes on his face (the texture is blended over his skin): tall white eyes with blue irises,
    the eyebrows above. kind: center, left, right, up, down, half_closed, closed, dead."""
    def paint(w, h):
        p = Pic(w, h)
        for side, cx in ((-1, 10.0), (1, 22.0)):
            cy = 17.5
            # eyebrow: thick, dark, rising to the outside
            p.line([(cx - 5.5 * side * -1, 7.0), (cx, 5.2), (cx + 4.8 * side, 6.4)], BROWN, 2.6)
            if kind == "closed":
                p.arc(cx, cy - 2, 5.2, 6, 20, 160, BROWN, 1.6)
                continue
            if kind == "dead":
                p.line([(cx - 4, cy - 5), (cx + 4, cy + 5)], BLACK, 2.0)
                p.line([(cx + 4, cy - 5), (cx - 4, cy + 5)], BLACK, 2.0)
                continue
            p.ellipse(cx, cy, 5.6, 9.0, BLACK)
            p.ellipse(cx, cy, 4.7, 8.1, WHITE)
            ox, oy = {"left": (-1.6, 0.5), "right": (1.6, 0.5), "up": (0, -2.5), "down": (0, 2.8)}.get(kind, (0.0, 1.0))
            ix = cx + ox - side * 0.6
            p.ellipse(ix, cy + oy, 3.5, 5.4, rgba(0x2070F0))
            p.ellipse(ix, cy + oy + 0.6, 2.0, 3.4, BLACK)
            p.ellipse(ix - 1.0, cy + oy - 2.6, 1.0, 1.4, WHITE)
            if kind == "half_closed":
                p.poly([(cx - 6, cy - 10), (cx + 6, cy - 10), (cx + 6, cy - 0.5), (cx - 6, cy + 0.5)], SKIN)
                p.arc(cx, cy + 6, 6.5, 6.5, 200, 340, BROWN, 1.2)
        return p.done()
    return paint


def mario_logo(w, h):
    """The emblem on Mario's cap: a red M on a white disc (the cap's red around it)."""
    p = Pic(w, h)
    p.ellipse(16, 16, 13.5, 12.5, WHITE)
    m = [(8.5, 23), (8.5, 9), (11.8, 9), (16, 15.5), (20.2, 9), (23.5, 9), (23.5, 23), (20.4, 23), (20.4, 14.5),
         (16, 20.5), (11.6, 14.5), (11.6, 23)]
    p.poly(m, RED)
    return p.done()


def mario_mustache(w, h):
    p = Pic(w, h)
    col = rgba(0x281408)
    for x, y, rx, ry in ((6, 17, 6, 5), (12, 15, 6, 6), (20, 15, 6, 6), (26, 17, 6, 5), (16, 14, 5, 5),
                         (3.5, 20, 3.5, 3.5), (28.5, 20, 3.5, 3.5)):
        p.ellipse(x, y, rx, ry, col)
    return p.done()


def mario_sideburn(w, h):
    p = Pic(w, h)
    col = rgba(0x4A2810)
    p.poly([(0, 0), (20, 0), (22, 6), (17, 10), (21, 15), (16, 20), (19, 26), (12, 32), (0, 32)], col)
    return p.done()


def mario_button(w, h):
    p = Pic(w, h)
    p.ellipse(16, 16, 11, 11, rgba(0xB08800))
    p.ellipse(16, 16, 9.5, 9.5, rgba(0xFFE000))
    p.ellipse(14, 13.5, 3.5, 3, rgba(0xFFFFA0))
    return p.done()


def wing(tip):
    """A white wing (the Wing Cap's): feathers fanning out, grey lines between them."""
    def paint(w, h):
        p = Pic(w, h)
        n = 4 if tip else 5
        for i in range(n):
            y0 = 4 + i * (56 / n)
            x1 = w - 1 - i * (3 if tip else 2)
            p.poly([(0, y0 + 2), (x1, y0), (x1 - 2, y0 + 56 / n + 3), (0, y0 + 56 / n + 4)], rgba(0xC8C8D8))
            p.poly([(1, y0 + 3), (x1 - 1.5, y0 + 1.2), (x1 - 3, y0 + 56 / n + 1.5), (1, y0 + 56 / n + 2.5)], WHITE)
        return p.done()
    return paint


def metal(w, h):
    """The Metal Cap's mirror: the sky above, a bright horizon, dark ground below (the N64 maps it by normals)."""
    xs, ys = grid(w, h)
    t = ys / h
    v = np.where(t < 0.45, 170 + 70 * (t / 0.45), np.where(t < 0.55, 250, 140 - 100 * (t - 0.55) / 0.45))
    v = v + 18 * np.sin(xs / w * 2 * math.pi * 2) * (t < 0.5)
    out = np.zeros((h, w, 4), np.uint8)
    out[..., 0] = np.clip(v * 0.95, 0, 255)
    out[..., 1] = np.clip(v * 0.97, 0, 255)
    out[..., 2] = np.clip(v * 1.05 + 10, 0, 255)
    out[..., 3] = 255
    return out


# ------------------------------------------------------------------------------------------------ coins, stars
def coin(kind):
    """A coin (intensity and alpha: the game tints it yellow, red or blue): a disc with a raised rim and a slot,
    seen from the front, tilted or edge on."""
    def paint(w, h):
        rx = {"front": 14.5, "tilt_left": 10, "tilt_right": 10, "side": 3.2}[kind]
        xs, ys = grid(w, h)
        dx, dy = (xs - 16) / rx, (ys - 16) / 14.5
        d = np.sqrt(dx * dx + dy * dy)
        inside = d <= 1
        lit = 0.75 + 0.25 * np.clip(-(dx * 0.6 + dy * 0.8), -1, 1)
        if kind == "tilt_left":
            lit = lit + 0.12 * dx
        elif kind == "tilt_right":
            lit = lit - 0.12 * dx
        v = 205 * lit
        v = np.where(d > 0.8, 245 * lit, v)                       # the rim
        v = np.where((d > 0.74) & (d <= 0.8), 150, v)
        if kind != "side":
            slot = (np.abs(xs - 16) < max(0.8, rx * 0.07)) & (np.abs(ys - 16) < 7)
            v = np.where(slot, 120, v)
        a = np.clip((1 - d) * min(rx, 14.5) + 0.5, 0, 1) * 255
        out = np.zeros((h, w, 4), np.uint8)
        out[..., :3] = np.clip(v, 0, 255)[..., None]
        out[..., 3] = (a * inside).astype(np.uint8)
        return out
    return paint


def star_surface(w, h):
    """The Power Star's shine (mapped by the normals): gold, a white glint up high, orange low down."""
    xs, ys = grid(w, h)
    t = ys / h
    base = np.stack([np.full_like(t, 255), 200 + 40 * (1 - t), 40 + 60 * (1 - t)], -1)
    glint = np.exp(-(((xs - w * 0.35) / (w * 0.18)) ** 2 + ((ys - h * 0.28) / (h * 0.1)) ** 2))
    base = base + glint[..., None] * np.array([0, 40, 180])
    low = np.clip((t - 0.65) / 0.35, 0, 1)
    base = base * (1 - 0.35 * low[..., None]) + low[..., None] * np.array([30, -20, -40])
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = np.clip(base, 0, 255)
    out[..., 3] = 255
    return out


def star_eye(w, h):
    p = Pic(w, h)
    p.ellipse(16, 16, 6.5, 12.5, BLACK)
    return p.done()


# ------------------------------------------------------------------------------------------------ enemies
GOOMBA = rgba(0x9C5020)


def goomba_face(blink):
    """A Goomba's face on its brown head: angry black eyebrows, white eyes, a mouth with two fangs."""
    def paint(w, h):
        p = Pic(w, h, GOOMBA)
        p.rect(0, 22, 32, 32, rgba(0x8A4418))
        for side, cx in ((-1, 10.5), (1, 21.5)):
            if blink:
                p.line([(cx - 4, 13), (cx + 4, 13)], BLACK, 1.6)
            else:
                p.ellipse(cx, 12.5, 4.2, 6.0, BLACK)
                p.ellipse(cx, 12.5, 3.3, 5.1, WHITE)
                p.ellipse(cx + side * -0.8, 13.5, 1.9, 3.2, BLACK)
            # the eyebrows slope down to the middle
            out, inn = cx + 5.5 * side, cx - 5.5 * side
            p.poly([(out, 2.5), (inn, 6.5), (inn, 9.5), (out, 5.5)], BLACK)
        p.poly([(6, 24), (26, 24), (23, 29), (9, 29)], rgba(0x401808))
        p.poly([(9, 29), (12, 24.5), (14, 29)], WHITE)
        p.poly([(18, 29), (20, 24.5), (23, 29)], WHITE)
        return p.done()
    return paint


def goomba_body(w, h):
    xs, ys = grid(w, h)
    n = noise(w, h, 8, 3)
    out = np.zeros((h, w, 4), np.uint8)
    out[..., 0] = 236 - 30 * n
    out[..., 1] = 196 - 30 * n
    out[..., 2] = 130 - 30 * n
    out[..., 3] = 255
    return out


def bobomb_side(right, color):
    """Half of a Bob-omb's round body (a picture that always faces the camera): a ball with a shine, the left or
    right half."""
    def paint(w, h):
        full = shaded_ball(2 * w, h, w, h / 2, w - 1, color, spec=0.9)
        p = Pic(2 * w, h)
        p.ring(w, h / 2, w - 1.2, w - 1.2, rgba(0x000000), 1.2)
        full = over(full, p.done())
        return full[:, w:] if right else full[:, :w]
    return paint


def bobomb_eyes(blink):
    def paint(w, h):
        p = Pic(w, h)
        for cx in (8, 24):
            if blink:
                p.line([(cx - 4, 16), (cx + 4, 16)], WHITE, 2)
            else:
                p.ellipse(cx, 16, 5.5, 13, WHITE)
                p.ellipse(cx, 17, 2.4, 6.5, BLACK)
        return p.done()
    return paint


def chomp_eye(w, h):
    p = Pic(w, h, WHITE)
    p.ellipse(16, 16, 11, 11, rgba(0xE8E8F0))
    p.ellipse(16, 17, 7.5, 7.5, BLACK)
    p.ellipse(13.5, 13.5, 2.2, 2.2, WHITE)
    return p.done()


def chomp_shine(bright):
    """The Chain Chomp's black metal, mapped by the normals: dark with a broad glint."""
    def paint(w, h):
        xs, ys = grid(w, h)
        g = np.exp(-(((xs - w * 0.35) / (w * 0.25)) ** 2 + ((ys - h * 0.3) / (h * 0.22)) ** 2))
        v = 30 + (170 if bright else 90) * g
        out = solid(w, h, (0, 0, 0))
        out[..., 0] = out[..., 1] = np.clip(v, 0, 255)
        out[..., 2] = np.clip(v + 12, 0, 255)
        return out
    return paint


def teeth(w, h):
    p = Pic(w, h, rgba(0xF4F4F4))
    for i in range(4):
        p.line([(i * 8 + 4, 0), (i * 8 + 4, 32)], rgba(0xC0C0C8), 0.8)
    return p.done()


def tongue(w, h):
    p = Pic(w, h, rgba(0xE03040))
    p.line([(w / 2, 2), (w / 2, h - 2)], rgba(0xA01828), 1.2)
    return p.done()


# ------------------------------------------------------------------------------------------------ trees
def bubbly_tree(right):
    """Half of the round tree of the grass levels (a picture facing the camera): a lumpy green ball with darker
    lumps, on a short trunk; the left half is in the light."""
    def paint(w, h):
        W2 = 2 * w
        p = Pic(W2, h)
        p.rect(w - 3.5, 46, w + 3.5, 64, rgba(0x6A3A18))
        p.rect(w - 2.5, 46, w - 0.5, 64, rgba(0x8A5226))
        dark, mid, light = rgba(0x1E6A10), rgba(0x2E9A18), rgba(0x58C428)
        lumps = [(w, 26, 26, 24)] + [(w + 22 * math.cos(a), 26 + 21 * math.sin(a), 7, 7)
                                     for a in np.linspace(0, 2 * math.pi, 12, endpoint=False)]
        for x, y, rx, ry in lumps:
            p.ellipse(x, y, rx + 1, ry + 1, dark)
        for x, y, rx, ry in lumps:
            p.ellipse(x, y, rx, ry, mid)
        for x, y, r in ((w - 9, 15, 6), (w - 15, 24, 5), (w - 4, 9, 4), (w + 6, 14, 4), (w - 10, 33, 4)):
            p.ellipse(x, y, r, r * 0.8, light)
        for x, y, r in ((w + 12, 32, 6), (w + 4, 40, 5), (w + 16, 20, 4)):
            p.ellipse(x, y, r, r * 0.8, rgba(0x247A12))
        img = p.done()
        half = img[:, w:] if right else img[:, :w]
        if right:
            half = half.copy()
            half[..., :3] = (half[..., :3] * 0.85).astype(np.uint8)
        return half
    return paint


def pine_tree(w, h):
    p = Pic(w, h)
    p.rect(13.5, 52, 18.5, 64, rgba(0x5A3216))
    for i, (y0, y1, half) in enumerate(((2, 22, 8), (12, 36, 11), (24, 50, 14), (36, 58, 15.5))):
        p.poly([(16, y0), (16 + half, y1), (16 - half, y1)], rgba(0x0E4A1E))
        p.poly([(16, y0 + 2), (16 + half - 2, y1 - 1.5), (16 - half + 1, y1 - 1.5)], rgba(0x1C6E2C))
        p.poly([(16, y0 + 3), (16 - half + 3, y1 - 2), (13, y1 - 2)], rgba(0x2E8A3A))
    return p.done()


# ------------------------------------------------------------------------------------------------ doors, pipes, boxes
def wood_grain(w, h, base, dark, seed, vertical=True):
    xs, ys = grid(w, h)
    n = noise(w, h, 4, seed)
    a = xs if vertical else ys
    g = np.sin(a * 1.7 + n * 6) * 0.5 + 0.5
    col = np.array(base[:3], np.float32) * (1 - g[..., None] * 0.25) + np.array(dark[:3]) * g[..., None] * 0.25
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = np.clip(col, 0, 255)
    out[..., 3] = 255
    return out


def door(kind):
    """The castle's doors: polished wood with panels, rough planks, or grey metal with rivets."""
    def paint(w, h):
        if kind == "metal":
            img = solid(w, h, (150, 150, 160))
            xs, ys = grid(w, h)
            img[..., :3] = np.clip(img[..., :3] - (noise(w, h, 4, 9) * 30)[..., None], 0, 255)
            p = Pic(w, h)
            p.rect(0, 0, w, 2, rgba(0x606068)).rect(0, h - 2, w, h, rgba(0x606068))
            p.rect(0, 0, 2, h, rgba(0x606068)).rect(w - 2, 0, w, h, rgba(0x606068))
            for y in (5, h / 2, h - 5):
                for x in (5, w - 5):
                    p.ellipse(x, y, 1.5, 1.5, rgba(0xD8D8E0))
            return over(img, p.done())
        if kind == "rough":
            img = wood_grain(w, h, rgba(0x9A6030), rgba(0x5A3010), 11)
            p = Pic(w, h)
            for x in (8, 16, 24):
                p.line([(x, 0), (x, h)], rgba(0x4A2408), 1.0)
            p.rect(0, 10, w, 13, rgba(0x6A3A18)).rect(0, h - 14, w, h - 11, rgba(0x6A3A18))
            return over(img, p.done())
        img = wood_grain(w, h, rgba(0xB06A30), rgba(0x6A3010), 13)
        p = Pic(w, h)
        for y0, y1 in ((4, h * 0.45), (h * 0.52, h - 4)):
            p.rect(4, y0, w - 4, y1, rgba(0x7A4018))
            p.rect(5.5, y0 + 1.5, w - 5.5, y1 - 1.5, rgba(0xC07A3A))
        return over(img, p.done())
    return paint


def door_overlay(kind):
    """What the game draws over a door: its knob and the frame's shadow (clear elsewhere)."""
    def paint(w, h):
        p = Pic(w, h)
        if kind == "polished":
            p.ellipse(w - 6, h * 0.55, 2.5, 2.5, rgba(0xE0B020))
            p.ellipse(w - 6.6, h * 0.55 - 0.7, 1, 1, rgba(0xFFF0A0))
        else:
            p.ellipse(w - 7, h / 2, 3, 3, rgba(0x404048))
            p.ellipse(w - 7.5, h / 2 - 0.8, 1.2, 1.2, rgba(0xA0A0A8))
        return p.done()
    return paint


def star_door_sign(n):
    """The plaque on a star door: a star, and the stars it takes."""
    def paint(w, h):
        p = Pic(w, h, rgba(0x302010))
        star(p, 16, 13 if n else 16, 11 if n else 13, rgba(0xFFD800))
        if n:
            g = glyphs._grid(glyphs.DIALOG[str(n)])
            for y, x in zip(*np.nonzero(g)):
                p.rect(13.5 + x * 1.2, 22 + y * 1.2, 14.7 + x * 1.2, 23.2 + y * 1.2, WHITE)
        return p.done()
    return paint


def star(p, cx, cy, r, fill):
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        rr = r if i % 2 == 0 else r * 0.45
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    p.poly(pts, fill)


def door_lock(w, h):
    p = Pic(w, h, rgba(0xC8A030))
    p.ellipse(8, 11, 3, 3, BLACK)
    p.poly([(6.5, 12), (9.5, 12), (10.5, 22), (5.5, 22)], BLACK)
    return p.done()


def pipe_side(w, h):
    """The green warp pipe's side: lit from the left, dark at the back."""
    xs, ys = grid(w, h)
    t = xs / w
    v = 0.45 + 0.55 * np.sin(np.clip(t, 0, 1) * math.pi) ** 0.6
    v = v + 0.35 * np.exp(-((t - 0.3) / 0.08) ** 2)
    out = solid(w, h, (0, 0, 0))
    out[..., 0] = np.clip(40 * v, 0, 255)
    out[..., 1] = np.clip(190 * v, 0, 255)
    out[..., 2] = np.clip(50 * v, 0, 255)
    return out


def pipe_top(w, h):
    p = Pic(w, h, rgba(0x20A030))
    p.ellipse(16, 16, 15, 15, rgba(0x30C040))
    p.ellipse(16, 16, 11, 11, rgba(0x082008))
    p.ellipse(16, 16, 9, 9, BLACK)
    return p.done()


def box_front(kind):
    """The front of a ! block: yellow with a red !, or the caps' (red wing cap, green metal cap, blue vanish cap)."""
    def paint(w, h):
        bg = {"exclamation": 0xF0C000, "wing": 0xE02020, "metal": 0x20A040, "vanish": 0x2040E0}[kind]
        p = Pic(w, h, rgba(bg))
        p.rect(0, 0, w, 2, rgba(0xFFFFFF, 160)).rect(0, h - 2, w, h, rgba(0x000000, 120))
        p.rect(0, 0, 2, h, rgba(0xFFFFFF, 120)).rect(w - 2, 0, w, h, rgba(0x000000, 100))
        if kind == "exclamation":
            p.poly([(12, 5), (20, 5), (18, 20), (14, 20)], rgba(0xD02000))
            p.ellipse(16, 25, 3, 3, rgba(0xD02000))
        else:
            p.ellipse(16, 18, 10, 8, WHITE)
            p.ellipse(16, 15, 9, 7, rgba(bg))
            p.rect(5, 17, 27, 21, WHITE)
            if kind == "wing":
                p.poly([(3, 12), (9, 15), (7, 17), (1, 15)], WHITE)
                p.poly([(29, 12), (23, 15), (25, 17), (31, 15)], WHITE)
        return p.done()
    return paint


def box_side(kind):
    def paint(w, h):
        bg = {"exclamation": 0xF0C000, "wing": 0xE02020, "metal": 0x20A040, "vanish": 0x2040E0}[kind]
        img = solid(w, h, rgba(bg))
        xs, ys = grid(w, h)
        shade = 1 - 0.18 * (ys / h)
        img[..., :3] = np.clip(img[..., :3] * shade[..., None], 0, 255)
        p = Pic(w, h)
        p.rect(0, 0, w, 1.5, rgba(0xFFFFFF, 140)).rect(0, h - 1.5, w, h, rgba(0x000000, 120))
        return over(img, p.done())
    return paint


def box_outline(w, h):
    p = Pic(w, h)
    for i in range(0, 32, 4):
        for x0, y0, x1, y1 in ((i, 0, i + 2, 2), (i, 30, i + 2, 32), (0, i, 2, i + 2), (30, i, 32, i + 2)):
            p.rect(x0, y0, x1, y1, rgba(0xF0C000))
    return p.done()


def exclamation_point(w, h):
    p = Pic(w, h)
    p.poly([(4, 3), (12, 3), (10, 21), (6, 21)], rgba(0xF0C000))
    p.ellipse(8, 26.5, 3, 3, rgba(0xF0C000))
    return p.done()


def signpost(front):
    def paint(w, h):
        img = wood_grain(w, h, rgba(0xB07840), rgba(0x6A4018), 21, vertical=False)
        p = Pic(w, h)
        p.rect(0, 0, w, 1.5, rgba(0x5A3010)).rect(0, h - 1.5, w, h, rgba(0x5A3010))
        if front:
            for y in (8, 13, 18, 23):
                p.line([(5, y), (27 - (y % 5) * 2, y)], rgba(0x4A2408), 1.4)
        return over(img, p.done())
    return paint


# ------------------------------------------------------------------------------------------------ dust, sparkles
def puff(kind, i):
    """The little clouds Mario kicks up (intensity and alpha): white puffs, swelling and thinning frame by frame."""
    def paint(w, h):
        frames = 7 if kind == "walk" else 6
        t = i / (frames - 1)
        rng = np.random.default_rng(100 + i)
        xs, ys = grid(w, h)
        a = np.zeros((h, w), np.float32)
        for _ in range(5):
            cx, cy = 16 + rng.normal() * 5 * (0.5 + t), 16 + rng.normal() * 5 * (0.5 + t)
            r = 6 + 6 * t + rng.random() * 3
            a = np.maximum(a, np.clip(1 - np.hypot(xs - cx, ys - cy) / r, 0, 1))
        a = np.clip(a * 1.8, 0, 1) * (1 - 0.7 * t)
        out = np.zeros((h, w, 4), np.uint8)
        out[..., :3] = (220 + 35 * a)[..., None].astype(np.uint8)
        out[..., 3] = (a * 255).astype(np.uint8)
        return out
    return paint


def sparkle(i, ia=False):
    """A twinkle: a four-pointed star of light, growing then shrinking."""
    def paint(w, h):
        s = [0.5, 0.8, 1.0, 0.85, 0.6, 0.35][i % 6]
        xs, ys = grid(w, h)
        dx, dy = np.abs(xs - 16) / 15, np.abs(ys - 16) / 15
        a = np.clip(1 - (np.sqrt(dx) + np.sqrt(dy)) / s, 0, 1)
        core = np.clip(1 - np.hypot(dx, dy) / (0.35 * s), 0, 1)
        out = np.zeros((h, w, 4), np.uint8)
        if ia:
            out[..., :3] = 255
        else:
            out[..., 0], out[..., 1] = 255, (230 + 25 * core).astype(np.uint8)
            out[..., 2] = (90 + 165 * core).astype(np.uint8)
        out[..., 3] = (np.clip(a * 1.5 + core, 0, 1) * 255).astype(np.uint8)
        return out
    return paint


def explosion(i):
    def paint(w, h):
        t = i / 6
        xs, ys = grid(w, h)
        n = noise(w, h, 6, 40 + i)
        d = np.hypot(xs - 16, ys - 16) / (8 + 8 * t) + n * 0.4
        a = np.clip(1.2 - d, 0, 1)
        out = np.zeros((h, w, 4), np.uint8)
        heat = np.clip(1 - d, 0, 1) * (1 - t * 0.8)
        out[..., 0] = 255
        out[..., 1] = np.clip(120 + 135 * heat, 0, 255).astype(np.uint8)
        out[..., 2] = np.clip(40 + 200 * heat - 100 * t, 0, 255).astype(np.uint8)
        out[..., 3] = (np.where(a > 0.5, 255, 0)).astype(np.uint8)
        return out
    return paint


def splash(i):
    def paint(w, h):
        p = Pic(w, h)
        t = i / 7
        for k in range(6):
            x = 16 + (k - 2.5) * (3 + 6 * t)
            top = 60 - (50 * math.sin(min(1, t * 1.3 + 0.1) * math.pi) * (1 - abs(k - 2.5) / 4))
            p.line([(16 + (k - 2.5) * 2, 62), (x, top)], rgba(0xC8E8FF, 220), 3)
            p.ellipse(x, top, 2.5, 2.5, rgba(0xFFFFFF, 230))
        return p.done()
    return paint


def wave(i):
    def paint(w, h):
        xs, ys = grid(w, h)
        r = np.hypot(xs - 16, ys - 16)
        rad = 8 + i * 2.2
        a = np.clip(1 - np.abs(r - rad) / 2.2, 0, 1) * (1 - i / 5)
        out = np.zeros((h, w, 4), np.uint8)
        out[..., :3] = 255
        out[..., 3] = (a * 255).astype(np.uint8)
        return out
    return paint


def bubble(w, h):
    p = Pic(w, h)
    p.ring(16, 16, 14, 14, rgba(0xD0F0FF, 230), 2)
    p.ellipse(11, 10, 3.5, 2.5, rgba(0xFFFFFF, 240))
    return p.done()


def small_ball(color, light=True):
    def paint(w, h):
        return shaded_ball(w, h, w / 2, h / 2, w / 2 - 0.5, color, spec=0.5 if light else 0)
    return paint


def leaf(w, h):
    p = Pic(w, h)
    p.poly([(8, 1), (14, 8), (8, 15), (2, 8)], rgba(0x40A020))
    p.line([(8, 2), (8, 14)], rgba(0x206010), 0.8)
    return p.done()


def mist(w, h):
    xs, ys = grid(w, h)
    a = np.clip(1 - np.hypot(xs - 16, ys - 16) / 15, 0, 1) ** 1.5
    out = np.zeros((h, w, 4), np.uint8)
    out[..., :3] = 255
    out[..., 3] = (a * 200).astype(np.uint8)
    return out


def heart(w, h):
    p = Pic(w, h)
    p.ellipse(10.5, 12, 7, 7, rgba(0xFF2050))
    p.ellipse(21.5, 12, 7, 7, rgba(0xFF2050))
    p.poly([(4, 14), (28, 14), (16, 29)], rgba(0xFF2050))
    p.ellipse(9, 10, 2.5, 2, rgba(0xFFB0C0))
    return p.done()


def mushroom_1up(w, h):
    """The green 1-up mushroom (a picture facing the camera): a green cap with white spots on a pale stalk with eyes."""
    p = Pic(w, h)
    p.rect(8, 34, 24, 60, rgba(0xF8E8C8))
    p.ellipse(16, 60, 8, 3, rgba(0xF8E8C8))
    p.ellipse(13, 44, 1.5, 3.5, BLACK).ellipse(19, 44, 1.5, 3.5, BLACK)
    p.ellipse(16, 24, 15.5, 14, rgba(0x108020))
    p.ellipse(16, 23, 14.5, 13, rgba(0x30C040))
    p.rect(0, 30, 32, 36, rgba(0, 0))
    p.ellipse(16, 18, 6, 5, WHITE).ellipse(5, 25, 3.5, 4, WHITE).ellipse(27, 25, 3.5, 4, WHITE)
    return p.done()


# ------------------------------------------------------------------------------------------------ the table
def _mario():
    t = {}
    for k in ("center", "closed", "half_closed", "dead"):
        t[f"mario/mario_eyes_{k}.rgba16.png"] = mario_eyes(k)
    for k in ("left", "right", "up", "down"):
        t[f"mario/mario_eyes_{k}_unused.rgba16.png"] = mario_eyes(k)
    t["mario/mario_logo.rgba16.png"] = mario_logo
    t["mario_cap/mario_cap_logo.rgba16.png"] = mario_logo
    t["mario/mario_mustache.rgba16.png"] = mario_mustache
    t["mario/mario_sideburn.rgba16.png"] = mario_sideburn
    t["mario/mario_overalls_button.rgba16.png"] = mario_button
    for d in ("mario/mario", "mario_cap/mario_cap"):
        t[f"{d}_wing.rgba16.png"] = wing(False)
        t[f"{d}_wing_tip.rgba16.png"] = wing(True)
        t[f"{d}_metal.rgba16.png"] = metal
        t[f"{d}_metal_wing_unused.rgba16.png"] = wing(False)
        t[f"{d}_metal_wing_tip_unused.rgba16.png"] = wing(True)
    return t


ACTORS = dict(_mario())
ACTORS.update({
    "coin/coin_front.ia16.png": coin("front"),
    "coin/coin_tilt_left.ia16.png": coin("tilt_left"),
    "coin/coin_tilt_right.ia16.png": coin("tilt_right"),
    "coin/coin_side.ia16.png": coin("side"),
    "star/star_surface.rgba16.png": star_surface,
    "star/star_eye.rgba16.png": star_eye,
    "goomba/goomba_face.rgba16.png": goomba_face(False),
    "goomba/goomba_face_blink.rgba16.png": goomba_face(True),
    "goomba/goomba_body.rgba16.png": goomba_body,
    "bobomb/bob-omb_left_side.rgba16.png": bobomb_side(False, rgba(0x202028)),
    "bobomb/bob-omb_right_side.rgba16.png": bobomb_side(True, rgba(0x202028)),
    "bobomb/bob-omb_buddy_left_side.rgba16.png": bobomb_side(False, rgba(0xF070A0)),
    "bobomb/bob-omb_buddy_right_side.rgba16.png": bobomb_side(True, rgba(0xF070A0)),
    "bobomb/bob-omb_eyes.rgba16.png": bobomb_eyes(False),
    "bobomb/bob-omb_eyes_blink.rgba16.png": bobomb_eyes(True),
    "chain_chomp/chain_chomp_eye.rgba16.png": chomp_eye,
    "chain_chomp/chain_chomp_bright_shine.rgba16.png": chomp_shine(True),
    "chain_chomp/chain_chomp_dull_shine.rgba16.png": chomp_shine(False),
    "chain_chomp/chain_chomp_tooth.rgba16.png": teeth,
    "chain_chomp/chain_chomp_tongue.rgba16.png": tongue,
    "chain_ball/chain_ball.rgba16.png": chomp_shine(True),
    "tree/tree_left_side.rgba16.png": bubbly_tree(False),
    "tree/tree_right_side.rgba16.png": bubbly_tree(True),
    "tree/pine_tree.rgba16.png": pine_tree,
    "door/polished_wooden_door.rgba16.png": door("polished"),
    "door/polished_wooden_door_overlay.rgba16.png": door_overlay("polished"),
    "door/rough_wooden_door.rgba16.png": door("rough"),
    "door/rough_wooden_door_overlay.rgba16.png": door_overlay("rough"),
    "door/metal_door.rgba16.png": door("metal"),
    "door/metal_door_overlay.rgba16.png": door_overlay("metal"),
    "door/zero_star_door_sign.rgba16.png": star_door_sign(0),
    "door/one_star_door_sign.rgba16.png": star_door_sign(1),
    "door/three_star_door_sign.rgba16.png": star_door_sign(3),
    "door/door_lock.rgba16.png": door_lock,
    "warp_pipe/warp_pipe_side.rgba16.png": pipe_side,
    "warp_pipe/warp_pipe_top.rgba16.png": pipe_top,
    "exclamation_box/exclamation_box_front.rgba16.png": box_front("exclamation"),
    "exclamation_box/exclamation_box_side.rgba16.png": box_side("exclamation"),
    "exclamation_box/wing_cap_box_front.rgba16.png": box_front("wing"),
    "exclamation_box/wing_cap_box_side.rgba16.png": box_side("wing"),
    "exclamation_box/metal_cap_box_front.rgba16.png": box_front("metal"),
    "exclamation_box/metal_cap_box_side.rgba16.png": box_side("metal"),
    "exclamation_box/vanish_cap_box_front.rgba16.png": box_front("vanish"),
    "exclamation_box/vanish_cap_box_side.rgba16.png": box_side("vanish"),
    "exclamation_box_outline/exclamation_box_outline.rgba16.png": box_outline,
    "exclamation_box_outline/exclamation_point.rgba16.png": exclamation_point,
    "wooden_signpost/wooden_signpost_front.rgba16.png": signpost(True),
    "wooden_signpost/wooden_signpost_back.rgba16.png": signpost(False),
    "bubble/bubble.rgba16.png": bubble,
    "bubble/mr_i_bubble.rgba16.png": bubble,
    "water_bubble/water_bubble.rgba16.png": bubble,
    "mist/mist.ia16.png": mist,
    "leaves/leaf.rgba16.png": leaf,
    "heart/spinning_heart.rgba16.png": heart,
    "mushroom_1up/1-up_mushroom.rgba16.png": mushroom_1up,
    "white_particle/snow_particle.rgba16.png": small_ball(rgba(0xFFFFFF)),
    "white_particle_small/small_snow_particle.rgba16.png": small_ball(rgba(0xFFFFFF)),
    "sand/sand_particle.rgba16.png": small_ball(rgba(0xE8C878), False),
    "dirt/dirt_particle.rgba16.png": small_ball(rgba(0x8A5A2A), False),
    "pebble/pebble.rgba16.png": small_ball(rgba(0xA0A0A0)),
    "burn_smoke/burn_smoke.ia16.png": puff("stomp", 2),
})
ACTORS.update({f"walk_smoke/walk_smoke_{i}.ia16.png": puff("walk", i) for i in range(7)})
ACTORS.update({f"stomp_smoke/stomp_smoke_{i}.ia16.png": puff("stomp", i) for i in range(6)})
ACTORS.update({f"sparkle/sparkle_{i}.rgba16.png": sparkle(i) for i in range(6)})
ACTORS.update({f"sparkle_animation/sparkle_animation_{i}.ia16.png": sparkle(i, True) for i in range(5)})
ACTORS.update({f"explosion/explosion_{i}.rgba16.png": explosion(i) for i in range(7)})
ACTORS.update({f"water_splash/water_splash_{i}.rgba16.png": splash(i) for i in range(8)})
ACTORS.update({f"water_wave/water_wave_{i}.ia16.png": wave(i) for i in range(4)})


# ------------------------------------------------------------------------------------------------ Peach, Toad, Lakitu
# (these are multiplied by the vertex colours: white where the skin or the cloth shows as it is)
def peach_eye(lid):
    """One of Peach's eyes (the other is the same, mirrored): blue, long lashes; lid: 0 open .. 3 closed."""
    def paint(w, h):
        p = Pic(w, h, WHITE)
        cx, cy = 16, 18
        if lid < 3:
            p.ellipse(cx, cy, 8.5, 10, rgba(0x302020))
            p.ellipse(cx, cy, 7.5, 9, rgba(0xFFFFFF))
            p.ellipse(cx + 0.5, cy + 1, 5.5, 7, rgba(0x3060D0))
            p.ellipse(cx + 0.5, cy + 1.5, 3, 4, rgba(0x102040))
            p.ellipse(cx - 1.5, cy - 2, 1.4, 1.8, WHITE)
        top = cy - 10 + lid * 6.5
        if lid:
            p.rect(cx - 10, cy - 12, cx + 10, top, WHITE)
            p.line([(cx - 8.5, top), (cx + 8.5, top)], rgba(0x302020), 1.6)
        else:
            p.arc(cx, cy, 8.5, 10, 200, 340, rgba(0x302020), 1.6)
        for k in range(3):   # the lashes, out to the side
            x = cx + 5 + k * 2.2
            y = (top if lid else cy - 7) + k * 2
            p.line([(x, y), (x + 3, y - 2.5)], rgba(0x302020), 1.0)
        return p.done()
    return paint


def peach_lips(scrunched):
    def paint(w, h):
        p = Pic(w, h, WHITE)
        col = rgba(0xFF6080)
        if scrunched:
            p.ellipse(16, 16, 5, 4, col)
            p.line([(13, 16), (19, 16)], rgba(0xB03050), 0.8)
        else:
            p.ellipse(16, 14.5, 8, 2.8, col)
            p.ellipse(16, 17.5, 7, 3.2, col)
            p.line([(9, 16), (23, 16)], rgba(0xB03050), 0.8)
        return p.done()
    return paint


def peach_dress(w, h):
    """The folds of Peach's dress (its pink is the vertex colours'): soft vertical light and shade."""
    xs, ys = grid(w, h)
    v = 225 + 30 * np.sin(xs / w * 2 * math.pi * 2) + 8 * noise(w, h, 4, 5)
    out = solid(w, h, (0, 0, 0))
    out[..., 0] = out[..., 1] = out[..., 2] = np.clip(v, 0, 255)
    return out


def jewel(color):
    def paint(w, h):
        img = shaded_ball(w, h, w / 2, h / 2, w / 2 - 0.5, color, spec=1.0)
        img[..., 3] = np.where(img[..., 3] > 0, 255, 0)
        return over(solid(w, h, (255, 230, 100)), img)
    return paint


def nostril(w, h):
    p = Pic(w, h, WHITE)
    p.ellipse(8, 9, 2, 1.4, rgba(0xC08070))
    return p.done()


def toad_face(w, h):
    p = Pic(w, h, WHITE)
    p.ellipse(11, 12, 2.6, 5, BLACK).ellipse(21, 12, 2.6, 5, BLACK)
    p.ellipse(10.4, 10, 0.9, 1.4, WHITE).ellipse(20.4, 10, 0.9, 1.4, WHITE)
    p.arc(16, 20, 4, 3, 20, 160, rgba(0x602010), 1.2)
    return p.done()


def toad_head(w, h):
    p = Pic(w, h, WHITE)
    p.ellipse(16, 16, 10.5, 10.5, rgba(0xF02020))
    return p.done()


def lakitu_eyes(closed):
    """Lakitu's two eyes behind his goggles' glass."""
    def paint(w, h):
        p = Pic(w, h, rgba(0xF0E070))
        for cx in (16, 48):
            p.ellipse(cx, 16, 14, 14, rgba(0x505058))
            p.ellipse(cx, 16, 11.5, 11.5, rgba(0xFFFFFF))
            if closed:
                p.rect(cx - 12, 4, cx + 12, 17, rgba(0xF0E070))
                p.line([(cx - 10, 17), (cx + 10, 17)], BLACK, 1.5)
            else:
                p.ellipse(cx + (2 if cx < 32 else -2), 17, 4.5, 7, BLACK)
                p.ellipse(cx + (1 if cx < 32 else -3), 13.5, 1.5, 2, WHITE)
        return p.done()
    return paint


def hexes(w, h, bg, fg, size, seed=0):
    p = Pic(w, h, bg)
    step_y, step_x = size * 1.7, size * 2
    y = -size
    row = 0
    while y < h + size:
        x = -size + (size if row % 2 else 0)
        while x < w + size:
            pts = [(x + size * 0.85 * math.cos(a), y + size * 0.85 * math.sin(a))
                   for a in np.linspace(0, 2 * math.pi, 7)[:-1]]
            p.poly(pts, fg)
            x += step_x
        y += step_y
        row += 1
    return p


def lakitu_shell(w, h):
    return hexes(w, h, rgba(0xF0F0F0), rgba(0x30A040), 6).done()


def lakitu_frown(w, h):
    p = Pic(w, h, WHITE)
    p.arc(16, 22, 7, 5, 200, 340, rgba(0x401808), 1.6)
    return p.done()


def camera_lens(w, h):
    p = Pic(w, h, rgba(0x202020))
    p.ellipse(8, 8, 6.5, 6.5, rgba(0x304080))
    p.ellipse(6, 6, 2, 2, rgba(0xC0D8FF))
    return p.done()


# ------------------------------------------------------------------------------------------------ more enemies
def koopa_shell(front, top=False):
    """A Koopa's shell: green plates edged in white on the back; the yellow belly in rows on the front."""
    def paint(w, h):
        if front:
            p = Pic(w, h, rgba(0xF0D060))
            for y in (8, 16, 24):
                p.line([(0, y), (32, y)], rgba(0xA07820), 1.2)
            p.line([(16, 0), (16, 32)], rgba(0xA07820), 1.2)
            if top:
                p.rect(0, 0, 32, 4, WHITE)
            return p.done()
        return hexes(w, h, WHITE, rgba(0x20A030), 7).done()
    return paint


def koopa_eyes(closed):
    def paint(w, h):
        p = Pic(w, h, WHITE)
        if closed:
            p.rect(0, 0, 32, 32, rgba(0xF0D060))
            p.line([(6, 18), (26, 18)], BLACK, 1.6)
        else:
            p.ellipse(16, 17, 6, 9, BLACK)
            p.ellipse(14, 13, 2, 2.5, WHITE)
        return p.done()
    return paint


def koopa_eye_border(w, h):
    return Pic(w, h, rgba(0x101010)).ellipse(16, 16, 13, 13, WHITE).done()


def koopa_nostrils(w, h):
    p = Pic(w, h, WHITE)
    p.ellipse(26, 16, 2.5, 3.5, rgba(0x404000)).ellipse(38, 16, 2.5, 3.5, rgba(0x404000))
    return p.done()


def koopa_shoe(w, h):
    img = shaded_ball(w, h, 16, 16, 22, rgba(0xF08020), spec=0.4)
    img[..., 3] = 255
    return img


def whomp(kind):
    """The Whomp's stone: grey blocks; his face (angry), his back (a plaster), his hands."""
    def paint(w, h):
        n = noise(w, h, 4, 50 + len(kind))
        img = solid(w, h, (0, 0, 0))
        v = 170 + 40 * n
        img[..., 0] = v
        img[..., 1] = v
        img[..., 2] = v + 6
        p = Pic(w, h)
        if kind == "surface":
            p.line([(0, 16), (32, 16)], rgba(0x707078), 1.0).line([(16, 0), (16, 16)], rgba(0x707078), 1.0)
            p.line([(6, 16), (6, 32)], rgba(0x707078), 1.0).line([(26, 16), (26, 32)], rgba(0x707078), 1.0)
        elif kind == "face":
            for cx in (9, 23):
                p.ellipse(cx, 22, 5, 6, WHITE).ellipse(cx, 23, 2.3, 3.5, BLACK)
            p.poly([(2, 12), (14, 16), (14, 18.5), (2, 15)], rgba(0x404048))
            p.poly([(30, 12), (18, 16), (18, 18.5), (30, 15)], rgba(0x404048))
            p.poly([(7, 40), (25, 40), (22, 47), (10, 47)], rgba(0x302830))
            p.poly([(10, 47), (12, 42), (14, 47)], WHITE).poly([(18, 47), (20, 42), (22, 47)], WHITE)
        elif kind == "back":
            p.rect(8, 20, 24, 30, rgba(0xF0D8B0)).rect(4, 23, 28, 27, rgba(0xF0D8B0))
        else:
            p.ellipse(16, 16, 14, 14, rgba(0xA0A0A8))
        return over(img, p.done())
    return paint


def king_bobomb_eyes(w, h):
    p = Pic(w, h)
    for cx in (8, 24):
        p.ellipse(cx, 16, 6, 12, WHITE)
        p.ellipse(cx, 18, 2.6, 6, BLACK)
    return p.done()


def crown_rim(w, h):
    p = Pic(w, h, rgba(0xF0C020))
    p.rect(0, 0, w, 3, rgba(0xFFF0A0)).rect(0, h - 3, w, h, rgba(0xA07010))
    for y in (8, 16, 24):
        p.ellipse(8, y, 2.5, 2.5, rgba(0xE02020))
    return p.done()


def glove(w, h):
    img = shaded_ball(w, h, 16, 16, 20, rgba(0xF4F4F4), spec=0.2)
    img[..., 3] = 255
    return img


def dark_metal(w, h):
    return solid(w, h, (40, 40, 48))


def piranha(kind):
    def paint(w, h):
        if kind == "skin":
            p = Pic(w, h, rgba(0xE02020))
            for x, y, r in ((8, 8, 4), (24, 12, 3.5), (12, 24, 3.5), (27, 27, 3), (2, 20, 2.5)):
                p.ellipse(x, y, r, r, WHITE)
            return p.done()
        if kind == "stem":
            img = pipe_side(w, h)
            img[..., 0] = img[..., 0] // 2
            return img
        if kind == "lip":
            return Pic(w, h, rgba(0xF8E8C8)).done()
        if kind == "tooth":
            return teeth(w, h)
        if kind == "tongue":
            return tongue(w, h)
        p = Pic(w, h)   # the leaf
        p.poly([(16, 1), (29, 22), (24, 50), (16, 63), (8, 50), (3, 22)], rgba(0x30A020))
        p.line([(16, 3), (16, 61)], rgba(0x186010), 1.2)
        for y in (16, 28, 40):
            p.line([(16, y), (24, y - 6)], rgba(0x186010), 0.8).line([(16, y), (8, y - 6)], rgba(0x186010), 0.8)
        return p.done()
    return paint


def chuckya(kind):
    def paint(w, h):
        if kind == "eyes":
            p = Pic(w, h, rgba(0xA040C0))
            p.ellipse(9, 16, 5, 7, WHITE).ellipse(23, 16, 5, 7, WHITE)
            p.ellipse(10, 17, 2, 3.5, BLACK).ellipse(22, 17, 2, 3.5, BLACK)
            return p.done()
        if kind == "hand":
            return Pic(w, h, rgba(0xF0B030)).ellipse(16, 16, 10, 10, rgba(0xFFD860)).done()
        img = shaded_ball(w, h, w / 2, h / 2, max(w, h) * 0.7, rgba(0xA040C0), spec=0.3)
        img[..., 3] = 255
        return img
    return paint


def amp(kind):
    def paint(w, h):
        if kind == "body":
            return chomp_shine(True)(w, h)
        if kind == "eyes":
            p = Pic(w, h, rgba(0x202020))
            p.ellipse(9, 16, 6, 8, WHITE).ellipse(23, 16, 6, 8, WHITE)
            p.ellipse(9, 17, 2.5, 4, BLACK).ellipse(23, 17, 2.5, 4, BLACK)
            return p.done()
        if kind == "mouth":
            p = Pic(w, h, rgba(0x202020))
            for i in range(5):
                p.poly([(3 + i * 5.5, 10), (8.5 + i * 5.5, 10), (5.75 + i * 5.5, 22)], WHITE)
            return p.done()
        p = Pic(w, h)   # the electricity
        p.line([(8, 0), (4, 8), (11, 14), (5, 22), (12, 28), (8, 32)], rgba(0xC0E0FF), 2.2)
        p.line([(8, 0), (4, 8), (11, 14), (5, 22), (12, 28), (8, 32)], WHITE, 0.9)
        return p.done()
    return paint


def boo(kind, bbh=False):
    def paint(w, h):
        p = Pic(w, h)
        if kind == "eyes":
            for cx in (16, 48):
                p.ellipse(cx, 16, 7, 12, BLACK)
                p.ellipse(cx, 12, 2, 3, rgba(0xE02040) if bbh else WHITE)
        else:
            p.ellipse(16, 18, 13, 11, rgba(0x600010))
            p.ellipse(16, 24, 7, 4, rgba(0xF04060))
            p.poly([(6, 10), (10, 10), (8, 15)], WHITE).poly([(22, 10), (26, 10), (24, 15)], WHITE)
        return p.done()
    return paint


def flyguy(kind):
    def paint(w, h):
        if kind == "face":
            p = Pic(w, h, rgba(0xF0F0F0))
            p.ellipse(11, 12, 3, 4.5, BLACK).ellipse(21, 12, 3, 4.5, BLACK)
            p.ellipse(16, 23, 3, 3.5, BLACK)
            return p.done()
        xs, ys = grid(w, h)
        if kind == "cloth":
            v = 210 + 40 * np.sin(xs / w * 2 * math.pi * 3 + ys / h)
            out = solid(w, h, (0, 0, 0))
            out[..., 0] = np.clip(v, 0, 255)
            out[..., 1] = np.clip(v * 0.12, 0, 255)
            out[..., 2] = np.clip(v * 0.12, 0, 255)
            return out
        r = np.hypot(xs - 16, ys - 16) / 15   # the propeller's blur
        a = np.clip(1 - r, 0, 1) * (0.6 + 0.4 * np.cos(np.arctan2(ys - 16, xs - 16) * 2))
        out = np.zeros((h, w, 4), np.uint8)
        out[..., :3] = 230
        out[..., 3] = (a * 255).astype(np.uint8)
        return out
    return paint


def cannon(kind):
    def paint(w, h):
        if kind == "lid":
            n = noise(w, h, 4, 61)
            img = solid(w, h, (0, 0, 0))
            img[..., 0] = 200 + 30 * n
            img[..., 1] = 170 + 30 * n
            img[..., 2] = 150 + 30 * n
            return img
        return chomp_shine(kind == "barrel")(w, h)
    return paint


def frame(p, w, h, col, t=1.5):
    p.rect(0, 0, w, t, col).rect(0, h - t, w, h, col).rect(0, 0, t, h, col).rect(w - t, 0, w, h, col)


def cork_box(w, h):
    n = noise(w, h, 2, 70)
    s = np.random.default_rng(71).random((h, w))
    img = solid(w, h, (0, 0, 0))
    img[..., 0] = np.clip(190 + 40 * n - 50 * (s > 0.85), 0, 255)
    img[..., 1] = np.clip(140 + 30 * n - 40 * (s > 0.85), 0, 255)
    img[..., 2] = np.clip(80 + 20 * n - 30 * (s > 0.85), 0, 255)
    p = Pic(w, h)
    frame(p, w, h, rgba(0x6A4018))
    return over(img, p.done())


def crazy_box(w, h):
    p = Pic(w, h, rgba(0xC08040))
    p.rect(2, 2, 30, 30, rgba(0xE0A050))
    p.ellipse(16, 16, 9, 9, rgba(0xF06030))
    p.ellipse(16, 16, 5, 5, rgba(0xFFE040))
    return p.done()


def metal_box(w, h):
    img = door("metal")(w, h)
    p = Pic(w, h)
    p.poly([(13, 10), (19, 10), (18, 38), (14, 38)], rgba(0xD8D8E0))
    p.ellipse(16, 46, 3, 3, rgba(0xD8D8E0))
    return over(img, p.done())


def checker(side):
    def paint(w, h):
        xs, ys = grid(w, h)
        c = ((xs // 8 + ys // 8) % 2).astype(bool) if not side else ((xs // 8) % 2).astype(bool)
        img = solid(w, h, (0, 0, 0))
        img[..., 0] = np.where(c, 240, 140)
        img[..., 1] = np.where(c, 200, 90)
        img[..., 2] = np.where(c, 60, 30)
        return img
    return paint


def switch(color, kind):
    def paint(w, h):
        if kind == "base":
            return solid(w, h, (120, 120, 130))
        if kind == "mark":
            p = Pic(w, h, rgba(color))
            p.poly([(5, 4), (11, 4), (10, 20), (6, 20)], WHITE)
            p.ellipse(8, 25, 2.5, 2.5, WHITE)
            return p.done()
        img = solid(w, h, rgba(color))
        xs, ys = grid(w, h)
        img[..., :3] = np.clip(img[..., :3] * (0.8 + 0.3 * (1 - ys / h))[..., None], 0, 255)
        return img
    return paint


def pole(top):
    def paint(w, h):
        if top:
            xs, ys = grid(w, h)
            v = 0.8 + 0.2 * np.sin(np.hypot(xs - 16, ys - 16) * 1.6)
            img = solid(w, h, (0, 0, 0))
            img[..., 0] = np.clip(200 * v, 0, 255)
            img[..., 1] = np.clip(150 * v, 0, 255)
            img[..., 2] = np.clip(90 * v, 0, 255)
            return img
        return wood_grain(w, h, rgba(0xB07840), rgba(0x6A4018), 81)
    return paint


def koopa_flag(w, h):
    p = Pic(w, h, rgba(0xE02020))
    p.ellipse(16, 16, 9, 9, WHITE)
    p.ellipse(16, 16, 6, 6, rgba(0x20A030))
    return p.done()


def butterfly_wing(w, h):
    p = Pic(w, h)
    p.ellipse(16, 18, 14, 16, rgba(0xF0F0F0))
    p.ellipse(16, 46, 11, 13, rgba(0xF0F0F0))
    p.ellipse(16, 18, 6, 7, rgba(0xF0C020)).ellipse(16, 46, 4, 5, rgba(0xF0C020))
    return p.done()


def blue_fish(w, h):
    p = Pic(w, h)
    p.ellipse(14, 16, 11, 7, rgba(0x2060E0))
    p.poly([(23, 16), (31, 9), (31, 23)], rgba(0x2060E0))
    p.ellipse(8, 14, 2, 2, WHITE).ellipse(8, 14, 1, 1, BLACK)
    return p.done()


def mips_eyes(w, h):
    p = Pic(w, h, rgba(0xF0E060))
    p.ellipse(10, 16, 4, 6, BLACK).ellipse(22, 16, 4, 6, BLACK)
    p.ellipse(9, 13.5, 1.3, 1.7, WHITE).ellipse(21, 13.5, 1.3, 1.7, WHITE)
    return p.done()


def yoshi(kind):
    def paint(w, h):
        p = Pic(w, h, WHITE)
        if kind == "eye":
            p.ellipse(8.5, 9, 3, 4.5, BLACK).ellipse(7.5, 7.5, 1, 1.3, WHITE)
        elif kind == "blink":
            p.line([(4, 9), (13, 9)], BLACK, 1.4)
        else:
            p.ellipse(8, 8, 1.5, 2, rgba(0x206010))
        return p.done()
    return paint


def bbh(kind):
    """Big Boo's Haunt's furniture: chairs, the mad piano, the books, the cage."""
    def paint(w, h):
        if kind in ("chair_bottom", "chair_leg", "chair_front", "cage_wooden_base"):
            return wood_grain(w, h, rgba(0x8A4A20), rgba(0x4A2008), 90 + len(kind))
        if kind == "piano_body":
            return chomp_shine(False)(w, h)
        if kind in ("piano_keys", "piano_keys_corner", "piano_keys_edge"):
            p = Pic(w, h, WHITE)
            for x in range(0, w, 4):
                p.line([(x, 0), (x, h)], rgba(0x404040), 0.6)
            if kind == "piano_keys":
                for x in range(2, w, 8):
                    p.rect(x, 0, x + 2.5, h * 0.6, BLACK)
            return p.done()
        if kind in ("piano_mouth", "bookend_mouth"):
            return Pic(w, h, rgba(0x501020)).done()
        if kind in ("piano_tooth", "bookend_tooth"):
            return teeth(w, h)
        if kind in ("bookend_cover", "book_cover"):
            p = Pic(w, h, rgba(0xB02020))
            p.rect(3, 3, w - 3, h - 3, rgba(0xC83030))
            p.rect(6, 8, w - 6, 10, rgba(0xF0C040))
            return p.done()
        if kind == "bookend_pages":
            p = Pic(w, h, rgba(0xF4F0E0))
            for y in range(2, h, 3):
                p.line([(0, y), (w, y)], rgba(0xC8C0A8), 0.5)
            return p.done()
        if kind == "bookend_spine":
            p = Pic(w, h, rgba(0x902020))
            p.rect(0, 6, w, 8, rgba(0xF0C040)).rect(0, h - 8, w, h - 6, rgba(0xF0C040))
            return p.done()
        if kind == "cage_floor":
            return wood_grain(w, h, rgba(0x6A3A18), rgba(0x3A1A08), 97, vertical=False)
        if kind == "cage_bars":
            p = Pic(w, h)
            for x in (4, 12, 20, 28):
                p.rect(x - 1.2, 0, x + 1.2, h, rgba(0x404048))
            return p.done()
        p = Pic(w, h)   # the cage's ornaments
        p.ring(w / 2, h / 2, w / 2 - 2, h / 2 - 2, rgba(0x505058), 2)
        return p.done()
    return paint


ACTORS.update({
    "peach/peach_eye_open.rgba16.png": peach_eye(0),
    "peach/peach_eye_mostly_open.rgba16.png": peach_eye(1),
    "peach/peach_eye_mostly_closed.rgba16.png": peach_eye(2),
    "peach/peach_eye_closed.rgba16.png": peach_eye(3),
    "peach/peach_lips.rgba16.png": peach_lips(False),
    "peach/peach_lips_scrunched.rgba16.png": peach_lips(True),
    "peach/peach_dress.rgba16.png": peach_dress,
    "peach/peach_crown_jewel.rgba16.png": jewel(rgba(0xE02040)),
    "peach/peach_chest_jewel.rgba16.png": jewel(rgba(0x3060F0)),
    "peach/peach_nostril.rgba16.png": nostril,
    "toad/toad_face.rgba16.png": toad_face,
    "toad/toad_head.rgba16.png": toad_head,
    "lakitu_cameraman/lakitu_cameraman_eyes_open.rgba16.png": lakitu_eyes(False),
    "lakitu_cameraman/lakitu_cameraman_eyes_closed.rgba16.png": lakitu_eyes(True),
    "lakitu_cameraman/lakitu_cameraman_shell.rgba16.png": lakitu_shell,
    "lakitu_cameraman/lakitu_cameraman_frown.rgba16.png": lakitu_frown,
    "lakitu_cameraman/lakitu_camera_lens.rgba16.png": camera_lens,
    "koopa/koopa_shell_front.rgba16.png": koopa_shell(True),
    "koopa/koopa_shell_front_top.rgba16.png": koopa_shell(True, True),
    "koopa/koopa_shell_back.rgba16.png": koopa_shell(False),
    "koopa_shell/koopa_shell_front.rgba16.png": koopa_shell(True),
    "koopa_shell/koopa_shell_back.rgba16.png": koopa_shell(False),
    "koopa/koopa_eyes_open.rgba16.png": koopa_eyes(False),
    "koopa/koopa_eyes_closed.rgba16.png": koopa_eyes(True),
    "koopa/koopa_eye_border.rgba16.png": koopa_eye_border,
    "koopa/koopa_nostrils.rgba16.png": koopa_nostrils,
    "koopa/koopa_shoe.rgba16.png": koopa_shoe,
    "whomp/whomp_surface.rgba16.png": whomp("surface"),
    "whomp/whomp_face.rgba16.png": whomp("face"),
    "whomp/whomp_back.rgba16.png": whomp("back"),
    "whomp/whomp_hand.rgba16.png": whomp("hand"),
    "king_bobomb/king_bob-omb_left_side.rgba16.png": bobomb_side(False, rgba(0x202028)),
    "king_bobomb/king_bob-omb_right_side.rgba16.png": bobomb_side(True, rgba(0x202028)),
    "king_bobomb/king_bob-omb_eyes.rgba16.png": king_bobomb_eyes,
    "king_bobomb/king_bob-omb_crown_rim.rgba16.png": crown_rim,
    "king_bobomb/king_bob-omb_arm.rgba16.png": dark_metal,
    "king_bobomb/king_bob-omb_hand.rgba16.png": glove,
    "piranha_plant/piranha_plant_skin.rgba16.png": piranha("skin"),
    "piranha_plant/piranha_plant_stem.rgba16.png": piranha("stem"),
    "piranha_plant/piranha_plant_bottom_lip.rgba16.png": piranha("lip"),
    "piranha_plant/piranha_plant_tooth.rgba16.png": piranha("tooth"),
    "piranha_plant/piranha_plant_tongue.rgba16.png": piranha("tongue"),
    "piranha_plant/piranha_plant_leaf.rgba16.png": piranha("leaf"),
    "chuckya/chuckya_eyes.rgba16.png": chuckya("eyes"),
    "chuckya/chuckya_hand_antenna.rgba16.png": chuckya("hand"),
    "chuckya/chuckya_body_arm_left_side.rgba16.png": chuckya("body"),
    "chuckya/chuckya_body_arm_right_side.rgba16.png": chuckya("body"),
    "amp/amp_body.rgba16.png": amp("body"),
    "amp/amp_eyes.rgba16.png": amp("eyes"),
    "amp/amp_mouth.rgba16.png": amp("mouth"),
    "amp/amp_electricity.rgba16.png": amp("electricity"),
    "boo/boo_eyes.rgba16.png": boo("eyes"),
    "boo/boo_mouth.rgba16.png": boo("mouth"),
    "boo_castle/bbh_boo_eyes.rgba16.png": boo("eyes", True),
    "boo_castle/bbh_boo_mouth.rgba16.png": boo("mouth", True),
    "flyguy/flyguy_face.rgba16.png": flyguy("face"),
    "flyguy/flyguy_cloth_wrinkle.rgba16.png": flyguy("cloth"),
    "flyguy/flyguy_propeller.ia16.png": flyguy("propeller"),
    "cannon_lid/cannon_lid.rgba16.png": cannon("lid"),
    "cannon_barrel/cannon_barrel.rgba16.png": cannon("barrel"),
    "cannon_base/cannon_base.rgba16.png": cannon("base"),
    "breakable_box/cork_box_surface.rgba16.png": cork_box,
    "breakable_box/crazy_box_surface.rgba16.png": crazy_box,
    "metal_box/metal_box_side.rgba16.png": metal_box,
    "checkerboard_platform/checkerboard_platform.rgba16.png": checker(False),
    "checkerboard_platform/checkerboard_platform_side.rgba16.png": checker(True),
    "purple_switch/purple_switch_base.rgba16.png": switch(0x9030C0, "base"),
    "purple_switch/purple_switch_exclamation_point.rgba16.png": switch(0x9030C0, "mark"),
    "blue_coin_switch/blue_coin_switch_side.rgba16.png": switch(0x2050E0, "side"),
    "blue_coin_switch/blue_coin_switch_top.rgba16.png": switch(0x2050E0, "side"),
    "poundable_pole/poundable_pole_top.rgba16.png": pole(True),
    "poundable_pole/poundable_pole_side.rgba16.png": pole(False),
    "koopa_flag/koopa_flag_banner.rgba16.png": koopa_flag,
    "butterfly/butterfly_wing.rgba16.png": butterfly_wing,
    "blue_fish/blue_fish.rgba16.png": blue_fish,
    "mips/mips_eyes.rgba16.png": mips_eyes,
    "yoshi/yoshi_eye.rgba16.png": yoshi("eye"),
    "yoshi/yoshi_eye_blink.rgba16.png": yoshi("blink"),
    "yoshi/yoshi_nostril.rgba16.png": yoshi("nostril"),
    "chair/chair_bottom.rgba16.png": bbh("chair_bottom"),
    "chair/chair_leg.rgba16.png": bbh("chair_leg"),
    "chair/chair_front.rgba16.png": bbh("chair_front"),
    "mad_piano/mad_piano_body.rgba16.png": bbh("piano_body"),
    "mad_piano/mad_piano_keys.rgba16.png": bbh("piano_keys"),
    "mad_piano/mad_piano_keys_corner.rgba16.png": bbh("piano_keys_corner"),
    "mad_piano/mad_piano_keys_edge.rgba16.png": bbh("piano_keys_edge"),
    "mad_piano/mad_piano_mouth.rgba16.png": bbh("piano_mouth"),
    "mad_piano/mad_piano_tooth.rgba16.png": bbh("piano_tooth"),
    "bookend/bookend_cover.rgba16.png": bbh("bookend_cover"),
    "bookend/bookend_mouth.rgba16.png": bbh("bookend_mouth"),
    "bookend/bookend_pages.rgba16.png": bbh("bookend_pages"),
    "bookend/bookend_spine.rgba16.png": bbh("bookend_spine"),
    "bookend/bookend_tooth.rgba16.png": bbh("bookend_tooth"),
    "book/book_cover.rgba16.png": bbh("book_cover"),
    "haunted_cage/bbh_cage_floor.rgba16.png": bbh("cage_floor"),
    "haunted_cage/bbh_cage_bars.rgba16.png": bbh("cage_bars"),
    "haunted_cage/bbh_cage_ornament.rgba16.png": bbh("cage_ornament"),
    "haunted_cage/bbh_cage_double_ornament.rgba16.png": bbh("cage_ornament"),
    "haunted_cage/bbh_cage_wooden_base.rgba16.png": bbh("cage_wooden_base"),
})
