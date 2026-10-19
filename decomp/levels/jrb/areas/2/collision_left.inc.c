/* (calculator) the surfaces are in src/port/gen/terrain.c */
// 0x070105B8 - 0x07011370
const Collision jrb_seg7_area_2_collision[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_SPECIAL_INIT(1),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_null_start, /*pos*/ 866, 798, -1248, /*yaw*/ 128), // unused, probably an early way to set initial position
    COL_WATER_BOX_INIT(1),
    COL_WATER_BOX(0, -4095, -4095,  4096,  4096,  2765),
    COL_END(),
};
