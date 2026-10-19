/* Made by tools/pack.py: the game's functions its models name (geo layouts' switches, held objects and
 * GEO_ASM), by the number tools/models.py gave them. */
#include <ultra64.h>
#include "types.h"
#include "engine/graph_node.h"

Gfx *geo_mirror_mario_backface_culling();
Gfx *geo_mirror_mario_set_alpha();
Gfx *geo_switch_mario_stand_run();
Gfx *geo_switch_mario_cap_effect();
Gfx *geo_move_mario_part_from_parent();
Gfx *geo_mario_tilt_torso();
Gfx *geo_mario_head_rotation();
Gfx *geo_switch_mario_cap_on_off();
Gfx *geo_switch_mario_eyes();
Gfx *geo_mario_rotate_wing_cap_wings();
Gfx *geo_switch_mario_hand();
Gfx *geo_mario_hand_foot_scaler();
Gfx *geo_switch_mario_hand_grab_pos();
Gfx *geo_switch_anim_state();
Gfx *geo_update_layer_transparency();
Gfx *geo_scale_bowser_key();
Gfx *geo_update_held_mario_pos();
Gfx *geo_switch_peach_eyes();

const GraphNodeFunc port_geo_funcs[19] = {
    NULL,
    (GraphNodeFunc) geo_mirror_mario_backface_culling,
    (GraphNodeFunc) geo_mirror_mario_set_alpha,
    (GraphNodeFunc) geo_switch_mario_stand_run,
    (GraphNodeFunc) geo_switch_mario_cap_effect,
    (GraphNodeFunc) geo_move_mario_part_from_parent,
    (GraphNodeFunc) geo_mario_tilt_torso,
    (GraphNodeFunc) geo_mario_head_rotation,
    (GraphNodeFunc) geo_switch_mario_cap_on_off,
    (GraphNodeFunc) geo_switch_mario_eyes,
    (GraphNodeFunc) geo_mario_rotate_wing_cap_wings,
    (GraphNodeFunc) geo_switch_mario_hand,
    (GraphNodeFunc) geo_mario_hand_foot_scaler,
    (GraphNodeFunc) geo_switch_mario_hand_grab_pos,
    (GraphNodeFunc) geo_switch_anim_state,
    (GraphNodeFunc) geo_update_layer_transparency,
    (GraphNodeFunc) geo_scale_bowser_key,
    (GraphNodeFunc) geo_update_held_mario_pos,
    (GraphNodeFunc) geo_switch_peach_eyes,
};
