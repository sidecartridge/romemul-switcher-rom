/**
 * File: src/common/glyph.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared glyph rendering interface.
 */

#pragma once

const unsigned char *glyph_lookup(char character);
void glyph_plot(unsigned short col, unsigned short row, const unsigned char *rows,
                unsigned char color);
