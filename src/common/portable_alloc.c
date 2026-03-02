#include "portable_alloc.h"

enum { kDefAlign = 4U };

static unsigned long gHeapBeg = 0U;
static unsigned long gHeapEnd = 0U;
static unsigned long gHeapCur = 0U;

static unsigned long align_up(unsigned long v, unsigned long a) {
  if (a == 0U) {
    return v;
  }
  return (v + a - 1U) & ~(a - 1U);
}

static int is_pow2(unsigned long v) {
  return v != 0UL && (v & (v - 1UL)) == 0UL;
}

void pa_init(unsigned long base, unsigned long size) {
  if (size == 0UL || base > ((unsigned long)~(unsigned long)0) - (unsigned long)size) {
    gHeapBeg = 0U;
    gHeapEnd = 0U;
    gHeapCur = 0U;
    return;
  }

  gHeapBeg = base;
  gHeapEnd = base + (unsigned long)size;
  gHeapCur = base;
}

void *pa_alloc(unsigned long size) {
  unsigned long start;
  unsigned long end;

  if (size == 0UL || gHeapBeg == 0U || gHeapEnd <= gHeapBeg) {
    return (void *)0;
  }

  if (gHeapCur > ((unsigned long)~(unsigned long)0) - ((unsigned long)kDefAlign - 1U)) {
    return (void *)0;
  }
  start = align_up(gHeapCur, (unsigned long)kDefAlign);

  end = start + (unsigned long)size;
  if (end < start || end > gHeapEnd) {
    return (void *)0;
  }

  gHeapCur = end;
  return (void *)start;
}

void pa_free(void *ptr) { (void)ptr; }

void *pa_alloc_aligned(unsigned long size, unsigned long align) {
  unsigned long padded;
  void *raw;
  unsigned long aligned;

  if (size == 0UL || !is_pow2(align)) {
    return (void *)0;
  }

  if (size > (~0UL - (align - 1UL))) {
    return (void *)0;
  }

  padded = size + align - 1UL;
  raw = pa_alloc(padded);
  if (!raw) {
    return (void *)0;
  }

  aligned = align_up((unsigned long)raw, (unsigned long)align);
  return (void *)aligned;
}
