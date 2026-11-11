---
id: EPIC-04
iteration: 2
title: The defects carried over from v4.0.0
status: todo
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
  - the A19 test (EPIC-06);
  - any change to the shared contract.

## Platforms

All three images: STORY-01 and STORY-02 are in `src/common`. STORY-03 and STORY-04 are ST and STE
only. The verification covers both boards on an ST or Mega ST, an STE and an Amiga.

## Stories

- STORY-01: Every exit screen leads somewhere
- STORY-02: A bounded catalog walk
- STORY-03: The ST keyboard keeps its keys
- STORY-04: A debug image that does not trap on hardware
- STORY-05: Verification (Diego's checkpoint)

## Notes

These are the open candidates of iterations 1 and 2 in `ITERATIONS.md`, all except the Amiga
reset race (EPIC-05). Diego, 2026-11-11: "write the epics and stories for the new iteration".

They come first because none of them waits for a sibling repository. They are also small: each
is one file and one harness session.
