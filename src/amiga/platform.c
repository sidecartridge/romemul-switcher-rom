/**
 * File: src/amiga/platform.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga platform-specific ROM helper routines.
 */

#include "../common/nonce.h"
#include "../common/platform.h"

#include "hw.h"
#include "mem.h"

static inline unsigned long pack_00f8(unsigned short value) {
  return 0x00F80000UL | (unsigned long)value;
}

#if defined(_DEBUG) && (_DEBUG > 0)
/* The trace channel of debug builds: Paula's serial port, which FS-UAE maps
   to a host pseudo-terminal (EPIC-00 STORY-02). The same register sequence as
   sidecartos-config's EmuTOS-on-Amiga debug build. */
enum {
  kSerperPal115200 = 30U, /* 3,546,895 Hz / (30 + 1): about 115,200 baud */
  kSerdatrTxBufferEmpty = 0x2000U,
  kSerdatStopBit = 0x0100U
};

void platform_trace_init(void) { AMIGA_SERPER = kSerperPal115200; }

void platform_trace_write(const char *text) {
  if (text == (const char *)0) {
    return;
  }
  while (*text != '\0') {
    while ((AMIGA_SERDATR & kSerdatrTxBufferEmpty) == 0U) {
    }
    AMIGA_SERDAT = (unsigned short)(kSerdatStopBit | (unsigned char)*text);
    text++;
  }
}
#endif

unsigned long platform_get_system_time_seed(void) {
  return *(volatile unsigned long *)0x00000004UL;
}

unsigned long platform_send_magic_sequence(unsigned long rom_base_address,
                                           unsigned short command,
                                           unsigned short param) {
  /* A new nonce on every frame (EPIC-03 STORY-01). */
  const unsigned long magic_number = nonce_next(rom_base_address);
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

unsigned long platform_rom_image_size(void) { return AMIGA_ROM_SIZE_BYTES; }

/* Machine information (EPIC-03 STORY-09): the Agnus ID in VPOSR bits 8 to 14
   (bit 12 set for NTSC) and DENISEID, which an OCS Denise does not drive. */
/* The soak test's clock (EPIC-03 STORY-11): a frame each time the beam's
   vertical position (VPOSR bit 0 and VHPOSR's high byte) wraps. */
static unsigned long gFrames = 0UL;
static unsigned short gLastLine = 0U;

unsigned long platform_frames(void) {
  const unsigned short line = (unsigned short)(((AMIGA_VPOSR & 1U) << 8) |
                                               (AMIGA_VHPOSR >> 8));
  if (line < gLastLine) {
    gFrames++;
  }
  gLastLine = line;
  return gFrames;
}

unsigned short platform_frame_rate(void) {
  return ((AMIGA_VPOSR & 0x1000U) != 0U) ? 60U : 50U; /* NTSC Agnus: 60 Hz */
}

/* The RAM (EPIC-03 STORY-13), found the way DiagROM V2 finds it
   (srcs/asm/initcode.s, DetectMemory): a block is RAM when patterns read back,
   each after other data went on the bus so that a floating bus cannot echo
   it, and a block is a shadow when a tag written there shows up at a lower
   address. Chip RAM repeats every 512 KB or 1 MB below the 2 MB that Agnus
   can address. At 0xC00000, where the slow RAM sits, a machine without it
   shows nothing or the custom chips' mirror, and neither reads back. */
enum {
  kChipStep = 0x80000UL, /* 512 KB: our own RAM is below the first step */
  kChipTop = 0x200000UL,
  kSlowBase = 0xC00000UL,
  kSlowStep = 0x40000UL,    /* 256 KB, as DiagROM steps */
  kSlowSize = 0x180000UL,   /* 0xC00000 to 0xD7FFFF */
  kShadowTag = 0x53484457UL /* "SHDW" */
};

static volatile unsigned long gRamProbe[2]; /* chip RAM of ours, below 512 KB */

static int amigaIsRam(volatile unsigned long *at) {
  static const unsigned long kPatterns[] = {0xFF0000FFUL, 0x00FFFF00UL,
                                            0xAAAA5555UL, 0x5555AAAAUL};
  const unsigned long saved0 = at[0];
  const unsigned long saved1 = at[1];
  int ok = 1;
  unsigned short i;

  for (i = 0U; i < 4U && ok; ++i) {
    at[0] = kPatterns[i];
    at[1] = ~kPatterns[i]; /* other data on the bus */
    ok = (at[0] == kPatterns[i]);
  }
  at[0] = saved0;
  at[1] = saved1;
  return ok;
}

static int amigaIsShadow(volatile unsigned long *at,
                         volatile unsigned long *below) {
  const unsigned long saved_below = *below;
  const unsigned long saved_at = *at;
  int shadow;

  *below = 0UL;
  *at = kShadowTag;
  shadow = (*below == kShadowTag);
  *at = saved_at;
  *below = saved_below;
  return shadow;
}

void platform_probe_memory(platform_memory_t *out) {
  unsigned long chip = kChipStep;
  unsigned long slow = 0UL;

  while (chip < kChipTop) {
    volatile unsigned long *const at =
        (volatile unsigned long *)((unsigned long)gRamProbe + chip);
    if (amigaIsShadow(at, gRamProbe) || !amigaIsRam(at)) {
      break;
    }
    chip += kChipStep;
  }
  while (slow < kSlowSize) {
    volatile unsigned long *const at =
        (volatile unsigned long *)(kSlowBase + slow);
    if ((slow != 0UL &&
         amigaIsShadow(at, (volatile unsigned long *)kSlowBase)) ||
        !amigaIsRam(at)) {
      break;
    }
    slow += kSlowStep;
  }
  out->part_kb[0] = chip >> 10;
  out->part_kb[1] = slow >> 10;
  out->part_name[0] = "chip";
  out->part_name[1] = "slow";
}

void platform_probe_machine(platform_machine_t *out) {
  const unsigned short agnus = (unsigned short)((AMIGA_VPOSR >> 8) & 0x7FU);

  out->name = "amiga";
  out->label = "Amiga";
  switch (agnus) {
    case 0x00U:
      out->detail = "OCS Agnus, PAL";
      break;
    case 0x10U:
      out->detail = "OCS Agnus, NTSC";
      break;
    case 0x20U:
      out->detail = "ECS Agnus, PAL";
      break;
    case 0x30U:
      out->detail = "ECS Agnus, NTSC";
      break;
    case 0x22U:
      out->detail = "AGA Alice, PAL";
      break;
    case 0x32U:
      out->detail = "AGA Alice, NTSC";
      break;
    default:
      out->detail = "unknown Agnus";
      break;
  }
  out->chip_id = agnus;
  out->denise_id = (unsigned short)(AMIGA_DENISEID & 0xFFU);
  out->has_monitor_line = 0U;
  out->monitor_mono = 0U;
  out->monitor_changes = 0U;
  out->monitor_samples = 0U;
}
