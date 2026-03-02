#pragma once


#define SCR_WIDTH_CHARS 80
#define SCR_HEIGHT_LINES 25
#define SCR_CHAR_W 8
#define SCR_CHAR_H 8

void text_init(void);
void text_clear(void);
void text_set_cursor(unsigned short col, unsigned short row);
void text_set_color(unsigned char color);
void text_putc(char character);
void text_puts(const char *text);
int text_printf(const char *fmt, ...);
