/**
 * File: src/st/screen.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST screen control declarations.
 */

#pragma once

#define SCR_LINE_BYTES_HI 80
#define SCR_LINE_BYTES_MED 160

void screen_init(void);
void screen_clear(void);
unsigned char screen_is_medium_mode(void);
void screen_set_display_palette(void);
