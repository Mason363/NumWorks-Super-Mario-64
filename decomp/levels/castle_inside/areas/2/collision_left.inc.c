/* (calculator) the surfaces are in src/port/gen/terrain.c */
// 0x0706E364 - 0x07073AAA
const Collision inside_castle_seg7_area_2_collision[] = {
    COL_INIT(),
    COL_VERTEX_INIT(0),
    COL_TRI_STOP(),
    COL_SPECIAL_INIT(4),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_0stars_door, /*pos*/ -2695, 1203,  1828, /*yaw*/  64),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_0stars_door, /*pos*/  6994, 1203,  1828, /*yaw*/  64),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_0stars_door, /*pos*/  1670, 1203,  1828, /*yaw*/ 192),
    SPECIAL_OBJECT_WITH_YAW(/*preset*/ special_wooden_door, /*pos*/  -997, 1203,  1178, /*yaw*/   0),
    COL_END(),
};
