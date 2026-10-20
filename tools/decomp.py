"""Reads the Super Mario 64 decompilation's data files (github.com/n64decomp/sm64, CC0): vertices, lights,
display lists, geo layouts, collision, macro objects, level scripts and animations. Everything here is
plain parsing of the C sources: the game's ROM is never needed (its textures are not in the decompilation:
tools/assets.py gets them).

The display lists are run like the N64's RSP would: vertices loaded into a 16-entry buffer, then triangles
from it, under the current texture, combiner, geometry mode and lights. What comes out is a list of
triangles, each with its material and three vertices (position, texture coordinates in texels, colour or
lit colour)."""
import glob
import math
import os
import re

CONST = {
    "G_TX_WRAP": 0, "G_TX_MIRROR": 1, "G_TX_CLAMP": 2, "G_TX_NOMIRROR": 0, "G_TX_NOMASK": 0, "G_TX_NOLOD": 0,
    "G_TEXTURE_IMAGE_FRAC": 2, "G_TX_RENDERTILE": 0, "G_TX_LOADTILE": 7,
    "G_IM_FMT_RGBA": 0, "G_IM_FMT_YUV": 1, "G_IM_FMT_CI": 2, "G_IM_FMT_IA": 3, "G_IM_FMT_I": 4,
    "G_IM_SIZ_4b": 0, "G_IM_SIZ_8b": 1, "G_IM_SIZ_16b": 2, "G_IM_SIZ_32b": 3,
    "G_IM_SIZ_16b_BYTES": 2, "G_IM_SIZ_8b_BYTES": 1, "G_IM_SIZ_4b_BYTES": 0, "G_IM_SIZ_32b_BYTES": 4,
    "G_ON": 1, "G_OFF": 0, "TRUE": 1, "FALSE": 0, "NULL": 0,
}
FMT = {0: "rgba", 2: "ci", 3: "ia", 4: "i"}
SIZ = {0: 4, 1: 8, 2: 16, 3: 32}


def load_constants(root):
    """Every plain #define NAME number and every enum of the decompilation's headers, into CONST."""
    paths = glob.glob(os.path.join(root, "include", "*.h")) + glob.glob(os.path.join(root, "src", "game", "*.h")) + \
        glob.glob(os.path.join(root, "src", "engine", "*.h"))
    for p in paths:
        txt = strip_comments(open(p, encoding="utf-8", errors="replace").read())
        for m in re.finditer(r"#define\s+([A-Z_][A-Z0-9_]*)\s+\(?(-?(?:0x[0-9A-Fa-f]+|\d+))\)?\s*$", txt, re.M):
            CONST.setdefault(m.group(1), int(m.group(2), 0))
        for m in re.finditer(r"enum\s*\w*\s*\{(.*?)\}", txt, re.S):
            v = -1
            for item in m.group(1).split(","):
                item = item.strip()
                if not item:
                    continue
                if "=" in item:
                    n, e = item.split("=", 1)
                    try:
                        v = cval(e)
                    except Exception:
                        continue
                    n = n.strip()
                else:
                    n, v = item, v + 1
                if re.match(r"^[A-Za-z_]\w*$", n):
                    CONST.setdefault(n, v)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def cval(s, extra=None):
    """A C constant expression's value."""
    s = s.strip()
    if not s:
        return 0
    s = re.sub(r"(0x[0-9A-Fa-f]+|\d+)[uUlL]+\b", r"\1", s)

    def name(m):
        n = m.group(0)
        if extra and n in extra:
            return str(extra[n])
        if n in CONST:
            return str(CONST[n])
        if n.startswith("0x") or n[0].isdigit():
            return n
        raise KeyError(n)
    expr = re.sub(r"\b[A-Za-z_]\w*\b|0x[0-9A-Fa-f]+", name, s)
    expr = expr.replace("/", "//")
    return int(eval(expr, {"__builtins__": {}}))


def split_args(s):
    """The arguments of a macro call, split on the commas that are not nested."""
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "({[":
            depth += 1
        elif ch in ")}]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def arrays(text, ctype):
    """name -> body of every `... ctype name[] = { body };` in text."""
    out = {}
    pat = re.compile(r"\b" + ctype + r"\s+(\w+)\s*\[[^\]]*\]\s*=\s*\{", re.S)
    for m in pat.finditer(text):
        i, depth = m.end(), 1
        while depth:
            c = text[i]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
            i += 1
        out[m.group(1)] = text[m.end():i - 1]
    return out


def calls(body):
    """[(macro, [args])] of a body made of MACRO(args), MACRO(args), ..."""
    out = []
    i, n = 0, len(body)
    while i < n:
        m = re.compile(r"([A-Za-z_]\w*)\s*\(").search(body, i)
        if not m:
            break
        j, depth = m.end(), 1
        while depth:
            c = body[j]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            j += 1
        out.append((m.group(1), split_args(body[m.end():j - 1])))
        i = j
    return out


def us_version(text, version="VERSION_US"):
    """The text as the US version's build sees it: the #if, #ifdef and #ifndef whose conditions name only
    VERSION_* (and defined, !, ||, &&) are decided, the others are kept as they are."""
    out, stack = [], []   # stack: (decided, taking, taken) for each #if; decided False: kept as is

    def taking():
        return all(t for d, t, _ in stack if d)

    def decide(cond):
        c = cond.strip()
        m = re.fullmatch(r"(!?)\s*(?:defined\s*\(?\s*)?(VERSION_\w+)\s*\)?", c)
        names = re.findall(r"\b\w+\b", c)
        if any(n not in ("defined",) and not n.startswith("VERSION_") for n in names):
            return None
        e = re.sub(r"defined\s*\(\s*(\w+)\s*\)|defined\s+(\w+)",
                   lambda k: str((k.group(1) or k.group(2)) == version), c)
        e = re.sub(r"\bVERSION_\w+\b", lambda k: str(k.group(0) == version), e)
        e = e.replace("||", " or ").replace("&&", " and ").replace("!", " not ")
        try:
            return bool(eval(e, {"__builtins__": {}}, {}))
        except Exception:
            return None
    for line in text.split("\n"):
        d = line.strip()
        m = re.match(r"#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)", d)
        if m:
            kw, rest = m.group(1), m.group(2)
            if kw in ("if", "ifdef", "ifndef"):
                cond = rest if kw == "if" else (("" if kw == "ifdef" else "!") + "defined(" + rest.strip() + ")")
                v = decide(cond)
                stack.append((v is not None, bool(v), bool(v)))
                if v is None and taking():
                    out.append(line)
                continue
            if not stack:
                out.append(line)
                continue
            dec, tk, taken = stack[-1]
            if not dec:
                stack.pop() if kw == "endif" else None
                if taking():
                    out.append(line)
                continue
            if kw == "elif":
                v = decide(rest)
                v = bool(v) and not taken
                stack[-1] = (True, v, taken or v)
            elif kw == "else":
                stack[-1] = (True, not taken, True)
            else:
                stack.pop()
            continue
        if taking():
            out.append(line)
    return "\n".join(out)


class Source:
    """Every data file read so far, by symbol."""

    def __init__(self, root):
        self.root = root
        self.vtx, self.lights, self.gfx, self.tex, self.geo = {}, {}, {}, {}, {}
        self.col, self.macro, self.anims, self.values, self.indices = {}, {}, {}, {}, {}
        self.trajectories, self.paintings, self.texture_lists = {}, {}, {}
        self.read_files = set()

    def read(self, rel):
        path = os.path.join(self.root, rel)
        if path in self.read_files or not os.path.exists(path):
            return
        self.read_files.add(path)
        text = us_version(strip_comments(open(path, encoding="utf-8", errors="replace").read()))
        for name, body in arrays(text, "Vtx").items():
            vs = []
            for m in re.finditer(r"\{\{\{([^}]*)\},([^,]*),\{([^}]*)\},\{([^}]*)\}\}\}", body.replace(" ", "")):
                p = [cval(x) for x in m.group(1).split(",")]
                t = [cval(x) for x in m.group(3).split(",")]
                c = [cval(x) for x in m.group(4).split(",")]
                vs.append((p[0], p[1], p[2], t[0], t[1], c[0], c[1], c[2], c[3]))
            self.vtx[name] = vs
        for m in re.finditer(r"Lights1\s+(\w+)\s*=\s*gdSPDefLights1\(([^)]*)\)", text):
            a = [cval(x) for x in split_args(m.group(2))]
            self.lights[m.group(1)] = (a[0:3], a[3:6], [x - 256 if x > 127 else x for x in a[6:9]])
        for name, body in arrays(text, "Gfx").items():
            self.gfx[name] = calls(body)
        for name, body in arrays(text, "Texture").items():
            inc = re.search(r'#include\s+"([^"]+)"', body)
            if inc:
                self.tex[name] = inc.group(1).replace(".inc.c", ".png")
        for name, body in arrays(text, "GeoLayout").items():
            self.geo[name] = calls(body)
        for name, body in arrays(text, "Collision").items():
            self.col[name] = calls(body)
        for name, body in arrays(text, "MacroObject").items():
            self.macro[name] = calls(body)
        for name, body in arrays(text, "Trajectory").items():
            self.trajectories[name] = calls(body)
        for name, body in arrays(text, "s16").items():
            self.values[name] = [cval(x) for x in body.split(",") if x.strip()]
        for name, body in arrays(text, "u16").items():
            self.indices[name] = [cval(x) for x in body.split(",") if x.strip()]
        for m in re.finditer(r"struct\s+Animation\s+(\w+)\s*(?:\[\s*\])?\s*=\s*\{(.*?)\};", text, re.S):
            self.anims[m.group(1)] = split_args(m.group(2))
        # the paintings: where they hang, how big, their images
        for m in re.finditer(r"struct\s+Painting\s+(\w+)\s*=\s*\{(.*?)\};", text, re.S):
            f = [x.strip() for x in split_args(m.group(2))]
            try:
                self.paintings[m.group(1)] = {"rot": (float(f[7].rstrip("f")), float(f[8].rstrip("f"))),
                                              "pos": tuple(float(x.rstrip("f")) for x in f[9:12]),
                                              "textures": f[29], "tw": cval(f[30]), "th": cval(f[31]),
                                              "size": float(f[-1].rstrip("f"))}
            except (ValueError, IndexError):
                pass
        for m in re.finditer(r"Texture\s*\*const\s+(\w+)\s*\[\]\s*=\s*\{(.*?)\};", text, re.S):
            self.texture_lists[m.group(1)] = [x.strip() for x in m.group(2).split(",") if x.strip()]
        # texture symbols declared as u8 arrays with an include (some actors)
        for m in re.finditer(r"(?:Texture|u8)\s+(\w+)\s*\[[^\]]*\]\s*=\s*\{\s*#include\s+\"([^\"]+)\"", text):
            self.tex.setdefault(m.group(1), m.group(2).replace(".inc.c", ".png"))


# ---------------------------------------------------------------------------------------------- display lists

class Material:
    """What a triangle is drawn with."""
    __slots__ = ("tex", "fmt", "siz", "w", "h", "cms", "cmt", "combine", "layer", "cutout", "xlu", "fog", "env",
                 "lit", "scale", "texgen", "decal")

    def key(self):
        return (self.tex, self.w, self.h, self.cms, self.cmt, self.combine, self.layer, self.cutout, self.xlu,
                self.texgen)


class Tri:
    __slots__ = ("mat", "v")

    def __init__(self, mat, v):
        self.mat, self.v = mat, v


def tex_combine(cc):
    """What the combiner does: 'shade' (no texture), 'mod' (texture x shade), 'decal' (texture alone), 'blend' (the
    texture where it is opaque, the shade where it is clear: Mario's eyes on his face)."""
    if cc.startswith("G_CC_SHADE"):
        return "shade"
    if cc.startswith("G_CC_BLENDRGB"):
        return "blend"
    if "MODULATE" in cc:
        return "mod"
    if "DECAL" in cc:
        return "decal"
    if "BLEND" in cc or cc == "G_CC_FADEA" or cc == "G_CC_FADE":
        return "shade"
    return "mod"


def run_dl(src, name, layer, out, light_dir=None, state=None, light=None):
    """Appends the triangles of display list `name` to out (a list of Tri) and returns the state it ends in. The
    state carries on into nested lists, as on the N64; light: the lights a list drawn before it left set."""
    if state is None:
        state = {"vbuf": [None] * 32, "tex": None, "fmt": 0, "siz": 2, "w": 32, "h": 32, "cms": 0, "cmt": 0,
                 "combine": "mod", "lit": True, "light": light, "scale": (1.0, 1.0), "on": False, "cutout": False,
                 "xlu": False, "fog": False, "env": None, "texgen": False, "render_tex": False}
    for cmd, a in src.gfx[name]:
        if cmd == "gsSPDisplayList":
            run_dl(src, a[0], layer, out, light_dir, state)
        elif cmd == "gsSPBranchList":
            run_dl(src, a[0], layer, out, light_dir, state)
            return state
        elif cmd == "gsSPEndDisplayList":
            return state
        elif cmd == "gsSPVertex":
            ref = a[0].replace(" ", "")
            off = 0
            if "+" in ref:
                ref, o = ref.split("+")
                off = cval(o)
            n, v0 = cval(a[1]), cval(a[2])
            vs = src.vtx[ref][off:off + n]
            for i, v in enumerate(vs):
                x, y, z, s, t, r, g, b, al = v
                if state["lit"]:
                    # (lights set by a display list drawn before, on the N64: white, from the front, if none here)
                    L = state["light"] or ([0x7F] * 3, [0xFF] * 3, [0x28] * 3)
                    col = ("n", [c - 256 if c > 127 else c for c in (r, g, b)], al, L)
                else:
                    col = ("c", (r, g, b), al, None)
                state["vbuf"][v0 + i] = (x, y, z, s * state["scale"][0], t * state["scale"][1], col)
        elif cmd in ("gsSP1Triangle", "gsSP2Triangles"):
            idx = [cval(x) for x in a]
            groups = [idx[0:3]] + ([idx[4:7]] if cmd == "gsSP2Triangles" else [])
            mat = Material()
            textured = state["on"] and state["combine"] != "shade" and state["tex"] is not None
            mat.tex = state["tex"] if textured else None
            mat.fmt, mat.siz, mat.w, mat.h = state["fmt"], state["siz"], state["w"], state["h"]
            mat.cms, mat.cmt = state["cms"], state["cmt"]
            mat.combine = state["combine"] if textured else "shade"
            mat.layer = layer
            mat.cutout = layer in (4,) or state["cutout"]
            mat.xlu = layer in (5, 6, 7) or state["xlu"]
            mat.fog, mat.env, mat.lit = state["fog"], state["env"], state["lit"]
            mat.texgen = state["texgen"]
            mat.decal = layer in (2, 6)
            for g in groups:
                vs = [state["vbuf"][i] for i in g]
                if None in vs:
                    continue
                if state.get("fade") and state["env"] is not None and len(state["env"]) > 3:
                    ea = state["env"][3]
                    vs = [(x, y, z, s, t, (c[0], c[1], c[2] * ea // 255, c[3])) for x, y, z, s, t, c in vs]
                out.append(Tri(mat, vs))
        elif cmd == "gsSPLight":
            ln = a[0].replace("&", "").strip()
            m = re.match(r"^(\w+)\.([la])", ln)
            if not m:
                continue   # (a light read from inside another array: kept as it was)
            base, part = m.group(1), m.group(2)
            L = src.lights.get(base)
            if L:
                if part == "a":
                    state["light"] = (L[0], state["light"][1] if state["light"] else L[1], L[2])
                else:
                    state["light"] = (state["light"][0] if state["light"] else L[0], L[1], L[2])
        elif cmd == "gsSPSetGeometryMode":
            if "G_LIGHTING" in a[0]:
                state["lit"] = True
            if "G_TEXTURE_GEN" in a[0]:
                state["texgen"] = True
            if "G_FOG" in a[0]:
                state["fog"] = True
        elif cmd == "gsSPClearGeometryMode":
            if "G_LIGHTING" in a[0]:
                state["lit"] = False
            if "G_TEXTURE_GEN" in a[0]:
                state["texgen"] = False
            if "G_FOG" in a[0]:
                state["fog"] = False
        elif cmd == "gsSPTexture":
            s, t, on = cval(a[0]), cval(a[1]), cval(a[4])
            state["scale"] = ((s + 1) / 65536 if s != 0xFFFF else 1.0, (t + 1) / 65536 if t != 0xFFFF else 1.0)
            state["on"] = bool(on)
        elif cmd == "gsDPSetTextureImage":
            state["tex"] = a[3].strip()
            state["fmt"], state["siz"] = cval(a[0]), cval(a[1])
        elif cmd == "gsDPLoadTextureBlock":
            state["tex"] = a[0].strip()
            state["fmt"], state["siz"] = cval(a[1]), cval(a[2])
            state["w"], state["h"] = cval(a[3]), cval(a[4])
            state["cms"], state["cmt"] = cval(a[6]), cval(a[7])
        elif cmd == "gsDPSetTile":
            if cval(a[4]) == 0:
                state["cmt"], state["cms"] = cval(a[6]), cval(a[9])
                state["fmt"], state["siz"] = cval(a[0]), cval(a[1])
        elif cmd == "gsDPSetTileSize":
            if cval(a[0]) == 0:
                state["w"] = (cval(a[3]) >> 2) + 1
                state["h"] = (cval(a[4]) >> 2) + 1
        elif cmd == "gsDPSetCombineMode":
            state["combine"] = tex_combine(a[0])
            # (the shade's alpha times the environment's: a see-through model, the transparent star)
            state["fade"] = "FADEA" in a[0]
        elif cmd == "gsDPSetRenderMode":
            m = a[0] + a[1]
            state["cutout"] = "TEX_EDGE" in m
            state["xlu"] = "XLU" in m
            state["fog"] = "FOG" in m or state["fog"]
        elif cmd == "gsDPSetEnvColor":
            state["env"] = tuple(cval(x) for x in a)
    return state


def shade(v, light_dir=None):
    """A vertex's colour: its own, or lit (the N64's ambient plus one directional light, in world space)."""
    x, y, z, s, t, col = v
    kind, c, al, L = col
    if kind == "c":
        return (c[0], c[1], c[2], al)
    amb, dif, d = L
    if light_dir is not None:
        d = light_dir
    n = math.sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]) or 1
    dl = math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) or 1
    dot = max(0.0, (c[0] * d[0] + c[1] * d[1] + c[2] * d[2]) / (n * dl))
    return tuple(min(255, int(amb[i] + dif[i] * dot)) for i in range(3)) + (255,)


# ---------------------------------------------------------------------------------------------- geo layouts

LAYERS = {"LAYER_FORCE": 0, "LAYER_OPAQUE": 1, "LAYER_OPAQUE_DECAL": 2, "LAYER_OPAQUE_INTER": 3, "LAYER_ALPHA": 4,
          "LAYER_TRANSPARENT": 5, "LAYER_TRANSPARENT_DECAL": 6, "LAYER_TRANSPARENT_INTER": 7}


def layer_of(s):
    s = s.strip()
    return LAYERS[s] if s in LAYERS else cval(s)


class GeoNode:
    def __init__(self, kind, args, parent=None):
        self.kind, self.args, self.parent, self.children = kind, args, parent, []


def geo_tree(src, name):
    """The geo layout as a tree of GeoNode (branches followed)."""
    root = GeoNode("root", [])
    stack, last = [root], None

    def walk(nm):
        nonlocal last
        for cmd, a in src.geo[nm]:
            if cmd == "GEO_OPEN_NODE":
                stack.append(last)
            elif cmd == "GEO_CLOSE_NODE":
                stack.pop()
            elif cmd in ("GEO_END", "GEO_RETURN"):
                return
            elif cmd == "GEO_BRANCH":
                walk(a[1].strip())
                if cval(a[0]) == 0:
                    return
            elif cmd == "GEO_BRANCH_AND_LINK":
                walk(a[0].strip())
            else:
                node = GeoNode(cmd, a, stack[-1])
                stack[-1].children.append(node)
                last = node
    walk(name)
    return root
