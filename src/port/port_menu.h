/* The screens around the game, on the calculator (src/port/menu.c). */
#ifndef SM_PORT_MENU_H
#define SM_PORT_MENU_H
#include <PR/ultratypes.h>

/* the title and file select: the loop returns the save file chosen (1 to 4), 0 while it goes on */
s32 port_title_init(s16 arg, s32 unused);
s32 port_title_loop(s16 arg, s32 unused);
/* the level the file starts in */
s32 port_title_start_level(s16 arg, s32 unused);
s32 port_start_level(void);
/* the act select: the loop returns the act chosen (1 to 6), 0 while it goes on */
s32 port_act_select_init(s16 arg, s32 unused);
s32 port_act_select_loop(s16 arg, s32 unused);
/* draws the screen that is on, if one is, in the frame's display list (src/port/scene.c's port_render) */
extern s32 port_menu_active;
void port_menu_draw(void);
/* the keys the screens take before the game sees them (the pause menu's way out) */
void port_menu_input(void);
/* the screens' scenes (src/port/scene.c): their camera (and background, RGBA 5551), then models (data_ids.h's
 * PORT_SCREEN_*) where the game's menus put them */
void port_screen_camera(f32 x, f32 y, f32 z, f32 fx, f32 fy, f32 fz, f32 fov, f32 near, f32 far, u16 background);
void port_screen_model(int model, f32 x, f32 y, f32 z, s16 pitch, s16 yaw, s16 roll, f32 scale, u8 alpha);
void port_screen_model_scaled(int model, f32 x, f32 y, f32 z, s16 pitch, s16 yaw, s16 roll, f32 sx, f32 sy, f32 sz,
                              u8 alpha);
/* Mario's head, as on the screen after the title (goddard's), t frames after it shows */
void port_draw_mario_head(s32 t);
/* the player chose to quit (src/main.c) */
extern int port_quit;
/* the save file saved, then the app ends (the pause menu's SAVE & QUIT, the Home key) */
void port_save_and_quit(void);
#endif
