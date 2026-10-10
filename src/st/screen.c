/**
 * File: src/st/screen.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST screen setup and framebuffer control.
 */

#include "screen.h"

#include "../common/trace.h"
#include "mem.h"
#include "palette.h"

#define ST_VIDEO_BASE ((volatile unsigned char *)ST_SCREEN_BASE_ADDR)
#define ST_PALETTE_BASE ((volatile unsigned short *)(unsigned long)0x00FF8240UL)
#define ST_MFP_GPIP (*(volatile unsigned char *)(unsigned long)0x00FFFA01UL)
#define ST_SHIFTER_MODE (*(volatile unsigned char *)(unsigned long)0x00FF8260UL)
#define ST_SHIFTER_BASE_HI (*(volatile unsigned char *)(unsigned long)0x00FF8201UL)
#define ST_SHIFTER_BASE_MID (*(volatile unsigned char *)(unsigned long)0x00FF8203UL)
#define ST_VIDEO_COUNT_HI (*(volatile unsigned char *)(unsigned long)0x00FF8205UL)
#define ST_VIDEO_COUNT_MID (*(volatile unsigned char *)(unsigned long)0x00FF8207UL)
#define ST_VIDEO_COUNT_LO (*(volatile unsigned char *)(unsigned long)0x00FF8209UL)
/* STE shifter only: the base's low byte, the line offset, the fine scroll. */
#define ST_STE_BASE_LO_ADDR 0x00FF820DUL
#define ST_STE_BASE_LO (*(volatile unsigned char *)ST_STE_BASE_LO_ADDR)
#define ST_STE_LINE_OFFSET (*(volatile unsigned char *)(unsigned long)0x00FF820FUL)
#define ST_STE_HSCROLL (*(volatile unsigned char *)(unsigned long)0x00FF8265UL)
#define ST_PALETTE_0 (*(volatile unsigned short *)(unsigned long)0x00FF8240UL)
#define ST_VIDEO_MODE \
  (*(volatile unsigned char *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 5U))
#define ST_VIDEO_MODE_VALID \
  (*(volatile unsigned char *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 6U))

enum {
  kShifterModeMedium = 0x01U,
  kShifterModeHi = 0x02U,
  kShifterModeMask = 0x03U,
  kMonitorColorDetectBit = 0x80U,
  kByteShiftHigh = 16,
  kByteShiftMid = 8,
  kByteMask = 0xFFU,
  kVblStableReads = 4U,     /* the counter held at the base: no line displays */
  kVblGuardPolls = 20000UL, /* several frames: never wait forever */
  kSteBaseLoTest = 90U
};

extern int st_probe_read(unsigned long address); /* probe.s */

volatile unsigned char *term_video_base(void) { return ST_VIDEO_BASE; }

volatile unsigned short *term_cursor_col_ptr(void) {
  return (volatile unsigned short *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 0U);
}

volatile unsigned short *term_cursor_row_ptr(void) {
  return (volatile unsigned short *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 2U);
}

volatile unsigned char *term_text_color_ptr(void) {
  return (volatile unsigned char *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 4U);
}

unsigned short term_text_row_bytes(void) { return 1280U; }

void term_screen_clear(void) { screen_clear(); }

/* The video address counter. The shifter reloads it from the base at each
   vertical sync and holds it there until the first displayed line. */
static unsigned long screenVideoCounter(void) {
  return ((unsigned long)ST_VIDEO_COUNT_HI << kByteShiftHigh) |
         ((unsigned long)ST_VIDEO_COUNT_MID << kByteShiftMid) |
         (unsigned long)ST_VIDEO_COUNT_LO;
}

/* Waits for the next vertical blank, with interrupts masked: until the
   counter leaves the base (a line displays), then until it holds at the base
   again for kVblStableReads reads in a row. A read of the three bytes can tear
   as the counter carries and look like the base once, never four times. The
   blank lasts a few milliseconds and a poll of a debug build takes about
   80 us in Hatari, so the reads in a row must stay few. Returns 0 if the
   counter never did that. */
static int screenWaitVbl(unsigned long base) {
  unsigned long guard;
  unsigned short stable = 0U;

  for (guard = 0UL; guard < kVblGuardPolls && screenVideoCounter() == base;
       ++guard) {
  }
  for (guard = 0UL; guard < kVblGuardPolls && stable < kVblStableReads;
       ++guard) {
    stable = (screenVideoCounter() == base) ? (unsigned short)(stable + 1U) : 0U;
  }
  return (stable == kVblStableReads) ? 1 : 0;
}

/* An STE shifter, found as EmuTOS finds it (bios/machine.c, detect_video()):
   the base's low byte must hold what is written to it. Leaves it at 0. */
static int screenHasSteShifter(void) {
  if (!st_probe_read(ST_STE_BASE_LO_ADDR)) {
    return 0;
  }
  ST_STE_BASE_LO = kSteBaseLoTest;
  (void)ST_SHIFTER_BASE_MID;
  if (ST_STE_BASE_LO != kSteBaseLoTest) {
    return 0;
  }
  ST_STE_BASE_LO = 0U;
  (void)ST_PALETTE_0;
  return (ST_STE_BASE_LO == 0U) ? 1 : 0;
}

/* The resolution changes only in the vertical blank, after two vertical syncs,
   as EmuTOS does (bios/screen.c, screen_init_mode()). Changed while a line
   displays, the shifter's planes shift, and after the reset in the startup
   code the Glue may need a second vertical sync to settle: without it "a
   monochrome screen display may wrap". On a real Mega ST in mono that wrap
   put the last 4 bytes of every line at its start. */
void screen_init(void) {
  const unsigned long base = ST_SCREEN_BASE_ADDR;
  const int ste = screenHasSteShifter();
  unsigned char gpip;
  int vbl;

  ST_SHIFTER_BASE_HI = (unsigned char)((base >> kByteShiftHigh) & kByteMask);
  ST_SHIFTER_BASE_MID = (unsigned char)((base >> kByteShiftMid) & kByteMask);

  vbl = screenWaitVbl(base);
  vbl &= screenWaitVbl(base);

  /* GPIP bit 7: 0=mono monitor, 1=color monitor. Read late, once the machine
     has run two frames. */
  gpip = ST_MFP_GPIP;
  if (gpip & kMonitorColorDetectBit) {
    ST_SHIFTER_MODE = kShifterModeMedium; /* 640x200, 4 colours */
  } else {
    ST_SHIFTER_MODE = kShifterModeHi; /* 640x400, monochrome */
  }
  if (ste) {
    /* Whatever a program left there before the reset. */
    ST_STE_LINE_OFFSET = 0U;
    ST_STE_HSCROLL = 0U;
  }
  TRACE("screen %s", (gpip & kMonitorColorDetectBit) ? "medium" : "mono");
  TRACE("screen vbl %s ste=%d", vbl ? "ok" : "timeout", ste);

  /* Force lazy mode cache refresh on first query. */
  ST_VIDEO_MODE_VALID = 0U;
}

void screen_clear(void) {
  volatile unsigned char *videoPtr = ST_VIDEO_BASE;
  for (unsigned long i = 0; i < ST_SCREEN_SIZE_BYTES; ++i) {
    videoPtr[i] = 0x00;
  }
}

unsigned char screen_is_medium_mode(void) {
  if (!ST_VIDEO_MODE_VALID) {
    ST_VIDEO_MODE = (unsigned char)(ST_SHIFTER_MODE & kShifterModeMask);
    ST_VIDEO_MODE_VALID = 1U;
  }
  return (unsigned char)(ST_VIDEO_MODE == kShifterModeMedium);
}

void screen_set_display_palette(void) {
  if (ST_MFP_GPIP & kMonitorColorDetectBit) {
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_0] = SCREEN_PAL_COLOR_IDX0;
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_1] = SCREEN_PAL_COLOR_IDX1;
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_2] = SCREEN_PAL_COLOR_IDX2;
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_3] = SCREEN_PAL_COLOR_IDX3;
  } else {
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_0] = SCREEN_PAL_MONO_IDX0;
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_1] = SCREEN_PAL_MONO_IDX1;
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_2] = SCREEN_PAL_MONO_IDX2;
    ST_PALETTE_BASE[SCREEN_PAL_INDEX_3] = SCREEN_PAL_MONO_IDX3;
  }
}
