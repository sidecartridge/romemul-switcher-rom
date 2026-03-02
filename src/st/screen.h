#pragma once

#define ST_MONO_DETECT_BIT 0x80U

#define SCR_LINE_BYTES_HI 80
#define SCR_LINE_BYTES_MED 160

#define ST_RAM_TOP_ADDR_UL 0x00080000UL
#define ST_STACK_TOP_ADDR_UL (ST_RAM_TOP_ADDR_UL - 32768UL)
#define ST_SCREEN_BASE_ADDR_UL ST_STACK_TOP_ADDR_UL
#define ST_SCREEN_SIZE_BYTES 32000UL
#define ST_RAM_VARS_BASE_ADDR_UL (ST_STACK_TOP_ADDR_UL - (128UL * 1024UL))
#define ST_RAM_VARS_SIZE_BYTES (124UL * 1024UL)

/* Compile-time layout guards. */
enum {
  ST_LAYOUT_VARS_END = ST_RAM_VARS_BASE_ADDR_UL + ST_RAM_VARS_SIZE_BYTES,
  ST_LAYOUT_SCREEN_END = ST_SCREEN_BASE_ADDR_UL + ST_SCREEN_SIZE_BYTES,
  ST_LAYOUT_CHECK_SCREEN_TOP =
      1 / ((ST_LAYOUT_SCREEN_END <= ST_RAM_TOP_ADDR_UL) ? 1 : 0),
  ST_LAYOUT_CHECK_SCREEN_VARS_NON_OVERLAP =
      1 / (((ST_LAYOUT_VARS_END <= ST_SCREEN_BASE_ADDR_UL) ||
            (ST_LAYOUT_SCREEN_END <= ST_RAM_VARS_BASE_ADDR_UL))
               ? 1
               : 0),
  ST_LAYOUT_CHECK_VARS_TOP =
      1 / ((ST_LAYOUT_VARS_END <= ST_RAM_TOP_ADDR_UL) ? 1 : 0)
};

#define ST_STACK_TOP_ADDR ((unsigned long)ST_STACK_TOP_ADDR_UL)
#define ST_SCREEN_BASE_ADDR ((unsigned long)ST_SCREEN_BASE_ADDR_UL)
#define ST_RAM_VARS_BASE_ADDR ((unsigned long)ST_RAM_VARS_BASE_ADDR_UL)

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

void screen_init(void);
void screen_clear(void);
unsigned char screen_is_medium_mode(void);
void screen_set_display_palette(void);
void screen_wait_vbl(void);
