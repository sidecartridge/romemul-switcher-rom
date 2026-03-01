#pragma once

#include <stdint.h>

void rom_switcher_main(void);

/* Atari ST hardware helpers */
#define ST_PALETTE_BASE     ((volatile uint16_t *)0xFFFF8240)
#define ST_BORDER_COLOR     (*(volatile uint16_t *)0xFFFF820A)
#define ST_IKBD_STATUS      (*(volatile uint8_t  *)0xFFFFFC00)
#define ST_IKBD_DATA        (*(volatile uint8_t  *)0xFFFFFC02)

#define IKBD_STATUS_RX_FULL 0x01

/* Simple colors for monochrome output */
#define COLOR_BANK0 0x000
#define COLOR_BANK1 0x777
#define COLOR_BANK2 0x700
#define COLOR_BANK3 0x070

uint8_t st_poll_scancode(void);
void    st_wait_vbl(void);
void    st_set_border(uint16_t color);
void    st_draw_banner(void);
