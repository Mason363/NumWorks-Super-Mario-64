/* The objects' memory on the calculator. The pool takes the RAM after the game's own (a calculator whose software
 * leaves apps more RAM has more objects, up to the N64's 240); an object's matrix, which most never use, comes
 * from a small pool the first time it is used (types.h: OBJ_TRANSFORM). */
#include <string.h>
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "engine/math_util.h"
#include "game/object_list_processor.h"
#include "port_game.h"
#include "plat.h"

#define MAX_OBJECTS 240
#define TRANSFORMS 64
/* a computer's objects are bigger (64-bit pointers): tests have as many as the calculator with the least RAM */
#ifndef HOST_OBJECTS
#define HOST_OBJECTS 160
#endif

static Mat4 sTransforms[TRANSFORMS];
static u8 sFree[TRANSFORMS], sNumFree;
static Mat4 sSpare;   /* (when every one is taken: shared) */
int port_transforms_most;

void port_objects_init(void) {
  uint32_t len;
  u8 *mem = plat_heap(&len);
  uintptr_t a = ((uintptr_t) mem + 7) & ~(uintptr_t) 7;
  len -= (uint32_t) (a - (uintptr_t) mem);
  u32 n = len / sizeof(struct Object);
#ifdef HOST
  if (n > HOST_OBJECTS) n = HOST_OBJECTS;
#endif
  gObjectPool = (struct Object *) a;
  gObjectPoolCapacity = n > MAX_OBJECTS ? MAX_OBJECTS : (s32) n;
  memset(gObjectPool, 0, gObjectPoolCapacity * sizeof(struct Object));
  port_obj_transforms_reset();
}

void port_obj_transforms_reset(void) {
  for (int i = 0; i < TRANSFORMS; i++) sFree[i] = (u8) (TRANSFORMS - 1 - i);
  sNumFree = TRANSFORMS;
}

f32 (*port_obj_transform(struct Object *obj))[4] {
  Mat4 *m = &sSpare;
  if (sNumFree > 0) {
    m = &sTransforms[sFree[--sNumFree]];
    if (TRANSFORMS - sNumFree > port_transforms_most) port_transforms_most = TRANSFORMS - sNumFree;
  }
  mtxf_identity(*m);
  return obj->transform = *m;
}

void port_obj_transform_free(struct Object *obj) {
  Mat4 *m = (Mat4 *) obj->transform;
  if (m >= sTransforms && m < sTransforms + TRANSFORMS) sFree[sNumFree++] = (u8) (m - sTransforms);
  obj->transform = NULL;
}
