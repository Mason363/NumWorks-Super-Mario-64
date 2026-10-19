/* The screens' display lists, played on the calculator: the decompilation's HUD, dialog boxes, menus, text and
 * screen transitions build the N64 display lists the game always built (orthographic triangles and texture
 * rectangles, with the RDP's colour combiner and alpha); this plays one over the rows being drawn (render.h's
 * r_band), once for each band. Their textures are the calculator's (PORT_TEX_STUB: an index in src/data.bin). */
#include <math.h>
#include <string.h>
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "port.h"
#include "port_gfx.h"
#include "../render.h"
#include "../data.h"

typedef float M4[4][4];

/* a command's code, as its first byte holds it (F3D's are negative numbers in gbi.h) */
#define OP(c) ((u8) (c))

/* a vertex on the screen (pixels, y down), its texel coordinates and colour */
typedef struct {
  float x, y, s, t;
  u8 c[4];
} GVtx;

static M4 sModel[10];
static int sModelTop;
static M4 sProj;
static GVtx sV[16];
static const Tex *sTex;
static u32 sFill, sOtherH;
static bool sMvpDirty;   /* (the matrices changed since the vertices' last were made) */
static float sTexScaleS, sTexScaleT;
static int sTexOn;
/* the render tile: its clamp and mirror bits, masks and rectangle (texels), s then t */
static int sCm[2], sMask[2], sUl[2], sLr[2];

/* ------------------------------------------------------------------------------------------------ matrices */
static void m4_mul(M4 d, M4 a, M4 b) {
  M4 r;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) r[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
  memcpy(d, r, sizeof r);
}

static void m4_ident(M4 m) {
  memset(m, 0, sizeof(M4));
  m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1;
}

/* the N64's fixed point matrix (guMtxF2L's layout: whole parts, then fractions) */
static void m4_from_mtx(M4 d, const Mtx *m) {
  const s32 *w = (const s32 *) m;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++) {
      int k = r * 4 + c;
      u32 hi = (u32) w[k >> 1], lo = (u32) w[8 + (k >> 1)];
      s32 whole = (s16) ((k & 1) ? (hi & 0xFFFF) : (hi >> 16));
      u32 frac = (k & 1) ? (lo & 0xFFFF) : (lo >> 16);
      d[r][c] = (f32) ((s32) ((u32) whole << 16 | frac)) / 65536.0f;
    }
}

/* ------------------------------------------------------------------------------------------------ textures */
static const Tex *tex_of(uintptr_t p) {
  const u8 *b = (const u8 *) p;
  if (b == NULL || b[0] != 0x50 || b[1] != 0x54 || b[2] != 0x58 || b[3] != 0x54) return NULL;
  u16 i = (u16) (b[4] | b[5] << 8);
  return i < HDR->ntex ? TEX(i) : NULL;
}

/* a texel coordinate as the tile takes it: from its corner, clamped (or with no mask), mirrored, masked */
static inline int tile_coord(int v, int axis, int size) {
  int m = sMask[axis];
  v -= sUl[axis];
  if ((sCm[axis] & G_TX_CLAMP) || m == 0) {
    int hi = sLr[axis] - sUl[axis];
    v = v < 0 ? 0 : v > hi ? hi : v;
  }
  if (m) {
    if ((sCm[axis] & G_TX_MIRROR) && ((v >> m) & 1)) v = ~v;
    v &= (1 << m) - 1;
  }
  return v & (size - 1);
}

/* the texel at (s, t) as RGBA */
static inline void texel(int s, int t, u8 out[4]) {
  const Tex *x = sTex;
  int w = 1 << x->wl, h = 1 << x->hl;
  s = tile_coord(s, 0, w), t = tile_coord(t, 1, h);
  const u8 *base = DAT(u8, x->off);
  const u16 *pal = (const u16 *) (const void *) base;
  int depth = x->fmt >> 4, i = t * w + s;
  u16 v = depth == 1 ? pal[base[32 + (i >> 1)] >> ((i & 1) << 2) & 15] : depth == 2 ? pal[base[512 + i]]
                                                                         : ((const u16 *) (const void *) base)[i];
  int f = x->fmt & 15;
  if (f == TF_IA) {
    out[0] = out[1] = out[2] = (u8) (v >> 8);
    out[3] = (u8) v;
  } else {
    out[0] = (u8) ((v >> 11) << 3), out[1] = (u8) (((v >> 5) & 63) << 2), out[2] = (u8) ((v & 31) << 3);
    out[3] = f == TF_565K && v == TEX_KEY ? 0 : 255;
  }
}

/* ------------------------------------------------------------------------------------------------ the combiner */
/* the combiner's inputs, as bytes: the texel's RGBA, the shade's, the primitive colour's, the environment's,
 * then 0 and 255 */
enum { SRC_TEX = 0, SRC_SHADE = 4, SRC_PRIM = 8, SRC_ENV = 12, SRC_0 = 16, SRC_1 = 17 };
static u8 sSrc[18] = {[SRC_1] = 255};
/* the first cycle's (a - b) * c + d, compiled when it is set: for red, green, blue and alpha, the sSrc bytes its
 * a, b, c and d take; whether it takes the texel, the shade */
static u8 sCC[4][4];
static bool sCCTex, sCCShade, sCCTexAlpha;   /* (sCCTexAlpha: its alpha is the texel's times something) */

static int cc_rgb_src(int sel, int k, int which) {
  /* (which: 0 a, 1 b, 2 c, 3 d; TEXEL1 is taken as TEXEL0, COMBINED as 0: one cycle) */
  switch (sel) {
    case 1: case 2: return SRC_TEX + k;
    case 3: return SRC_PRIM + k;
    case 4: return SRC_SHADE + k;
    case 5: return SRC_ENV + k;
    case 6: return which == 0 || which == 3 ? SRC_1 : SRC_0;   /* (1; b: CENTER, c: SCALE, taken as 0) */
    case 8: case 9: return which == 2 ? SRC_TEX + 3 : SRC_0;
    case 10: return which == 2 ? SRC_PRIM + 3 : SRC_0;
    case 11: return which == 2 ? SRC_SHADE + 3 : SRC_0;
    case 12: return which == 2 ? SRC_ENV + 3 : SRC_0;
    default: return SRC_0;
  }
}

static int cc_alpha_src(int sel, int which) {
  switch (sel) {
    case 0: return which == 2 ? SRC_0 : SRC_1;
    case 1: case 2: return SRC_TEX + 3;
    case 3: return SRC_PRIM + 3;
    case 4: return SRC_SHADE + 3;
    case 5: return SRC_ENV + 3;
    case 6: return which == 2 ? SRC_0 : SRC_1;
    default: return SRC_0;
  }
}

static void cc_compile(u32 w0, u32 w1) {
  int rgb[4] = {(int) (w0 >> 20) & 15, (int) (w1 >> 28) & 15, (int) (w0 >> 15) & 31, (int) (w1 >> 15) & 7};
  int al[4] = {(int) (w0 >> 12) & 7, (int) (w1 >> 12) & 7, (int) (w0 >> 9) & 7, (int) (w1 >> 9) & 7};
  sCCTex = sCCShade = false;
  for (int i = 0; i < 4; i++) {
    for (int k = 0; k < 3; k++) sCC[k][i] = (u8) cc_rgb_src(rgb[i], k, i);
    sCC[3][i] = (u8) cc_alpha_src(al[i], i);
    for (int k = 0; k < 4; k++) {
      sCCTex |= sCC[k][i] < SRC_SHADE;
      sCCShade |= sCC[k][i] >= SRC_SHADE && sCC[k][i] < SRC_PRIM;
    }
  }
  const u8 *m = sCC[3];
  sCCTexAlpha = m[1] == SRC_0 && m[3] == SRC_0 && (m[0] == SRC_TEX + 3 || m[2] == SRC_TEX + 3);
}

static void set_color(int at, u32 rgba) {
  for (int k = 0; k < 4; k++) sSrc[at + k] = (u8) (rgba >> (24 - 8 * k));
}

/* the combiner's colour of sSrc's texel and shade (RGBA) */
static inline void combine(u8 out[4]) {
  for (int k = 0; k < 4; k++) {
    const u8 *m = sCC[k];
    int v = (sSrc[m[0]] - sSrc[m[1]]) * sSrc[m[2]];
    v = ((v + (v >> 8) + 128) >> 8) + sSrc[m[3]];
    out[k] = (u8) (v < 0 ? 0 : v > 255 ? 255 : v);
  }
}

static inline void put(uint16_t *px, const u8 *c) {
  if (c[3] == 0) return;
  uint16_t v = rgb565(c[0], c[1], c[2]);
  *px = c[3] == 255 ? v : blend565(v, *px, c[3] + 1);
}

/* ------------------------------------------------------------------------------------------------ drawing */
static int cycle(void) { return (int) (sOtherH >> G_MDSFT_CYCLETYPE) & 3; }

static void fill_rect(int x0, int y0, int x1, int y1) {
  u8 c[4];
  if (cycle() == 3) {
    u16 f = (u16) sFill;
    c[0] = (u8) ((f >> 11) << 3), c[1] = (u8) (((f >> 6) & 31) << 3), c[2] = (u8) (((f >> 1) & 31) << 3), c[3] = 255;
    x1++, y1++;
  } else {
    memset(sSrc, 0, SRC_PRIM);
    combine(c);
  }
  if (y0 < r_band_y) y0 = r_band_y;
  if (y1 > r_band_y + r_band_h) y1 = r_band_y + r_band_h;
  if (x0 < 0) x0 = 0;
  if (x1 > 320) x1 = 320;
  for (int y = y0; y < y1; y++) {
    uint16_t *row = r_row(y);
    for (int x = x0; x < x1; x++) put(row + x, c);
  }
}

static void tex_rect(int xl, int yl, int xh, int yh, int s0, int t0, int dsdx, int dtdy, bool flip) {
  /* (10.2 coordinates, 10.5 texels, 5.10 steps; the copy mode steps four texels a pixel and includes its far edge) */
  bool copy = cycle() == 2;
  float x0 = xl / 4.0f, y0 = yl / 4.0f, x1 = xh / 4.0f + (copy ? 1 : 0), y1 = yh / 4.0f + (copy ? 1 : 0);
  float ds = dsdx / 1024.0f / (copy ? 4 : 1), dt = dtdy / 1024.0f;
  int ya = (int) ceilf(y0 - 0.5f), yb = (int) ceilf(y1 - 0.5f), xa = (int) ceilf(x0 - 0.5f), xb = (int) ceilf(x1 - 0.5f);
  if (ya < r_band_y) ya = r_band_y;
  if (yb > r_band_y + r_band_h) yb = r_band_y + r_band_h;
  if (xa < 0) xa = 0;
  if (xb > 320) xb = 320;
  if (sTex == NULL || ya >= yb || xa >= xb) return;
  memset(sSrc + SRC_SHADE, 255, 4);
  /* (along a row: s, or t when flipped) */
  float da = flip ? dt : ds;
  for (int y = ya; y < yb; y++) {
    uint16_t *row = r_row(y);
    float along = (flip ? t0 : s0) / 32.0f + (xa + 0.5f - x0) * da;
    int across = (int) floorf((flip ? s0 : t0) / 32.0f + (y + 0.5f - y0) * (flip ? ds : dt));
    for (int x = xa; x < xb; x++, along += da) {
      int a = (int) floorf(along);
      u8 c[4];
      texel(flip ? across : a, flip ? a : across, sSrc + SRC_TEX);
      if (copy || (sSrc[SRC_TEX + 3] == 0 && sCCTexAlpha)) put(row + x, sSrc + SRC_TEX);   /* (copy: no combiner) */
      else {
        combine(c);
        put(row + x, c);
      }
    }
  }
}

/* a triangle, a row at a time; its texel coordinates and shade from their planes over the screen */
static void tri(const GVtx *a, const GVtx *b, const GVtx *c) {
  float e1x = b->x - a->x, e1y = b->y - a->y, e2x = c->x - a->x, e2y = c->y - a->y;
  float area = e1x * e2y - e2x * e1y;
  if (fabsf(area) < 1e-3f) return;
  float ymin = fminf(a->y, fminf(b->y, c->y)), ymax = fmaxf(a->y, fmaxf(b->y, c->y));
  int ya = (int) ceilf(ymin - 0.5f), yb = (int) ceilf(ymax - 0.5f);
  if (ya < r_band_y) ya = r_band_y;
  if (yb > r_band_y + r_band_h) yb = r_band_y + r_band_h;
  if (ya >= yb) return;
  bool tex = sTexOn && sTex != NULL && sCCTex, shade = sCCShade;
  if (shade && memcmp(a->c, b->c, 4) == 0 && memcmp(a->c, c->c, 4) == 0) {
    memcpy(sSrc + SRC_SHADE, a->c, 4);   /* (one shade all over: the screen's fades) */
    shade = false;
  }
  /* the planes of s, t, r, g, b and alpha: value = at + dx * (x - a->x) + dy * (y - a->y) */
  float at[6], dx[6], dy[6], ia = 1 / area;
  float va[6] = {a->s, a->t, a->c[0], a->c[1], a->c[2], a->c[3]};
  float vb[6] = {b->s, b->t, b->c[0], b->c[1], b->c[2], b->c[3]};
  float vc[6] = {c->s, c->t, c->c[0], c->c[1], c->c[2], c->c[3]};
  for (int i = 0; i < 6; i++) {
    float d1 = vb[i] - va[i], d2 = vc[i] - va[i];
    at[i] = va[i], dx[i] = (d1 * e2y - d2 * e1y) * ia, dy[i] = (d2 * e1x - d1 * e2x) * ia;
  }
  u8 col[4];
  if (!tex) sSrc[SRC_TEX] = sSrc[SRC_TEX + 1] = sSrc[SRC_TEX + 2] = 0, sSrc[SRC_TEX + 3] = 255;
  if (!tex && !shade) combine(col);   /* (one colour all over) */
  const GVtx *v[3] = {a, b, c};
  for (int y = ya; y < yb; y++) {
    float py = y + 0.5f, xl = 1e9f, xr = -1e9f;
    for (int e = 0; e < 3; e++) {
      const GVtx *p = v[e], *q = v[e == 2 ? 0 : e + 1];
      if ((p->y <= py && q->y > py) || (q->y <= py && p->y > py)) {
        float x = p->x + (py - p->y) * (q->x - p->x) / (q->y - p->y);
        xl = fminf(xl, x), xr = fmaxf(xr, x);
      }
    }
    int xa = (int) ceilf(xl - 0.5f), xb = (int) ceilf(xr - 0.5f);
    if (xa < 0) xa = 0;
    if (xb > 320) xb = 320;
    if (xa >= xb) continue;
    uint16_t *row = r_row(y);
    if (!tex && !shade) {
      for (int x = xa; x < xb; x++) put(row + x, col);
      continue;
    }
    float ox = xa + 0.5f - a->x, oy = py - a->y, f[6];
    for (int i = 0; i < 6; i++) f[i] = at[i] + dx[i] * ox + dy[i] * oy;
    for (int x = xa; x < xb; x++) {
      if (tex) {
        texel((int) floorf(f[0]), (int) floorf(f[1]), sSrc + SRC_TEX);
        if (sSrc[SRC_TEX + 3] == 0 && sCCTexAlpha) goto next;   /* (clear) */
      }
      if (shade)
        for (int k = 0; k < 4; k++) {
          float s = f[2 + k];
          sSrc[SRC_SHADE + k] = (u8) (s < 0 ? 0 : s > 255 ? 255 : s);
        }
      combine(col);
      put(row + x, col);
    next:
      for (int i = 0; i < 6; i++) f[i] += dx[i];
    }
  }
}

static void load_vertices(const Vtx *v, int n, int v0) {
  static M4 mvp;
  if (sMvpDirty) m4_mul(mvp, sModel[sModelTop], sProj), sMvpDirty = false;
  for (int i = 0; i < n && v0 + i < 16; i++) {
    const Vtx_t *s = &v[i].v;
    float x = s->ob[0], y = s->ob[1], z = s->ob[2];
    float cx = x * mvp[0][0] + y * mvp[1][0] + z * mvp[2][0] + mvp[3][0];
    float cy = x * mvp[0][1] + y * mvp[1][1] + z * mvp[2][1] + mvp[3][1];
    float cw = x * mvp[0][3] + y * mvp[1][3] + z * mvp[2][3] + mvp[3][3];
    if (cw == 0) cw = 1;
    GVtx *d = &sV[v0 + i];
    d->x = (cx / cw + 1) * 160.0f;
    d->y = (1 - cy / cw) * 120.0f;
    d->s = s->tc[0] / 32.0f * sTexScaleS;
    d->t = s->tc[1] / 32.0f * sTexScaleT;
    memcpy(d->c, s->cn, 4);
  }
}

/* ------------------------------------------------------------------------------------------------ the list */
void port_gfx_run(const Gfx *dl) {
  const Gfx *stack[10];
  int depth = 0;
  m4_ident(sModel[0]);
  m4_ident(sProj);
  sModelTop = 0;
  sTex = NULL, sTexOn = 0, sTexScaleS = sTexScaleT = 1;
  sCm[0] = sCm[1] = sMask[0] = sMask[1] = sUl[0] = sUl[1] = 0;
  sLr[0] = sLr[1] = 1023;
  set_color(SRC_PRIM, 0xFFFFFFFF), set_color(SRC_ENV, 0xFFFFFFFF), sFill = 0, sOtherH = 0;
  cc_compile(0, 0);
  sMvpDirty = true;
  for (int guard = 0; guard < 8192 && dl != NULL; guard++) {
    const Gfx *g = dl++;
    u32 w0 = (u32) g->words.w0;
    uintptr_t w1 = g->words.w1;
    switch (w0 >> 24) {
      case OP(G_ENDDL):
        dl = depth > 0 ? stack[--depth] : NULL;
        break;
      case OP(G_DL):
        if (((w0 >> 16) & 255) == G_DL_PUSH && depth < 10) stack[depth++] = dl;
        dl = (const Gfx *) w1;
        break;
      case OP(G_MTX): {
        u32 p = (w0 >> 16) & 255;
        sMvpDirty = true;
        M4 m;
        m4_from_mtx(m, (const Mtx *) w1);
        if (p & G_MTX_PROJECTION) {
          if (p & G_MTX_LOAD) memcpy(sProj, m, sizeof m);
          else m4_mul(sProj, m, sProj);
        } else {
          if ((p & G_MTX_PUSH) && sModelTop < 9) {
            memcpy(sModel[sModelTop + 1], sModel[sModelTop], sizeof(M4));
            sModelTop++;
          }
          if (p & G_MTX_LOAD) memcpy(sModel[sModelTop], m, sizeof m);
          else m4_mul(sModel[sModelTop], m, sModel[sModelTop]);
        }
        break;
      }
      case OP(PORT_G_TRANSLATE): {
        sMvpDirty = true;
        if ((w0 & 255) && sModelTop < 9) {
          memcpy(sModel[sModelTop + 1], sModel[sModelTop], sizeof(M4));
          sModelTop++;
        }
        M4 t;
        m4_ident(t);
        t[3][0] = (s16) (w1 >> 16) / 16.0f, t[3][1] = (s16) (w1 & 0xFFFF) / 16.0f;
        m4_mul(sModel[sModelTop], t, sModel[sModelTop]);
        break;
      }
      case OP(G_POPMTX):
        if (sModelTop > 0) sModelTop--;
        sMvpDirty = true;
        break;
      case OP(G_VTX):
        load_vertices((const Vtx *) w1, (int) ((w0 >> 20) & 15) + 1, (int) ((w0 >> 16) & 15));
        break;
      case OP(G_TRI1): {
        int a = (int) ((w1 >> 16) & 255) / 10, b = (int) ((w1 >> 8) & 255) / 10, c = (int) (w1 & 255) / 10;
        if (a < 16 && b < 16 && c < 16) tri(&sV[a], &sV[b], &sV[c]);
        break;
      }
      case OP(G_TEXTURE):
        sTexOn = (int) (w0 & 255);
        sTexScaleS = (float) ((w1 >> 16) & 0xFFFF) / 65536.0f;
        sTexScaleT = (float) (w1 & 0xFFFF) / 65536.0f;
        if (sTexScaleS > 0.99f) sTexScaleS = 1;
        if (sTexScaleT > 0.99f) sTexScaleT = 1;
        break;
      case OP(G_SETTIMG):
        sTex = tex_of(w1);
        break;
      case OP(G_SETTILE):
        if (((w1 >> 24) & 7) == G_TX_RENDERTILE) {
          sCm[0] = (int) (w1 >> 8) & 3, sMask[0] = (int) (w1 >> 4) & 15;
          sCm[1] = (int) (w1 >> 18) & 3, sMask[1] = (int) (w1 >> 14) & 15;
        }
        break;
      case OP(G_SETTILESIZE):
        if (((w1 >> 24) & 7) == G_TX_RENDERTILE) {
          sUl[0] = (int) ((w0 >> 12) & 0xFFF) >> 2, sUl[1] = (int) (w0 & 0xFFF) >> 2;
          sLr[0] = (int) ((w1 >> 12) & 0xFFF) >> 2, sLr[1] = (int) (w1 & 0xFFF) >> 2;
        }
        break;
      case OP(G_SETCOMBINE):
        cc_compile(w0, (u32) w1);
        break;
      case OP(G_SETENVCOLOR):
        set_color(SRC_ENV, (u32) w1);
        break;
      case OP(G_SETPRIMCOLOR):
        set_color(SRC_PRIM, (u32) w1);
        break;
      case OP(G_SETFILLCOLOR):
        sFill = (u32) w1;
        break;
      case OP(G_SETOTHERMODE_H): {
        /* (F3D: shift in bits 8-15, length in bits 0-7) */
        u32 sft = (w0 >> 8) & 255, len = w0 & 255, mask = ((1u << len) - 1) << sft;
        sOtherH = (sOtherH & ~mask) | ((u32) w1 & mask);
        break;
      }
      case OP(G_RDPSETOTHERMODE):
        sOtherH = w0 & 0xFFFFFF;
        break;
      case OP(G_FILLRECT):
        fill_rect((int) ((w1 >> 14) & 1023), (int) ((w1 >> 2) & 1023), (int) ((w0 >> 14) & 1023), (int) ((w0 >> 2) & 1023));
        break;
      case OP(G_TEXRECT):
      case OP(G_TEXRECTFLIP): {
        const Gfx *h = dl;
        u32 st, dd;
        u8 next = (u8) (h->words.w0 >> 24);
        if (next == OP(G_RDPHALF_1) || next == OP(G_RDPHALF_2)) {
          st = (u32) h[0].words.w1, dd = (u32) h[1].words.w1;
          dl += 2;
        } else {
          st = (u32) h[0].words.w0, dd = (u32) h[0].words.w1;
          dl += 1;
        }
        tex_rect((int) ((w1 >> 12) & 0xFFF), (int) (w1 & 0xFFF), (int) ((w0 >> 12) & 0xFFF), (int) (w0 & 0xFFF),
                 (s16) (st >> 16), (s16) (st & 0xFFFF), (s16) (dd >> 16), (s16) (dd & 0xFFFF),
                 (w0 >> 24) == OP(G_TEXRECTFLIP));
        break;
      }
      default:
        break;
    }
  }
}
