/* (calculator) the surfaces are in src/port/gen/terrain.c */
// 0x070102D8 - 0x07011C36
const Collision wf_seg7_collision_070102D8[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_SPECIAL_INIT(6),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_null_start,   /*pos*/  2560,  256, 5120, /*yaw*/ 64), // unused, probably an early way to set initial position
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_level_geo_0E, /*pos*/  3584,  154, 4864, /*yaw*/ 0),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_level_geo_0F, /*pos*/  4608,  256, 1792, /*yaw*/ 0),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_level_geo_11, /*pos*/ -2499, 1792, -261, /*yaw*/ 0),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_level_geo_12, /*pos*/ -2661,  384,  640, /*yaw*/ 0),
    SPECIAL_OBJECT(/*preset*/ special_bubble_tree,  /*pos*/  2560,  256, 4608),
    COL_WATER_BOX_INIT(1),
    COL_WATER_BOX(0, -1023, 1024, 3226, 4096, 973),
    COL_END(),
};
