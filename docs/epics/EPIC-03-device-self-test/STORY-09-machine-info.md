---
id: STORY-09
epic: EPIC-03
title: Machine information
status: done
---

## Goal

The self-test says which machine it runs on and what it sees of it, with no OS to ask.

## Tasks

- [x] Atari: ST, Mega ST, STE or Mega STE, by probing hardware with a temporary bus-error
      handler (for example the STE's DMA sound registers, the Mega ST's real-time clock, the
      Mega STE's cache register); the probes and the handler recorded here
- [x] Atari: the monitor-detect line (MFP GPIP bit 7) sampled over about a second, reported as
      mono or colour and whether it changed, which bears on the mono cold-boot candidate
- [x] Amiga: the Agnus revision and PAL or NTSC from `VPOSR`, and what else the chipset tells
      without an OS
- [x] `@@ selftest machine ...` lines; harness: the machine Hatari or FS-UAE was started as

## Acceptance

The right machine named for every machine the harnesses run (ST, Mega ST, STE, Mega STE, A500).

## Notes

**Done 2026-10-09.** `platform_probe_machine()` on both platforms fills a `platform_machine_t`
(`platform.h`); the self-test shows "Machine" and, on the Atari, "Monitor line".

- Atari probes, each a byte read under a temporary bus-error handler (`src/st/probe.s`,
  `st_probe_read()`: saves the vector at `0x8`, restores the stack on a fault): the STE's DMA
  sound control (`0xFF8901`), the Mega STE's cache control (`0xFF8E21`), the blitter
  (`0xFF8A3C`). DMA sound and cache: Mega STE; DMA sound only: STE; blitter only: Mega ST;
  none: ST. Hatari names all four right.
- The Mega ST was first probed through its real-time clock (`0xFFFC21`), as planned; Hatari's
  plain ST does not fault there, so every ST read as a Mega ST, and writing to the clock to
  check it is unsafe on a machine where that address may not be a clock. The blitter replaced
  it: standard on the Mega ST, absent from a stock ST, so an ST with a blitter upgrade is shown
  as "Atari Mega ST, blitter, no DMA sound (or an ST with a blitter)".
- The monitor-detect line (MFP GPIP bit 7) sampled 32 times with a delay between samples (about
  0.4 s at `-O2`, slower in debug builds), reported as mono or colour and stable or changing;
  `ok` when stable. Hatari: colour or mono as started, never changing.
- Amiga: the Agnus ID from `VPOSR` bits 8 to 14 (OCS, ECS or AGA; bit 12 for NTSC) and the low
  byte of `DENISEID`, shown raw. FS-UAE's A500: ID `0x00`, "OCS Agnus, PAL", Denise `0x00`.
- `@@ selftest machine <name> chip=0x.. denise=0x..`, `@@ selftest monitor mono|colour
  changes=.. samples=..`; the harness expects the machine it started each emulator as and,
  on the Atari, a stable line in the mode it chose. `all_harness.sh` 14 of 14 PASS.
- Release payloads: `st` 21,540, `amiga` 21,788 bytes.
