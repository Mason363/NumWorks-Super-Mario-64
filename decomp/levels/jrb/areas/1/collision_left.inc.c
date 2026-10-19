/* (calculator) the surfaces are in src/port/gen/terrain.c */
// 0x0700B058 - 0x0700C4E8
const Collision jrb_seg7_area_1_collision[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_SPECIAL_INIT(1),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_null_start, /*pos*/ -6750, 1126, 1482, /*yaw*/ 64), // unused, probably an early way to set initial position
    COL_WATER_BOX_INIT(3),
    COL_WATER_BOX(0, -6304, -669, 7992, 7814, 1024),
    COL_WATER_BOX(1, 4433, -4253, 5969, -669, -3016),
    COL_WATER_BOX(51, -7818, -1125, 9055, 7814, 1229),
    COL_END(),
};
