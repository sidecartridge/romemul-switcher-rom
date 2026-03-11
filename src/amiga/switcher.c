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
#include "../common/text.h"
#include "mem.h"
#include "screen.h"

enum { kPolledInterruptLevel = 7U, kColorDefault = 1U };

extern unsigned char _end;

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

#if defined(_DEBUG) && (_DEBUG > 0)
  chooser_loop(ROM_BASE_ADDR_UL, screen_trace_msg, kColorDefault,
               "Amiga 500/2000");
#else
  chooser_loop(ROM_BASE_ADDR_UL, (helper_trace_fn_t)0, kColorDefault,
               "Amiga 500/2000");
#endif
}
