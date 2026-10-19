/* The screens' display lists on the calculator (src/port/gfx.c): the decompilation's HUD, menus, text and
 * transitions build N64 display lists as the game did; the calculator plays them over each band of the
 * frame. Included last by those files: the commands that only kept the N64's pipeline in step do nothing
 * (they would fill the frame's small display list memory), and the menus' translations take one command
 * instead of a matrix (the text moves by one for each letter). */
#ifndef SM_PORT_GFX_H
#define SM_PORT_GFX_H
#include <PR/gbi.h>

#undef gDPPipeSync
#define gDPPipeSync(pkt) ((void) 0)
#undef gDPLoadSync
#define gDPLoadSync(pkt) ((void) 0)
#undef gDPTileSync
#define gDPTileSync(pkt) ((void) 0)

/* the calculator's own command (a code the N64's F3D microcode does not use): a translation of the modelview
 * matrix by (x, y), in 1/16ths, pushed first or not */
#define PORT_G_TRANSLATE 0xD0
#define gPortTranslate(pkt, push, x, y)                                                                         \
  {                                                                                                             \
    Gfx *_g = (Gfx *) (pkt);                                                                                    \
    _g->words.w0 = _SHIFTL(PORT_G_TRANSLATE, 24, 8) | _SHIFTL((push), 0, 8);                                     \
    _g->words.w1 = _SHIFTL((s32) ((x) * 16.0f), 16, 16) | _SHIFTL((s32) ((y) * 16.0f), 0, 16);                    \
  }

/* plays the frame's display list (gfx.c) over the rows being drawn (render.h's r_band) */
void port_gfx_run(const Gfx *dl);
#endif
