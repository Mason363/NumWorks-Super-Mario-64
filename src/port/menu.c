/* The screens around the game on the calculator: the title and file select, the act select (the
 * decompilation's file_select.c and star_select.c made them of objects and models; the dialog boxes, the pause
 * menu, the HUD and the transitions are the game's own: ingame_menu.c, hud.c, print.c, screen_transition.c). */
#include <string.h>
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "game/area.h"
#include "game/ingame_menu.h"
#include "game/level_update.h"
#include "game/save_file.h"
#include "game/print.h"
#include "game/hud.h"
#include "game/game_init.h"
#include "port.h"
#include "port_game.h"
#include "port_menu.h"
#include "plat.h"

/* ------------------------------------------------------------------------------------------------ the title */
static s32 sTitleTimer;
s32 port_title_init(s16 arg, s32 unused) {
  sTitleTimer = 0;
  return 0;
}
s32 port_title_loop(s16 arg, s32 unused) {
  sTitleTimer++;
  if (sTitleTimer > 10 && (gPlayer1Controller->buttonPressed & (A_BUTTON | START_BUTTON))) return 1;
  return 0;
}
s32 port_title_start_level(s16 arg, s32 unused) {
  s32 level = port_start_level();
  /* (a test that starts in a course: the file exists, no opening scene with Peach's letter) */
  if (level != LEVEL_CASTLE_GROUNDS) save_file_set_flags(SAVE_FLAG_FILE_EXISTS);
  return level;
}

s32 port_act_select_init(s16 arg, s32 unused) { return 0; }
s32 port_act_select_loop(s16 arg, s32 unused) { return 1; }
