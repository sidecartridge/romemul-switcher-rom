#pragma once

#include "mem.h"

#define ST_VIDEO_BASE ((volatile unsigned char *)ST_SCREEN_BASE_ADDR)

#define ST_CURSOR_COL \
  (*(volatile unsigned short *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 0U))
#define ST_CURSOR_ROW \
  (*(volatile unsigned short *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 2U))
#define ST_TEXT_COLOR \
  (*(volatile unsigned char *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 4U))
#define ST_VIDEO_MODE \
  (*(volatile unsigned char *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 5U))
#define ST_VIDEO_MODE_VALID \
  (*(volatile unsigned char *)(unsigned long)(ST_RAM_VARS_BASE_ADDR_UL + 6U))

#define ST_MONO_DETECT_BIT 0x80U

#define SCR_LINE_BYTES_HI 80
#define SCR_LINE_BYTES_MED 160

void screen_init(void);
void screen_clear(void);
unsigned char screen_is_medium_mode(void);
void screen_set_display_palette(void);
void screen_wait_vbl(void);
