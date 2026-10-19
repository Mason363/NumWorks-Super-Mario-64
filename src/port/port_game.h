/* The port's own parts (src/port): needs the decompilation's types. */
#ifndef SM_PORT_GAME_H
#define SM_PORT_GAME_H
#include "types.h"
#include "engine/graph_node.h"

#define PORT_NODE_MODEL 0x7001

/* an area as the calculator keeps it: the root the game's code knows, then what drawing it needs */
struct PortArea {
  struct GraphNodeRoot root;
  u16 mesh;          /* its mesh in src/data.bin (0xFFFF: none) */
  u16 fov, near;
  u32 far;
  u16 background;    /* BACKGROUND_* of the skybox, or 0xFFFF and a plain colour */
  u32 fog_color;     /* 0: no fog */
  u16 fog_near, fog_far;
};

extern const LevelScript port_level_script_entry[];
void port_game_init(void);
void port_game_frame(void);

/* the save chip's 512 bytes (src/port/save.c) */
void port_eeprom_read(int offset, u8 *buf, int n);
void port_eeprom_write(int offset, const u8 *buf, int n);

/* drawing (src/port/scene.c, src/port/menu.c) */
void port_draw_area(struct GraphNodeRoot *root);
void port_flush_frame(void);
#endif
