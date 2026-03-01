#include "glyph.h"

#include "../common/font8x8_basic.h"
#include "../common/text.h"
#include "screen.h"
#include "term.h"

enum {
  kRowShift1024 = 10,
  kRowShift256 = 8,
  kColorPlane0Bit = 0x01U,
  kColorPlane1Bit = 0x02U,
  kPlaneMaskSet = 0xFFU,
  kPlaneMaskClear = 0x00U,
  kEvenColumnMask = 0xFFFEU,
  kOddColumnMask = 0x01U,
  kPlane1Offset = 2U,
  kMonoColorMask = 0x01U,
  kMediumColorMask = 0x03U,
  kDoubleScanlineFactor = 2U,
  kGlyphRowCount = 8U,
  kGlyphRow5 = 5U,
  kGlyphRow6 = 6U,
  kGlyphRow7 = 7U
};

const uint8_t *glyph_lookup(char character) {
  return font8x8_basic[(uint8_t)character];
}

static inline uint32_t glyphRowBaseBytes(uint16_t row) {
  const uint32_t rowValue = (uint32_t)row;
  /* row * 1280 = row * (1024 + 256) */
  return (rowValue << kRowShift1024) + (rowValue << kRowShift256);
}

static inline void glyphPlotMedium(uint16_t col, uint16_t row,
                                   const uint8_t *rows, uint8_t colorIdx) {
  const uint8_t plane0Mask =
      (uint8_t)((colorIdx & kColorPlane0Bit) ? kPlaneMaskSet : kPlaneMaskClear);
  const uint8_t plane1Mask =
      (uint8_t)((colorIdx & kColorPlane1Bit) ? kPlaneMaskSet : kPlaneMaskClear);
  const uint32_t base = glyphRowBaseBytes(row);
  const uint32_t group = ((uint32_t)(col & kEvenColumnMask))
                         << 1;                            /* (col >> 1) * 4 */
  const uint32_t lane = (uint32_t)(col & kOddColumnMask); /* even=0, odd=1 */
  volatile uint8_t *plane0Ptr = TERM_VIDEO_BASE + base + group + lane;
  volatile uint8_t *plane1Ptr = plane0Ptr + kPlane1Offset;
  uint8_t bits;

  bits = rows[0];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[1];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[2];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[3];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[4];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[kGlyphRow5];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[kGlyphRow6];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[kGlyphRow7];
  *plane0Ptr = (uint8_t)(bits & plane0Mask);
  *plane1Ptr = (uint8_t)(bits & plane1Mask);
}

static inline void glyphPlotMono(uint16_t col, uint16_t row,
                                 const uint8_t *rows, uint8_t colorIdx) {
  const uint8_t plane0Mask =
      (uint8_t)((colorIdx & kMonoColorMask) ? kPlaneMaskSet : kPlaneMaskClear);
  const uint32_t base = glyphRowBaseBytes(row);
  volatile uint8_t *monoPtr = TERM_VIDEO_BASE + base + (uint32_t)col;

  for (uint8_t rowIdx = 0; rowIdx < kGlyphRowCount; ++rowIdx) {
    const uint8_t bits = (uint8_t)(rows[rowIdx] & plane0Mask);
    monoPtr[0] = bits;
    monoPtr[SCR_LINE_BYTES_HI] = bits;
    monoPtr += (SCR_LINE_BYTES_HI * kDoubleScanlineFactor);
  }
}

void glyph_plot(uint16_t col, uint16_t row, const uint8_t *rows,
                uint8_t color) {
  const uint8_t medium = screen_is_medium_mode();
  const uint8_t colorIdx =
      (uint8_t)(medium ? (color & kMediumColorMask) : (color & kMonoColorMask));
  if (medium) {
    glyphPlotMedium(col, row, rows, colorIdx);
  } else {
    glyphPlotMono(col, row, rows, colorIdx);
  }
}
