#!/usr/bin/env python3
"""Checks of Super Mario 64 on a computer (make test): the host build, with AddressSanitizer and UBSan, through
the title and the keys, Peach's letter and the castle grounds, every level the calculator has, a star got and
saved (and its copy in sm64_saves.py), the pause menu's way out, and the ways into levels left out.

    python3 tests/check.py build/play"""
import base64
import os
import re
import shutil
import subprocess
import sys
import tempfile

PLAY = sys.argv[1] if len(sys.argv) > 1 else "build/play"
failures = []
ENV = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
# the game's level numbers (levels/level_defines.h)
# (the castle grounds: in the new game)
LEVELS = {6: "the castle", 26: "the castle's courtyard", 9: "Bob-omb Battlefield",
          24: "Whomp's Fortress", 12: "Jolly Roger Bay", 5: "Cool, Cool Mountain", 4: "Big Boo's Haunt"}
# (the calculator's display list memory: 640 commands, its matrices and vertices counted in commands too)
GFX = 640


def run(args, saves=None, what=""):
    """Plays (in a save folder, a new one if None); returns the last state printed and the folder's files."""
    own = saves is None
    if own:
        saves = tempfile.mkdtemp(prefix="sm64_check_")
    shots = tempfile.mkdtemp(prefix="sm64_shots_")
    p = subprocess.run([PLAY, "--saves", saves, "--out", shots, "--stats"] + args, capture_output=True, text=True,
                       env=ENV, timeout=1800)
    shutil.rmtree(shots, ignore_errors=True)
    files = {f: open(os.path.join(saves, f), "rb").read() for f in sorted(os.listdir(saves))}
    if own:
        shutil.rmtree(saves, ignore_errors=True)
    if p.returncode or "runtime error" in p.stderr or "AddressSanitizer" in p.stderr:
        failures.append(f"{what or args}: crashed or a sanitizer error\n{p.stderr[-2000:]}")
        print("FAIL", what, flush=True)
        return None
    frames = re.findall(r"frame (\d+): .*?; level (\d+) area (\d+) act (\d+); mario (-?\d+) (-?\d+) (-?\d+) "
                        r"action ([0-9a-f]+)", p.stdout)
    peaks = re.findall(r"objects \d+ of (\d+) \(most (\d+)\).*?pool free \d+ \(least (\d+)\), display list most (\d+)",
                       p.stdout)
    if not frames or not peaks:
        failures.append(f"{what}: no state printed\n{p.stdout[-500:]}")
        return None
    f = frames[-1]
    return {"frame": int(f[0]), "level": int(f[1]), "area": int(f[2]), "act": int(f[3]),
            "pos": tuple(int(x) for x in f[4:7]), "action": int(f[7], 16), "levels": {int(x[1]) for x in frames},
            "capacity": int(peaks[-1][0]), "objects": int(peaks[-1][1]), "pool": int(peaks[-1][2]),
            "gfx": int(peaks[-1][3]), "quit": "quit at frame" in p.stdout, "files": files}


def check(cond, what):
    if not cond:
        failures.append(what)
    print(("ok  " if cond else "FAIL"), what, flush=True)


def limits(e, what):
    check(e["objects"] < e["capacity"] and e["pool"] > 1024 and e["gfx"] <= GFX - 32,
          f"{what}: {e['objects']} objects of {e['capacity']}, {e['pool'] // 1024} KB of the pool left, "
          f"{e['gfx']} display list commands")


# a new game: the title, Mario's head, the file select, the keys, Peach's letter read, Lakitu's flight to the
# castle, the warp pipe and Lakitu's welcome read, then Mario runs
saves = tempfile.mkdtemp(prefix="sm64_check_")
e = run(["--frames", "2020", "--keys", "20:a,40:a,60:a,80:a," + ",".join(f"{t + 20}:a" for t in (
         300, 400, 500, 600, 700, 1550, 1600, 1650, 1700, 1750)) + ",1820-2020:up"], saves, "a new game")
if e:
    check(e["level"] == 16 and e["pos"][2] < 4000 and "sm64.sav" in e["files"] and "sm64_saves.py" in e["files"],
          f"a new game: the title, the keys, Peach's letter, the castle grounds, Mario runs to {e['pos']}; "
          "the save chip written")
    limits(e, "the castle grounds")

# every level the calculator has, its first act: Mario runs and jumps about
walk = ",".join(f"{t}-{t + 60}:up,{t + 60}-{t + 90}:left,{t + 20}:a" for t in range(150, 900, 90))
for num, name in LEVELS.items():
    e = run(["--level", str(num), "--frames", "900", "--keys", "15:a,60:a,100:a," + walk], what=name)
    if e:
        check(e["level"] == num and e["frame"] >= 870, f"{name}: played to the end, at {e['pos']}")
        limits(e, name)

# a star: Behind Chain Chomp's Gate, got from above; saved, and in sm64_saves.py too
shutil.rmtree(saves, ignore_errors=True)
saves = tempfile.mkdtemp(prefix="sm64_check_")
e = run(["--level", "9", "--frames", "700", "--at", "120:1550,1500,300,0",
         "--keys", "15:a,40:a,80:a,300:a,330:a,360:a,390:a,420:a,450:a"], saves, "a star")
if e:
    sav = e["files"].get("sm64.sav", b"")
    copy = e["files"].get("sm64_saves.py", b"")
    line = copy.split(b"#>sm64.sav:")[1].split(b"\n")[0] if b"#>sm64.sav:" in copy else b""
    check(e["level"] == 6 and len(sav) == 512 and sav[12] & 0x20,
          "Bob-omb Battlefield: a star got, back in the castle, saved in sm64.sav")
    check(line and base64.b64decode(line) == sav, "sm64_saves.py holds the same save")
    # installing apps again keeps only the scripts: the save comes back from the copy
    os.remove(os.path.join(saves, "sm64.sav"))
    e2 = run(["--frames", "60"], saves, "the save from its copy")
    if e2:
        check(e2["files"].get("sm64.sav") == sav, "after a reinstall, sm64.sav comes back from sm64_saves.py")
shutil.rmtree(saves, ignore_errors=True)

# the pause menu's way out: Shift asks, Back stays, then OK quits
e = run(["--level", "9", "--frames", "400", "--keys", "15:a,40:a,80:a,130:start,200:z,215:b,240:z,260:a"],
        what="quitting")
if e:
    check(e["quit"] and e["frame"] < 270, "the pause menu: Shift asks, Back stays, OK quits the game")

# the ways into levels the calculator does not have: the Princess's Secret Slide and Bowser's trapdoor do nothing
e = run(["--level", "6", "--frames", "300", "--at", "120:2013,900,-2014,0", "--keys", "15:a,40:a"],
        what="the slide's way in")
if e:
    check(e["levels"] == {6, 1} or e["levels"] == {6}, "the Princess's Secret Slide: its way in does nothing")
e = run(["--level", "6", "--frames", "300", "--at", "120:-5513,700,-4324,0", "--keys", "15:a,40:a,150-200:up"],
        what="the trapdoor")
if e:
    check(6 in e["levels"] and 17 not in e["levels"], "Bowser's trapdoor stays shut")

if failures:
    print(f"{len(failures)} failed")
    sys.exit(1)
print("all ok")
