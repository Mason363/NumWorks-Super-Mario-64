/* The level scripts around the game: the decompilation's intro (the Mario head), file select and act select
 * (levels/intro, levels/menu) drew themselves with the N64's display lists; the calculator's versions keep
 * their flow and call the calculator's screens (src/port/menu.c). */
#include <ultra64.h>
#include "sm64.h"
#include "level_commands.h"
#include "game/level_update.h"
#include "levels/scripts.h"
#include "port.h"
#include "port_game.h"
#include "port_menu.h"

#include "make_const_nonconst.h"

/* the title and the file select, then the castle grounds (levels/menu/script.c's level_main_menu_entry_1) */
const LevelScript level_main_menu_entry_1[] = {
    INIT_LEVEL(),
    BLACKOUT(/*active*/ FALSE),
    CALL(/*arg*/ 0, /*func*/ port_title_init),
    CALL_LOOP(/*arg*/ 0, /*func*/ port_title_loop),
    GET_OR_SET(/*op*/ OP_SET, /*var*/ VAR_CURR_SAVE_FILE_NUM),
    CALL(/*arg*/ 0, /*func*/ port_title_start_level),
    CLEAR_LEVEL(),
    SLEEP_BEFORE_EXIT(/*frames*/ 1),
    EXIT_AND_EXECUTE(/*seg*/ 0x15, _scriptsSegmentRomStart, _scriptsSegmentRomEnd, level_main_scripts_entry),
};

/* the act select, when Mario jumps into a course's painting (levels/menu/script.c's level_main_menu_entry_2) */
const LevelScript level_main_menu_entry_2[] = {
    /*0*/ CALL(/*arg*/ 0, /*func*/ lvl_set_current_level),
    /*2*/ JUMP_IF(/*op*/ OP_EQ, /*arg*/ 0, level_main_menu_entry_2 + 11),
    /*5*/ CALL(/*arg*/ 0, /*func*/ port_act_select_init),
    /*7*/ CALL_LOOP(/*arg*/ 0, /*func*/ port_act_select_loop),
    /*9*/ GET_OR_SET(/*op*/ OP_SET, /*var*/ VAR_CURR_ACT_NUM),
    /*10*/ SLEEP_BEFORE_EXIT(/*frames*/ 1),
    // L1:
    /*11*/ EXIT(),
};

/* the game starts at the title (levels/entry.c's level_script_entry, without the Mario head) */
const LevelScript port_level_script_entry[] = {
    INIT_LEVEL(),
    SLEEP(/*frames*/ 2),
    BLACKOUT(/*active*/ FALSE),
    SET_REG(/*value*/ 0),
    EXECUTE(/*seg*/ 0x14, NULL, NULL, level_main_menu_entry_1),
    JUMP(/*target*/ port_level_script_entry),
};

/* the intro's scripts lead to the title: a game over, the Mario head after the ending */
const LevelScript level_intro_splash_screen[] = {
    EXIT_AND_EXECUTE(/*seg*/ 0x14, NULL, NULL, level_main_menu_entry_1),
};
const LevelScript level_intro_mario_head_regular[] = {
    EXIT_AND_EXECUTE(/*seg*/ 0x14, NULL, NULL, level_main_menu_entry_1),
};
const LevelScript level_intro_mario_head_dizzy[] = {
    EXIT_AND_EXECUTE(/*seg*/ 0x14, NULL, NULL, level_main_menu_entry_1),
};
const LevelScript level_intro_entry_4[] = {
    EXIT_AND_EXECUTE(/*seg*/ 0x14, NULL, NULL, level_main_menu_entry_1),
};
