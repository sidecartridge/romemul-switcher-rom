/**
 * File: src/st/platform.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST platform-specific ROM helper routines.
 */

#include "../common/nonce.h"
#include "../common/platform.h"
#include "mem.h"

#if defined(_DEBUG) && (_DEBUG > 0)
#include "htrace.h"

/* The trace channel of debug builds: Hatari's NF_STDERR native feature,
   which Hatari prints on its stderr (EPIC-00 STORY-02). */
void platform_trace_init(void) { hatari_trace_init(); }

void platform_trace_write(const char *text) { hatari_trace_msg(text); }
#endif

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
  /* A new nonce on every frame (EPIC-03 STORY-01). */
  const unsigned long magic_number = nonce_next(rom_base_address);
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

unsigned long platform_rom_image_size(void) { return ST_ROM_IMAGE_SIZE_BYTES; }

/* The soak test's clock (EPIC-03 STORY-11): a frame each time the video
   counter (0xFF8205/07/09) falls back to the screen base. It is read until two
   reads agree, as its three bytes change while they are read. */
enum {
  kStVideoCountHigh = 0x00FF8205,
  kStVideoCountMid = 0x00FF8207,
  kStVideoCountLow = 0x00FF8209,
  kStSyncMode = 0x00FF820A,
  kStShifterMode = 0x00FF8260
};

static unsigned long gFrames = 0UL;
static unsigned long gLastVideo = 0UL;

static unsigned long stVideoCounter(void) {
  unsigned long a;
  unsigned long b;

  do {
    a = ((unsigned long)*(volatile unsigned char *)kStVideoCountHigh << 16) |
        ((unsigned long)*(volatile unsigned char *)kStVideoCountMid << 8) |
        (unsigned long)*(volatile unsigned char *)kStVideoCountLow;
    b = ((unsigned long)*(volatile unsigned char *)kStVideoCountHigh << 16) |
        ((unsigned long)*(volatile unsigned char *)kStVideoCountMid << 8) |
        (unsigned long)*(volatile unsigned char *)kStVideoCountLow;
  } while ((a >> 8) != (b >> 8));
  return b;
}

unsigned long platform_frames(void) {
  const unsigned long video = stVideoCounter();
  if (video < gLastVideo) {
    gFrames++;
  }
  gLastVideo = video;
  return gFrames;
}

unsigned short platform_frame_rate(void) {
  if ((*(volatile unsigned char *)kStShifterMode & 0x03U) == 0x02U) {
    return 71U; /* monochrome */
  }
  return ((*(volatile unsigned char *)kStSyncMode & 0x02U) != 0U) ? 50U : 60U;
}

/* Machine information (EPIC-03 STORY-09): with no TOS to ask, probe hardware
   that only some models have, under a temporary bus-error handler
   (probe.s), and sample the MFP's monitor-detect line (GPIP bit 7). */
extern int st_probe_read(unsigned long address);

enum {
  kStDmaSoundControl = 0x00FF8901, /* STE, Mega STE */
  kStMegaSteCache = 0x00FF8E21,    /* Mega STE only */
  kStBlitterControl = 0x00FF8A3C,  /* Mega ST, STE, Mega STE: the blitter */
  kStMfpGpip = 0x00FFFA01,
  kStGpipMonitorColor = 0x80,
  kMonitorSamples = 32,
  kMonitorSampleDelay = 10000
};

void platform_probe_machine(platform_machine_t *out) {
  const int dma_sound = st_probe_read(kStDmaSoundControl);
  const int cache = st_probe_read(kStMegaSteCache);
  /* The Mega ST's clock (0xFFFC21) does not fault on a plain ST, in Hatari at
     least, and writing to it to look for a clock is unsafe there: the blitter
     tells them apart instead, standard on the Mega ST and absent from a stock
     ST. */
  const int blitter = st_probe_read(kStBlitterControl);
  unsigned char last;
  unsigned short i;

  if (dma_sound && cache) {
    out->name = "megaste";
    out->label = "Atari Mega STE";
    out->detail = "DMA sound and cache control";
  } else if (dma_sound) {
    out->name = "ste";
    out->label = "Atari STE";
    out->detail = "DMA sound, no cache control";
  } else if (blitter) {
    out->name = "megast";
    out->label = "Atari Mega ST";
    out->detail = "blitter, no DMA sound (or an ST with a blitter)";
  } else {
    out->name = "st";
    out->label = "Atari ST";
    out->detail = "no blitter, no DMA sound";
  }
  out->chip_id = 0U;
  out->denise_id = 0U;

  /* The line the ROM reads once at power-on to pick mono or colour; a change
     here would explain a cold boot in the wrong mode. */
  out->has_monitor_line = 1U;
  out->monitor_changes = 0U;
  last = (unsigned char)(*(volatile unsigned char *)kStMfpGpip & kStGpipMonitorColor);
  for (i = 0U; i < kMonitorSamples; ++i) {
    volatile unsigned long delay;
    unsigned char now;
    for (delay = 0UL; delay < kMonitorSampleDelay; ++delay) {
    }
    now = (unsigned char)(*(volatile unsigned char *)kStMfpGpip & kStGpipMonitorColor);
    if (now != last) {
      out->monitor_changes++;
    }
    last = now;
  }
  out->monitor_mono = (unsigned char)(last == 0U);
  out->monitor_samples = kMonitorSamples;
}

/* The RAM (EPIC-03 STORY-13), set before rom_switcher_main() runs: by the ROM
   startup's bank probe (memconf.inc), which sees the RAM only while it changes
   the MMU, or in the PRG from TOS's own probe (start.s). The total, and the
   MMU value that fits it: bank 0's size in bits 2 and 3. */
unsigned long st_ram_bytes = 0UL;
unsigned char st_ram_mmu = 0U;

void platform_probe_memory(platform_memory_t *out) {
  /* 128 KB, 512 KB or 2 MB. */
  unsigned long bank0 = 0x20000UL << (((st_ram_mmu >> 2) & 3U) << 1);

  if (bank0 > st_ram_bytes) {
    bank0 = st_ram_bytes;
  }
  out->part_kb[0] = bank0 >> 10;
  out->part_kb[1] = (st_ram_bytes - bank0) >> 10;
  out->part_name[0] = "bank 0";
  out->part_name[1] = "bank 1";
}

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
