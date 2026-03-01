#include "rom_switcher.h"

#include "../common/romemul_hw.h"
#include "../common/text.h"
#include "kbd.h"
#include "screen.h"

#ifndef APP_VERSION_STR
#define APP_VERSION_STR "0.0.0"
#endif

enum {
  kInvalidScancode = 0xFFU,
  kKeyReleaseMask = 0x80U,
  kPollDelayLoops = 2500U,
  kStatusRegisterIplMask = 0x0700U,
  kIplLevelMask = 0x7U,
  kIplShiftBits = 8U,
  kTitleWidthChars = 80U,
  kTitleBufferSize = 81U,
  kPolledInterruptLevel = 7U,
  kScancodeKey1 = 0x02U,
  kScancodeKey2 = 0x03U,
  kScancodeKey3 = 0x04U,
  kScancodeKey4 = 0x05U,
  kColorDefault = 1U,
  kColorActive = 2U,
  kColorPrompt = 3U
};

/*
static const uint16_t kForegroundColors[] = {
    COLOR_LIGHT,
    COLOR_DARK
};
*/

static void delay(uint32_t loops) {
  while (loops--) {
    __asm__ volatile("nop\n nop\n" ::: "memory");
  }
}

/*
static void apply_bank(uint8_t bank)
{
    romemul_select_bank(bank);
    romemul_wait_ready();
}
*/

static uint8_t mapScancode(uint8_t code) {
  switch (code) {
    case kScancodeKey1:
      return 0;  // '1'
    case kScancodeKey2:
      return 1;  // '2'
    case kScancodeKey3:
      return 2;  // '3'
    case kScancodeKey4:
      return 3;  // '4'
    default:
      return kInvalidScancode;
  }
}

static void setInterruptLevelMask(uint16_t maskLevel) {
  uint16_t statusRegister;
  __asm__ volatile("move.w %%sr,%0" : "=d"(statusRegister));
  statusRegister =
      (uint16_t)((statusRegister & (uint16_t)~kStatusRegisterIplMask) |
                 (uint16_t)((maskLevel & kIplLevelMask) << kIplShiftBits));
  __asm__ volatile("move.w %0,%%sr" ::"d"(statusRegister) : "cc");
}

static void buildTitleLine(char out[kTitleBufferSize]) {
  static const char kTitleText[] =
      "SidecarTridge Atari ST Rescue Switcher - v" APP_VERSION_STR;
  uint16_t len = 0;
  uint16_t left = 0;

  while (kTitleText[len] && len < kTitleWidthChars) {
    len++;
  }

  if (len < kTitleWidthChars) {
    left = (uint16_t)((kTitleWidthChars - len) / 2U);
  }

  for (uint16_t i = 0; i < kTitleWidthChars; ++i) {
    out[i] = ' ';
  }

  for (uint16_t i = 0; i < len; ++i) {
    out[left + i] = kTitleText[i];
  }

  out[kTitleWidthChars] = '\0';
}

static void printMenu(uint8_t activeBank) {
  const uint8_t medium = screen_is_medium_mode();
  char titleLine[kTitleBufferSize];

  text_clear();
  text_set_color((uint8_t)(medium ? kColorDefault : kColorDefault));
  buildTitleLine(titleLine);

  /* VT-52: cursor to row 0 col 0, reverse on, print 80-char centered title,
   * reverse off. */
  text_printf("\x1BY  \x1Bp%s\x1Bq", titleLine);

  /* VT-52: move to row 2 col 0 before rendering menu entries. */
  text_printf("\x1BY\" ");

  for (uint8_t i = 0; i < 4; ++i) {
    if (medium && i == activeBank) {
      text_set_color(kColorActive);
    } else {
      text_set_color(kColorDefault);
    }
    text_puts("[");
    text_putc('1' + i);
    text_puts("] BANK ");
    text_putc('0' + i);
    if (i == activeBank) {
      text_puts("  <ACTIVE>");
    }
    text_putc('\n');
  }
  text_putc('\n');
  text_set_color((uint8_t)(medium ? kColorPrompt : kColorDefault));
  text_puts("PRESS 1-4 TO SELECT ROM BANK\n");
}

void rom_switcher_main(void) {
  uint8_t currentBank = 0;

  /* Polled runtime: mask all IRQ levels (IPL=7). */
  setInterruptLevelMask(kPolledInterruptLevel);

  screen_init();
  screen_set_display_palette();
  text_init();
  // apply_bank(currentBank);
  printMenu(currentBank);

  for (;;) {
    uint8_t scan = kbd_poll_scancode();
    if (scan == kInvalidScancode) {
      delay(kPollDelayLoops);
      continue;
    }

    if (scan & kKeyReleaseMask) {
      // ignore key releases
      continue;
    }

    uint8_t mapped = mapScancode(scan);
    if (mapped != kInvalidScancode && mapped != currentBank) {
      currentBank = mapped;
      // apply_bank(currentBank);
      printMenu(currentBank);
    }
  }
}
