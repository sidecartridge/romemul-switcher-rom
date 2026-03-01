#pragma once

#include <stdint.h>

const uint8_t *glyph_lookup(char c);
void glyph_plot(uint16_t col, uint16_t row, const uint8_t *rows, uint8_t color);
