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
/* collision in flash (src/port/gen/terrain.c): the terrain data of a level area, and its cells */
struct PortTerrain {
  const short *terrain;
  const void *cells;   /* SpatialPartitionCell[16][16] */
};
extern const struct PortTerrain port_terrains[];

/* Mario's animations, by number (src/port/gen/mario_anims.c) */
struct Animation;
extern const struct Animation *const port_mario_anims[];
extern const int port_mario_anim_count;

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
