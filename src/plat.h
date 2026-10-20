/* Super Mario 64: the platform (the calculator, or a computer for tests).
 *
 * plat_eadk.c: the calculator's EADK display, keyboard, time and files.
 * plat_host.c: a frame buffer that tests save as images, scripted keys and
 * simulated time (tests/play.c). */
#ifndef SM_PLAT_H
#define SM_PLAT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240

/* keys held now, as bits: the N64 controller's buttons on the calculator's keyboard */
enum {
  K_LEFT = 1 << 0, K_RIGHT = 1 << 1, K_UP = 1 << 2, K_DOWN = 1 << 3,   /* arrows: the control stick */
  K_A = 1 << 4,       /* OK, EXE: A (jump); confirm */
  K_B = 1 << 5,       /* Back: B (punch, kick, dive, pick up); back in menus */
  K_Z = 1 << 6,       /* shift: Z (crouch, ground pound) */
  K_WALK = 1 << 7,    /* alpha, held: the stick only half way (walk, tiptoe) */
  K_CL = 1 << 8,      /* 4, x,n,t: C-left (the camera around to the left) */
  K_CR = 1 << 9,      /* 6, var: C-right */
  K_CU = 1 << 10,     /* 8, toolbox: C-up (the camera closer, then looking around) */
  K_CD = 1 << 11,     /* 2, ln: C-down (the camera further) */
  K_START = 1 << 12,  /* backspace: Start (pause) */
  K_HOME = 1 << 13,   /* Home, On/Off: save and quit */
  K_R = 1 << 14,      /* 0, e^x: R (Mario's camera or Lakitu's) */
  K_ANY = 1 << 15
};
uint32_t plat_keys(void);
uint32_t plat_millis(void);
void plat_sleep(uint32_t ms);
void plat_push(int x, int y, int w, int h, const uint16_t *px);
/* waits for the screen's next refresh to start (false: there is none to wait for) */
bool plat_vblank(void);
bool plat_save(const char *name, const void *data, uint32_t len);
const uint8_t *plat_load(const char *name, uint32_t *len);   /* NULL if absent */
/* a file of len bytes to write in place (the calculator's storage is RAM): plat_reserve gives its
 * bytes, plat_reserved says they are written; NULL if there is no room */
uint8_t *plat_reserve(const char *name, uint32_t len);
void plat_reserved(const char *name, uint32_t len);
bool plat_remove(const char *name);
uint32_t plat_random(void);
void plat_begin(void);
int plat_end(void);
/* an N0110 or N0115 (216 MHz, the N0120 has 550) */
bool plat_slow(void);
/* the RAM after the game's own: what the calculator's software has left for the app */
void *plat_heap(uint32_t *len);
#endif
