/**
 * File: src/amiga/glyph.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga bitmap glyph rendering implementation.
 */

#include "../common/font8x8.h"
#include "../common/glyph.h"
#include "../common/text.h"
#include "../common/term.h"
#include "mem.h"

const unsigned char *glyph_lookup(char character) {
  return font8x8_basic[(unsigned char)character];
}

void glyph_plot(unsigned short col, unsigned short row, const unsigned char *rows,
                unsigned char color) {
  const unsigned long glyph_offset =
      ((unsigned long)row * AMIGA_TEXT_ROW_BYTES) + (unsigned long)col;
  volatile unsigned char *plane0 = TERM_VIDEO_BASE + glyph_offset;
  volatile unsigned char *plane1 =
      TERM_VIDEO_BASE + (unsigned long)AMIGA_SCREEN_PLANE_SIZE_BYTES + glyph_offset;
  const unsigned char plane0_mask =
      (unsigned char)((color & 0x01U) ? 0xFFU : 0x00U);
  const unsigned char plane1_mask =
      (unsigned char)((color & 0x02U) ? 0xFFU : 0x00U);

  for (unsigned char i = 0U; i < SCR_CHAR_H; ++i) {
    const unsigned char bits = rows[i];
    plane0[0] = (unsigned char)(bits & plane0_mask);
    plane1[0] = (unsigned char)(bits & plane1_mask);
    plane0 += AMIGA_SCREEN_ROW_BYTES;
    plane1 += AMIGA_SCREEN_ROW_BYTES;
  }
}
