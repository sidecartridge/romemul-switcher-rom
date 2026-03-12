/**
 * File: src/common/rom_check.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-12
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared ROM checksum verification helpers.
 */

#include "rom_check.h"

#include "platform.h"

static const unsigned long kCheckFieldSizeBytes = 4UL;
static const unsigned long kProgressPollStrideBytes = 4096UL;

static unsigned long load_be32(const volatile unsigned char *address) {
  unsigned long value = 0UL;
  value |= ((unsigned long)address[0] << 24U);
  value |= ((unsigned long)address[1] << 16U);
  value |= ((unsigned long)address[2] << 8U);
  value |= (unsigned long)address[3];
  return value;
}

static unsigned long add_range_be16(const volatile unsigned char *rom_base,
                                    unsigned long start_offset,
                                    unsigned long end_offset,
                                    unsigned long running_sum,
                                    unsigned long *processed_bytes,
                                    unsigned long total_bytes,
                                    rom_check_progress_fn_t progress_fn) {
  unsigned long offset = start_offset;

  while (offset + 1UL < end_offset) {
    running_sum +=
        (unsigned long)(((unsigned short)rom_base[offset] << 8U) |
                        (unsigned short)rom_base[offset + 1UL]);
    offset += 2UL;
    *processed_bytes += 2UL;

    if (((*processed_bytes % kProgressPollStrideBytes) == 0UL) ||
        (*processed_bytes == total_bytes)) {
      platform_poll();
      if (progress_fn != (rom_check_progress_fn_t)0) {
        progress_fn(*processed_bytes, total_bytes);
      }
    }
  }

  if (offset < end_offset) {
    running_sum += (unsigned long)((unsigned short)rom_base[offset] << 8U);
    *processed_bytes += 1UL;
    platform_poll();
    if (progress_fn != (rom_check_progress_fn_t)0) {
      progress_fn(*processed_bytes, total_bytes);
    }
  }

  return running_sum;
}

void rom_check_verify(const volatile unsigned char *rom_base,
                      unsigned long rom_size_bytes,
                      unsigned long check_field_offset_bytes,
                      unsigned long ignored_field_offset_bytes,
                      unsigned long ignored_field_size_bytes,
                      rom_check_progress_fn_t progress_fn,
                      rom_check_result_t *out_result) {
  unsigned long processed_bytes = 0UL;
  unsigned long checksum = 0UL;
  unsigned long check_field_end = 0UL;

  if (out_result == (rom_check_result_t *)0 ||
      rom_base == (const volatile unsigned char *)0) {
    return;
  }

  out_result->stored_value = 0UL;
  out_result->computed_value = 0UL;
  out_result->matches = 0U;

  check_field_end = check_field_offset_bytes + kCheckFieldSizeBytes;
  if (check_field_end > rom_size_bytes) {
    return;
  }

  out_result->stored_value = load_be32(rom_base + check_field_offset_bytes);

  checksum = add_range_be16(rom_base, 0UL, check_field_offset_bytes, checksum,
                            &processed_bytes, rom_size_bytes, progress_fn);

  if (ignored_field_size_bytes != 0UL) {
    if ((ignored_field_offset_bytes + ignored_field_size_bytes) >
        rom_size_bytes) {
      return;
    }
    checksum = add_range_be16(rom_base, check_field_end,
                              ignored_field_offset_bytes, checksum,
                              &processed_bytes, rom_size_bytes, progress_fn);
    checksum = add_range_be16(
        rom_base, ignored_field_offset_bytes + ignored_field_size_bytes,
        rom_size_bytes, checksum, &processed_bytes, rom_size_bytes,
        progress_fn);
  } else {
    checksum = add_range_be16(rom_base, check_field_end, rom_size_bytes,
                              checksum, &processed_bytes, rom_size_bytes,
                              progress_fn);
  }

  if (processed_bytes < rom_size_bytes) {
    platform_poll();
    if (progress_fn != (rom_check_progress_fn_t)0) {
      progress_fn(rom_size_bytes, rom_size_bytes);
    }
  }

  out_result->computed_value = checksum;
  out_result->matches =
      (unsigned short)((out_result->computed_value == out_result->stored_value)
                           ? 1U
                           : 0U);
}
