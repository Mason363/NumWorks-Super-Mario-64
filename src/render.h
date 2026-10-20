/* The 3D renderer: triangles with textures, Gouraud colours, fog and transparency, drawn in bands.
 *
 * There is no room for a whole frame (320 x 240 colours and depths would take 300 KB), so the screen is drawn
 * a band of rows at a time: the scene is first described (a list of items: a mesh, its matrix, how it is lit),
 * then each band draws every item that reaches it, opaque ones first, then decals and cutouts, then the
 * transparent ones from back to front, then the flat overlays (the HUD, text), and goes to the screen.
 *
 * Everything a frame needs lives in an RFrame, on the stack of whoever draws the frame (r_begin to r_draw):
 * the game's own RAM keeps none of it. */
#ifndef SM_RENDER_H
#define SM_RENDER_H
#include <stdbool.h>
#include <stdint.h>
#include "data.h"
#include "smath.h"

#define R_BAND_PIXELS (320 * 10)   /* a band: 10 rows at full resolution, 20 at half */
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
  uint16_t batch0;         /* its batches' rows in RFrame.rows */
  uint8_t y0, y1;          /* the rows it may cover */
  uint8_t kind, alpha, layers, flags;
} RItem;

typedef struct {
  uint16_t col[R_BAND_PIXELS], z[R_BAND_PIXELS];
  RItem items[R_MAX_ITEMS];
  uint8_t rows[R_MAX_BATCHES][2];
  RVert verts[R_MAX_VERTS];
  uint8_t order[R_MAX_ITEMS];
  uint8_t overlay[R_OVERLAY_BYTES];   /* the overlays' commands (r_overlay_alloc) */
} RFrame;

/* the screen is drawn at full or half resolution (the N0110 and N0115) */
extern int r_w, r_h, r_scale;

/* the frame's memory, until r_draw */
void r_begin(RFrame *f);
/* the camera, for the frame being described: view takes the world to the camera's space (x right, y up, z
 * forwards), roll turns the picture (radians) */
void r_frame(const Mtx43 *view, float roll, float fov_deg, float near, float far);
/* memory free until r_draw (the bands'), for whoever describes the scene */
void *r_scratch(int *bytes);
/* the background: a skybox (the game's BACKGROUND_*: the camera's yaw places it, tint multiplies its colours,
 * 0xFFFFFF none), or a plain colour (0x8000 | its RGBA5551) */
void r_sky(int sky, int16_t yaw, uint32_t tint);
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

/* overlay drawing, for r_overlay functions: the rows being drawn (320 wide) */
extern uint16_t *r_band;
extern int r_band_y, r_band_h;
void r_hline(int x0, int x1, int y, uint16_t c, uint8_t alpha);
static inline uint16_t *r_row(int y) { return r_band + (y - r_band_y) * 320; }

/* draws every band and pushes it to the screen */
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
