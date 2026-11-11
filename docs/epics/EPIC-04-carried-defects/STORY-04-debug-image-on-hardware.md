---
id: STORY-04
epic: EPIC-04
title: A debug image that does not trap on hardware
status: done
---

## Goal

A debug image put on a real ST by mistake runs without traces, instead of crashing on its first
trace.

## Tasks

- [x] `src/st/natfeat.s`: detect native features with Hatari's probe, `0x7300` (`_nf_get_id`),
  behind an illegal-instruction vector that the debug build installs for the probe. On a real
  ST the vector answers "no native features", and every trace becomes a no-op.
- [x] The vector is restored afterwards, or stays pointed at a stub that reports the fault.
  Choose and record which.
- [x] Harness: all ST and STE sessions still trace in Hatari.
- [x] A debug image with native features disabled (Hatari `--natfeats no`) reaches the list.
  This stands in for a real ST.

## Acceptance

In Hatari with `--natfeats no`, the debug image boots to the list with no traces. With native
features on, the harness passes as before.

## Notes

- From iteration 1's candidates (2026-10-09). The rule "debug images never go on hardware"
  stays. This story only makes breaking it harmless.
- The Amiga's debug trace goes to the serial port and needs no probe. This story is ST and STE
  only.

**Done 2026-11-11**, on `epic-04-carried-defects`, uncommitted.

- **The probe.** `_nf_get_id` saves vector 4 (illegal instruction, `0x10`) in `a1` and points it
  at `nfNoFeatures`. It then runs `0x7300` with `d0` cleared and restores the vector. On an
  illegal-instruction exception, `nfNoFeatures` clears `d0` and returns to the restore by
  rewriting the frame's PC.
- **Restored, not kept.** The vector holds the handler only for the probe. `_nf_call` runs only
  with an ID the probe returned, so it never runs on a real ST.
- **The harness.** All 19 Hatari sessions trace and pass, with native features on.
- **With native features off,** using the debug-test ST image, by screenshot:
  - the old `st-debug` image (from 2026-10-09) never draws: the screen stays black;
  - the new one reaches the list of 33 ROMs, and Hatari's log shows no exception.
