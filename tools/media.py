#!/usr/bin/env python3
"""Super Mario 64's pictures and animations, from the computer build (which draws exactly what the calculator does):
docs/*.png, and the repository's docs/media/sm64.gif (640 x 480), docs/media/gallery/sm64.gif (320 x 240) and
docs/media/thumb_sm64.png (160 x 120).

    make host && python3 tools/media.py build/play"""
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image

GAME = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.dirname(os.path.dirname(GAME))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from record import save_gif  # noqa: E402 (the repository's GIF maker: one palette, gifsicle)

PLAY = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(GAME, "build/play"))
ENV = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
# a new game: the title, the keys, Peach's letter, Lakitu's flight, the warp pipe and Lakitu's welcome read
NEW = "20:a,60:a,300:a,400:a,500:a,600:a,700:a,1550:a,1600:a,1650:a,1700:a,1750:a"
# a course (a save file made): the title, its act select, the first act
COURSE = "15:a,40:a,90:a"


def play(args, shots, frames):
    """Plays in a new save folder and returns {frame: picture} for each shot asked."""
    saves, out = tempfile.mkdtemp(prefix="sm64_media_"), tempfile.mkdtemp(prefix="sm64_media_")
    shots = sorted(set(shots))
    subprocess.run([PLAY, "--saves", saves, "--out", out, "--frames", str(frames), "--shots",
                    ",".join(map(str, shots))] + args, check=True, capture_output=True, env=ENV)
    pics = {}
    for f in shots:
        with Image.open(os.path.join(out, f"shot_{f:05d}.ppm")) as im:
            pics[f] = im.convert("RGB")
    shutil.rmtree(saves)
    shutil.rmtree(out)
    return pics


def save_png(im, path):
    im.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).save(path, optimize=True)
    print(path, os.path.getsize(path) // 1024, "KB")


docs = os.path.join(GAME, "docs")
media = os.path.join(ROOT, "docs", "media")
os.makedirs(docs, exist_ok=True)

# a new game: the title, the keys, Peach's letter, then Mario runs up to the castle
RUN = list(range(1810, 1990, 4))
new = {"title": 10, "keys": 40, "letter": 330, "grounds": 1880}
pics = play(["--keys", NEW + ",1800-2000:up"], list(new.values()) + RUN, 2000)
for name, f in new.items():
    save_png(pics[f], os.path.join(docs, f"{name}.png"))
anim = [pics[f] for f in RUN]

# the courses: Mario runs about in each
courses = {"bob": 9, "wf": 24, "jrb": 12, "ccm": 5, "bbh": 4}
walk = "200-260:up,260-280:left,280-340:up"
for name, num in courses.items():
    shots = [300] + (list(range(200, 340, 4)) if name == "bob" else [])
    pics = play(["--level", str(num), "--keys", COURSE + "," + walk], shots, 340)
    save_png(pics[300], os.path.join(docs, f"{name}.png"))
    if name == "bob":
        anim += [pics[f] for f in range(200, 340, 4)]

# the act select, and a star: Behind Chain Chomp's Gate, back in the castle with it
pics = play(["--level", "9", "--keys", "15:a,40:a,80:a,300:a,330:a,360:a,390:a,420:a,450:a",
             "--at", "120:1550,1500,300,0"], [70] + list(range(160, 420, 4)), 420)
save_png(pics[70], os.path.join(docs, "acts.png"))
save_png(pics[400], os.path.join(docs, "star.png"))
anim += [pics[f] for f in range(160, 420, 4)]

# the castle's lobby: the painting of Bob-omb Battlefield; the pause menu's question
pics = play(["--level", "6", "--keys", "15:a,40:a", "--at", "150:-4500,500,-153,-16384"], [250], 260)
save_png(pics[250], os.path.join(docs, "painting.png"))
pics = play(["--level", "9", "--keys", COURSE + ",130:start,200:z"], [215], 220)
save_png(pics[215], os.path.join(docs, "quit.png"))

# (every 4th frame of the game's 30 a second)
save_gif([(i * 133, im) for i, im in enumerate(anim)], os.path.join(media, "sm64.gif"), 2, 8)
print("sm64.gif", os.path.getsize(os.path.join(media, "sm64.gif")) // 1024, "KB")
save_gif([(i * 266, im) for i, im in enumerate(anim[::2])], os.path.join(media, "gallery", "sm64.gif"), 1, 4)
print("gallery/sm64.gif", os.path.getsize(os.path.join(media, "gallery", "sm64.gif")) // 1024, "KB")
thumb = Image.open(os.path.join(docs, "grounds.png")).convert("RGB").resize((160, 120), Image.LANCZOS)
save_png(thumb, os.path.join(media, "thumb_sm64.png"))
