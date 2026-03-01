#include "text.h"

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

#include "../st/glyph.h"
#include "../st/term.h"

enum {
  VT52_STATE_IDLE = 0U,
  VT52_STATE_ESC = 1U,
  VT52_STATE_Y_ROW = 2U,
  VT52_STATE_Y_COL = 3U,
  VT52_STATE_B_VAL = 4U,
  VT52_STATE_SKIP_1 = 5U
};

enum {
  kTextRowBytes = 1280U,
  kVt52CoordBias = 32U,
  kEscCode = 0x1BU,
  kTabWidth = 8U,
  kTabMask = 7U,
  kHexShiftStart = 28,
  kNibbleBitCount = 4,
  kHexNibbleMask = 0x0FU,
  kDecimalRadix = 10,
  kDigitsBufferSize = 16
};

static uint8_t gVt52State = VT52_STATE_IDLE;
static uint8_t gVt52RowCode = 0U;
static uint16_t gVt52SavedCol = 0U;
static uint16_t gVt52SavedRow = 0U;
static uint8_t gVt52Wrap = 1U;
static uint8_t gVt52Reverse = 0U;

static inline void textPutSpaceAt(uint16_t col, uint16_t row) {
  glyph_plot(col, row, glyph_lookup(' '), TERM_TEXT_COLOR);
}

static volatile uint8_t *textRowPtr(uint16_t row) {
  volatile uint8_t *videoPtr = TERM_VIDEO_BASE;
  while (row--) {
    videoPtr += kTextRowBytes;
  }
  return videoPtr;
}

static void textClearRow(uint16_t row) {
  volatile uint8_t *dst = textRowPtr(row);
  for (uint16_t i = 0; i < kTextRowBytes; ++i) {
    dst[i] = 0x00;
  }
}

static void textCopyRow(uint16_t dstRow, uint16_t srcRow) {
  volatile uint8_t *src = textRowPtr(srcRow);
  volatile uint8_t *dst = textRowPtr(dstRow);
  for (uint16_t i = 0; i < kTextRowBytes; ++i) {
    dst[i] = src[i];
  }
}

static void textInsertLineAtCursor(void) {
  const uint16_t row = TERM_CURSOR_ROW;
  for (uint16_t rowIter = (uint16_t)(SCR_HEIGHT_LINES - 1U); rowIter > row;
       --rowIter) {
    textCopyRow(rowIter, (uint16_t)(rowIter - 1U));
  }
  textClearRow(row);
}

static void textDeleteLineAtCursor(void) {
  const uint16_t row = TERM_CURSOR_ROW;
  for (uint16_t rowIter = row; rowIter + 1U < SCR_HEIGHT_LINES; ++rowIter) {
    textCopyRow(rowIter, (uint16_t)(rowIter + 1U));
  }
  textClearRow((uint16_t)(SCR_HEIGHT_LINES - 1U));
}

static void textEraseLine(void) { textClearRow(TERM_CURSOR_ROW); }

static void textEraseToEndOfLine(void) {
  const uint16_t row = TERM_CURSOR_ROW;
  for (uint16_t col = TERM_CURSOR_COL; col < SCR_WIDTH_CHARS; ++col) {
    textPutSpaceAt(col, row);
  }
}

static void textEraseToEndOfScreen(void) {
  const uint16_t startRow = TERM_CURSOR_ROW;
  const uint16_t startCol = TERM_CURSOR_COL;

  for (uint16_t row = startRow; row < SCR_HEIGHT_LINES; ++row) {
    uint16_t col = (row == startRow) ? startCol : 0U;
    for (; col < SCR_WIDTH_CHARS; ++col) {
      textPutSpaceAt(col, row);
    }
  }
}

static void textEraseToStartOfLine(void) {
  const uint16_t row = TERM_CURSOR_ROW;
  for (uint16_t col = 0; col <= TERM_CURSOR_COL; ++col) {
    textPutSpaceAt(col, row);
  }
}

static void textEraseToStartOfScreen(void) {
  const uint16_t endRow = TERM_CURSOR_ROW;
  const uint16_t endCol = TERM_CURSOR_COL;

  for (uint16_t row = 0; row <= endRow; ++row) {
    uint16_t colEnd =
        (row == endRow) ? endCol : (uint16_t)(SCR_WIDTH_CHARS - 1U);
    for (uint16_t col = 0; col <= colEnd; ++col) {
      textPutSpaceAt(col, row);
    }
  }
}

static void textPutcRaw(char character) {
  uint16_t cursorCol = TERM_CURSOR_COL;
  uint16_t cursorRow = TERM_CURSOR_ROW;

  if (character == '\n') {
    cursorCol = 0;
    cursorRow++;
    if (cursorRow >= SCR_HEIGHT_LINES) {
      cursorRow = 0;
    }
    TERM_CURSOR_COL = cursorCol;
    TERM_CURSOR_ROW = cursorRow;
    return;
  }

  {
    const uint8_t *glyph = glyph_lookup(character);
    if (gVt52Reverse) {
      uint8_t inv[SCR_CHAR_H];
      for (uint8_t i = 0; i < SCR_CHAR_H; ++i) {
        inv[i] = (uint8_t)~glyph[i];
      }
      glyph_plot(cursorCol, cursorRow, inv, TERM_TEXT_COLOR);
    } else {
      glyph_plot(cursorCol, cursorRow, glyph, TERM_TEXT_COLOR);
    }
  }

  cursorCol++;
  if (cursorCol >= SCR_WIDTH_CHARS) {
    if (gVt52Wrap) {
      cursorCol = 0;
      cursorRow++;
      if (cursorRow >= SCR_HEIGHT_LINES) {
        cursorRow = 0;
      }
    } else {
      cursorCol = (uint16_t)(SCR_WIDTH_CHARS - 1U);
    }
  }

  TERM_CURSOR_COL = cursorCol;
  TERM_CURSOR_ROW = cursorRow;
}

static void textVt52DirectCursor(uint8_t rowCode, uint8_t colCode) {
  uint16_t row = 0;
  uint16_t col = 0;

  if (rowCode >= kVt52CoordBias) {
    row = (uint16_t)(rowCode - kVt52CoordBias);
  }
  if (colCode >= kVt52CoordBias) {
    col = (uint16_t)(colCode - kVt52CoordBias);
  }

  if (row >= SCR_HEIGHT_LINES) {
    row = (uint16_t)(SCR_HEIGHT_LINES - 1U);
  }
  if (col >= SCR_WIDTH_CHARS) {
    col = (uint16_t)(SCR_WIDTH_CHARS - 1U);
  }

  text_set_cursor(col, row);
}

static void textVt52Exec(uint8_t cmd) {
  uint16_t row = TERM_CURSOR_ROW;
  uint16_t col = TERM_CURSOR_COL;

  switch (cmd) {
    case 'A': /* cursor up */
      if (row > 0U) {
        row--;
      }
      text_set_cursor(col, row);
      break;
    case 'B': /* cursor down */
      if (row + 1U < SCR_HEIGHT_LINES) {
        row++;
      }
      text_set_cursor(col, row);
      break;
    case 'C': /* cursor right */
      if (col + 1U < SCR_WIDTH_CHARS) {
        col++;
      }
      text_set_cursor(col, row);
      break;
    case 'D': /* cursor left */
      if (col > 0U) {
        col--;
      }
      text_set_cursor(col, row);
      break;
    case 'E': /* clear screen + home */
      text_clear();
      break;
    case 'H': /* home */
      text_set_cursor(0U, 0U);
      break;
    case 'I': /* reverse line feed (basic: move up) */
      if (row > 0U) {
        row--;
      }
      text_set_cursor(col, row);
      break;
    case 'J': /* erase to end of screen */
      textEraseToEndOfScreen();
      break;
    case 'K': /* erase to end of line */
      textEraseToEndOfLine();
      break;
    case 'L': /* insert line at cursor */
      textInsertLineAtCursor();
      break;
    case 'M': /* delete line at cursor */
      textDeleteLineAtCursor();
      break;
    case 'd': /* erase from top to cursor */
      textEraseToStartOfScreen();
      break;
    case 'l': /* erase full current line */
      textEraseLine();
      break;
    case 'o': /* erase from line start to cursor */
      textEraseToStartOfLine();
      break;
    case 'j': /* save cursor */
      gVt52SavedCol = TERM_CURSOR_COL;
      gVt52SavedRow = TERM_CURSOR_ROW;
      break;
    case 'k': /* restore cursor */
      text_set_cursor(gVt52SavedCol, gVt52SavedRow);
      break;
    case 'p': /* reverse video on */
      gVt52Reverse = 1U;
      break;
    case 'q': /* reverse video off */
      gVt52Reverse = 0U;
      break;
    case 'v': /* wrap on */
      gVt52Wrap = 1U;
      break;
    case 'w': /* wrap off */
      gVt52Wrap = 0U;
      break;
    default:
      break;
  }
}

void text_set_cursor(uint16_t col, uint16_t row) {
  if (col >= SCR_WIDTH_CHARS) col = 0;
  if (row >= SCR_HEIGHT_LINES) row = 0;
  TERM_CURSOR_COL = col;
  TERM_CURSOR_ROW = row;
}

void text_set_color(uint8_t color) {
  TERM_TEXT_COLOR = (uint8_t)(color & 0x03U);
}

void text_clear(void) {
  gVt52State = VT52_STATE_IDLE;
  gVt52Reverse = 0U;
  text_set_cursor(0, 0);
  for (uint16_t row = 0; row < SCR_HEIGHT_LINES; ++row) {
    for (uint16_t col = 0; col < SCR_WIDTH_CHARS; ++col) {
      textPutcRaw(' ');
    }
  }
  text_set_cursor(0, 0);
}

void text_init(void) {
  screen_clear();
  text_set_color(1U);
}

void text_putc(char character) {
  const uint8_t inputCode = (uint8_t)character;

  if (gVt52State == VT52_STATE_B_VAL) {
    gVt52State = VT52_STATE_IDLE;
    text_set_color((uint8_t)(inputCode & 0x03U));
    return;
  }

  if (gVt52State == VT52_STATE_SKIP_1) {
    gVt52State = VT52_STATE_IDLE;
    return;
  }

  if (gVt52State == VT52_STATE_ESC) {
    gVt52State = VT52_STATE_IDLE;
    if (inputCode == (uint8_t)'Y') {
      gVt52State = VT52_STATE_Y_ROW;
      return;
    }
    if (inputCode == (uint8_t)'b') {
      gVt52State = VT52_STATE_B_VAL;
      return;
    }
    if (inputCode == (uint8_t)'c') {
      /* Not implemented by request: consume one parameter byte. */
      gVt52State = VT52_STATE_SKIP_1;
      return;
    }
    textVt52Exec(inputCode);
    return;
  }

  if (gVt52State == VT52_STATE_Y_ROW) {
    gVt52RowCode = inputCode;
    gVt52State = VT52_STATE_Y_COL;
    return;
  }

  if (gVt52State == VT52_STATE_Y_COL) {
    textVt52DirectCursor(gVt52RowCode, inputCode);
    gVt52State = VT52_STATE_IDLE;
    return;
  }

  if (inputCode == kEscCode) {
    gVt52State = VT52_STATE_ESC;
    return;
  }

  if (inputCode == '\r') {
    text_set_cursor(0U, TERM_CURSOR_ROW);
    return;
  }

  if (inputCode == '\b') {
    if (TERM_CURSOR_COL > 0U) {
      text_set_cursor((uint16_t)(TERM_CURSOR_COL - 1U), TERM_CURSOR_ROW);
    }
    return;
  }

  if (inputCode == '\t') {
    uint8_t spaces = (uint8_t)(kTabWidth - (TERM_CURSOR_COL & kTabMask));
    while (spaces--) {
      textPutcRaw(' ');
    }
    return;
  }

  textPutcRaw(character);
}

void text_puts(const char *text) {
  while (*text) {
    text_putc(*text++);
  }
}

static void textEmitRepeat(char character, uint32_t count, int *written) {
  while (count--) {
    text_putc(character);
    (*written)++;
  }
}

static uint32_t textStrlenLocal(const char *text) {
  uint32_t len = 0;
  while (text[len]) {
    len++;
  }
  return len;
}

static void textEmitSpan(const char *text, uint32_t len, int *written) {
  for (uint32_t i = 0; i < len; ++i) {
    text_putc(text[i]);
  }
  *written += (int)len;
}

static uint32_t u32ToDec(uint32_t value, char *out) {
  static const uint32_t kPow10[] = {
      1000000000U, 100000000U, 10000000U, 1000000U, 100000U,
      10000U,      1000U,      100U,      10U,      1U};
  uint32_t len = 0;
  uint8_t started = 0;

  for (uint32_t i = 0; i < (uint32_t)(sizeof(kPow10) / sizeof(kPow10[0]));
       ++i) {
    uint32_t digit = 0;
    while (value >= kPow10[i]) {
      value -= kPow10[i];
      digit++;
    }
    if (started || digit || (kPow10[i] == 1U)) {
      out[len++] = (char)('0' + digit);
      started = 1;
    }
  }

  return len;
}

static uint32_t u32ToHex(uint32_t value, char *out, uint8_t uppercase) {
  uint32_t len = 0;
  uint8_t started = 0;
  const char hexBaseChar = uppercase ? 'A' : 'a';

  for (int shift = kHexShiftStart; shift >= 0; shift -= kNibbleBitCount) {
    uint8_t nibble = (uint8_t)((value >> (uint32_t)shift) & kHexNibbleMask);
    if (started || nibble || shift == 0) {
      if (nibble < kDecimalRadix) {
        out[len++] = (char)('0' + (char)nibble);
      } else {
        out[len++] = (char)(hexBaseChar + (char)(nibble - kDecimalRadix));
      }
      started = 1;
    }
  }

  return len;
}

static uint32_t uptrToHex(uintptr_t value, char *out) {
  uint32_t len = 0;
  uint8_t started = 0;

  for (int shift = (int)(sizeof(uintptr_t) * CHAR_BIT - kNibbleBitCount);
       shift >= 0; shift -= kNibbleBitCount) {
    uint8_t nibble =
        (uint8_t)((value >> (uint32_t)shift) & (uintptr_t)kHexNibbleMask);
    if (started || nibble || shift == 0) {
      if (nibble < kDecimalRadix) {
        out[len++] = (char)('0' + (char)nibble);
      } else {
        out[len++] = (char)('a' + (char)(nibble - kDecimalRadix));
      }
      started = 1;
    }
  }

  return len;
}

static void textEmitFormattedNumber(const char *prefix, uint32_t prefixLen,
                                    const char *digits, uint32_t digitsLen,
                                    int width, int precision, uint8_t leftAlign,
                                    uint8_t zeroPad, int *written) {
  uint32_t zeroCount = 0;
  uint32_t fieldLen;
  uint32_t padCount;

  if (precision >= 0) {
    uint32_t precisionValue = (uint32_t)precision;
    if (digitsLen < precisionValue) {
      zeroCount = precisionValue - digitsLen;
    }
  }

  fieldLen = prefixLen + zeroCount + digitsLen;
  padCount = 0;
  if (width > 0 && (uint32_t)width > fieldLen) {
    padCount = (uint32_t)width - fieldLen;
  }

  if (!leftAlign) {
    if (zeroPad && precision < 0) {
      textEmitSpan(prefix, prefixLen, written);
      textEmitRepeat('0', padCount, written);
      textEmitRepeat('0', zeroCount, written);
      textEmitSpan(digits, digitsLen, written);
      return;
    }
    textEmitRepeat(' ', padCount, written);
  }

  textEmitSpan(prefix, prefixLen, written);
  textEmitRepeat('0', zeroCount, written);
  textEmitSpan(digits, digitsLen, written);

  if (leftAlign) {
    textEmitRepeat(' ', padCount, written);
  }
}

int text_printf(const char *fmt, ...) {
  va_list argList;
  int written = 0;

  va_start(argList, fmt);

  while (*fmt) {
    if (*fmt != '%') {
      text_putc(*fmt++);
      written++;
      continue;
    }

    fmt++; /* skip '%' */
    if (*fmt == '%') {
      text_putc('%');
      written++;
      fmt++;
      continue;
    }

    {
      uint8_t leftAlign = 0;
      uint8_t plusSign = 0;
      uint8_t spaceSign = 0;
      uint8_t alt = 0;
      uint8_t zeroPad = 0;
      int width = 0;
      int precision = -1;
      uint8_t longArg = 0;
      char spec;
      char digits[kDigitsBufferSize];
      uint32_t digitsLen = 0;
      char prefix[3];
      uint32_t prefixLen = 0;

      /* flags */
      while (*fmt == '-' || *fmt == '+' || *fmt == ' ' || *fmt == '#' ||
             *fmt == '0') {
        if (*fmt == '-') leftAlign = 1;
        if (*fmt == '+') plusSign = 1;
        if (*fmt == ' ') spaceSign = 1;
        if (*fmt == '#') alt = 1;
        if (*fmt == '0') zeroPad = 1;
        fmt++;
      }

      /* width */
      while (*fmt >= '0' && *fmt <= '9') {
        width = width * kDecimalRadix + (*fmt - '0');
        fmt++;
      }

      /* precision */
      if (*fmt == '.') {
        precision = 0;
        fmt++;
        while (*fmt >= '0' && *fmt <= '9') {
          precision = precision * kDecimalRadix + (*fmt - '0');
          fmt++;
        }
        zeroPad = 0;
      }

      /* length */
      if (*fmt == 'l') {
        longArg = 1;
        fmt++;
        if (*fmt == 'l') {
          fmt++; /* treat ll as l in this 32-bit freestanding build */
        }
      } else if (*fmt == 'h') {
        fmt++;
        if (*fmt == 'h') {
          fmt++;
        }
      }

      spec = *fmt ? *fmt++ : '\0';

      if (spec == 'c') {
        char outputChar = (char)va_arg(argList, int);
        uint32_t pad = 0;
        if (width > 1) {
          pad = (uint32_t)(width - 1);
        }
        if (!leftAlign) {
          textEmitRepeat(' ', pad, &written);
        }
        text_putc(outputChar);
        written++;
        if (leftAlign) {
          textEmitRepeat(' ', pad, &written);
        }
        continue;
      }

      if (spec == 's') {
        const char *strArg = va_arg(argList, const char *);
        uint32_t len;
        uint32_t outLen;
        uint32_t pad = 0;
        if (!strArg) {
          strArg = "(null)";
        }
        len = textStrlenLocal(strArg);
        outLen = len;
        if (precision >= 0 && outLen > (uint32_t)precision) {
          outLen = (uint32_t)precision;
        }
        if (width > 0 && (uint32_t)width > outLen) {
          pad = (uint32_t)width - outLen;
        }
        if (!leftAlign) {
          textEmitRepeat(' ', pad, &written);
        }
        textEmitSpan(strArg, outLen, &written);
        if (leftAlign) {
          textEmitRepeat(' ', pad, &written);
        }
        continue;
      }

      if (spec == 'd' || spec == 'i') {
        int32_t signedValue = longArg ? (int32_t)va_arg(argList, long)
                                      : (int32_t)va_arg(argList, int);
        uint32_t magnitude = (uint32_t)signedValue;
        if (signedValue < 0) {
          prefix[prefixLen++] = '-';
          magnitude = (~magnitude) + 1U;
        } else if (plusSign) {
          prefix[prefixLen++] = '+';
        } else if (spaceSign) {
          prefix[prefixLen++] = ' ';
        }
        digitsLen = u32ToDec(magnitude, digits);
        if (precision == 0 && magnitude == 0U) {
          digitsLen = 0;
        }
        textEmitFormattedNumber(prefix, prefixLen, digits, digitsLen, width,
                                precision, leftAlign, zeroPad, &written);
        continue;
      }

      if (spec == 'u' || spec == 'x' || spec == 'X') {
        uint32_t unsignedValue = longArg
                                     ? (uint32_t)va_arg(argList, unsigned long)
                                     : (uint32_t)va_arg(argList, unsigned int);
        if (spec == 'u') {
          digitsLen = u32ToDec(unsignedValue, digits);
        } else {
          digitsLen = u32ToHex(unsignedValue, digits, (uint8_t)(spec == 'X'));
          if (alt && unsignedValue != 0U) {
            prefix[prefixLen++] = '0';
            prefix[prefixLen++] = (char)((spec == 'X') ? 'X' : 'x');
          }
        }
        if (precision == 0 && unsignedValue == 0U) {
          digitsLen = 0;
        }
        textEmitFormattedNumber(prefix, prefixLen, digits, digitsLen, width,
                                precision, leftAlign, zeroPad, &written);
        continue;
      }

      if (spec == 'p') {
        uintptr_t pointerValue = (uintptr_t)va_arg(argList, void *);
        prefix[prefixLen++] = '0';
        prefix[prefixLen++] = 'x';
        digitsLen = uptrToHex(pointerValue, digits);
        textEmitFormattedNumber(prefix, prefixLen, digits, digitsLen, width,
                                precision, leftAlign, zeroPad, &written);
        continue;
      }

      /* Unknown specifier: print it literally to aid debugging. */
      text_putc('%');
      written++;
      if (spec) {
        text_putc(spec);
        written++;
      }
    }
  }

  va_end(argList);
  return written;
}
