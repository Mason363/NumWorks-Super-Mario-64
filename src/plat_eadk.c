/* The calculator: EADK display, keyboard, time and files. */
#include <eadk.h>
#include "../common/epsilon_app.h"
#include "../common/epsilon_files.h"
#include "plat.h"

#define KEY(k) ((uint64_t)1 << (k))

uint32_t plat_keys(void) {
  uint64_t k = eadk_keyboard_scan();
  uint32_t r = 0;
  if (k & KEY(eadk_key_left)) r |= K_LEFT;
  if (k & KEY(eadk_key_right)) r |= K_RIGHT;
  if (k & KEY(eadk_key_up)) r |= K_UP;
  if (k & KEY(eadk_key_down)) r |= K_DOWN;
  if (k & (KEY(eadk_key_ok) | KEY(eadk_key_exe))) r |= K_A;
  if (k & KEY(eadk_key_back)) r |= K_B;
  if (k & KEY(eadk_key_shift)) r |= K_Z;
  if (k & KEY(eadk_key_alpha)) r |= K_WALK;
  if (k & (KEY(eadk_key_four) | KEY(eadk_key_xnt))) r |= K_CL;
  if (k & (KEY(eadk_key_six) | KEY(eadk_key_var))) r |= K_CR;
  if (k & (KEY(eadk_key_eight) | KEY(eadk_key_toolbox))) r |= K_CU;
  if (k & (KEY(eadk_key_two) | KEY(eadk_key_ln))) r |= K_CD;
  if (k & KEY(eadk_key_backspace)) r |= K_START;
  if (k & (KEY(eadk_key_zero) | KEY(eadk_key_exp))) r |= K_R;
  if (k & (KEY(eadk_key_home) | KEY(eadk_key_on_off))) r |= K_HOME;
  if (k) r |= K_ANY;
  return r;
}

uint32_t plat_millis(void) { return (uint32_t)eadk_timing_millis(); }
void plat_sleep(uint32_t ms) { eadk_timing_msleep(ms); }
void plat_push(int x, int y, int w, int h, const uint16_t *px) {
  eadk_display_push_rect((eadk_rect_t){(uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h}, px);
}
bool plat_save(const char *name, const void *data, uint32_t len) { return ef_write(name, data, len); }
const uint8_t *plat_load(const char *name, uint32_t *len) { return ef_read(name, len); }
uint8_t *plat_reserve(const char *name, uint32_t len) {
  uint32_t n;
  return ef_write(name, NULL, len) ? (uint8_t *)(uintptr_t)ef_read(name, &n) : NULL;   /* (the storage is RAM) */
}
void plat_reserved(const char *name, uint32_t len) { (void)name, (void)len; }
bool plat_remove(const char *name) { return ef_remove(name); }
uint32_t plat_random(void) { return eadk_random(); }
void plat_begin(void) { np_app_begin(); }
int plat_end(void) { return np_app_end(); }
bool plat_slow(void) { return epsilon_slow_model(); }
void *plat_heap(uint32_t *len) {
  extern char _heap_start[], _heap_end[];
  *len = (uint32_t)(_heap_end - _heap_start);
  return _heap_start;
}
