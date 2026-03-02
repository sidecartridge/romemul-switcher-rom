#pragma once


const unsigned char *glyph_lookup(char c);
void glyph_plot(unsigned short col, unsigned short row, const unsigned char *rows, unsigned char color);
