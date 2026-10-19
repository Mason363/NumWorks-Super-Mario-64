/* Vectors, matrices and angles, as the game computes them (src/engine/math_util.c in the decompilation).
 * Matrices act on row vectors: a point is x * m[0] + y * m[1] + z * m[2] + m[3]. */
#include "smath.h"

/* the game's arctangent table had 1025 steps from 0 to 45 degrees: the ratio is rounded to them */
static uint16_t atan_lookup(float y, float x) {
  if (x == 0) return 0;
  int i = (int)(y / x * 1024 + 0.5f);
  return (uint16_t)(atanf((float)i / 1024.0f) * (32768.0f / M_PI_F) + 0.5f);
}

int16_t atan2s(float y, float x) {
  uint16_t r;
  if (x >= 0) {
    if (y >= 0) r = y >= x ? atan_lookup(x, y) : (uint16_t)(0x4000 - atan_lookup(y, x));
    else {
      y = -y;
      r = y < x ? (uint16_t)(0x4000 + atan_lookup(y, x)) : (uint16_t)(0x8000 - atan_lookup(x, y));
    }
  } else {
    x = -x;
    if (y < 0) {
      y = -y;
      r = y >= x ? (uint16_t)(0x8000 + atan_lookup(x, y)) : (uint16_t)(0xC000 - atan_lookup(y, x));
    } else r = y < x ? (uint16_t)(0xC000 + atan_lookup(y, x)) : (uint16_t)(-atan_lookup(x, y));
  }
  return (int16_t)r;
}

float atan2f_game(float y, float x) { return (float)atan2s(y, x) * (M_PI_F / 32768.0f); }

void vec3f_set(Vec3f v, float x, float y, float z) { v[0] = x, v[1] = y, v[2] = z; }
void vec3f_copy(Vec3f d, const Vec3f s) { d[0] = s[0], d[1] = s[1], d[2] = s[2]; }
float vec3f_len(const Vec3f v) { return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
void vec3f_normalize(Vec3f v) {
  float l = vec3f_len(v);
  if (l == 0) return;
  l = 1 / l;
  v[0] *= l, v[1] *= l, v[2] *= l;
}
void vec3f_cross(Vec3f d, const Vec3f a, const Vec3f b) {
  float x = a[1] * b[2] - b[1] * a[2], y = a[2] * b[0] - b[2] * a[0], z = a[0] * b[1] - b[0] * a[1];
  d[0] = x, d[1] = y, d[2] = z;
}

void mtx_identity(Mtx43 *m) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 3; j++) m->m[i][j] = i == j;
}

void mtx_rotate_zxy_translate(Mtx43 *d, const Vec3f t, const Vec3s r) {
  float sx = sins(r[0]), cx = coss(r[0]), sy = sins(r[1]), cy = coss(r[1]), sz = sins(r[2]), cz = coss(r[2]);
  d->m[0][0] = cy * cz + sx * sy * sz;
  d->m[1][0] = -cy * sz + sx * sy * cz;
  d->m[2][0] = cx * sy;
  d->m[3][0] = t[0];
  d->m[0][1] = cx * sz;
  d->m[1][1] = cx * cz;
  d->m[2][1] = -sx;
  d->m[3][1] = t[1];
  d->m[0][2] = -sy * cz + sx * cy * sz;
  d->m[1][2] = sy * sz + sx * cy * cz;
  d->m[2][2] = cx * cy;
  d->m[3][2] = t[2];
}

void mtx_rotate_xyz_translate(Mtx43 *d, const Vec3f t, const Vec3s r) {
  float sx = sins(r[0]), cx = coss(r[0]), sy = sins(r[1]), cy = coss(r[1]), sz = sins(r[2]), cz = coss(r[2]);
  d->m[0][0] = cy * cz;
  d->m[0][1] = cy * sz;
  d->m[0][2] = -sy;
  d->m[1][0] = sx * sy * cz - cx * sz;
  d->m[1][1] = sx * sy * sz + cx * cz;
  d->m[1][2] = sx * cy;
  d->m[2][0] = cx * sy * cz + sx * sz;
  d->m[2][1] = cx * sy * sz - sx * cz;
  d->m[2][2] = cx * cy;
  d->m[3][0] = t[0], d->m[3][1] = t[1], d->m[3][2] = t[2];
}

void mtx_mul(Mtx43 *d, const Mtx43 *a, const Mtx43 *b) {
  Mtx43 r;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 3; j++) {
      float v = a->m[i][0] * b->m[0][j] + a->m[i][1] * b->m[1][j] + a->m[i][2] * b->m[2][j];
      r.m[i][j] = i == 3 ? v + b->m[3][j] : v;
    }
  *d = r;
}

void mtx_scale(Mtx43 *m, float s) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) m->m[i][j] *= s;
}

void mtx_apply(const Mtx43 *m, const Vec3f in, Vec3f out) {
  float x = in[0], y = in[1], z = in[2];
  for (int j = 0; j < 3; j++) out[j] = x * m->m[0][j] + y * m->m[1][j] + z * m->m[2][j] + m->m[3][j];
}

void mtx_align_terrain(Mtx43 *m, const Vec3f n, const Vec3f pos, int16_t yaw) {
  Vec3f lat = {sins(yaw), 0, coss(yaw)}, up, left, fwd;
  vec3f_copy(up, n);
  vec3f_normalize(up);
  vec3f_cross(left, up, lat);
  vec3f_normalize(left);
  vec3f_cross(fwd, left, up);
  vec3f_normalize(fwd);
  for (int j = 0; j < 3; j++) m->m[0][j] = left[j], m->m[1][j] = up[j], m->m[2][j] = fwd[j], m->m[3][j] = pos[j];
}
