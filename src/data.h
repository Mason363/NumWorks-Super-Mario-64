/* The game's data (src/data.bin, made by tools/pack.py): meshes, textures, models and their animations,
 * collision, objects and texts of the levels. It stays in flash and is read in place; every table is found
 * through the header at its start. */
#ifndef SM_DATA_H
#define SM_DATA_H
#include <stdint.h>
#include "data_ids.h"

extern const uint8_t sm_data[];
#define DAT(type, off) ((const type *)(const void *)(sm_data + (off)))

/* a vertex: position, texture coordinates (texels x 32), colour (or, lit, its normal) and alpha */
typedef struct {
  int16_t x, y, z, s, t;
  uint8_t c[4];
} MVert;

/* how a batch of triangles is drawn */
enum {
  M_TEX = 1,        /* textured */
  M_SHADE = 2,      /* the texture times the vertex colours (else the texture alone) */
  M_CUTOUT = 4,     /* the texture's transparent texels are holes */
  M_XLU = 8,        /* see-through: blended over what is behind */
  M_DECAL = 16,     /* on a surface: drawn over it even at the same depth */
  M_LIT = 32,       /* the vertices carry normals: lit with the scene's light */
  M_FOG = 64,
  M_TEXGEN = 128,   /* the texture coordinates come from the normals (shiny metal) */
};
/* texture wrap modes, per axis (Batch.wrap: s in bits 0-1, t in bits 2-3) */
enum { W_WRAP = 0, W_MIRROR = 1, W_CLAMP = 2 };

typedef struct {
  uint16_t tex;     /* index in the texture table (when M_TEX) */
  uint8_t mode, wrap;
  uint16_t tri0, ntri;   /* in the mesh's triangles */
  uint16_t v0, nv;       /* the vertices they use */
  int16_t lo[3], hi[3];  /* their bounds */
  uint8_t light[6];      /* lit: the ambient colour, then the light's */
} Batch;

typedef struct {
  uint32_t vtx, tri, batch;   /* offsets: MVert[], uint16_t[3] each, Batch[] */
  uint16_t nvtx, ntri, nbatch, flags;
  int16_t lo[3], hi[3];
} Mesh;

/* a texture: texels in flash, w and h powers of two */
enum { TF_565, TF_565K, TF_IA };   /* opaque; with a transparent colour (TEX_KEY); intensity and alpha */
#define TEX_KEY 0x0020
typedef struct {
  uint32_t off;
  uint8_t wl, hl, fmt, pad;   /* log2 of the size */
} Tex;

/* a model: a tree of parts (the N64's animated parts and their display lists) */
typedef struct {
  int8_t parent;      /* -1: the root */
  uint8_t anim;       /* takes the animation's rotation (1) or not (0) */
  uint8_t sw, swcase; /* drawn only when switch sw (0: none) is on case swcase */
  int16_t t[3];       /* translation from its parent */
  int16_t r[3];       /* fixed rotation (angles, 0x10000 a turn) */
  uint16_t mesh;      /* 0xFFFF: none */
  uint16_t scale;     /* x 1/256 */
} Part;
typedef struct {
  uint32_t parts;
  uint8_t nparts, nanimated, shadow, pad;
  uint16_t scale;     /* x 1/256 */
  int16_t radius;
} Model;

/* an animation, as the game keeps them: per animated value an index (frames, start) into the values */
typedef struct {
  int16_t flags, ydiv, start, loop_start, loop_end, nparts;
  uint32_t values, index;   /* offsets: int16_t[], uint16_t[] (pairs: frames, start) */
} Anim;

/* collision: the game's surfaces, with what find_floor needs precomputed, sorted into cells */
typedef struct {
  int16_t type, force;
  int16_t v[3][3];
  int16_t lo_y, hi_y;
  float nx, ny, nz, off;
  uint8_t flags, room;
  int16_t pad;
} Surf;
typedef struct {
  uint32_t surfs, cells, list;   /* Surf[]; per cell (16 x 16) and kind (floor, ceiling, wall) a range in list */
  uint16_t nsurfs, ncells;
} Collision;

/* an object placed in a level */
typedef struct {
  uint16_t kind;    /* OBJ_* */
  uint16_t model;
  int16_t x, y, z, ry;
  uint32_t param;
  uint8_t acts, area, pad[2];
} Placed;

typedef struct {
  uint32_t magic, size;
  uint32_t tex, mesh, model, anim, level, text;   /* offsets of the tables */
  uint16_t ntex, nmesh, nmodel, nanim, nlevel, ntext;
} DataHeader;

#define HDR DAT(DataHeader, 0)
#define TEX(i) (DAT(Tex, HDR->tex) + (i))
#define MESH(i) (DAT(Mesh, HDR->mesh) + (i))
#define MODEL(i) (DAT(Model, HDR->model) + (i))
#define ANIM(i) (DAT(Anim, HDR->anim) + (i))
#endif
