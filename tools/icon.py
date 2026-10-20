#!/usr/bin/env python3
"""icon.py REF ASSETS OUT.png: the app's icon (55 x 56), from the game itself: Mario's head (goddard's model, as
tools/goddard.py reads it from the decompilation in REF) in front of the sky of Peach's castle (the game's water
skybox, from the folder tools/assets.py fills). Then python3 tools/round_icons.py OUT.png rounds its corners."""
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import goddard  # noqa: E402

S = 4                   # drawn 4 times bigger, then reduced
W, H = 55 * S, 56 * S


def render(tris, scale, cx, cy):
    """The triangles seen from the front (+z), lit from the upper left, with a depth buffer: colours and coverage."""
    img = np.zeros((H, W, 3), np.float32)
    depth = np.full((H, W), -1e9, np.float32)
    light = np.array([-0.45, 0.55, 0.70])
    light /= np.linalg.norm(light)
    ys, xs = np.mgrid[0:H, 0:W]
    for p, col in tris:
        n = np.cross(p[1] - p[0], p[2] - p[0])
        ln = np.linalg.norm(n)
        if ln == 0 or n[2] <= 0:
            continue
        k = 0.42 + 0.62 * max(0.0, n.dot(light) / ln)
        sx = cx + p[:, 0] * scale
        sy = cy - p[:, 1] * scale
        x0, x1 = int(max(0, np.floor(sx.min()))), int(min(W - 1, np.ceil(sx.max())))
        y0, y1 = int(max(0, np.floor(sy.min()))), int(min(H - 1, np.ceil(sy.max())))
        if x0 > x1 or y0 > y1:
            continue
        px, py = xs[y0:y1 + 1, x0:x1 + 1] + 0.5, ys[y0:y1 + 1, x0:x1 + 1] + 0.5
        d = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0])
        if abs(d) < 1e-9:
            continue
        w1 = ((px - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (py - sy[0])) / d
        w2 = ((sx[1] - sx[0]) * (py - sy[0]) - (px - sx[0]) * (sy[1] - sy[0])) / d
        w0 = 1 - w1 - w2
        inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        z = w0 * p[0, 2] + w1 * p[1, 2] + w2 * p[2, 2]
        sub = depth[y0:y1 + 1, x0:x1 + 1]
        take = inside & (z > sub)
        sub[take] = z[take]
        img[y0:y1 + 1, x0:x1 + 1][take] = np.clip(np.array(col) * k, 0, 1)
    return img, depth > -1e9


def main():
    ref, assets, out = sys.argv[1:4]
    tris = goddard.head(ref)
    pts = np.concatenate([p for p, _ in tris])
    lo, hi = pts.min(0), pts.max(0)
    scale = (H * 0.98) / (hi[1] - lo[1])
    img, cover = render(tris, scale, W / 2 - (lo[0] + hi[0]) / 2 * scale, H * 0.99 + lo[1] * scale)
    # the sky behind: the top of the castle's skybox (blue, its clouds)
    sky = np.array(Image.open(os.path.join(assets, "textures", "skyboxes", "water.png")).convert("RGB"))
    back = np.array(Image.fromarray(sky[8:8 + 96, 20:20 + 94]).resize((W, H), Image.BICUBIC)).astype(np.float32) / 255
    rgb = np.where(cover[..., None], img, back)
    icon = Image.fromarray((rgb * 255 + 0.5).astype(np.uint8)).resize((55, 56), Image.LANCZOS).convert("RGBA")
    icon.save(out)
    print(out, icon.size)


if __name__ == "__main__":
    main()
