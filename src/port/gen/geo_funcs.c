/* Made by tools/pack.py: the game's functions its models name (geo layouts' switches, held objects and
 * GEO_ASM), by the number tools/models.py gave them. */
#include <ultra64.h>
#include "types.h"
#include "engine/graph_node.h"
#include "game/mario_misc.h"
#include "game/object_helpers.h"
#include "game/behavior_actions.h"
#include "game/mario_actions_cutscene.h"
#include "game/geo_misc.h"
#include "game/moving_texture.h"
#include "game/paintings.h"
#include "game/envfx_snow.h"
#include "game/level_geo.h"
#include "game/camera.h"
#include "game/skybox.h"

const GraphNodeFunc port_geo_funcs[24] = {
    NULL,
    (GraphNodeFunc) geo_update_layer_transparency,
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
    (GraphNodeFunc) geo_scale_bowser_key,
    (GraphNodeFunc) geo_update_held_mario_pos,
    (GraphNodeFunc) geo_switch_peach_eyes,
    (GraphNodeFunc) geo_snufit_move_mask,
    (GraphNodeFunc) geo_snufit_scale_body,
    (GraphNodeFunc) geo_update_body_rot_from_parent,
    (GraphNodeFunc) geo_update_projectile_pos_from_parent,
    (GraphNodeFunc) geo_switch_tuxie_mother_eyes,
};
