"""The game's own textures: PNG files of the ROM's textures, by their path in the decompilation (for example
"actors/mario/mario_logo.rgba16.png"), in the folder tools/assets.py fills (ASSETS, or SM64_ASSETS in the
environment). tools/assets.py checks each one against the ROM's own bytes (rom_textures.txt)."""
import os

import numpy as np
from PIL import Image

ASSETS = os.environ.get("SM64_ASSETS", "")


def path_of(path):
    """The PNG of a texture of the decompilation (its .png or .inc.c path)."""
    if path.endswith(".inc.c"):
        path = path[:-len(".inc.c")] + ".png"
    return os.path.join(ASSETS, path)


def has(path):
    return bool(ASSETS) and os.path.exists(path_of(path))


def load(path, w=None, h=None):
    """The texture as RGBA (numpy, h x w x 4), its texels in the order the N64 reads them: a texture loaded at
    another width than its PNG's (the dialog font's letters, stored on their side) is the same texels, in rows of
    w. None when the folder has no such file."""
    if not has(path):
        return None
    img = np.array(Image.open(path_of(path)).convert("RGBA"))
    if w is None or h is None or img.shape[:2] == (h, w):
        return img
    flat = img.reshape(-1, 4)
    if len(flat) >= w * h:
        return flat[: w * h].reshape(h, w, 4).copy()
    # (a texture drawn bigger than it is: repeated, as the N64's texture memory wraps)
    return np.resize(flat, (h * w, 4)).reshape(h, w, 4).copy()
