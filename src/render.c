/* The 3D renderer (render.h): a band of rows at a time, each with its colours and a 15-bit depth buffer
 * (the inverse of the distance, so near things are precise). Triangles are clipped against the near plane in
 * camera space, then filled a row at a time: depth, colours and fog step linearly, texture coordinates are
 * corrected for perspective every 8 pixels. -O2: this runs for every pixel. */
#include <string.h>
#include "plat.h"
#include "render.h"

#define BAND_PIXELS (320 * 40)
#define MAX_ITEMS 192
#define MAX_BATCHES 768
#define MAX_VERTS 256
#define MAX_OVERLAYS 24
#define NEAR_MIN 10.0f
#define DECAL_BIAS 24

int r_w = 320, r_h = 240, r_scale = 1;
uint16_t *r_band;
int r_band_y, r_band_h;
int r_stat_tris, r_stat_items;

static uint16_t col_buf[BAND_PIXELS], z_buf[BAND_PIXELS];

/* the camera */
static Vec3f cam_eye, cam_right, cam_up, cam_fwd;
static float cam_f, cam_cx, cam_cy, cam_near, cam_far, z_k;
static Mtx43 view;   /* world to camera: x right, y up, z forward */

/* the sky and the fog */
static int sky_kind = -1;
static float sky_pitch;   /* the camera's pitch, for the horizon */
static uint16_t fog_col;
static float fog_near, fog_far;
static bool fog_on;

/* what the frame draws */
enum { I_MESH, I_BILLBOARD, I_SHADOW };
typedef struct {
  uint8_t kind, alpha, layers;   /* layers: bit 0 opaque, 1 cutout/decal, 2 transparent */
  uint8_t flags;
  int16_t y0, y1;                /* rows it may cover */
  uint16_t batch0;               /* its batches' rows in item_rows */
  const Mesh *mesh;
  Mtx43 mv;                      /* model to camera */
  Vec3f light;                   /* the light, in model space (lit meshes) */
  uint32_t tint;
  float depth;
  int tex;
} Item;
static Item items[MAX_ITEMS];
static int nitems;
static int16_t item_rows[MAX_BATCHES][2];
static int nrows;

typedef struct { r_overlay_fn fn; void *ctx; } Overlay;
static Overlay overlays[MAX_OVERLAYS];
static int noverlays;

/* ------------------------------------------------------------------------------------------------ the frame */
void r_frame(const Vec3f eye, const Vec3f at, float roll, float fov_deg, float near, float far) {
  Vec3f f = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
  vec3f_normalize(f);
  Vec3f up = {sinf(roll) * f[2], cosf(roll), -sinf(roll) * f[0]};
  Vec3f r;
  vec3f_cross(r, up, f);   /* (x right, as seen) */
  if (vec3f_len(r) < 1e-4f) vec3f_set(r, 1, 0, 0);
  vec3f_normalize(r);
  vec3f_cross(up, f, r);
  vec3f_copy(cam_eye, eye), vec3f_copy(cam_right, r), vec3f_copy(cam_up, up), vec3f_copy(cam_fwd, f);
  /* view: v_cam = (v - eye) . (r, up, f) */
  for (int j = 0; j < 3; j++) view.m[j][0] = r[j], view.m[j][1] = up[j], view.m[j][2] = f[j];
  for (int j = 0; j < 3; j++) view.m[3][j] = -(eye[0] * view.m[0][j] + eye[1] * view.m[1][j] + eye[2] * view.m[2][j]);
  cam_f = (r_h * 0.5f) / tanf(fov_deg * (M_PI_F / 360.0f));
  cam_cx = r_w * 0.5f, cam_cy = r_h * 0.5f;
  cam_near = near < NEAR_MIN ? NEAR_MIN : near, cam_far = far;
  z_k = 32767.0f * cam_near;
  sky_pitch = asinf(f[1] > 1 ? 1 : f[1] < -1 ? -1 : f[1]);
  nitems = nrows = noverlays = 0;
  fog_on = false;
  r_stat_tris = 0;
}

void r_sky(int sky) { sky_kind = sky; }
void r_fog(uint16_t color, float near, float far) {
  fog_on = far > near && far > 0;
  fog_col = color, fog_near = near, fog_far = far;
}

bool r_project(const Vec3f p, float *sx, float *sy, float *depth) {
  Vec3f c;
  mtx_apply(&view, p, c);
  if (c[2] < cam_near) return false;
  float iz = 1 / c[2];
  *sx = cam_cx + cam_f * c[0] * iz, *sy = cam_cy - cam_f * c[1] * iz;
  if (depth) *depth = c[2];
  return true;
}

/* the rows a box (model space, under mv) covers; false when it is out of sight */
static bool box_rows(const Mtx43 *mv, const int16_t lo[3], const int16_t hi[3], int16_t *y0, int16_t *y1, float *depth) {
  float ymin = 1e9f, ymax = -1e9f, zmin = 1e9f;
  bool behind = false, left = true, right = true, above = true, below = true, far = true;
  for (int i = 0; i < 8; i++) {
    Vec3f p = {(float)(i & 1 ? hi[0] : lo[0]), (float)(i & 2 ? hi[1] : lo[1]), (float)(i & 4 ? hi[2] : lo[2])}, c;
    mtx_apply(mv, p, c);
    if (c[2] < zmin) zmin = c[2];
    if (c[2] < cam_far) far = false;
    /* (against the frustum's side planes, with a little room) */
    float lim = c[2] * (cam_cx + 2) / cam_f, limy = c[2] * (cam_cy + 2) / cam_f;
    if (c[0] > -lim) left = false;
    if (c[0] < lim) right = false;
    if (c[1] < limy) above = false;
    if (c[1] > -limy) below = false;
    if (c[2] < cam_near) {
      behind = true;
      continue;
    }
    float sy = cam_cy - cam_f * c[1] / c[2];
    if (sy < ymin) ymin = sy;
    if (sy > ymax) ymax = sy;
  }
  if (far || left || right || above || below) return false;
  if (zmin < cam_near && !behind) return false;
  if (behind) ymin = 0, ymax = (float)r_h;
  if (ymax < 0 || ymin >= r_h) return false;
  *y0 = (int16_t)(ymin < 0 ? 0 : ymin);
  *y1 = (int16_t)(ymax >= r_h ? r_h : ymax + 1);
  if (depth) *depth = zmin;
  return true;
}

void r_mesh(const Mesh *m, const Mtx43 *mtx, uint32_t tint, uint8_t alpha) {
  if (nitems >= MAX_ITEMS || !m || !alpha) return;
  Item *it = &items[nitems];
  if (mtx) mtx_mul(&it->mv, mtx, &view);
  else it->mv = view;
  int16_t y0, y1;
  float depth;
  if (!box_rows(&it->mv, m->lo, m->hi, &y0, &y1, &depth)) return;
  if (nrows + m->nbatch > MAX_BATCHES) return;
  it->kind = I_MESH, it->mesh = m, it->tint = tint, it->alpha = alpha, it->depth = depth;
  it->y0 = y0, it->y1 = y1, it->batch0 = (uint16_t)nrows;
  it->layers = 0;
  const Batch *b = DAT(Batch, m->batch);
  for (int i = 0; i < m->nbatch; i++) {
    int16_t *r = item_rows[nrows++];
    if (m->nbatch == 1) r[0] = y0, r[1] = y1;
    else if (!box_rows(&it->mv, b[i].lo, b[i].hi, &r[0], &r[1], NULL)) r[0] = r[1] = 0;
    if (r[1] > r[0]) it->layers |= alpha < 255 || (b[i].mode & M_XLU) ? 4 : (b[i].mode & (M_CUTOUT | M_DECAL)) ? 2 : 1;
  }
  if (!it->layers) {
    nrows -= m->nbatch;
    return;
  }
  /* the light in model space: the camera's light (from the upper left, in front), back through mv */
  static const Vec3f cam_light = {-0.35f, 0.70f, -0.62f};
  for (int j = 0; j < 3; j++) {
    float l = 0, n2 = 0;
    for (int k = 0; k < 3; k++) l += it->mv.m[j][k] * cam_light[k], n2 += it->mv.m[j][k] * it->mv.m[j][k];
    it->light[j] = n2 > 0 ? l / sqrtf(n2) : 0;
  }
  vec3f_normalize(it->light);
  nitems++;
}

void r_billboard(const Vec3f p, float w, float h, int tex, uint32_t tint, uint8_t alpha, bool flipx) {
  if (nitems >= MAX_ITEMS) return;
  Item *it = &items[nitems];
  Vec3f c;
  mtx_apply(&view, p, c);
  if (c[2] < cam_near || c[2] > cam_far) return;
  float sx = cam_cx + cam_f * c[0] / c[2], sy = cam_cy - cam_f * c[1] / c[2];
  float hw = cam_f * w * 0.5f / c[2], hh = cam_f * h * 0.5f / c[2];
  if (sx + hw < 0 || sx - hw >= r_w || sy + hh < 0 || sy - hh >= r_h) return;
  it->kind = I_BILLBOARD, it->tex = tex, it->tint = tint, it->alpha = alpha, it->depth = c[2];
  it->mv.m[0][0] = sx, it->mv.m[0][1] = sy, it->mv.m[0][2] = c[2];
  it->mv.m[1][0] = hw, it->mv.m[1][1] = hh, it->mv.m[1][2] = flipx;
  it->y0 = (int16_t)(sy - hh < 0 ? 0 : sy - hh), it->y1 = (int16_t)(sy + hh + 1 > r_h ? r_h : sy + hh + 1);
  it->layers = alpha < 255 ? 4 : 2;
  nitems++;
}

void r_shadow(const Vec3f p, const Vec3f n, float r, uint8_t alpha, bool square) {
  if (nitems >= MAX_ITEMS || !alpha) return;
  Item *it = &items[nitems];
  Mtx43 m;
  mtx_align_terrain(&m, n, p, 0);
  mtx_mul(&it->mv, &m, &view);
  int16_t lo[3] = {(int16_t)-r, 0, (int16_t)-r}, hi[3] = {(int16_t)r, 1, (int16_t)r}, y0, y1;
  float depth;
  if (!box_rows(&it->mv, lo, hi, &y0, &y1, &depth)) return;
  it->kind = I_SHADOW, it->alpha = alpha, it->depth = depth, it->y0 = y0, it->y1 = y1, it->layers = 4;
  it->mv.m[3][0] += 0;   /* (the radius is kept in tint) */
  it->tint = (uint32_t)r | (square ? 0x80000000u : 0);
  nitems++;
}

void r_overlay(r_overlay_fn fn, void *ctx) {
  if (noverlays < MAX_OVERLAYS) overlays[noverlays++] = (Overlay){fn, ctx};
}

/* ------------------------------------------------------------------------------------------------ rasterizing */
/* a vertex in camera space with everything that is interpolated */
typedef struct {
  float x, y, z;
  float u, v;          /* texels */
  float r, g, b, a;    /* 0..255 */
  float fog;           /* 0..255 */
} CV;
/* projected */
typedef struct {
  float sx, sy, iz;    /* screen, 1/z */
  float uz, vz;        /* u/z, v/z */
  float r, g, b, a, fog;
} PV;

/* the texture and the way the triangle is drawn */
static const uint16_t *tx_px;
static int tx_wl, tx_wm, tx_hm, tx_fmt;
static uint8_t tx_wrap, d_mode;
static uint32_t d_tint;
static uint8_t d_alpha;
static bool d_zwrite;

static inline int wrap_coord(int c, int mask, int mode) {
  if (mode == W_WRAP) return c & mask;
  if (mode == W_CLAMP) return c < 0 ? 0 : c > mask ? mask : c;
  if (c & (mask + 1)) c = ~c;
  return c & mask;
}

static inline uint16_t mod565(uint16_t t, int r, int g, int b) {
  int tr = t >> 11, tg = (t >> 5) & 63, tb = t & 31;
  tr = tr * r >> 8, tg = tg * g >> 8, tb = tb * b >> 8;
  return (uint16_t)(tr << 11 | tg << 5 | tb);
}

static inline uint16_t blend565(uint16_t s, uint16_t d, int a) {   /* a: 0..256 */
  uint32_t ss = (s | (uint32_t)s << 16) & 0x07E0F81Fu, dd = (d | (uint32_t)d << 16) & 0x07E0F81Fu;
  uint32_t r = (dd + ((ss - dd) * (uint32_t)(a >> 3) >> 5)) & 0x07E0F81Fu;
  return (uint16_t)(r | r >> 16);
}

static inline uint16_t rgb565(int r, int g, int b) {
  return (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

/* one row of a triangle, from x0 to x1 (exclusive), the attributes at x0 and their steps per pixel */
static void span(int y, int x0, int x1, const PV *p, const PV *d) {
  if (x0 < 0) x0 = 0;
  if (x1 > r_w) x1 = r_w;
  if (x0 >= x1) return;
  uint16_t *cb = col_buf + (y - r_band_y) * r_w, *zb = z_buf + (y - r_band_y) * r_w;
  float iz = p->iz, uz = p->uz, vz = p->vz;
  int32_t z = (int32_t)(iz * z_k * 65536.0f), dz = (int32_t)(d->iz * z_k * 65536.0f);
  int32_t r = (int32_t)(p->r * 65536), g = (int32_t)(p->g * 65536), b = (int32_t)(p->b * 65536);
  int32_t dr = (int32_t)(d->r * 65536), dg = (int32_t)(d->g * 65536), db = (int32_t)(d->b * 65536);
  int32_t a = (int32_t)(p->a * 65536), da = (int32_t)(d->a * 65536);
  int32_t f = (int32_t)(p->fog * 65536), df = (int32_t)(d->fog * 65536);
  uint8_t mode = d_mode;
  bool tex = mode & M_TEX;
  int wm = tx_wm, hm = tx_hm, wl = tx_wl, ws = tx_wrap & 3, wt = tx_wrap >> 2 & 3;
  int tr = d_tint >> 16 & 255, tg = d_tint >> 8 & 255, tb = d_tint & 255;
  bool tinted = d_tint != 0xFFFFFF;
  for (int x = x0; x < x1;) {
    int n = x1 - x > 8 ? 8 : x1 - x;
    int32_t u = 0, v = 0, du = 0, dv = 0;
    if (tex) {
      float z0 = 1 / iz, u0 = uz * z0, v0 = vz * z0;
      float iz1 = iz + d->iz * n, z1 = 1 / iz1, u1 = (uz + d->uz * n) * z1, v1 = (vz + d->vz * n) * z1;
      u = (int32_t)(u0 * 65536), v = (int32_t)(v0 * 65536);
      du = (int32_t)((u1 - u0) * 65536) / n, dv = (int32_t)((v1 - v0) * 65536) / n;
    }
    iz += d->iz * n, uz += d->uz * n, vz += d->vz * n;
    for (int k = 0; k < n; k++, x++, z += dz, r += dr, g += dg, b += db, a += da, f += df, u += du, v += dv) {
      uint16_t zz = (uint16_t)(z >> 16);
      if (zz + ((mode & M_DECAL) ? DECAL_BIAS : 0) < zb[x]) continue;
      int cr = r >> 16, cg = g >> 16, cb2 = b >> 16, ca = d_alpha;
      uint16_t out;
      if (tex) {
        int tu = wrap_coord(u >> 16, wm, ws), tv = wrap_coord(v >> 16, hm, wt);
        uint16_t t = tx_px[tv << wl | tu];
        if (tx_fmt == TF_IA) {
          int in = t >> 8, ta = t & 255;
          if (mode & M_CUTOUT) {
            if (ta < 128) continue;
          } else ca = ca * ta >> 8;
          out = (mode & M_SHADE) ? rgb565(cr * in >> 8, cg * in >> 8, cb2 * in >> 8) : rgb565(in, in, in);
        } else {
          if (t == TEX_KEY && tx_fmt == TF_565K) {
            continue;
          }
          out = (mode & M_SHADE) ? mod565(t, cr + 1, cg + 1, cb2 + 1) : t;
        }
      } else out = rgb565(cr > 255 ? 255 : cr, cg > 255 ? 255 : cg, cb2 > 255 ? 255 : cb2);
      if (tinted) out = mod565(out, tr + 1, tg + 1, tb + 1);
      if (fog_on && (mode & M_FOG)) {
        int ff = f >> 16;
        if (ff > 0) out = blend565(fog_col, out, ff > 255 ? 256 : ff);
      }
      if (mode & M_XLU) {
        ca = ca * ((a >> 16) + 1) >> 8;
        if (ca <= 0) continue;
        out = blend565(out, cb[x], ca >= 255 ? 256 : ca);
      } else zb[x] = zz;
      cb[x] = out;
    }
  }
}

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
  /* the attributes' steps across a row, from the plane of the triangle */
  float dx1 = b->sx - a->sx, dy1 = b->sy - a->sy, dx2 = c->sx - a->sx, dy2 = c->sy - a->sy;
  float det = dx1 * dy2 - dx2 * dy1;
  if (det > -1e-6f && det < 1e-6f) return;
  float id = 1 / det;
#define GRAD(f) ((((b->f - a->f) * dy2 - (c->f - a->f) * dy1) * id))
  PV dx = {0, 0, GRAD(iz), GRAD(uz), GRAD(vz), GRAD(r), GRAD(g), GRAD(b), GRAD(a), GRAD(fog)};
#undef GRAD
  for (int y = ys; y < ye; y++) {
    float yc = y + 0.5f;
    /* the long edge a-c, and a-b or b-c */
    float tl = (yc - a->sy) / (c->sy - a->sy);
    float xl = a->sx + (c->sx - a->sx) * tl;
    float xs;
    if (yc < b->sy) {
      float t = (yc - a->sy) / (b->sy - a->sy);
      xs = a->sx + (b->sx - a->sx) * t;
    } else {
      float h = c->sy - b->sy;
      float t = h > 1e-6f ? (yc - b->sy) / h : 1;
      xs = b->sx + (c->sx - b->sx) * t;
    }
    float xa = xl < xs ? xl : xs, xb = xl < xs ? xs : xl;
    int x0 = (int)ceilf(xa - 0.5f), x1 = (int)ceilf(xb - 0.5f);
    if (x0 >= x1) continue;
    /* the attributes at (x0 + 0.5, yc) from vertex a */
    float ox = x0 + 0.5f - a->sx, oy = yc - a->sy;
    /* (the row step comes from the plane too: d/dy = -(d/dx * dx/dy) ... computed directly) */
    PV p;
    float gy_iz = ((c->iz - a->iz) * dx1 - (b->iz - a->iz) * dx2) * id;
#define AT(f, gy) (a->f + dx.f * ox + (gy) * oy)
    float gy_uz = ((c->uz - a->uz) * dx1 - (b->uz - a->uz) * dx2) * id;
    float gy_vz = ((c->vz - a->vz) * dx1 - (b->vz - a->vz) * dx2) * id;
    float gy_r = ((c->r - a->r) * dx1 - (b->r - a->r) * dx2) * id;
    float gy_g = ((c->g - a->g) * dx1 - (b->g - a->g) * dx2) * id;
    float gy_b = ((c->b - a->b) * dx1 - (b->b - a->b) * dx2) * id;
    float gy_a = ((c->a - a->a) * dx1 - (b->a - a->a) * dx2) * id;
    float gy_f = ((c->fog - a->fog) * dx1 - (b->fog - a->fog) * dx2) * id;
    p.iz = AT(iz, gy_iz), p.uz = AT(uz, gy_uz), p.vz = AT(vz, gy_vz);
    p.r = AT(r, gy_r), p.g = AT(g, gy_g), p.b = AT(b, gy_b), p.a = AT(a, gy_a), p.fog = AT(fog, gy_f);
#undef AT
    span(y, x0, x1, &p, &dx);
  }
}

static float fog_of(float z) {
  if (!fog_on) return 0;
  float f = (z - fog_near) / (fog_far - fog_near);
  return f <= 0 ? 0 : f >= 1 ? 255 : f * 255;
}

static void project(const CV *c, PV *p) {
  float iz = 1 / c->z;
  p->sx = cam_cx + cam_f * c->x * iz;
  p->sy = cam_cy - cam_f * c->y * iz;
  p->iz = iz, p->uz = c->u * iz, p->vz = c->v * iz;
  p->r = c->r, p->g = c->g, p->b = c->b, p->a = c->a, p->fog = c->fog;
}

static void lerp_cv(CV *o, const CV *a, const CV *b, float t) {
  o->x = a->x + (b->x - a->x) * t, o->y = a->y + (b->y - a->y) * t, o->z = a->z + (b->z - a->z) * t;
  o->u = a->u + (b->u - a->u) * t, o->v = a->v + (b->v - a->v) * t;
  o->r = a->r + (b->r - a->r) * t, o->g = a->g + (b->g - a->g) * t, o->b = a->b + (b->b - a->b) * t;
  o->a = a->a + (b->a - a->a) * t, o->fog = a->fog + (b->fog - a->fog) * t;
}

/* a triangle in camera space: culled, clipped against the near plane, projected and filled */
static void draw_tri(const CV *v0, const CV *v1, const CV *v2, bool cull) {
  if (cull) {
    /* the N64 culls back faces: those whose corners turn clockwise on the screen */
    float e1x = v1->x - v0->x, e1y = v1->y - v0->y, e1z = v1->z - v0->z;
    float e2x = v2->x - v0->x, e2y = v2->y - v0->y, e2z = v2->z - v0->z;
    float nx = e1y * e2z - e1z * e2y, ny = e1z * e2x - e1x * e2z, nz = e1x * e2y - e1y * e2x;
    if (nx * v0->x + ny * v0->y + nz * v0->z >= 0) return;
  }
  const CV *in[3] = {v0, v1, v2};
  int behind = (v0->z < cam_near) + (v1->z < cam_near) + (v2->z < cam_near);
  if (behind == 3) return;
  PV p[4];
  if (!behind) {
    project(v0, &p[0]), project(v1, &p[1]), project(v2, &p[2]);
    fill_tri(&p[0], &p[1], &p[2]);
    return;
  }
  CV out[4];
  int n = 0;
  for (int i = 0; i < 3; i++) {
    const CV *a = in[i], *b = in[(i + 1) % 3];
    bool ain = a->z >= cam_near, bin = b->z >= cam_near;
    if (ain) out[n++] = *a;
    if (ain != bin) lerp_cv(&out[n++], a, b, (cam_near - a->z) / (b->z - a->z));
  }
  for (int i = 0; i < n; i++) project(&out[i], &p[i]);
  for (int i = 1; i + 1 < n; i++) fill_tri(&p[0], &p[i], &p[i + 1]);
}

static CV vbuf[MAX_VERTS];

static void use_texture(const Batch *b) {
  if (b->mode & M_TEX) {
    const Tex *t = TEX(b->tex);
    tx_px = DAT(uint16_t, t->off), tx_wl = t->wl, tx_wm = (1 << t->wl) - 1, tx_hm = (1 << t->hl) - 1;
    tx_fmt = t->fmt;
  }
  tx_wrap = b->wrap;
}

static void draw_batch(const Item *it, const Batch *b, int layer) {
  const Mesh *m = it->mesh;
  const MVert *mv = DAT(MVert, m->vtx);
  const uint16_t *tri = DAT(uint16_t, m->tri);
  bool lit = b->mode & M_LIT;
  int nv = b->nv > MAX_VERTS ? MAX_VERTS : b->nv;
  const Mtx43 *M = &it->mv;
  const uint8_t *light = b->light;
  for (int i = 0; i < nv; i++) {
    const MVert *s = &mv[b->v0 + i];
    CV *c = &vbuf[i];
    float x = s->x, y = s->y, z = s->z;
    c->x = x * M->m[0][0] + y * M->m[1][0] + z * M->m[2][0] + M->m[3][0];
    c->y = x * M->m[0][1] + y * M->m[1][1] + z * M->m[2][1] + M->m[3][1];
    c->z = x * M->m[0][2] + y * M->m[1][2] + z * M->m[2][2] + M->m[3][2];
    c->u = s->s * (1.0f / 32), c->v = s->t * (1.0f / 32);
    if (lit) {
      float nx = (int8_t)s->c[0], ny = (int8_t)s->c[1], nz = (int8_t)s->c[2];
      float d = (nx * it->light[0] + ny * it->light[1] + nz * it->light[2]) * (1.0f / 127);
      if (d < 0) d = 0;
      c->r = light[0] + light[3] * d, c->g = light[1] + light[4] * d, c->b = light[2] + light[5] * d;
      if (c->r > 255) c->r = 255;
      if (c->g > 255) c->g = 255;
      if (c->b > 255) c->b = 255;
    } else c->r = s->c[0], c->g = s->c[1], c->b = s->c[2];
    c->a = s->c[3];
    c->fog = fog_of(c->z);
  }
  use_texture(b);
  d_mode = b->mode;
  if (it->alpha < 255) d_mode |= M_XLU;
  d_tint = it->tint, d_alpha = it->alpha;
  d_zwrite = !(d_mode & M_XLU);
  (void)layer;
  for (int i = 0; i < b->ntri; i++) {
    const uint16_t *t = tri + (b->tri0 + i) * 3;
    int i0 = t[0] - b->v0, i1 = t[1] - b->v0, i2 = t[2] - b->v0;
    if ((unsigned)i0 >= (unsigned)nv || (unsigned)i1 >= (unsigned)nv || (unsigned)i2 >= (unsigned)nv) continue;
    draw_tri(&vbuf[i0], &vbuf[i1], &vbuf[i2], true);
    r_stat_tris++;
  }
}

static void draw_item(const Item *it, int layer) {
  if (it->kind == I_MESH) {
    const Batch *b = DAT(Batch, it->mesh->batch);
    for (int i = 0; i < it->mesh->nbatch; i++) {
      const int16_t *rows = item_rows[it->batch0 + i];
      if (rows[1] <= r_band_y || rows[0] >= r_band_y + r_band_h) continue;
      int bl = it->alpha < 255 || (b[i].mode & M_XLU) ? 4 : (b[i].mode & (M_CUTOUT | M_DECAL)) ? 2 : 1;
      if (bl != layer) continue;
      draw_batch(it, &b[i], layer);
    }
  } else if (it->kind == I_BILLBOARD) {
    const Tex *t = TEX(it->tex);
    const uint16_t *px = DAT(uint16_t, t->off);
    float sx = it->mv.m[0][0], sy = it->mv.m[0][1], hw = it->mv.m[1][0], hh = it->mv.m[1][1];
    bool flip = it->mv.m[1][2] != 0;
    uint16_t zz = (uint16_t)(z_k / it->mv.m[0][2]);
    int x0 = (int)(sx - hw), x1 = (int)(sx + hw), y0 = (int)(sy - hh), y1 = (int)(sy + hh);
    int w = 1 << t->wl, h = 1 << t->hl;
    float du = w / (2 * hw), dv = h / (2 * hh);
    int tr = it->tint >> 16 & 255, tg = it->tint >> 8 & 255, tb = it->tint & 255;
    for (int y = y0 < r_band_y ? r_band_y : y0; y < y1 && y < r_band_y + r_band_h; y++) {
      int tv = (int)((y + 0.5f - (sy - hh)) * dv);
      if (tv < 0 || tv >= h) continue;
      uint16_t *cb = col_buf + (y - r_band_y) * r_w, *zb = z_buf + (y - r_band_y) * r_w;
      for (int x = x0 < 0 ? 0 : x0; x < x1 && x < r_w; x++) {
        int tu = (int)((x + 0.5f - (sx - hw)) * du);
        if (tu < 0 || tu >= w) continue;
        if (flip) tu = w - 1 - tu;
        if (zz < zb[x]) continue;
        uint16_t c = px[tv * w + tu];
        int a = it->alpha;
        if (t->fmt == TF_IA) {
          int in = c >> 8;
          a = a * (c & 255) >> 8;
          if (a < 8) continue;
          c = rgb565(tr * in >> 8, tg * in >> 8, tb * in >> 8);
        } else {
          if (c == TEX_KEY) continue;
          if (it->tint != 0xFFFFFF) c = mod565(c, tr + 1, tg + 1, tb + 1);
        }
        if (a < 255) cb[x] = blend565(c, cb[x], a);
        else cb[x] = c, zb[x] = zz;
      }
    }
  } else if (it->kind == I_SHADOW) {
    /* a disc (or square) on the ground: 12 triangles around its centre, dark and see-through */
    float rad = (float)(it->tint & 0xFFFF);
    bool square = it->tint & 0x80000000u;
    CV c[13];
    int n = square ? 4 : 12;
    for (int i = 0; i <= n; i++) {
      float lx = 0, lz = 0;
      if (i) {
        float an = (i - 1) * (2 * M_PI_F / n) + (square ? M_PI_F / 4 : 0);
        float rr = square ? rad * 1.4142f : rad;
        lx = cosf(an) * rr, lz = sinf(an) * rr;
      }
      const Mtx43 *M = &it->mv;
      c[i].x = lx * M->m[0][0] + 2 * M->m[1][0] + lz * M->m[2][0] + M->m[3][0];
      c[i].y = lx * M->m[0][1] + 2 * M->m[1][1] + lz * M->m[2][1] + M->m[3][1];
      c[i].z = lx * M->m[0][2] + 2 * M->m[1][2] + lz * M->m[2][2] + M->m[3][2];
      c[i].u = c[i].v = 0, c[i].r = c[i].g = c[i].b = 0, c[i].a = i ? (square ? 255 : 120) : 255, c[i].fog = 0;
    }
    d_mode = M_XLU | M_DECAL, d_tint = 0xFFFFFF, d_alpha = it->alpha, d_zwrite = false;
    for (int i = 1; i <= n; i++) {
      draw_tri(&c[0], &c[i], &c[i % n + 1], false);
    }
  }
}

/* ------------------------------------------------------------------------------------------------ the sky */
static void draw_sky(void) {
  /* the horizon's row, from the camera's pitch */
  float hz = cam_cy + cam_f * tanf(sky_pitch);
  static const uint32_t skies[][2] = {
    {0x4878E8, 0xA8D0F8},   /* blue sky (the grass levels) */
    {0x101830, 0x304878},   /* night (Bowser, the Big Boo) */
    {0x182860, 0x6890D0},   /* water levels */
    {0xC8D8F0, 0xF8F8F8},   /* snow */
    {0x601808, 0xE07818},   /* fire */
    {0x8878B8, 0xD8C8E8},   /* clouds (Rainbow Ride, the castle's top) */
    {0x0, 0x0},             /* black (inside) */
  };
  int k = sky_kind < 0 || sky_kind > 6 ? 0 : sky_kind;
  uint32_t top = skies[k][0], hor = skies[k][1];
  for (int y = r_band_y; y < r_band_y + r_band_h; y++) {
    float t = (y - (hz - r_h * 1.2f)) / (r_h * 1.2f);
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int r = (int)((top >> 16 & 255) + (((int)(hor >> 16 & 255) - (int)(top >> 16 & 255)) * t));
    int g = (int)((top >> 8 & 255) + (((int)(hor >> 8 & 255) - (int)(top >> 8 & 255)) * t));
    int b = (int)((top & 255) + (((int)(hor & 255) - (int)(top & 255)) * t));
    uint16_t c = rgb565(r, g, b);
    uint16_t *row = col_buf + (y - r_band_y) * r_w;
    for (int x = 0; x < r_w; x++) row[x] = c;
  }
}

/* ------------------------------------------------------------------------------------------------ overlays */
/* overlays draw at full resolution into ov_buf, which holds rows ov_y to ov_y + ov_h, 320 wide */
static uint16_t *ov_buf;
static int ov_y, ov_h;

void r_putpixel(int x, int y, uint16_t c) {
  if ((unsigned)x < 320u && y >= ov_y && y < ov_y + ov_h) ov_buf[(y - ov_y) * 320 + x] = c;
}
void r_hline(int x0, int x1, int y, uint16_t c, uint8_t alpha) {
  if (y < ov_y || y >= ov_y + ov_h) return;
  if (x0 < 0) x0 = 0;
  if (x1 > 320) x1 = 320;
  uint16_t *row = ov_buf + (y - ov_y) * 320;
  for (int x = x0; x < x1; x++) row[x] = alpha == 255 ? c : blend565(c, row[x], alpha + 1);
}

typedef struct { int16_t x, y, w, h; uint16_t c; uint8_t a; } RectOv;
static RectOv rects[32];
static int nrects;
void r_rect(int x, int y, int w, int h, uint16_t color, uint8_t alpha) {
  if (nrects < 32) rects[nrects++] = (RectOv){(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, color, alpha};
}

static void run_overlays(uint16_t *buf, int y, int h) {
  ov_buf = buf, ov_y = y, ov_h = h;
  r_band = buf, r_band_y = y, r_band_h = h;
  for (int i = 0; i < nrects; i++)
    for (int yy = rects[i].y; yy < rects[i].y + rects[i].h; yy++) r_hline(rects[i].x, rects[i].x + rects[i].w, yy, rects[i].c, rects[i].a);
  for (int i = 0; i < noverlays; i++) overlays[i].fn(overlays[i].ctx);
}

/* ------------------------------------------------------------------------------------------------ the bands */
static int by_depth(const void *a, const void *b) {
  float da = items[*(const uint8_t *)a].depth, db = items[*(const uint8_t *)b].depth;
  return da < db ? 1 : da > db ? -1 : 0;
}

#include <stdlib.h>
/* half resolution: 8 rows at a time doubled to full resolution, with the overlays */
#define HALF_ROWS 4
static uint16_t full_rows[320 * HALF_ROWS * 2];

void r_draw(void) {
  static uint8_t order[MAX_ITEMS];
  int nxlu = 0;
  for (int i = 0; i < nitems; i++)
    if (items[i].layers & 4) order[nxlu++] = (uint8_t)i;
  qsort(order, nxlu, 1, by_depth);
  int band_rows = BAND_PIXELS / r_w;
  r_stat_items = nitems;
  for (int band_y = 0; band_y < r_h; band_y += band_rows) {
    r_band = col_buf;
    r_band_y = band_y;
    r_band_h = r_h - band_y < band_rows ? r_h - band_y : band_rows;
    int bh = r_band_h;
    memset(z_buf, 0, (size_t)r_w * bh * 2);
    draw_sky();
    for (int layer = 1; layer <= 2; layer++)
      for (int i = 0; i < nitems; i++) {
        const Item *it = &items[i];
        if (!(it->layers & layer) || it->y1 <= band_y || it->y0 >= band_y + bh) continue;
        draw_item(it, layer);
      }
    for (int k = 0; k < nxlu; k++) {
      const Item *it = &items[order[k]];
      if (it->y1 <= band_y || it->y0 >= band_y + bh) continue;
      draw_item(it, 4);
    }
    if (r_scale == 1) {
      run_overlays(col_buf, band_y, bh);
      plat_push(0, band_y, r_w, bh, col_buf);
    } else {
      for (int y = 0; y < bh; y += HALF_ROWS) {
        int n = bh - y < HALF_ROWS ? bh - y : HALF_ROWS;
        for (int k = 0; k < n; k++) {
          const uint16_t *src = col_buf + (y + k) * r_w;
          uint16_t *d = full_rows + k * 2 * 320;
          for (int x = 0; x < r_w; x++) d[2 * x] = d[2 * x + 1] = src[x];
          memcpy(d + 320, d, 640);
        }
        run_overlays(full_rows, (band_y + y) * 2, n * 2);
        plat_push(0, (band_y + y) * 2, 320, n * 2, full_rows);
      }
    }
  }
  nrects = 0;
}
