/* (calculator) the surfaces are in src/port/gen/terrain.c */
// 0x07068B88 - 0x0706E362
const Collision inside_castle_seg7_area_1_collision[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_SPECIAL_INIT(11),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_null_start,        /*pos*/ -1023,    0,  1152, /*yaw*/ 128), // unused, probably an early way to set initial position
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_wooden_door,       /*pos*/  -271,    0,  -824, /*yaw*/  32),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_wooden_door,       /*pos*/ -1775,    0,  -824, /*yaw*/ 224),
    SPECIAL_OBJECT_WITH_YAW_AND_PARAM(/*preset*/ special_castle_door_warp,  /*pos*/ -1100,    0,  2202, /*yaw*/   0, /*bhvParam2*/ 0),
    SPECIAL_OBJECT_WITH_YAW_AND_PARAM(/*preset*/ special_castle_door_warp,  /*pos*/ -946,     0,  2202, /*yaw*/ 128, /*bhvParam2*/ 1),
    SPECIAL_OBJECT_WITH_YAW_AND_PARAM(/*preset*/ special_wooden_door_warp,  /*pos*/ -1023, -101, -5170, /*yaw*/   0, /*bhvParam2*/ 2),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_0stars_door,       /*pos*/ -3122,  205,  -793, /*yaw*/  64),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_1star_door,        /*pos*/   256,    0, -1074, /*yaw*/   0),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_1star_door,        /*pos*/   644,  614, -1476, /*yaw*/ 224),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_3star_door,        /*pos*/  1075,  205,  -229, /*yaw*/ 192),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_3star_door,        /*pos*/ -2303,    0, -1074, /*yaw*/   0),
    COL_END(),
};
