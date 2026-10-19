"""The castle's own textures, painted after the game's: its walls, roofs and windows, Peach's stained-glass window
over the door, the lobby's red carpet and the sunlight on its floor, and the four paintings that lead into the
courses (Bob-omb Battlefield, Whomp's Fortress, Jolly Roger Bay and Cool, Cool Mountain). The decompilation has no
pictures (they are in the game's ROM): these are drawn in code, like tools/art.py's actors.

TABLE maps a texture's path in the decompilation to its painter (tools/textures.py looks them up)."""
import math

import numpy as np

from art import Pic, rgba, noise, grid, star


def _rgb(h):
    return np.array([(h >> 16) & 255, (h >> 8) & 255, h & 255], np.float32)


def _out(img, alpha=None):
    h, w = img.shape[:2]
    o = np.zeros((h, w, 4), np.uint8)
    o[..., :3] = np.clip(img, 0, 255)
    o[..., 3] = 255 if alpha is None else alpha
    return o


# ------------------------------------------------------------------------------------------------ the outside
def castle_wall(w, h):
    """The castle's walls: big cream stone blocks, two rows of them, with grey mortar and a little shading."""
    xs, ys = grid(w, h)
    rows = 2
    rh = h / rows
    row = (ys // rh).astype(int)
    bw = w / 2
    off = (row % 2) * bw / 2
    n = noise(w, h, 4, 71) * 0.6 + noise(w, h, 2, 72) * 0.4
    base, dark = _rgb(0xF0E2B8), _rgb(0xC8B488)
    img = base[None, None] * (1 - n[..., None] * 0.18)
    # each block lit from above: lighter at its top, darker down to its bottom edge
    fy = (ys % rh) / rh
    img *= (1.06 - 0.12 * fy)[..., None]
    mortar = ((ys % rh) < 1.0) | (((xs + off) % bw) < 1.0)
    img[mortar] = dark * 0.92
    return _out(img)


def castle_roof(w, h):
    """The roofs: dark red tiles in rows, each row a little shadowed at its bottom."""
    xs, ys = grid(w, h)
    rh = h / 8
    fy = (ys % rh) / rh
    row = (ys // rh).astype(int)
    tile = ((xs + (row % 2) * 2) % 4) < 0.9
    base = _rgb(0xB02018)
    n = noise(w, h, 4, 73)
    img = base[None, None] * (1.12 - 0.35 * fy[..., None] - 0.12 * n[..., None])
    img[tile] *= 0.75
    return _out(img)


def castle_window(w, h):
    """A window: a dark blue pane under an arch, a cream stone frame round it, a cross of lead."""
    p = Pic(w, h, rgba(0xE8D8B0))
    p.rect(0, 0, w, h, rgba(0xE8D8B0))
    m = w * 0.18
    p.rect(m, h * 0.35, w - m, h - m, rgba(0x203060))
    p.ellipse(w / 2, h * 0.38, w / 2 - m, h * 0.3, rgba(0x203060))
    p.rect(w / 2 - 0.6, h * 0.12, w / 2 + 0.6, h - m, rgba(0x90805A))
    p.rect(m, h * 0.6, w - m, h * 0.6 + 1.2, rgba(0x90805A))
    p.rect(m + 1, h * 0.4, w / 2 - 1.5, h * 0.55, rgba(0x4060A0))
    return p.done()


def castle_bridge(w, h):
    """The bridge and the stone round the castle: light grey flagstones."""
    xs, ys = grid(w, h)
    n = noise(w, h, 4, 75)
    base = _rgb(0xC0C0B8)
    img = base[None, None] * (1 - 0.15 * n[..., None])
    sz = w / 2
    row = (ys // sz).astype(int)
    edge = ((ys % sz) < 1) | (((xs + (row % 2) * sz / 2) % sz) < 1)
    img[edge] = _rgb(0x808078)
    return _out(img)


def castle_path(w, h):
    """The paths: pale sandy ground, a few pebbles."""
    n = noise(w, h, 6, 77) * 0.6 + noise(w, h, 3, 78) * 0.4
    img = _rgb(0xD8B878)[None, None] * (1 - 0.18 * n[..., None])
    s = np.random.default_rng(79).random((h, w))
    img[s > 0.94] = _rgb(0xF0D8A0)
    img[s < 0.04] = _rgb(0x9A7A48)
    return _out(img)


def paving(w, h):
    """The courtyard's paving and the ground at the castle's door: big pale stone slabs."""
    xs, ys = grid(w, h)
    n = noise(w, h, 4, 87)
    img = _rgb(0xD8D0B8)[None, None] * (1 - 0.14 * n[..., None])
    sz = w / 2
    edge = ((ys % sz) < 1) | ((xs % sz) < 1)
    img[edge] = _rgb(0x989078)
    return _out(img)


def lawn(w, h):
    """The castle's lawns: bright green, darker and lighter blades."""
    xs, ys = grid(w, h)
    n = noise(w, h, 8, 89) * 0.6 + noise(w, h, 4, 90) * 0.4
    img = _rgb(0x40B030)[None, None] * (1.08 - 0.3 * n[..., None])
    s = np.random.default_rng(91).random((h, w))
    img[((xs.astype(int) + ys.astype(int) * 2) % 7 == 0) & (s > 0.5)] = _rgb(0x287818)
    img[((xs.astype(int) * 3 + ys.astype(int)) % 11 == 0) & (s < 0.3)] = _rgb(0x78D850)
    return _out(img)


def castle_trim(w, h):
    """The stone at the foot of the castle's walls: grey blocks."""
    xs, ys = grid(w, h)
    n = noise(w, h, 4, 81)
    img = _rgb(0xA8A098)[None, None] * (1 - 0.15 * n[..., None])
    edge = ((ys % (h / 2)) < 1) | (((xs + ((ys // (h / 2)) % 2) * w / 4) % (w / 2)) < 1)
    img[edge] = _rgb(0x686058)
    return _out(img)


def tree_shadow(w, h):
    """The trees' shadows, laid on the grass (intensity and alpha): a soft dark disc."""
    xs, ys = grid(w, h)
    d = np.sqrt(((xs - w / 2) / (w / 2)) ** 2 + ((ys - h / 2) / (h / 2)) ** 2)
    o = np.zeros((h, w, 4), np.uint8)
    o[..., 3] = (np.clip((1 - d) * 2.5, 0, 1) * 150).astype(np.uint8)
    return o


def stained_glass(top):
    """Peach in the stained-glass window over the castle's door: her crowned head and golden hair (the top half),
    her pink dress (the bottom), on blue glass with dark lead between the pieces."""
    def paint(w, h):
        p = Pic(w, h, rgba(0x2850B0))
        lead = rgba(0x202028)
        # (the glass behind her: a few lighter panes)
        for i, x in enumerate(range(0, w, 8)):
            p.rect(x, 0, x + 8, h, rgba(0x3060C8 if i % 2 else 0x2850B0))
            p.rect(x, 0, x + 0.8, h, lead)
        cx = w / 2
        if top:
            # hair, face, crown with its jewels
            p.ellipse(cx, h * 0.95, w * 0.2, h * 0.75, rgba(0xF8D040))
            p.ellipse(cx, h * 0.78, w * 0.11, h * 0.36, rgba(0xF8D8B8))
            p.ellipse(cx - w * 0.04, h * 0.74, 1.2, 1.6, rgba(0x2050C0))
            p.ellipse(cx + w * 0.04, h * 0.74, 1.2, 1.6, rgba(0x2050C0))
            p.rect(cx - w * 0.03, h * 0.92, cx + w * 0.03, h * 0.97, rgba(0xE06070))
            p.poly([(cx - w * 0.1, h * 0.42), (cx - w * 0.1, h * 0.2), (cx - w * 0.05, h * 0.32), (cx, h * 0.12),
                    (cx + w * 0.05, h * 0.32), (cx + w * 0.1, h * 0.2), (cx + w * 0.1, h * 0.42)], rgba(0xF8C820))
            p.ellipse(cx, h * 0.34, 1.4, 1.4, rgba(0xE02040))
            p.line([(cx - w * 0.2, h), (cx - w * 0.2, h * 0.6), (cx, h * 0.38), (cx + w * 0.2, h * 0.6),
                    (cx + w * 0.2, h)], lead, 0.8)
        else:
            # the dress, wider down to the sill, a brooch, her arms in white gloves
            p.poly([(cx - w * 0.12, 0), (cx + w * 0.12, 0), (cx + w * 0.3, h), (cx - w * 0.3, h)], rgba(0xF080B0))
            p.poly([(cx - w * 0.3, h), (cx - w * 0.22, h * 0.7), (cx + w * 0.22, h * 0.7), (cx + w * 0.3, h)],
                   rgba(0xD05090))
            p.ellipse(cx, h * 0.12, 2, 2, rgba(0x40A0F0))
            p.ellipse(cx - w * 0.14, h * 0.35, 2.4, 2.4, rgba(0xFFFFFF))
            p.ellipse(cx + w * 0.14, h * 0.35, 2.4, 2.4, rgba(0xFFFFFF))
            for y in (h * 0.45, h * 0.7):
                p.line([(cx - w * 0.3, y), (cx + w * 0.3, y)], lead, 0.6)
        p.rect(0, 0, w, 1, lead)
        p.rect(0, h - 1, w, h, lead)
        return p.done()
    return paint


# ------------------------------------------------------------------------------------------------ the inside
def red_carpet(w, h):
    """The lobby's red carpet, up the stairs: deep red, a gold band near each edge."""
    xs, ys = grid(w, h)
    n = noise(w, h, 3, 85)
    img = _rgb(0xB01818)[None, None] * (1 - 0.15 * n[..., None])
    band = ((ys > h * 0.06) & (ys < h * 0.16)) | ((ys > h * 0.84) & (ys < h * 0.94))
    img[band] = _rgb(0xE8B830)
    return _out(img)


def sunlight(w, h):
    """The light through the lobby's window, on the floor (intensity and alpha): bright in the middle."""
    xs, ys = grid(w, h)
    d = np.sqrt(((xs - w / 2) / (w / 2)) ** 2 + ((ys - h / 2) / (h / 2)) ** 2)
    a = np.clip(1 - d, 0, 1) ** 0.8 * 200
    o = np.zeros((h, w, 4), np.uint8)
    o[..., :3] = 255
    o[..., 3] = a.astype(np.uint8)
    return o


# ------------------------------------------------------------------------------------------------ the paintings
# each is two halves of 64 x 32, the top and the bottom of one picture (64 x 64): painted whole, then cut
def _painting(draw):
    cache = {}

    def half(top):
        def paint(w, h):
            if (w, h) not in cache:
                p = Pic(w, h * 2)
                draw(p, w, h * 2)
                cache[(w, h)] = p.done()
            img = cache[(w, h)]
            return img[:h] if top else img[h:]
        return paint
    return half


def _frame(p, w, h):
    p.rect(0, 0, w, 2, rgba(0x8A5A20))
    p.rect(0, h - 2, w, h, rgba(0x8A5A20))
    p.rect(0, 0, 2, h, rgba(0x8A5A20))
    p.rect(w - 2, 0, w, h, rgba(0x8A5A20))


def _bob(p, w, h):
    """Bob-omb Battlefield: a Bob-omb on the green hills, the mountain behind, blue sky."""
    p.rect(0, 0, w, h, rgba(0x78B8F8))
    p.poly([(w * 0.35, h * 0.62), (w * 0.62, h * 0.18), (w * 0.9, h * 0.62)], rgba(0x8A6A48))
    p.poly([(w * 0.55, h * 0.3), (w * 0.62, h * 0.18), (w * 0.69, h * 0.3)], rgba(0x4CA838))
    p.ellipse(w * 0.2, h * 0.75, w * 0.45, h * 0.25, rgba(0x48B030))
    p.ellipse(w * 0.85, h * 0.8, w * 0.4, h * 0.22, rgba(0x3C9C28))
    p.rect(0, h * 0.78, w, h, rgba(0x48B030))
    # the Bob-omb: black body, white eyes, the fuse and its key
    cx, cy, r = w * 0.42, h * 0.62, w * 0.17
    p.ellipse(cx, cy, r, r, rgba(0x202028))
    p.ellipse(cx - r * 0.35, cy - r * 0.35, r * 0.25, r * 0.2, rgba(0x505060))
    p.ellipse(cx - r * 0.3, cy - r * 0.05, r * 0.16, r * 0.3, rgba(0xFFFFFF))
    p.ellipse(cx + r * 0.3, cy - r * 0.05, r * 0.16, r * 0.3, rgba(0xFFFFFF))
    p.ellipse(cx - r * 0.3, cy, r * 0.07, r * 0.15, rgba(0x000000))
    p.ellipse(cx + r * 0.3, cy, r * 0.07, r * 0.15, rgba(0x000000))
    p.rect(cx - r * 0.12, cy - r * 1.3, cx + r * 0.12, cy - r * 0.9, rgba(0xC8C8C8))
    p.line([(cx, cy - r * 1.3), (cx + r * 0.2, cy - r * 1.6)], rgba(0xF0E0A0), 1)
    p.ellipse(cx - r * 0.55, cy + r * 0.95, r * 0.35, r * 0.18, rgba(0xF0A818))
    p.ellipse(cx + r * 0.55, cy + r * 0.95, r * 0.35, r * 0.18, rgba(0xF0A818))
    p.rect(cx + r * 0.95, cy - r * 0.2, cx + r * 1.15, cy + r * 0.2, rgba(0xC8C8C8))
    p.ellipse(cx + r * 1.25, cy - r * 0.3, r * 0.12, r * 0.25, rgba(0xE0E0E0))
    p.ellipse(cx + r * 1.25, cy + r * 0.3, r * 0.12, r * 0.25, rgba(0xE0E0E0))
    _frame(p, w, h)


def _wf(p, w, h):
    """Whomp's Fortress: a Whomp's angry stone face before the fortress's tower, under a pale sky."""
    p.rect(0, 0, w, h, rgba(0xA8D0F0))
    p.rect(w * 0.62, h * 0.15, w * 0.82, h * 0.8, rgba(0xC0B090))
    for y in range(int(h * 0.15), int(h * 0.8), 6):
        p.rect(w * 0.62, y, w * 0.82, y + 0.8, rgba(0x8A7A5A))
    p.rect(w * 0.6, h * 0.1, w * 0.84, h * 0.17, rgba(0xA89878))
    p.rect(0, h * 0.8, w, h, rgba(0x58B038))
    # the Whomp: a grey slab, its brows, eyes and grim mouth
    x0, x1, y0, y1 = w * 0.1, w * 0.55, h * 0.22, h * 0.92
    p.rect(x0, y0, x1, y1, rgba(0x9898A0))
    p.rect(x0, y0, x1, y0 + 2, rgba(0xB8B8C0))
    p.rect(x1 - 2, y0, x1, y1, rgba(0x707078))
    ex = (x0 + x1) / 2
    p.ellipse(ex - w * 0.09, h * 0.45, w * 0.06, h * 0.07, rgba(0xFFFFFF))
    p.ellipse(ex + w * 0.09, h * 0.45, w * 0.06, h * 0.07, rgba(0xFFFFFF))
    p.ellipse(ex - w * 0.08, h * 0.47, w * 0.025, h * 0.04, rgba(0x000000))
    p.ellipse(ex + w * 0.08, h * 0.47, w * 0.025, h * 0.04, rgba(0x000000))
    p.poly([(ex - w * 0.17, h * 0.33), (ex - w * 0.02, h * 0.39), (ex - w * 0.02, h * 0.42),
            (ex - w * 0.17, h * 0.37)], rgba(0x404048))
    p.poly([(ex + w * 0.17, h * 0.33), (ex + w * 0.02, h * 0.39), (ex + w * 0.02, h * 0.42),
            (ex + w * 0.17, h * 0.37)], rgba(0x404048))
    p.rect(ex - w * 0.12, h * 0.66, ex + w * 0.12, h * 0.72, rgba(0x404048))
    _frame(p, w, h)


def _jrb(p, w, h):
    """Jolly Roger Bay: the sunken pirate ship in the blue deep, fish swimming by."""
    xs, ys = grid(w, h)
    p.rect(0, 0, w, h, rgba(0x2060B0))
    p.rect(0, 0, w, h * 0.3, rgba(0x3070C0))
    p.rect(0, h * 0.85, w, h, rgba(0xC8B880))
    # the ship, leaning on the sand: hull, mast, a torn sail
    p.poly([(w * 0.25, h * 0.65), (w * 0.85, h * 0.6), (w * 0.78, h * 0.82), (w * 0.32, h * 0.88)], rgba(0x6A4020))
    p.rect(w * 0.27, h * 0.66, w * 0.82, h * 0.68, rgba(0x8A5A30))
    p.line([(w * 0.55, h * 0.62), (w * 0.5, h * 0.2)], rgba(0x5A3418), 1.5)
    p.poly([(w * 0.51, h * 0.25), (w * 0.72, h * 0.3), (w * 0.68, h * 0.48), (w * 0.53, h * 0.45)], rgba(0xD8D0B0))
    p.ellipse(w * 0.6, h * 0.36, w * 0.04, h * 0.04, rgba(0x202020))
    # fish
    for fx, fy, c in ((0.18, 0.3, 0xF0A020), (0.82, 0.18, 0xF06040), (0.15, 0.55, 0x60D0F0)):
        x, y = w * fx, h * fy
        p.ellipse(x, y, w * 0.06, h * 0.035, rgba(c))
        p.poly([(x + w * 0.05, y), (x + w * 0.1, y - h * 0.03), (x + w * 0.1, y + h * 0.03)], rgba(c))
        p.ellipse(x - w * 0.03, y - h * 0.005, 0.8, 0.8, rgba(0x000000))
    for bx, by in ((0.3, 0.2), (0.33, 0.12), (0.7, 0.1)):
        p.ring(w * bx, h * by, 1.5, 1.5, rgba(0xC0E0FF), 0.5)
    _frame(p, w, h)


def _ccm(p, w, h):
    """Cool, Cool Mountain: the snowy peak and its snowman under a cold blue sky."""
    p.rect(0, 0, w, h, rgba(0x90B8F0))
    p.poly([(0, h * 0.75), (w * 0.45, h * 0.12), (w, h * 0.7), (w, h), (0, h)], rgba(0xF0F4FF))
    p.poly([(w * 0.45, h * 0.12), (w * 0.62, h * 0.38), (w * 0.4, h * 0.5)], rgba(0xC8D8F0))
    for tx in (0.12, 0.8, 0.9):
        x = w * tx
        p.poly([(x, h * 0.6), (x - w * 0.06, h * 0.78), (x + w * 0.06, h * 0.78)], rgba(0x2A7040))
        p.poly([(x, h * 0.58), (x - w * 0.03, h * 0.65), (x + w * 0.03, h * 0.65)], rgba(0xF0F4FF))
    # the snowman: two balls of snow, coal eyes, a carrot nose
    cx = w * 0.5
    p.ellipse(cx, h * 0.8, w * 0.15, h * 0.13, rgba(0xFFFFFF))
    p.ellipse(cx, h * 0.6, w * 0.1, h * 0.09, rgba(0xFFFFFF))
    p.ellipse(cx - w * 0.035, h * 0.58, 1.2, 1.2, rgba(0x202020))
    p.ellipse(cx + w * 0.035, h * 0.58, 1.2, 1.2, rgba(0x202020))
    p.poly([(cx, h * 0.61), (cx + w * 0.08, h * 0.63), (cx, h * 0.64)], rgba(0xF08020))
    for k in range(3):
        p.ellipse(w * (0.2 + 0.25 * k), h * (0.08 + 0.05 * k), 1, 1, rgba(0xFFFFFF))
    _frame(p, w, h)


_BOB, _WF, _JRB, _CCM = _painting(_bob), _painting(_wf), _painting(_jrb), _painting(_ccm)
OUT = "textures/outside/castle_grounds_textures."
IN = "textures/inside/inside_castle_textures."
CI = "levels/castle_inside/"
TABLE = {
    OUT + "01000.rgba16.png": castle_wall,
    OUT + "07800.rgba16.png": castle_roof,
    OUT + "0A800.rgba16.png": castle_window,
    OUT + "03800.rgba16.png": castle_bridge,
    OUT + "08000.rgba16.png": castle_path,
    OUT + "03000.rgba16.png": castle_trim,
    OUT + "0A000.rgba16.png": paving,
    OUT + "04000.rgba16.png": lawn,
    OUT + "0BC00.ia16.png": tree_shadow,
    "levels/castle_grounds/2.rgba16.png": stained_glass(True),
    "levels/castle_grounds/1.rgba16.png": stained_glass(False),
    IN + "01000.rgba16.png": red_carpet,
    CI + "castle_light.ia16.png": sunlight,
    # (the paintings: paintings.c's textures, the bottom half first)
    CI + "17.rgba16.png": _BOB(True), CI + "18.rgba16.png": _BOB(False),
    CI + "19.rgba16.png": _CCM(True), CI + "20.rgba16.png": _CCM(False),
    CI + "21.rgba16.png": _WF(True), CI + "22.rgba16.png": _WF(False),
    CI + "23_us.rgba16.png": _JRB(True), CI + "24_us.rgba16.png": _JRB(False),
}
