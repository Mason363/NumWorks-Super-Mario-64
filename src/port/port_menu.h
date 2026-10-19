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
#endif
