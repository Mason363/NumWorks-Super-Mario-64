/* Plays Super Mario 64 on a computer, with scripted keys, and saves frames.
 *
 *   play [--frames N] [--keys "10-40:right,50:a,..."] [--shots 30,60|every:N] [--out DIR] [--saves DIR]
 *        [--level N] [--half] [--stats]
 *
 * Keys: left right up down a b z walk cl cr cu cd start r home. "a-b:key" holds the key from frame a to b,
 * "a:key" for one frame (frames are the game's, 30 a second). --level starts the save file in that level
 * (the game's level numbers: 9 is Bob-omb Battlefield) instead of the castle grounds. Screenshots are
 * DIR/shot_<frame>.ppm. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../src/plat.h"
#include "../src/render.h"

extern uint32_t host_keys, host_time;
void host_shot(const char *path);
void host_save_dir(const char *d);
void port_game_init(void);
void port_game_frame(void);
extern int port_start_level_num;
void port_debug_print(void);

typedef struct { int a, b; uint32_t k; } Hold;
static Hold holds[4096];
static int nholds;

static uint32_t key_of(const char *s) {
  static const struct { const char *n; uint32_t k; } names[] = {
    {"left", K_LEFT}, {"right", K_RIGHT}, {"up", K_UP}, {"down", K_DOWN}, {"a", K_A}, {"b", K_B}, {"z", K_Z},
    {"walk", K_WALK}, {"cl", K_CL}, {"cr", K_CR}, {"cu", K_CU}, {"cd", K_CD}, {"start", K_START}, {"r", K_R},
    {"home", K_HOME},
  };
  for (unsigned i = 0; i < sizeof names / sizeof names[0]; i++)
    if (!strcmp(s, names[i].n)) return names[i].k;
  fprintf(stderr, "play: unknown key %s\n", s);
  exit(2);
}

static void parse_keys(const char *spec) {
  char buf[65536];
  snprintf(buf, sizeof buf, "%s", spec);
  for (char *t = strtok(buf, ","); t && nholds < 4096; t = strtok(NULL, ",")) {
    char *colon = strchr(t, ':');
    if (!colon) continue;
    *colon = 0;
    int a = atoi(t), b = a + 1;
    char *dash = strchr(t, '-');
    if (dash) b = atoi(dash + 1) + 1;
    holds[nholds++] = (Hold){a, b, key_of(colon + 1)};
  }
}

int main(int argc, char **argv) {
  int frames = 300, every = 0;
  const char *out = "build/shots", *shots = "";
  bool stats = false;
  for (int i = 1; i < argc; i++) {
    const char *a = argv[i], *v = i + 1 < argc ? argv[i + 1] : "";
    if (!strcmp(a, "--frames")) frames = atoi(v), i++;
    else if (!strcmp(a, "--keys")) parse_keys(v), i++;
    else if (!strcmp(a, "--shots")) {
      shots = v, i++;
      if (!strncmp(shots, "every:", 6)) every = atoi(shots + 6);
    } else if (!strcmp(a, "--out")) out = v, i++;
    else if (!strcmp(a, "--saves")) host_save_dir(v), i++;
    else if (!strcmp(a, "--level")) port_start_level_num = atoi(v), i++;
    else if (!strcmp(a, "--half")) r_w = 160, r_h = 120, r_scale = 2;
    else if (!strcmp(a, "--stats")) stats = true;
  }
  mkdir(out, 0755);
  port_game_init();
  for (int f = 0; f < frames; f++) {
    host_keys = 0;
    for (int i = 0; i < nholds; i++)
      if (f >= holds[i].a && f < holds[i].b) host_keys |= holds[i].k;
    if (host_keys & K_HOME) break;
    port_game_frame();
    host_time += 33;
    bool shot = every ? f % every == 0 : false;
    if (!every) {
      char want[16];
      snprintf(want, sizeof want, "%d", f);
      for (const char *p = shots; *p;) {
        size_t n = strcspn(p, ",");
        if (n == strlen(want) && !strncmp(p, want, n)) shot = true;
        p += n + (p[n] == ',');
      }
    }
    if (shot) {
      char p[512];
      snprintf(p, sizeof p, "%s/shot_%05d.ppm", out, f);
      host_shot(p);
    }
    if (stats && f % 30 == 0) {
      printf("frame %d: %d items, %d triangles, %d vertices; ", f, r_stat_items, r_stat_tris, r_stat_verts);
      port_debug_print();
    }
  }
  return 0;
}
