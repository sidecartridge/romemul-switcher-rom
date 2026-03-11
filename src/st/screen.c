/**
 * File: src/st/screen.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST screen setup and framebuffer control.
 */

#include "screen.h"

#include "mem.h"
#include "palette.h"

#define ST_VIDEO_BASE ((volatile unsigned char *)ST_SCREEN_BASE_ADDR)
#define ST_PALETTE_BASE ((volatile unsigned short *)(unsigned long)0x00FF8240UL)
#define ST_MFP_GPIP (*(volatile unsigned char *)(unsigned long)0x00FFFA01UL)
#define ST_SHIFTER_MODE (*(volatile unsigned char *)(unsigned long)0x00FF8260UL)
#define ST_SHIFTER_BASE_HI (*(volatile unsigned char *)(unsigned long)0x00FF8201UL)
#define ST_SHIFTER_BASE_MID (*(volatile unsigned char *)(unsigned long)0x00FF8203UL)
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
  kByteMask = 0xFFU
};

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

void screen_init(void) {
  const unsigned long base = ST_SCREEN_BASE_ADDR;
  const unsigned char gpip = ST_MFP_GPIP;

  /* GPIP bit 7: 0=mono monitor, 1=color monitor. */
  if (gpip & kMonitorColorDetectBit) {
    ST_SHIFTER_MODE = kShifterModeMedium; /* 640x200, 4 colours */
  } else {
    ST_SHIFTER_MODE = kShifterModeHi; /* 640x400, monochrome */
  }

  ST_SHIFTER_BASE_HI = (unsigned char)((base >> kByteShiftHigh) & kByteMask);
  ST_SHIFTER_BASE_MID = (unsigned char)((base >> kByteShiftMid) & kByteMask);

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
