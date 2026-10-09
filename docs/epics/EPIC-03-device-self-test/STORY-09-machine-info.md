---
id: STORY-09
epic: EPIC-03
title: Machine information
status: todo
---

## Goal

The self-test says which machine it runs on and what it sees of it, with no OS to ask.

## Tasks

- [ ] Atari: ST, Mega ST, STE or Mega STE, by probing hardware with a temporary bus-error
      handler (for example the STE's DMA sound registers, the Mega ST's real-time clock, the
      Mega STE's cache register); the probes and the handler recorded here
- [ ] Atari: the monitor-detect line (MFP GPIP bit 7) sampled over about a second, reported as
      mono or colour and whether it changed, which bears on the mono cold-boot candidate
- [ ] Amiga: the Agnus revision and PAL or NTSC from `VPOSR`, and what else the chipset tells
      without an OS
- [ ] `@@ selftest machine ...` lines; harness: the machine Hatari or FS-UAE was started as

## Acceptance

The right machine named for every machine the harnesses run (ST, Mega ST, STE, Mega STE, A500).

## Notes
