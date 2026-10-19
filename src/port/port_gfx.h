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

/* the calculator's other command: a letter of the dialogs' font (its texture, then segment2's
 * dl_ia_text_tex_settings), and after it a step along x of the pen, in 1/16ths (0 at first) */
#define PORT_G_GLYPH 0xD1
#define gPortGlyph(pkt, tex)                                                                                    \
  {                                                                                                             \
    Gfx *_g = (Gfx *) (pkt);                                                                                    \
    _g->words.w0 = _SHIFTL(PORT_G_GLYPH, 24, 8);                                                                \
    _g->words.w1 = (uintptr_t) (tex);                                                                           \
  }

/* the frame's display list memory (src/port/game.c) */
extern Gfx port_gfx_pool[];

/* a step of the pen along x after a letter or another step, joined to the command before it if it can be:
 * the text takes one command a letter */
static inline int port_gfx_join_step(Gfx *prev, float x) {
  if (prev < port_gfx_pool) return 0;
  u32 op = (u32) prev->words.w0 >> 24;
  s32 step = (s32) (x * 16.0f);
  if (op == PORT_G_GLYPH && (prev->words.w0 & 0xFFFF) == 0) {
    prev->words.w0 |= (u32) (u16) step;
    return 1;
  }
  if (op == PORT_G_TRANSLATE) {
    s32 px = (s16) (prev->words.w1 >> 16) + step;
    prev->words.w1 = (prev->words.w1 & 0xFFFF) | (u32) (u16) px << 16;
    return 1;
  }
  return 0;
}

/* plays the frame's display list (gfx.c) over the rows being drawn (render.h's r_band) */
void port_gfx_run(const Gfx *dl);
#endif
