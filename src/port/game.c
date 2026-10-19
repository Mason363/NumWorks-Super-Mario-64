/* The game itself, on the calculator: what the N64's game_init.c and main.c did around the decompilation's
 * code (memory, the controller, the save chip, the frame), and the models and areas its scripts load. */
#include <string.h>
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "game/game_init.h"
#include "game/memory.h"
#include "game/area.h"
#include "game/camera.h"
#include "game/save_file.h"
#include "game/level_update.h"
#include "engine/level_script.h"
#include "engine/graph_node.h"
#include "port.h"
#include "port_game.h"
#include "port_menu.h"
#include "plat.h"

/* the N64 had 4 MB for this; the levels' scripts, cameras, warps and the objects' surfaces fit in less. The
 * effects' pool (the HUD's text, the bubbles and snow) was 16 KB of it */
#define EFFECTS_SIZE (4 * 1024)
#ifdef HOST
#define POOL_SIZE (60 * 1024)   /* (a computer's pointers are twice as big) */
#else
#define POOL_SIZE (26 * 1024 + EFFECTS_SIZE)
#endif
static u64 sPool[POOL_SIZE / 8];


struct Controller gControllers[3];
struct Controller *gPlayer1Controller = &gControllers[0];
struct Controller *gPlayer2Controller = &gControllers[1];
struct Controller *gPlayer3Controller = &gControllers[2];
u8 gControllerBits = 1;
u32 gGlobalTimer;
u16 gAreaUpdateCounter;
struct GraphNode gObjParentGraphNode;
struct SaveBuffer gSaveBuffer;
static OSContPad sPad;

/* ------------------------------------------------------------------------------------------------ models */
/* what a model stub turns into: a node with the calculator's model, or (an area's) a root with its camera */
struct PortNode {
  struct GraphNode node;
  u16 model;
};

struct PortArea gPortAreas[8];

struct GraphNode *process_geo_layout(struct AllocOnlyPool *pool, void *segptr) {
  const GeoLayout *g = segptr;
  if (!g || g[0] != PORT_MODEL_TAG) return NULL;
  if (g[2] == PORT_AREA_TAG) {
    /* an area: a root, its camera node and the camera itself (geo_camera_main's GEO_CONTEXT_CREATE) */
    struct PortArea *a = alloc_only_pool_alloc(pool, sizeof(struct PortArea));
    struct GraphNodeCamera *gc = alloc_only_pool_alloc(pool, sizeof(struct GraphNodeCamera));
    struct GraphNode **views = alloc_only_pool_alloc(pool, sizeof(struct GraphNode *));
    memset(a, 0, sizeof *a);
    memset(gc, 0, sizeof *gc);
    a->root.node.type = GRAPH_NODE_TYPE_ROOT;
    a->root.node.flags = GRAPH_RENDER_ACTIVE;
    a->root.numViews = 1;
    a->root.views = views;
    views[0] = &gc->fnNode.node;
    gc->config.mode = (s32) g[3];
    gc->pos[0] = (s16) g[4], gc->pos[1] = (s16) g[5], gc->pos[2] = (s16) g[6];
    gc->focus[0] = (s16) g[7], gc->focus[1] = (s16) g[8], gc->focus[2] = (s16) g[9];
    create_camera(gc, pool);
    a->camera = gc;
    a->mesh = (u16) g[1];
    a->fov = (u16) g[10], a->near = (u16) g[11], a->far = (u32) g[12];
    a->background = (u16) g[13];
    a->fog_color = (u32) g[14];
    a->fog_near = (u16) g[15], a->fog_far = (u16) g[16];
    a->rooms = (const u16 *) g[17];
    return &a->root.node;
  }
  struct PortNode *n = alloc_only_pool_alloc(pool, sizeof(struct PortNode));
  memset(n, 0, sizeof *n);
  n->node.type = PORT_NODE_MODEL;
  n->node.flags = GRAPH_RENDER_ACTIVE;
  n->model = (u16) g[1];
  return &n->node;
}

int port_model_of(const void *p) {
  const struct GraphNode *n = p;
  if (!n) return 0xFFFF;
  if (n->type == PORT_NODE_MODEL) return ((const struct PortNode *) n)->model;
  if (n->type == GRAPH_NODE_TYPE_DISPLAY_LIST || n->type == GRAPH_NODE_TYPE_SCALE) {
    const u32 *dl = n->type == GRAPH_NODE_TYPE_SCALE ? ((const struct GraphNodeScale *) n)->displayList
                                                     : ((const struct GraphNodeDisplayList *) n)->displayList;
    if (dl && dl[0] == PORT_MODEL_TAG) return (int) dl[1];
  }
  return 0xFFFF;
}

/* ------------------------------------------------------------------------------------------------ the save chip */
/* the EEPROM (512 bytes) is a file on the calculator, with its copy in a Python script (port/save.c) */
s32 osEepromLongRead(OSMesgQueue *mq, u8 address, u8 *buffer, int nbytes) {
  port_eeprom_read(address * 8, buffer, nbytes);
  return 0;
}
s32 osEepromLongWrite(OSMesgQueue *mq, u8 address, u8 *buffer, int nbytes) {
  port_eeprom_write(address * 8, buffer, nbytes);
  return 0;
}

/* ------------------------------------------------------------------------------------------------ the controller */
/* the arrows are the stick: full tilt, or half with alpha held (walking); diagonals at the same tilt */
static void read_controller(void) {
  uint32_t k = plat_keys();
  struct Controller *c = gPlayer1Controller;
  int dx = (k & K_RIGHT ? 1 : 0) - (k & K_LEFT ? 1 : 0), dy = (k & K_UP ? 1 : 0) - (k & K_DOWN ? 1 : 0);
  float tilt = (k & K_WALK) ? 34.0f : 80.0f;
  float sx = (float) dx, sy = (float) dy;
  if (dx && dy) sx *= 0.7071f, sy *= 0.7071f;
  sPad.stick_x = (s8) (sx * tilt);
  sPad.stick_y = (s8) (sy * tilt);
  u16 b = 0;
  if (k & K_A) b |= A_BUTTON;
  if (k & K_B) b |= B_BUTTON;
  if (k & K_Z) b |= Z_TRIG;
  if (k & K_START) b |= START_BUTTON;
  if (k & K_R) b |= R_TRIG;
  if (k & K_CL) b |= L_CBUTTONS;
  if (k & K_CR) b |= R_CBUTTONS;
  if (k & K_CU) b |= U_CBUTTONS;
  if (k & K_CD) b |= D_CBUTTONS;
  sPad.button = b;
  c->controllerData = &sPad;
  c->rawStickX = sPad.stick_x;
  c->rawStickY = sPad.stick_y;
  c->buttonPressed = sPad.button & (sPad.button ^ c->buttonDown);
  c->buttonDown = sPad.button;
  /* (game_init.c's adjust_analog_stick) */
  c->stickX = 0, c->stickY = 0;
  if (c->rawStickX <= -8) c->stickX = c->rawStickX + 6;
  if (c->rawStickX >= 8) c->stickX = c->rawStickX - 6;
  if (c->rawStickY <= -8) c->stickY = c->rawStickY + 6;
  if (c->rawStickY >= 8) c->stickY = c->rawStickY - 6;
  c->stickMag = sqrtf(c->stickX * c->stickX + c->stickY * c->stickY);
  if (c->stickMag > 64) {
    c->stickX *= 64 / c->stickMag;
    c->stickY *= 64 / c->stickMag;
    c->stickMag = 64;
  }
  *gPlayer3Controller = *gPlayer1Controller;
}

#include "game/object_list_processor.h"
/* the frame's display list memory: the screens' commands (HUD, menus, text: src/port/gfx.c plays them), from
 * the start, and their matrices and vertices (alloc_display_list), from the end */
#define GFX_POOL 512
Gfx port_gfx_pool[GFX_POOL];
/* the most the game used of its objects, pool and surfaces (tests read them: tests/play.c --stats, and in the
 * calculator's emulator, by their names) */
s32 port_stat_objects, port_stat_surfaces, port_stat_nodes, port_stat_gfx;
u32 port_stat_pool = 0xFFFFFFFF;
static void port_peaks(void) {
  if (gPrevFrameObjectCount > port_stat_objects) port_stat_objects = gPrevFrameObjectCount;
  if (gSurfacesAllocated - gNumStaticSurfaces > port_stat_surfaces) port_stat_surfaces = gSurfacesAllocated - gNumStaticSurfaces;
  if (gSurfaceNodesAllocated - gNumStaticSurfaceNodes > port_stat_nodes) port_stat_nodes = gSurfaceNodesAllocated - gNumStaticSurfaceNodes;
  if (main_pool_available() < port_stat_pool) port_stat_pool = main_pool_available();
  /* (the frame's display list memory: its commands from the start, its matrices and vertices from the end) */
  s32 gfx = (s32) (gDisplayListHead - port_gfx_pool) + (s32) ((u8 *) (port_gfx_pool + GFX_POOL) - gGfxPoolEnd) / 8;
  if (gfx > port_stat_gfx) port_stat_gfx = gfx;
}

#ifdef HOST
#include <stdio.h>
#include <stdlib.h>
#include "game/mario.h"
#include "engine/math_util.h"
#include "behavior_data.h"
extern struct LakituState gLakituState;
/* the state of the game, for tests (tests/play.c --stats) */
void port_debug_print(void) {
  struct MarioState *m = gMarioState;
  printf("level %d area %d act %d; mario %.0f %.0f %.0f action %08x; camera %.0f %.0f %.0f -> %.0f %.0f %.0f; "
         "transition %d pause %d\n", gCurrLevelNum, gCurrAreaIndex, gCurrActNum, m->pos[0], m->pos[1], m->pos[2],
         (unsigned) m->action, gLakituState.pos[0], gLakituState.pos[1], gLakituState.pos[2], gLakituState.focus[0],
         gLakituState.focus[1], gLakituState.focus[2], gWarpTransition.isActive, gWarpTransition.pauseRendering);
  if (getenv("PORT_OBJECTS"))
    for (int i = 0; i < OBJECT_POOL_CAPACITY; i++)
      if (gObjectPool[i].activeFlags) printf("  obj %ld\n", (long) ((const char *) gObjectPool[i].behavior - (const char *) bhvMario));
  printf("  objects %d of %d (most %d), transforms most %d, surfaces most %d, nodes most %d, pool free %u (least %u), "
         "display list most %d\n", gPrevFrameObjectCount, gObjectPoolCapacity, port_stat_objects, port_transforms_most,
         port_stat_surfaces, port_stat_nodes, (unsigned) main_pool_available(), (unsigned) port_stat_pool, port_stat_gfx);
}

/* puts Mario somewhere (tests/play.c --at), facing yaw */
void port_debug_place(f32 x, f32 y, f32 z, s16 yaw) {
  struct MarioState *m = gMarioState;
  if (m->marioObj == NULL) return;
  vec3f_set(m->pos, x, y, z);
  m->faceAngle[1] = yaw;
  m->vel[1] = 0;
  set_mario_action(m, ACT_FREEFALL, 0);
}
#endif

/* the level a new game starts in (tests start elsewhere) */
int port_start_level_num = LEVEL_CASTLE_GROUNDS;
s32 port_start_level(void) { return port_start_level_num; }

/* ------------------------------------------------------------------------------------------------ the frame */
static struct LevelCommand *sScript;

void init_rcp(void) {}

void port_game_init(void) {
  main_pool_init(sPool, (u8 *) sPool + sizeof sPool);
  gEffectsMemoryPool = mem_pool_init(EFFECTS_SIZE, MEMORY_POOL_LEFT);
  port_objects_init();
  gPlayer1Controller->statusData = NULL;
  gPlayer1Controller->controllerData = &sPad;
  gDisplayListHead = port_gfx_pool;
  gGfxPoolEnd = (u8 *) (port_gfx_pool + GFX_POOL);
  save_file_load_all();
  sScript = (struct LevelCommand *) port_level_script_entry;
}

void port_game_frame(void) {
  read_controller();
  port_menu_input();
  gDisplayListHead = port_gfx_pool;
  gGfxPoolEnd = (u8 *) (port_gfx_pool + GFX_POOL);
  sScript = level_script_execute(sScript);
  gGlobalTimer++;
  port_peaks();
}
