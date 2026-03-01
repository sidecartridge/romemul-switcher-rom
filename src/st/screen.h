#pragma once

#include <stdint.h>

#define ST_MONO_DETECT_BIT 0x80U

#define SCR_LINE_BYTES_HI 80
#define SCR_LINE_BYTES_MED 160

#define ST_RAM_TOP_ADDR_UL 0x00040000UL
#define ST_STACK_TOP_ADDR_UL 0x00038000UL
#define ST_SCREEN_BASE_ADDR_UL ST_STACK_TOP_ADDR_UL
#define ST_SCREEN_SIZE_BYTES 32000UL
#define ST_RAM_VARS_BASE_ADDR_UL 0x0003FD00UL
#define ST_RAM_VARS_SIZE_BYTES 768UL

/* Compile-time layout guards. */
enum {
  ST_LAYOUT_SCREEN_END = ST_SCREEN_BASE_ADDR_UL + ST_SCREEN_SIZE_BYTES,
  ST_LAYOUT_VARS_END = ST_RAM_VARS_BASE_ADDR_UL + ST_RAM_VARS_SIZE_BYTES,
  ST_LAYOUT_CHECK_SCREEN_VARS =
      1 / ((ST_LAYOUT_SCREEN_END <= ST_RAM_VARS_BASE_ADDR_UL) ? 1 : 0),
  ST_LAYOUT_CHECK_VARS_TOP =
      1 / ((ST_LAYOUT_VARS_END <= ST_RAM_TOP_ADDR_UL) ? 1 : 0)
};

#define ST_STACK_TOP_ADDR ((uintptr_t)ST_STACK_TOP_ADDR_UL)
#define ST_SCREEN_BASE_ADDR ((uintptr_t)ST_SCREEN_BASE_ADDR_UL)
#define ST_RAM_VARS_BASE_ADDR ((uintptr_t)ST_RAM_VARS_BASE_ADDR_UL)

#define ST_VIDEO_BASE ((volatile uint8_t *)ST_SCREEN_BASE_ADDR)

#define ST_CURSOR_COL \
  (*(volatile uint16_t *)(uintptr_t)(ST_RAM_VARS_BASE_ADDR_UL + 0U))
#define ST_CURSOR_ROW \
  (*(volatile uint16_t *)(uintptr_t)(ST_RAM_VARS_BASE_ADDR_UL + 2U))
#define ST_TEXT_COLOR \
  (*(volatile uint8_t *)(uintptr_t)(ST_RAM_VARS_BASE_ADDR_UL + 4U))
#define ST_VIDEO_MODE \
  (*(volatile uint8_t *)(uintptr_t)(ST_RAM_VARS_BASE_ADDR_UL + 5U))
#define ST_VIDEO_MODE_VALID \
  (*(volatile uint8_t *)(uintptr_t)(ST_RAM_VARS_BASE_ADDR_UL + 6U))

void screen_init(void);
void screen_clear(void);
uint8_t screen_is_medium_mode(void);
void screen_set_display_palette(void);
void screen_wait_vbl(void);
