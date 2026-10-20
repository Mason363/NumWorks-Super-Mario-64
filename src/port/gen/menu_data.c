/* Made by tools/port.py from the decompilation's levels/menu/leveldata.c: its textures are the calculator's (PORT_TEX_STUB). */
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "make_const_nonconst.h"
#include "port.h"
#include <PR/ultratypes.h>
#include <PR/gbi.h>

#include "macros.h"
#include "surface_terrains.h"
#include "types.h"

#ifdef VERSION_EU
#include "text_strings.h"
#endif
#include "make_const_nonconst.h"

// 0x07000000 - 0x07000018
static const Lights1 lights_menu_save_button = gdSPDefLights1(
    0x3f, 0x3f, 0x3f,
    0xff, 0xff, 0xff, 0x28, 0x28, 0x28
);

// 0x07000018 - 0x07000818
ALIGNED8 static const Texture texture_menu_stone[] = {
PORT_TEX_STUB(65535)
};

// 0x07000818 - 0x07001018
ALIGNED8 static const Texture texture_menu_dark_stone[] = {
PORT_TEX_STUB(65535)
};

// 0x07001018 - 0x07002018
ALIGNED8 static const Texture texture_menu_mario_save[] = {
PORT_TEX_STUB(65535)
};

// 0x07002018 - 0x07003018
ALIGNED8 static const Texture texture_menu_mario_new[] = {
PORT_TEX_STUB(65535)
};

// 0x07003018 - 0x07003118
static const Vtx vertex_menu_save_button_borders[] = {
    {{{  -163,   -122,      0}, 0, {     0,    990}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{   163,   -122,      0}, 0, {   990,    990}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{  -122,    -81,     30}, 0, {    96,    820}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{   122,    -81,     30}, 0, {   862,    820}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{  -163,   -122,      0}, 0, {     0,    990}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -122,    -81,     30}, 0, {    96,    820}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -163,    122,      0}, 0, {     0,      0}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -122,     81,     30}, 0, {    96,    138}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -122,     81,     30}, 0, {    96,    138}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{   122,     81,     30}, 0, {   862,    138}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{   163,    122,      0}, 0, {   990,      0}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{  -163,    122,      0}, 0, {     0,      0}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{   122,     81,     30}, 0, {   862,    138}, {0x4a, 0x00, 0x66, 0xff}}},
    {{{   122,    -81,     30}, 0, {   862,    820}, {0x4a, 0x00, 0x66, 0xff}}},
    {{{   163,   -122,      0}, 0, {   990,    990}, {0x4a, 0x00, 0x66, 0xff}}},
    {{{   163,    122,      0}, 0, {   990,      0}, {0x4a, 0x00, 0x66, 0xff}}},
};

// 0x07003118 - 0x07003158
static const Vtx vertex_menu_save_button_front[] = {
    {{{   122,     81,     30}, 0, {  2012,      0}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{  -122,     81,     30}, 0, {     0,      0}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{   122,    -81,     30}, 0, {  2012,    990}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{  -122,    -81,     30}, 0, {     0,    990}, {0x00, 0x00, 0x7f, 0xff}}},
};

// 0x07003158 - 0x070031A0
static const Gfx dl_tex_block_menu_save_button_base[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_MODULATERGB, G_CC_MODULATERGB),
    gsSPClearGeometryMode(G_SHADING_SMOOTH),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (32 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPEndDisplayList(),
};

// 0x070031A0 - 0x07003218
static const Gfx dl_vertex_menu_save_button_borders[] = {
    gsSPLight(&lights_menu_save_button.l, 1),
    gsSPLight(&lights_menu_save_button.a, 2),
    gsSPVertex(vertex_menu_save_button_borders, 16, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  1,  3,  2, 0x0),
    gsSP2Triangles( 4,  5,  6, 0x0,  5,  7,  6, 0x0),
    gsSP2Triangles( 8,  9, 10, 0x0, 11,  8, 10, 0x0),
    gsSP2Triangles(12, 13, 14, 0x0, 15, 12, 14, 0x0),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, 6, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (64 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPEndDisplayList(),
};

// 0x07003218 - 0x07003258
static const Gfx dl_vertex_menu_save_button_front[] = {
    gsSPVertex(vertex_menu_save_button_front, 4, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  1,  3,  2, 0x0),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPSetGeometryMode(G_SHADING_SMOOTH),
    gsSPEndDisplayList(),
};

// 0x07003258 - 0x07003298
static const Vtx vertex_menu_save_button_back[] = {
    {{{   163,   -122,      0}, 0, {     0,    990}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -163,   -122,      0}, 0, {   990,    990}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   163,    122,      0}, 0, {     0,      0}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -163,    122,      0}, 0, {   990,      0}, {0x00, 0x00, 0x81, 0xff}}},
};

// 0x07003298 - 0x070032E0
static const Gfx dl_tex_block_menu_save_button_back[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_MODULATERGB, G_CC_MODULATERGB),
    gsSPClearGeometryMode(G_SHADING_SMOOTH),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (32 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPEndDisplayList(),
};

// 0x070032E0 - 0x07003330
static const Gfx dl_vertex_menu_save_button_back[] = {
    gsSPLight(&lights_menu_save_button.l, 1),
    gsSPLight(&lights_menu_save_button.a, 2),
    gsSPVertex(vertex_menu_save_button_back, 4, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  1,  3,  2, 0x0),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPSetGeometryMode(G_SHADING_SMOOTH),
    gsSPEndDisplayList(),
};

// 0x07003330 - 0x07003380
const Gfx dl_menu_mario_save_button_base[] = {
    gsSPDisplayList(dl_tex_block_menu_save_button_base),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_stone),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_vertex_menu_save_button_borders),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_mario_save),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 64 * 32 - 1, CALC_DXT(64, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_vertex_menu_save_button_front),
    gsSPEndDisplayList(),
};

// 0x07003380 - 0x070033D0
const Gfx dl_menu_mario_new_button_base[] = {
    gsSPDisplayList(dl_tex_block_menu_save_button_base),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_stone),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_vertex_menu_save_button_borders),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_mario_new),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 64 * 32 - 1, CALC_DXT(64, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_vertex_menu_save_button_front),
    gsSPEndDisplayList(),
};

// 0x070033D0 - 0x07003400
const Gfx dl_menu_save_button_back[] = {
    gsSPDisplayList(dl_tex_block_menu_save_button_back),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_dark_stone),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_vertex_menu_save_button_back),
    gsSPEndDisplayList(),
};

// 0x07003400 - 0x07003450
const Gfx dl_menu_save_button_fade_back[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_SHADING_SMOOTH),
    gsSPLight(&lights_menu_save_button.l, 1),
    gsSPLight(&lights_menu_save_button.a, 2),
    gsSPVertex(vertex_menu_save_button_back, 4, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  1,  3,  2, 0x0),
    gsDPPipeSync(),
    gsSPSetGeometryMode(G_SHADING_SMOOTH),
    gsSPEndDisplayList(),
};

// 0x07003450 - 0x07003468
static const Lights1 lights_menu_main_button = gdSPDefLights1(
    0x3f, 0x3f, 0x3f,
    0xff, 0xff, 0xff, 0x28, 0x28, 0x28
);

// 0x07003468 - 0x07003468
ALIGNED8 static const Texture texture_menu_erase[] = {
PORT_TEX_STUB(65535)
};

// 0x07003C68 - 0x07003C68
ALIGNED8 static const Texture texture_menu_copy[] = {
PORT_TEX_STUB(65535)
};

// 0x07004468 - 0x07004468
ALIGNED8 static const Texture texture_menu_file[] = {
PORT_TEX_STUB(65535)
};

// 0x07004C68 - 0x07004C68
ALIGNED8 static const Texture texture_menu_score[] = {
PORT_TEX_STUB(65535)
};

// 0x07005468 - 0x07005468
ALIGNED8 static const Texture texture_menu_sound[] = {
PORT_TEX_STUB(65535)
};

// 0x07005C68 - 0x07005D68
static const Vtx vertex_menu_main_button_group1[] = {
    {{{  -163,   -122,      0}, 0, {   990,      0}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -122,    -81,     30}, 0, {   862,    138}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -163,    122,      0}, 0, {   990,    990}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -143,    102,      0}, 0, {   926,    904}, {0x59, 0x00, 0xa7, 0xff}}},
    {{{  -133,     92,     10}, 0, {   894,    862}, {0x59, 0x00, 0xa7, 0xff}}},
    {{{  -133,    -92,     10}, 0, {   894,     96}, {0x59, 0x00, 0xa7, 0xff}}},
    {{{  -133,     92,     10}, 0, {   894,    862}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   133,    -92,     10}, 0, {    64,     96}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -133,    -92,     10}, 0, {   894,     96}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   133,     92,     10}, 0, {    64,    862}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   133,     92,     10}, 0, {    64,    862}, {0x00, 0xa7, 0xa7, 0xff}}},
    {{{  -133,     92,     10}, 0, {   894,    862}, {0x00, 0xa7, 0xa7, 0xff}}},
    {{{  -143,    102,      0}, 0, {   926,    904}, {0x00, 0xa7, 0xa7, 0xff}}},
    {{{   143,   -102,      0}, 0, {    32,     54}, {0xa7, 0x00, 0xa7, 0xff}}},
    {{{   133,     92,     10}, 0, {    64,    862}, {0xa7, 0x00, 0xa7, 0xff}}},
    {{{   143,    102,      0}, 0, {    32,    904}, {0xa7, 0x00, 0xa7, 0xff}}},
};

// 0x07005D68 - 0x07005E68
static const Vtx vertex_menu_main_button_group2[] = {
    {{{   143,   -102,      0}, 0, {    32,     54}, {0xa7, 0x00, 0xa7, 0xff}}},
    {{{   133,    -92,     10}, 0, {    64,     96}, {0xa7, 0x00, 0xa7, 0xff}}},
    {{{   133,     92,     10}, 0, {    64,    862}, {0xa7, 0x00, 0xa7, 0xff}}},
    {{{   133,     92,     10}, 0, {    64,    862}, {0x00, 0xa7, 0xa7, 0xff}}},
    {{{  -143,    102,      0}, 0, {   926,    904}, {0x00, 0xa7, 0xa7, 0xff}}},
    {{{   143,    102,      0}, 0, {    32,    904}, {0x00, 0xa7, 0xa7, 0xff}}},
    {{{  -143,   -102,      0}, 0, {   926,     54}, {0x00, 0x59, 0xa7, 0xff}}},
    {{{   133,    -92,     10}, 0, {    64,     96}, {0x00, 0x59, 0xa7, 0xff}}},
    {{{   143,   -102,      0}, 0, {    32,     54}, {0x00, 0x59, 0xa7, 0xff}}},
    {{{  -133,    -92,     10}, 0, {   894,     96}, {0x00, 0x59, 0xa7, 0xff}}},
    {{{  -143,    102,      0}, 0, {   926,    904}, {0x59, 0x00, 0xa7, 0xff}}},
    {{{  -133,    -92,     10}, 0, {   894,     96}, {0x59, 0x00, 0xa7, 0xff}}},
    {{{  -143,   -102,      0}, 0, {   926,     54}, {0x59, 0x00, 0xa7, 0xff}}},
    {{{   163,    122,      0}, 0, {     0,    990}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -143,    102,      0}, 0, {   926,    904}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -163,    122,      0}, 0, {   990,    990}, {0x00, 0x00, 0x81, 0xff}}},
};

// 0x07005E68 - 0x07005F48
static const Vtx vertex_menu_main_button_group3[] = {
    {{{   163,    122,      0}, 0, {     0,    990}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   143,    102,      0}, 0, {    32,    904}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -143,    102,      0}, 0, {   926,    904}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   143,   -102,      0}, 0, {    32,     54}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   163,   -122,      0}, 0, {     0,      0}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -163,    122,      0}, 0, {   990,    990}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -143,   -102,      0}, 0, {   926,     54}, {0x00, 0x00, 0x81, 0xff}}},
    {{{  -163,   -122,      0}, 0, {   990,      0}, {0x00, 0x00, 0x81, 0xff}}},
    {{{   163,   -122,      0}, 0, {     0,      0}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{   122,    -81,     30}, 0, {    96,    138}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{  -122,    -81,     30}, 0, {   862,    138}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{  -122,    -81,     30}, 0, {   862,    138}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -122,     81,     30}, 0, {   862,    820}, {0xb6, 0x00, 0x66, 0xff}}},
    {{{  -163,    122,      0}, 0, {   990,    990}, {0xb6, 0x00, 0x66, 0xff}}},
};

// 0x07005F48 - 0x07006038
static const Vtx vertex_menu_main_button_group4[] = {
    {{{  -122,     81,     30}, 0, {   862,    820}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{  -122,    -81,     30}, 0, {   862,    138}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{   122,    -81,     30}, 0, {    96,    138}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{  -163,   -122,      0}, 0, {   990,      0}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{   163,   -122,      0}, 0, {     0,      0}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{  -122,    -81,     30}, 0, {   862,    138}, {0x00, 0xb6, 0x66, 0xff}}},
    {{{  -122,     81,     30}, 0, {   862,    820}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{   122,     81,     30}, 0, {    96,    820}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{   163,    122,      0}, 0, {     0,    990}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{  -163,    122,      0}, 0, {   990,    990}, {0x00, 0x4a, 0x66, 0xff}}},
    {{{   122,     81,     30}, 0, {    96,    820}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{   163,    122,      0}, 0, {     0,    990}, {0x4a, 0x00, 0x66, 0xff}}},
    {{{   122,     81,     30}, 0, {    96,    820}, {0x4a, 0x00, 0x66, 0xff}}},
    {{{   163,   -122,      0}, 0, {     0,      0}, {0x4a, 0x00, 0x66, 0xff}}},
    {{{   122,    -81,     30}, 0, {    96,    138}, {0x4a, 0x00, 0x66, 0xff}}},
};

// 0x07006038 - 0x07006150
static const Gfx dl_vertex_menu_main_button[] = {
    gsSPLight(&lights_menu_main_button.l, 1),
    gsSPLight(&lights_menu_main_button.a, 2),
    gsSPVertex(vertex_menu_main_button_group1, 16, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  3,  4,  5, 0x0),
    gsSP2Triangles( 6,  7,  8, 0x0,  6,  9,  7, 0x0),
    gsSP2Triangles(10, 11, 12, 0x0, 13, 14, 15, 0x0),
    gsSPVertex(vertex_menu_main_button_group2, 16, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  3,  4,  5, 0x0),
    gsSP2Triangles( 6,  7,  8, 0x0,  6,  9,  7, 0x0),
    gsSP2Triangles(10, 11, 12, 0x0, 13, 14, 15, 0x0),
    gsSPVertex(vertex_menu_main_button_group3, 14, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  3,  1, 0x0),
    gsSP2Triangles( 0,  4,  3, 0x0,  5,  2,  6, 0x0),
    gsSP2Triangles( 5,  6,  7, 0x0,  6,  3,  4, 0x0),
    gsSP2Triangles( 6,  4,  7, 0x0,  8,  9, 10, 0x0),
    gsSP1Triangle(11, 12, 13, 0x0),
    gsSPVertex(vertex_menu_main_button_group4, 15, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  3,  4,  5, 0x0),
    gsSP2Triangles( 6,  7,  8, 0x0,  9,  6,  8, 0x0),
    gsSP2Triangles(10,  0,  2, 0x0, 11, 12, 13, 0x0),
    gsSP1Triangle(12, 14, 13, 0x0),
    gsSPEndDisplayList(),
};

// 0x07006150 - 0x07006198
static const Gfx dl_tex_block_menu_main_button[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_MODULATERGB, G_CC_MODULATERGB),
    gsSPClearGeometryMode(G_SHADING_SMOOTH),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (32 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPEndDisplayList(),
};

// 0x07006198 - 0x070061C8
static const Gfx dl_menu_main_button[] = {
    gsSPDisplayList(dl_vertex_menu_main_button),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPSetGeometryMode(G_SHADING_SMOOTH),
    gsSPEndDisplayList(),
};

// 0x070061C8 - 0x070061F8
const Gfx dl_menu_erase_button[] = {
    gsSPDisplayList(dl_tex_block_menu_main_button),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_erase),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_menu_main_button),
    gsSPEndDisplayList(),
};

// 0x070061F8 - 0x07006228
const Gfx dl_menu_copy_button[] = {
    gsSPDisplayList(dl_tex_block_menu_main_button),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_copy),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_menu_main_button),
    gsSPEndDisplayList(),
};

// 0x07006228 - 0x07006258
const Gfx dl_menu_file_button[] = {
    gsSPDisplayList(dl_tex_block_menu_main_button),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_file),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_menu_main_button),
    gsSPEndDisplayList(),
};

// 0x07006258 - 0x07006288
const Gfx dl_menu_score_button[] = {
    gsSPDisplayList(dl_tex_block_menu_main_button),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_score),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_menu_main_button),
    gsSPEndDisplayList(),
};

// 0x07006288 - 0x070062B8
const Gfx dl_menu_sound_button[] = {
    gsSPDisplayList(dl_tex_block_menu_main_button),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_sound),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_menu_main_button),
    gsSPEndDisplayList(),
};

// 0x070062B8 - 0x070062E8
const Gfx dl_menu_generic_button[] = {
    gsSPDisplayList(dl_tex_block_menu_main_button),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_stone),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPDisplayList(dl_menu_main_button),
    gsSPEndDisplayList(),
};

// 0x070062E8 - 0x07006328
static const Vtx vertex_menu_hand[] = {
    {{{     0,      0,      0}, 0, {     0,   1984}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{    32,      0,      0}, 0, {  1984,   1984}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{    32,     32,      0}, 0, {  1984,      0}, {0x00, 0x00, 0x7f, 0xff}}},
    {{{     0,     32,      0}, 0, {     0,      0}, {0x00, 0x00, 0x7f, 0xff}}},
};

// 0x07006328 - 0x07006B28
ALIGNED8 static const Texture texture_menu_idle_hand[] = {
PORT_TEX_STUB(716)
};

// 0x07006B28 - 0x07007328
ALIGNED8 static const Texture texture_menu_grabbing_hand[] = {
PORT_TEX_STUB(717)
};

// 0x07007328 - 0x070073A0
static const Gfx dl_menu_hand[] = {
    gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
    gsDPSetRenderMode(G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2),
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (32 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPVertex(vertex_menu_hand, 4, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  2,  3, 0x0),
    gsSPTexture(0x0001, 0x0001, 0, G_TX_RENDERTILE, G_OFF),
    gsDPSetRenderMode(G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};

// 0x070073A0 - 0x070073B8
const Gfx dl_menu_idle_hand[] = {
    gsDPPipeSync(),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_idle_hand),
    gsSPBranchList(dl_menu_hand),
};

// 0x070073B8 - 0x070073D0
const Gfx dl_menu_grabbing_hand[] = {
    gsDPPipeSync(),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_grabbing_hand),
    gsSPBranchList(dl_menu_hand),
};

// 0x070073D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_hu[] = {
PORT_TEX_STUB(718)
};

// 0x070075D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_small_a[] = {
PORT_TEX_STUB(719)
};

// 0x070077D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_i[] = {
PORT_TEX_STUB(720)
};

// 0x070079D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_ru[] = {
PORT_TEX_STUB(721)
};

// 0x07007BD0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_se[] = {
PORT_TEX_STUB(722)
};

// 0x07007DD0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_re[] = {
PORT_TEX_STUB(723)
};

// 0x07007FD0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_ku[] = {
PORT_TEX_STUB(724)
};

// 0x070081D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_to[] = {
PORT_TEX_STUB(725)
};

// 0x070083D0
ALIGNED8 static const Texture texture_menu_hud_char_hiragana_wo[] = {
PORT_TEX_STUB(726)
};

// 0x070085D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_ko[] = {
PORT_TEX_STUB(727)
};

// 0x070087D0
ALIGNED8 static const Texture texture_menu_hud_char_kana_handakuten_pi[] = {
PORT_TEX_STUB(728)
};

// 0x070089D0
ALIGNED8 static const Texture texture_menu_hud_char_choonpu[] = {
PORT_TEX_STUB(729)
};

// 0x07008BD0
ALIGNED8 static const Texture texture_menu_hud_char_hiragana_su[] = {
PORT_TEX_STUB(730)
};

// 0x07008DD0
ALIGNED8 static const Texture texture_menu_hud_char_hiragana_ru[] = {
PORT_TEX_STUB(731)
};

// 0x07008FD0
ALIGNED8 static const Texture texture_menu_hud_char_hiragana_ke[] = {
PORT_TEX_STUB(732)
};

// 0x070091D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_ma[] = {
PORT_TEX_STUB(733)
};

// 0x070093D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_ri[] = {
PORT_TEX_STUB(734)
};

// 0x070095D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_o[] = {
PORT_TEX_STUB(735)
};

// 0x070097D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_su[] = {
PORT_TEX_STUB(736)
};

// 0x070099D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_a[] = {
PORT_TEX_STUB(737)
};

// 0x07009BD0
ALIGNED8 static const Texture texture_menu_hud_char_hiragana_mi[] = {
PORT_TEX_STUB(738)
};

// 0x07009DD0
ALIGNED8 static const Texture texture_menu_hud_char_hira_dakuten_do[] = {
PORT_TEX_STUB(739)
};

// 0x07009FD0
ALIGNED8 static const Texture texture_menu_hud_char_hiragana_no[] = {
PORT_TEX_STUB(740)
};

// 0x0700A1D0
ALIGNED8 static const Texture texture_menu_hud_char_question[] = {
PORT_TEX_STUB(741)
};

// 0x0700A3D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_sa[] = {
PORT_TEX_STUB(742)
};

// 0x0700A5D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_u[] = {
PORT_TEX_STUB(743)
};

// 0x0700A7D0
ALIGNED8 static const Texture texture_menu_hud_char_katakana_n[] = {
PORT_TEX_STUB(744)
};

// 0x0700A9D0
ALIGNED8 static const Texture texture_menu_hud_char_kana_dakuten_do[] = {
PORT_TEX_STUB(745)
};

// Menu HUD print table, only used in JP
// 0x0700ABD0
const Texture *const menu_hud_lut[] = {
    texture_menu_hud_char_katakana_hu, texture_menu_hud_char_katakana_small_a,         texture_menu_hud_char_katakana_i, texture_menu_hud_char_katakana_ru,
    texture_menu_hud_char_katakana_se,      texture_menu_hud_char_katakana_re,        texture_menu_hud_char_katakana_ku, texture_menu_hud_char_katakana_to,
    texture_menu_hud_char_hiragana_wo,      texture_menu_hud_char_katakana_ko, texture_menu_hud_char_kana_handakuten_pi, texture_menu_hud_char_choonpu,
    texture_menu_hud_char_hiragana_su,      texture_menu_hud_char_hiragana_ru,        texture_menu_hud_char_hiragana_ke, texture_menu_hud_char_katakana_ma,
    texture_menu_hud_char_katakana_ri,       texture_menu_hud_char_katakana_o,        texture_menu_hud_char_katakana_su, texture_menu_hud_char_katakana_a,
    texture_menu_hud_char_hiragana_mi,  texture_menu_hud_char_hira_dakuten_do,        texture_menu_hud_char_hiragana_no, texture_menu_hud_char_question,
    texture_menu_hud_char_katakana_sa,       texture_menu_hud_char_katakana_u,         texture_menu_hud_char_katakana_n, texture_menu_hud_char_kana_dakuten_do,
};

#if defined(VERSION_JP) || defined(VERSION_SH)
UNUSED static const u64 menu_unused_0 = 0;

// 0x0700AC48
ALIGNED8 static const Texture texture_menu_font_char_jp_0[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AC88
ALIGNED8 static const Texture texture_menu_font_char_jp_1[] = {
PORT_TEX_STUB(65535)
};

// 0x0700ACC8
ALIGNED8 static const Texture texture_menu_font_char_jp_2[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AD08
ALIGNED8 static const Texture texture_menu_font_char_jp_3[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AD48
ALIGNED8 static const Texture texture_menu_font_char_jp_4[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AD88
ALIGNED8 static const Texture texture_menu_font_char_jp_5[] = {
PORT_TEX_STUB(65535)
};

// 0x0700ADC8
ALIGNED8 static const Texture texture_menu_font_char_jp_6[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AE08
ALIGNED8 static const Texture texture_menu_font_char_jp_7[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AE48
ALIGNED8 static const Texture texture_menu_font_char_jp_8[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AE88
ALIGNED8 static const Texture texture_menu_font_char_jp_9[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AEC8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_a[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AF08
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_i[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AF48
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_u[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AF88
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_c[] = {
PORT_TEX_STUB(65535)
};

// 0x0700AFC8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_o[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B008
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ka[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B048
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ki[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B088
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ku[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B0C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ke[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B108
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ko[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B148
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_sa[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B188
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_shi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B1C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_su[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B208
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_se[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B248
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_so[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B288
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ta[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B2C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_chi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B308
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_tsu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B348
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_te[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B388
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_to[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B3C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_na[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B408
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ni[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B448
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_nu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B488
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ne[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B4C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_no[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B508
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ha[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B548
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_hi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B588
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_hu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B5C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_he[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B608
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ho[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B648
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ma[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B688
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_mi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B6C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_mu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B708
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_me[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B748
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_mo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B788
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ya[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B7C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_yu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B808
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_yo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B848
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ra[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B888
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ri[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B8C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ru[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B908
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_re[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B948
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_ro[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B988
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_wa[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B9C8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_wo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BA08
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_n[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BA48
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_a[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BA88
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_i[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BAC8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_u[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BB08
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_e[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BB48
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_o[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BB88
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_ka[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BBC8
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_yu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BC08
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_yo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BC48
ALIGNED8 static const Texture texture_menu_font_char_jp_hiragana_small_tsu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BC88
ALIGNED8 static const Texture texture_menu_font_char_jp_handakuten[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BCC8
ALIGNED8 static const Texture texture_menu_font_char_jp_dakuten[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BD08
ALIGNED8 static const Texture texture_menu_font_char_jp_long_vowel[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BD48
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_a[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BD88
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_i[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BDC8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_u[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BE08
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_e[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BE48
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_o[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BE88
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ka[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BEC8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ki[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BF08
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ku[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BF48
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ke[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BF88
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ko[] = {
PORT_TEX_STUB(65535)
};

// 0x0700BFC8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_sa[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C008
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_shi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C048
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_su[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C088
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_se[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C0C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_so[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C108
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ta[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C148
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_chi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C188
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_tsu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C1C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_te[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C208
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_to[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C248
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_na[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C288
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ni[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C2C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_nu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C308
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ne[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C348
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_no[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C388
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ha[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C3C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_hi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C408
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_hu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C448
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_he[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C488
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ho[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C4C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ma[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C508
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_mi[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C548
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_mu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C588
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_me[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C5C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_mo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C608
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ya[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C648
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_yu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C688
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_yo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C6C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ra[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C708
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ri[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C748
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ru[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C788
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_re[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C7C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_ro[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C808
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_wa[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C848
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_wo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C888
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_n[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C8C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_a[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C908
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_i[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C948
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_u[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C988
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_e[] = {
PORT_TEX_STUB(65535)
};

// 0x0700C9C8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_o[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CA08
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_ka[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CA48
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_yu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CA88
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_yo[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CAC8
ALIGNED8 static const Texture texture_menu_font_char_jp_katakana_small_tsu[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CB08
ALIGNED8 static const Texture texture_menu_font_char_jp_A[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CB48
ALIGNED8 static const Texture texture_menu_font_char_jp_B[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CB88
ALIGNED8 static const Texture texture_menu_font_char_jp_C[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CBC8
ALIGNED8 static const Texture texture_menu_font_char_jp_D[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CC08
ALIGNED8 static const Texture texture_menu_font_char_jp_coin[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CC48
ALIGNED8 static const Texture texture_menu_font_char_jp_star_filled[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CC88
ALIGNED8 static const Texture texture_menu_font_char_jp_multiply[] = {
PORT_TEX_STUB(65535)
};

// 0x0700CCC8
ALIGNED8 static const Texture texture_menu_font_char_jp_exclamation[] = {
PORT_TEX_STUB(65535)
};

#else

// 0x0700AC40
ALIGNED8 static const Texture texture_menu_font_char_0[] = {
PORT_TEX_STUB(746)
};

// 0x0700AC80
ALIGNED8 static const Texture texture_menu_font_char_1[] = {
PORT_TEX_STUB(747)
};

// 0x0700ACC0
ALIGNED8 static const Texture texture_menu_font_char_2[] = {
PORT_TEX_STUB(748)
};

// 0x0700AD00
ALIGNED8 static const Texture texture_menu_font_char_3[] = {
PORT_TEX_STUB(749)
};

// 0x0700AD40
ALIGNED8 static const Texture texture_menu_font_char_4[] = {
PORT_TEX_STUB(750)
};

// 0x0700AD80
ALIGNED8 static const Texture texture_menu_font_char_5[] = {
PORT_TEX_STUB(751)
};

// 0x0700ADC0
ALIGNED8 static const Texture texture_menu_font_char_6[] = {
PORT_TEX_STUB(752)
};

// 0x0700AE00
ALIGNED8 static const Texture texture_menu_font_char_7[] = {
PORT_TEX_STUB(753)
};

// 0x0700AE40
ALIGNED8 static const Texture texture_menu_font_char_8[] = {
PORT_TEX_STUB(754)
};

// 0x0700AE80
ALIGNED8 static const Texture texture_menu_font_char_9[] = {
PORT_TEX_STUB(755)
};

// 0x0700AEC0
ALIGNED8 static const Texture texture_menu_font_char_A[] = {
PORT_TEX_STUB(756)
};

// 0x0700AF00
ALIGNED8 static const Texture texture_menu_font_char_B[] = {
PORT_TEX_STUB(757)
};

// 0x0700AF40
ALIGNED8 static const Texture texture_menu_font_char_C[] = {
PORT_TEX_STUB(758)
};

#ifdef VERSION_EU
// 0x0700AF80
ALIGNED8 static const Texture texture_menu_font_char_D[] = {
PORT_TEX_STUB(759)
};

#else

// 0x0700AF80
ALIGNED8 static const Texture texture_menu_font_char_D[] = {
PORT_TEX_STUB(759)
};
#endif

// 0x0700AFC0
ALIGNED8 static const Texture texture_menu_font_char_E[] = {
PORT_TEX_STUB(760)
};

// 0x0700B000
ALIGNED8 static const Texture texture_menu_font_char_F[] = {
PORT_TEX_STUB(761)
};

// 0x0700B040
ALIGNED8 static const Texture texture_menu_font_char_G[] = {
PORT_TEX_STUB(762)
};

// 0x0700B080
ALIGNED8 static const Texture texture_menu_font_char_H[] = {
PORT_TEX_STUB(763)
};

// 0x0700B0C0
ALIGNED8 static const Texture texture_menu_font_char_I[] = {
PORT_TEX_STUB(764)
};

// 0x0700B100
ALIGNED8 static const Texture texture_menu_font_char_J[] = {
PORT_TEX_STUB(765)
};

// 0x0700B140
ALIGNED8 static const Texture texture_menu_font_char_K[] = {
PORT_TEX_STUB(766)
};

// 0x0700B180
ALIGNED8 static const Texture texture_menu_font_char_L[] = {
PORT_TEX_STUB(767)
};

// 0x0700B1C0
ALIGNED8 static const Texture texture_menu_font_char_M[] = {
PORT_TEX_STUB(768)
};

// 0x0700B200
ALIGNED8 static const Texture texture_menu_font_char_N[] = {
PORT_TEX_STUB(769)
};

// 0x0700B240
ALIGNED8 static const Texture texture_menu_font_char_O[] = {
PORT_TEX_STUB(770)
};

// 0x0700B280
ALIGNED8 static const Texture texture_menu_font_char_P[] = {
PORT_TEX_STUB(771)
};

// 0x0700B2C0
ALIGNED8 static const Texture texture_menu_font_char_Q[] = {
PORT_TEX_STUB(772)
};

// 0x0700B300
ALIGNED8 static const Texture texture_menu_font_char_R[] = {
PORT_TEX_STUB(773)
};

// 0x0700B340
ALIGNED8 static const Texture texture_menu_font_char_S[] = {
PORT_TEX_STUB(774)
};

// 0x0700B380
ALIGNED8 static const Texture texture_menu_font_char_T[] = {
PORT_TEX_STUB(775)
};

// 0x0700B3C0
ALIGNED8 static const Texture texture_menu_font_char_U[] = {
PORT_TEX_STUB(776)
};

// 0x0700B400
ALIGNED8 static const Texture texture_menu_font_char_V[] = {
PORT_TEX_STUB(777)
};

// 0x0700B440
ALIGNED8 static const Texture texture_menu_font_char_W[] = {
PORT_TEX_STUB(778)
};

// 0x0700B480
ALIGNED8 static const Texture texture_menu_font_char_X[] = {
PORT_TEX_STUB(779)
};

// 0x0700B4C0
ALIGNED8 static const Texture texture_menu_font_char_Y[] = {
PORT_TEX_STUB(780)
};

// 0x0700B500
ALIGNED8 static const Texture texture_menu_font_char_Z[] = {
PORT_TEX_STUB(781)
};

// 0x0700B540
ALIGNED8 static const Texture texture_menu_font_char_coin[] = {
PORT_TEX_STUB(782)
};

// 0x0700B580
ALIGNED8 static const Texture texture_menu_font_char_multiply[] = {
PORT_TEX_STUB(783)
};

// 0x0700B5C0
ALIGNED8 static const Texture texture_menu_font_char_star_filled[] = {
PORT_TEX_STUB(784)
};

// 0x0700B600
ALIGNED8 static const Texture texture_menu_font_char_dash[] = {
PORT_TEX_STUB(785)
};

#ifdef VERSION_EU
// 0x0700B640
ALIGNED8 static const Texture texture_menu_font_char_comma[] = {
PORT_TEX_STUB(786)
};

// 0x0700B680
ALIGNED8 static const Texture texture_menu_font_char_apostrophe[] = {
PORT_TEX_STUB(787)
};

#else

// 0x0700B640
ALIGNED8 static const Texture texture_menu_font_char_comma[] = {
PORT_TEX_STUB(786)
};

// 0x0700B680
ALIGNED8 static const Texture texture_menu_font_char_apostrophe[] = {
PORT_TEX_STUB(787)
};
#endif

// 0x0700B6C0
ALIGNED8 static const Texture texture_menu_font_char_exclamation[] = {
PORT_TEX_STUB(788)
};

// 0x0700B700
ALIGNED8 static const Texture texture_menu_font_char_question[] = {
PORT_TEX_STUB(789)
};

// 0x0700B740
ALIGNED8 static const Texture texture_menu_font_char_mface1[] = {
PORT_TEX_STUB(790)
};

// 0x0700B780
ALIGNED8 static const Texture texture_menu_font_char_mface2[] = {
PORT_TEX_STUB(791)
};

// 0x0700B7C0
ALIGNED8 static const Texture texture_menu_font_char_period[] = {
PORT_TEX_STUB(792)
};

// 0x0700B800
ALIGNED8 static const Texture texture_menu_font_char_ampersand[] = {
PORT_TEX_STUB(793)
};
#endif

#ifdef VERSION_EU
// 0x0700B840
ALIGNED8 static const Texture texture_menu_font_char_umlaut[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B880
ALIGNED8 static const Texture texture_menu_font_char_cedilla_mayus[] = {
PORT_TEX_STUB(65535)
};

// 0x0700B8C0
ALIGNED8 static const Texture texture_menu_font_char_colon[] = {
PORT_TEX_STUB(65535)
};
#endif

// Menu small font print table
// 0x0700CD08
const Texture *const menu_font_lut[] = {
#if defined(VERSION_JP) || defined(VERSION_SH)
    texture_menu_font_char_jp_0, texture_menu_font_char_jp_1, texture_menu_font_char_jp_2, texture_menu_font_char_jp_3,
    texture_menu_font_char_jp_4, texture_menu_font_char_jp_5, texture_menu_font_char_jp_6, texture_menu_font_char_jp_7,
    texture_menu_font_char_jp_8, texture_menu_font_char_jp_9, texture_menu_font_char_jp_A, texture_menu_font_char_jp_B,
    texture_menu_font_char_jp_C, texture_menu_font_char_jp_D,                   0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
                              0x0,                      0x0,                      0x0,                      0x0,
    texture_menu_font_char_jp_hiragana_a, texture_menu_font_char_jp_hiragana_i, texture_menu_font_char_jp_hiragana_u, texture_menu_font_char_jp_hiragana_c,
    texture_menu_font_char_jp_hiragana_o, texture_menu_font_char_jp_hiragana_ka, texture_menu_font_char_jp_hiragana_ki, texture_menu_font_char_jp_hiragana_ku,
    texture_menu_font_char_jp_hiragana_ke, texture_menu_font_char_jp_hiragana_ko, texture_menu_font_char_jp_hiragana_sa, texture_menu_font_char_jp_hiragana_shi,
    texture_menu_font_char_jp_hiragana_su, texture_menu_font_char_jp_hiragana_se, texture_menu_font_char_jp_hiragana_so, texture_menu_font_char_jp_hiragana_ta,
    texture_menu_font_char_jp_hiragana_chi, texture_menu_font_char_jp_hiragana_tsu, texture_menu_font_char_jp_hiragana_te, texture_menu_font_char_jp_hiragana_to,
    texture_menu_font_char_jp_hiragana_na, texture_menu_font_char_jp_hiragana_ni, texture_menu_font_char_jp_hiragana_nu, texture_menu_font_char_jp_hiragana_ne,
    texture_menu_font_char_jp_hiragana_no, texture_menu_font_char_jp_hiragana_ha, texture_menu_font_char_jp_hiragana_hi, texture_menu_font_char_jp_hiragana_hu,
    texture_menu_font_char_jp_hiragana_he, texture_menu_font_char_jp_hiragana_ho, texture_menu_font_char_jp_hiragana_ma, texture_menu_font_char_jp_hiragana_mi,
    texture_menu_font_char_jp_hiragana_mu, texture_menu_font_char_jp_hiragana_me, texture_menu_font_char_jp_hiragana_mo, texture_menu_font_char_jp_hiragana_ya,
    texture_menu_font_char_jp_hiragana_yu, texture_menu_font_char_jp_hiragana_yo, texture_menu_font_char_jp_hiragana_ra, texture_menu_font_char_jp_hiragana_ri,
    texture_menu_font_char_jp_hiragana_ru, texture_menu_font_char_jp_hiragana_re, texture_menu_font_char_jp_hiragana_ro, texture_menu_font_char_jp_hiragana_wa,
    texture_menu_font_char_jp_hiragana_wo, texture_menu_font_char_jp_hiragana_n, 0x0,                                0x0,
    texture_menu_font_char_jp_katakana_a, texture_menu_font_char_jp_katakana_i, texture_menu_font_char_jp_katakana_u, texture_menu_font_char_jp_katakana_e,
    texture_menu_font_char_jp_katakana_o, texture_menu_font_char_jp_katakana_ka, texture_menu_font_char_jp_katakana_ki, texture_menu_font_char_jp_katakana_ku,
    texture_menu_font_char_jp_katakana_ke, texture_menu_font_char_jp_katakana_ko, texture_menu_font_char_jp_katakana_sa, texture_menu_font_char_jp_katakana_shi,
    texture_menu_font_char_jp_katakana_su, texture_menu_font_char_jp_katakana_se, texture_menu_font_char_jp_katakana_so, texture_menu_font_char_jp_katakana_ta,
    texture_menu_font_char_jp_katakana_chi, texture_menu_font_char_jp_katakana_tsu, texture_menu_font_char_jp_katakana_te, texture_menu_font_char_jp_katakana_to,
    texture_menu_font_char_jp_katakana_na, texture_menu_font_char_jp_katakana_ni, texture_menu_font_char_jp_katakana_nu, texture_menu_font_char_jp_katakana_ne,
    texture_menu_font_char_jp_katakana_no, texture_menu_font_char_jp_katakana_ha, texture_menu_font_char_jp_katakana_hi, texture_menu_font_char_jp_katakana_hu,
    texture_menu_font_char_jp_katakana_he, texture_menu_font_char_jp_katakana_ho, texture_menu_font_char_jp_katakana_ma, texture_menu_font_char_jp_katakana_mi,
    texture_menu_font_char_jp_katakana_mu, texture_menu_font_char_jp_katakana_me, texture_menu_font_char_jp_katakana_mo, texture_menu_font_char_jp_katakana_ya,
    texture_menu_font_char_jp_katakana_yu, texture_menu_font_char_jp_katakana_yo, texture_menu_font_char_jp_katakana_ra, texture_menu_font_char_jp_katakana_ri,
    texture_menu_font_char_jp_katakana_ru, texture_menu_font_char_jp_katakana_re, texture_menu_font_char_jp_katakana_ro, texture_menu_font_char_jp_katakana_wa,
    texture_menu_font_char_jp_katakana_wo, texture_menu_font_char_jp_katakana_n,                                   0x0, texture_menu_font_char_jp_long_vowel,
    texture_menu_font_char_jp_hiragana_small_e, texture_menu_font_char_jp_hiragana_small_tsu, texture_menu_font_char_jp_hiragana_small_ka, texture_menu_font_char_jp_hiragana_small_yu,
    texture_menu_font_char_jp_hiragana_small_yo, texture_menu_font_char_jp_hiragana_small_a, texture_menu_font_char_jp_hiragana_small_i, texture_menu_font_char_jp_hiragana_small_u,
    texture_menu_font_char_jp_hiragana_small_o,          0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    texture_menu_font_char_jp_katakana_small_e, texture_menu_font_char_jp_katakana_small_tsu, texture_menu_font_char_jp_katakana_small_ka, texture_menu_font_char_jp_katakana_small_yu,
    texture_menu_font_char_jp_katakana_small_yo, texture_menu_font_char_jp_katakana_small_a, texture_menu_font_char_jp_katakana_small_i, texture_menu_font_char_jp_katakana_small_u,
    texture_menu_font_char_jp_katakana_small_o, 0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    texture_menu_font_char_jp_dakuten, texture_menu_font_char_jp_handakuten, texture_menu_font_char_jp_exclamation, 0x0,
    0x0,                      0x0,                      0x0,                      0x0,
    0x0,                      texture_menu_font_char_jp_coin, texture_menu_font_char_jp_star_filled, texture_menu_font_char_jp_multiply,
    0x0,                      0x0,                      0x0,                      0x0,
#else
    texture_menu_font_char_0, texture_menu_font_char_1, texture_menu_font_char_2, texture_menu_font_char_3,
    texture_menu_font_char_4, texture_menu_font_char_5, texture_menu_font_char_6, texture_menu_font_char_7,
    texture_menu_font_char_8, texture_menu_font_char_9, texture_menu_font_char_A, texture_menu_font_char_B,
    texture_menu_font_char_C, texture_menu_font_char_D, texture_menu_font_char_E, texture_menu_font_char_F,
    texture_menu_font_char_G, texture_menu_font_char_H, texture_menu_font_char_I, texture_menu_font_char_J,
    texture_menu_font_char_K, texture_menu_font_char_L, texture_menu_font_char_M, texture_menu_font_char_N,
    texture_menu_font_char_O, texture_menu_font_char_P, texture_menu_font_char_Q, texture_menu_font_char_R,
    texture_menu_font_char_S, texture_menu_font_char_T, texture_menu_font_char_U, texture_menu_font_char_V,
    texture_menu_font_char_W, texture_menu_font_char_X, texture_menu_font_char_Y, texture_menu_font_char_Z,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0, texture_menu_font_char_apostrophe, texture_menu_font_char_period,
    texture_menu_font_char_mface1, texture_menu_font_char_mface2,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0, texture_menu_font_char_comma,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0, texture_menu_font_char_dash,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
#ifdef VERSION_EU
          0x0, texture_menu_font_char_ampersand, texture_menu_font_char_colon,       0x0,
          0x0, texture_menu_font_char_umlaut,       0x0,       0x0,
          0x0, texture_menu_font_char_cedilla_mayus,       0x0,       0x0,
#else
          0x0, texture_menu_font_char_ampersand,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
          0x0,       0x0,       0x0,       0x0,
#endif
          0x0,       0x0, texture_menu_font_char_exclamation,       0x0,
    texture_menu_font_char_question,       0x0,       0x0,       0x0,
          0x0, texture_menu_font_char_coin, texture_menu_font_char_star_filled, texture_menu_font_char_multiply,
          0x0,       0x0,       0x0,       0x0,
#endif
};

// 0x0700D108 - 0x0700D160
const Gfx dl_menu_ia8_text_begin[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetCombineMode(G_CC_FADEA, G_CC_FADEA),
    gsDPSetEnvColor(255, 255, 255, 255),
    gsDPSetRenderMode(G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2),
    gsDPSetTextureFilter(G_TF_POINT),
    gsDPSetTile(G_IM_FMT_IA, G_IM_SIZ_8b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_IA, G_IM_SIZ_8b, 1, 0, G_TX_RENDERTILE, 0, G_TX_CLAMP, 3, G_TX_NOLOD, G_TX_CLAMP, 3, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (8 - 1) << G_TEXTURE_IMAGE_FRAC, (8 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPEndDisplayList(),
};

// 0x0700D160 - 0x0700D1A0
const Gfx dl_menu_ia8_text_end[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetRenderMode(G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsDPSetEnvColor(255, 255, 255, 255),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsSPEndDisplayList(),
};

UNUSED static const u64 menu_unused_1 = 0;

#ifdef VERSION_EU

// 0x0700BDA0 - 0x0700CDA0
ALIGNED8 static const Texture texture_menu_course_upper[] = {
PORT_TEX_STUB(794)
};

// 0x0700CDA0 - 0x0700DDA0
ALIGNED8 static const Texture texture_menu_niveau_upper[] = {
PORT_TEX_STUB(65535)
};

// 0x0700DDA0 - 0x0700EDA0
ALIGNED8 static const Texture texture_menu_kurs_upper[] = {
PORT_TEX_STUB(65535)
};

// 0x0700EDA0 - 0x0700FDA0
ALIGNED8 static const Texture texture_menu_course_lower[] = {
PORT_TEX_STUB(795)
};

#elif defined(VERSION_CN)

// 0x070BCE0 - 0x070CCE0
ALIGNED8 static const Texture texture_menu_course_upper[] = {
PORT_TEX_STUB(794)
};

// 0x0700CCE0 - 0x0700DCE0
ALIGNED8 static const Texture texture_menu_course_lower[] = {
PORT_TEX_STUB(795)
};

#else

// 0x0700D1A8 - 0x0700E1A8
ALIGNED8 static const Texture texture_menu_course_upper[] = {
PORT_TEX_STUB(794)
};

// 0x0700E1A8 - 0x0700F1A8
ALIGNED8 static const Texture texture_menu_course_lower[] = {
PORT_TEX_STUB(795)
};
#endif

// 0x0700F1A8 - 0x0700F1E8
static const Vtx vertex_menu_course_upper[] = {
    {{{   -32,      0,      0}, 0, {     0,   1984}, {0x00, 0x00, 0x7f, 0x00}}},
    {{{    32,      0,      0}, 0, {  4032,   1984}, {0x00, 0x00, 0x7f, 0x00}}},
    {{{    32,     32,      0}, 0, {  4032,      0}, {0x00, 0x00, 0x7f, 0x00}}},
    {{{   -32,     32,      0}, 0, {     0,      0}, {0x00, 0x00, 0x7f, 0x00}}},
};

// 0x0700F1E8 - 0x0700F228
static const Vtx vertex_menu_course_lower[] = {
    {{{   -32,    -32,      0}, 0, {     0,   1984}, {0x00, 0x00, 0x7f, 0x00}}},
    {{{    32,    -32,      0}, 0, {  4032,   1984}, {0x00, 0x00, 0x7f, 0x00}}},
    {{{    32,      0,      0}, 0, {  4032,      0}, {0x00, 0x00, 0x7f, 0x00}}},
    {{{   -32,      0,      0}, 0, {     0,      0}, {0x00, 0x00, 0x7f, 0x00}}},
};

// 0x0700F228 - 0x0700F2F8
const Gfx dl_menu_rgba16_wood_course[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetRenderMode(G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2),
#ifdef VERSION_EU
    gsSPEndDisplayList(),
};
const Gfx dl_menu_rgba16_wood_course_end[] = {
#else
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_course_upper),
#endif
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 64 * 32 - 1, CALC_DXT(64, G_IM_SIZ_16b_BYTES)),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (64 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPVertex(vertex_menu_course_upper, 4, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  2,  3, 0x0),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_course_lower),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 64 * 32 - 1, CALC_DXT(64, G_IM_SIZ_16b_BYTES)),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (64 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPVertex(vertex_menu_course_lower, 4, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  2,  3, 0x0),
    gsDPSetRenderMode(G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2),
    gsSPTexture(0x0001, 0x0001, 0, G_TX_RENDERTILE, G_OFF),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};

#ifdef VERSION_EU
// 0x0700FEF0 - 0x0700FF00
const Gfx dl_menu_texture_course_upper[] = {
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_course_upper),
    gsSPEndDisplayList(),
};

// 0x0700FF00 - 0x0700FF10
const Gfx dl_menu_texture_niveau_upper[] = {
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_niveau_upper),
    gsSPEndDisplayList(),
};

// 0x0700FF10 - 0x0700FF20
const Gfx dl_menu_texture_kurs_upper[] = {
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_menu_kurs_upper),
    gsSPEndDisplayList(),
};
#endif

// 0x0700F2F8 - 0x0700F328
const Collision main_menu_seg7_collision[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0x4),
    COL_VERTEX( 8192, -1000, -8192),
    COL_VERTEX(-8192, -1000, -8192),
    COL_VERTEX(-8192, -1000,  8192),
    COL_VERTEX( 8192, -1000,  8192),
    COL_TRI_INIT(SURFACE_DEFAULT, 2),
    COL_TRI(0, 1, 2),
    COL_TRI(0, 2, 3),
    COL_TRI_STOP(),
    COL_END(),
};

#ifdef VERSION_EU

// Duplicate course name tables; the main menu needs all languages loaded at
// once since it switches language, so the copies in segment 19 aren't good
// enough.

#define COURSE_TABLE eu_course_strings_en_table
#include "text/us/define_courses.inc.c"
#undef COURSE_TABLE

#define COURSE_TABLE eu_course_strings_fr_table
#include "text/fr/define_courses.inc.c"
#undef COURSE_TABLE

#define COURSE_TABLE eu_course_strings_de_table
#include "text/de/define_courses.inc.c"
#undef COURSE_TABLE

#endif
