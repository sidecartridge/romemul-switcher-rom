/**
 * File: src/common/rom_check.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-12
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared ROM checksum verification declarations.
 */

#pragma once

typedef struct {
  unsigned long stored_value;
  unsigned long computed_value;
  unsigned short matches;
} rom_check_result_t;

typedef void (*rom_check_progress_fn_t)(unsigned long processed_bytes,
                                        unsigned long total_bytes);

void rom_check_verify(const volatile unsigned char *rom_base,
                      unsigned long rom_size_bytes,
                      unsigned long check_field_offset_bytes,
                      unsigned long ignored_field_offset_bytes,
                      unsigned long ignored_field_size_bytes,
                      rom_check_progress_fn_t progress_fn,
                      rom_check_result_t *out_result);
