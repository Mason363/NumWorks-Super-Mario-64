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

/* the screen of the calculator's that is drawn over the frame (port_menu_draw) */
enum { SCREEN_NONE, SCREEN_ACT, SCREEN_TITLE, SCREEN_KEYS };
s32 port_menu_active;
/* (ingame_menu.c's: the pause menu is up; a scale of the menus' matrix) */
extern s16 gMenuMode;
void create_dl_scale_matrix(s8 pushOp, f32 x, f32 y, f32 z);
void shade_screen(void);
/* the player chose to quit, from the pause menu (src/main.c stops) */
int port_quit;

/* ------------------------------------------------------------------------------------------------ text */
/* the game's characters for plain text (charmap.txt's US values) */
static void to_game_text(const char *s, u8 *out) {
  for (; *s; s++) {
    char c = *s;
    *out++ = c >= '0' && c <= '9' ? (u8) (c - '0') : c >= 'A' && c <= 'Z' ? (u8) (0x0A + c - 'A')
           : c >= 'a' && c <= 'z' ? (u8) (0x24 + c - 'a') : c == '\'' ? 0x3E : c == '.' ? 0x3F : c == ',' ? 0x6F
           : c == '-' ? 0x9F : c == ':' ? 0xE6 : c == '(' ? 0xE1 : c == ')' ? 0xE3 : c == '!' ? 0xF2
           : c == '?' ? 0xF4 : c == '/' ? 0xD0 : c == '&' ? 0xE5 : 0x9E;
  }
  *out = DIALOG_CHAR_TERMINATOR;
}

/* in the dialogs' font (between dl_ia_text_begin and dl_ia_text_end), y from the bottom; x < 0: centred */
static void print_text_at(s16 x, s16 y, const char *s) {
  u8 buf[48];
  to_game_text(s, buf);
  print_generic_string(x < 0 ? get_str_x_pos_from_center(160, buf, 10.0f) : x, y, buf);
}

/* in the HUD's letters, size 16 or 32 pixels, each letter in turn in one of the n colours (from the first), y from
 * the top */
static void print_hud_at(s16 x, s16 y, s32 size, const char *s, const u8 (*colours)[3], s32 n, s32 first) {
  void **lut = segmented_to_virtual(main_hud_lut);
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
  gDPSetCombineLERP(gDisplayListHead++, TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT,
                    0, TEXEL0, 0, ENVIRONMENT, 0);
  for (s32 i = 0; s[i]; i++) {
    char c = s[i];
    s32 g = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'Z' ? 10 + c - 'A' : -1;
    if (g >= 0 && lut[g] != NULL) {
      const u8 *k = colours[(i + first) % n];
      gDPPipeSync(gDisplayListHead++);
      gDPSetEnvColor(gDisplayListHead++, k[0], k[1], k[2], 255);
      gDPSetTextureImage(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, lut[g]);
      gSPDisplayList(gDisplayListHead++, dl_rgba16_load_tex_block);
      gSPTextureRectangle(gDisplayListHead++, x << 2, y << 2, (x + size) << 2, (y + size) << 2, G_TX_RENDERTILE, 0, 0,
                          (16 << 10) / size, (16 << 10) / size);
    }
    x += (s16) (c == ' ' ? size / 2 : size * 3 / 4);
  }
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
}

static void fill_screen(u8 r, u8 g, u8 b) {
  gDPSetCycleType(gDisplayListHead++, G_CYC_FILL);
  gDPSetFillColor(gDisplayListHead++, GPACK_RGBA5551(r, g, b, 1) << 16 | GPACK_RGBA5551(r, g, b, 1));
  gDPFillRectangle(gDisplayListHead++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
  gDPSetCycleType(gDisplayListHead++, G_CYC_1CYCLE);
}

/* ------------------------------------------------------------------------------------------------ the title */
/* The game's title and its one save file (the N64 had four: here it is file A), then the keys the first time
 * (and with Shift). */
static s32 sTitleTimer;
static s8 sStartAfterKeys;
s32 port_title_init(s16 arg, s32 unused) {
  sTitleTimer = 0;
  port_menu_active = SCREEN_TITLE;
  return 0;
}
s32 port_title_loop(s16 arg, s32 unused) {
  u16 pressed = gPlayer1Controller->buttonPressed;
  if (++sTitleTimer < 10) return 0;
  if (port_menu_active == SCREEN_KEYS) {
    if (pressed & (A_BUTTON | START_BUTTON | B_BUTTON)) {
      port_menu_active = SCREEN_TITLE;
      if (sStartAfterKeys) {
        port_menu_active = SCREEN_NONE;
        return 1;
      }
    }
    return 0;
  }
  if (pressed & Z_TRIG) {
    port_menu_active = SCREEN_KEYS;
    sStartAfterKeys = 0;
  } else if (pressed & (A_BUTTON | START_BUTTON)) {
    /* (a new game: the keys first) */
    if (!save_file_exists(0)) {
      port_menu_active = SCREEN_KEYS;
      sStartAfterKeys = 1;
      return 0;
    }
    port_menu_active = SCREEN_NONE;
    return 1;
  }
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
  port_menu_active = SCREEN_ACT;
  return 0;
}

s32 port_act_select_loop(s16 arg, s32 unused) {
  sActTimer++;
  sSpin += 0x400;
  if (sDone) {
    /* (a moment for the chosen star, as the game waited) */
    if (++sDone > 20) {
      port_menu_active = SCREEN_NONE;
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

static void draw_title(void) {
  static const u8 logo[][3] = {{230, 30, 30}, {250, 200, 20}, {40, 170, 40}, {40, 90, 230}};
  static const u8 white[][3] = {{255, 255, 255}};
  sSpin += 0x300;
  fill_screen(10, 20, 90);
  create_dl_ortho_matrix();
  print_hud_at(95, 30, 32, "SUPER", logo, 4, 0);
  print_hud_at(61, 68, 32, "MARIO 64", logo, 4, 1);
  if (save_file_exists(0)) {
    /* the file's stars */
    gDPSetCombineMode(gDisplayListHead++, G_CC_SHADE, G_CC_SHADE);
    gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    draw_star(140, 100, 13, sSpin, 255, 210, 30, 255);
    char n[8];
    s32 stars = save_file_get_total_star_count(0, COURSE_MIN - 1, COURSE_MAX - 1), i = 7;
    n[i] = 0;
    do n[--i] = (char) ('0' + stars % 10), stars /= 10; while (stars);
    print_hud_at(158, 132, 16, n + i, white, 1, 0);
  }
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  if (sTitleTimer / 15 % 2 == 0) print_text_at(-1, 60, "PRESS START");
  gDPSetEnvColor(gDisplayListHead++, 160, 170, 220, 255);
  print_text_at(-1, 20, "Shift: the keys");
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

/* the calculator's keys for the N64 controller's buttons (src/plat.h) */
static void draw_keys(void) {
  static const char *const keys[][2] = {
    {"Arrows", "Move (alpha held: walk)"}, {"OK, EXE", "A: jump, swim, read"}, {"Back", "B: punch, kick, pick up"},
    {"Shift", "Z: crouch, ground pound"}, {"4  6", "Turn the camera"}, {"8  2", "Camera closer, further"},
    {"0", "R: Mario's or Lakitu's camera"}, {"Backspace", "Start: pause"},
    {"Home", "Quit (stars are saved)"},
  };
  static const u8 gold[][3] = {{255, 210, 40}};
  fill_screen(10, 20, 90);
  create_dl_ortho_matrix();
  print_hud_at(94, 16, 16, "HOW TO PLAY", gold, 1, 0);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  for (u32 i = 0; i < sizeof keys / sizeof keys[0]; i++) {
    s16 y = (s16) (186 - 16 * (s32) i);
    gDPSetEnvColor(gDisplayListHead++, 255, 210, 40, 255);
    print_text_at(22, y, keys[i][0]);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
    print_text_at(112, y, keys[i][1]);
  }
  gDPSetEnvColor(gDisplayListHead++, 160, 170, 220, 255);
  if (sTitleTimer / 15 % 2 == 0) print_text_at(-1, 22, sStartAfterKeys ? "OK: start" : "OK: back");
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

/* ------------------------------------------------------------------------------------------------ quitting */
/* The game's pause menu, with the calculator's way out: Shift asks, in the game's own look (the box of its
 * castle pause menu, its font, its arrow, the choices of its save menu): SAVE & QUIT saves the file (the stars
 * are already saved as Mario gets them, as on the N64) and quits, CONTINUE (or Back) goes back to the pause
 * menu. */
static s8 sAskQuit, sQuitLine;
int port_menu_asking(void) { return sAskQuit; }
void port_menu_input(void) {
  if (gMenuMode != MENU_MODE_RENDER_PAUSE_SCREEN) {
    sAskQuit = 0;
    return;
  }
  u16 pressed = gPlayer1Controller->buttonPressed;
  if (!sAskQuit) {
    if (!(pressed & Z_TRIG)) return;
    sAskQuit = 1, sQuitLine = 1;
  } else if (pressed & (A_BUTTON | START_BUTTON)) {
    if (sQuitLine == 1) {
      port_save_and_quit();
    } else sAskQuit = 0;
  } else if (pressed & B_BUTTON) sAskQuit = 0;
  /* (the game's menu does not see the keys while the question is up) */
  gPlayer1Controller->buttonPressed = gPlayer3Controller->buttonPressed = 0;
}

/* the file saved if anything changed (save_file.c's flags: a door opened, a cannon), then the app ends */
void port_save_and_quit(void) {
  if (gCurrSaveFileNum >= 1 && save_file_exists(gCurrSaveFileNum - 1)) save_file_do_save(gCurrSaveFileNum - 1);
  port_quit = 1;
}

static void draw_quit(void) {
  u8 textSaveAndQuit[] = {TEXT_SAVE_AND_QUIT};
  u8 textContinue[] = {TEXT_CONTINUE};
  if (!sAskQuit) {
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, gMenuTextAlpha);
    print_text_at(-1, 12, "Shift: quit");
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
    return;
  }
  /* the screen darkened and the box (render_pause_screen's shade_screen, render_pause_castle_menu_box's box) */
  shade_screen();
  create_dl_translation_matrix(MENU_MTX_PUSH, 160 - 78, 120 + 32, 0);
  create_dl_scale_matrix(MENU_MTX_NOPUSH, 1.2f, 0.8f, 1.0f);
  gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, 105);
  gSPDisplayList(gDisplayListHead++, dl_draw_text_bg_box);
  gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
  /* the choices and the arrow (render_save_confirmation's) */
  handle_menu_scrolling(MENU_SCROLL_VERTICAL, &sQuitLine, 1, 2);
  s16 x = 160 - 46, y = 126;
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  print_generic_string(x + 12, y, textSaveAndQuit);
  print_generic_string(x + 12, y - 20, textContinue);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
  create_dl_translation_matrix(MENU_MTX_PUSH, x, y - (sQuitLine - 1) * 20, 0);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  gSPDisplayList(gDisplayListHead++, dl_draw_triangle);
  gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

void port_menu_draw(void) {
  if (gMenuMode == MENU_MODE_RENDER_PAUSE_SCREEN) draw_quit();
  if (port_menu_active == SCREEN_TITLE) draw_title();
  if (port_menu_active == SCREEN_KEYS) draw_keys();
  if (port_menu_active != SCREEN_ACT) return;
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
