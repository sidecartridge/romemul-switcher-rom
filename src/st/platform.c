/**
 * File: src/st/platform.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST platform-specific ROM helper routines.
 */

#include "../common/platform.h"

enum {
  kSeedSamples = 16U,
  kSeedRotateBits = 5U,
  kHardResetDelayLoops = 0x000FFFFFUL
};

unsigned long platform_get_system_time_seed(void) {
  unsigned long s = 0xA5A5A5A5UL;

  for (unsigned long i = 0; i < kSeedSamples; ++i) {
    const unsigned long tc =
        (unsigned long)(*(volatile unsigned char *)0x00FFFA23UL);
    const unsigned long tcd =
        (unsigned long)(*(volatile unsigned char *)0x00FFFA1DUL);

    s ^= (tc << ((i & 3UL) * 8UL));
    s = (s << kSeedRotateBits) | (s >> (32U - kSeedRotateBits));
    s ^= (tcd * 0x9EUL);
  }

  s ^= ((unsigned long)(*(volatile unsigned char *)0x00FFFA25UL) << 16);
  return s;
}

unsigned long platform_send_magic_sequence(unsigned long rom_base_address,
                                           unsigned short command,
                                           unsigned short param) {
  unsigned long seed = platform_get_system_time_seed();

  seed ^= (seed << 13);
  seed ^= (seed >> 17);
  seed ^= (seed << 5);

  const unsigned long magic_number = seed & 0xFFFEFFFEUL;
  const unsigned short magic_number_lsb =
      (unsigned short)(magic_number & 0xFFFFU);
  const unsigned short magic_number_msb =
      (unsigned short)((magic_number >> 16) & 0xFFFFU);
  unsigned short checksum = command;

  checksum = (unsigned short)(checksum + param);
  checksum = (unsigned short)(checksum + magic_number_lsb);
  checksum = (unsigned short)(checksum + magic_number_msb);
  checksum = (unsigned short)(checksum & 0xFFFEU);

  if (rom_base_address == 0x00FC0000UL) {
    __asm__ volatile(
        "move.l #0xFC0000, %%d1\n\t"
        "move.l %%d1, %%d2\n\t"
        "move.l %%d1, %%d3\n\t"
        "move.l %%d1, %%d4\n\t"
        "move.l %%d1, %%d5\n\t"
        "move.w %0, %%d1\n\t"
        "move.l %%d1, %%a0\n\t"
        "move.w %1, %%d2\n\t"
        "move.l %%d2, %%a1\n\t"
        "move.w %2, %%d3\n\t"
        "move.l %%d3, %%a2\n\t"
        "move.w %3, %%d4\n\t"
        "move.l %%d4, %%a3\n\t"
        "move.w %4, %%d5\n\t"
        "move.l %%d5, %%a4\n\t"
        "move.b 0xFC1234, %%d0\n\t"
        "move.b 0xFCFC42, %%d0\n\t"
        "move.b 0xFC6452, %%d0\n\t"
        "move.b 0xFCCDE0, %%d0\n\t"
        "move.b 0xFC5CA2, %%d0\n\t"
        "move.b 0xFC8CA4, %%d0\n\t"
        "move.b 0xFC1F94, %%d0\n\t"
        "move.b 0xFCE642, %%d0\n\t"
        "move.b (%%a0), %%d1\n\t"
        "move.b (%%a1), %%d2\n\t"
        "move.b (%%a2), %%d3\n\t"
        "move.b (%%a3), %%d4\n\t"
        "move.b (%%a4), %%d5\n\t"
        :
        : "m"(magic_number_msb), "m"(magic_number_lsb), "m"(command),
          "m"(param), "m"(checksum)
        : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "a2", "a3",
          "a4");
  } else if (rom_base_address == 0x00E00000UL) {
    __asm__ volatile(
        "move.l #0xE00000, %%d1\n\t"
        "move.l %%d1, %%d2\n\t"
        "move.l %%d1, %%d3\n\t"
        "move.l %%d1, %%d4\n\t"
        "move.l %%d1, %%d5\n\t"
        "move.w %0, %%d1\n\t"
        "move.l %%d1, %%a0\n\t"
        "move.w %1, %%d2\n\t"
        "move.l %%d2, %%a1\n\t"
        "move.w %2, %%d3\n\t"
        "move.l %%d3, %%a2\n\t"
        "move.w %3, %%d4\n\t"
        "move.l %%d4, %%a3\n\t"
        "move.w %4, %%d5\n\t"
        "move.l %%d5, %%a4\n\t"
        "move.b 0xE01234, %%d0\n\t"
        "move.b 0xE0FC42, %%d0\n\t"
        "move.b 0xE06452, %%d0\n\t"
        "move.b 0xE0CDE0, %%d0\n\t"
        "move.b 0xE05CA2, %%d0\n\t"
        "move.b 0xE08CA4, %%d0\n\t"
        "move.b 0xE01F94, %%d0\n\t"
        "move.b 0xE0E642, %%d0\n\t"
        "move.b (%%a0), %%d1\n\t"
        "move.b (%%a1), %%d2\n\t"
        "move.b (%%a2), %%d3\n\t"
        "move.b (%%a3), %%d4\n\t"
        "move.b (%%a4), %%d5\n\t"
        :
        : "m"(magic_number_msb), "m"(magic_number_lsb), "m"(command),
          "m"(param), "m"(checksum)
        : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "a2", "a3",
          "a4");
  }

  return magic_number;
}

void platform_poll(void) {}

void platform_hard_reset(void) {
  __asm__ volatile(
      "move.w #0x2700, %%sr\n\t"
      "move.l %0, %%d0\n\t"
      "1: nop\n\t"
      "subq.l #1, %%d0\n\t"
      "bne.s 1b\n\t"
      "clr.l 0x00000420\n\t"
      "clr.l 0x0000043A\n\t"
      "clr.l 0x0000051A\n\t"
      "move.l (0x4), %%a0\n\t"
      "jmp (%%a0)\n\t"
      :
      : "i"(kHardResetDelayLoops)
      : "d0", "a0", "memory", "cc");
}
