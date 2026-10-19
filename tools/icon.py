#!/usr/bin/env python3
"""icon.py OUT.png: the app's icon (55 x 56): a Power Star, gold with its eyes, on a sky-blue round tile."""
import math
import sys

from PIL import Image, ImageDraw

S = 4   # drawn 4 times larger, then reduced
W, H = 55 * S, 56 * S
im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
d.rounded_rectangle([0, 0, W - 1, H - 1], radius=12 * S, fill=(70, 140, 235, 255))
d.rounded_rectangle([0, H // 2, W - 1, H - 1], radius=12 * S, fill=(45, 105, 205, 255))
d.rectangle([0, H // 2, W - 1, H // 2 + 12 * S], fill=(45, 105, 205, 255))
cx, cy, ro, ri = W / 2, H / 2 + 2 * S, 25 * S, 11 * S
pts = []
for i in range(10):
    a = -math.pi / 2 + i * math.pi / 5
    r = ro if i % 2 == 0 else ri
    pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
d.polygon([(x + 2 * S, y + 3 * S) for x, y in pts], fill=(150, 90, 10, 255))
d.polygon(pts, fill=(255, 205, 40, 255))
inner = [(cx + (x - cx) * 0.72, cy - 2 * S + (y - cy) * 0.72) for x, y in pts]
d.polygon(inner, fill=(255, 235, 110, 255))
for ex in (-5, 5):
    d.ellipse([cx + ex * S - 2.2 * S, cy - 6 * S, cx + ex * S + 2.2 * S, cy + 3 * S], fill=(30, 20, 10, 255))
im = im.resize((55, 56), Image.LANCZOS)
im.save(sys.argv[1])
