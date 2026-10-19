/* The save chip: the N64 cartridge's 512-byte EEPROM held the four save files (save_file.c reads and writes
 * it whole or in blocks). On the calculator it is the file sm64.sav, with a copy in a Python script,
 * sm64_saves.py: installing apps from the NumWorks website keeps only Python scripts, so the stars come back
 * from it when the file itself is gone. The copy is a comment line "#>sm64.sav:base64" (the way NumPlay and its
 * big games keep theirs). */
#include <string.h>
#include <ultra64.h>
#include "port_game.h"
#include "plat.h"

#define SAVE_NAME "sm64.sav"
#define COPY_NAME "sm64_saves.py"
#define EEPROM_SIZE 512

static u8 sEeprom[EEPROM_SIZE];
static bool sLoaded;

static const char copy_head[] =
  "# Super Mario 64 keeps a copy of your stars here, so that\n"
  "# reinstalling it doesn't erase them. If you delete this\n"
  "# file, the game writes it again.\n";
static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void copy_write(const u8 *d, u32 len) {
  u32 nl = sizeof SAVE_NAME - 1, total = 1 + (sizeof copy_head - 1) + 2 + nl + 1 + (len + 2) / 3 * 4 + 1 + 1;
  plat_remove(COPY_NAME);
  u8 *o = plat_reserve(COPY_NAME, total), *p = o;
  if (!o) return;
  *p++ = 0;   /* (the script's status byte: not imported) */
  memcpy(p, copy_head, sizeof copy_head - 1), p += sizeof copy_head - 1;
  *p++ = '#', *p++ = '>';
  memcpy(p, SAVE_NAME, nl), p += nl;
  *p++ = ':';
  for (u32 i = 0; i < len; i += 3, p += 4) {
    /* (written out: GCC 15.2 for the calculator got a loop over the four wrong, see NumPlay's
     * launcher/src/storage.c) */
    u32 left = len - i, v = (u32) d[i] << 16 | (left > 1 ? d[i + 1] << 8 : 0) | (left > 2 ? d[i + 2] : 0);
    p[0] = (u8) b64[v >> 18 & 63];
    p[1] = (u8) b64[v >> 12 & 63];
    p[2] = left > 1 ? (u8) b64[v >> 6 & 63] : '=';
    p[3] = left > 2 ? (u8) b64[v & 63] : '=';
  }
  *p++ = '\n';
  *p++ = 0;
  plat_reserved(COPY_NAME, (u32) (p - o));
}

static int b64_value(u8 c) {
  for (int i = 0; i < 64; i++)
    if ((u8) b64[i] == c) return i;
  return -1;
}

static bool copy_read(u8 *out, u32 size) {
  u32 n, nl = sizeof SAVE_NAME - 1;
  const u8 *c = plat_load(COPY_NAME, &n);
  if (!c) return false;
  for (u32 i = 0; i + nl + 3 <= n; i++) {
    if (c[i] != '#' || c[i + 1] != '>' || memcmp(c + i + 2, SAVE_NAME, nl) || c[i + 2 + nl] != ':') continue;
    const u8 *l = c + i + 3 + nl;
    u32 m = 0;
    while (l + m < c + n && l[m] != '\n' && l[m]) m++;
    if (!m || m % 4) return false;
    if (m / 4 * 3 - (l[m - 1] == '=') - (l[m - 2] == '=') != size) return false;
    u32 o = 0;
    for (u32 k = 0; k < m; k += 4) {
      int v0 = b64_value(l[k]), v1 = b64_value(l[k + 1]);
      int v2 = l[k + 2] == '=' ? 0 : b64_value(l[k + 2]), v3 = l[k + 3] == '=' ? 0 : b64_value(l[k + 3]);
      if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return false;
      u32 w = (u32) (v0 << 18 | v1 << 12 | v2 << 6 | v3);
      if (o < size) out[o++] = (u8) (w >> 16);
      if (o < size) out[o++] = (u8) (w >> 8);
      if (o < size) out[o++] = (u8) w;
    }
    return true;
  }
  return false;
}

static void load(void) {
  if (sLoaded) return;
  sLoaded = true;
  u32 len;
  const u8 *d = plat_load(SAVE_NAME, &len);
  if (d && len == EEPROM_SIZE) memcpy(sEeprom, d, EEPROM_SIZE);
  else if (copy_read(sEeprom, EEPROM_SIZE)) plat_save(SAVE_NAME, sEeprom, EEPROM_SIZE);   /* back where it belongs */
  else memset(sEeprom, 0, EEPROM_SIZE);   /* (the game sees bad checksums and starts the files afresh) */
}

void port_eeprom_read(int offset, u8 *buf, int n) {
  load();
  if (offset < 0 || offset + n > EEPROM_SIZE) return;
  memcpy(buf, sEeprom + offset, (size_t) n);
}

void port_eeprom_write(int offset, const u8 *buf, int n) {
  load();
  if (offset < 0 || offset + n > EEPROM_SIZE) return;
  if (!memcmp(sEeprom + offset, buf, (size_t) n)) return;
  memcpy(sEeprom + offset, buf, (size_t) n);
  plat_save(SAVE_NAME, sEeprom, EEPROM_SIZE);
  copy_write(sEeprom, EEPROM_SIZE);
}
