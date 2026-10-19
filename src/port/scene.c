/* Drawing the game's world: what the decompilation's rendering_graph_node.c did (it walked the area's graph and
 * made the N64's display lists), for the calculator's renderer. The area's mesh, then each object of the area
 * with its model (src/data.bin: its geo layout as a list of nodes, see data.h), its animation, its shadow and
 * what it holds, through the same matrices (the camera's space, as the game's matrix stack), the same culling
 * and the game's own callbacks (src/port/gen/geo_funcs.c), called with nodes made for the occasion. */
#include <string.h>
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "game/area.h"
#include "game/game_init.h"
#include "game/camera.h"
#include "game/level_update.h"
#include "game/object_list_processor.h"
#include "game/object_helpers.h"
#include "game/mario.h"
#include "game/mario_misc.h"
#include "game/shadow.h"
#include "engine/math_util.h"
#include "engine/graph_node.h"
#include "engine/surface_collision.h"
#include "behavior_data.h"
#include "port.h"
#include "port_game.h"
#include "port_gfx.h"
#include "port_menu.h"
#include "../render.h"

extern struct LakituState gLakituState;
extern const GraphNodeFunc port_geo_funcs[];

/* the game's renderer's globals (some of its code reads them) */
struct GraphNodeRoot *gCurGraphNodeRoot;
struct GraphNodeMasterList *gCurGraphNodeMasterList;
struct GraphNodePerspective *gCurGraphNodeCamFrustum;
struct GraphNodeCamera *gCurGraphNodeCamera;
struct GraphNodeObject *gCurGraphNodeObject;
struct GraphNodeHeldObject *gCurGraphNodeHeldObject;

/* the animation being drawn (its gCurrAnim* globals) */
enum { ANIM_TYPE_NONE, ANIM_TYPE_TRANSLATION, ANIM_TYPE_VERTICAL_TRANSLATION, ANIM_TYPE_LATERAL_TRANSLATION,
       ANIM_TYPE_NO_TRANSLATION, ANIM_TYPE_ROTATION };
typedef struct {
  s16 type, frame;
  u8 enabled;
  f32 mul;
  u16 *attr;
  s16 *data;
} AnimState;
static AnimState sAnim;

static Mat4 sCamera;                       /* the camera's look-at matrix: the stack's bottom */
static struct GraphNodePerspective sFrustum;
static u8 sAlpha;                          /* the object's opacity (geo_update_layer_transparency) */

/* the matrix stack, as deep as models nest (a held object's inside its holder's) */
#define DEPTH 40
typedef struct {
  Mat4 m;
  s16 node;          /* whose matrix it is */
  s16 selected;      /* a switch's case */
  u8 skip;           /* its children are not drawn */
  u8 wings;          /* (geo_switch_mario_cap_on_off: its translation-rotation children shown or not) */
} Level;
static Level *sStack;

/* ------------------------------------------------------------------------------------------------ meshes */
static void add_mesh(u16 mesh, Mat4 m) {
  if (mesh == 0xFFFF || mesh >= HDR->nmesh) return;
  /* from the game's camera space (z backwards) to the renderer's (z forwards) */
  Mtx43 c;
  for (int i = 0; i < 4; i++) c.m[i][0] = m[i][0], c.m[i][1] = m[i][1], c.m[i][2] = -m[i][2];
  r_mesh(MESH(mesh), &c, 0xFFFFFF, sAlpha, R_CAMERA);
}

/* ------------------------------------------------------------------------------------------------ shadows */
/* create_shadow_below_xyz's: how big and dark, on what floor */
static void draw_shadow(f32 x, f32 y, f32 z, s16 scale, u8 solidity, s8 type) {
  struct FloorGeometry *floor;
  f32 h = find_floor_height_and_data(x, y, z, &floor);
  f32 nx = 0, ny = 1, nz = 0;
  f32 water = find_water_level(x, z);
  if (water >= FLOOR_LOWER_LIMIT_SHADOW && y >= water && water >= h) {
    h = water;
    if (type >= SHADOW_SQUARE_PERMANENT && type != SHADOW_CIRCLE_PLAYER) solidity = 200;
  } else {
    if (h < FLOOR_LOWER_LIMIT_SHADOW || floor == NULL || floor->normalY <= 0) return;
    nx = floor->normalX, ny = floor->normalY, nz = floor->normalZ;
  }
  f32 dist = y - h, radius;
  bool square = false;
  if (type == SHADOW_SQUARE_PERMANENT || type == SHADOW_SQUARE_SCALABLE || type == SHADOW_SQUARE_TOGGLABLE ||
      (type >= SHADOW_RECTANGLE_HARDCODED_OFFSET && type != SHADOW_CIRCLE_PLAYER)) {
    square = true;
    nx = 0, ny = 1, nz = 0;
    if (type == SHADOW_SQUARE_PERMANENT) radius = scale / 2;
    else if (type == SHADOW_SQUARE_TOGGLABLE) radius = dist >= 600 ? 0 : scale / 2.0f;
    else radius = (dist <= 0 ? scale : dist >= 600 ? scale * 0.5f : scale * (1 - 0.5f * dist / 600)) / 2;
  } else {
    if (type == SHADOW_CIRCLE_PLAYER && gMarioObject != NULL) {
      /* (correct_shadow_solidity_for_animations: none on a ledge) */
      s16 anim = gMarioObject->header.gfx.animInfo.animID;
      if (anim == MARIO_ANIM_IDLE_ON_LEDGE) return;
    }
    if (solidity >= 121 && dist > 0) solidity = dist >= 600 ? 120 : (u8) ((120 - solidity) * dist / 600 + solidity);
    radius = (dist <= 0 ? scale : dist >= 600 ? scale * 0.5f : scale * (1 - 0.5f * dist / 600)) / 2;
  }
  if (radius <= 0 || solidity == 0) return;
  Vec3f p = {x, h, z}, n = {nx, ny, nz};
  r_shadow(p, n, radius, (u8) (solidity * sAlpha / 255), square);
}

/* ------------------------------------------------------------------------------------------------ models */
static void set_anim(struct AnimInfo *a, s32 has_anim) {
  struct Animation *anim = a->curAnim;
  if (has_anim) a->animFrame = geo_update_animation_frame(a, &a->animFrameAccelAssist);
  a->animTimer = gAreaUpdateCounter;
  if (anim->flags & ANIM_FLAG_HOR_TRANS) sAnim.type = ANIM_TYPE_VERTICAL_TRANSLATION;
  else if (anim->flags & ANIM_FLAG_VERT_TRANS) sAnim.type = ANIM_TYPE_LATERAL_TRANSLATION;
  else if (anim->flags & ANIM_FLAG_6) sAnim.type = ANIM_TYPE_NO_TRANSLATION;
  else sAnim.type = ANIM_TYPE_TRANSLATION;
  sAnim.frame = a->animFrame;
  sAnim.enabled = (anim->flags & ANIM_FLAG_5) == 0;
  sAnim.attr = (u16 *) anim->index;
  sAnim.data = (s16 *) anim->values;
  sAnim.mul = anim->animYTransDivisor == 0 ? 1.0f : (f32) a->animYTrans / (f32) anim->animYTransDivisor;
}

#define ANIM_VALUE() port_anim_value(sAnim.frame, &sAnim.attr, sAnim.data)
#define CH PORT_ANIM_CHANNEL

/* geo_process_animated_part's: the part's translation and rotation from the animation */
static void animated_part(const MNode *n, Mat4 out, Mat4 parent) {
  Vec3s r = {0, 0, 0};
  Vec3f t = {n->t[0], n->t[1], n->t[2]};
  if (sAnim.type == ANIM_TYPE_TRANSLATION) {
    t[0] += ANIM_VALUE() * sAnim.mul;
    t[1] += ANIM_VALUE() * sAnim.mul;
    t[2] += ANIM_VALUE() * sAnim.mul;
    sAnim.type = ANIM_TYPE_ROTATION;
  } else if (sAnim.type == ANIM_TYPE_LATERAL_TRANSLATION) {
    t[0] += ANIM_VALUE() * sAnim.mul;
    sAnim.attr += CH;
    t[2] += ANIM_VALUE() * sAnim.mul;
    sAnim.type = ANIM_TYPE_ROTATION;
  } else if (sAnim.type == ANIM_TYPE_VERTICAL_TRANSLATION) {
    sAnim.attr += CH;
    t[1] += ANIM_VALUE() * sAnim.mul;
    sAnim.attr += CH;
    sAnim.type = ANIM_TYPE_ROTATION;
  } else if (sAnim.type == ANIM_TYPE_NO_TRANSLATION) {
    sAnim.attr += 3 * CH;
    sAnim.type = ANIM_TYPE_ROTATION;
  }
  if (sAnim.type == ANIM_TYPE_ROTATION) {
    r[0] = ANIM_VALUE();
    r[1] = ANIM_VALUE();
    r[2] = ANIM_VALUE();
  }
  Mat4 local;
  mtxf_rotate_xyz_and_translate(local, t, r);
  mtxf_mul(out, local, parent);
}

/* geo_process_shadow's: under the object (or what holds it), moved with the animation's own translation;
 * child: its first child (a scale node scales that translation) */
static void shadow_node(const MNode *n, const MNode *child, Mat4 cur) {
  if (gCurGraphNodeObject == NULL) return;
  Vec3f pos;
  f32 scale;
  if (gCurGraphNodeHeldObject != NULL) {
    get_pos_from_transform_mtx(pos, cur, sCamera);
    scale = n->t[1];
  } else {
    vec3f_copy(pos, gCurGraphNodeObject->pos);
    scale = n->t[1] * gCurGraphNodeObject->scale[0];
  }
  if (sAnim.enabled && (sAnim.type == ANIM_TYPE_TRANSLATION || sAnim.type == ANIM_TYPE_LATERAL_TRANSLATION)) {
    f32 objScale = child != NULL && child->kind == N_SCALE ? child->scale / 65536.0f : 1;
    f32 ox = ANIM_VALUE() * sAnim.mul * objScale;
    sAnim.attr += CH;
    f32 oz = ANIM_VALUE() * sAnim.mul * objScale;
    sAnim.attr -= 3 * CH;
    f32 s = sins(gCurGraphNodeObject->angle[1]), c = coss(gCurGraphNodeObject->angle[1]);
    pos[0] += ox * c + oz * s;
    pos[2] += -ox * s + oz * c;
  }
  draw_shadow(pos[0], pos[1], pos[2], (s16) scale, (u8) n->t[0], (s8) n->arg);
}

static void walk_model(int model, int base);

/* geo_process_held_object's: the held object drawn at the hand, with its own model and animation */
static void held_node(const MNode *n, Level *L, int top) {
  struct GraphNodeHeldObject h;
  memset(&h, 0, sizeof h);
  h.fnNode.node.type = GRAPH_NODE_TYPE_HELD_OBJ;
  h.fnNode.node.flags = GRAPH_RENDER_ACTIVE;
  h.fnNode.node.next = h.fnNode.node.prev = &h.fnNode.node;
  h.playerIndex = n->arg;
  vec3s_set(h.translation, n->t[0], n->t[1], n->t[2]);
  GraphNodeFunc fn = port_geo_funcs[n->fn];
  if (fn != NULL) fn(GEO_CONTEXT_RENDER, &h.fnNode.node, L->m);
  struct Object *held = h.objNode;
  if (held == NULL || held->header.gfx.sharedChild == NULL || top + 1 >= DEPTH) return;
  int model = port_model_of(held->header.gfx.sharedChild);
  if (model == 0xFFFF || model >= HDR->nmodel) return;
  s32 has_anim = (held->header.gfx.node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0;
  Mat4 t;
  Vec3f tr = {h.translation[0] / 4.0f, h.translation[1] / 4.0f, h.translation[2] / 4.0f};
  Level *H = &sStack[top + 1];
  mtxf_translate(t, tr);
  mtxf_copy(H->m, *gCurGraphNodeObject->throwMatrix);
  H->m[3][0] = L->m[3][0], H->m[3][1] = L->m[3][1], H->m[3][2] = L->m[3][2];
  mtxf_mul(H->m, t, H->m);
  mtxf_scale_vec3f(H->m, H->m, held->header.gfx.scale);
  if (fn != NULL) fn(GEO_CONTEXT_HELD_OBJ, &h.fnNode.node, (struct AllocOnlyPool *) H->m);
  AnimState saved = sAnim;
  u8 alpha = sAlpha;
  sAnim.type = ANIM_TYPE_NONE;
  sAlpha = 255;
  gCurGraphNodeHeldObject = &h;
  if (held->header.gfx.animInfo.curAnim != NULL) set_anim(&held->header.gfx.animInfo, has_anim);
  walk_model(model, top + 1);
  gCurGraphNodeHeldObject = NULL;
  sAnim = saved;
  sAlpha = alpha;
}

/* the nodes of a model, under the matrix at sStack[base] */
static void walk_model(int model, int base) {
  const MModel *mm = MODEL(model);
  const MNode *nodes = DAT(MNode, mm->nodes);
  int top = base;
  sStack[base].node = -1, sStack[base].skip = 0, sStack[base].wings = 1;
  /* what a callback did to the node after it (geo_mario_tilt_torso and others turn or scale it) */
  int over_node = -1;
  union {
    struct GraphNodeRotation rot;
    struct GraphNodeScale scale;
    struct GraphNodeTranslationRotation tr;
  } over;
  for (int i = 0; i < mm->nnodes;) {
    const MNode *n = &nodes[i];
    int p = n->parent == 0xFFFF ? -1 : n->parent;
    while (top > base && sStack[top].node != p) top--;
    Level *L = &sStack[top];
    /* (not drawn: the node and what is below it) */
    if ((p >= 0 && nodes[p].kind == N_SWITCH && n->cse != L->selected) || (n->kind == N_TRANSROT && !L->wings) ||
        top + 1 >= DEPTH) {
      i = n->end;
      continue;
    }
    Level *C = &sStack[top + 1];
    C->node = (s16) i, C->skip = 0, C->selected = 0, C->wings = 1;
    bool has_over = over_node == i;
    Vec3s rot = {n->r[0], n->r[1], n->r[2]};
    Vec3f tr = {n->t[0], n->t[1], n->t[2]};
    f32 scale = n->scale / 65536.0f;
    if (has_over) {
      if (n->kind == N_ROTATE) vec3s_copy(rot, over.rot.rotation);
      else if (n->kind == N_SCALE) scale = over.scale.scale;
      else if (n->kind == N_TRANSROT || n->kind == N_TRANSLATE) {
        vec3s_to_vec3f(tr, over.tr.translation);
        vec3s_copy(rot, over.tr.rotation);
      }
    }
    switch (n->kind) {
      case N_PART:
        animated_part(n, C->m, L->m);
        break;
      case N_TRANSLATE:
      case N_TRANSROT:
      case N_ROTATE: {
        Mat4 local;
        Vec3f zero = {0, 0, 0};
        Vec3s none = {0, 0, 0};
        mtxf_rotate_zxy_and_translate(local, n->kind == N_ROTATE ? zero : tr, n->kind == N_TRANSLATE ? none : rot);
        mtxf_mul(C->m, local, L->m);
        break;
      }
      case N_SCALE: {
        Vec3f s = {scale, scale, scale};
        mtxf_scale_vec3f(C->m, L->m, s);
        break;
      }
      case N_BILLBOARD:
        mtxf_billboard(C->m, L->m, tr, gCurGraphNodeCamera->roll);
        if (gCurGraphNodeHeldObject != NULL) mtxf_scale_vec3f(C->m, C->m, gCurGraphNodeHeldObject->objNode->header.gfx.scale);
        else if (gCurGraphNodeObject != NULL) mtxf_scale_vec3f(C->m, C->m, gCurGraphNodeObject->scale);
        break;
      case N_RANGE: {
        /* geo_process_level_of_detail: by the distance in front of the camera */
        s16 d = (s16) -L->m[3][2];
        mtxf_copy(C->m, L->m);
        if (!(n->t[0] <= d && d < n->t[1])) C->skip = 1;
        break;
      }
      case N_SWITCH: {
        struct GraphNodeSwitchCase sw;
        memset(&sw, 0, sizeof sw);
        sw.fnNode.node.type = GRAPH_NODE_TYPE_SWITCH_CASE;
        sw.fnNode.node.flags = GRAPH_RENDER_ACTIVE;
        sw.fnNode.node.next = sw.fnNode.node.prev = &sw.fnNode.node;
        sw.numCases = n->arg;
        GraphNodeFunc fn = port_geo_funcs[n->fn];
        if (fn != NULL) fn(GEO_CONTEXT_RENDER, &sw.fnNode.node, L->m);
        C->selected = sw.selectedCase;
        if (fn == (GraphNodeFunc) geo_switch_mario_cap_on_off) C->wings = (gBodyStates[n->arg].capState & 2) != 0;
        mtxf_copy(C->m, L->m);
        /* (the wings are the cap switch's siblings: what it decided goes to their parent) */
        if (fn == (GraphNodeFunc) geo_switch_mario_cap_on_off) L->wings = C->wings;
        break;
      }
      case N_ASM: {
        struct GraphNodeGenerated g;
        memset(&g, 0, sizeof g);
        g.fnNode.node.type = GRAPH_NODE_TYPE_GENERATED_LIST;
        g.fnNode.node.flags = GRAPH_RENDER_ACTIVE | (1 << 8);
        g.parameter = n->scale;
        /* the node after it, which it may change */
        const MNode *next = n->end < mm->nnodes && nodes[n->end].parent == n->parent ? &nodes[n->end] : NULL;
        memset(&over, 0, sizeof over);
        if (next != NULL) {
          vec3s_set(over.tr.translation, next->t[0], next->t[1], next->t[2]);
          vec3s_set(over.tr.rotation, next->r[0], next->r[1], next->r[2]);
          if (next->kind == N_ROTATE) vec3s_set(over.rot.rotation, next->r[0], next->r[1], next->r[2]);
          if (next->kind == N_SCALE) over.scale.scale = next->scale / 65536.0f;
          g.fnNode.node.next = &over.rot.node;
        } else g.fnNode.node.next = &g.fnNode.node;
        GraphNodeFunc fn = port_geo_funcs[n->fn];
        if (fn != NULL) fn(GEO_CONTEXT_RENDER, &g.fnNode.node, L->m);
        if (next != NULL) over_node = (int) (next - nodes);
        /* (the callbacks that fade what follows: their display list sets its alpha) */
        if (fn == (GraphNodeFunc) geo_update_layer_transparency) {
          struct Object *o = gCurGraphNodeHeldObject ? gCurGraphNodeHeldObject->objNode : (struct Object *) gCurGraphNodeObject;
          sAlpha = (u8) o->oOpacity;
        } else if (fn == (GraphNodeFunc) geo_mirror_mario_set_alpha) {
          struct MarioBodyState *b = &gBodyStates[n->scale & 1];
          sAlpha = (b->modelState & 0x100) ? (u8) (b->modelState & 0xFF) : 255;
        }
        mtxf_copy(C->m, L->m);
        break;
      }
      case N_SHADOW:
        mtxf_copy(C->m, L->m);
        shadow_node(n, i + 1 < n->end ? &nodes[i + 1] : NULL, L->m);
        break;
      case N_HELD:
        mtxf_copy(C->m, L->m);
        held_node(n, L, top + 1);
        break;
      default:
        mtxf_copy(C->m, L->m);
        break;
    }
    if (C->skip) {
      i = n->end;
      continue;
    }
    if (n->mesh != 0xFFFF) add_mesh(n->mesh, C->m);
    top++;
    i++;
  }
}

/* geo_process_object's: its matrix, animation and culling, then its model */
static bool in_view(struct GraphNodeObject *g, Mat4 m, int model) {
  if (g->node.flags & GRAPH_RENDER_INVISIBLE) return false;
  s16 halfFov = (s16) ((sFrustum.fov / 2.0f + 1.0f) * 32768.0f / 180.0f + 0.5f);
  f32 edge = -m[3][2] * sins(halfFov) / coss(halfFov);
  s16 radius = MODEL(model)->cull;
  if (m[3][2] > -100.0f + radius) return false;
  if (m[3][2] < -20000.0f - radius) return false;
  if (m[3][0] > edge + radius || m[3][0] < -edge - radius) return false;
  return true;
}

static void draw_object(struct Object *o) {
  struct GraphNodeObject *g = &o->header.gfx;
  int model = port_model_of(g->sharedChild);
  s32 has_anim = (g->node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0;
  Level *L = &sStack[1];
  Mat4 local;
  if (g->throwMatrix != NULL) mtxf_mul(L->m, *g->throwMatrix, sCamera);
  else if (g->node.flags & GRAPH_RENDER_BILLBOARD) mtxf_billboard(L->m, sCamera, g->pos, gCurGraphNodeCamera->roll);
  else {
    mtxf_rotate_zxy_and_translate(local, g->pos, g->angle);
    mtxf_mul(L->m, local, sCamera);
  }
  mtxf_scale_vec3f(L->m, L->m, g->scale);
  g->throwMatrix = &L->m;
  g->cameraToObject[0] = L->m[3][0], g->cameraToObject[1] = L->m[3][1], g->cameraToObject[2] = L->m[3][2];
  sAnim.type = ANIM_TYPE_NONE;
  if (g->animInfo.curAnim != NULL) set_anim(&g->animInfo, has_anim);
  if (model != 0xFFFF && model < HDR->nmodel && in_view(g, L->m, model)) {
    gCurGraphNodeObject = g;
    sAlpha = 255;
    walk_model(model, 1);
    gCurGraphNodeObject = NULL;
  }
  sAnim.type = ANIM_TYPE_NONE;
  g->throwMatrix = NULL;
}

/* ------------------------------------------------------------------------------------------------ the area */
void port_draw_area(struct GraphNodeRoot *root) {
  struct PortArea *a = (struct PortArea *) root;
  struct GraphNodeCamera *gc = a->camera;
  gCurGraphNodeRoot = root;
  gCurGraphNodeCamera = gc;
  /* the camera node's own function moves it where Lakitu is (geo_camera_main) */
  geo_camera_main(GEO_CONTEXT_RENDER, &gc->fnNode.node, NULL);
  mtxf_lookat(sCamera, gc->pos, gc->focus, gc->roll);
  gc->matrixPtr = &sCamera;
  /* the frustum, and its function's field of view (geo_camera_fov) */
  memset(&sFrustum, 0, sizeof sFrustum);
  sFrustum.fnNode.node.type = GRAPH_NODE_TYPE_PERSPECTIVE;
  sFrustum.fov = a->fov ? a->fov : 45;
  sFrustum.near = a->near, sFrustum.far = (s16) a->far;
  geo_camera_fov(GEO_CONTEXT_RENDER, &sFrustum.fnNode.node, NULL);
  gCurGraphNodeCamFrustum = &sFrustum;
  Mtx43 view;
  for (int i = 0; i < 4; i++) view.m[i][0] = sCamera[i][0], view.m[i][1] = sCamera[i][1], view.m[i][2] = -sCamera[i][2];
  r_frame(&view, gc->rollScreen * (M_PI_F / 32768.0f), sFrustum.fov, a->near ? a->near : 100,
          a->far ? (float) a->far : 20000);
  r_sky(a->background, (s16) atan2s(gc->focus[2] - gc->pos[2], gc->focus[0] - gc->pos[0]));
  if (a->fog_color) r_fog((u16) a->fog_color, a->fog_near, a->fog_far);
  if (a->mesh != 0xFFFF && a->mesh < HDR->nmesh) r_mesh(MESH(a->mesh), NULL, 0xFFFFFF, 255, 0);
  if (a->rooms) {
    /* the room Mario is in (geo_switch_area, which also tells the objects) */
    struct GraphNodeSwitchCase sw;
    memset(&sw, 0, sizeof sw);
    geo_switch_area(GEO_CONTEXT_RENDER, &sw.fnNode.node, NULL);
    const u16 *r = a->rooms + 1;
    for (int i = 0; i < sw.selectedCase && i < a->rooms[0]; i++) r += 1 + r[0];
    if (sw.selectedCase >= 0 && sw.selectedCase < a->rooms[0])
      for (int i = 1; i <= r[0]; i++)
        if (r[i] < HDR->nmesh) r_mesh(MESH(r[i]), NULL, 0xFFFFFF, 255, 0);
  }
  sAlpha = 255;
  /* the objects (GEO_RENDER_OBJ: every one of the area's, in the order they were made); the matrix stack in the
     renderer's memory that is free until it draws */
  int room;
  sStack = r_scratch(&room);
  if (sStack == NULL || room < (int) (DEPTH * sizeof(Level))) return;
  mtxf_copy(sStack[0].m, sCamera);
  for (int i = 0; i < OBJECT_POOL_CAPACITY; i++) {
    struct Object *o = &gObjectPool[i];
    struct GraphNodeObject *g = &o->header.gfx;
    if (!(g->node.flags & GRAPH_RENDER_ACTIVE)) {
      g->throwMatrix = NULL;
      continue;
    }
    if (g->areaIndex == root->areaIndex) draw_object(o);
  }
  sStack = NULL;
  gCurGraphNodeCamFrustum = NULL;
  gCurGraphNodeCamera = NULL;
  gCurGraphNodeRoot = NULL;
}

void geo_process_root(struct GraphNodeRoot *node, Vp *b, Vp *c, s32 clearColor) {
  if (node->node.flags & GRAPH_RENDER_ACTIVE) port_draw_area(node);
}

/* the screens' display list (the HUD, the menus, the text and the transitions), over each band */
static void run_screens(const void *cmd) { port_gfx_run(*(const Gfx *const *) cmd); }

/* the frame: everything the renderer needs is here, on the stack, until the frame is on the screen */
void port_render(void) {
  RFrame f;
  r_begin(&f);
  Mtx43 view;
  mtx_identity(&view);
  r_frame(&view, 0, 45, 100, 20000);
  init_rcp();
  Gfx *screens = gDisplayListHead;
  render_game();
  port_menu_draw();
  gSPEndDisplayList(gDisplayListHead++);
  const Gfx **cmd = r_overlay_alloc(run_screens, sizeof(const Gfx *));
  if (cmd != NULL) *cmd = screens;
  r_draw();
}
