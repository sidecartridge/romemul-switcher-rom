/**
 * File: src/common/platform.h
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Shared platform abstraction declarations.
 */

#pragma once

unsigned long platform_get_system_time_seed(void);
unsigned long platform_send_magic_sequence(unsigned long rom_base_address,
                                           unsigned short command,
                                           unsigned short param);
void platform_poll(void);
void platform_hard_reset(void);

#if defined(_DEBUG) && (_DEBUG > 0)
/* Debug builds only: the channel the harnesses read (EPIC-00 STORY-02).
   Hatari's NF_STDERR on the ST, Paula's serial port on the Amiga. */
void platform_trace_init(void);
void platform_trace_write(const char *text);
#endif
