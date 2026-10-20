"""Mario's head on the screen after the title: goddard's model (the decompilation's src/goddard/dynlists), as a
still mesh for the calculator's renderer. Goddard (the game's own engine for the head) moves it with skins and
joints, which don't fit: here are its shapes as they rest, the face, the eyebrows and the mustache where their
data puts them, and each eye's iris on the white of its eye (where its joint turns it to, looking ahead), with
the materials' colours and smooth normals for the renderer's light."""
import os
import re

import numpy as np

DYN = os.path.join("src", "goddard", "dynlists")


def _shapes(text):
    """A dynlist file's vertex and face arrays, and its materials' colours (by group, then id)."""
    verts = {m.group(1): np.array([[int(x) for x in g] for g in re.findall(r"\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}",
                                                                           m.group(2))], float)
             for m in re.finditer(r"static s16 (\w+)\[\]\[3\] = \{(.*?)\};", text, re.S)}
    faces = {m.group(1): [[int(x) for x in g] for g in re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*\}",
                                                                   m.group(2))]
             for m in re.finditer(r"static u16 (\w+)\[\]\[4\] = \{(.*?)\};", text, re.S)}
    mats = {}
    for blk in re.finditer(r"StartGroup\((\w+)\),(.*?)EndGroup", text, re.S):
        for mm in re.finditer(r"SetId\((\d+)\),\s*SetAmbient\([^)]*\),\s*SetDiffuse\(([^)]*)\)", blk.group(2)):
            mats.setdefault(blk.group(1), {})[int(mm.group(1))] = [float(x) for x in mm.group(2).split(",")]
    return verts, faces, mats


def _normal(p):
    n = np.cross(p[1] - p[0], p[2] - p[0])
    ln = np.linalg.norm(n)
    return n / ln if ln else n


def head(ref):
    """The head's triangles: [(3 points, colour (0..1 RGB))], in goddard's space (y up, the face towards +z)."""
    def read(name):
        return _shapes(open(os.path.join(ref, DYN, name)).read())
    fv, ff, fm = read("dynlist_mario_face.c")
    ev, ef, em = read("dynlists_mario_eyes.c")
    bv, bf, bm = read("dynlists_mario_eyebrows_mustache.c")
    V, F = fv["mario_Face_VtxData"], ff["mario_Face_FaceData"]
    face_colours = fm["DYNOBJ_MARIO_FACE_MTL_GROUP"]
    tris = [(V[[a, b, c]], face_colours[m]) for m, a, b, c in F]
    # the irises: each eye's shape (a disc facing its joint's x) turned onto the middle of the white of its eye
    for verts, faces, mats, side in (("verts_mario_eye_right", "facedata_mario_eye_right",
                                      "DYNOBJ_MARIO_RIGHT_EYE_MTL_GROUP", 1),
                                     ("verts_mario_eye_left", "facedata_mario_eye_left",
                                      "DYNOBJ_MARIO_LEFT_EYE_MTL_GROUP", -1)):
        whites = [V[[a, b, c]] for m, a, b, c in F if m == 4 and V[[a, b, c]][:, 0].mean() * side > 0]
        centre = np.mean([w.mean(0) for w in whites], 0)
        n = np.sum([np.cross(w[1] - w[0], w[2] - w[0]) for w in whites], 0)
        n /= np.linalg.norm(n)
        if n[2] < 0:
            n = -n
        up = np.array([0.0, 1.0, 0.0])
        y = up - n * up.dot(n)
        y /= np.linalg.norm(y)
        x = np.cross(y, n)
        E = ev[verts]
        mid = E.mean(0)
        for m, a, b, c in ef[faces]:
            pts = [centre + n * (E[i][0] - mid[0] + 6) + y * (E[i][2] - mid[2]) * 0.9 + x * (E[i][1] - mid[1]) * 0.9 * side
                   for i in (a, b, c)]
            if side < 0:
                pts = [pts[0], pts[2], pts[1]]   # (mirrored: the same side outwards)
            tris.append((np.array(pts), em[mats][m]))
    # the eyebrows and the mustache (their material is black, a little lighter so the light shows)
    for verts, faces in (("verts_mario_eyebrow_right", "facedata_mario_eyebrow_right"),
                         ("verts_mario_eyebrow_left", "facedata_mario_eyebrow_left"),
                         ("verts_mario_mustache", "facedata_mario_mustache")):
        for m, a, b, c in bf[faces]:
            tris.append((bv[verts][[a, b, c]], [0.06, 0.04, 0.03]))
    return tris


def head_tris(ref, decomp):
    """The head as decomp.Tri, lit (normals) with each material's colour as its light's."""
    tris = head(ref)
    # smooth normals: each vertex's faces of the same colour
    acc = {}
    for p, col in tris:
        n = np.cross(p[1] - p[0], p[2] - p[0])
        for q in p:
            k = (tuple(np.round(q, 1)), tuple(col))
            acc[k] = acc.get(k, 0) + n
    out = []
    for p, col in tris:
        m = decomp.Material()
        m.tex, m.fmt, m.siz, m.w, m.h, m.cms, m.cmt = None, 0, 2, 32, 32, 0, 0
        m.combine, m.layer, m.cutout, m.xlu, m.fog, m.env, m.lit = "shade", 1, False, False, False, None, True
        m.texgen, m.decal, m.scale = False, False, (1.0, 1.0)
        amb = [int(c * 255 * 0.42) for c in col]
        dif = [int(c * 255 * 0.70) for c in col]
        light = (amb, dif, [0x28, 0x28, 0x28])
        vs = []
        for q in p:
            n = acc[(tuple(np.round(q, 1)), tuple(col))]
            n = n / (np.linalg.norm(n) or 1)
            nb = [int(round(c * 127)) & 255 for c in n]
            vs.append((int(round(q[0])), int(round(q[1])), int(round(q[2])), 0, 0, ("n", [c - 256 if c > 127 else c for c in nb],
                                                                                     255, light)))
        out.append(decomp.Tri(m, vs))
    return out


if __name__ == "__main__":
    import sys
    print(len(head(sys.argv[1])), "triangles")
