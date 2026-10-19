#!/usr/bin/env python3
"""Downloads the decompilation this port is made from: n64decomp/sm64 (github.com/n64decomp/sm64), at the commit it
was made from, into <folder>/decomp. It has the game's code and its data files (models, levels, animations, text),
not the ROM's pictures and sounds. Then make data REF=<folder>/decomp makes src/data.bin, src/data_ids.h and
src/port/gen/ (tools/pack.py and tools/port.py), and copies the files the calculator compiles into decomp/.

    python3 tools/fetch.py build/ref"""
import io
import os
import subprocess
import sys
import tarfile
import urllib.request

COMMIT = "9921382a68bb0c865e5e45eb594d9c64db59b1af"
REPO = "https://github.com/n64decomp/sm64"


def from_tarball(out):
    with urllib.request.urlopen(f"https://codeload.github.com/n64decomp/sm64/tar.gz/{COMMIT}", timeout=600) as r:
        data = r.read()
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tar:
        for m in tar.getmembers():
            # (sm64-<commit>/... -> decomp/...)
            parts = m.name.split("/", 1)
            if len(parts) < 2 or not parts[1] or m.issym() or m.islnk() or ".." in parts[1].split("/"):
                continue
            m.name = parts[1]
            tar.extract(m, out)


def from_git(out):
    subprocess.run(["git", "clone", "--quiet", "--filter=blob:none", "--no-checkout", REPO, out], check=True)
    subprocess.run(["git", "-C", out, "checkout", "--quiet", COMMIT], check=True)


def main():
    out = os.path.join(sys.argv[1] if len(sys.argv) > 1 else "build/ref", "decomp")
    if os.path.exists(os.path.join(out, "Makefile")):
        print(out, "is there already")
        return
    try:
        from_tarball(out)
    except OSError as e:
        print("the archive:", e, "- cloning instead")
        from_git(out)
    print(out, "ready: make data REF=" + out)


if __name__ == "__main__":
    main()
