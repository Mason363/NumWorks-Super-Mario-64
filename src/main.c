/* Super Mario 64 on the NumWorks calculator: the decompilation's game (decomp/, src/port) at 30 frames a
 * second, as on the N64, drawn by the calculator's renderer (src/render.c). */
#include "plat.h"
#include "render.h"

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Super Mario 64";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

void port_game_init(void);
void port_game_frame(void);
/* the player quit from the pause menu (src/port/menu.c) */
extern int port_quit;

/* frames drawn, the game's ticks, and the time the drawn ones take (the calculator's emulator reports them) */
uint32_t perf_frames, perf_ticks, perf_max, perf_slow;
/* 0: this tick is not drawn (src/port/scene.c) */
extern int port_draw;

/* the game thinks 30 times a second whatever the drawing costs: when a frame took too long, the next ticks
 * are not drawn (up to MAX_SKIP in a row) until the game is on time again */
#define MAX_SKIP 3

int main(void) {
  plat_begin();
  /* the N0110 and N0115 draw at half resolution (2.5 times slower than the N0120) */
  if (plat_slow()) r_w = 160, r_h = 120, r_scale = 2;
  port_game_init();
  uint32_t t0 = plat_millis(), ticks = 0;
  int skipped = 0;
  for (;;) {
    if ((plat_keys() & K_HOME) || port_quit) break;
    /* tick number ticks is due at t0 + ticks / 30 s */
    uint32_t now = plat_millis(), due = t0 + ticks * 100 / 3;
    int32_t late = (int32_t) (now - due);
    if (late < 0) {
      plat_sleep((uint32_t) -late);
      late = 0;
    } else if (late > 250) {
      /* far behind (a file saved, or frames too slow even when skipped): on time from now */
      t0 = now, ticks = 0, late = 0;
    }
    /* behind time (more than a few ms: the clock's jitter): this tick is not drawn */
    port_draw = late <= 4 || skipped >= MAX_SKIP;
    skipped = port_draw ? 0 : skipped + 1;
    uint32_t start = plat_millis();
    port_game_frame();
    ticks++;
    perf_ticks++;
    if (port_draw) {
      uint32_t took = plat_millis() - start;
      perf_frames++;
      if (took > perf_max) perf_max = took;
      if (took > 33) perf_slow++;
    }
  }
  return plat_end();
}
