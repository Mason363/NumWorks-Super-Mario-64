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
#include "plat.h"

/* the N64 had 4 MB for this; the levels' scripts, cameras, warps and the objects' surfaces fit in less */
#define POOL_SIZE (36 * 1024)
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
    a->root.numViews = 1;
    a->root.views = views;
    views[0] = &gc->fnNode.node;
    gc->config.mode = (s32) g[3];
    gc->pos[0] = (s16) g[4], gc->pos[1] = (s16) g[5], gc->pos[2] = (s16) g[6];
    gc->focus[0] = (s16) g[7], gc->focus[1] = (s16) g[8], gc->focus[2] = (s16) g[9];
    create_camera(gc, pool);
    a->mesh = (u16) g[1];
    a->fov = (u16) g[10], a->near = (u16) g[11], a->far = (u32) g[12];
    a->background = (u16) g[13];
    a->fog_color = (u32) g[14];
    a->fog_near = (u16) g[15], a->fog_far = (u16) g[16];
    return &a->root.node;
  }
  struct PortNode *n = alloc_only_pool_alloc(pool, sizeof(struct PortNode));
  memset(n, 0, sizeof *n);
  n->node.type = PORT_NODE_MODEL;
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

#ifdef HOST
#include <stdio.h>
#include "game/mario.h"
extern struct LakituState gLakituState;
/* the state of the game, for tests (tests/play.c --stats) */
void port_debug_print(void) {
  struct MarioState *m = gMarioState;
  printf("level %d area %d act %d; mario %.0f %.0f %.0f action %08x; camera %.0f %.0f %.0f -> %.0f %.0f %.0f; "
         "transition %d pause %d\n", gCurrLevelNum, gCurrAreaIndex, gCurrActNum, m->pos[0], m->pos[1], m->pos[2],
         (unsigned) m->action, gLakituState.pos[0], gLakituState.pos[1], gLakituState.pos[2], gLakituState.focus[0],
         gLakituState.focus[1], gLakituState.focus[2], gWarpTransition.isActive, gWarpTransition.pauseRendering);
}
#endif

/* the level a new game starts in (tests start elsewhere) */
int port_start_level_num = LEVEL_CASTLE_GROUNDS;
s32 port_start_level(void) { return port_start_level_num; }

/* ------------------------------------------------------------------------------------------------ the frame */
static struct LevelCommand *sScript;
static Gfx sGfxSink[64];

void init_rcp(void) {
  gDisplayListHead = sGfxSink;
  gGfxPoolEnd = (u8 *) sGfxSink;   /* (so alloc_display_list finds no room: nothing draws with it) */
}

void port_game_init(void) {
  main_pool_init(sPool, (u8 *) sPool + sizeof sPool);
  gPlayer1Controller->statusData = NULL;
  gPlayer1Controller->controllerData = &sPad;
  gDisplayListHead = sGfxSink;
  save_file_load_all();
  sScript = (struct LevelCommand *) port_level_script_entry;
}

void port_game_frame(void) {
  read_controller();
  gDisplayListHead = sGfxSink;
  sScript = level_script_execute(sScript);
  gGlobalTimer++;
}
