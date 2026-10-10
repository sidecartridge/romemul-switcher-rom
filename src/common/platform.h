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
/* The size of the image the device serves: 192, 256 or 512 KB. */
unsigned long platform_rom_image_size(void);

/* What the self-test says about the machine (EPIC-03 STORY-09). */
typedef struct {
  const char *name;   /* in the trace: st, megast, ste, megaste, amiga */
  const char *label;  /* on the screen: "Atari Mega STE", "Amiga" */
  const char *detail; /* what told them apart, or the chipset */
  unsigned short chip_id;          /* Amiga: VPOSR's Agnus ID; Atari: 0 */
  unsigned short denise_id;        /* Amiga: DENISEID's low byte; Atari: 0 */
  unsigned char has_monitor_line;  /* Atari: the MFP's monitor-detect line */
  unsigned char monitor_mono;      /* its last sample: 1 for mono */
  unsigned short monitor_changes;  /* how often it changed between samples */
  unsigned short monitor_samples;
} platform_machine_t;

void platform_probe_machine(platform_machine_t *out);

/* The RAM the self-test shows (EPIC-03 STORY-13), in KB, in two parts: the
   ST's banks 0 and 1, or the Amiga's chip and slow RAM. */
typedef struct {
  unsigned long part_kb[2];
  const char *part_name[2]; /* "bank 0" and "bank 1", "chip" and "slow" */
} platform_memory_t;

void platform_probe_memory(platform_memory_t *out);

/* A clock for the soak test (EPIC-03 STORY-11), with interrupts masked: video
   frames counted by watching the video counter (Atari) or the beam (Amiga).
   platform_frames() must be called at least a few times a frame. */
unsigned long platform_frames(void);
unsigned short platform_frame_rate(void);

#if defined(_DEBUG) && (_DEBUG > 0)
/* Debug builds only: the channel the harnesses read (EPIC-00 STORY-02).
   Hatari's NF_STDERR on the ST, Paula's serial port on the Amiga. */
void platform_trace_init(void);
void platform_trace_write(const char *text);
#endif
