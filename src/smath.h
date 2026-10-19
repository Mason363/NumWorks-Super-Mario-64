/* Vectors, matrices and the game's angles: an angle is 16 bits, 0x10000 a whole turn, as on the N64. */
#ifndef SM_MATH_H
#define SM_MATH_H
#include <math.h>
#include <stdint.h>

typedef float Vec3f[3];
typedef int16_t Vec3s[3];
/* rows: x axis, y axis, z axis (each 3 floats), then the translation */
typedef struct { float m[4][3]; } Mtx43;

#define M_PI_F 3.14159265f
/* the game's sine and cosine: its table had 4096 steps a turn */
static inline float sins(int32_t a) { return sinf((float)((uint16_t)a >> 4) * (M_PI_F / 2048.0f)); }
static inline float coss(int32_t a) { return cosf((float)((uint16_t)a >> 4) * (M_PI_F / 2048.0f)); }
int16_t atan2s(float y, float x);   /* the game's: atan2s(y, x) is the angle of (x, y) */
float atan2f_game(float y, float x);

static inline float sqr(float x) { return x * x; }
static inline float approach_f(float cur, float target, float inc, float dec) {
  if (cur < target) { cur += inc; if (cur > target) cur = target; }
  else { cur -= dec; if (cur < target) cur = target; }
  return cur;
}
static inline int32_t approach_i(int32_t cur, int32_t target, int32_t inc, int32_t dec) {
  if (cur < target) { cur += inc; if (cur > target) cur = target; }
  else { cur -= dec; if (cur < target) cur = target; }
  return cur;
}

void vec3f_set(Vec3f v, float x, float y, float z);
void vec3f_copy(Vec3f d, const Vec3f s);
float vec3f_len(const Vec3f v);
void vec3f_normalize(Vec3f v);
void vec3f_cross(Vec3f d, const Vec3f a, const Vec3f b);

void mtx_identity(Mtx43 *m);
/* rotation by (pitch x, yaw y, roll z) in the game's order (z, then x, then y), then translation t */
void mtx_rotate_zxy_translate(Mtx43 *m, const Vec3f t, const Vec3s r);
/* the game's rotate_xyz (for animated parts: x, then y, then z) */
void mtx_rotate_xyz_translate(Mtx43 *m, const Vec3f t, const Vec3s r);
/* d = a * b (b applied first) */
void mtx_mul(Mtx43 *d, const Mtx43 *a, const Mtx43 *b);
void mtx_scale(Mtx43 *m, float s);
void mtx_apply(const Mtx43 *m, const Vec3f in, Vec3f out);
/* a matrix that stands an object on a floor with normal n, facing yaw */
void mtx_align_terrain(Mtx43 *m, const Vec3f n, const Vec3f pos, int16_t yaw);
#endif
