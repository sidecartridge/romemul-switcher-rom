#pragma once

#define SCR_WIDTH_CHARS 80
#define SCR_HEIGHT_LINES 25
#define SCR_CHAR_W 8
#define SCR_CHAR_H 8

#define invertVideoEnable() text_printf("\033p")   // Enable invert video mode
#define invertVideoDisable() text_printf("\033q")  // Disable invert video mode
#define deleteLineFromCursor() \
  text_printf("\033K")                     // Delete from cursor to end of line
#define deleteLine() text_printf("\033M")  // Delete

void text_init(void);
void text_clear(void);
void text_set_cursor(unsigned short col, unsigned short row);
void text_set_color(unsigned char color);
void text_putc(char character);
void text_puts(const char *text);
void text_build_title_line(char out[SCR_WIDTH_CHARS + 1], const char *computer_model);
int text_printf(const char *fmt, ...);
unsigned short text_run_feature_tests(void);
