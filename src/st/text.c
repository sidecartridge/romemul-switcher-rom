#include "text.h"
#include "font8x8_basic.h"
#include <stddef.h>

static const uint8_t *lookup_glyph(char c)
{
    return font8x8_basic[(uint8_t)c];
}

static inline void plot_glyph(uint16_t col, uint16_t row, const uint8_t *rows)
{
    for (uint8_t y = 0; y < SCR_CHAR_H; ++y) {
        uint32_t line = (row * SCR_CHAR_H + y) * SCR_BYTES_PER_LINE;
        uint32_t offset = line + (col * SCR_BYTES_PER_CHAR);
        volatile uint8_t *dst = ST_VIDEO_BASE + offset;
        uint8_t pattern = rows[y];
        dst[0] = pattern;
        dst[1] = pattern; // double width for readability
    }
}

static uint16_t cursor_col = 0;
static uint16_t cursor_row = 0;

void text_clear(void)
{
    volatile uint8_t *p = ST_VIDEO_BASE;
    for (uint32_t i = 0; i < (uint32_t)SCR_BYTES_PER_LINE * SCR_CHAR_H * SCR_HEIGHT_LINES; ++i) {
        p[i] = 0x00;
    }
}

void text_set_cursor(uint16_t col, uint16_t row)
{
    if (col >= SCR_WIDTH_CHARS) col = 0;
    if (row >= SCR_HEIGHT_LINES) row = 0;
    cursor_col = col;
    cursor_row = row;
}

void text_init(void)
{
    text_clear();
    text_set_cursor(0, 0);
}

void text_putc(char c)
{
    if (c == '\n') {
        cursor_col = 0;
        cursor_row = (cursor_row + 1) % SCR_HEIGHT_LINES;
        return;
    }
    const uint8_t *glyph = lookup_glyph(c);
    plot_glyph(cursor_col, cursor_row, glyph);
    cursor_col++;
    if (cursor_col >= SCR_WIDTH_CHARS) {
        cursor_col = 0;
        cursor_row = (cursor_row + 1) % SCR_HEIGHT_LINES;
    }
}

void text_puts(const char *s)
{
    while (*s) {
        text_putc(*s++);
    }
}

void text_print_menu(uint8_t active_bank)
{
    text_clear();
    text_set_cursor(0, 0);
    text_puts("SIDECARTRIDGE ROM SWITCHER\n\n");
    for (uint8_t i = 0; i < 4; ++i) {
        text_puts("[");
        text_putc('1' + i);
        text_puts("] BANK ");
        text_putc('0' + i);
        if (i == active_bank) {
            text_puts("  <ACTIVE>");
        }
        text_putc('\n');
    }
    text_putc('\n');
    text_puts("PRESS 1-4 TO SELECT ROM BANK\n");
}
