/* The screens around the game on the calculator: the title and file select, the act select (the
 * decompilation's file_select.c and star_select.c made them of objects and models; the dialog boxes, the pause
 * menu, the HUD and the transitions are the game's own: ingame_menu.c, hud.c, print.c, screen_transition.c). */
#include <math.h>
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
#include "game/memory.h"
#include "game/segment2.h"
#include "engine/math_util.h"
#include "text_strings.h"
#include "port.h"
#include "port_game.h"
#include "port_menu.h"
#include "plat.h"

/* a screen of the calculator's draws over the frame (port_menu_draw) */
s32 port_menu_active;

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

/* ------------------------------------------------------------------------------------------------ the act select */
/* The stars of the course, as star_select.c chose them: the acts done (the clear stars), the next one, the
 * selection starting at the first star not yet collected. Drawn in the frame's display list (port_menu_draw)
 * where the game had them, on its white screen: a row of stars (the chosen one bigger and spinning) and their
 * numbers, the act's name, the best coin score, the course's name. */
static s8 sVisibleStars, sSelected, sDone;
static u8 sStarModel[6];   /* 1: a star to get, 2: a star got */
static s16 sActTimer;
static s16 sSpin;

s32 port_act_select_init(s16 arg, s32 unused) {
  u8 stars = save_file_get_star_flags(gCurrSaveFileNum - 1, COURSE_NUM_TO_INDEX(gCurrCourseNum));
  s32 obtained = save_file_get_course_star_count(gCurrSaveFileNum - 1, COURSE_NUM_TO_INDEX(gCurrCourseNum));
  s32 got = 0, first = -1;
  sVisibleStars = 0;
  while (got != obtained && sVisibleStars < 6) {
    if (stars & (1 << sVisibleStars)) sStarModel[sVisibleStars] = 2, got++;
    else {
      sStarModel[sVisibleStars] = 1;
      if (first < 0) first = sVisibleStars;
    }
    sVisibleStars++;
  }
  /* (collected in order so far: the next one shows) */
  if (sVisibleStars == obtained && sVisibleStars != 6) {
    sStarModel[sVisibleStars] = 1;
    first = sVisibleStars++;
  }
  sSelected = (s8) (first >= 0 ? first : sVisibleStars - 1);
  sDone = 0;
  sActTimer = 0;
  sSpin = 0;
  port_menu_active = 1;
  return 0;
}

s32 port_act_select_loop(s16 arg, s32 unused) {
  sActTimer++;
  sSpin += 0x400;
  if (sDone) {
    /* (a moment for the chosen star, as the game waited) */
    if (++sDone > 20) {
      port_menu_active = 0;
      gDialogCourseActNum = sSelected + 1;
      return sSelected + 1;
    }
    return 0;
  }
  s16 stickX = gPlayer1Controller->rawStickX;
  static s8 sHeld;
  if (stickX > 60 || stickX < -60) {
    if (!sHeld) {
      sSelected = (s8) (sSelected + (stickX > 0 ? 1 : -1));
      if (sSelected < 0) sSelected = (s8) (sVisibleStars - 1);
      if (sSelected >= sVisibleStars) sSelected = 0;
    }
    sHeld = 1;
  } else sHeld = 0;
  if (sActTimer > 10 && (gPlayer1Controller->buttonPressed & (A_BUTTON | START_BUTTON | B_BUTTON))) sDone = 1;
  return 0;
}

/* a star of polygons: ten points around a middle, (x, y) on the N64's screen (y up), r its size, its yaw */
static void draw_star(s16 x, s16 y, f32 r, s16 yaw, u8 cr, u8 cg, u8 cb, u8 ca) {
  Vtx *v = alloc_display_list(11 * sizeof(Vtx));
  if (v == NULL) return;
  f32 sx = coss(yaw);
  for (s32 i = 0; i <= 10; i++) {
    f32 rr = i == 0 ? 0 : (i % 2 ? r : r * 0.45f);
    f32 a = (i - 1) * (3.14159265f / 5);
    f32 px = i == 0 ? 0 : sinf(a) * rr, py = i == 0 ? 0 : cosf(a) * rr;
    s32 shade = i == 0 ? 255 : (i % 2 ? 200 : 150);
    v[i].v.ob[0] = (s16) (x + px * sx), v[i].v.ob[1] = (s16) (y + py), v[i].v.ob[2] = 0;
    v[i].v.flag = 0, v[i].v.tc[0] = v[i].v.tc[1] = 0;
    v[i].v.cn[0] = (u8) (cr * shade / 255), v[i].v.cn[1] = (u8) (cg * shade / 255);
    v[i].v.cn[2] = (u8) (cb * shade / 255), v[i].v.cn[3] = ca;
  }
  gSPVertex(gDisplayListHead++, v, 11, 0);
  for (s32 i = 1; i <= 10; i++) gSP1Triangle(gDisplayListHead++, 0, i, i % 10 + 1, 0);
}

void port_menu_draw(void) {
  if (!port_menu_active) return;
  u8 **courses = segmented_to_virtual(seg2_course_name_table);
  u8 **acts = segmented_to_virtual(seg2_act_name_table);
  s32 course = COURSE_NUM_TO_INDEX(gCurrCourseNum);
  u8 *name = segmented_to_virtual(courses[course]);
  /* the white screen (its GEO_BACKGROUND_COLOR) */
  gDPSetCycleType(gDisplayListHead++, G_CYC_FILL);
  gDPSetFillColor(gDisplayListHead++, 0xFFFFFFFF);
  gDPFillRectangle(gDisplayListHead++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
  gDPSetCycleType(gDisplayListHead++, G_CYC_1CYCLE);
  /* the stars */
  create_dl_ortho_matrix();
  gDPSetCombineMode(gDisplayListHead++, G_CC_SHADE, G_CC_SHADE);
  gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
  for (s32 i = 0; i < sVisibleStars; i++) {
    s16 x = (s16) (160 - (sVisibleStars - 1) * 17 + i * 34);
    f32 r = i == sSelected ? 18 : 14;
    s16 yaw = i == sSelected ? sSpin : 0;
    if (sStarModel[i] == 2) draw_star(x, 175, r, yaw, 110, 150, 255, 170);
    else draw_star(x, 175, r, yaw, 255, 210, 30, 255);
  }
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  /* the stars' numbers, the chosen act's name, the course's name, in black */
  gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, 255);
  for (s32 i = 0; i < sVisibleStars; i++) {
    u8 digit[2] = {(u8) (i + 1), DIALOG_CHAR_TERMINATOR};
    print_generic_string((s16) (157 - (sVisibleStars - 1) * 17 + i * 34), 196, digit);
  }
  if (sVisibleStars > 0) {
    u8 *act = segmented_to_virtual(acts[course * 6 + sSelected]);
    print_generic_string(get_str_x_pos_from_center(160, act, 10.0f), 140, act);
  }
  if (save_file_get_course_coin_score(gCurrSaveFileNum - 1, course) != 0) {
    u8 score[] = {TEXT_MYSCORE};
    print_generic_string(95, 118, score);
  }
  print_generic_string(get_str_x_pos_from_center(160, name + 3, 10.0f), 33, name + 3);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  if (save_file_get_course_coin_score(gCurrSaveFileNum - 1, course) != 0)
    print_hud_my_score_coins(1, gCurrSaveFileNum - 1, course, 155, 106);
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
}
