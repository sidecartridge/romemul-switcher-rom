#pragma once

#include <stdint.h>

void rom_switcher_main(void);

/* Atari ST hardware helpers */
#define ST_PALETTE_BASE     ((volatile uint16_t *)0xFFFF8240)
#define ST_IKBD_STATUS      (*(volatile uint8_t  *)0xFFFFFC00)
#define ST_IKBD_DATA        (*(volatile uint8_t  *)0xFFFFFC02)

#define IKBD_STATUS_RX_FULL 0x01

/* Two-colour scheme for monochrome-safe feedback */
#define COLOR_DARK  0x000
#define COLOR_LIGHT 0x777

#define ST_PALETTE_BG_INDEX 0
#define ST_PALETTE_FG_INDEX 1

uint8_t st_poll_scancode(void);
void    st_wait_vbl(void);
void    st_set_palette_pair(uint16_t fg_color);
void    st_draw_banner(void);
