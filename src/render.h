/* The 3D renderer: triangles with textures, Gouraud colours, fog and transparency, drawn in bands.
 *
 * There is no room for a whole frame (320 x 240 colours and depths would take 300 KB), so the screen is drawn
 * a band of rows at a time: the scene is first described (a list of items: a mesh, its matrix, how it is lit),
 * then each band draws every item that reaches it, opaque ones first, then cutouts, then the transparent ones
 * from back to front, then the flat overlays (the HUD, text), and goes to the screen. */
#ifndef SM_RENDER_H
#define SM_RENDER_H
#include <stdbool.h>
#include <stdint.h>
#include "data.h"
#include "smath.h"

/* the screen is drawn at full or half resolution (the N0110 and N0115) */
extern int r_w, r_h, r_scale;

/* the camera, for the frame being described */
void r_frame(const Vec3f eye, const Vec3f at, float roll, float fov_deg, float near, float far);
/* a background: the sky (a vertical gradient with the skybox's own picture, panned with the camera) */
void r_sky(int sky);
/* fog: colour, from distance near to far (0 to switch it off) */
void r_fog(uint16_t color, float near, float far);

/* the ways an item is lit */
enum { L_BAKED, L_LIT };
/* draws mesh m with matrix mtx (4x3, world space; NULL: the identity), lit with the scene's light when its
 * vertices carry normals; tint multiplies its colours (0xFFFFFF: none), alpha fades it (255: opaque) */
void r_mesh(const Mesh *m, const Mtx43 *mtx, uint32_t tint, uint8_t alpha);
/* a camera-facing quad of texture tex, centred on p, w x h world units */
void r_billboard(const Vec3f p, float w, float h, int tex, uint32_t tint, uint8_t alpha, bool flipx);
/* a flat shadow on the ground: a dark disc of radius r at p, along the floor's normal n */
void r_shadow(const Vec3f p, const Vec3f n, float r, uint8_t alpha, bool square);

/* flat overlays, in screen pixels (drawn last in each band) */
void r_rect(int x, int y, int w, int h, uint16_t color, uint8_t alpha);
void r_image(int x, int y, int tex, int scale, uint32_t tint);   /* a texture, as is */
typedef void (*r_overlay_fn)(void *ctx);
void r_overlay(r_overlay_fn fn, void *ctx);

/* draws every band and pushes it to the screen */
void r_draw(void);

/* where world point p lands on the screen (false behind the camera) */
bool r_project(const Vec3f p, float *sx, float *sy, float *depth);

/* overlay drawing, for r_overlay functions: the band being drawn */
extern uint16_t *r_band;
extern int r_band_y, r_band_h;
void r_putpixel(int x, int y, uint16_t c);
void r_hline(int x0, int x1, int y, uint16_t c, uint8_t alpha);

/* statistics of the last frame */
extern int r_stat_tris, r_stat_items;
#endif
