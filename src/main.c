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

/* frames, and the time they take (the calculator's emulator reports them) */
uint32_t perf_frames, perf_max, perf_slow;

int main(void) {
  plat_begin();
  /* the N0110 and N0115 draw at half resolution (2.5 times slower than the N0120) */
  if (plat_slow()) r_w = 160, r_h = 120, r_scale = 2;
  port_game_init();
  uint32_t next = plat_millis();
  for (;;) {
    if ((plat_keys() & K_HOME) || port_quit) break;
    uint32_t start = plat_millis();
    port_game_frame();
    /* the game's 30 frames a second */
    next += 33;
    uint32_t now = plat_millis();
    perf_frames++;
    if (now - start > perf_max) perf_max = now - start;
    if (now - start > 33) perf_slow++;
    if ((int32_t)(next - now) > 0) plat_sleep(next - now);
    else next = now;
  }
  return plat_end();
}
