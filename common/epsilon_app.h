/* Staying safe as an external app on the NumWorks calculator (Epsilon).
 *
 * Home. Epsilon normally leaves an app the moment Home is pressed, then
 * reuses the app's RAM for its own memory pools. Whether a pool has to be
 * rebuilt is decided by a flag stored in that same RAM, so an app that used
 * it can leave the flag set over a broken pool: the calculator crashes soon
 * after, resets, and loses its files (saves included). np_app_begin() asks
 * Epsilon to hold Home back while the app runs, so the app sees Home as a key
 * and quits by itself, and np_app_end() clears the app's RAM before handing it
 * back. Both do nothing when the game runs inside the NumPlay launcher, which
 * does the same for all its games.
 *
 * Files. The file system is found through the userland header of the
 * software that runs this app: at the start of the userland of either
 * firmware slot, 64 KiB later on models with an extra data sector. Only
 * addresses in external flash are probed, which exist on every model.
 *
 * Upsilon. The custom software of the N0110 and N0115 runs .nwa apps too. Its
 * header has the same first words but no device name (the footer comes two
 * words sooner, then Omega's magic), and it has none of the system calls for
 * Home (10, 13), checksums (15, 16) or flash: those are never made there.
 *
 * Models. The N0110 and N0115 (216 MHz) keep their userland right after the
 * kernel and their RAM at 0x20000000; the N0120 (550 MHz) has an extra data
 * sector before the userland and its RAM at 0x24000000.
 *
 * Include it in one file per app. */
#ifndef EPSILON_APP_H
#define EPSILON_APP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t magic;
  char version[8];
  uint32_t storage_ram, storage_size;
  uint32_t apps_flash_start, apps_flash_end;
  uint32_t apps_ram_start, apps_ram_end;
  uint32_t name_flash_start, name_flash_end; /* (not on Upsilon: the footer, then Omega's magic) */
  uint32_t footer;
} epsilon_userland_t;

#if PLATFORM_DEVICE && !defined(HOST)
#include <eadk.h>

/* The header of the software running this app, or NULL; *upsilon tells which. */
__attribute__((unused)) static const epsilon_userland_t *epsilon_userland_of(bool *upsilon) {
  static const uint32_t where[] = {0x90010000u, 0x90020000u, 0x90410000u, 0x90420000u};
  uint32_t self = (uint32_t)(uintptr_t)&epsilon_userland_of;
  const epsilon_userland_t *found = NULL;
  bool ups = false;
  for (unsigned i = 0; i < sizeof where / sizeof where[0]; i++) {
    const volatile epsilon_userland_t *h = (const volatile epsilon_userland_t *)(uintptr_t)where[i];
    const volatile uint32_t *w = (const volatile uint32_t *)(uintptr_t)where[i];
    if (h->magic != 0xDEC0EDFEu) continue;
    bool official = h->footer == 0xDEC0EDFEu, omega = w[9] == 0xDEC0EDFEu && w[10] == 0xEFBEADDEu;
    if (!official && !omega) continue;
    if (self < h->apps_flash_start || self >= h->apps_flash_end) continue;
    if (found) return NULL; /* ambiguous: better not guess */
    found = (const epsilon_userland_t *)h;
    ups = omega;
  }
  if (upsilon) *upsilon = found && ups;
  return found;
}
__attribute__((unused)) static const epsilon_userland_t *epsilon_userland(void) { return epsilon_userland_of(NULL); }

/* Upsilon (or Omega) runs this app: none of Epsilon's extra system calls. */
__attribute__((unused)) static bool epsilon_is_upsilon(void) {
  bool u = false;
  epsilon_userland_of(&u);
  return u;
}

/* An N0110 or N0115: 2.5 times slower than the N0120. */
__attribute__((unused)) static bool epsilon_slow_model(void) {
  const epsilon_userland_t *h = epsilon_userland();
  return h ? h->storage_ram < 0x24000000u : false;
}

/* The record area of Epsilon's file system (after its magic word), or NULL. */
__attribute__((unused)) static uint8_t *epsilon_storage(uint32_t *size) {
  const epsilon_userland_t *h = epsilon_userland();
  if (!h) return NULL;
  uint32_t ram = h->storage_ram, n = h->storage_size;
  bool in_ram = (ram >= 0x20000000u && ram + n + 8 <= 0x20040000u) || (ram >= 0x24000000u && ram + n + 8 <= 0x24040000u);
  if (!in_ram || (ram & 3) || n < 1024 || n > 0x10000) return NULL;
  const volatile uint8_t *fs = (const volatile uint8_t *)(uintptr_t)ram;
  uint32_t head = fs[0] | fs[1] << 8 | fs[2] << 16 | (uint32_t)fs[3] << 24;
  const volatile uint8_t *f = fs + 4 + n;
  uint32_t foot = f[0] | f[1] << 8 | f[2] << 16 | (uint32_t)f[3] << 24;
  if (head != 0xEE0BDDBAu || foot != 0xEE0BDDBAu) return NULL;
  *size = n;
  return (uint8_t *)(uintptr_t)ram + 4;
}

/* Bounds of this app's RAM, from the installer's linker script. */
extern char _data_section_start_ram[], _heap_end[];
/* Defined by the NumPlay launcher only. */
extern const uint8_t numplay_launcher[] __attribute__((weak));
static bool epsilon_home_held;

/* Epsilon's circuit breaker: while locked, Home cannot interrupt the app
 * (system calls 10 and 13, unchanged since 2021). */
__attribute__((unused)) static void np_app_begin(void) {
  bool upsilon = false;
  if (numplay_launcher || !epsilon_userland_of(&upsilon) || upsilon) return;
  __asm__ volatile("svc 10" ::: "r0", "r1", "r2", "r3", "r12", "memory");
  epsilon_home_held = true;
}

/* Call as `return np_app_end();` from main: nothing in RAM is used after.
 * A key still held as the app ends (OK or EXE on a Quit button) would reach the home screen, which opens the
 * app again at once: the keys are let go first (for two seconds at most). */
__attribute__((unused, noinline)) static int np_app_end(void) {
  if (numplay_launcher) return 0;
  for (int i = 0; i < 200 && eadk_keyboard_scan(); i++) eadk_timing_msleep(10);
  bool unlock = epsilon_home_held;
  for (volatile uint32_t *p = (volatile uint32_t *)(void *)_data_section_start_ram; p < (volatile uint32_t *)(void *)_heap_end; p++)
    *p = 0;
  /* a Home press held back so far may take effect now: the RAM is clean */
  if (unlock) __asm__ volatile("svc 13" ::: "r0", "r1", "r2", "r3", "r12", "memory");
  return 0;
}

#else /* simulator, host tools */
__attribute__((unused)) static uint8_t *epsilon_storage(uint32_t *size) {
  (void)size;
  return NULL;
}
static inline bool epsilon_is_upsilon(void) { return false; }
static inline bool epsilon_slow_model(void) { return false; }
static inline void np_app_begin(void) {}
static inline int np_app_end(void) { return 0; }
#endif

#endif
