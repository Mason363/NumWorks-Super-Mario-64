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
#define PORT_AREA(mesh, mode, px, py, pz, fx, fy, fz, fov, near, far, bg, fogc, fogn, fogf) \
  PORT_MODEL_TAG, (mesh), PORT_AREA_TAG, (mode), (px), (py), (pz), (fx), (fy), (fz), (fov), (near), (far), (bg), \
  (fogc), (fogn), (fogf)
/* a level the game does not have: its paintings and pipes don't take Mario there */
#define PORT_LEVEL_MISSING() EXIT()

struct Surface;
struct SurfaceNode;
/* collision in flash (src/port/gen/terrain.c): a level area's surfaces, and its 16 x 16 cells' lists of floors,
 * ceilings and walls as surface numbers (cell z, x, list k starts at cells[(z * 16 + x) * 3 + k], ends where
 * the next starts) */
struct PortTerrain {
  const short *terrain;               /* the game's collision data it stands for */
  const struct Surface *surfaces;
  const unsigned short *index;
  const unsigned short *cells;
};
extern const struct PortTerrain port_terrains[];
/* a list of surfaces to walk: the objects' (linked nodes in RAM) or the level's (numbers in flash) */
struct PortSurfaceList {
  struct SurfaceNode *node;
  const unsigned short *at, *end;
  const struct Surface *surfaces;
};
struct Surface *port_surface_next(struct PortSurfaceList *list);
struct PortSurfaceList port_dynamic_list(int cellZ, int cellX, int kind);
struct PortSurfaceList port_static_list(int cellZ, int cellX, int kind);

/* Mario's animations, by number (src/port/gen/anims.c) */
struct Animation;
extern const struct Animation *const port_mario_anims[];
extern const int port_mario_anim_count;
/* the animations are packed (tools/port.py's pack_anim_data): a value is one byte, its channel (four u16:
 * count, offset, base, step) says what it stands for. The next channel's value at frame (what the game's
 * values[retrieve_animation_index(frame, &attr)] was) */
#define PORT_ANIM_CHANNEL 4
static inline short port_anim_value(int frame, unsigned short **attr, const void *values) {
  const unsigned short *c = *attr;
  if (c[0] == 0xFFFF) return 0;   /* (the table's end) */
  int i = frame < c[0] ? frame : c[0] - 1;
  *attr += PORT_ANIM_CHANNEL;
  if (i < 0) i = 0;
  return (short) (c[2] + ((const unsigned char *) values)[c[1] + i] * c[3]);
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
