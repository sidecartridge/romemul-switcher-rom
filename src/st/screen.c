#include "screen.h"

#include "palette.h"

#define ST_PALETTE_BASE ((volatile unsigned short *)(unsigned long)0x00FF8240UL)
#define ST_MFP_GPIP (*(volatile unsigned char *)(unsigned long)0x00FFFA01UL)

#define ST_SHIFTER_MODE (*(volatile unsigned char *)(unsigned long)0x00FF8260UL)

#define ST_SHIFTER_BASE_HI (*(volatile unsigned char *)(unsigned long)0x00FF8201UL)
#define ST_SHIFTER_BASE_MID (*(volatile unsigned char *)(unsigned long)0x00FF8203UL)

enum {
  kShifterModeMedium = 0x01U,
  kShifterModeHi = 0x02U,
  kShifterModeMask = 0x03U,
  kByteShiftHigh = 16,
  kByteShiftMid = 8,
  kByteMask = 0xFFU,
  kMfpGpipVblMask = 0x0080U
};

void screen_init(void) {
  const unsigned long base = ST_SCREEN_BASE_ADDR;
  const unsigned char gpip = ST_MFP_GPIP;

  /* GPIP bit 7: 0=mono monitor, 1=color monitor. */
  if (gpip & ST_MONO_DETECT_BIT) {
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
  if (ST_MFP_GPIP & ST_MONO_DETECT_BIT) {
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

void screen_wait_vbl(void) {
  volatile unsigned short *const mfpVblReg = (volatile unsigned short *)0xFFFA06;
  while (!(*mfpVblReg & kMfpGpipVblMask)) {
    /* wait */
  }
}
