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
/* the player chose to quit (src/main.c) */
extern int port_quit;
/* the save file saved, then the app ends (the pause menu's SAVE & QUIT, the Home key) */
void port_save_and_quit(void);
#endif
