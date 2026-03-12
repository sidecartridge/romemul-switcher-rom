/**
 * File: src/amiga/switcher.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga ROM switcher entry point.
 */

#include "../common/chooser.h"
#include "../common/kbd.h"
#include "../common/palloc.h"
#include "../common/rom_check.h"
#include "../common/text.h"
#include "mem.h"
#include "screen.h"

enum {
  kPolledInterruptLevel = 7U,
  kColorDefault = 1U,
  kColorSuccess = 2U,
  kColorFailure = 3U,
  kCrcStatusRow = 0U,
  kCrcResultRow = 1U,
  kCrcDetailsRow = 2U,
  kCrcPromptRow = 4U,
  kCrcSpinnerCol = 27U
};

extern unsigned char _end;
static unsigned char gCrcSpinnerIndex = 0U;

#if defined(_DEBUG) && (_DEBUG > 0)
static void screen_trace_msg(const char *message) {
  if (message != (const char *)0) {
    text_printf("%s", message);
  }
}
#endif

static void setInterruptLevelMask(unsigned short maskLevel) {
  unsigned short statusRegister;
  __asm__ volatile("move.w %%sr,%0" : "=d"(statusRegister));
  statusRegister =
      (unsigned short)((statusRegister & (unsigned short)~0x0700U) |
                       (unsigned short)((maskLevel & 0x7U) << 8U));
  __asm__ volatile("move.w %0,%%sr" : : "d"(statusRegister) : "cc");
}

static unsigned long computeHeapBase(void) {
  const unsigned long ramBase = AMIGA_RAM_VARS_BASE_ADDR;
  const unsigned long ramEnd =
      AMIGA_RAM_VARS_BASE_ADDR + AMIGA_RAM_VARS_SIZE_BYTES;
  unsigned long heapBase =
      AMIGA_RAM_VARS_BASE_ADDR + AMIGA_RAM_VARS_RESERVED_BYTES;
  const unsigned long imageEnd = (unsigned long)&_end;

  if (imageEnd > heapBase && imageEnd < ramEnd) {
    heapBase = (imageEnd + 3UL) & ~3UL;
  }
  if (heapBase > ramEnd) {
    heapBase = ramEnd;
  }
  if (heapBase < ramBase) {
    heapBase = ramBase;
  }

  return heapBase;
}

static void show_check_progress(unsigned long processed_bytes,
                                unsigned long total_bytes) {
  static const char kSpinnerChars[] = {'|', '/', '-', '\\'};
  (void)processed_bytes;
  (void)total_bytes;

  text_set_cursor(kCrcSpinnerCol, kCrcStatusRow);
  text_printf("%c", kSpinnerChars[gCrcSpinnerIndex & 0x03U]);
  gCrcSpinnerIndex = (unsigned char)((gCrcSpinnerIndex + 1U) & 0x03U);
}

static void verify_rom_check_before_menu(void) {
  rom_check_result_t check_result;

  gCrcSpinnerIndex = 0U;
  text_clear();
  text_set_color(kColorDefault);
  text_set_cursor(0U, kCrcStatusRow);
  text_printf("Checking ROM checksum...");

  rom_check_verify((const volatile unsigned char *)ROM_BASE_ADDR_UL,
                   AMIGA_ROM_SIZE_BYTES, AMIGA_ROM_CRC_FIELD_OFFSET_BYTES,
                   AMIGA_ROM_CHECKSUM_FIELD_OFFSET_BYTES,
                   AMIGA_ROM_CHECKSUM_FIELD_SIZE_BYTES, show_check_progress,
                   &check_result);

  text_set_cursor(0U, kCrcResultRow);
  text_set_color(check_result.matches ? kColorSuccess : kColorFailure);
  text_printf("ROM checksum %s", check_result.matches ? "OK  " : "FAIL");
  text_set_color(kColorDefault);

  if (!check_result.matches) {
    text_set_cursor(0U, kCrcDetailsRow);
    text_printf("Stored:   %08lX", check_result.stored_value);
    text_set_cursor(0U, (unsigned short)(kCrcDetailsRow + 1U));
    text_printf("Computed: %08lX", check_result.computed_value);
    text_set_cursor(0U, kCrcPromptRow);
    text_printf("Press any key to continue...");
    kbd_wait_for_key_press();
  }

  text_set_color(kColorDefault);
  text_set_cursor(0U, (unsigned short)(kCrcPromptRow + 2U));
}

void rom_switcher_main(void) {
  const unsigned long heapBase = computeHeapBase();
  const unsigned long ramEnd =
      AMIGA_RAM_VARS_BASE_ADDR + AMIGA_RAM_VARS_SIZE_BYTES;
  const unsigned long heapSize =
      (heapBase < ramEnd) ? (ramEnd - heapBase) : 0UL;
  pa_init(heapBase, heapSize);

  setInterruptLevelMask(kPolledInterruptLevel);

  screen_init();
  text_init();

  text_clear();
  text_set_color(kColorDefault);
  verify_rom_check_before_menu();

#if defined(_DEBUG) && (_DEBUG > 0)
  chooser_loop(ROM_BASE_ADDR_UL, screen_trace_msg, kColorDefault,
               "Amiga 500/2000");
#else
  chooser_loop(ROM_BASE_ADDR_UL, (helper_trace_fn_t)0, kColorDefault,
               "Amiga 500/2000");
#endif
}
