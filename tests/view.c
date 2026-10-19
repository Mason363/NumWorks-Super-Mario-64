/* Draws one mesh from a camera, to an image: view MESH EYE_X,Y,Z AT_X,Y,Z OUT.ppm [half] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/render.h"

void host_shot(const char *path);

int main(int argc, char **argv) {
  if (argc < 5) return 2;
  int mesh = atoi(argv[1]);
  Vec3f eye, at;
  sscanf(argv[2], "%f,%f,%f", &eye[0], &eye[1], &eye[2]);
  sscanf(argv[3], "%f,%f,%f", &at[0], &at[1], &at[2]);
  if (argc > 5 && !strcmp(argv[5], "half")) r_w = 160, r_h = 120, r_scale = 2;
  r_frame(eye, at, 0, 45, 100, 30000);
  r_sky(0);
  r_mesh(MESH(mesh), NULL, 0xFFFFFF, 255);
  r_draw();
  printf("%d items, %d triangles\n", r_stat_items, r_stat_tris);
  host_shot(argv[4]);
  return 0;
}
