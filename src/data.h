/* The calculator's own data for the game (src/data.bin, made by tools/pack.py): the meshes and textures of the
 * levels and of the models. It stays in flash and is read in place; every table is found through the header
 * at its start. (Collision, objects and animations are the decompilation's own C data: src/port.) */
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

/* a model: the game's geo layout as a list of nodes, each with its parent: the animated parts (which take an
 * animation's rotations, in order), the switches (which choose one child) and the meshes */
enum { N_PART, N_TRANSLATE, N_ROTATE, N_SCALE, N_SWITCH, N_BILLBOARD, N_SHADOW, N_HELD, N_MESH, N_GROUP, N_RANGE,
       N_ASM };
typedef struct {
  uint8_t kind, parent;       /* parent: 0xFF for the root */
  uint8_t sw, arg;            /* switches and callbacks: which function (tools/models.py); meshes: their layer */
  uint8_t cse, pad;           /* the node's place among its parent's children (a switch draws one of them) */
  uint16_t mesh;              /* meshes (and nodes with one): 0xFFFF none */
  int16_t t[3];               /* translation (render ranges: from, to) */
  int16_t r[3];               /* rotation */
  uint32_t scale;             /* x 1/65536 (scale nodes) */
} MNode;
typedef struct {
  uint32_t nodes;
  uint16_t nnodes;
  uint8_t shadow_type, pad;
  int16_t shadow_size, shadow_alpha;
  int16_t lo[3], hi[3];       /* a box around it, for culling */
} MModel;

typedef struct {
  uint32_t magic, size;
  uint32_t tex, mesh, model, pad1, pad2, pad3;   /* offsets of the tables */
  uint16_t ntex, nmesh, nmodel, pad4, pad5, pad6;
} DataHeader;

#define HDR DAT(DataHeader, 0)
#define TEX(i) (DAT(Tex, HDR->tex) + (i))
#define MESH(i) (DAT(Mesh, HDR->mesh) + (i))
#define MODEL(i) (DAT(MModel, HDR->model) + (i))
#endif
