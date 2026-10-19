/* (calculator) the surfaces are in src/port/gen/terrain.c */
// 0x07020880 - 0x07023468
const Collision ccm_seg7_area_2_collision[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_SPECIAL_INIT(2),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_null_start,       /*pos*/ -5836,  7465, -6143, /*yaw*/ 64), // unused, probably an early way to set initial position
    SPECIAL_OBJECT_WITH_YAW_AND_PARAM(/*preset*/ special_wooden_door_warp, /*pos*/ -7628, -5836, -6988, /*yaw*/ 64, /*bhvParam2*/ 20),
    COL_END(),
};
