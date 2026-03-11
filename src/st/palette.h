/**
 * File: src/st/palette.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST palette definitions.
 */

#pragma once

/* Atari ST 12-bit RGB palette values (0x0RGB). */

#define SCREEN_PAL_INDEX_0 0U
#define SCREEN_PAL_INDEX_1 1U
#define SCREEN_PAL_INDEX_2 2U
#define SCREEN_PAL_INDEX_3 3U

/* Color monitor palette: 0=black, 1=white, 2=green, 3=red. */
#define SCREEN_PAL_COLOR_IDX0 0x000U
#define SCREEN_PAL_COLOR_IDX1 0x777U
#define SCREEN_PAL_COLOR_IDX2 0x070U
#define SCREEN_PAL_COLOR_IDX3 0x700U

/* Mono monitor effective palette (use first two entries). */
#define SCREEN_PAL_MONO_IDX0 0x000U
#define SCREEN_PAL_MONO_IDX1 0x777U
#define SCREEN_PAL_MONO_IDX2 SCREEN_PAL_MONO_IDX1
#define SCREEN_PAL_MONO_IDX3 SCREEN_PAL_MONO_IDX1
