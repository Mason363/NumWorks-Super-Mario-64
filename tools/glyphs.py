"""The game's letters and screen pictures, drawn for the calculator (the ROM's are not in the decompilation):
the dialog font, the HUD's font and icons, the credits' font, the camera icons, the power meter, the screen
transitions' masks and the water boxes' textures. paint_ui(name, w, h) gives an RGBA image (numpy, h x w x 4)
for a texture symbol of bin/segment2.c or actors/power_meter."""
import math

import numpy as np

# ------------------------------------------------------------------------------------------------ the dialog font
# One pixel strokes, each letter a pixel or two narrower than the width the game gives it (gDialogCharWidths). Capitals and digits are 9 rows; small letters have 3 rows
# above their x-height and 2 below for descenders.
DIALOG = {
    "0": [".###.", "#...#", "#..##", "#.#.#", "#.#.#", "##..#", "#...#", "#...#", ".###."],
    "1": ["..#..", ".##..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."],
    "2": [".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#....", "#....", "#####"],
    "3": [".###.", "#...#", "....#", "....#", "..##.", "....#", "....#", "#...#", ".###."],
    "4": ["...#.", "..##.", ".#.#.", "#..#.", "#..#.", "#####", "...#.", "...#.", "...#."],
    "5": ["#####", "#....", "#....", "####.", "....#", "....#", "....#", "#...#", ".###."],
    "6": [".###.", "#...#", "#....", "#....", "####.", "#...#", "#...#", "#...#", ".###."],
    "7": ["#####", "....#", "...#.", "...#.", "..#..", "..#..", ".#...", ".#...", ".#..."],
    "8": [".###.", "#...#", "#...#", "#...#", ".###.", "#...#", "#...#", "#...#", ".###."],
    "9": [".###.", "#...#", "#...#", "#...#", ".####", "....#", "....#", "#...#", ".###."],
    "A": [".##.", "#..#", "#..#", "#..#", "####", "#..#", "#..#", "#..#", "#..#"],
    "B": ["###.", "#..#", "#..#", "#..#", "###.", "#..#", "#..#", "#..#", "###."],
    "C": [".##.", "#..#", "#...", "#...", "#...", "#...", "#...", "#..#", ".##."],
    "D": ["###.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "###."],
    "E": ["####", "#...", "#...", "#...", "###.", "#...", "#...", "#...", "####"],
    "F": ["####", "#...", "#...", "#...", "###.", "#...", "#...", "#...", "#..."],
    "G": [".##.", "#..#", "#...", "#...", "#.##", "#..#", "#..#", "#..#", ".###"],
    "H": ["#..#", "#..#", "#..#", "#..#", "####", "#..#", "#..#", "#..#", "#..#"],
    "I": ["###", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", "###"],
    "J": ["...#", "...#", "...#", "...#", "...#", "...#", "#..#", "#..#", ".##."],
    "K": ["#..#", "#..#", "#.#.", "#.#.", "##..", "#.#.", "#.#.", "#..#", "#..#"],
    "L": ["#..", "#..", "#..", "#..", "#..", "#..", "#..", "#..", "###"],
    "M": ["#....#", "##..##", "#.##.#", "#.##.#", "#....#", "#....#", "#....#", "#....#", "#....#"],
    "N": ["#....#", "##...#", "##...#", "#.#..#", "#.#..#", "#..#.#", "#...##", "#...##", "#....#"],
    "O": [".##.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", ".##."],
    "P": ["###.", "#..#", "#..#", "#..#", "###.", "#...", "#...", "#...", "#..."],
    "Q": [".##.", "#..#", "#..#", "#..#", "#..#", "#..#", "#.##", "#..#", ".##."],
    "R": ["###.", "#..#", "#..#", "#..#", "###.", "#.#.", "#..#", "#..#", "#..#"],
    "S": [".##.", "#..#", "#...", "#...", ".##.", "...#", "...#", "#..#", ".##."],
    "T": ["###", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", ".#."],
    "U": ["#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", ".##."],
    "V": ["#..#", "#..#", "#..#", "#..#", "#..#", "#..#", ".##.", ".##.", ".##."],
    "W": ["#....#", "#....#", "#....#", "#....#", "#.##.#", "#.##.#", "#.##.#", "##..##", "#....#"],
    "X": ["#...#", "#...#", ".#.#.", ".#.#.", "..#..", ".#.#.", ".#.#.", "#...#", "#...#"],
    "Y": ["#..#", "#..#", "#..#", ".##.", ".##.", ".##.", ".##.", ".##.", ".##."],
    "Z": ["####", "...#", "...#", "..#.", ".##.", ".#..", "#...", "#...", "####"],
    "a": ["....", "....", "....", ".##.", "...#", ".###", "#..#", "#..#", ".###"],
    "b": ["#..", "#..", "#..", "##.", "#.#", "#.#", "#.#", "#.#", "##."],
    "c": ["...", "...", "...", ".##", "#..", "#..", "#..", "#..", ".##"],
    "d": ["...#", "...#", "...#", ".###", "#..#", "#..#", "#..#", "#..#", ".###"],
    "e": ["...", "...", "...", ".#.", "#.#", "###", "#..", "#..", ".##"],
    "f": [".##", "#..", "#..", "###", "#..", "#..", "#..", "#..", "#.."],
    "g": ["....", "....", "....", ".###", "#..#", "#..#", "#..#", ".###", "...#", "#..#", ".##."],
    "h": ["#..", "#..", "#..", "##.", "#.#", "#.#", "#.#", "#.#", "#.#"],
    "i": [".", "#", ".", "#", "#", "#", "#", "#", "#"],
    "j": ["...", "..#", "...", "..#", "..#", "..#", "..#", "..#", "..#", "#.#", ".#."],
    "k": ["#..", "#..", "#..", "#.#", "#.#", "##.", "#.#", "#.#", "#.#"],
    "l": ["#", "#", "#", "#", "#", "#", "#", "#", "#"],
    "m": [".....", ".....", ".....", "####.", "#.#.#", "#.#.#", "#.#.#", "#.#.#", "#.#.#"],
    "n": ["...", "...", "...", "##.", "#.#", "#.#", "#.#", "#.#", "#.#"],
    "o": ["...", "...", "...", ".#.", "#.#", "#.#", "#.#", "#.#", ".#."],
    "p": ["...", "...", "...", "##.", "#.#", "#.#", "#.#", "##.", "#..", "#..", "#.."],
    "q": ["....", "....", "....", ".###", "#..#", "#..#", "#..#", ".###", "...#", "...#", "...#"],
    "r": ["...", "...", "...", "#.#", "##.", "#..", "#..", "#..", "#.."],
    "s": ["...", "...", "...", ".##", "#..", "#..", ".#.", "..#", "##."],
    "t": [".#.", ".#.", ".#.", "###", ".#.", ".#.", ".#.", ".#.", "..#"],
    "u": ["...", "...", "...", "#.#", "#.#", "#.#", "#.#", "#.#", ".##"],
    "v": ["...", "...", "...", "#.#", "#.#", "#.#", "#.#", ".#.", ".#."],
    "w": [".....", ".....", ".....", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."],
    "x": [".....", ".....", ".....", "#...#", ".#.#.", "..#..", "..#..", ".#.#.", "#...#"],
    "y": ["...", "...", "...", "#.#", "#.#", "#.#", "#.#", ".##", "..#", "..#", "##."],
    "z": ["...", "...", "...", "###", "..#", ".#.", ".#.", "#..", "###"],
    "apostrophe": ["#", "#", "."],
    "period": [".", ".", ".", ".", ".", ".", ".", "#", "#"],
    "comma": [".", ".", ".", ".", ".", ".", ".", "#", "#", "#"],
    "exclamation": ["#", "#", "#", "#", "#", "#", ".", "#", "#"],
    "question": [".###.", "#...#", "....#", "...#.", "..#..", "..#..", ".....", "..#..", "..#.."],
    "percent": ["##...", "##..#", "...#.", "..#..", "..#..", ".#...", "#..##", "...##", "....."],
    "ampersand": [".##..", "#..#.", "#..#.", ".##..", ".#.#.", "#...#", "#..#.", "#..#.", ".##.#"],
    # the US font's textures named slash and ellipsis hold the hyphen and the colon (charmap.txt's '-' and ':')
    "slash": [".....", ".....", ".....", ".....", "####.", ".....", ".....", ".....", "....."],
    "open_parentheses": [".#", "#.", "#.", "#.", "#.", "#.", "#.", "#.", ".#"],
    "close_parentheses": ["#.", ".#", ".#", ".#", ".#", ".#", ".#", ".#", "#."],
    "close_open_parentheses": ["#......#", ".#....#.", ".#....#.", ".#....#.", ".#....#.", ".#....#.",
                               ".#....#.", ".#....#.", "#......#"],
    "double_quote_open": [".#.#", "#.#.", "....", "....", "....", "....", "....", "....", "...."],
    "double_quote_close": [".#.#", "#.#.", "....", "....", "....", "....", "....", "....", "...."],
    "tilde": ["......", "......", "......", ".##..#", "#..##.", "......", "......", "......", "......"],
    "interpunct": [".", ".", ".", ".", "#", ".", ".", ".", "."],
    "ellipsis": [".", ".", "#", "#", ".", ".", "#", "#", "."],
    "multiply": [".....", ".....", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", ".....", "....."],
    "left_right_arrow": [".........", ".........", "..#...#..", ".#.....#.", "#########", ".#.....#.",
                         "..#...#..", ".........", "........."],
}

# the HUD's letters: a bold 6 x 7 font (two pixels a stroke), doubled to 12 x 14
HUD = {
    "0": [".####.", "##..##", "##.###", "###.##", "##..##", "##..##", ".####."],
    "1": ["..##..", ".###..", "..##..", "..##..", "..##..", "..##..", ".####."],
    "2": [".####.", "##..##", "....##", "..###.", ".##...", "##....", "######"],
    "3": [".####.", "##..##", "....##", "..###.", "....##", "##..##", ".####."],
    "4": ["...##.", "..###.", ".####.", "##.##.", "######", "...##.", "...##."],
    "5": ["######", "##....", "#####.", "....##", "....##", "##..##", ".####."],
    "6": [".####.", "##....", "#####.", "##..##", "##..##", "##..##", ".####."],
    "7": ["######", "....##", "...##.", "..##..", "..##..", ".##...", ".##..."],
    "8": [".####.", "##..##", "##..##", ".####.", "##..##", "##..##", ".####."],
    "9": [".####.", "##..##", "##..##", ".#####", "....##", "...##.", ".###.."],
    "A": ["..##..", ".####.", "##..##", "##..##", "######", "##..##", "##..##"],
    "B": ["#####.", "##..##", "##..##", "#####.", "##..##", "##..##", "#####."],
    "C": [".####.", "##..##", "##....", "##....", "##....", "##..##", ".####."],
    "D": ["####..", "##.##.", "##..##", "##..##", "##..##", "##.##.", "####.."],
    "E": ["######", "##....", "##....", "#####.", "##....", "##....", "######"],
    "F": ["######", "##....", "##....", "#####.", "##....", "##....", "##...."],
    "G": [".####.", "##..##", "##....", "##.###", "##..##", "##..##", ".#####"],
    "H": ["##..##", "##..##", "##..##", "######", "##..##", "##..##", "##..##"],
    "I": [".####.", "..##..", "..##..", "..##..", "..##..", "..##..", ".####."],
    "J": ["....##", "....##", "....##", "....##", "##..##", "##..##", ".####."],
    "K": ["##..##", "##.##.", "####..", "###...", "####..", "##.##.", "##..##"],
    "L": ["##....", "##....", "##....", "##....", "##....", "##....", "######"],
    "M": ["##...##", "###.###", "#######", "##.#.##", "##...##", "##...##", "##...##"],
    "N": ["##..##", "###.##", "######", "##.###", "##..##", "##..##", "##..##"],
    "O": [".####.", "##..##", "##..##", "##..##", "##..##", "##..##", ".####."],
    "P": ["#####.", "##..##", "##..##", "#####.", "##....", "##....", "##...."],
    "Q": [".####.", "##..##", "##..##", "##..##", "##.###", "##..##", ".###.#"],
    "R": ["#####.", "##..##", "##..##", "#####.", "####..", "##.##.", "##..##"],
    "S": [".####.", "##..##", "##....", ".####.", "....##", "##..##", ".####."],
    "T": ["######", "..##..", "..##..", "..##..", "..##..", "..##..", "..##.."],
    "U": ["##..##", "##..##", "##..##", "##..##", "##..##", "##..##", ".####."],
    "V": ["##..##", "##..##", "##..##", "##..##", "##..##", ".####.", "..##.."],
    "W": ["##...##", "##...##", "##...##", "##.#.##", "#######", "###.###", "##...##"],
    "X": ["##..##", "##..##", ".####.", "..##..", ".####.", "##..##", "##..##"],
    "Y": ["##..##", "##..##", "##..##", ".####.", "..##..", "..##..", "..##.."],
    "Z": ["######", "....##", "...##.", "..##..", ".##...", "##....", "######"],
    "apostrophe": ["##", "##", ".#", "..", "..", "..", ".."],
    "double_quote": ["##.##", "##.##", ".#..#", ".....", ".....", ".....", "....."],
    "exclamation": ["##", "##", "##", "##", "##", "..", "##"],
    "double_exclamation": ["##.##", "##.##", "##.##", "##.##", "##.##", ".....", "##.##"],
    "question": [".####.", "##..##", "....##", "..###.", "..##..", "......", "..##.."],
    "ampersand": [".###..", "##.##.", ".###..", "###.##", "##.##.", "##..##", ".###.#"],
    "percent": ["##...#", "##..##", "...##.", "..##..", ".##...", "##..##", "#...##"],
    "multiply": [".....", "##.##", ".###.", "..#..", ".###.", "##.##", "....."],
    "decimal_point": ["..", "..", "..", "..", "..", "##", "##"],
    "umlaut": ["##.##", ".....", ".....", ".....", ".....", ".....", "....."],
}


def _grid(rows):
    w = max(len(r) for r in rows)
    return np.array([[1 if (i < len(r) and r[i] == "#") else 0 for i in range(w)] for r in rows], dtype=np.uint8)


def dialog_glyph(name):
    """An 8 x 16 glyph image (alpha) as the game's dialog font draws it, the capitals' tops on row 3."""
    g = _grid(DIALOG[name])
    out = np.zeros((16, 8), np.uint8)
    h, w = g.shape
    w = min(w, 8)
    out[3:3 + h, 0:w] = g[:, :w] * 255
    return out


def hud_glyph(name):
    """A 16 x 16 HUD letter: doubled, white going to a warm grey downwards, with a dark edge."""
    g = _grid(HUD[name])
    g = np.kron(g, np.ones((2, 2), np.uint8))
    h, w = g.shape
    ox, oy = (16 - w) // 2, (16 - h) // 2
    ink = np.zeros((16, 16), np.uint8)
    ink[oy:oy + h, ox:ox + w] = g
    edge = np.zeros_like(ink)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            edge |= np.roll(np.roll(ink, dy, 0), dx, 1)
    img = np.zeros((16, 16, 4), np.float32)
    ys = np.arange(16)[:, None] * np.ones((1, 16))
    t = np.clip((ys - oy) / max(1, h), 0, 1)
    img[..., 0] = 255 - 30 * t
    img[..., 1] = 255 - 40 * t
    img[..., 2] = 255 - 90 * t
    img[..., 3] = 255
    out = np.zeros((16, 16, 4), np.float32)
    out[edge > 0] = (30, 30, 40, 255)
    out[ink > 0] = img[ink > 0]
    return out.astype(np.uint8)


# ------------------------------------------------------------------------------------------------ pictures
def disc(h, w, cx, cy, r, soft=1.0):
    ys, xs = np.mgrid[0:h, 0:w]
    d = np.sqrt((xs + 0.5 - cx) ** 2 + (ys + 0.5 - cy) ** 2)
    return np.clip((r - d) / soft + 0.5, 0, 1)


def star_mask(h, w, cx, cy, r_out, r_in, rot=-math.pi / 2):
    ys, xs = np.mgrid[0:h, 0:w]
    px, py = xs + 0.5 - cx, ys + 0.5 - cy
    ang = (np.arctan2(py, px) - rot) % (2 * math.pi / 5)
    half = math.pi / 5
    t = np.abs(ang - half) / half                      # 0 at a point's middle... 1 at the inner corner
    rr = r_in + (r_out - r_in) * (1 - t)
    d = np.sqrt(px * px + py * py)
    return np.clip((rr - d) + 0.5, 0, 1)


def hud_coin():
    img = np.zeros((16, 16, 4), np.float32)
    a = disc(16, 16, 8, 8, 7.2)
    core = disc(16, 16, 8, 8, 5.8)
    img[..., 0], img[..., 1], img[..., 2] = 200, 140, 20
    img[..., :3] = img[..., :3] * (1 - core[..., None]) + np.array([255, 216, 40]) * core[..., None]
    img[5:11, 7:9, :3] = (190, 120, 10)
    img[..., 3] = (a > 0.5) * 255
    return img.astype(np.uint8)


def hud_star():
    img = np.zeros((16, 16, 4), np.float32)
    m = star_mask(16, 16, 8, 8.6, 8, 3.6)
    inner = star_mask(16, 16, 8, 8.6, 6, 2.6)
    img[..., :3] = np.array([230, 160, 20]) * (1 - inner[..., None]) + np.array([255, 236, 60]) * inner[..., None]
    img[..., 3] = (m > 0.5) * 255
    return img.astype(np.uint8)


def hud_mario_head():
    """Mario's head, seen from the front: red cap with its white circle, hair, face, nose and moustache."""
    rows = [
        "....RRRRRR......",
        "..RRRRRRRRRR....",
        ".RRRRRWWRRRRR...",
        ".RRRRWRMWRRRRR..",
        "RRRRRWWWWRRRRR..",
        "RRRRRRRRRRRRRRR.",
        "HHHSSSSSSSSHHH..",
        "HHSSBSSSSBSSHH..",
        "HSSSBSSSSBSSSH..",
        "HSSSSSNNSSSSSH..",
        ".SSSSNNNNSSSS...",
        ".SKKKKKKKKKKS...",
        "..SKKKSSKKKS....",
        "...SSSSSSSS.....",
        "....SSSSSS......",
        "................",
    ]
    pal = {"R": (224, 16, 24), "W": (255, 255, 255), "M": (224, 16, 24), "H": (72, 40, 16), "S": (252, 196, 140),
           "B": (32, 48, 160), "N": (240, 160, 120), "K": (40, 24, 16)}
    img = np.zeros((16, 16, 4), np.uint8)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c in pal:
                img[y, x, :3] = pal[c]
                img[y, x, 3] = 255
    return img


def camera_icon(kind):
    img = np.zeros((16, 16, 4), np.uint8)
    if kind == "lakitu":
        rows = ["......GGGG......", "....GGGGGGGG....", "...GGWWGGWWGG...", "...GWBWGGWBWG...",
                "..GGWBWGGWBWGG..", "..GGGWGGGGWGGG..", "..GGGGOOOOGGGG..", "...GGGOOOOGGG...",
                "....GGGGGGGG....", "...WWWWWWWWWW...", "..WWWWWWWWWWWW..", ".WWWWWWWWWWWWWW.",
                ".WWWWWWWWWWWWWW.", "..WWWWWWWWWWWW..", "...WWW....WWW...", "................"]
        pal = {"G": (232, 200, 40), "W": (250, 250, 250), "B": (20, 20, 20), "O": (240, 120, 40)}
    else:
        rows = ["................", "................", "..BBBB..........", ".BBBBBBBBBBBB...",
                ".BGGGGGGGGGGBBB.", ".BGGGGGGGGGGBBBB", ".BGGGLLLLGGGBBBB", ".BGGLLDDLLGGBBBB",
                ".BGGLDDDDLGGBBBB", ".BGGLLDDLLGGBBBB", ".BGGGLLLLGGGBBB.", ".BGGGGGGGGGGB...",
                ".BBBBBBBBBBBB...", "................", "................", "................"]
        pal = {"B": (16, 16, 24), "G": (120, 120, 140), "L": (180, 200, 220), "D": (40, 60, 120)}
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c in pal:
                img[y, x, :3] = pal[c]
                img[y, x, 3] = 255
    if kind == "no_camera":
        for i in range(14):
            for j in (0, 1):
                for x, y in ((1 + i + j, 1 + i), (14 - i - j, 1 + i)):
                    if 0 <= x < 16:
                        img[y, x] = (230, 20, 20, 255)
    return img


def arrow(up):
    rows = ["...##...", "..####..", ".######.", "########", "..####..", "..####..", "..####..", "........"]
    if not up:
        rows = rows[::-1][1:] + ["........"]
    img = np.zeros((8, 8, 4), np.uint8)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "#":
                img[y, x] = (255, 220, 40, 255)
    return img


def button_glyph(name):
    """The controller's buttons in the dialog font: a disc (or a C button's arrow) with its letter, in alpha."""
    out = np.zeros((16, 8), np.float32)
    if name.startswith("button_C_"):
        d = name[len("button_C_"):]
        rows = {"up": ["...#...", "..###..", ".#####.", "#######", "..###..", "..###..", "..###.."],
                "down": ["..###..", "..###..", "..###..", "#######", ".#####.", "..###..", "...#..."],
                "left": ["...#...", "..##...", ".######", "#######", ".######", "..##...", "...#..."],
                "right": ["...#...", "...##..", "######.", "#######", "######.", "...##..", "...#..."]}[d]
        g = _grid(rows)
        out[4:11, 0:7] = g * 255
        return out
    letter = name[-1]
    out[3:12, 0:8] = disc(9, 8, 4, 4.5, 4.4) * 255
    g = _grid(DIALOG[letter]) if letter in DIALOG else np.zeros((9, 4), np.uint8)
    g = g[1:8, :]
    h, w = g.shape
    ox = (8 - w) // 2
    out[4:4 + h, ox:ox + w] *= (1 - g * 0.85)
    return out


def dialog_icon(name):
    """The dialog font's pictures: the coin, the stars, in alpha."""
    out = np.zeros((16, 8), np.float32)
    if name == "coin":
        out[3:12, 0:8] = disc(9, 8, 4, 4.5, 4.2) * 255
        out[5:10, 3:5] *= 0.4
    elif name in ("star_filled", "star_hollow"):
        m = star_mask(10, 8, 4, 5.2, 5, 2.2)
        if name == "star_hollow":
            m = np.clip(m - star_mask(10, 8, 4, 5.2, 3.2, 1.3), 0, 1)
        out[2:12, 0:8] = m * 255
    return out


def sideways(g):
    """The US dialog font's textures lie on their side (16 x 8): texel (s, t) is the glyph's pixel (x = 7 - t,
    row from the top = 15 - s) - see bin/segment2.c's vertex_ia8_char."""
    return np.ascontiguousarray(g[::-1, ::-1].T)


def credits_glyph(ch):
    img = np.zeros((8, 8, 4), np.uint8)
    key = {"period": "decimal_point"}.get(ch, ch)
    if key in HUD:
        g = _grid(HUD[key])
        h, w = g.shape
        w = min(w, 8)
        img[0:h, 0:w, :3] = 255
        img[0:h, 0:w, 3] = g[:, :w] * 255
    return img


def power_meter(name, w, h):
    img = np.zeros((h, w, 4), np.float32)
    if name.endswith("side"):
        # the meter's round frame: a 64 x 64 disc in two halves, blue with a light rim and POWER on top
        full = np.zeros((64, 64, 4), np.float32)
        a = disc(64, 64, 32, 32, 31)
        rim = disc(64, 64, 32, 32, 31) - disc(64, 64, 32, 32, 27)
        full[..., :3] = np.array([24, 72, 200])
        full[..., :3] = full[..., :3] * (1 - rim[..., None]) + np.array([220, 230, 255]) * rim[..., None]
        full[..., 3] = (a > 0.5) * 255
        # POWER, small, along the top
        x = 13
        for ch in "POWER":
            g = _grid(HUD[ch])
            hh, ww = g.shape
            full[6:6 + hh, x:x + ww][g > 0] = (255, 255, 255, 255)
            x += ww + 1
        img = full[:, :32] if "left" in name else full[:, 32:]
        return img.astype(np.uint8)
    # the health: a pie of 8 wedges, as many as are left, coloured as the game's (blue, green, yellow, red)
    n = {"full": 8, "seven": 7, "six": 6, "five": 5, "four": 4, "three": 3, "two": 2, "one": 1}
    k = next(v for key, v in n.items() if "_" + key + "_" in name + "_")
    col = (40, 200, 255) if k >= 7 else (40, 230, 60) if k >= 5 else (250, 220, 30) if k >= 3 else (240, 40, 30)
    ys, xs = np.mgrid[0:h, 0:w]
    px, py = xs + 0.5 - w / 2, ys + 0.5 - h / 2
    ang = (np.arctan2(px, -py)) % (2 * math.pi)
    wedge = (ang < k * 2 * math.pi / 8)
    a = disc(h, w, w / 2, h / 2, w / 2 - 1)
    img[..., :3] = col
    img[..., :3] *= 0.75 + 0.25 * (1 - np.sqrt(px ** 2 + py ** 2) / (w / 2))[..., None]
    lines = np.abs(((ang * 8 / (2 * math.pi)) % 1) - 0.5) > 0.47
    img[lines & (a > 0.5)] = (20, 20, 40, 255)
    img[..., 3] = ((a > 0.5) & wedge) * 255
    return np.clip(img, 0, 255).astype(np.uint8)


def transition(name, w, h):
    """The warps' masks (IA8): clear where the picture shows through, white (the warp's colour) around it.
    Halves are the shape's right half (the game mirrors them)."""
    img = np.zeros((h, w, 4), np.uint8)
    if "star" in name:
        m = star_mask(h, 2 * w, w, h * 0.55, h * 0.5, h * 0.21)[:, w:]
    elif "circle" in name:
        m = disc(h, 2 * w, w, h / 2, h / 2 - 0.5)[:, w:]
    elif "bowser" in name:
        m = np.clip(disc(h, 2 * w, w, h * 0.55, h * 0.42) + star_mask(h, 2 * w, w, h * 0.3, h * 0.3, h * 0.2), 0, 1)[:, w:]
    else:   # Mario's face
        m = np.clip(disc(h, w, w / 2, h * 0.55, w * 0.42) + disc(h, w, w / 2, h * 0.3, w * 0.45) *
                    (np.mgrid[0:h, 0:w][0] < h * 0.35), 0, 1)
    v = ((1 - m) * 255).astype(np.uint8)
    img[..., 0] = img[..., 1] = img[..., 2] = 255
    img[..., 3] = v
    return img


def letter_paper(right, w, h):
    """A half of Peach's letter (32 x 64, RGBA16): cream paper, darker towards its edges, with a rose border."""
    rng = np.random.default_rng(5 + right)
    ys, xs = np.mgrid[0:h, 0:w].astype(np.float32)
    ex = (w - 1 - xs) if right else xs                  # distance to the letter's outer side
    edge = np.minimum(np.minimum(ex, ys), h - 1 - ys)
    img = np.zeros((h, w, 4), np.float32)
    img[..., :3] = np.array([252, 243, 222]) - (np.clip(6 - edge, 0, 6) * 6)[..., None]
    img[..., :3] += (rng.random((h, w)) * 6 - 3)[..., None]
    rose = (edge >= 2) & (edge < 3)
    img[rose, :3] = (226, 150, 160)
    img[edge < 1, :3] = (190, 160, 120)
    img[..., 3] = 255
    return np.clip(img, 0, 255).astype(np.uint8)


def signature(w, h):
    """Peach's signature (64 x 32, IA8): her name in slanted script, white (the game tints it pink)."""
    g = np.concatenate([np.pad(_grid(DIALOG[c]), ((0, 9 - len(DIALOG[c])), (0, 1))) for c in "Peach"], axis=1)
    g = np.repeat(np.repeat(g, 2, axis=0), 2, axis=1).astype(np.float32)
    out = np.zeros((h, w), np.float32)
    gh, gw = g.shape
    for y in range(gh):
        shift = int((gh - y) * 0.35)
        n = min(gw, w - 2 - shift)
        out[y + 6, 2 + shift:2 + shift + n] = g[y, :n]
    soft = out.copy()
    soft[1:, :] = np.maximum(soft[1:, :], out[:-1, :] * 0.5)
    soft[:, 1:] = np.maximum(soft[:, 1:], out[:, :-1] * 0.5)
    img = np.zeros((h, w, 4), np.uint8)
    img[..., :3] = 255
    img[..., 3] = (soft * 255).astype(np.uint8)
    return img


def waterbox(name, w, h):
    rng = np.random.default_rng(len(name))
    ys, xs = np.mgrid[0:h, 0:w]
    wave = (np.sin((xs + ys * 0.5) * 2 * math.pi / w * 2) + np.sin((xs * 0.7 - ys) * 2 * math.pi / h * 3)) * 0.25 + 0.5
    n = rng.random((h, w)) * 0.15
    img = np.zeros((h, w, 4), np.float32)
    if "lava" in name:
        base, hi = np.array([200, 40, 0]), np.array([255, 200, 40])
    elif "mist" in name:
        base, hi = np.array([200, 200, 210]), np.array([255, 255, 255])
    elif "jrb" in name:
        base, hi = np.array([20, 70, 150]), np.array([110, 170, 240])
    else:
        base, hi = np.array([30, 90, 200]), np.array([140, 200, 255])
    t = np.clip(wave + n, 0, 1)[..., None]
    img[..., :3] = base * (1 - t) + hi * t
    img[..., 3] = 255
    return img.astype(np.uint8)


def paint_ui(name, w, h):
    """The screens' texture name (a symbol of bin/segment2.c or actors/power_meter) at w x h, RGBA."""
    if name.startswith("texture_hud_char_"):
        key = name[len("texture_hud_char_"):]
        if key in HUD:
            return hud_glyph(key)
        if key == "coin":
            return hud_coin()
        if key == "star":
            return hud_star()
        if key == "mario_head":
            return hud_mario_head()
        if key in ("camera", "lakitu", "no_camera"):
            return camera_icon(key)
        if key in ("arrow_up", "arrow_down"):
            return arrow(key == "arrow_up")
        return np.zeros((h, w, 4), np.uint8)
    if name.startswith("texture_font_char_us_"):
        key = name[len("texture_font_char_us_"):]
        if key in DIALOG:
            g = dialog_glyph(key).astype(np.float32)
        elif key.startswith("button_"):
            g = button_glyph(key)
        else:
            g = dialog_icon(key)
        a = sideways(g)
        img = np.zeros((h, w, 4), np.uint8)
        img[..., :3] = 255
        img[..., 3] = np.clip(a, 0, 255).astype(np.uint8)[:h, :w]
        return img
    if name.startswith("texture_credits_char_"):
        return credits_glyph(name[len("texture_credits_char_"):])
    if name.startswith("texture_power_meter_"):
        return power_meter(name, w, h)
    if name.startswith("texture_transition_"):
        return transition(name, w, h)
    if name.startswith("texture_waterbox_"):
        return waterbox(name, w, h)
    if name.startswith("texture_shadow_"):
        img = np.zeros((h, w, 4), np.uint8)
        img[..., 3] = (disc(h, w, 0, 0, w) * 255).astype(np.uint8) if "circle" in name else 255
        return img
    if name in ("castle_grounds_seg7_texture_0700C9E8", "castle_grounds_seg7_texture_0700D9E8"):
        return letter_paper(name.endswith("D9E8"), w, h)
    if name == "castle_grounds_seg7_us_texture_0700EAE8":
        return signature(w, h)
    if name == "texture_ia8_up_arrow":
        img = np.zeros((h, w, 4), np.uint8)
        g = arrow(True)[..., 3]
        img[..., :3] = 255
        img[:8, :8, 3] = g
        return img
    return np.zeros((h, w, 4), np.uint8)


# the size and kind of each of the screens' textures, by name (the ROM's sizes)
def ui_size(name, path):
    """(w, h, fmt) with fmt 'rgba' or 'ia'."""
    ia = ".ia" in path or ".i4" in path or ".i8" in path
    if name.startswith("texture_font_char"):
        return 16, 8, "ia"
    if name.startswith("texture_credits_char"):
        return 8, 8, "rgba"
    if name in ("texture_hud_char_arrow_up", "texture_hud_char_arrow_down"):
        return 8, 8, "rgba"
    if name.startswith("texture_hud_char"):
        return 16, 16, "rgba"
    if name.startswith("texture_power_meter_") and name.endswith("side"):
        return 32, 64, "rgba"
    if name.startswith("texture_power_meter_"):
        return 32, 32, "rgba"
    if name == "texture_transition_mario":
        return 64, 64, "ia"
    if name.startswith("texture_transition_"):
        return 32, 64, "ia"
    if name.startswith("texture_waterbox_"):
        return 32, 32, "ia" if ia else "rgba"
    if name.startswith("texture_shadow_"):
        return 16, 16, "ia"
    if name == "texture_ia8_up_arrow":
        return 8, 16, "ia"
    if name.startswith("castle_grounds_seg7_texture"):
        return 32, 64, "rgba"
    if name.startswith("castle_grounds_seg7_us_texture"):
        return 64, 32, "ia"
    return 16, 16, "ia" if ia else "rgba"
