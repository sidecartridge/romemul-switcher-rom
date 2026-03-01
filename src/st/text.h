#pragma once

#include <stdint.h>

#define SCR_WIDTH_CHARS   40
#define SCR_HEIGHT_LINES  25
#define SCR_CHAR_W        8
#define SCR_CHAR_H        8
#define SCR_BYTES_PER_LINE 80
#define SCR_BYTES_PER_CHAR  2

#define ST_VIDEO_BASE ((volatile uint8_t *)0x78000)

void text_init(void);
void text_clear(void);
void text_set_cursor(uint16_t col, uint16_t row);
void text_putc(char c);
void text_puts(const char *s);
void text_print_menu(uint8_t active_bank);
