---
id: STORY-02
epic: EPIC-03
title: The self-test screen
status: done
---

## Goal

A key in the chooser opens a screen that runs the tests and shows their results, and ESC
returns to the list. Built first, as the frame every test of STORY-03 to STORY-09 adds its line
to.

## Tasks

- [x] The key: one the chooser does not use (today the arrows, RETURN/ENTER, ESC, and D, R, M,
      U, of which D and R act like RETURN and M and U redraw the list), mapped on both
      keyboards (`src/common/kbd.h`, the ST scancode and the Amiga raw code) and in the Amiga
      debug build's serial key bytes; the help line names it
- [x] The screen: a title, one line per test (later stories fill them in), a summary, ESC to the
      list; fits 80 columns by 25 rows and the mono screen; `@@ selftest start` and
      `@@ selftest end ok|fail` in debug builds; `src/common/selftest.c` with the platform facts
      (image size, address lines) passed in
- [x] Harness: every session opens the screen, waits for the end line, checks each test's line,
      and returns with ESC, on all three images
- [x] `README.md` "How it works" describes the self-test

## Acceptance

The screen runs from the chooser on all three images in the emulators, and the harness sessions
pass.

## Notes

**Done 2026-10-09**: `src/common/selftest.c`/`.h` (`selftest_run()`, a result-line helper
the tests report through, a full-width title bar, the summary, ESC back to the list),
`text_vprintf()` in every build for the result lines, `platform_rom_image_size()` on both
platforms, `KEY_T` (0x14 on both keyboards, `t` on the Amiga debug serial port), and the help
bar: "[ENTER] or [RETURN] to load the ROM. [T] to test the device." The harness session opens
the self-test after the first list, waits for `@@ selftest end`, shoots it, and returns with
ESC; `selftest_lines()` in `harness_common.py` is where each test story adds its expected
lines. `all_harness.sh`: 9 of 9 PASS. Screenshots: the colour list with the new help bar,
the mono self-test screen with its title bar.
