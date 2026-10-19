/* The calculator's own data for the game (src/data.bin, made by tools/pack.py): the meshes and textures of the
 * levels and of the models. It stays in flash and is read in place; every table is found through the header
 * at its start. (Collision, objects and animations are the decompilation's own C data: src/port.) */
#ifndef SM_DATA_H
#define SM_DATA_H
#include <stdint.h>
#include "data_ids.h"

extern const uint8_t sm_data[];
#define DAT(type, off) ((const type *)(const void *)(sm_data + (off)))

/* a vertex: position, texture coordinates (texels x 32) and colour: RGB 565 (its alpha is the batch's), RGBA
 * 4444 when the batch has W_VALPHA, or, lit, its normal folded onto an octahedron (x in the low byte, y in the
 * high one: tools/pack.py's vertex_colour) */
typedef struct {
  int16_t x, y, z, s, t;
  uint16_t c;
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
/* texture wrap modes, per axis (Batch.wrap: s in bits 0-1, t in bits 2-3); bit 4: the texture over the vertex
 * colours, which show where it is clear (the N64's G_CC_BLENDRGBA: Mario's eyes, a Goomba's face) */
enum { W_WRAP = 0, W_MIRROR = 1, W_CLAMP = 2 };
#define W_BLEND 16
#define W_VALPHA 32   /* (the vertices have alphas of their own) */

typedef struct {
  uint16_t tex;     /* index in the texture table (when M_TEX) */
  uint8_t mode, wrap;
  uint16_t tri0, ntri;   /* in the mesh's triangles (3 bytes each: indices among the batch's vertices) */
  uint16_t v0, nv;       /* the vertices they use (128 at most) */
  int16_t lo[3], hi[3];  /* their bounds */
  uint8_t light[6];      /* lit: the ambient colour, then the light's */
  uint8_t alpha, pad;    /* the vertices' alpha (but W_VALPHA) */
} Batch;

typedef struct {
  uint32_t vtx, tri, batch;   /* offsets: MVert[], uint8_t[3] each, Batch[] */
  uint16_t nvtx, ntri, nbatch, flags;
  int16_t lo[3], hi[3];
} Mesh;

/* a texture: a palette of 16 or 256 texel values, then the texels (4 or 8 bits each), w and h powers of two.
 * fmt: the values' kind (bits 0-3), and bits 4-5: 1 for 16 colours, 2 for 256 (3: no palette, 16-bit texels) */
enum { TF_565, TF_565K, TF_IA };   /* opaque; with a transparent colour (TEX_KEY); intensity and alpha */
#define TEX_KEY 0x0020
typedef struct {
  uint32_t off;
  uint8_t wl, hl, fmt, shift;   /* log2 of the size; shift: the texture is that many times halved (the triangles'
                                   texel coordinates are the full size's) */
} Tex;

/* a model: the game's geo layout as a list of nodes, parents first (in the order the game draws them), each
 * with its parent: the animated parts (which take an animation's values, in order), the switches (which choose
 * one child), the callbacks, the shadow and the meshes */
enum { N_PART, N_TRANSLATE, N_ROTATE, N_SCALE, N_SWITCH, N_BILLBOARD, N_SHADOW, N_HELD, N_MESH, N_GROUP, N_RANGE,
       N_ASM, N_TRANSROT };
typedef struct {
  uint8_t kind;
  uint8_t fn;                 /* switches, held objects and callbacks: the game's function (port_geo_funcs) */
  uint8_t arg;                /* meshes: their layer; switches: how many cases; shadows: the type; held objects:
                                 the player */
  uint8_t cse;                /* the node's place among its parent's children (a switch draws one of them) */
  uint16_t parent;            /* 0xFFFF for the root's children */
  uint16_t end;               /* where its subtree ends: the next node not below it */
  uint16_t mesh;              /* meshes (and nodes with one): 0xFFFF none */
  uint16_t pad;
  int16_t t[3];               /* translation (render ranges: from, to; shadows: solidity, scale) */
  int16_t r[3];               /* rotation */
  uint32_t scale;             /* x 1/65536 (scale nodes); callbacks: their parameter */
} MNode;
typedef struct {
  uint32_t nodes;
  uint16_t nnodes;
  uint8_t pad0, pad1;
  int16_t cull;               /* the culling radius */
  int16_t pad2, pad3, pad4;
  int16_t lo[3], hi[3];       /* a box around it */
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
