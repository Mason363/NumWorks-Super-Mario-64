/* The 3D renderer: triangles with textures, Gouraud colours, fog and transparency, drawn in bands.
 *
 * There is no room for a whole frame (320 x 240 colours and depths would take 300 KB), so the screen is drawn
 * a band of columns at a time: the scene is first described (a list of items: a mesh, its matrix, how it is
 * lit), then each band draws every item that reaches it, opaque ones first, then decals and cutouts, then the
 * transparent ones from back to front, then the flat overlays (the HUD, text), and goes to the screen.
 *
 * The bands are columns because the calculator's screen refreshes itself a column at a time, from its own
 * memory: the bands go in the order the refresh takes them, each one once the refresh has passed it and before
 * the next one comes to it, so that every refresh shows one whole frame (render.c: the refresh).
 *
 * Everything a frame needs lives in an RFrame, on the stack of whoever draws the frame (r_begin to r_draw):
 * the game's own RAM keeps none of it. */
#ifndef SM_RENDER_H
#define SM_RENDER_H
#include <stdbool.h>
#include <stdint.h>
#include "data.h"
#include "smath.h"

#define R_BAND_PIXELS (320 * 10)   /* a band: 13 columns at full resolution, 26 at half */
#define R_MAX_ITEMS 96
#define R_MAX_BATCHES 896
#define R_MAX_VERTS 128            /* a batch's vertices (tools/pack.py: BATCH_VERTS) */
#define R_OVERLAY_BYTES 1024

/* a vertex in camera space with what is interpolated (colours 0..255, fog 0..255), and on the screen (sx, sy,
 * 1 / z: when it is in front of the near plane) */
typedef struct {
  float x, y, z, u, v, sx, sy, iz;
  uint8_t r, g, b, a, fog, pad[3];
} RVert;

/* what the frame draws */
typedef struct {
  const Mesh *mesh;
  Mtx43 mv;                /* model to camera */
  float depth;             /* the nearest point, for the transparent ones' order */
  uint32_t tint;           /* multiplies the colours (0xFFFFFF none), or a shadow's radius */
  uint16_t batch0;         /* its batches' columns in RFrame.cols */
  uint8_t x0, x1;          /* the columns it may cover, in pairs (x0 * 2 to x1 * 2) */
  uint8_t kind, alpha, layers, flags;
} RItem;

/* (a band's colours are in rows, as the screen takes them; its depths go down its columns one after the other) */
typedef struct {
  uint16_t col[R_BAND_PIXELS], z[R_BAND_PIXELS];
  RItem items[R_MAX_ITEMS];
  uint8_t cols[R_MAX_BATCHES][2];
  RVert verts[R_MAX_VERTS];
  uint8_t order[R_MAX_ITEMS];
  uint8_t overlay[R_OVERLAY_BYTES];   /* the overlays' commands (r_overlay_alloc) */
} RFrame;

/* the screen is drawn at full resolution (320 x 240) or half (160 x 120, each pixel doubled: faster) */
extern int r_w, r_h, r_scale;
void r_set_half(bool half);
/* the way the calculator's screen refreshes: 0 from the left, 1 from the right, 2 not waited for (the bands go as
 * soon as they are drawn, and the picture tears) */
extern int r_refresh;

/* the frame's memory, until r_draw */
void r_begin(RFrame *f);
/* the camera, for the frame being described: view takes the world to the camera's space (x right, y up, z
 * forwards), roll turns the picture (radians) */
void r_frame(const Mtx43 *view, float roll, float fov_deg, float near, float far);
/* memory free until r_draw (the bands'), for whoever describes the scene */
void *r_scratch(int *bytes);
/* the background: a skybox (the game's BACKGROUND_*: the camera's yaw places it, tint multiplies its colours,
 * 0xFFFFFF none), or a plain colour (0x8000 | its RGB 555: the game's RGBA 5551 >> 1) */
void r_sky(int sky, int16_t yaw, uint32_t tint);
/* (and the backdrop of Mario's head, a picture repeated over the screen) */
#define R_SKY_TITLE 10
/* fog: colour, from distance near to far (0 to switch it off) */
void r_fog(uint16_t color, float near, float far);

/* draws mesh m with matrix mtx (4x3, world space; NULL: the identity), lit with the scene's light when its
 * vertices carry normals; tint multiplies its colours (0xFFFFFF: none), alpha fades it (255: opaque).
 * flags: R_CAMERA (mtx takes the mesh to the camera's space, not the world's), R_NOCULL (both faces) */
enum { R_CAMERA = 1, R_NOCULL = 2 };
void r_mesh(const Mesh *m, const Mtx43 *mtx, uint32_t tint, uint8_t alpha, int flags);
/* a flat shadow on the ground: a dark disc (or square) of radius r at p, along the floor's normal n */
void r_shadow(const Vec3f p, const Vec3f n, float r, uint8_t alpha, bool square);
/* where world point p lands on the screen (false behind the camera) */
bool r_project(const Vec3f p, float *sx, float *sy, float *depth);
/* the camera's matrix (world to camera) and position */
const Mtx43 *r_view(void);

/* flat overlays, in screen pixels (320 x 240 whatever the resolution), drawn last in each band, in order */
typedef void (*r_overlay_fn)(const void *cmd);
/* room for an overlay command of n bytes, drawn by fn (NULL: no room left) */
void *r_overlay_alloc(r_overlay_fn fn, int n);
void r_rect(int x, int y, int w, int h, uint16_t color, uint8_t alpha);

/* overlay drawing, for r_overlay functions: the columns being drawn (r_band_x to r_band_x + r_band_w, all 240
 * rows of them), in rows: r_px(x, y) is pixel (x, y) when it is in the band */
extern uint16_t *r_band;
extern int r_band_x, r_band_w;
void r_hline(int x0, int x1, int y, uint16_t c, uint8_t alpha);
/* the screen's columns x0 to x1 that something an overlay draws covers (all of them, the band's or not: the
 * overlays are not run for the bands they do not reach) */
void r_touch(int x0, int x1);
static inline uint16_t *r_px(int x, int y) { return r_band + y * r_band_w + (x - r_band_x); }

/* draws every band and pushes it to the screen, in step with its refresh */
void r_draw(void);
/* ends the frame without drawing it */
void r_skip(void);

/* statistics of the last frame */
extern int r_stat_tris, r_stat_items, r_stat_verts;

static inline uint16_t rgb565(int r, int g, int b) {
  return (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}
static inline uint16_t blend565(uint16_t s, uint16_t d, int a) {   /* a: 0..256 */
  uint32_t ss = (s | (uint32_t)s << 16) & 0x07E0F81Fu, dd = (d | (uint32_t)d << 16) & 0x07E0F81Fu;
  uint32_t r = (dd + ((ss - dd) * (uint32_t)(a >> 3) >> 5)) & 0x07E0F81Fu;
  return (uint16_t)(r | r >> 16);
}
#endif
