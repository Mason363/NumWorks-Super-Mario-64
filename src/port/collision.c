/* The surfaces on the calculator: the level's stay in flash (src/port/gen/terrain.c), listed by number in each
 * cell, the objects' are linked in RAM as the game did; and neither keeps its normal, worked out here when it is
 * read, exactly as surface_load.c's read_surface_data did when it made the surface. */
#include <math.h>
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "engine/surface_load.h"
#include "port.h"

/* (the last surface asked about: its normal is read a coordinate at a time) */
static const struct Surface *sLast;
static f32 sNormal[4];

void port_surface_changed(void) { sLast = NULL; }

/* the water's pseudo floor (mario_step.c): flat, its origin offset set where it is used */
extern struct Surface gWaterSurfacePseudoFloor;
f32 port_water_floor_offset;

f32 port_surface_normal(const struct Surface *surf, s32 i) {
  if (surf == &gWaterSurfacePseudoFloor) return i == 1 ? 1.0f : i == 3 ? port_water_floor_offset : 0.0f;
  if (surf != sLast) {
    const s16 *v1 = SURF_V(surf, 1), *v2 = SURF_V(surf, 2), *v3 = SURF_V(surf, 3);
    s32 x1 = v1[0], y1 = v1[1], z1 = v1[2];
    s32 x2 = v2[0], y2 = v2[1], z2 = v2[2];
    s32 x3 = v3[0], y3 = v3[1], z3 = v3[2];
    f32 nx = (y2 - y1) * (z3 - z2) - (z2 - z1) * (y3 - y2);
    f32 ny = (z2 - z1) * (x3 - x2) - (x2 - x1) * (z3 - z2);
    f32 nz = (x2 - x1) * (y3 - y2) - (y2 - y1) * (x3 - x2);
    f32 mag = sqrtf(nx * nx + ny * ny + nz * nz);
    mag = (f32) (1.0 / mag);
    nx *= mag;
    ny *= mag;
    nz *= mag;
    sNormal[0] = nx, sNormal[1] = ny, sNormal[2] = nz;
    sNormal[3] = -(nx * x1 + ny * y1 + nz * z1);
    sLast = surf;
  }
  return sNormal[i];
}

const s16 *port_level_vertex(u16 i) { return gPortTerrain->vertices[i]; }

/* a level's surface's lowest and highest y, 5 units further (as surface_load.c made them) */
s32 port_level_surface_y(const struct Surface *surf, s32 upper) {
  s32 a = gPortTerrain->vertices[surf->u.corner[0]][1], b = gPortTerrain->vertices[surf->u.corner[1]][1];
  s32 c = gPortTerrain->vertices[surf->u.corner[2]][1];
  if (upper) return (a > b ? (a > c ? a : c) : (b > c ? b : c)) + 5;
  return (a < b ? (a < c ? a : c) : (b < c ? b : c)) - 5;
}

struct Surface *port_surface_next(struct PortSurfaceList *list) {
  if (list->node != NULL) {
    struct Surface *s = list->node->surface;
    list->node = list->node->next;
    return s;
  }
  const struct PortLevelSurface *all = gPortTerrain->surfaces;
  s32 run = list->run < list->run_end, more = list->at < list->end;
  if (run && more && list->dir) {
    /* (the one the game linked first: higher floor, lower ceiling) */
    s32 a = gPortTerrain->vertices[all[list->run].corner[0]][1] * list->dir;
    s32 b = gPortTerrain->vertices[all[*list->at].corner[0]][1] * list->dir;
    if (b > a) run = FALSE;
  }
  if (run) return (struct Surface *) &all[list->run++];
  if (more) return (struct Surface *) &all[*list->at++];
  return NULL;
}

struct PortSurfaceList port_dynamic_list(int cellZ, int cellX, int kind) {
  struct PortSurfaceList l = {gDynamicSurfaceHeads[cellZ][cellX][kind], 0, 0, NULL, NULL, 0};
  return l;
}

struct PortSurfaceList port_static_list(int cellZ, int cellX, int kind) {
  int i = (cellZ * 16 + cellX) * 3 + kind;
  const u16 *c = gPortTerrain->cells + i, *r = gPortTerrain->runs + i;
  struct PortSurfaceList l = {NULL, r[0], r[1], gPortTerrain->index + c[0], gPortTerrain->index + c[1],
                              kind == SPATIAL_PARTITION_FLOORS ? 1 : kind == SPATIAL_PARTITION_CEILS ? -1 : 0};
  return l;
}
