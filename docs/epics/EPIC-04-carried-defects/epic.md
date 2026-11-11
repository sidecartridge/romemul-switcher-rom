---
id: EPIC-04
iteration: 2
title: The defects carried over from v4.0.0
status: done
---

## Goal

Fix the four defects found by reading the code that v4.0.0 shipped with. None has been seen on
the bench, but each one leaves the user stuck when it does happen. These are: an "exit" that
goes nowhere, a catalog walk with no bound, keys lost on the ST, and a debug image that traps on
a real ST.

## Scope

- In scope:
  - every path where `chooser_loop()` returns;
  - the bound on `parse_rom_description()`;
  - the overrun handling and the poll delay in `src/st/kbd.c`;
  - the native-features probe in debug builds.
- Out of scope:
  - the Amiga reset after a selection (EPIC-05);
  - the ROM size line (EPIC-06);
  - any change to the shared contract.

## Platforms

All three images: STORY-01 and STORY-02 are in `src/common`. STORY-03 and STORY-04 are ST and STE
only. The verification covers both boards on an ST or Mega ST, an STE and an Amiga.

## Stories

- STORY-01: Error screens that try again
- STORY-02: A bounded catalog walk
- STORY-03: The ST keyboard keeps its keys
- STORY-04: A debug image that does not trap on hardware
- STORY-05: Verification (Diego's checkpoint)

## Notes

These are the open candidates of iterations 1 and 2 in `ITERATIONS.md`, all except the Amiga
reset race (EPIC-05). Diego, 2026-11-11: "write the epics and stories for the new iteration".

They come first because none of them waits for a sibling repository. They are also small: each
is one file and one harness session.

**Started 2026-11-11** on `epic-04-carried-defects`, cut from a new `release/v4.0.0` at `b2cbaa8`
(`main`). Diego: "ok, start". The uncommitted screenshot build of 2026-10-11 rode along in the
working tree. STORY-01 to STORY-04 were built and run the same day, uncommitted. `all_harness.sh`
passed 31 of 31 with 0 key retries. Open: Diego's choice for STORY-01, the timed poll gap of
STORY-03, and the bench (STORY-05).

**Done 2026-11-11.** Diego's bench check passed with the release images of `f5cb491` (see the verification story). The code reaches `release/v4.0.0` through EPIC-07.
