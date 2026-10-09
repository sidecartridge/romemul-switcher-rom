/**
 * File: src/common/trace.c
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-09
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Debug-build trace lines for the emulator harnesses.
 */

#include "trace.h"

#if defined(_DEBUG) && (_DEBUG > 0)

#include "platform.h"
#include "text.h"

enum { kTraceLineSize = 128 };

void trace_line(const char *fmt, ...) {
  static const char kPrefix[] = "@@ ";
  char line[kTraceLineSize];
  __builtin_va_list args;
  unsigned long len = 0UL;

  while (kPrefix[len] != '\0') {
    line[len] = kPrefix[len];
    len++;
  }

  __builtin_va_start(args, fmt);
  (void)text_vsnprintf(line + len, (unsigned long)kTraceLineSize - len - 1UL,
                       fmt, args);
  __builtin_va_end(args);

  while (line[len] != '\0') {
    len++;
  }
  line[len++] = '\n';
  line[len] = '\0';
  platform_trace_write(line);
}

#endif
