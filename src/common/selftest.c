/**
 * File: src/common/selftest.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-09
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The device self-test screen.
 */

#include "selftest.h"

#include "commands.h"
#include "kbd.h"
#include "palloc.h"
#include "platform.h"
#include "text.h"
#include "trace.h"

#ifndef APP_VERSION_STR
#define APP_VERSION_STR "0.0.0"
#endif
#ifndef BUILD_ID_STR
#define BUILD_ID_STR "unknown"
#endif

enum {
  kSelftestFirstRow = 2U,
  kDetailColumn = 28U, /* label 20 + result 8 */
  kDetailBytes = 208U, /* four rows of 52 characters */
  kColorDefault = 1U,
  kColorOk = 2U,
  kColorFail = 3U
};

typedef enum {
  kResultOk = 0,
  kResultFail = 1,
  kResultSkipped = 2
} selftest_result_t;

typedef struct {
  const selftest_env_t *env;
  unsigned short row;
  unsigned short passed;
  unsigned short failed;
  unsigned short skipped;
  unsigned char device_answers; /* set by the ping test */
} selftest_ctx_t;

/* Details from column 28, wrapped at spaces onto the next rows at the same
   column, so no line runs past column 80 (EPIC-03 STORY-12). */
static void selftestDetails(selftest_ctx_t *ctx, const char *text) {
  unsigned short col = kDetailColumn;

  while (*text != '\0') {
    unsigned short len = 0U;
    unsigned short i;

    while (text[len] != '\0' && text[len] != ' ') {
      len++;
    }
    if (col > kDetailColumn && col + len > SCR_WIDTH_CHARS) {
      text_set_cursor(kDetailColumn, ctx->row++);
      col = kDetailColumn;
    }
    for (i = 0U; i < len; ++i) {
      text_putc(text[i]);
    }
    col = (unsigned short)(col + len);
    text += len;
    while (*text == ' ') {
      if (col < SCR_WIDTH_CHARS) {
        text_putc(' ');
        col++;
      }
      text++;
    }
  }
}

/* A failure list built piece by piece, then reported as one detail text. */
typedef struct {
  char text[kDetailBytes];
  unsigned short len;
} detail_t;

static void detailInit(detail_t *d) {
  d->text[0] = '\0';
  d->len = 0U;
}

static void detailAdd(detail_t *d, const char *fmt, ...) {
  __builtin_va_list args;

  __builtin_va_start(args, fmt);
  (void)text_vsnprintf(d->text + d->len, kDetailBytes - d->len, fmt, args);
  __builtin_va_end(args);
  while (d->text[d->len] != '\0') {
    d->len++;
  }
}

/* One result line: the test, its result in colour, then the details. */
static void selftestReport(selftest_ctx_t *ctx, const char *label,
                           selftest_result_t result, const char *fmt, ...) {
  static const char *const kResultNames[] = {"ok", "FAIL", "skipped"};
  static const unsigned char kResultColors[] = {kColorOk, kColorFail,
                                                kColorDefault};
  char details[kDetailBytes];
  __builtin_va_list args;

  text_set_cursor(0U, ctx->row++);
  text_set_color(kColorDefault);
  text_printf("%-20s", label);
  text_set_color(kResultColors[result]);
  text_printf("%-8s", kResultNames[result]);
  text_set_color(kColorDefault);
  __builtin_va_start(args, fmt);
  (void)text_vsnprintf(details, sizeof(details), fmt, args);
  __builtin_va_end(args);
  selftestDetails(ctx, details);

  if (result == kResultOk) {
    ctx->passed++;
  } else if (result == kResultFail) {
    ctx->failed++;
  } else {
    ctx->skipped++;
  }
}

/* More details for the line above, on a row of their own. */
static void selftestMore(selftest_ctx_t *ctx, const char *fmt, ...) {
  char details[kDetailBytes];
  __builtin_va_list args;

  text_set_cursor(kDetailColumn, ctx->row++);
  __builtin_va_start(args, fmt);
  (void)text_vsnprintf(details, sizeof(details), fmt, args);
  __builtin_va_end(args);
  selftestDetails(ctx, details);
}

/* The address-line test (STORY-03). The layout mirrors scripts/finalize_rom.py,
   address_layout(), which writes the signatures into the image's free space:
   for line An a reference word and a partner word whose offsets differ only in
   that line. A stuck line makes one read return the other's signature. */
enum {
  kAddrRefSignature = 0x5AA5U,
  kAddrLowRefSignature = 0x5A5AU,
  kAddrPartnerSignature = 0xA500U,
  kAddrMaxReported = 4U
};

static unsigned short selftestAddressLineCount(unsigned long size) {
  unsigned long top = size - 1UL;
  unsigned short bits = 0U;

  while (top != 0UL) {
    bits++;
    top >>= 1;
  }
  return (unsigned short)(bits - 1U); /* no A0 on the 68000 */
}

static void selftestAddressLayout(unsigned long size, unsigned short n,
                                  unsigned long *ref, unsigned short *ref_sig,
                                  unsigned long *partner) {
  *ref = size - 0x100UL;
  *ref_sig = kAddrRefSignature;
  *partner = *ref ^ (1UL << n);
  if (*partner >= size) {
    /* Only the ST's 192 KB window, line A16: a second reference low in the
       image. */
    *ref = (size - 0x200UL) & 0xFFFFUL;
    *ref_sig = kAddrLowRefSignature;
    *partner = *ref ^ (1UL << n);
  }
}

static void selftestAddress(selftest_ctx_t *ctx) {
  static const char *const kStatus[] = {"ok", "stuck", "wrong"};
  const unsigned long base = ctx->env->rom_base;
  const unsigned long size = ctx->env->rom_size;
  const unsigned short lines = selftestAddressLineCount(size);
  unsigned char status[32];
  unsigned short failures = 0U;
  unsigned short n;

  for (n = 1U; n <= lines; ++n) {
    unsigned long ref;
    unsigned long partner;
    unsigned short ref_sig;
    const unsigned short partner_sig =
        (unsigned short)(kAddrPartnerSignature | n);
    unsigned short ref_read;
    unsigned short partner_read;

    selftestAddressLayout(size, n, &ref, &ref_sig, &partner);
    ref_read = *(const volatile unsigned short *)(base + ref);
    partner_read = *(const volatile unsigned short *)(base + partner);
    if (ref_read == ref_sig && partner_read == partner_sig) {
      status[n] = 0U;
    } else if (partner_read == ref_sig || ref_read == partner_sig) {
      status[n] = 1U;
    } else {
      status[n] = 2U;
    }
    if (status[n] != 0U) {
      failures++;
    }
    TRACE("selftest addr A%u %s", (unsigned int)n, kStatus[status[n]]);
  }

  if (failures == 0U) {
    selftestReport(ctx, "Address lines", kResultOk, "A1-A%u", (unsigned int)lines);
    return;
  }
  {
    detail_t d;

    detailInit(&d);
    failures = 0U;
    for (n = 1U; n <= lines; ++n) {
      if (status[n] != 0U) {
        if (failures++ == kAddrMaxReported) {
          detailAdd(&d, "...");
          break;
        }
        detailAdd(&d, "A%u %s ", (unsigned int)n, kStatus[status[n]]);
      }
    }
    selftestReport(ctx, "Address lines", kResultFail, "%s", d.text);
  }
}

/* The data-line test (STORY-04): 16 walking-one words then 16 walking-zero
   words, 4 KB from the end of the image, written by scripts/finalize_rom.py,
   data_patterns(). Read as words (both byte lanes) and as bytes (UDS for the
   even byte, LDS for the odd one). */
enum { kDataBlockFromEnd = 0x1000U, kDataZerosOffset = 32U, kDataLines = 16U };

static void selftestDataLines(detail_t *d, const char *what,
                              unsigned short mask) {
  unsigned short k;

  if (mask == 0U) {
    return;
  }
  detailAdd(d, "%s:", what);
  for (k = 0U; k < kDataLines; ++k) {
    if ((mask & (1U << k)) != 0U) {
      detailAdd(d, " D%u", (unsigned int)k);
    }
  }
  detailAdd(d, " ");
}

static void selftestData(selftest_ctx_t *ctx) {
  const unsigned long block =
      ctx->env->rom_base + ctx->env->rom_size - kDataBlockFromEnd;
  unsigned short stuck0 = 0U;
  unsigned short stuck1 = 0U;
  unsigned short extra = 0U;
  unsigned short lanes = 0U; /* 1: the upper byte (UDS), 2: the lower (LDS) */
  unsigned short k;

  for (k = 0U; k < kDataLines; ++k) {
    const unsigned short one = (unsigned short)(1U << k);
    const unsigned long at_one = block + 2UL * k;
    const unsigned long at_zero = block + kDataZerosOffset + 2UL * k;
    const unsigned short w1 = *(const volatile unsigned short *)at_one;
    const unsigned short w0 = *(const volatile unsigned short *)at_zero;
    const unsigned char hi1 = *(const volatile unsigned char *)at_one;
    const unsigned char lo1 = *(const volatile unsigned char *)(at_one + 1UL);
    const unsigned char hi0 = *(const volatile unsigned char *)at_zero;
    const unsigned char lo0 = *(const volatile unsigned char *)(at_zero + 1UL);

    if ((w1 & one) == 0U) {
      stuck0 = (unsigned short)(stuck0 | one);
    }
    extra = (unsigned short)(extra | (w1 & (unsigned short)~one));
    if ((w0 & one) != 0U) {
      stuck1 = (unsigned short)(stuck1 | one);
    }
    if (hi1 != (unsigned char)(w1 >> 8) || hi0 != (unsigned char)(w0 >> 8)) {
      lanes |= 1U;
    }
    if (lo1 != (unsigned char)w1 || lo0 != (unsigned char)w0) {
      lanes |= 2U;
    }
  }
  /* A line stuck at 1 shows in every walking-one word: not a short. */
  extra = (unsigned short)(extra & (unsigned short)~stuck1);

  if ((stuck0 | stuck1 | extra | lanes) == 0U) {
    TRACE("selftest data ok");
    selftestReport(ctx, "Data lines", kResultOk, "D0-D15, both byte lanes");
    return;
  }
  TRACE("selftest data stuck0=%04x stuck1=%04x shorted=%04x lanes=%u",
        (unsigned int)stuck0, (unsigned int)stuck1, (unsigned int)extra,
        (unsigned int)lanes);
  {
    detail_t d;

    detailInit(&d);
    selftestDataLines(&d, "stuck at 0", stuck0);
    selftestDataLines(&d, "stuck at 1", stuck1);
    selftestDataLines(&d, "shorted", extra);
    if ((lanes & 1U) != 0U) {
      detailAdd(&d, "upper byte lane ");
    }
    if ((lanes & 2U) != 0U) {
      detailAdd(&d, "lower byte lane");
    }
    selftestReport(ctx, "Data lines", kResultFail, "%s", d.text);
  }
}

/* Stress reads (STORY-11). The build fills the unused space with
   selftestFillWord(offset) (scripts/finalize_rom.py, fill_word()), so any read
   in the stress region can be checked: random addresses, each read as a word,
   two bytes and a long, plus the address with every line flipped. */
enum {
  kStressStart = 0x10000,
  kStressTopGap = 0x1000,
  kStressQuickSteps = 4000,
  kSoakSeconds = 30,
  kSoakKeyPollSteps = 32,
  kStressMaxReported = 6
};

typedef struct {
  unsigned long base;
  unsigned long lo;
  unsigned long hi;
  unsigned long mask; /* every address line of the window */
  unsigned long ref;
  unsigned long ref2;
  unsigned long kickety;
  unsigned char has_ref2;
  unsigned short lines;
  unsigned long state; /* xorshift32 of the addresses */
  unsigned long reads;
  unsigned long errors;
  unsigned long data_count[16]; /* errors blamed on data line Dk */
  unsigned long addr_count[20]; /* errors blamed on address line An */
  unsigned short data;          /* the lines behind at least 1/64 of the */
  unsigned long addr;           /* errors (stressBlame()): bit k, bit n */
} stress_t;

/* Mirrors scripts/finalize_rom.py, fill_word(): shifts and additions, not
   linear, so a word one address line away differs in a different way at every
   address. */
static unsigned short selftestFillWord(unsigned long offset) {
  unsigned long x = offset ^ 0x9E3779B9UL;

  x += x << 10;
  x ^= x >> 6;
  x += x << 3;
  x ^= x >> 11;
  x += x << 15;
  return (unsigned short)(x ^ (x >> 16));
}

/* The self-test's own words: a reference and every offset one address line
   away from it (its partners), and the Amiga's kickety split at the middle. */
static int stressReserved(const stress_t *st, unsigned long off) {
  unsigned long x = off ^ st->ref;

  if ((x & (x - 1UL)) == 0UL) {
    return 1;
  }
  if (st->has_ref2) {
    x = off ^ st->ref2;
    if ((x & (x - 1UL)) == 0UL) {
      return 1;
    }
  }
  return (off >= st->kickety && off < st->kickety + 8UL) ? 1 : 0;
}

static void stressInit(stress_t *st, const selftest_env_t *env) {
  const unsigned long size = env->rom_size;
  unsigned short i;

  st->base = env->rom_base;
  st->lo = kStressStart;
  st->hi = size - kStressTopGap;
  st->lines = selftestAddressLineCount(size);
  st->mask = (1UL << (st->lines + 1U)) - 1UL;
  st->ref = size - 0x100UL;
  /* A window that is not a power of two (the ST's 192 KB) has the second
     reference of the address test (selftestAddressLayout()). */
  st->has_ref2 = (unsigned char)((size & (size - 1UL)) != 0UL);
  st->ref2 = (size - 0x200UL) & 0xFFFFUL;
  st->kickety = size >> 1;
  st->state = 0x2545F491UL;
  st->reads = 0UL;
  st->errors = 0UL;
  st->data = 0U;
  st->addr = 0UL;
  for (i = 0U; i < 16U; ++i) {
    st->data_count[i] = 0UL;
  }
  for (i = 0U; i < 20U; ++i) {
    st->addr_count[i] = 0UL;
  }
}

/* A mismatch can match another address's word by chance (16-bit words: about
   once in 4,000 errors), so a line is blamed only when it accounts for at
   least 1/64 of the errors. */
static void stressBlame(stress_t *st) {
  const unsigned long floor = st->errors >> 6;
  unsigned short n;

  st->data = 0U;
  st->addr = 0UL;
  for (n = 0U; n < 16U; ++n) {
    if (st->data_count[n] != 0UL && st->data_count[n] >= floor) {
      st->data = (unsigned short)(st->data | (1U << n));
    }
  }
  for (n = 1U; n <= st->lines; ++n) {
    if (st->addr_count[n] != 0UL && st->addr_count[n] >= floor) {
      st->addr |= 1UL << n;
    }
  }
}

/* lane: the byte lanes the read drove, 0xFFFF for a word, 0xFF00 for the even
   byte (UDS), 0x00FF for the odd one (LDS); only those bits are compared. */
static void stressCheck(stress_t *st, unsigned long off, unsigned short got,
                        unsigned short expected, unsigned short lane) {
  const unsigned short diff = (unsigned short)((got ^ expected) & lane);
  unsigned short n;

  if (diff == 0U) {
    return;
  }
  st->errors++;
  /* One bit off: a data line. Otherwise, the word of an address one line
     away: that address line. */
  if ((diff & (diff - 1U)) != 0U) {
    for (n = 1U; n <= st->lines; ++n) {
      if (((got ^ selftestFillWord(off ^ (1UL << n))) & lane) == 0U) {
        st->addr_count[n]++;
        return;
      }
    }
  }
  for (n = 0U; n < 16U; ++n) {
    if ((diff & (1U << n)) != 0U) {
      st->data_count[n]++;
    }
  }
}

static void stressStep(stress_t *st) {
  unsigned long r = st->state;
  unsigned long off;
  unsigned long complement;

  r ^= r << 13;
  r ^= r >> 17;
  r ^= r << 5;
  st->state = r;
  off = r & st->mask & ~1UL;
  if (off >= st->lo && off + 4UL <= st->hi && !stressReserved(st, off) &&
      !stressReserved(st, off + 2UL)) {
    const unsigned long at = st->base + off;
    const unsigned short e0 = selftestFillWord(off);
    const unsigned short e1 = selftestFillWord(off + 2UL);
    const unsigned short word = *(const volatile unsigned short *)at;
    const unsigned char upper = *(const volatile unsigned char *)at;
    const unsigned char lower = *(const volatile unsigned char *)(at + 1UL);
    const unsigned long both = *(const volatile unsigned long *)at;

    stressCheck(st, off, word, e0, 0xFFFFU);
    stressCheck(st, off, (unsigned short)(upper << 8), e0, 0xFF00U);
    stressCheck(st, off, (unsigned short)lower, e0, 0x00FFU);
    stressCheck(st, off, (unsigned short)(both >> 16), e0, 0xFFFFU);
    stressCheck(st, off + 2UL, (unsigned short)both, e1, 0xFFFFU);
    st->reads += 5UL; /* a word, two bytes, a long of two words */
  }
  /* Every address line of the window flipped at once. */
  complement = (off ^ st->mask) & ~1UL;
  if (complement >= st->lo && complement + 2UL <= st->hi &&
      !stressReserved(st, complement)) {
    stressCheck(st, complement,
                *(const volatile unsigned short *)(st->base + complement),
                selftestFillWord(complement), 0xFFFFU);
    st->reads++;
  }
}

/* The lines at fault, after the details: "D3 x120 A9 x4 ..." */
static void stressLines(const stress_t *st, detail_t *d) {
  unsigned short shown = 0U;
  unsigned short n;

  for (n = 0U; n < 16U && shown < kStressMaxReported; ++n) {
    if ((st->data & (1U << n)) != 0U) {
      detailAdd(d, " D%u x%lu", (unsigned int)n, st->data_count[n]);
      shown++;
    }
  }
  for (n = 1U; n <= st->lines && shown < kStressMaxReported; ++n) {
    if ((st->addr & (1UL << n)) != 0UL) {
      detailAdd(d, " A%u x%lu", (unsigned int)n, st->addr_count[n]);
      shown++;
    }
  }
}

static void selftestStress(selftest_ctx_t *ctx) {
  stress_t st;
  unsigned short i;

  stressInit(&st, ctx->env);
  for (i = 0U; i < kStressQuickSteps; ++i) {
    stressStep(&st);
  }
  stressBlame(&st);
  TRACE("selftest stress reads=%lu errors=%lu data=%04x addr=%05lx", st.reads,
        st.errors, (unsigned int)st.data, st.addr);
  if (st.errors == 0UL) {
    selftestReport(ctx, "Stress reads", kResultOk, "%lu reads, no error", st.reads);
    return;
  }
  {
    detail_t d;

    detailInit(&d);
    detailAdd(&d, "%lu errors in %lu reads:", st.errors, st.reads);
    stressLines(&st, &d);
    selftestReport(ctx, "Stress reads", kResultFail, "%s", d.text);
  }
}

/* The 30-second soak (S on the self-test screen): the same steps, timed in
   video frames, with live counts; ESC stops it early. */
static void selftestSoak(const selftest_env_t *env, unsigned short row) {
  selftest_ctx_t ctx; /* for the result line only: its counts are dropped */
  stress_t st;
  const unsigned short rate = platform_frame_rate();
  const unsigned long start = platform_frames();
  unsigned long total = 0UL;
  unsigned long next_second;
  unsigned short seconds = 0U;
  unsigned short steps = 0U;
  unsigned char stopped = 0U;
  unsigned short i;

  for (i = 0U; i < kSoakSeconds; ++i) {
    total += rate;
  }
  next_second = rate;
  stressInit(&st, env);
  TRACE("soak start");
  for (i = 0U; i < 3U; ++i) { /* the last result may have taken three rows */
    text_set_cursor(0U, (unsigned short)(row + i));
    text_printf("\x1BK");
  }
  for (;;) {
    unsigned long frames;

    stressStep(&st);
    frames = platform_frames() - start;
    if (frames >= next_second) {
      seconds++;
      next_second += rate;
      text_set_cursor(0U, row);
      /* At most 80 columns, even with ten-digit counts. */
      text_printf("%-28s%u s, %lu reads, %lu errors, ESC stops", "Soak test",
                  (unsigned int)seconds, st.reads, st.errors);
    }
    if (frames >= total) {
      break;
    }
    if (++steps == kSoakKeyPollSteps) {
      steps = 0U;
      if (kbd_poll_scancode() == KEY_ESC) {
        stopped = 1U;
        break;
      }
    }
  }
  stressBlame(&st);
  TRACE("soak end seconds=%u reads=%lu errors=%lu data=%04x addr=%05lx stopped=%u",
        (unsigned int)seconds, st.reads, st.errors, (unsigned int)st.data,
        st.addr, (unsigned int)stopped);
  text_set_cursor(0U, row);
  text_printf("\x1BK");
  ctx.env = env;
  ctx.row = row;
  ctx.passed = 0U;
  ctx.failed = 0U;
  ctx.skipped = 0U;
  ctx.device_answers = 0U;
  {
    detail_t d;

    detailInit(&d);
    detailAdd(&d, "%u s%s, %lu reads, %lu errors", (unsigned int)seconds,
              stopped ? " (stopped)" : "", st.reads, st.errors);
    stressLines(&st, &d);
    selftestReport(&ctx, "Soak test",
                   st.errors == 0UL ? kResultOk : kResultFail, "%s", d.text);
  }
}

/* The command test (STORY-05): pings, each restored. The result decides
   whether the tests that need the device run. */
enum { kPingCount = 8U };

static void selftestPing(selftest_ctx_t *ctx) {
  static const char *const kNames[] = {"answered", "noanswer", "checksum",
                                       "norestore"};
  unsigned short answered = 0U;
  (void)kNames; /* used by the trace only, which release builds compile out */
  unsigned short counts[4] = {0U, 0U, 0U, 0U};
  unsigned short i;

  for (i = 0U; i < kPingCount; ++i) {
    const int result = command_ping();
    const unsigned short kind = (unsigned short)((result == 0) ? 0 : -result);

    counts[kind]++;
    if (kind == 0U) {
      answered++;
    }
    TRACE("selftest ping %u %s", (unsigned int)(i + 1U), kNames[kind]);
  }
  TRACE("selftest ping answered=%u sent=%u", (unsigned int)answered,
        (unsigned int)kPingCount);
  ctx->device_answers = (unsigned char)(answered > 0U);

#if defined(_TEST) && (_TEST > 0)
  /* A test build runs in Hatari and FS-UAE, where no device answers (C-03):
     the pings still go out, as the harness checks, but are not a failure. */
  if (answered == 0U && counts[1] == kPingCount) {
    selftestReport(ctx, "Ping", kResultSkipped,
                   "test build: %u sent, no device to answer",
                   (unsigned int)kPingCount);
    return;
  }
#endif
  if (answered == kPingCount) {
    selftestReport(ctx, "Ping", kResultOk, "%u of %u answered",
                   (unsigned int)answered, (unsigned int)kPingCount);
  } else if (answered == 0U && counts[1] == kPingCount) {
    selftestReport(ctx, "Ping", kResultFail,
                   "no answer: the device does not take commands");
  } else {
    selftestReport(ctx, "Ping", kResultFail,
                   "%u of %u answered, %u no answer, %u checksum, %u restore",
                   (unsigned int)answered, (unsigned int)kPingCount,
                   (unsigned int)counts[1], (unsigned int)counts[2],
                   (unsigned int)counts[3]);
  }
}

/* Command reliability (STORY-06): many pings in a row when the device answers,
   long enough to see the 0.6 % frame loss firmware EPIC-06 measured under TOS;
   here the frames go out with interrupts masked. */
enum { kReliabilityCount = 1000U, kReliabilityProgressStep = 100U };

static void selftestReliability(selftest_ctx_t *ctx) {
  unsigned short counts[4] = {0U, 0U, 0U, 0U};
  unsigned short i;
  const unsigned short row = ctx->row;

  if (!ctx->device_answers) {
    TRACE("selftest reliability skipped");
    selftestReport(ctx, "Command reliability", kResultSkipped,
                   "the device does not answer");
    return;
  }
  for (i = 0U; i < kReliabilityCount; ++i) {
    const int result = command_ping();
    counts[(result == 0) ? 0 : -result]++;
    if ((i % kReliabilityProgressStep) == 0U) {
      text_set_cursor(0U, row);
      text_printf("Command reliability     %u/%u", (unsigned int)i,
                  (unsigned int)kReliabilityCount);
    }
  }
  TRACE("selftest reliability answered=%u noanswer=%u checksum=%u norestore=%u",
        (unsigned int)counts[0], (unsigned int)counts[1],
        (unsigned int)counts[2], (unsigned int)counts[3]);
  text_set_cursor(0U, row);
  text_printf("\x1BK");
  selftestReport(ctx, "Command reliability",
                 (counts[0] == kReliabilityCount) ? kResultOk : kResultFail,
                 "%u of %u answered, %u lost, %u checksum, %u restore",
                 (unsigned int)counts[0], (unsigned int)kReliabilityCount,
                 (unsigned int)counts[1], (unsigned int)counts[2],
                 (unsigned int)counts[3]);
}

/* Flash read consistency (STORY-07): the parameters page and the catalog pages
   that hold entries, read again kFlashRounds times and compared byte for byte
   with what the chooser loaded at start-up. */
enum {
  kFlashRounds = 3U,
  kFlashPageBytes = 4096U,
  kCatalogRecordBytes = 256U,
  kFlashEndianBig = 1U
};

static unsigned short selftestCompare(const unsigned char *a,
                                      const unsigned char *b) {
  unsigned short differences = 0U;
  unsigned short i;

  for (i = 0U; i < kFlashPageBytes; ++i) {
    if (a[i] != b[i]) {
      differences++;
    }
  }
  return differences;
}

static void selftestFlash(selftest_ctx_t *ctx) {
  static unsigned char *page = (unsigned char *)0; /* allocated once */
  const unsigned short catalog_pages = (unsigned short)(
      ((unsigned long)ctx->env->entries * kCatalogRecordBytes + kFlashPageBytes) /
      kFlashPageBytes);
  const unsigned long retries_before = command_block_retries();
  unsigned long differences = 0UL;
  unsigned short errors = 0U;
  unsigned short round;
  unsigned short p;

  if (!ctx->device_answers) {
    TRACE("selftest flash skipped");
    selftestReport(ctx, "Flash reads", kResultSkipped,
                   "the device does not answer");
    return;
  }
  if (page == (unsigned char *)0) {
    page = (unsigned char *)pa_alloc_aligned(kFlashPageBytes, 16UL);
  }
  if (page == (unsigned char *)0) {
    selftestReport(ctx, "Flash reads", kResultFail, "no memory for a page");
    return;
  }
  for (round = 0U; round < kFlashRounds; ++round) {
    if (read_flash_page(page, FLASH_PARAMS_START, kFlashEndianBig) != 0) {
      errors++;
    } else {
      differences += selftestCompare(page, ctx->env->params);
    }
    for (p = 0U; p < catalog_pages; ++p) {
      if (read_flash_page(page, FLASH_CATALOG_START + (unsigned long)p * kFlashPageBytes,
                          kFlashEndianBig) != 0) {
        errors++;
      } else {
        differences += selftestCompare(
            page, ctx->env->catalog + (unsigned long)p * kFlashPageBytes);
      }
    }
  }
  TRACE("selftest flash pages=%u rounds=%u differences=%lu errors=%u retries=%lu",
        (unsigned int)(catalog_pages + 1U), (unsigned int)kFlashRounds,
        differences, (unsigned int)errors,
        command_block_retries() - retries_before);
  selftestReport(ctx, "Flash reads",
                 (differences == 0UL && errors == 0U) ? kResultOk : kResultFail,
                 "%u pages x %u, %lu bytes differ, %u failed, %lu retries",
                 (unsigned int)(catalog_pages + 1U), (unsigned int)kFlashRounds,
                 differences, (unsigned int)errors,
                 command_block_retries() - retries_before);
}

/* Catalog and parameters (STORY-08): what the chooser loaded, checked, and the
   CONFIG.TXT values stored in the device, decoded as sidecartos-config's
   show_config_page() does. Offsets from the firmware's parameters_t (C-04). */
enum {
  kProtocolExpected = 0x0040U,
  kParamsVersion = 512U,
  kParamsTicks = 516U,
  kParamsUseOe = 520U,
  kParamsBusFlags = 524U,
  kParamsRescueTimeout = 528U,
  kParamsRomFlags = 532U,
  kParamsDefaultIndex = 70U,
  kParamsRescueIndex = 326U,
  kCatalogMaxEntries = 64U,
  kCatalogNameBytes = 64U,
  kCatalogBlocksOffset = 64U,
  kCatalogMaxBlocks = 256U, /* 4 KB blocks: the 1 MB the firmware serves at most */
  kRescueTimeoutMax = 600U,
  kBusFlagCeSyncBypass = 0x1U,
  kBusFlagWriteOnDrive = 0x2U,
  kBusFlagDmaHighPriority = 0x4U,
  kRomFlagLargeRoms = 0x1U,
  kRomFlagAcceleratedBus = 0x2U
};

static unsigned long selftestParamsU32(const unsigned char *params,
                                       unsigned short at) {
  return (unsigned long)params[at] | ((unsigned long)params[at + 1U] << 8) |
         ((unsigned long)params[at + 2U] << 16) |
         ((unsigned long)params[at + 3U] << 24);
}

static void selftestCatalog(selftest_ctx_t *ctx) {
  const unsigned char *params = ctx->env->params;
  const unsigned char *catalog = ctx->env->catalog;
  const int entries = ctx->env->entries;
  const unsigned short protocol = (unsigned short)(
      params[kParamsVersion] | (params[kParamsVersion + 1U] << 8));
  const unsigned short default_index = (unsigned short)(
      params[kParamsDefaultIndex] | (params[kParamsDefaultIndex + 1U] << 8));
  const unsigned short rescue_index = (unsigned short)(
      params[kParamsRescueIndex] | (params[kParamsRescueIndex + 1U] << 8));
  unsigned short bad_names = 0U;
  unsigned short bad_sizes = 0U;
  int i;

  for (i = 0; i < entries && i < (int)kCatalogMaxEntries; ++i) {
    const unsigned char *record = catalog + (unsigned long)i * kCatalogRecordBytes;
    /* Little-endian, as the chooser's size column reads it (its
       BIG_ENDIAN_TO_LITTLE_ENDIAN_WORD on the two bytes). */
    const unsigned short blocks = (unsigned short)(
        record[kCatalogBlocksOffset] | (record[kCatalogBlocksOffset + 1U] << 8));
    unsigned short c;

    if (record[0] == 0U) {
      bad_names++;
    }
    for (c = 0U; c < kCatalogNameBytes && record[c] != 0U; ++c) {
      if (record[c] < 0x20U || record[c] > 0x7EU) {
        bad_names++;
        break;
      }
    }
    if (blocks == 0U || blocks > kCatalogMaxBlocks) {
      bad_sizes++;
    }
  }
  {
    const int ok = protocol == kProtocolExpected && entries >= 1 &&
                   entries <= (int)kCatalogMaxEntries &&
                   default_index < (unsigned short)entries &&
                   rescue_index < (unsigned short)entries && bad_names == 0U &&
                   bad_sizes == 0U;
    TRACE("selftest catalog %s entries=%d protocol=0x%04x default=%u rescue=%u "
          "badnames=%u badsizes=%u",
          ok ? "ok" : "fail", entries, (unsigned int)protocol,
          (unsigned int)default_index, (unsigned int)rescue_index,
          (unsigned int)bad_names, (unsigned int)bad_sizes);
    selftestReport(ctx, "Catalog", ok ? kResultOk : kResultFail,
                   "%d entries, protocol 0x%04x, default %u, rescue %u%s",
                   entries, (unsigned int)protocol, (unsigned int)default_index,
                   (unsigned int)rescue_index,
                   (bad_names != 0U || bad_sizes != 0U) ? ", bad records" : "");
  }
}

static void selftestConfig(selftest_ctx_t *ctx) {
  const unsigned char *params = ctx->env->params;
  const unsigned long ticks = selftestParamsU32(params, kParamsTicks);
  const unsigned long use_oe = selftestParamsU32(params, kParamsUseOe);
  unsigned long bus = selftestParamsU32(params, kParamsBusFlags);
  unsigned long rescue = selftestParamsU32(params, kParamsRescueTimeout);
  unsigned long rom = selftestParamsU32(params, kParamsRomFlags);

  /* As the emulator reads them: an erased word is no flags, a timeout above
     600 s is ignored as corrupt. */
  if (bus == 0xFFFFFFFFUL) {
    bus = 0UL;
  }
  if (rom == 0xFFFFFFFFUL) {
    rom = 0UL;
  }
  if (rescue > kRescueTimeoutMax) {
    rescue = 0UL;
  }
  TRACE("selftest config ticks=%lu oe=%u busflags=0x%lx rescue=%lu romflags=0x%lx",
        ticks, (unsigned int)(use_oe != 0UL), bus, rescue, rom);
  selftestReport(ctx, "Configuration", kResultOk,
                 "READ_BUS_TICKS %lu, USE_OE %u, RESCUE_TIMEOUT %lu s", ticks,
                 (unsigned int)(use_oe != 0UL), rescue);
  selftestMore(ctx, "CE_SYNC_BYPASS %u, WRITE_ON_DRIVE %u",
               (unsigned int)((bus & kBusFlagCeSyncBypass) != 0UL),
               (unsigned int)((bus & kBusFlagWriteOnDrive) != 0UL));
  selftestMore(ctx, "DMA_HIGH_PRIORITY %u, LARGE_ROMS %u, ACCELERATED_BUS %u",
               (unsigned int)((bus & kBusFlagDmaHighPriority) != 0UL),
               (unsigned int)((rom & kRomFlagLargeRoms) != 0UL),
               (unsigned int)((rom & kRomFlagAcceleratedBus) != 0UL));
}

/* Machine information (STORY-09): what the platform probed. */
static void selftestMachine(selftest_ctx_t *ctx) {
  platform_machine_t machine;

  platform_probe_machine(&machine);
  TRACE("selftest machine %s chip=0x%02x denise=0x%02x", machine.name,
        (unsigned int)machine.chip_id, (unsigned int)machine.denise_id);
  if (machine.chip_id != 0U || machine.denise_id != 0U) {
    selftestReport(ctx, "Machine", kResultOk, "%s, %s (ID 0x%02x/0x%02x)",
                   machine.label, machine.detail,
                   (unsigned int)machine.chip_id,
                   (unsigned int)machine.denise_id);
  } else {
    selftestReport(ctx, "Machine", kResultOk, "%s, %s", machine.label,
                   machine.detail);
  }
  if (machine.has_monitor_line) {
    TRACE("selftest monitor %s changes=%u samples=%u",
          machine.monitor_mono ? "mono" : "colour",
          (unsigned int)machine.monitor_changes,
          (unsigned int)machine.monitor_samples);
    selftestReport(ctx, "Monitor line",
                   (machine.monitor_changes == 0U) ? kResultOk : kResultFail,
                   "%s, %s over %u samples",
                   machine.monitor_mono ? "mono" : "colour",
                   (machine.monitor_changes == 0U) ? "stable" : "changing",
                   (unsigned int)machine.monitor_samples);
  }
}

/* The image itself (STORY-13): its size is the ROM window the machine model
   gives the cartridge, 192, 256 or 512 KB. */
static void selftestImage(selftest_ctx_t *ctx) {
  const unsigned long kb = ctx->env->rom_size >> 10;

  TRACE("selftest image size=%lu base=%06lx", kb, ctx->env->rom_base);
  selftestReport(ctx, "Rescue image", kResultOk,
                 "%lu KB at 0x%06lX, v%s, build %s", kb, ctx->env->rom_base,
                 APP_VERSION_STR, BUILD_ID_STR);
}

/* The RAM (STORY-13), as the platform measured it: the ST's two banks, the
   Amiga's chip and slow RAM. */
static void selftestRam(selftest_ctx_t *ctx) {
  platform_memory_t mem;

  platform_probe_memory(&mem);
  TRACE("selftest ram total=%lu parts=%lu+%lu", mem.part_kb[0] + mem.part_kb[1],
        mem.part_kb[0], mem.part_kb[1]);
  selftestReport(ctx, "RAM", kResultOk, "%lu KB: %s %lu KB, %s %lu KB",
                 mem.part_kb[0] + mem.part_kb[1], mem.part_name[0],
                 mem.part_kb[0], mem.part_name[1], mem.part_kb[1]);
}

/* A full-width inverted title bar, as the ROM list has. */
static void selftestTitle(void) {
  static const char kTitle[] = "SidecarTridge device self-test";
  char line[SCR_WIDTH_CHARS + 1];
  unsigned short len = 0U;
  unsigned short i;
  unsigned short pad;

  while (kTitle[len] != '\0') {
    len++;
  }
  pad = (unsigned short)((SCR_WIDTH_CHARS - len) / 2U);
  for (i = 0U; i < SCR_WIDTH_CHARS; ++i) {
    line[i] = (i >= pad && i < pad + len) ? kTitle[i - pad] : ' ';
  }
  line[SCR_WIDTH_CHARS] = '\0';
  text_printf("\x1BY  \x1Bp%s\x1Bq", line);
}

void selftest_run(const selftest_env_t *env) {
  selftest_ctx_t ctx;
  unsigned short soak_row;

  ctx.env = env;
  ctx.row = kSelftestFirstRow;
  ctx.passed = 0U;
  ctx.failed = 0U;
  ctx.skipped = 0U;
  ctx.device_answers = 0U;

  text_clear();
  text_set_color(env->default_color);
  selftestTitle();
  TRACE("selftest start");

  selftestImage(&ctx);
  selftestAddress(&ctx);
  selftestData(&ctx);
  selftestStress(&ctx);
  selftestPing(&ctx);
  selftestReliability(&ctx);
  selftestFlash(&ctx);
  selftestCatalog(&ctx);
  selftestConfig(&ctx);
  selftestMachine(&ctx);
  selftestRam(&ctx);

  text_set_cursor(0U, (unsigned short)(ctx.row + 1U));
  text_printf("%u passed, %u failed, %u skipped.", (unsigned int)ctx.passed,
              (unsigned int)ctx.failed, (unsigned int)ctx.skipped);
  TRACE("selftest end passed=%u failed=%u skipped=%u", (unsigned int)ctx.passed,
        (unsigned int)ctx.failed, (unsigned int)ctx.skipped);
  text_set_cursor(0U, (unsigned short)(ctx.row + 2U));
  text_printf("S runs a 30-second soak test. ESC returns to the ROM list.");
  /* The soak's result line, with room for two more rows below it. */
  soak_row = (unsigned short)(ctx.row + 4U);
  if (soak_row > SCR_HEIGHT_LINES - 3U) {
    soak_row = SCR_HEIGHT_LINES - 3U;
  }

  for (;;) {
    unsigned char key;

    TRACE("waitkey");
    key = kbd_poll_scancode_wait();
    if (key == KEY_ESC) {
      return;
    }
    if (key == KEY_S) {
      selftestSoak(env, soak_row);
    }
  }
}
