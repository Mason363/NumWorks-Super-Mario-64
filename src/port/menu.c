/* The game's screens and words on the calculator: the title and file select, the act select, the dialog boxes,
 * the pause menu, the HUD, the text labels and the transitions (the decompilation's ingame_menu.c, hud.c,
 * print.c, file_select.c, star_select.c and screen_transition.c drew them with the N64's display lists). */
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

s32 gDialogResponse;
s8 gRedCoinsCollected;
static s16 sDialogId = -1;
static s16 sDialogTimer;
static s16 sMenuMode = -1;

/* ------------------------------------------------------------------------------------------------ dialogs */
void create_dialog_box(s16 dialog) {
  if (sDialogId == -1) sDialogId = dialog, sDialogTimer = 0;
}
void create_dialog_box_with_var(s16 dialog, s32 dialogVar) { create_dialog_box(dialog); }
void create_dialog_inverted_box(s16 dialog) { create_dialog_box(dialog); }
void create_dialog_box_with_response(s16 dialog) {
  create_dialog_box(dialog);
  gDialogResponse = 1;
}
s16 get_dialog_id(void) { return sDialogId; }
void reset_dialog_render_state(void) { sDialogId = -1; }
void set_menu_mode(s16 mode) { sMenuMode = mode; }
void set_cutscene_message(s16 xOffset, s16 yOffset, s16 msgIndex, s16 msgDuration) {}
void reset_cutscene_msg_fade(void) {}
void dl_rgba16_begin_cutscene_msg_fade(void) {}
void dl_rgba16_stop_cutscene_msg_fade(void) {}
void do_cutscene_handler(void) {}
void print_credits_str_ascii(s16 x, s16 y, const char *str) {}
void set_text_array_x_y(s32 xOffset, s32 yOffset) {}
void nop_change_course(void) {}
void reset_red_coins_collected(void) { gRedCoinsCollected = 0; }
void set_hud_camera_status(s16 status) {}

s16 render_menus_and_dialogs(void) {
  if (sDialogId != -1) {
    /* (for now the dialog closes after a moment) */
    if (++sDialogTimer > 30) sDialogId = -1;
  }
  if (sMenuMode != -1) {
    sMenuMode = -1;
    return MENU_OPT_NONE;
  }
  return MENU_OPT_NONE;
}

/* ------------------------------------------------------------------------------------------------ text */
void print_text_centered(s32 x, s32 y, const char *str) {}
void print_text_fmt_int(s32 x, s32 y, const char *str, s32 n) {}
void print_text(s32 x, s32 y, const char *str) {}
void render_text_labels(void) {}
void render_hud(void) {}
s32 render_screen_transition(s8 fadeTimer, s8 transType, u8 transTime, struct WarpTransitionData *transData) {
  return TRUE;
}

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
