/* The 3D renderer (render.h): a band of rows at a time, each with its colours and a 15-bit depth buffer (the
 * inverse of the distance, so near things are precise). Triangles are clipped against the near plane in camera
 * space, then filled a row at a time: depth, colours and fog step linearly, texture coordinates are corrected
 * for perspective every 8 pixels. -O2: this runs for every pixel. */
#include <stdlib.h>
#include <string.h>
#include "plat.h"
#include "render.h"

#define NEAR_MIN 10.0f
#define DECAL_BIAS 24

int r_w = 320, r_h = 240, r_scale = 1;
uint16_t *r_band;
int r_band_y, r_band_h;
int r_stat_tris, r_stat_items, r_stat_verts;

static RFrame *F;
static int nitems, nrows, overlay_used;

/* the camera */
static float cam_f, cam_cx, cam_cy, cam_near, cam_far, z_k;
static Mtx43 view;   /* world to camera: x right, y up, z forward (the roll included) */
static float roll_c = 1, roll_s;

/* the sky and the fog */
static int sky_kind;
static int16_t sky_yaw;
static float sky_pitch;
static uint16_t fog_col;
static float fog_near, fog_far, fog_k;   /* (fog_k: 255 / (far - near)) */
static bool fog_on;

enum { I_MESH, I_SHADOW };

/* ------------------------------------------------------------------------------------------------ the frame */
void r_begin(RFrame *f) {
  F = f;
  nitems = nrows = overlay_used = 0;
  r_stat_tris = r_stat_verts = 0;
}

/* turns camera-space points by the picture's roll */
static void apply_roll(Mtx43 *m) {
  if (roll_s == 0) return;
  for (int i = 0; i < 4; i++) {
    float x = m->m[i][0], y = m->m[i][1];
    m->m[i][0] = x * roll_c - y * roll_s, m->m[i][1] = x * roll_s + y * roll_c;
  }
}

void r_frame(const Mtx43 *v, float roll, float fov_deg, float near, float far) {
  roll_c = cosf(roll), roll_s = sinf(roll);
  view = *v;
  apply_roll(&view);
  Vec3f f = {v->m[0][2], v->m[1][2], v->m[2][2]};
  v3_normalize(f);
  cam_f = (r_h * 0.5f) / tanf(fov_deg * (M_PI_F / 360.0f));
  cam_cx = r_w * 0.5f, cam_cy = r_h * 0.5f;
  cam_near = near < NEAR_MIN ? NEAR_MIN : near, cam_far = far;
  z_k = 16000.0f * cam_near;   /* (the depth at the near plane: 14 bits, room to step past it) */
  sky_pitch = asinf(f[1] > 1 ? 1 : f[1] < -1 ? -1 : f[1]);
  fog_on = false;
  sky_kind = 0x8000;
}

const Mtx43 *r_view(void) { return &view; }

void *r_scratch(int *bytes) {
  *bytes = (int)sizeof F->col;
  return F ? F->col : NULL;
}

void r_sky(int sky, int16_t yaw) { sky_kind = sky, sky_yaw = yaw; }
void r_fog(uint16_t color, float near, float far) {
  fog_on = far > near && far > 0;
  fog_col = color, fog_near = near, fog_far = far;
  fog_k = far > near ? 255.0f / (far - near) : 0;
}

bool r_project(const Vec3f p, float *sx, float *sy, float *depth) {
  Vec3f c;
  mtx_apply(&view, p, c);
  if (c[2] < cam_near) return false;
  float iz = 1 / c[2];
  *sx = (cam_cx + cam_f * c[0] * iz) * r_scale, *sy = (cam_cy - cam_f * c[1] * iz) * r_scale;
  if (depth) *depth = c[2];
  return true;
}

/* the rows a box (model space, under mv) covers; false when it is out of sight */
static bool box_rows(const Mtx43 *mv, const int16_t lo[3], const int16_t hi[3], uint8_t *y0, uint8_t *y1, float *depth) {
  float ymin = 1e9f, ymax = -1e9f, zmin = 1e9f;
  bool behind = false, left = true, right = true, above = true, below = true, far = true;
  for (int i = 0; i < 8; i++) {
    float px = i & 1 ? hi[0] : lo[0], py = i & 2 ? hi[1] : lo[1], pz = i & 4 ? hi[2] : lo[2];
    float cx = px * mv->m[0][0] + py * mv->m[1][0] + pz * mv->m[2][0] + mv->m[3][0];
    float cy = px * mv->m[0][1] + py * mv->m[1][1] + pz * mv->m[2][1] + mv->m[3][1];
    float cz = px * mv->m[0][2] + py * mv->m[1][2] + pz * mv->m[2][2] + mv->m[3][2];
    if (cz < zmin) zmin = cz;
    if (cz < cam_far) far = false;
    /* (against the frustum's side planes, with a little room) */
    float lim = cz * (cam_cx + 2) / cam_f, limy = cz * (cam_cy + 2) / cam_f;
    if (cx > -lim) left = false;
    if (cx < lim) right = false;
    if (cy < limy) above = false;
    if (cy > -limy) below = false;
    if (cz < cam_near) {
      behind = true;
      continue;
    }
    float sy = cam_cy - cam_f * cy / cz;
    if (sy < ymin) ymin = sy;
    if (sy > ymax) ymax = sy;
  }
  if (far || left || right || above || below) return false;
  if (behind) ymin = 0, ymax = (float)r_h;
  if (ymax < 0 || ymin >= r_h) return false;
  *y0 = (uint8_t)(ymin < 0 ? 0 : ymin);
  *y1 = (uint8_t)(ymax >= r_h ? r_h : ymax + 1);
  if (depth) *depth = zmin;
  return true;
}

static int layer_of(const Batch *b, uint8_t alpha) {
  return alpha < 255 || (b->mode & M_XLU) ? 4 : (b->mode & (M_CUTOUT | M_DECAL)) ? 2 : 1;
}

void r_mesh(const Mesh *m, const Mtx43 *mtx, uint32_t tint, uint8_t alpha, int flags) {
  if (!F || nitems >= R_MAX_ITEMS || !m || !alpha) return;
  RItem *it = &F->items[nitems];
  if (flags & R_CAMERA) {
    it->mv = *mtx;
    apply_roll(&it->mv);
  } else if (mtx) mtx_mul(&it->mv, mtx, &view);
  else it->mv = view;
  uint8_t y0, y1;
  float depth;
  if (!box_rows(&it->mv, m->lo, m->hi, &y0, &y1, &depth)) return;
  if (nrows + m->nbatch > R_MAX_BATCHES) return;
  it->kind = I_MESH, it->mesh = m, it->tint = tint, it->alpha = alpha, it->depth = depth, it->flags = (uint8_t)flags;
  it->y0 = y0, it->y1 = y1, it->batch0 = (uint16_t)nrows;
  it->layers = 0;
  const Batch *b = DAT(Batch, m->batch);
  for (int i = 0; i < m->nbatch; i++) {
    uint8_t *r = F->rows[nrows++];
    if (m->nbatch == 1) r[0] = y0, r[1] = y1;
    else if (!box_rows(&it->mv, b[i].lo, b[i].hi, &r[0], &r[1], NULL)) r[0] = r[1] = 0;
    if (r[1] > r[0]) it->layers |= (uint8_t)layer_of(&b[i], alpha);
  }
  if (!it->layers) {
    nrows -= m->nbatch;
    return;
  }
  nitems++;
}

void r_shadow(const Vec3f p, const Vec3f n, float r, uint8_t alpha, bool square) {
  if (!F || nitems >= R_MAX_ITEMS || !alpha) return;
  RItem *it = &F->items[nitems];
  Mtx43 m;
  mtx_align_terrain(&m, n, p, 0);
  mtx_mul(&it->mv, &m, &view);
  int16_t lo[3] = {(int16_t)-r, 0, (int16_t)-r}, hi[3] = {(int16_t)r, 1, (int16_t)r};
  uint8_t y0, y1;
  float depth;
  if (!box_rows(&it->mv, lo, hi, &y0, &y1, &depth)) return;
  it->kind = I_SHADOW, it->mesh = NULL, it->alpha = alpha, it->depth = depth, it->y0 = y0, it->y1 = y1;
  it->layers = 4, it->flags = 0;
  it->tint = (uint32_t)r | (square ? 0x80000000u : 0);
  nitems++;
}

/* ------------------------------------------------------------------------------------------------ rasterizing */
/* projected */
typedef struct {
  float sx, sy, iz;    /* screen, 1/z */
  float uz, vz;        /* u/z, v/z */
  float r, g, b, a, fog;
} PV;

/* the texture and the way the triangle is drawn */
static const uint8_t *tx_texels;
static const uint16_t *tx_pal;
static int tx_wl, tx_wm, tx_hm, tx_fmt, tx_depth;
static uint8_t tx_wrap, d_mode;
static bool tx_blend;   /* (W_BLEND) */
static uint32_t d_tint;
static uint8_t d_alpha;

static inline int wrap_coord(int c, int mask, int mode) {
  if (mode == W_WRAP) return c & mask;
  if (mode == W_CLAMP) return c < 0 ? 0 : c > mask ? mask : c;
  if (c & (mask + 1)) c = ~c;
  return c & mask;
}

static inline uint16_t mod565(uint16_t t, int r, int g, int b) {   /* r, g, b: 1..256 */
  int tr = t >> 11, tg = (t >> 5) & 63, tb = t & 31;
  tr = tr * r >> 8, tg = tg * g >> 8, tb = tb * b >> 8;
  return (uint16_t)(tr << 11 | tg << 5 | tb);
}

static inline int32_t fix16(float f) {
  if (f > 32000.0f) f = 32000.0f;
  if (f < -32000.0f) f = -32000.0f;
  return (int32_t)(f * 65536.0f);
}

/* one row of a triangle, from x0 to x1 (exclusive), the attributes at x0 and their steps per pixel; depth:
 * the texture's texel size (0: no texture, 1: 4 bits, 2: 8 bits, 3: 16 bits) */
static inline __attribute__((always_inline)) void span_body(int y, int x0, int x1, const PV *p, const PV *d,
                                                            const int depth) {
  uint16_t *cb = F->col + (y - r_band_y) * r_w, *zb = F->z + (y - r_band_y) * r_w;
  float iz = p->iz, uz = p->uz, vz = p->vz;
  /* (the depth at both ends of the row, kept in range: past the triangle's corners it can run away) */
  float zs = iz * z_k, ze = zs + d->iz * z_k * (float)(x1 - x0);
  if (zs > 30000.0f) zs = 30000.0f;
  if (zs < 0) zs = 0;
  if (ze > 30000.0f) ze = 30000.0f;
  if (ze < 0) ze = 0;
  int32_t z = (int32_t)(zs * 65536.0f), dz = (int32_t)((ze - zs) * 65536.0f / (float)(x1 - x0));
  int32_t r = fix16(p->r), g = fix16(p->g), b = fix16(p->b), dr = fix16(d->r), dg = fix16(d->g), db = fix16(d->b);
  int32_t a = fix16(p->a), da = fix16(d->a), f = fix16(p->fog), df = fix16(d->fog);
  const uint8_t mode = d_mode;
  const int wm = tx_wm, hm = tx_hm, wl = tx_wl, ws = tx_wrap & 3, wt = tx_wrap >> 2 & 3;
  const int tr = d_tint >> 16 & 255, tg = d_tint >> 8 & 255, tb = d_tint & 255;
  const bool tinted = d_tint != 0xFFFFFF, fog = fog_on && (mode & M_FOG), ia = tx_fmt == TF_IA;
  const bool key = tx_fmt == TF_565K, blend = tx_blend;
  const int bias = (mode & M_DECAL) ? DECAL_BIAS : 0;
  const uint8_t *texels = tx_texels;
  const uint16_t *pal = tx_pal;
  for (int x = x0; x < x1;) {
    int n = x1 - x > 8 ? 8 : x1 - x;
    int32_t u = 0, v = 0, du = 0, dv = 0;
    if (depth) {
      float izc = iz > 1e-7f ? iz : 1e-7f;
      float z0 = 1 / izc, u0 = uz * z0, v0 = vz * z0;
      float iz1 = iz + d->iz * n;
      if (iz1 < 1e-7f) iz1 = 1e-7f;
      float z1 = 1 / iz1, u1 = (uz + d->uz * n) * z1, v1 = (vz + d->vz * n) * z1;
      u = fix16(u0), v = fix16(v0);
      du = (fix16(u1) - u) / n, dv = (fix16(v1) - v) / n;
    }
    iz += d->iz * n, uz += d->uz * n, vz += d->vz * n;
    for (int k = 0; k < n; k++, x++, z += dz, r += dr, g += dg, b += db, a += da, f += df, u += du, v += dv) {
      int zz = z >> 16;
      if (zz + bias < zb[x]) continue;
      int cr = r >> 16, cg = g >> 16, cbl = b >> 16, ca = d_alpha;
      uint16_t out;
      if (depth) {
        int i = wrap_coord(v >> 16, hm, wt) << wl | wrap_coord(u >> 16, wm, ws);
        uint16_t t = depth == 1 ? pal[texels[i >> 1] >> ((i & 1) << 2) & 15]
                   : depth == 2 ? pal[texels[i]] : ((const uint16_t *)(const void *)texels)[i];
        if (ia) {
          int in = t >> 8, ta = t & 255;
          if (mode & M_CUTOUT) {
            if (ta < 128) continue;
          } else ca = ca * ta >> 8;
          out = (mode & M_SHADE) ? rgb565(cr * in >> 8, cg * in >> 8, cbl * in >> 8) : rgb565(in, in, in);
        } else {
          if (key && t == TEX_KEY) {
            if (!blend) continue;
            out = rgb565(cr > 255 ? 255 : cr < 0 ? 0 : cr, cg > 255 ? 255 : cg < 0 ? 0 : cg,
                         cbl > 255 ? 255 : cbl < 0 ? 0 : cbl);
          } else out = (mode & M_SHADE) ? mod565(t, cr + 1, cg + 1, cbl + 1) : t;
        }
      } else out = rgb565(cr > 255 ? 255 : cr < 0 ? 0 : cr, cg > 255 ? 255 : cg < 0 ? 0 : cg,
                          cbl > 255 ? 255 : cbl < 0 ? 0 : cbl);
      if (tinted) out = mod565(out, tr + 1, tg + 1, tb + 1);
      if (fog) {
        int ff = f >> 16;
        if (ff > 0) out = blend565(fog_col, out, ff > 255 ? 256 : ff);
      }
      if (mode & M_XLU) {
        ca = ca * ((a >> 16) + 1) >> 8;
        if (ca <= 0) continue;
        out = blend565(out, cb[x], ca >= 255 ? 256 : ca);
      } else zb[x] = (uint16_t)zz;
      cb[x] = out;
    }
  }
}

static void span_flat(int y, int x0, int x1, const PV *p, const PV *d) { span_body(y, x0, x1, p, d, 0); }
static void span_t4(int y, int x0, int x1, const PV *p, const PV *d) { span_body(y, x0, x1, p, d, 1); }
static void span_t8(int y, int x0, int x1, const PV *p, const PV *d) { span_body(y, x0, x1, p, d, 2); }
static void span_t16(int y, int x0, int x1, const PV *p, const PV *d) { span_body(y, x0, x1, p, d, 3); }
static void (*span)(int, int, int, const PV *, const PV *) = span_flat;

/* the common case, made apart (most of the frame's pixels): an opaque texture that wraps, maybe times the
 * vertex colours, maybe fogged, not a decal. The texel coordinates are right every 8 pixels, on a line between
 * (persp), or on a line all along (a triangle whose depth varies little: p and d's uz and vz are then the texel
 * coordinates themselves); the colour and fog are the 8 pixels' middle's; s and t go together in a word (8.8
 * each: the textures are 64 texels at most) */
#define SPREAD(c) (((uint32_t)(c) | (uint32_t)(c) << 16) & 0x07E0F81Fu)   /* (r, g, b apart in a word) */
static inline __attribute__((always_inline)) void span_fast_body(int y, int x0, int x1, const PV *p, const PV *d,
                                                                 const int depth, const bool shade, const bool fog,
                                                                 const bool persp) {
  uint16_t *cb = F->col + (y - r_band_y) * r_w + x0, *zb = F->z + (y - r_band_y) * r_w + x0;
  float iz = p->iz, uz = p->uz, vz = p->vz;
  float zs = iz * z_k, ze = zs + d->iz * z_k * (float)(x1 - x0);
  if (zs > 30000.0f) zs = 30000.0f;
  if (zs < 0) zs = 0;
  if (ze > 30000.0f) ze = 30000.0f;
  if (ze < 0) ze = 0;
  int32_t z = (int32_t)(zs * 65536.0f), dz = (int32_t)((ze - zs) * 65536.0f / (float)(x1 - x0));
  const uint32_t wm = (uint32_t)tx_wm, vsh = 8 - (uint32_t)tx_wl, vmask = (uint32_t)(tx_hm << tx_wl);
  const uint8_t *texels = tx_texels;
  const uint16_t *pal = tx_pal;
  const int fr = fog_col >> 11, fg = (fog_col >> 5) & 63, fb = fog_col & 31;
  int32_t u, v;
  uint32_t uv = 0, duv = 0;
  if (persp) {
    float z0 = 1 / (iz > 1e-7f ? iz : 1e-7f);
    u = fix16(uz * z0), v = fix16(vz * z0);
  } else {
    u = fix16(uz), v = fix16(vz);
    uv = ((uint32_t)(u >> 8) << 16) + (uint32_t)(v >> 8);
    duv = ((uint32_t)(fix16(d->uz) >> 8) << 16) + (uint32_t)(fix16(d->vz) >> 8);
  }
  float cr = p->r, cg = p->g, cbl = p->b, cf = p->fog;
#define TEXEL()                                                                                               \
  (depth == 1 ? pal[texels[i >> 1] >> ((i & 1) << 2) & 15]                                                    \
   : depth == 2 ? pal[texels[i]] : ((const uint16_t *)(const void *)texels)[i])
#define PIXELS(OUT)                                                                                           \
  for (const uint16_t *end = cb + n; cb < end; cb++, zb++, z += dz, uv += duv) {                              \
    int zz = z >> 16;                                                                                         \
    if (zz < *zb) continue;                                                                                   \
    uint32_t i = ((uv >> 24) & wm) | ((uv >> vsh) & vmask);                                                   \
    uint32_t t = TEXEL();                                                                                     \
    *zb = (uint16_t)zz;                                                                                       \
    *cb = (uint16_t)(OUT);                                                                                    \
  }
  for (int x = x0; x < x1;) {
    int n = x1 - x > 8 ? 8 : x1 - x;
    int32_t u1 = 0, v1 = 0;
    if (persp) {
      iz += d->iz * n, uz += d->uz * n, vz += d->vz * n;
      float z1 = 1 / (iz > 1e-7f ? iz : 1e-7f);
      u1 = fix16(uz * z1), v1 = fix16(vz * z1);
      uv = ((uint32_t)(u >> 8) << 16) + (uint32_t)(v >> 8);
      duv = ((uint32_t)(((u1 - u) / n) >> 8) << 16) + (uint32_t)(((v1 - v) / n) >> 8);
    }
    float half = (float)n * 0.5f;
    /* the 8 pixels' colour (0..256 a channel) and fog (0..256) */
    int mr = 256, mg = 256, mb = 256, ff = 0;
    if (shade) {
      mr = (int)(cr + d->r * half) + 1, mg = (int)(cg + d->g * half) + 1, mb = (int)(cbl + d->b * half) + 1;
      mr = mr < 0 ? 0 : mr > 256 ? 256 : mr, mg = mg < 0 ? 0 : mg > 256 ? 256 : mg, mb = mb < 0 ? 0 : mb > 256 ? 256 : mb;
      cr += d->r * n, cg += d->g * n, cbl += d->b * n;
    }
    if (fog) {
      ff = (int)(cf + d->fog * half);
      ff = ff < 0 ? 0 : ff > 255 ? 256 : ff;
      cf += d->fog * n;
    }
    x += n;
    if (!shade && (!fog || ff == 0)) {
      PIXELS(t)
    } else if (mr - mg < 6 && mg - mr < 6 && mg - mb < 6 && mb - mg < 6) {
      /* (grey: the three channels at once, to 1/32) */
      uint32_t k = (uint32_t)(((mr + mg + mb) / 3 * (256 - ff)) >> 11), kf = (uint32_t)ff >> 3;
      if (k + kf > 32) k = 32 - kf;
      uint32_t fogw = SPREAD(fog_col) * kf;
      PIXELS(({
        uint32_t w = ((SPREAD(t) * k + fogw) >> 5) & 0x07E0F81Fu;
        w | w >> 16;
      }))
    } else {
      int kr = mr * (256 - ff) >> 8, kg = mg * (256 - ff) >> 8, kb = mb * (256 - ff) >> 8;
      int orr = fr * ff >> 8, org = fg * ff >> 8, orb = fb * ff >> 8;
      PIXELS((((int)(t >> 11) * kr >> 8) + orr) << 11 | ((((int)(t >> 5) & 63) * kg >> 8) + org) << 5 |
             (((int)(t & 31) * kb >> 8) + orb))
    }
    if (persp) u = u1, v = v1;
  }
#undef TEXEL
#undef PIXELS
}

#define FAST(dep, sh, fo)                                                                                   \
  static void span_fast_##dep##sh##fo(int y, int x0, int x1, const PV *p, const PV *d) {                   \
    span_fast_body(y, x0, x1, p, d, dep, sh, fo, true);                                                   \
  }                                                                                                         \
  static void span_aff_##dep##sh##fo(int y, int x0, int x1, const PV *p, const PV *d) {                    \
    span_fast_body(y, x0, x1, p, d, dep, sh, fo, false);                                                  \
  }
FAST(1, 0, 0) FAST(1, 1, 0) FAST(1, 0, 1) FAST(1, 1, 1)
#undef FAST
/* (the textures are 4 bits a texel: tools/pack.py) by affine or not, then shaded + 2 * fogged */
static void (*const fast_spans[2][4])(int, int, int, const PV *, const PV *) = {
    {span_aff_100, span_aff_110, span_aff_101, span_aff_111},
    {span_fast_100, span_fast_110, span_fast_101, span_fast_111},
};
/* (the span for triangles whose depth varies little, when the batch has one) */
static void (*span_aff)(int, int, int, const PV *, const PV *);

/* what the batch's spans take besides the depth and texel coordinates: the colours, alpha, fog */
static bool need_rgb, need_a, need_fog;

/* fills a triangle of projected vertices, the rows of the band only */
static void fill_tri(const PV *a, const PV *b, const PV *c) {
  /* sorted by row */
  if (b->sy < a->sy) { const PV *t = a; a = b; b = t; }
  if (c->sy < a->sy) { const PV *t = a; a = c; c = t; }
  if (c->sy < b->sy) { const PV *t = b; b = c; c = t; }
  float y0f = a->sy, y2f = c->sy;
  if (y2f - y0f < 1e-4f) return;
  int ys = (int)ceilf(y0f - 0.5f), ye = (int)ceilf(y2f - 0.5f);
  if (ys < r_band_y) ys = r_band_y;
  if (ye > r_band_y + r_band_h) ye = r_band_y + r_band_h;
  if (ys >= ye) return;
  void (*fn)(int, int, int, const PV *, const PV *) = span;
  PV ta, tb, tc;
  if (span_aff) {
    float lo = fminf(a->iz, fminf(b->iz, c->iz)), hi = fmaxf(a->iz, fmaxf(b->iz, c->iz));
    if (hi < lo * 1.06f) {
      /* (its depth varies little: the texel coordinates on lines over it, not divided by the depth) */
      ta = *a, tb = *b, tc = *c;
      ta.uz = a->uz / a->iz, ta.vz = a->vz / a->iz, tb.uz = b->uz / b->iz, tb.vz = b->vz / b->iz;
      tc.uz = c->uz / c->iz, tc.vz = c->vz / c->iz;
      a = &ta, b = &tb, c = &tc, fn = span_aff;
    }
  }
  /* the attributes' steps across a row and down a column, from the plane of the triangle */
  float dx1 = b->sx - a->sx, dy1 = b->sy - a->sy, dx2 = c->sx - a->sx, dy2 = c->sy - a->sy;
  float det = dx1 * dy2 - dx2 * dy1;
  if (det > -1e-6f && det < 1e-6f) return;
  float id = 1 / det;
#define GX(f) (((b->f - a->f) * dy2 - (c->f - a->f) * dy1) * id)
#define GY(f) (((c->f - a->f) * dx1 - (b->f - a->f) * dx2) * id)
  PV dx = {0, 0, GX(iz), GX(uz), GX(vz), 0, 0, 0, 0, 0};
  PV dy = {0, 0, GY(iz), GY(uz), GY(vz), 0, 0, 0, 0, 0};
  if (need_rgb) dx.r = GX(r), dx.g = GX(g), dx.b = GX(b), dy.r = GY(r), dy.g = GY(g), dy.b = GY(b);
  if (need_a) dx.a = GX(a), dy.a = GY(a);
  if (need_fog) dx.fog = GX(fog), dy.fog = GY(fog);
#undef GX
#undef GY
  float inv_ac = 1 / (c->sy - a->sy);
  float inv_ab = b->sy - a->sy > 1e-6f ? 1 / (b->sy - a->sy) : 0;
  float inv_bc = c->sy - b->sy > 1e-6f ? 1 / (c->sy - b->sy) : 0;
  for (int y = ys; y < ye; y++) {
    float yc = y + 0.5f;
    /* the long edge a-c, and a-b or b-c */
    float xl = a->sx + (c->sx - a->sx) * (yc - a->sy) * inv_ac;
    float xs = yc < b->sy ? a->sx + (b->sx - a->sx) * (yc - a->sy) * inv_ab
                          : b->sx + (c->sx - b->sx) * (yc - b->sy) * inv_bc;
    float xa = xl < xs ? xl : xs, xb = xl < xs ? xs : xl;
    if (xa < -1.0f) xa = -1.0f;
    if (xb > r_w + 1.0f) xb = r_w + 1.0f;
    int x0 = (int)ceilf(xa - 0.5f), x1 = (int)ceilf(xb - 0.5f);
    if (x0 < 0) x0 = 0;
    if (x1 > r_w) x1 = r_w;
    if (x0 >= x1) continue;
    /* the attributes at (x0 + 0.5, yc), from vertex a */
    float ox = x0 + 0.5f - a->sx, oy = yc - a->sy;
    PV p;
#define AT(f) (a->f + dx.f * ox + dy.f * oy)
    p.iz = AT(iz), p.uz = AT(uz), p.vz = AT(vz), p.r = a->r, p.g = a->g, p.b = a->b, p.a = a->a, p.fog = a->fog;
    if (need_rgb) p.r = AT(r), p.g = AT(g), p.b = AT(b);
    if (need_a) p.a = AT(a);
    if (need_fog) p.fog = AT(fog);
#undef AT
    fn(y, x0, x1, &p, &dx);
  }
}

/* a vertex on the screen, when it is in front of the near plane */
static inline void rv_project(RVert *c) {
  if (c->z < cam_near) return;
  float iz = 1 / c->z;
  c->iz = iz, c->sx = cam_cx + cam_f * c->x * iz, c->sy = cam_cy - cam_f * c->y * iz;
}

static inline void rv_pv(const RVert *v, PV *p) {
  p->sx = v->sx, p->sy = v->sy, p->iz = v->iz, p->uz = v->u * v->iz, p->vz = v->v * v->iz;
  p->r = v->r, p->g = v->g, p->b = v->b, p->a = v->a, p->fog = v->fog;
}

static void project(const float *c, PV *p) {   /* (x, y, z, u, v, r, g, b, a, fog) */
  float iz = 1 / c[2];
  p->sx = cam_cx + cam_f * c[0] * iz;
  p->sy = cam_cy - cam_f * c[1] * iz;
  p->iz = iz, p->uz = c[3] * iz, p->vz = c[4] * iz;
  p->r = c[5], p->g = c[6], p->b = c[7], p->a = c[8], p->fog = c[9];
}

static void unpack(const RVert *v, float *o) {
  o[0] = v->x, o[1] = v->y, o[2] = v->z, o[3] = v->u, o[4] = v->v;
  o[5] = v->r, o[6] = v->g, o[7] = v->b, o[8] = v->a, o[9] = v->fog;
}

/* a triangle in camera space: culled, clipped against the near plane, projected and filled */
static void draw_tri(const RVert *v0, const RVert *v1, const RVert *v2, bool cull) {
  int behind = (v0->z < cam_near) + (v1->z < cam_near) + (v2->z < cam_near);
  if (!behind) {
    /* (in front: the N64's back faces turn clockwise on its screen, here the other way: y goes down) */
    if (cull && (v1->sx - v0->sx) * (v2->sy - v0->sy) - (v2->sx - v0->sx) * (v1->sy - v0->sy) >= 0) return;
    float lo = fminf(v0->sy, fminf(v1->sy, v2->sy)), hi = fmaxf(v0->sy, fmaxf(v1->sy, v2->sy));
    if (hi < r_band_y - 1 || lo > r_band_y + r_band_h + 1) return;
    PV p[3];
    rv_pv(v0, &p[0]), rv_pv(v1, &p[1]), rv_pv(v2, &p[2]);
    fill_tri(&p[0], &p[1], &p[2]);
    return;
  }
  if (cull) {
    /* the N64 culls back faces: those whose corners turn clockwise on the screen */
    float e1x = v1->x - v0->x, e1y = v1->y - v0->y, e1z = v1->z - v0->z;
    float e2x = v2->x - v0->x, e2y = v2->y - v0->y, e2z = v2->z - v0->z;
    float nx = e1y * e2z - e1z * e2y, ny = e1z * e2x - e1x * e2z, nz = e1x * e2y - e1y * e2x;
    if (nx * v0->x + ny * v0->y + nz * v0->z <= 0) return;
  }
  if (behind == 3) return;
  float in[3][10];
  unpack(v0, in[0]), unpack(v1, in[1]), unpack(v2, in[2]);
  PV p[4];
  float out[4][10];
  int n = 0;
  for (int i = 0; i < 3; i++) {
    const float *a = in[i], *b = in[(i + 1) % 3];
    bool ain = a[2] >= cam_near, bin = b[2] >= cam_near;
    if (ain) memcpy(out[n++], a, sizeof out[0]);
    if (ain != bin) {
      float t = (cam_near - a[2]) / (b[2] - a[2]);
      for (int k = 0; k < 10; k++) out[n][k] = a[k] + (b[k] - a[k]) * t;
      n++;
    }
  }
  for (int i = 0; i < n; i++) project(out[i], &p[i]);
  for (int i = 1; i + 1 < n; i++) fill_tri(&p[0], &p[i], &p[i + 1]);
}

static float fog_of(float z) {
  if (!fog_on) return 0;
  float f = (z - fog_near) * fog_k;
  return f <= 0 ? 0 : f >= 255 ? 255 : f;
}

static void use_texture(const Batch *b) {
  span = span_flat, span_aff = NULL;
  if (b->mode & M_TEX) {
    const Tex *t = TEX(b->tex);
    const uint8_t *base = DAT(uint8_t, t->off);
    int depth = t->fmt >> 4;
    tx_depth = depth;
    tx_wl = t->wl, tx_wm = (1 << t->wl) - 1, tx_hm = (1 << t->hl) - 1;
    tx_fmt = t->fmt & 15;
    tx_pal = (const uint16_t *)(const void *)base;
    if (depth == 1) tx_texels = base + 32, span = span_t4;
    else if (depth == 2) tx_texels = base + 512, span = span_t8;
    else tx_texels = base, span = span_t16;
  }
  tx_wrap = b->wrap & 15, tx_blend = b->wrap & W_BLEND;   /* (the rest of wrap is the vertices') */
}

/* the scene's light for lit meshes: from the upper left, in front of the camera, in model space */
static void model_light(const Mtx43 *mv, Vec3f l) {
  static const Vec3f cam_light = {-0.35f, 0.70f, -0.62f};
  for (int j = 0; j < 3; j++) {
    float s = 0, n2 = 0;
    for (int k = 0; k < 3; k++) s += mv->m[j][k] * cam_light[k], n2 += mv->m[j][k] * mv->m[j][k];
    l[j] = n2 > 0 ? s / sqrtf(n2) : 0;
  }
  v3_normalize(l);
}

static void draw_batch(const RItem *it, const Batch *b) {
  const Mesh *m = it->mesh;
  const MVert *mv = DAT(MVert, m->vtx) + b->v0;
  const uint8_t *tri = DAT(uint8_t, m->tri) + b->tri0 * 3;
  bool lit = b->mode & M_LIT, texgen = b->mode & M_TEXGEN;
  int nv = b->nv > R_MAX_VERTS ? R_MAX_VERTS : b->nv;
  const Mtx43 *M = &it->mv;
  const uint8_t *light = b->light;
  Vec3f L = {0, 0, 0};
  if (lit) model_light(M, L);
  use_texture(b);
  float tw = (float)(tx_wm + 1), th = (float)(tx_hm + 1);
  r_stat_verts += nv;
  RVert *vb = F->verts;
  for (int i = 0; i < nv; i++) {
    const MVert *s = &mv[i];
    RVert *c = &vb[i];
    float x = s->x, y = s->y, z = s->z;
    c->x = x * M->m[0][0] + y * M->m[1][0] + z * M->m[2][0] + M->m[3][0];
    c->y = x * M->m[0][1] + y * M->m[1][1] + z * M->m[2][1] + M->m[3][1];
    c->z = x * M->m[0][2] + y * M->m[1][2] + z * M->m[2][2] + M->m[3][2];
    c->u = s->s * (1.0f / 32), c->v = s->t * (1.0f / 32);
    if (lit) {
      /* (the normal, unfolded from its octahedron, 127 long) */
      float nx = (s->c & 255) * (2.0f / 255) - 1, ny = (s->c >> 8) * (2.0f / 255) - 1;
      float nz = 1 - fabsf(nx) - fabsf(ny);
      if (nz < 0) {
        float ox = nx;
        nx = (1 - fabsf(ny)) * (nx >= 0 ? 1 : -1), ny = (1 - fabsf(ox)) * (ny >= 0 ? 1 : -1);
      }
      float nl = 127 / sqrtf(nx * nx + ny * ny + nz * nz);
      nx *= nl, ny *= nl, nz *= nl;
      float d = (nx * L[0] + ny * L[1] + nz * L[2]) * (1.0f / 127);
      if (d < 0) d = 0;
      int cr = (int)(light[0] + light[3] * d), cg = (int)(light[1] + light[4] * d), cb = (int)(light[2] + light[5] * d);
      c->r = (uint8_t)(cr > 255 ? 255 : cr), c->g = (uint8_t)(cg > 255 ? 255 : cg), c->b = (uint8_t)(cb > 255 ? 255 : cb);
      if (texgen) {
        /* the normal, in camera space, picks the texel (the N64's G_TEXTURE_GEN) */
        float ex = nx * M->m[0][0] + ny * M->m[1][0] + nz * M->m[2][0];
        float ey = nx * M->m[0][1] + ny * M->m[1][1] + nz * M->m[2][1];
        float el = sqrtf(ex * ex + ey * ey) + 127;
        c->u = (ex / el * 0.5f + 0.5f) * tw, c->v = (-ey / el * 0.5f + 0.5f) * th;
      }
      c->a = b->alpha;
    } else if (b->wrap & W_VALPHA) {
      c->r = (uint8_t)((s->c >> 12) * 17), c->g = (uint8_t)((s->c >> 8 & 15) * 17), c->b = (uint8_t)((s->c >> 4 & 15) * 17);
      c->a = (uint8_t)((s->c & 15) * 17);
    } else {
      c->r = (uint8_t)((s->c >> 11) * 255 / 31), c->g = (uint8_t)((s->c >> 5 & 63) * 255 / 63);
      c->b = (uint8_t)((s->c & 31) * 255 / 31), c->a = b->alpha;
    }
    c->fog = (uint8_t)fog_of(c->z);
    rv_project(c);
  }
  d_mode = b->mode;
  if (it->alpha < 255) d_mode |= M_XLU;
  d_tint = it->tint, d_alpha = it->alpha;
  if ((b->mode & M_TEX) && !(d_mode & (M_XLU | M_CUTOUT | M_DECAL)) && tx_fmt == TF_565 && tx_wrap == 0 &&
      d_tint == 0xFFFFFF && tx_depth == 1)
  {
    int k = ((d_mode & M_SHADE) ? 1 : 0) | ((fog_on && (d_mode & M_FOG)) ? 2 : 0);
    span = fast_spans[1][k], span_aff = fast_spans[0][k];
  }
  need_rgb = !(d_mode & M_TEX) || (d_mode & M_SHADE) || tx_blend, need_a = d_mode & M_XLU, need_fog = fog_on && (d_mode & M_FOG);
  bool cull = !(it->flags & R_NOCULL);
  for (int i = 0; i < b->ntri; i++, tri += 3) {
    if (tri[0] >= nv || tri[1] >= nv || tri[2] >= nv) continue;
    draw_tri(&vb[tri[0]], &vb[tri[1]], &vb[tri[2]], cull);
    r_stat_tris++;
  }
}

static void draw_shadow(const RItem *it) {
  /* a disc (or square) on the ground: 12 triangles around its centre, dark and see-through */
  float rad = (float)(it->tint & 0xFFFF);
  bool square = it->tint & 0x80000000u;
  RVert c[13];
  int n = square ? 4 : 12;
  const Mtx43 *M = &it->mv;
  for (int i = 0; i <= n; i++) {
    float lx = 0, lz = 0;
    if (i) {
      float an = (i - 1) * (2 * M_PI_F / n) + (square ? M_PI_F / 4 : 0);
      float rr = square ? rad * 1.4142f : rad;
      lx = cosf(an) * rr, lz = sinf(an) * rr;
    }
    c[i].x = lx * M->m[0][0] + 2 * M->m[1][0] + lz * M->m[2][0] + M->m[3][0];
    c[i].y = lx * M->m[0][1] + 2 * M->m[1][1] + lz * M->m[2][1] + M->m[3][1];
    c[i].z = lx * M->m[0][2] + 2 * M->m[1][2] + lz * M->m[2][2] + M->m[3][2];
    c[i].u = c[i].v = 0, c[i].r = c[i].g = c[i].b = 0, c[i].fog = 0;
    c[i].a = i ? (square ? 255 : 110) : 255;
    rv_project(&c[i]);
  }
  need_rgb = need_a = true, need_fog = false;
  d_mode = M_XLU | M_DECAL, d_tint = 0xFFFFFF, d_alpha = it->alpha;
  span = span_flat, span_aff = NULL;
  for (int i = 1; i <= n; i++) draw_tri(&c[0], &c[i], &c[i % n + 1], false);
}

static void draw_item(const RItem *it, int layer) {
  if (it->kind == I_MESH) {
    const Batch *b = DAT(Batch, it->mesh->batch);
    for (int i = 0; i < it->mesh->nbatch; i++) {
      const uint8_t *rows = F->rows[it->batch0 + i];
      if (rows[1] <= r_band_y || rows[0] >= r_band_y + r_band_h) continue;
      if (layer_of(&b[i], it->alpha) != layer) continue;
      draw_batch(it, &b[i]);
    }
  } else if (it->kind == I_SHADOW) draw_shadow(it);
}

/* ------------------------------------------------------------------------------------------------ the sky */
/* each background's colours: the top of the sky, the horizon, and below it */
static const uint32_t skies[10][3] = {
  {0x2858D8, 0x98C8F8, 0x6890C8},   /* ocean sky: Bob-omb Battlefield, the castle */
  {0x480800, 0xE05010, 0x601000},   /* flaming sky: Lethal Lava Land, Bowser in the Fire Sea */
  {0x081848, 0x3870B8, 0x102850},   /* underwater city: Dire, Dire Docks */
  {0x5888D8, 0xD8E8F8, 0xF0F0F8},   /* below clouds: Tall, Tall Mountain */
  {0x4878C8, 0xD0E0F0, 0xE8F0F8},   /* snow mountains: Cool, Cool Mountain, Snowman's Land */
  {0xC88838, 0xF8D890, 0xD0A060},   /* desert: Shifting Sand Land */
  {0x080810, 0x283040, 0x101018},   /* haunted: Big Boo's Haunt */
  {0x184818, 0x80C080, 0x305830},   /* green sky: Wet-Dry World */
  {0x3060C0, 0xC0D8F8, 0xF8F8F8},   /* above clouds: Rainbow Ride, Wing Mario over the Rainbow */
  {0x281048, 0x9060B8, 0x381858},   /* purple sky: Bowser in the Dark World and the Sky */
};

static void draw_sky(void) {
  if (sky_kind & 0x8000) {
    /* a plain colour (RGBA5551) */
    int c5 = sky_kind & 0x7FFF;
    uint16_t c = rgb565((c5 >> 11 & 31) << 3, (c5 >> 6 & 31) << 3, (c5 >> 1 & 31) << 3);
    for (int i = 0; i < r_w * r_band_h; i++) F->col[i] = c;
    return;
  }
  const uint32_t *k = skies[sky_kind < 0 || sky_kind > 9 ? 0 : sky_kind];
  /* the horizon's row, from the camera's pitch */
  float hz = cam_cy + cam_f * tanf(sky_pitch);
  for (int y = r_band_y; y < r_band_y + r_band_h; y++) {
    float t = (y - (hz - r_h)) / (float)r_h;
    uint32_t c0 = k[0], c1 = k[1];
    if (t > 1) c0 = k[1], c1 = k[2], t = (t - 1) * 4;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int r = (int)((c0 >> 16 & 255) + (((int)(c1 >> 16 & 255) - (int)(c0 >> 16 & 255)) * t));
    int g = (int)((c0 >> 8 & 255) + (((int)(c1 >> 8 & 255) - (int)(c0 >> 8 & 255)) * t));
    int b = (int)((c0 & 255) + (((int)(c1 & 255) - (int)(c0 & 255)) * t));
    uint16_t c = rgb565(r, g, b);
    uint16_t *row = F->col + (y - r_band_y) * r_w;
    for (int x = 0; x < r_w; x++) row[x] = c;
  }
  (void)sky_yaw;
}


/* ------------------------------------------------------------------------------------------------ overlays */
typedef struct { r_overlay_fn fn; uint16_t size; } OvHead;

void *r_overlay_alloc(r_overlay_fn fn, int n) {
  int need = (int)((sizeof(OvHead) + (unsigned)n + 3) & ~3u);
  if (!F || overlay_used + need > R_OVERLAY_BYTES) return NULL;
  OvHead *h = (OvHead *)(void *)(F->overlay + overlay_used);
  h->fn = fn, h->size = (uint16_t)need;
  overlay_used += need;
  return h + 1;
}

void r_hline(int x0, int x1, int y, uint16_t c, uint8_t alpha) {
  if (y < r_band_y || y >= r_band_y + r_band_h) return;
  if (x0 < 0) x0 = 0;
  if (x1 > 320) x1 = 320;
  uint16_t *row = r_row(y);
  for (int x = x0; x < x1; x++) row[x] = alpha == 255 ? c : blend565(c, row[x], alpha + 1);
}

typedef struct { int16_t x, y, w, h; uint16_t c; uint8_t a; } RectCmd;
static void rect_fn(const void *cmd) {
  const RectCmd *r = cmd;
  int y0 = r->y > r_band_y ? r->y : r_band_y, y1 = r->y + r->h < r_band_y + r_band_h ? r->y + r->h : r_band_y + r_band_h;
  for (int y = y0; y < y1; y++) r_hline(r->x, r->x + r->w, y, r->c, r->a);
}
void r_rect(int x, int y, int w, int h, uint16_t color, uint8_t alpha) {
  RectCmd *r = r_overlay_alloc(rect_fn, sizeof(RectCmd));
  if (r) *r = (RectCmd){(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, color, alpha};
}

static void run_overlays(uint16_t *buf, int y, int h) {
  r_band = buf, r_band_y = y, r_band_h = h;
  for (int off = 0; off < overlay_used;) {
    const OvHead *hd = (const OvHead *)(const void *)(F->overlay + off);
    hd->fn(hd + 1);
    off += hd->size;
  }
}

/* ------------------------------------------------------------------------------------------------ the bands */
static int by_depth(const void *a, const void *b) {
  float da = F->items[*(const uint8_t *)a].depth, db = F->items[*(const uint8_t *)b].depth;
  return da < db ? 1 : da > db ? -1 : 0;
}

void r_draw(void) {
  if (!F) return;
  int nxlu = 0;
  for (int i = 0; i < nitems; i++)
    if (F->items[i].layers & 4) F->order[nxlu++] = (uint8_t)i;
  qsort(F->order, (size_t)nxlu, 1, by_depth);
  int band_rows = R_BAND_PIXELS / r_w;
  r_stat_items = nitems;
  for (int band_y = 0; band_y < r_h; band_y += band_rows) {
    r_band = F->col;
    r_band_y = band_y;
    r_band_h = r_h - band_y < band_rows ? r_h - band_y : band_rows;
    int bh = r_band_h;
    memset(F->z, 0, (size_t)r_w * bh * 2);
    draw_sky();
    for (int layer = 1; layer <= 2; layer++)
      for (int i = 0; i < nitems; i++) {
        const RItem *it = &F->items[i];
        if (!(it->layers & layer) || it->y1 <= band_y || it->y0 >= band_y + bh) continue;
        draw_item(it, layer);
      }
    for (int k = 0; k < nxlu; k++) {
      const RItem *it = &F->items[F->order[k]];
      if (it->y1 <= band_y || it->y0 >= band_y + bh) continue;
      draw_item(it, 4);
    }
    if (r_scale == 1) {
      run_overlays(F->col, band_y, bh);
      plat_push(0, band_y, r_w, bh, F->col);
    } else {
      /* half resolution: rows doubled into the depth buffer's room (free now), then the overlays */
      int chunk = R_BAND_PIXELS / (320 * 2);
      for (int y = 0; y < bh; y += chunk) {
        int n = bh - y < chunk ? bh - y : chunk;
        for (int k = 0; k < n; k++) {
          const uint16_t *src = F->col + (y + k) * r_w;
          uint16_t *d = F->z + k * 2 * 320;
          for (int x = 0; x < r_w; x++) d[2 * x] = d[2 * x + 1] = src[x];
          memcpy(d + 320, d, 640);
        }
        run_overlays(F->z, (band_y + y) * 2, n * 2);
        plat_push(0, (band_y + y) * 2, 320, n * 2, F->z);
      }
    }
  }
  F = NULL;
}
