#include "glyph.h"

#include "../common/font8x8.h"
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

const unsigned char *glyph_lookup(char character) {
  return font8x8_basic[(unsigned char)character];
}

static inline unsigned long glyphRowBaseBytes(unsigned short row) {
  const unsigned long rowValue = (unsigned long)row;
  /* row * 1280 = row * (1024 + 256) */
  return (rowValue << kRowShift1024) + (rowValue << kRowShift256);
}

static inline void glyphPlotMedium(unsigned short col, unsigned short row,
                                   const unsigned char *rows, unsigned char colorIdx) {
  const unsigned char plane0Mask =
      (unsigned char)((colorIdx & kColorPlane0Bit) ? kPlaneMaskSet : kPlaneMaskClear);
  const unsigned char plane1Mask =
      (unsigned char)((colorIdx & kColorPlane1Bit) ? kPlaneMaskSet : kPlaneMaskClear);
  const unsigned long base = glyphRowBaseBytes(row);
  const unsigned long group = ((unsigned long)(col & kEvenColumnMask))
                         << 1;                            /* (col >> 1) * 4 */
  const unsigned long lane = (unsigned long)(col & kOddColumnMask); /* even=0, odd=1 */
  volatile unsigned char *plane0Ptr = TERM_VIDEO_BASE + base + group + lane;
  volatile unsigned char *plane1Ptr = plane0Ptr + kPlane1Offset;
  unsigned char bits;

  bits = rows[0];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[1];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[2];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[3];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[4];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[kGlyphRow5];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[kGlyphRow6];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
  plane0Ptr += SCR_LINE_BYTES_MED;
  plane1Ptr += SCR_LINE_BYTES_MED;

  bits = rows[kGlyphRow7];
  *plane0Ptr = (unsigned char)(bits & plane0Mask);
  *plane1Ptr = (unsigned char)(bits & plane1Mask);
}

static inline void glyphPlotMono(unsigned short col, unsigned short row,
                                 const unsigned char *rows, unsigned char colorIdx) {
  const unsigned char plane0Mask =
      (unsigned char)((colorIdx & kMonoColorMask) ? kPlaneMaskSet : kPlaneMaskClear);
  const unsigned long base = glyphRowBaseBytes(row);
  volatile unsigned char *monoPtr = TERM_VIDEO_BASE + base + (unsigned long)col;

  for (unsigned char rowIdx = 0; rowIdx < kGlyphRowCount; ++rowIdx) {
    const unsigned char bits = (unsigned char)(rows[rowIdx] & plane0Mask);
    monoPtr[0] = bits;
    monoPtr[SCR_LINE_BYTES_HI] = bits;
    monoPtr += (SCR_LINE_BYTES_HI * kDoubleScanlineFactor);
  }
}

void glyph_plot(unsigned short col, unsigned short row, const unsigned char *rows,
                unsigned char color) {
  const unsigned char medium = screen_is_medium_mode();
  const unsigned char colorIdx =
      (unsigned char)(medium ? (color & kMediumColorMask) : (color & kMonoColorMask));
  if (medium) {
    glyphPlotMedium(col, row, rows, colorIdx);
  } else {
    glyphPlotMono(col, row, rows, colorIdx);
  }
}
