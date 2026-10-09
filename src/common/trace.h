/**
 * File: src/common/trace.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-09
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Debug-build trace lines for the emulator harnesses.
 */

#ifndef TRACE_H_
#define TRACE_H_

/*
 * TRACE("list %d entries", n) sends "@@ list 12 entries\n" to the platform's
 * trace channel (platform_trace_write): Hatari's native features on the ST,
 * Paula's serial port on the Amiga. The harnesses of tools/dev/ wait for these
 * lines (EPIC-00 STORY-02). Release builds compile every TRACE() to nothing.
 */
#if defined(_DEBUG) && (_DEBUG > 0)
void trace_line(const char *fmt, ...);
#define TRACE(...) trace_line(__VA_ARGS__)
#else
#define TRACE(...) ((void)0)
#endif

#endif /* TRACE_H_ */
