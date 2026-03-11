/**
 * File: src/common/term.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared terminal control declarations.
 */

#pragma once

volatile unsigned char *term_video_base(void);
volatile unsigned short *term_cursor_col_ptr(void);
volatile unsigned short *term_cursor_row_ptr(void);
volatile unsigned char *term_text_color_ptr(void);
unsigned short term_text_row_bytes(void);
void term_screen_clear(void);

#define TERM_VIDEO_BASE term_video_base()
#define TERM_CURSOR_COL (*term_cursor_col_ptr())
#define TERM_CURSOR_ROW (*term_cursor_row_ptr())
#define TERM_TEXT_COLOR (*term_text_color_ptr())
#define TERM_TEXT_ROW_BYTES term_text_row_bytes()
