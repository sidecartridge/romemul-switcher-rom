/**
 * File: src/amiga/platform.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga platform-specific ROM helper routines.
 */

#include "../common/platform.h"

#include "hw.h"

static inline unsigned long pack_00f8(unsigned short value) {
  return 0x00F80000UL | (unsigned long)value;
}

unsigned long platform_get_system_time_seed(void) {
  return *(volatile unsigned long *)0x00000004UL;
}

unsigned long platform_send_magic_sequence(unsigned long rom_base_address,
                                           unsigned short command,
                                           unsigned short param) {
  unsigned long seed = platform_get_system_time_seed();

  (void)rom_base_address;

  seed ^= (seed << 13);
  seed ^= (seed >> 17);
  seed ^= (seed << 5);

  const unsigned long magic_number = seed & 0xFFFEFFFEUL;
  const unsigned short magic_number_lsb =
      (unsigned short)(magic_number & 0xFFFFU);
  const unsigned short magic_number_msb =
      (unsigned short)((magic_number >> 16) & 0xFFFFU);
  unsigned short checksum = command;
  register unsigned long a0_reg __asm__("a0") = pack_00f8(magic_number_msb);
  register unsigned long a1_reg __asm__("a1") = pack_00f8(magic_number_lsb);
  register unsigned long a2_reg __asm__("a2") = pack_00f8(command);
  register unsigned long a3_reg __asm__("a3") = pack_00f8(param);
  register unsigned long a4_reg __asm__("a4") = pack_00f8(checksum);

  checksum = (unsigned short)(checksum + param);
  checksum = (unsigned short)(checksum + magic_number_lsb);
  checksum = (unsigned short)(checksum + magic_number_msb);
  checksum = (unsigned short)(checksum & 0xFFFEU);

  a4_reg = pack_00f8(checksum);

  __asm__ volatile(
      "tst.b 0xF81234\n\t"
      "tst.b 0xF8FC42\n\t"
      "tst.b 0xF86452\n\t"
      "tst.b 0xF8CDE0\n\t"
      "tst.b 0xF85CA2\n\t"
      "tst.b 0xF88CA4\n\t"
      "tst.b 0xF81F94\n\t"
      "tst.b 0xF8E642\n\t"
      "tst.b (%%a0)\n\t"
      "tst.b (%%a1)\n\t"
      "tst.b (%%a2)\n\t"
      "tst.b (%%a3)\n\t"
      "tst.b (%%a4)\n\t"
      :
      : "a"(a0_reg), "a"(a1_reg), "a"(a2_reg), "a"(a3_reg), "a"(a4_reg)
      : "memory", "cc");

  return magic_number;
}

void platform_poll(void) {}
