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
#include "data_ids.h"
#include "../render.h"

/* the screen of the calculator's that is drawn over the frame (port_menu_draw) */
enum { SCREEN_NONE, SCREEN_ACT, SCREEN_LOGO, SCREEN_HEAD, SCREEN_FILES, SCREEN_KEYS };
s32 port_menu_active;
/* (ingame_menu.c's: the pause menu is up; a scale of the menus' matrix) */
extern s16 gMenuMode;
void create_dl_scale_matrix(s8 pushOp, f32 x, f32 y, f32 z);
void create_dl_rotation_matrix(s8 pushOp, f32 a, f32 x, f32 y, f32 z);
void shade_screen(void);
/* (the menus' display lists: src/port/gen/menu_data.c) */
extern const Gfx dl_menu_rgba16_wood_course[];
extern const Gfx dl_menu_ia8_text_begin[];
extern const Gfx dl_menu_ia8_text_end[];
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

/* ------------------------------------------------------------------------------------------------ the title */
/* The game's way in: its title (intro_geo.c: the SUPER MARIO 64 logo zooming in, Nintendo's copyright fading
 * in, the logo zooming away), the screen of Mario's head on its backdrop, then the file select (file_select.c:
 * SELECT FILE, the four files and their stars, the hand). A new file shows the calculator's keys first, as the
 * game's own dialog box; Shift shows them from the title and the file select. */
static s32 sTitleTimer, sScreenTimer;
static s8 sStartAfterKeys, sKeysFrom, sFile;
static s32 sLogoFrame, sCopyrightAlpha;

static void set_screen(s32 screen) {
  port_menu_active = screen;
  sScreenTimer = 0;
}

s32 port_title_init(s16 arg, s32 unused) {
  sTitleTimer = 0, sLogoFrame = 0, sCopyrightAlpha = 0, sFile = 0;
  /* (tests that start in a course go straight to the file select) */
  set_screen(port_start_level() != LEVEL_CASTLE_GROUNDS ? SCREEN_FILES : SCREEN_LOGO);
  return 0;
}

/* the logo's frames: zooming in, held, zooming away (intro_geo.c's INTRO_STEPS_*), then the head */
#define LOGO_ZOOM_IN 20
#define LOGO_HOLD 75
#define LOGO_ZOOM_OUT 91

s32 port_title_loop(s16 arg, s32 unused) {
  u16 pressed = gPlayer1Controller->buttonPressed;
  sTitleTimer++, sScreenTimer++;
  switch (port_menu_active) {
    case SCREEN_LOGO:
      /* (geo_intro_super_mario_64_logo, geo_intro_tm_copyright) */
      sLogoFrame++;
      if (sLogoFrame >= 19 && (sCopyrightAlpha += 26) > 255) sCopyrightAlpha = 255;
      if (sLogoFrame >= LOGO_ZOOM_OUT + 10 || (sScreenTimer > 5 && (pressed & (A_BUTTON | START_BUTTON))))
        set_screen(SCREEN_HEAD);
      break;
    case SCREEN_HEAD:
      if (sScreenTimer < 5) break;
      if (pressed & Z_TRIG) sKeysFrom = SCREEN_HEAD, sStartAfterKeys = 0, set_screen(SCREEN_KEYS);
      else if (pressed & (A_BUTTON | START_BUTTON)) set_screen(SCREEN_FILES);
      break;
    case SCREEN_FILES:
      if (sScreenTimer < 5) break;
      /* the hand goes from file to file (A B on top, C D below) */
      {
        static s8 sHeld;
        s8 x = gPlayer1Controller->rawStickX, y = gPlayer1Controller->rawStickY;
        if (x > 60 || x < -60 || y > 60 || y < -60) {
          if (!sHeld) sFile = (s8) (x > 60 || x < -60 ? sFile ^ 1 : sFile ^ 2);
          sHeld = 1;
        } else sHeld = 0;
      }
      if (pressed & Z_TRIG) sKeysFrom = SCREEN_FILES, sStartAfterKeys = 0, set_screen(SCREEN_KEYS);
      else if (pressed & B_BUTTON) set_screen(SCREEN_HEAD);
      else if (pressed & (A_BUTTON | START_BUTTON)) {
        /* (a new game: the keys first) */
        if (!save_file_exists(sFile)) {
          sStartAfterKeys = 1;
          set_screen(SCREEN_KEYS);
          break;
        }
        set_screen(SCREEN_NONE);
        return sFile + 1;
      }
      break;
    case SCREEN_KEYS:
      if (sScreenTimer < 5) break;
      if (pressed & (A_BUTTON | START_BUTTON | B_BUTTON)) {
        if (sStartAfterKeys && !(pressed & B_BUTTON)) {
          set_screen(SCREEN_NONE);
          return sFile + 1;
        }
        set_screen(sStartAfterKeys ? SCREEN_FILES : sKeysFrom);
        sStartAfterKeys = 0;
      }
      break;
  }
  return 0;
}
s32 port_title_start_level(s16 arg, s32 unused) {
  s32 level = port_start_level();
  /* (a test that starts in a course: the file exists, no opening scene with Peach's letter) */
  if (level != LEVEL_CASTLE_GROUNDS) save_file_set_flags(SAVE_FLAG_FILE_EXISTS);
  return level;
}

extern const f32 intro_seg7_table_0700C790[], intro_seg7_table_0700C880[];
extern const Gfx intro_seg7_dl_copyright[];
static void draw_logo(void) {
  /* the title's area (intro_geo_0002D0): black, its camera, the logo at its frame's scale */
  const f32 *t = NULL;
  if (sLogoFrame < LOGO_ZOOM_IN) t = intro_seg7_table_0700C790 + sLogoFrame * 3;
  else if (sLogoFrame >= LOGO_HOLD && sLogoFrame < LOGO_ZOOM_OUT) t = intro_seg7_table_0700C880 + (sLogoFrame - LOGO_HOLD) * 3;
  f32 sx = t ? t[0] : sLogoFrame < LOGO_HOLD ? 1 : 0, sy = t ? t[1] : sx, sz = t ? t[2] : sx;
  port_screen_camera(0, 0, 3200, 0, 0, 0, 45, 128, 16384, 0x0001);
  if (sx > 0) port_screen_model_scaled(PORT_SCREEN_LOGO, 0, 0, 0, 0, 0, 0, sx, sy, sz, 255);
  gSPDisplayList(gDisplayListHead++, dl_proj_mtx_fullscreen);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, sCopyrightAlpha);
  if (sCopyrightAlpha < 255) {
    gDPSetRenderMode(gDisplayListHead++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
  } else {
    gDPSetRenderMode(gDisplayListHead++, G_RM_AA_OPA_SURF, G_RM_AA_OPA_SURF2);
  }
  gSPDisplayList(gDisplayListHead++, intro_seg7_dl_copyright);
}

/* the backdrop of Mario's head (geo_intro_regular_backdrop: its picture twelve times), drawn as the sky */
static void draw_backdrop(void) {
  port_screen_camera(0, 0, 1000, 0, 0, 0, 45, 100, 25000, 0x0001);
  r_sky(R_SKY_TITLE, 0, 0xFFFFFF);
  create_dl_ortho_matrix();
}

static void draw_head(void) {
  port_draw_mario_head(sScreenTimer);
  create_dl_ortho_matrix();
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  if (sTitleTimer / 15 % 2 == 0) print_text_at(-1, 20, "PRESS START");
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

/* a file's stars, or NEW (file_select.c's print_save_file_star_count) */
static void print_save_file_star_count(s8 fileIndex, s16 x, s16 y) {
  u8 starIcon[] = {GLYPH_STAR, GLYPH_SPACE}, xIcon[] = {GLYPH_MULTIPLY, GLYPH_SPACE}, textNew[] = {TEXT_NEW};
  u8 starCountText[4];
  starIcon[1] = xIcon[1] = DIALOG_CHAR_TERMINATOR;
  if (save_file_exists(fileIndex)) {
    s16 starCount = save_file_get_total_star_count(fileIndex, COURSE_MIN - 1, COURSE_MAX - 1);
    s8 offset = 0;
    print_hud_lut_string(HUD_LUT_GLOBAL, x, y, starIcon);
    if (starCount < 100) {
      print_hud_lut_string(HUD_LUT_GLOBAL, x + 16, y, xIcon);
      offset = 16;
    }
    int_to_str(starCount, starCountText);
    print_hud_lut_string(HUD_LUT_GLOBAL, x + (offset + 16), y, starCountText);
  } else print_hud_lut_string(HUD_LUT_GLOBAL, x, y, textNew);
}

/* the file select's main menu (file_select.c): the yellow background (its file button, nine times as big), the
 * four files' buttons, the four below them, their words, the hand on the chosen file */
extern const Gfx dl_menu_idle_hand[];
static void draw_files(void) {
  static const s16 pos[4][2] = {{-6400, 2800}, {1500, 2800}, {-6400, 0}, {1500, 0}};
  static const s16 below[4][2] = {{-6400, -3500}, {-2134, -3500}, {2134, -3500}, {6400, -3500}};
  static const u8 belowModel[4] = {PORT_SCREEN_SCORE_BUTTON, PORT_SCREEN_COPY_BUTTON, PORT_SCREEN_ERASE_BUTTON,
                                   PORT_SCREEN_SOUND_BUTTON};
  u8 textSelectFile[] = {TEXT_SELECT_FILE}, textScore[] = {TEXT_SCORE}, textCopy[] = {TEXT_COPY};
  u8 textErase[] = {TEXT_ERASE}, textStereo[] = {TEXT_STEREO};
  u8 textMario[4][8] = {{TEXT_FILE_MARIO_A}, {TEXT_FILE_MARIO_B}, {TEXT_FILE_MARIO_C}, {TEXT_FILE_MARIO_D}};
  port_screen_camera(0, 0, 1000, 0, 0, 0, 45, 100, 25000, 0x0001);
  port_screen_model(PORT_SCREEN_FILE_BUTTON, 0, 0, -19000, 0, 0x8000, 0, 9.0f, 255);
  for (s32 i = 0; i < 4; i++) {
    port_screen_model(save_file_exists(i) ? PORT_SCREEN_SAVE_BUTTON : PORT_SCREEN_NEW_BUTTON, pos[i][0], pos[i][1],
                      -19000, 0, 0, 0, 1.0f, 255);
    port_screen_model(belowModel[i], below[i][0], below[i][1], -19000, 0, 0, 0, 1.0f, 255);
  }
  /* (print_main_menu_strings) */
  create_dl_ortho_matrix();
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  print_hud_lut_string(HUD_LUT_GLOBAL, 93, 35, textSelectFile);
  for (s32 i = 0; i < 4; i++) print_save_file_star_count(i, i % 2 ? 209 : 92, i < 2 ? 78 : 118);
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  print_generic_string(52, 39, textScore);
  print_generic_string(117, 39, textCopy);
  print_generic_string(177, 39, textErase);
  print_generic_string(get_str_x_pos_from_center(254, textStereo, 10.0f), 39, textStereo);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
  gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  for (s32 i = 0; i < 4; i++) print_menu_generic_string(i % 2 ? 207 : 92, i < 2 ? 65 : 105, textMario[i]);
  gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);
  /* the hand (print_menu_cursor), on the chosen file's button */
  static const s16 hand[4][2] = {{-97, 56}, {11, 56}, {-97, 16}, {11, 16}};
  create_dl_translation_matrix(MENU_MTX_PUSH, hand[sFile][0] + 160.0f - 5.0f, hand[sFile][1] + 120.0f - 25.0f, 0.0f);
  gSPDisplayList(gDisplayListHead++, dl_menu_idle_hand);
  gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

/* the calculator's keys for the N64 controller's buttons (src/plat.h), in the game's dialog box */
static void draw_keys(void) {
  static const char *const keys[][2] = {
    {"Arrows", "Move (alpha: walk)"}, {"OK", "A: jump, read"}, {"Back", "B: punch, pick up"},
    {"Shift", "Z: crouch, pound"}, {"4  6", "Turn the camera"}, {"8  2", "Camera near, far"},
    {"0", "R: Lakitu or Mario"}, {"Backspace", "Start: pause"}, {"Home", "Save and quit"},
  };
  draw_backdrop();
  /* the box of the game's dialogs (render_dialog_box_type: black, see-through) */
  create_dl_translation_matrix(MENU_MTX_PUSH, 18, 214, 0);
  create_dl_scale_matrix(MENU_MTX_NOPUSH, 2.18f, 2.45f, 1.0f);
  gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, 150);
  gSPDisplayList(gDisplayListHead++, dl_draw_text_bg_box);
  gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  for (u32 i = 0; i < sizeof keys / sizeof keys[0]; i++) {
    s16 y = (s16) (192 - 17 * (s32) i);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
    print_text_at(30, y, keys[i][0]);
    print_text_at(116, y, keys[i][1]);
  }
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
  /* the dialog's arrow: on to the game (render_dialog_triangle_choice's) */
  if (sTitleTimer / 8 % 2 == 0) {
    create_dl_translation_matrix(MENU_MTX_PUSH, 282, 34, 0);
    create_dl_rotation_matrix(MENU_MTX_NOPUSH, -90.0f, 0, 0, 1.0f);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
    gSPDisplayList(gDisplayListHead++, dl_draw_triangle);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
  }
}

/* ------------------------------------------------------------------------------------------------ the act select */
/* star_select.c's act select, on its white screen: the course's stars as the game's star models (the ones got
 * gold, the others clear; the chosen one bigger and turning), their numbers and the chosen act's name in the
 * menu font, the best coin score, the wooden COURSE sign with its number, the course's name. */
static s8 sVisibleStars, sObtainedStars, sInitSelectedActNum, sSelectableStarIndex, sSelectedActIndex;
static s8 sLoadedActNum;
static u8 sStarGot[7];          /* a gold star (else a clear one); [6]: the 100 coin star */
static f32 sStarSize[7];
static s16 sStarYaw[7];
static s16 sActSelectorMenuTimer;

s32 port_act_select_init(s16 arg, s32 unused) {
  /* (lvl_init_act_selector_values_and_stars) */
  u8 stars = save_file_get_star_flags(gCurrSaveFileNum - 1, COURSE_NUM_TO_INDEX(gCurrCourseNum));
  sLoadedActNum = 0, sInitSelectedActNum = 0, sVisibleStars = 0, sActSelectorMenuTimer = 0;
  sSelectableStarIndex = 0, sSelectedActIndex = 0;
  sObtainedStars = (s8) save_file_get_course_star_count(gCurrSaveFileNum - 1, COURSE_NUM_TO_INDEX(gCurrCourseNum));
  if (stars & (1 << 6)) sObtainedStars--;
  /* (bhv_act_selector_init) */
  s32 i = 0;
  while (i != sObtainedStars) {
    if (stars & (1 << sVisibleStars)) sStarGot[sVisibleStars] = 1, i++;
    else {
      sStarGot[sVisibleStars] = 0;
      if (sInitSelectedActNum == 0) {
        sInitSelectedActNum = sVisibleStars + 1;
        sSelectableStarIndex = sVisibleStars;
      }
    }
    sVisibleStars++;
  }
  if (sVisibleStars == sObtainedStars && sVisibleStars != 6) {
    sStarGot[sVisibleStars] = 0;
    sInitSelectedActNum = sVisibleStars + 1;
    sSelectableStarIndex = sVisibleStars;
    sVisibleStars++;
  }
  if (sObtainedStars == 6) sInitSelectedActNum = sVisibleStars;
  if (sObtainedStars == 0) sInitSelectedActNum = 1;
  for (i = 0; i < 7; i++) sStarSize[i] = 1.0f, sStarYaw[i] = 0;
  sStarGot[6] = (stars & (1 << 6)) != 0;
  sStarSize[6] = 0.8f;
  port_menu_active = SCREEN_ACT;
  return 0;
}

s32 port_act_select_loop(s16 arg, s32 unused) {
  u8 stars = save_file_get_star_flags(gCurrSaveFileNum - 1, COURSE_NUM_TO_INDEX(gCurrCourseNum));
  /* (bhv_act_selector_loop: only the stars got and the first not got can be chosen) */
  if (sObtainedStars != 6) {
    sSelectedActIndex = 0;
    handle_menu_scrolling(MENU_SCROLL_HORIZONTAL, &sSelectableStarIndex, 0, sObtainedStars);
    s32 left = sSelectableStarIndex;
    for (s32 i = 0; i < sVisibleStars; i++)
      if ((stars & (1 << i)) || i == sInitSelectedActNum - 1) {
        if (left == 0) {
          sSelectedActIndex = (s8) i;
          break;
        }
        left--;
      }
  } else {
    handle_menu_scrolling(MENU_SCROLL_HORIZONTAL, &sSelectableStarIndex, 0, sVisibleStars - 1);
    sSelectedActIndex = sSelectableStarIndex;
  }
  /* (bhv_act_selector_star_type_loop) */
  for (s32 i = 0; i < sVisibleStars; i++) {
    if (i == sSelectedActIndex) {
      sStarSize[i] += 0.1f;
      if (sStarSize[i] > 1.3f) sStarSize[i] = 1.3f;
      sStarYaw[i] += 0x800;
    } else {
      sStarSize[i] -= 0.1f;
      if (sStarSize[i] < 1.0f) sStarSize[i] = 1.0f;
      sStarYaw[i] = 0;
    }
  }
  sStarYaw[6] += 0x800;
  /* (lvl_update_obj_and_load_act_button_actions) */
  if (sActSelectorMenuTimer >= 11 && (gPlayer3Controller->buttonPressed & (A_BUTTON | START_BUTTON | B_BUTTON))) {
    sLoadedActNum = sInitSelectedActNum >= sSelectedActIndex + 1 ? sSelectedActIndex + 1 : sInitSelectedActNum;
    gDialogCourseActNum = sSelectedActIndex + 1;
  }
  sActSelectorMenuTimer++;
  if (sLoadedActNum) port_menu_active = SCREEN_NONE;
  return sLoadedActNum;
}


static void draw_act_select(void) {
  /* the act selector's area (geo_menu_act_selector_strings): its camera, its white background, the stars */
  port_screen_camera(0, 0, 1000, 0, 0, 0, 45, 100, 25000, 0xFFFF);
  for (s32 i = 0; i < sVisibleStars; i++)
    port_screen_model(sStarGot[i] ? PORT_SCREEN_STAR : PORT_SCREEN_TRANSPARENT_STAR,
                      (sVisibleStars - 1) * -75 + i * 152, 248, -300, 0, sStarYaw[i], 0, sStarSize[i], 255);
  if (sStarGot[6]) port_screen_model(PORT_SCREEN_STAR, 370, 24, -300, 0, sStarYaw[6], 0, sStarSize[6], 255);
  /* (print_act_selector_strings) */
  u8 **courses = segmented_to_virtual(seg2_course_name_table);
  u8 **acts = segmented_to_virtual(seg2_act_name_table);
  s32 course = COURSE_NUM_TO_INDEX(gCurrCourseNum);
  u8 *name = segmented_to_virtual(courses[course]);
  u8 myScore[] = {TEXT_MYSCORE};
  u8 starNumbers[2] = {0, DIALOG_CHAR_TERMINATOR};
  create_dl_ortho_matrix();
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  print_hud_my_score_coins(1, gCurrSaveFileNum - 1, course, 155, 106);
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, 255);
  if (save_file_get_course_coin_score(gCurrSaveFileNum - 1, course) != 0) print_generic_string(102, 118, myScore);
  print_generic_string(get_str_x_pos_from_center(160, name + 3, 10.0f), 33, name + 3);
  gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
  /* (print_course_number) */
  u8 courseNum[4];
  create_dl_translation_matrix(MENU_MTX_PUSH, 158.0f, 81.0f, 0.0f);
  gSPDisplayList(gDisplayListHead++, dl_menu_rgba16_wood_course);
  gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
  int_to_str(gCurrCourseNum, courseNum);
  print_hud_lut_string(HUD_LUT_GLOBAL, gCurrCourseNum < 10 ? 152 : 143, 158, courseNum);
  gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
  gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
  gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, 255);
  if (sVisibleStars != 0) {
    u8 *act = segmented_to_virtual(acts[course * 6 + sSelectedActIndex]);
    print_menu_generic_string(get_str_x_pos_from_center(163, act, 8.0f), 81, act);
  }
  for (s32 i = 1; i <= sVisibleStars; i++) {
    starNumbers[0] = (u8) i;
    print_menu_generic_string(122 - (sVisibleStars - 1) * 17 + i * 34, 38, starNumbers);
  }
  gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);
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
  if (port_menu_active == SCREEN_LOGO) draw_logo();
  if (port_menu_active == SCREEN_HEAD) draw_head();
  if (port_menu_active == SCREEN_FILES) draw_files();
  if (port_menu_active == SCREEN_KEYS) draw_keys();
  if (port_menu_active == SCREEN_ACT) draw_act_select();
}
