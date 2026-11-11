/**
 * File: src/common/screenshot.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The screenshot build: a cartridge that always answers and a
 *              fixed key script.
 */

#include "screenshot.h"

#include "commands.h"
#include "kbd.h"
#include "platform.h"

#ifndef APP_VERSION_STR
#define APP_VERSION_STR "0.0.0"
#endif

/* Offsets from the firmware's parameters_t and rom_catalog_t (C-04), as
   chooser.c and selftest.c read them. */
enum {
  kPageBytes = 4096U,
  kRecordShift = 8U, /* records of 256 bytes */
  kRecordNameBytes = 64U,
  kRecordBlocks = 64U,
  kBlockKbShift = 2U, /* blocks of 4 KB */
  kParamsDefaultIndex = 70U,
  kParamsRescueIndex = 326U,
  kParamsVersion = 512U,
  kParamsTicks = 516U,
  kParamsBusFlags = 524U,
  kProtocolVersion = 0x0040U,
  /* What the CONFIG.TXT that ships with firmware v4.0.0 stores: READ_BUS_TICKS
     4, USE_OE 0, CE_SYNC_BYPASS, WRITE_ON_DRIVE and DMA_HIGH_PRIORITY 1,
     RESCUE_TIMEOUT 0, LARGE_ROMS and ACCELERATED_BUS 0. */
  kReadBusTicks = 4U,
  kBusFlags = 0x7U,
  kHoldSeconds = 5U,
  kPauseLoops = 200U /* well under a frame at 8 MHz */
};

typedef struct {
  const char *name;
  unsigned short kb;
} screenshot_rom_t;

typedef struct {
  const screenshot_rom_t *roms;
  unsigned short count;
  unsigned short default_index;
  unsigned short rescue_index;
} screenshot_list_t;

/* Sorted by name without regard to case, as the firmware lists them
   (builder_sort_roms()). */
static const screenshot_rom_t kAtariSt[] = {
    {"EmuTOS 1.4.0 192KB - English UK - 50Hz.IMG", 192U},
    {"RESCUE_SWITCHER_v" APP_VERSION_STR "_192KB.img", 192U},
    {"TOS 1.00 - English UK - 50Hz.img", 192U},
    {"TOS 1.02 - English UK - 50Hz.img", 192U},
    {"TOS 1.04 - English UK - 50Hz.img", 192U},
    {"TOS 1.04 - German - 50Hz.img", 192U},
    {"TOS 1.62 - English UK - 50Hz.img", 256U},
    {"TOS 2.06 - English UK - 50Hz.img", 256U}};

static const screenshot_rom_t kAtariSte[] = {
    {"EmuTOS 1.4.0 256KB - English UK - 50Hz.IMG", 256U},
    {"RESCUE_SWITCHER_v" APP_VERSION_STR "_256KB.img", 256U},
    {"TOS 1.00 - English UK - 50Hz.img", 192U},
    {"TOS 1.02 - English UK - 50Hz.img", 192U},
    {"TOS 1.04 - English UK - 50Hz.img", 192U},
    {"TOS 1.04 - German - 50Hz.img", 192U},
    {"TOS 1.62 - English UK - 50Hz.img", 256U},
    {"TOS 2.06 - English UK - 50Hz.img", 256U}};

static const screenshot_rom_t kAmiga[] = {
    {"DiagROM-1.3.img", 512U},
    {"Kickstart 1.3 r34.005.img", 512U},
    {"Kickstart 2.04 r37.175.img", 512U},
    {"Kickstart 3.1 r40.063.img", 512U},
    {"RESCUE_SWITCHER_v" APP_VERSION_STR "_512KB.img", 512U}};

/* The list of this image, told by its size: 192, 256 or 512 KB. */
static const screenshot_list_t *screenshotList(void) {
  static const screenshot_list_t kLists[] = {
      {kAtariSt, sizeof(kAtariSt) / sizeof(kAtariSt[0]), 0U, 1U},
      {kAtariSte, sizeof(kAtariSte) / sizeof(kAtariSte[0]), 0U, 1U},
      {kAmiga, sizeof(kAmiga) / sizeof(kAmiga[0]), 0U, 4U}};
  const unsigned long size = platform_rom_image_size();

  if (size == 0x30000UL) {
    return &kLists[0];
  }
  if (size == 0x40000UL) {
    return &kLists[1];
  }
  return &kLists[2];
}

static void putLe16(unsigned char *at, unsigned short value) {
  at[0] = (unsigned char)value;
  at[1] = (unsigned char)(value >> 8);
}

int screenshot_read_flash_page(unsigned char *page, unsigned long offset) {
  const screenshot_list_t *list = screenshotList();
  unsigned short i;

  for (i = 0U; i < kPageBytes; ++i) {
    page[i] = 0U;
  }
  if (offset == FLASH_PARAMS_START) {
    putLe16(page + kParamsDefaultIndex, list->default_index);
    putLe16(page + kParamsRescueIndex, list->rescue_index);
    putLe16(page + kParamsVersion, kProtocolVersion);
    page[kParamsTicks] = kReadBusTicks;
    page[kParamsBusFlags] = kBusFlags;
    return 0;
  }
  if (offset >= FLASH_CATALOG_START && offset < FLASH_PARAMS_START) {
    const unsigned short first =
        (unsigned short)((offset - FLASH_CATALOG_START) >> kRecordShift);

    for (i = 0U; i < (kPageBytes >> kRecordShift); ++i) {
      const unsigned short r = (unsigned short)(first + i);
      unsigned char *record = page + (i << kRecordShift);
      const char *name;
      unsigned short c;

      if (r >= list->count) {
        break;
      }
      name = list->roms[r].name;
      for (c = 0U; c + 1U < kRecordNameBytes && name[c] != '\0'; ++c) {
        record[c] = (unsigned char)name[c];
      }
      putLe16(record + kRecordBlocks,
              (unsigned short)(list->roms[r].kb >> kBlockKbShift));
    }
  }
  return 0;
}

/* The screens the user documentation shows, each held kHoldSeconds once it
   waits for a key: the list; RETURN, the confirmation; ESC, the list again;
   T, the self-test once it has finished; S, the soak once it has finished;
   ESC, the list, which then stays. */
static const unsigned char kScript[] = {KEY_RETURN, KEY_ESC, KEY_T, KEY_S,
                                        KEY_ESC};
static unsigned short gScriptStep = 0U;

/* Some work between two looks at the frame count, as the soak's reads are:
   polled in a tight loop on a Mega STE in mono, Hatari's video counter made
   platform_frames() count frames far faster than they passed. */
static void screenshotPause(void) {
  volatile unsigned short n = kPauseLoops;

  while (n-- > 0U) {
  }
}

unsigned char screenshot_key(void) {
  const unsigned short rate = platform_frame_rate();
  const unsigned long start = platform_frames();
  unsigned long hold = 0UL;
  unsigned short i;

  if (gScriptStep >= sizeof(kScript)) {
    for (;;) {
    }
  }
  /* A sum, not a multiply: a 32-bit multiply needs libgcc. */
  for (i = 0U; i < kHoldSeconds; ++i) {
    hold += rate;
  }
  while (platform_frames() - start < hold) {
    screenshotPause();
  }
  return kScript[gScriptStep++];
}
