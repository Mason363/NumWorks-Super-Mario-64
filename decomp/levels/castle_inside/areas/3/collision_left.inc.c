/* (calculator) the surfaces are in src/port/gen/terrain.c */
// 0x07073AAC - 0x070775B4
const Collision inside_castle_seg7_area_3_collision[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_SPECIAL_INIT(5),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_wooden_door,     /*pos*/ -3097, -1279,  1434, /*yaw*/   0),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_wooden_door,     /*pos*/  -332,  -767,   205, /*yaw*/   0),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_wooden_door,     /*pos*/   717, -1177,  -869, /*yaw*/ 192),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_0stars_door,     /*pos*/  1126, -1074, -2661, /*yaw*/ 192),
    SPECIAL_OBJECT_WITH_YAW_AND_PARAM(/*preset*/ special_metal_door_warp, /*pos*/  7885, -1586,  -511, /*yaw*/ 192, /*bhvParam2*/ 2),
    COL_WATER_BOX_INIT(3),
    COL_WATER_BOX(1, 973, -1279, 5786, 256, -1228),
    COL_WATER_BOX(0, -3225, -4146, 870, -255, -1228),
    COL_WATER_BOX(2, 5786, -1330, 8038, 410, -1228),
    COL_END(),
};

// 0x070775B4 - 0x0707768C
const Collision inside_castle_seg7_collision_ddd_warp[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_END(),
};

// 0x0707768C - 0x07077764
const Collision inside_castle_seg7_collision_ddd_warp_2[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_END(),
};
