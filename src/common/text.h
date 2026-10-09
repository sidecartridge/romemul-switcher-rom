/**
 * File: src/common/text.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared terminal-style text rendering declarations.
 */

#pragma once

#define SCR_WIDTH_CHARS 80
#define SCR_HEIGHT_LINES 25
#define SCR_CHAR_H 8

#define invertVideoEnable() text_printf("\033p")   // Enable invert video mode
#define invertVideoDisable() text_printf("\033q")  // Disable invert video mode
#define deleteLineFromCursor() \
  text_printf("\033K")                     // Delete from cursor to end of line

void text_init(void);
void text_clear(void);
void text_set_cursor(unsigned short col, unsigned short row);
void text_set_color(unsigned char color);
void text_putc(char character);
void text_build_title_line(char out[SCR_WIDTH_CHARS + 1], const char *computer_model);
int text_printf(const char *fmt, ...);
#if defined(_DEBUG) && (_DEBUG > 0)
/* text_printf()'s formatter writing into out[size], always terminated.
   Debug builds only: the trace uses it (trace.c). */
int text_vsnprintf(char *out, unsigned long size, const char *fmt,
                   __builtin_va_list args);
#endif
unsigned short text_run_feature_tests(void);
