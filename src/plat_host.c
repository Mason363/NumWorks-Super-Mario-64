/* A computer: the screen goes to a frame buffer that tests save as images,
 * keys come from a script, time is simulated. */
#include <stdio.h>
#include <stdlib.h>
#include "plat.h"

uint16_t host_fb[SCREEN_W * SCREEN_H];
uint32_t host_time;               /* ms */
uint32_t host_keys;
uint32_t host_seed = 12345;

uint32_t plat_keys(void) { return host_keys; }
uint32_t plat_millis(void) { return host_time; }
void plat_sleep(uint32_t ms) { host_time += ms; }
void plat_push(int x, int y, int w, int h, const uint16_t *px) {
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++)
      if ((unsigned)(x + c) < SCREEN_W && (unsigned)(y + r) < SCREEN_H) host_fb[(y + r) * SCREEN_W + x + c] = px[r * w + c];
}

static char save_dir[256] = "build/host-saves";
void host_save_dir(const char *d) { snprintf(save_dir, sizeof save_dir, "%s", d); }
bool plat_save(const char *name, const void *data, uint32_t len) {
  char p[512];
  snprintf(p, sizeof p, "%s/%s", save_dir, name);
  FILE *f = fopen(p, "wb");
  if (!f) return false;
  bool ok = fwrite(data, 1, len, f) == len;
  fclose(f);
  return ok;
}
/* (each file read gets its own buffer: callers may hold two at once) */
static uint8_t loaded[4][1 << 16];
static int next_load;
const uint8_t *plat_load(const char *name, uint32_t *len) {
  char p[512];
  snprintf(p, sizeof p, "%s/%s", save_dir, name);
  FILE *f = fopen(p, "rb");
  if (!f) return NULL;
  uint8_t *b = loaded[next_load++ & 3];
  *len = (uint32_t)fread(b, 1, sizeof loaded[0], f);
  fclose(f);
  return b;
}
static uint8_t reserved[1 << 16];
bool host_storage_full;           /* tests: no room for any file */
uint8_t *plat_reserve(const char *name, uint32_t len) {
  (void)name;
  return len <= sizeof reserved && !host_storage_full ? reserved : NULL;
}
void plat_reserved(const char *name, uint32_t len) { plat_save(name, reserved, len); }
bool plat_remove(const char *name) {
  char p[512];
  snprintf(p, sizeof p, "%s/%s", save_dir, name);
  return remove(p) == 0;
}
uint32_t plat_random(void) {
  host_seed ^= host_seed << 13;
  host_seed ^= host_seed >> 17;
  host_seed ^= host_seed << 5;
  return host_seed;
}
void plat_begin(void) {}
int plat_end(void) { return 0; }
bool plat_slow(void) { return false; }

/* writes the screen as a PPM image */
void host_shot(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) return;
  fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
  for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
    uint16_t c = host_fb[i];
    uint8_t rgb[3] = {(uint8_t)((c >> 11) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63), (uint8_t)((c & 31) * 255 / 31)};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}
