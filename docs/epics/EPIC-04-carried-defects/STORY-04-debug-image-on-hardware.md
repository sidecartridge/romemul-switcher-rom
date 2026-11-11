---
id: STORY-04
epic: EPIC-04
title: A debug image that does not trap on hardware
status: todo
---

## Goal

A debug image put on a real ST by mistake runs without traces, instead of crashing on its first
trace.

## Tasks

- [ ] `src/st/natfeat.s`: detect native features with Hatari's probe, `0x7300` (`_nf_get_id`),
  behind an illegal-instruction vector that the debug build installs for the probe. On a real
  ST the vector answers "no native features", and every trace becomes a no-op.
- [ ] The vector is restored afterwards, or stays pointed at a stub that reports the fault.
  Choose and record which.
- [ ] Harness: all ST and STE sessions still trace in Hatari.
- [ ] A debug image with native features disabled (Hatari `--natfeats no`) reaches the list.
  This stands in for a real ST.

## Acceptance

In Hatari with `--natfeats no`, the debug image boots to the list with no traces. With native
features on, the harness passes as before.

## Notes

- From iteration 1's candidates (2026-10-09). The rule "debug images never go on hardware"
  stays. This story only makes breaking it harmless.
- The Amiga's debug trace goes to the serial port and needs no probe. This story is ST and STE
  only.
