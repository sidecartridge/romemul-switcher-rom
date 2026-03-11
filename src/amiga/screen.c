/**
 * File: src/amiga/screen.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga screen setup and framebuffer control.
 */

#include "screen.h"

#include "../common/term.h"
#include "hw.h"
#include "mem.h"

enum {
  kDisableAll = 0x7FFFU,
  kEnableCopperAndBitplaneDma = 0x8380U,
  kBaseBplcon0 = 0x0200U,
  kHiresTwoPlaneBplcon0 = 0xA200U,
  kBplcon1Default = 0x0000U,
  kBplcon2Default = 0x0000U,
  kBplcon3Default = 0x0020U,
  kDisplayWindowStart = 0x2C81U,
  kDisplayWindowStop = 0x2CC1U,
  kDataFetchStart = 0x003CU,
  kDataFetchStop = 0x00D4U,
  kBeamcon0DisplayEnable = 0x0020U,
  kCopperColorCount = 4U,
  kSpriteCount = 8U,
  kRegisterSpr0Pth = 0x0120U,
  kRegisterBpl1Pth = 0x00E0U,
  kRegisterColor00 = 0x0180U,
  kRegisterBplcon0 = 0x0100U,
  kRegisterBplcon1 = 0x0102U,
  kRegisterBplcon3 = 0x010CU,
  kRegisterDdfstrt = 0x0092U,
  kRegisterDdfstop = 0x0094U,
  kRegisterDiwstrt = 0x008EU,
  kRegisterDiwstop = 0x0090U,
  kRegisterBpl1mod = 0x0108U,
  kRegisterBpl2mod = 0x010AU
};

typedef union {
  unsigned long raw;
  struct {
    unsigned short addr;
    unsigned short data;
  } move;
} copper_ins_t;

typedef struct {
  copper_ins_t hi;
  copper_ins_t lo;
} copper_ptr_t;

typedef struct {
  copper_ptr_t sprpt[kSpriteCount];
  copper_ins_t bplcon0;
  copper_ins_t bplcon1;
  copper_ins_t bplcon3;
  copper_ins_t ddfstrt;
  copper_ins_t ddfstop;
  copper_ins_t diwstrt;
  copper_ins_t diwstop;
  copper_ins_t bpl1mod;
  copper_ins_t bpl2mod;
  copper_ins_t color[kCopperColorCount];
  copper_ptr_t bplpt[AMIGA_SCREEN_PLANE_COUNT];
  copper_ins_t end;
} hires_copper_t;

typedef struct {
  hires_copper_t copper;
} video_runtime_t;

typedef struct {
  volatile unsigned short cursor_col;
  volatile unsigned short cursor_row;
  volatile unsigned char text_color;
} screen_state_t;

#define COPPER_MOVE(reg, value)   { .move = { (unsigned short)(reg), (unsigned short)(value) } }

static screen_state_t gScreenState;
static video_runtime_t gVideoRuntime;

static const hires_copper_t kRomHiresCopper = {
    .bplcon0 = COPPER_MOVE(kRegisterBplcon0, kHiresTwoPlaneBplcon0),
    .bplcon1 = COPPER_MOVE(kRegisterBplcon1, kBplcon1Default),
    .bplcon3 = COPPER_MOVE(kRegisterBplcon3, kBplcon3Default),
    .ddfstrt = COPPER_MOVE(kRegisterDdfstrt, kDataFetchStart),
    .ddfstop = COPPER_MOVE(kRegisterDdfstop, kDataFetchStop),
    .diwstrt = COPPER_MOVE(kRegisterDiwstrt, kDisplayWindowStart),
    .diwstop = COPPER_MOVE(kRegisterDiwstop, kDisplayWindowStop),
    .bpl1mod = COPPER_MOVE(kRegisterBpl1mod, 0x0000U),
    .bpl2mod = COPPER_MOVE(kRegisterBpl2mod, 0x0000U),
    .end = {.raw = 0xFFFFFFFEUL}};

static const unsigned short kMenuPalette[kCopperColorCount] = {
    0x0000U, 0x0FFFU, 0x00F0U, 0x0F00U};

static void clear_bytes(volatile unsigned char *dst, unsigned long size_bytes) {
  for (unsigned long i = 0U; i < size_bytes; ++i) {
    dst[i] = 0x00U;
  }
}

static void copy_words(volatile unsigned short *dst, const unsigned short *src,
                       unsigned long word_count) {
  for (unsigned long i = 0U; i < word_count; ++i) {
    dst[i] = src[i];
  }
}

static void set_copper_pointer(volatile copper_ptr_t *entry,
                               unsigned short reg_base,
                               unsigned long pointer) {
  entry->hi.move.addr = reg_base;
  entry->hi.move.data = (unsigned short)((pointer >> 16U) & 0xFFFFU);
  entry->lo.move.addr = (unsigned short)(reg_base + 2U);
  entry->lo.move.data = (unsigned short)(pointer & 0xFFFFU);
}

static unsigned long screen_plane_base(unsigned short plane_index) {
  return AMIGA_SCREEN_BASE_ADDR +
         ((unsigned long)plane_index * (unsigned long)AMIGA_SCREEN_PLANE_SIZE_BYTES);
}

static volatile hires_copper_t *screen_runtime_copper(void) {
  return &gVideoRuntime.copper;
}

static void init_runtime_copper_register_map(volatile hires_copper_t *copper) {
  for (unsigned short i = 0U; i < kSpriteCount; ++i) {
    const unsigned short reg_base =
        (unsigned short)(kRegisterSpr0Pth + (unsigned short)(i * 4U));
    set_copper_pointer(&copper->sprpt[i], reg_base, 0U);
  }

  for (unsigned short i = 0U; i < kCopperColorCount; ++i) {
    copper->color[i].move.addr =
        (unsigned short)(kRegisterColor00 + (unsigned short)(i * 2U));
    copper->color[i].move.data = 0x0000U;
  }

  for (unsigned short i = 0U; i < AMIGA_SCREEN_PLANE_COUNT; ++i) {
    const unsigned short reg_base =
        (unsigned short)(kRegisterBpl1Pth + (unsigned short)(i * 4U));
    set_copper_pointer(&copper->bplpt[i], reg_base, screen_plane_base(i));
  }
}

static void patch_runtime_palette(const unsigned short *palette,
                                  unsigned short color_count) {
  volatile hires_copper_t *const copper = screen_runtime_copper();

  for (unsigned short i = 0U; i < kCopperColorCount; ++i) {
    const unsigned short value = (i < color_count) ? palette[i] : 0x0000U;
    copper->color[i].move.data = value;
  }

  AMIGA_COLOR00 = copper->color[0].move.data;
  AMIGA_COLOR01 = copper->color[1].move.data;
  AMIGA_COLOR02 = copper->color[2].move.data;
  AMIGA_COLOR03 = copper->color[3].move.data;
}

static void init_runtime_copper(void) {
  volatile hires_copper_t *const copper = screen_runtime_copper();

  copy_words((volatile unsigned short *)copper,
             (const unsigned short *)&kRomHiresCopper,
             (unsigned long)(sizeof(kRomHiresCopper) / sizeof(unsigned short)));
  init_runtime_copper_register_map(copper);
  patch_runtime_palette(kMenuPalette, kCopperColorCount);
}

static void init_screen_state(void) {
  gScreenState.cursor_col = 0U;
  gScreenState.cursor_row = 0U;
  gScreenState.text_color = 1U;
}

static void disable_display_dma_and_interrupts(void) {
  AMIGA_DMACON = kDisableAll;
  AMIGA_INTENA = kDisableAll;
  AMIGA_INTREQ = kDisableAll;
  AMIGA_ADKCON = kDisableAll;
  AMIGA_COP1LCH = 0x0000U;
  AMIGA_COP1LCL = 0x0000U;
}

static void start_runtime_copper(volatile hires_copper_t *copper) {
  const unsigned long copper_address = (unsigned long)copper;

  __asm__ volatile(
      "move.l %0, 0x00dff080\n\t"
      "move.w 0x00dff088, %%d0\n\t"
      "move.w %1, 0x00dff096\n\t"
      "move.w %2, 0x00dff1dc\n\t"
      :
      : "d"(copper_address), "d"(kEnableCopperAndBitplaneDma),
        "d"(kBeamcon0DisplayEnable)
      : "d0", "memory", "cc");
}

volatile unsigned char *term_video_base(void) {
  return (volatile unsigned char *)AMIGA_SCREEN_BASE_ADDR;
}

volatile unsigned short *term_cursor_col_ptr(void) {
  return &gScreenState.cursor_col;
}

volatile unsigned short *term_cursor_row_ptr(void) {
  return &gScreenState.cursor_row;
}

volatile unsigned char *term_text_color_ptr(void) {
  return &gScreenState.text_color;
}

unsigned short term_text_row_bytes(void) {
  return (unsigned short)AMIGA_TEXT_ROW_BYTES;
}

void term_screen_clear(void) { screen_clear(); }

void screen_clear(void) {
  clear_bytes((volatile unsigned char *)AMIGA_SCREEN_BASE_ADDR,
              (unsigned long)(AMIGA_SCREEN_PLANE_COUNT *
                              AMIGA_SCREEN_PLANE_SIZE_BYTES));
}

void screen_init(void) {
  init_screen_state();
  disable_display_dma_and_interrupts();

  clear_bytes((volatile unsigned char *)&gVideoRuntime,
              (unsigned long)sizeof(gVideoRuntime));
  clear_bytes((volatile unsigned char *)AMIGA_SCREEN_BASE_ADDR,
              (unsigned long)(AMIGA_SCREEN_PLANE_COUNT *
                              AMIGA_SCREEN_PLANE_SIZE_BYTES));
  init_runtime_copper();

  AMIGA_BPLCON0 = kBaseBplcon0;
  AMIGA_BPLCON1 = kBplcon1Default;
  AMIGA_BPLCON2 = kBplcon2Default;
  AMIGA_BPLCON3 = 0x0000U;
  AMIGA_BPL1MOD = 0x0000U;
  AMIGA_BPL2MOD = 0x0000U;

  start_runtime_copper(screen_runtime_copper());
}
