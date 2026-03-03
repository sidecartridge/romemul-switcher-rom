#include "switcher.h"

#include "../common/chooser.h"
#include "../common/palloc.h"
#include "../common/text.h"
#include "kbd.h"
#if defined(_DEBUG) && (_DEBUG > 0)
#include "htrace.h"
#endif
#include "screen.h"

enum {
  kStatusRegisterIplMask = 0x0700U,
  kIplLevelMask = 0x7U,
  kIplShiftBits = 8U,
  kPolledInterruptLevel = 7U,
  kColorDefault = 1U
};

extern unsigned char _end;

static void setInterruptLevelMask(unsigned short maskLevel) {
  unsigned short statusRegister;
  __asm__ volatile("move.w %%sr,%0" : "=d"(statusRegister));
  statusRegister = (unsigned short)((statusRegister &
                                     (unsigned short)~kStatusRegisterIplMask) |
                                    (unsigned short)((maskLevel & kIplLevelMask)
                                                     << kIplShiftBits));
  __asm__ volatile("move.w %0,%%sr" ::"d"(statusRegister) : "cc");
}

static unsigned long computeHeapBase(void) {
  const unsigned long ramBase = ST_RAM_VARS_BASE_ADDR;
  const unsigned long ramEnd = ST_RAM_VARS_BASE_ADDR + ST_RAM_VARS_SIZE_BYTES;
  unsigned long heapBase =
      ST_RAM_VARS_BASE_ADDR + (unsigned long)ST_RAM_VARS_RESERVED_BYTES;
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
  const unsigned long ramEnd = ST_RAM_VARS_BASE_ADDR + ST_RAM_VARS_SIZE_BYTES;
  const unsigned long heapSize =
      (heapBase < ramEnd) ? (ramEnd - heapBase) : 0UL;
  pa_init(heapBase, heapSize);

  /* Polled runtime: mask all IRQ levels (IPL=7). */
  setInterruptLevelMask(kPolledInterruptLevel);

#if defined(_DEBUG) && (_DEBUG > 0)
  hatari_trace_init();
  hatari_trace_msg("[romswdbg] boot\n");
#endif

  screen_init();

  screen_set_display_palette();
  text_init();

  text_clear();
  text_set_color(kColorDefault);
  // (void)text_run_feature_tests();
  // kbd_wait_for_key_press();
  // text_clear();
  // text_set_color(kColorDefault);

#if defined(_DEBUG) && (_DEBUG > 0)
  chooser_loop(ROM_BASE_ADDR_UL, hatari_trace_msg, kColorDefault, "Atari ST");
#else
  chooser_loop(ROM_BASE_ADDR_UL, (helper_trace_fn_t)0, kColorDefault,
               "Atari ST");
#endif
}
