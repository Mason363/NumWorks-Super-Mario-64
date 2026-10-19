#ifndef SURFACE_LOAD_H
#define SURFACE_LOAD_H

#include <PR/ultratypes.h>

#include "surface_collision.h"
#include "types.h"

#define NUM_CELLS       (2 * LEVEL_BOUNDARY_MAX / CELL_SIZE)
#define NUM_CELLS_INDEX (NUM_CELLS - 1)

struct SurfaceNode {
    struct SurfaceNode *next;
    struct Surface *surface;
};

enum {
    SPATIAL_PARTITION_FLOORS,
    SPATIAL_PARTITION_CEILS,
    SPATIAL_PARTITION_WALLS
};

typedef struct SurfaceNode SpatialPartitionCell[3];

// Needed for bs bss reordering memes.
extern s32 unused8038BE90;

/* (calculator) the level's surfaces are cut into cells beforehand and stay in flash (tools/port.py,
   src/port/gen/terrain.c): the game only points at the area's. The objects' cells keep only the first node of
   their lists (the rest of an N64 node was unused there) */
extern const struct PortTerrain *gPortTerrain;
extern struct SurfaceNode *gDynamicSurfaceHeads[NUM_CELLS][NUM_CELLS][3];
#define DYNAMIC_CELL_LIST(z, x, k) ((struct SurfaceNode *) &gDynamicSurfaceHeads[z][x][k])
extern struct SurfaceNode *sSurfaceNodePool;
extern struct Surface *sSurfacePool;
extern s16 sSurfacePoolSize;

void alloc_surface_pools(void);
#ifdef NO_SEGMENTED_MEMORY
u32 get_area_terrain_size(TerrainData *data);
#endif
void load_area_terrain(s16 index, TerrainData *data, RoomData *surfaceRooms, s16 *macroObjects);
void clear_dynamic_surfaces(void);
void load_object_collision_model(void);

#endif // SURFACE_LOAD_H
