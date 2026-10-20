#!/usr/bin/env python3
"""Super Mario 64's pictures and animations, from the computer build (which draws exactly what the calculator does):
docs/*.png, docs/media/sm64.gif (640 x 480), docs/media/gallery.gif (320 x 240) and docs/media/thumb_sm64.png
(160 x 120).

    make host && python3 tools/media.py build/play"""
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image

GAME = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = GAME
sys.path.insert(0, os.path.join(ROOT, "tools"))
from record import save_gif  # noqa: E402 (NumPlay's GIF maker: one palette, gifsicle)

PLAY = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(GAME, "build/play"))
ENV = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
# a new game: the title, Mario's head, the file select, the keys, Peach's letter, Lakitu's flight, the warp pipe
# and Lakitu's welcome read
NEW = "70:a,110:a,150:a,190:a," + ",".join(f"{t + 130}:a" for t in (300, 400, 500, 600, 700, 1550, 1600, 1650, 1700, 1750))
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

# (the GIF's frames: every 5th of the game's 30 a second)
STEP = 5

# a new game: the title, Mario's head, the file select, the keys, Peach's letter, then Mario runs up to the castle
RUN = list(range(1940, 2120, STEP))
new = {"title": 60, "head": 100, "files": 140, "keys": 180, "letter": 460, "grounds": 2010}
pics = play(["--keys", NEW + ",1930-2130:up"], list(new.values()) + RUN, 2130)
for name, f in new.items():
    save_png(pics[f], os.path.join(docs, f"{name}.png"))
anim = [pics[f] for f in RUN]

# the courses: as each starts, then Mario runs about
courses = {"bob": 9, "wf": 24, "jrb": 12, "ccm": 5, "bbh": 4}
walk = "200-260:up,260-280:left,280-340:up"
for name, num in courses.items():
    shots = [150] + (list(range(200, 340, STEP)) if name == "bob" else [])
    pics = play(["--level", str(num), "--keys", COURSE + "," + walk], shots, 340)
    save_png(pics[150], os.path.join(docs, f"{name}.png"))
    if name == "bob":
        anim += [pics[f] for f in range(200, 340, STEP)]

# the act select, and a star: Behind Chain Chomp's Gate, back in the castle with it
pics = play(["--level", "9", "--keys", "15:a,40:a,80:a,300:a,330:a,360:a,390:a,420:a,450:a",
             "--at", "120:1550,1500,300,0"], [70] + list(range(160, 420, STEP)), 420)
save_png(pics[70], os.path.join(docs, "acts.png"))
save_png(pics[400], os.path.join(docs, "star.png"))
anim += [pics[f] for f in range(160, 420, STEP)]

# the castle's lobby: the painting of Bob-omb Battlefield; the pause menu's question
pics = play(["--level", "6", "--keys", "15:a,40:a", "--at", "150:-4500,500,-153,-16384"], [250], 260)
save_png(pics[250], os.path.join(docs, "painting.png"))
pics = play(["--level", "9", "--keys", COURSE + ",130:start,200:z"], [215], 220)
save_png(pics[215], os.path.join(docs, "quit.png"))

save_gif([(i * 1000 * STEP // 30, im) for i, im in enumerate(anim)], os.path.join(media, "sm64.gif"), 2, 6)
print("sm64.gif", os.path.getsize(os.path.join(media, "sm64.gif")) // 1024, "KB")
save_gif([(i * 2000 * STEP // 30, im) for i, im in enumerate(anim[::2])], os.path.join(media, "gallery.gif"), 1, 3)
print("gallery.gif", os.path.getsize(os.path.join(media, "gallery.gif")) // 1024, "KB")
thumb = Image.open(os.path.join(docs, "grounds.png")).convert("RGB").resize((160, 120), Image.LANCZOS)
save_png(thumb, os.path.join(media, "thumb_sm64.png"))
