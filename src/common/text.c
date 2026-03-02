#include "text.h"

#include "../st/glyph.h"
#include "../st/term.h"

typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

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
  kCharBitCount = 8,
  kHexNibbleMask = 0x0FU,
  kDecimalRadix = 10,
  kDigitsBufferSize = 16
};

static unsigned char gVt52State = VT52_STATE_IDLE;
static unsigned char gVt52RowCode = 0U;
static unsigned short gVt52SavedCol = 0U;
static unsigned short gVt52SavedRow = 0U;
static unsigned char gVt52Wrap = 1U;
static unsigned char gVt52Reverse = 0U;

static inline void textPutSpaceAt(unsigned short col, unsigned short row) {
  glyph_plot(col, row, glyph_lookup(' '), TERM_TEXT_COLOR);
}

static volatile unsigned char *textRowPtr(unsigned short row) {
  volatile unsigned char *videoPtr = TERM_VIDEO_BASE;
  while (row--) {
    videoPtr += kTextRowBytes;
  }
  return videoPtr;
}

static void textClearRow(unsigned short row) {
  volatile unsigned char *dst = textRowPtr(row);
  for (unsigned short i = 0; i < kTextRowBytes; ++i) {
    dst[i] = 0x00;
  }
}

static void textCopyRow(unsigned short dstRow, unsigned short srcRow) {
  volatile unsigned char *src = textRowPtr(srcRow);
  volatile unsigned char *dst = textRowPtr(dstRow);
  for (unsigned short i = 0; i < kTextRowBytes; ++i) {
    dst[i] = src[i];
  }
}

static void textInsertLineAtCursor(void) {
  const unsigned short row = TERM_CURSOR_ROW;
  for (unsigned short rowIter = (unsigned short)(SCR_HEIGHT_LINES - 1U); rowIter > row;
       --rowIter) {
    textCopyRow(rowIter, (unsigned short)(rowIter - 1U));
  }
  textClearRow(row);
}

static void textDeleteLineAtCursor(void) {
  const unsigned short row = TERM_CURSOR_ROW;
  for (unsigned short rowIter = row; rowIter + 1U < SCR_HEIGHT_LINES; ++rowIter) {
    textCopyRow(rowIter, (unsigned short)(rowIter + 1U));
  }
  textClearRow((unsigned short)(SCR_HEIGHT_LINES - 1U));
}

static void textEraseLine(void) { textClearRow(TERM_CURSOR_ROW); }

static void textEraseToEndOfLine(void) {
  const unsigned short row = TERM_CURSOR_ROW;
  for (unsigned short col = TERM_CURSOR_COL; col < SCR_WIDTH_CHARS; ++col) {
    textPutSpaceAt(col, row);
  }
}

static void textEraseToEndOfScreen(void) {
  const unsigned short startRow = TERM_CURSOR_ROW;
  const unsigned short startCol = TERM_CURSOR_COL;

  for (unsigned short row = startRow; row < SCR_HEIGHT_LINES; ++row) {
    unsigned short col = (row == startRow) ? startCol : 0U;
    for (; col < SCR_WIDTH_CHARS; ++col) {
      textPutSpaceAt(col, row);
    }
  }
}

static void textEraseToStartOfLine(void) {
  const unsigned short row = TERM_CURSOR_ROW;
  for (unsigned short col = 0; col <= TERM_CURSOR_COL; ++col) {
    textPutSpaceAt(col, row);
  }
}

static void textEraseToStartOfScreen(void) {
  const unsigned short endRow = TERM_CURSOR_ROW;
  const unsigned short endCol = TERM_CURSOR_COL;

  for (unsigned short row = 0; row <= endRow; ++row) {
    unsigned short colEnd =
        (row == endRow) ? endCol : (unsigned short)(SCR_WIDTH_CHARS - 1U);
    for (unsigned short col = 0; col <= colEnd; ++col) {
      textPutSpaceAt(col, row);
    }
  }
}

static void textPutcRaw(char character) {
  unsigned short cursorCol = TERM_CURSOR_COL;
  unsigned short cursorRow = TERM_CURSOR_ROW;

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
    const unsigned char *glyph = glyph_lookup(character);
    if (gVt52Reverse) {
      unsigned char inv[SCR_CHAR_H];
      for (unsigned char i = 0; i < SCR_CHAR_H; ++i) {
        inv[i] = (unsigned char)~glyph[i];
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
      cursorCol = (unsigned short)(SCR_WIDTH_CHARS - 1U);
    }
  }

  TERM_CURSOR_COL = cursorCol;
  TERM_CURSOR_ROW = cursorRow;
}

static void textVt52DirectCursor(unsigned char rowCode, unsigned char colCode) {
  unsigned short row = 0;
  unsigned short col = 0;

  if (rowCode >= kVt52CoordBias) {
    row = (unsigned short)(rowCode - kVt52CoordBias);
  }
  if (colCode >= kVt52CoordBias) {
    col = (unsigned short)(colCode - kVt52CoordBias);
  }

  if (row >= SCR_HEIGHT_LINES) {
    row = (unsigned short)(SCR_HEIGHT_LINES - 1U);
  }
  if (col >= SCR_WIDTH_CHARS) {
    col = (unsigned short)(SCR_WIDTH_CHARS - 1U);
  }

  text_set_cursor(col, row);
}

static void textVt52Exec(unsigned char cmd) {
  unsigned short row = TERM_CURSOR_ROW;
  unsigned short col = TERM_CURSOR_COL;

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

void text_set_cursor(unsigned short col, unsigned short row) {
  if (col >= SCR_WIDTH_CHARS) col = 0;
  if (row >= SCR_HEIGHT_LINES) row = 0;
  TERM_CURSOR_COL = col;
  TERM_CURSOR_ROW = row;
}

void text_set_color(unsigned char color) {
  TERM_TEXT_COLOR = (unsigned char)(color & 0x03U);
}

void text_clear(void) {
  gVt52State = VT52_STATE_IDLE;
  gVt52Reverse = 0U;
  text_set_cursor(0, 0);
  for (unsigned short row = 0; row < SCR_HEIGHT_LINES; ++row) {
    for (unsigned short col = 0; col < SCR_WIDTH_CHARS; ++col) {
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
  const unsigned char inputCode = (unsigned char)character;

  if (gVt52State == VT52_STATE_B_VAL) {
    gVt52State = VT52_STATE_IDLE;
    text_set_color((unsigned char)(inputCode & 0x03U));
    return;
  }

  if (gVt52State == VT52_STATE_SKIP_1) {
    gVt52State = VT52_STATE_IDLE;
    return;
  }

  if (gVt52State == VT52_STATE_ESC) {
    gVt52State = VT52_STATE_IDLE;
    if (inputCode == (unsigned char)'Y') {
      gVt52State = VT52_STATE_Y_ROW;
      return;
    }
    if (inputCode == (unsigned char)'b') {
      gVt52State = VT52_STATE_B_VAL;
      return;
    }
    if (inputCode == (unsigned char)'c') {
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
      text_set_cursor((unsigned short)(TERM_CURSOR_COL - 1U), TERM_CURSOR_ROW);
    }
    return;
  }

  if (inputCode == '\t') {
    unsigned char spaces = (unsigned char)(kTabWidth - (TERM_CURSOR_COL & kTabMask));
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

static void textEmitRepeat(char character, unsigned long count, int *written) {
  while (count--) {
    text_putc(character);
    (*written)++;
  }
}

static unsigned long textStrlenLocal(const char *text) {
  unsigned long len = 0;
  while (text[len]) {
    len++;
  }
  return len;
}

static void textEmitSpan(const char *text, unsigned long len, int *written) {
  for (unsigned long i = 0; i < len; ++i) {
    text_putc(text[i]);
  }
  *written += (int)len;
}

static unsigned long u32ToDec(unsigned long value, char *out) {
  static const unsigned long kPow10[] = {
      1000000000U, 100000000U, 10000000U, 1000000U, 100000U,
      10000U,      1000U,      100U,      10U,      1U};
  unsigned long len = 0;
  unsigned char started = 0;

  for (unsigned long i = 0; i < (unsigned long)(sizeof(kPow10) / sizeof(kPow10[0]));
       ++i) {
    unsigned long digit = 0;
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

static unsigned long u32ToHex(unsigned long value, char *out, unsigned char uppercase) {
  unsigned long len = 0;
  unsigned char started = 0;
  const char hexBaseChar = uppercase ? 'A' : 'a';

  for (int shift = kHexShiftStart; shift >= 0; shift -= kNibbleBitCount) {
    unsigned char nibble = (unsigned char)((value >> (unsigned long)shift) & kHexNibbleMask);
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

static unsigned long uptrToHex(unsigned long value, char *out) {
  unsigned long len = 0;
  unsigned char started = 0;

  for (int shift = (int)(sizeof(unsigned long) * kCharBitCount - kNibbleBitCount);
       shift >= 0; shift -= kNibbleBitCount) {
    unsigned char nibble =
        (unsigned char)((value >> (unsigned long)shift) & (unsigned long)kHexNibbleMask);
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

static void textEmitFormattedNumber(const char *prefix, unsigned long prefixLen,
                                    const char *digits, unsigned long digitsLen,
                                    int width, int precision, unsigned char leftAlign,
                                    unsigned char zeroPad, int *written) {
  unsigned long zeroCount = 0;
  unsigned long fieldLen;
  unsigned long padCount;

  if (precision >= 0) {
    unsigned long precisionValue = (unsigned long)precision;
    if (digitsLen < precisionValue) {
      zeroCount = precisionValue - digitsLen;
    }
  }

  fieldLen = prefixLen + zeroCount + digitsLen;
  padCount = 0;
  if (width > 0 && (unsigned long)width > fieldLen) {
    padCount = (unsigned long)width - fieldLen;
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
      unsigned char leftAlign = 0;
      unsigned char plusSign = 0;
      unsigned char spaceSign = 0;
      unsigned char alt = 0;
      unsigned char zeroPad = 0;
      int width = 0;
      int precision = -1;
      unsigned char longArg = 0;
      char spec;
      char digits[kDigitsBufferSize];
      unsigned long digitsLen = 0;
      char prefix[3];
      unsigned long prefixLen = 0;

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
        unsigned long pad = 0;
        if (width > 1) {
          pad = (unsigned long)(width - 1);
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
        unsigned long len;
        unsigned long outLen;
        unsigned long pad = 0;
        if (!strArg) {
          strArg = "(null)";
        }
        len = textStrlenLocal(strArg);
        outLen = len;
        if (precision >= 0 && outLen > (unsigned long)precision) {
          outLen = (unsigned long)precision;
        }
        if (width > 0 && (unsigned long)width > outLen) {
          pad = (unsigned long)width - outLen;
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
        signed long signedValue = longArg ? (signed long)va_arg(argList, long)
                                      : (signed long)va_arg(argList, int);
        unsigned long magnitude = (unsigned long)signedValue;
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
        unsigned long unsignedValue = longArg
                                     ? (unsigned long)va_arg(argList, unsigned long)
                                     : (unsigned long)va_arg(argList, unsigned int);
        if (spec == 'u') {
          digitsLen = u32ToDec(unsignedValue, digits);
        } else {
          digitsLen = u32ToHex(unsignedValue, digits, (unsigned char)(spec == 'X'));
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
        unsigned long pointerValue = (unsigned long)va_arg(argList, void *);
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
