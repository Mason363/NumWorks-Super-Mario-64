/* The port: what the decompilation's code needs from the calculator, and what the calculator takes from it.
 * Every decompilation file is compiled with this header first (-include). */
#ifndef SM_PORT_H
#define SM_PORT_H
#ifndef __ASSEMBLER__
#include <stddef.h>
#include <stdint.h>

/* a model stub: the scripts name geo layouts and display lists; for the calculator each one only holds the
 * index of its own model (src/data.bin), with a tag to tell it from anything else */
#define PORT_MODEL_TAG 0x504D3634u
#define PORT_MODEL(i) PORT_MODEL_TAG, (i), 0
/* an area's geo layout: its mesh, then what its camera, frustum, background and fog nodes said */
#define PORT_AREA_TAG 0x41524541u
#define PORT_AREA(mesh, mode, px, py, pz, fx, fy, fz, fov, near, far, bg, fogc, fogn, fogf, rooms) \
  PORT_MODEL_TAG, (mesh), PORT_AREA_TAG, (mode), (px), (py), (pz), (fx), (fy), (fz), (fov), (near), (far), (bg), \
  (fogc), (fogn), (fogf), (rooms)
/* a texture of the screens (src/port/gen/segment2.c, power_meter.c): the index of the calculator's own in
 * src/data.bin, after a tag (src/port/gfx.c reads it) */
#define PORT_TEX_STUB(i) 0x50, 0x54, 0x58, 0x54, (i) & 0xFF, ((i) >> 8) & 0xFF

/* a text in the game's characters (tools/port.py turns the decompilation's _("...") into them): one macro
 * argument, however many bytes */
#define PORT_TEXT(...) __VA_ARGS__

/* a level the game does not have: its paintings and pipes don't take Mario there */
#define PORT_LEVEL_MISSING() EXIT()

struct Surface;
struct SurfaceNode;
/* collision in flash (src/port/gen/terrain.c): a level area's surfaces, and its 16 x 16 cells' lists of floors,
 * ceilings and walls as surface numbers (cell z, x, list k starts at cells[(z * 16 + x) * 3 + k], ends where
 * the next starts) */
struct PortLevelSurface;
struct PortTerrain {
  const short *terrain;               /* the game's collision data it stands for */
  const struct PortLevelSurface *surfaces;
  const short (*vertices)[3];         /* the surfaces' corners */
  const unsigned short *runs;         /* each cell list's run of surface numbers, from runs[i] to runs[i + 1] */
  const unsigned short *index;        /* the lists' other surfaces */
  const unsigned short *cells;
};
extern const struct PortTerrain port_terrains[];
/* a list of surfaces to walk: the objects' (linked nodes in RAM) or the level's (numbers in flash) */
struct PortSurfaceList {
  struct SurfaceNode *node;
  unsigned short run, run_end;   /* (the level's: a run of numbers, then a list, walked together) */
  const unsigned short *at, *end;
  int dir;                       /* 1: floors, highest first; -1: ceilings, lowest first; 0: walls */
};
struct Surface *port_surface_next(struct PortSurfaceList *list);
struct PortSurfaceList port_dynamic_list(int cellZ, int cellX, int kind);
struct PortSurfaceList port_static_list(int cellZ, int cellX, int kind);

/* Mario's animations, by number (src/port/gen/anims.c) */
struct Animation;
extern const struct Animation *const port_mario_anims[];
extern const int port_mario_anim_count;
/* the animations are packed (tools/port.py's pack_anim_data): a channel that keeps one value is 0, base (or
 * 1, base, 0: the first three, the translations, which the game may skip, take PORT_ANIM_CHANNEL u16); else
 * 0x8000 | (stride - 1) << 9 | (step - 1), base, offset: a byte at offset for every stride frames (the frames
 * between them on the line between their values), a value base + byte * step; 0xFFFF ends the table. The
 * frames' count is in the two bytes before the values. The next channel's value at frame (what the game's
 * values[retrieve_animation_index(frame, &attr)] was) */
#define PORT_ANIM_CHANNEL 3
static inline short port_anim_value(int frame, unsigned short **attr, const void *values) {
  const unsigned short *c = *attr;
  unsigned info = c[0];
  if (info == 0xFFFF) return 0;   /* (the table's end) */
  *attr += info == 0 ? 2 : PORT_ANIM_CHANNEL;
  if (info < 0x8000) return (short) c[1];
  const unsigned char *b = (const unsigned char *) values;
  int n = b[-2] | b[-1] << 8, s = (int) ((info >> 9) & 31) + 1, step = (int) (info & 511) + 1;
  int i = frame < n ? frame : n - 1;
  if (i < 0) i = 0;
  const unsigned char *v = (const unsigned char *) values + c[2];
  int k = i / s, r = i - k * s;
  if (r == 0) return (short) (c[1] + v[k] * step);
  return (short) (c[1] + ((v[k] * (s - r) + v[k + 1] * r) * step + s / 2) / s);
}

/* the model a geo layout or display list stub stands for (0xFFFF: none) */
int port_model_of(const void *geo_or_dl);

/* a level the game has (its script is not PORT_LEVEL_MISSING) */
int port_level_exists(int level);

/* draws the frame (src/port/scene.c): what level_script_execute did after the script, render_game */
void port_render(void);

/* no sound on the calculator */
#define PORT_NO_AUDIO 1
#endif
#endif
