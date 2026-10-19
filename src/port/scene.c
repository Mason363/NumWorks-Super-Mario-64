/* Drawing the game's world (the decompilation's rendering_graph_node.c walked the area's graph and made the N64's
 * display lists): the area's mesh, then each object of the area with its model, animation and shadow, seen from
 * where the game's camera (Lakitu) is. */
#include <string.h>
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "game/area.h"
#include "game/camera.h"
#include "game/level_update.h"
#include "game/object_list_processor.h"
#include "game/mario.h"
#include "engine/graph_node.h"
#include "port.h"
#include "port_game.h"
#include "../render.h"

extern struct LakituState gLakituState;
extern struct Object gObjectPool[];

static float area_fov(struct PortArea *a) {
  struct GraphNodePerspective p;
  memset(&p, 0, sizeof p);
  p.fov = a->fov ? a->fov : 45;
  geo_camera_fov(GEO_CONTEXT_RENDER, &p.fnNode.node, NULL);
  return p.fov;
}

static void draw_object(struct Object *o) {
  struct GraphNodeObject *g = &o->header.gfx;
  if (!(g->node.flags & GRAPH_RENDER_ACTIVE) || (g->node.flags & GRAPH_RENDER_INVISIBLE)) return;
  int model = port_model_of(g->sharedChild);
  if (model == 0xFFFF || model >= HDR->nmodel) return;
  /* (models come next: the object's matrix, its parts and animation) */
}

void port_draw_area(struct GraphNodeRoot *root) {
  struct PortArea *a = (struct PortArea *) root;
  Vec3f eye = {gLakituState.pos[0], gLakituState.pos[1], gLakituState.pos[2]};
  Vec3f at = {gLakituState.focus[0], gLakituState.focus[1], gLakituState.focus[2]};
  float roll = gLakituState.roll * (M_PI_F / 32768.0f);
  r_frame(eye, at, roll, area_fov(a), a->near ? a->near : 100, a->far ? (float) a->far : 20000);
  r_sky(0);
  if (a->fog_color) r_fog((u16) a->fog_color, a->fog_near, a->fog_far);
  if (a->mesh != 0xFFFF && a->mesh < HDR->nmesh) r_mesh(MESH(a->mesh), NULL, 0xFFFFFF, 255);
  for (int i = 0; i < OBJECT_POOL_CAPACITY; i++) {
    struct Object *o = &gObjectPool[i];
    if (o->activeFlags & ACTIVE_FLAG_ACTIVE && o->header.gfx.areaIndex == root->areaIndex) draw_object(o);
  }
}

void geo_process_root(struct GraphNodeRoot *node, Vp *b, Vp *c, s32 clearColor) { port_draw_area(node); }
void end_master_display_list(void) { r_draw(); }
