/* The N64 library's matrix functions the game's screens use (libultra's gu: their matrices go into display
 * lists, src/port/gfx.c reads them back). */
#include <math.h>
#include <string.h>
#include <ultra64.h>

void guMtxIdentF(float mf[4][4]) {
  memset(mf, 0, sizeof(float[4][4]));
  mf[0][0] = mf[1][1] = mf[2][2] = mf[3][3] = 1;
}

/* the fixed point layout: the whole parts of the 16 numbers, then their fractions */
void guMtxF2L(float mf[4][4], Mtx *m) {
  s32 *w = (s32 *) m;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 2; c++) {
      s32 a = (s32) (mf[r][2 * c] * 65536.0f), b = (s32) (mf[r][2 * c + 1] * 65536.0f);
      w[r * 2 + c] = (s32) (((u32) a & 0xFFFF0000u) | (((u32) b >> 16) & 0xFFFF));
      w[8 + r * 2 + c] = (s32) (((u32) a << 16) | ((u32) b & 0xFFFF));
    }
}

void guMtxIdent(Mtx *m) {
  float mf[4][4];
  guMtxIdentF(mf);
  guMtxF2L(mf, m);
}

void guTranslate(Mtx *m, float x, float y, float z) {
  float mf[4][4];
  guMtxIdentF(mf);
  mf[3][0] = x, mf[3][1] = y, mf[3][2] = z;
  guMtxF2L(mf, m);
}

void guScale(Mtx *m, float x, float y, float z) {
  float mf[4][4];
  guMtxIdentF(mf);
  mf[0][0] = x, mf[1][1] = y, mf[2][2] = z;
  guMtxF2L(mf, m);
}

void guRotate(Mtx *m, float a, float x, float y, float z) {
  float mf[4][4], l = sqrtf(x * x + y * y + z * z);
  if (l > 0) x /= l, y /= l, z /= l;
  float r = a * (3.14159265f / 180.0f), s = sinf(r), c = cosf(r), t = 1 - c;
  guMtxIdentF(mf);
  mf[0][0] = t * x * x + c, mf[0][1] = t * x * y + s * z, mf[0][2] = t * x * z - s * y;
  mf[1][0] = t * x * y - s * z, mf[1][1] = t * y * y + c, mf[1][2] = t * y * z + s * x;
  mf[2][0] = t * x * z + s * y, mf[2][1] = t * y * z - s * x, mf[2][2] = t * z * z + c;
  guMtxF2L(mf, m);
}

void guOrtho(Mtx *m, float l, float r, float b, float t, float n, float f, float scale) {
  float mf[4][4];
  guMtxIdentF(mf);
  mf[0][0] = 2 / (r - l) * scale, mf[1][1] = 2 / (t - b) * scale, mf[2][2] = -2 / (f - n) * scale;
  mf[3][0] = -(r + l) / (r - l) * scale, mf[3][1] = -(t + b) / (t - b) * scale, mf[3][2] = -(f + n) / (f - n) * scale;
  mf[3][3] = scale;
  guMtxF2L(mf, m);
}

void guPerspective(Mtx *m, u16 *perspNorm, float fovy, float aspect, float near, float far, float scale) {
  float mf[4][4], ct = 1 / tanf(fovy * (3.14159265f / 360.0f));
  guMtxIdentF(mf);
  mf[0][0] = ct / aspect * scale, mf[1][1] = ct * scale, mf[2][2] = (near + far) / (near - far) * scale;
  mf[2][3] = -1 * scale, mf[3][2] = 2 * near * far / (near - far) * scale, mf[3][3] = 0;
  if (perspNorm) *perspNorm = 0xFFFF;
  guMtxF2L(mf, m);
}
