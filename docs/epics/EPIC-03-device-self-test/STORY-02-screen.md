---
id: STORY-02
epic: EPIC-03
title: The self-test screen
status: todo
---

## Goal

A key in the chooser opens a screen that runs the tests and shows their results, and ESC
returns to the list. Built first, as the frame every test of STORY-03 to STORY-09 adds its line
to.

## Tasks

- [ ] The key: one the chooser does not use (today the arrows, RETURN/ENTER, ESC, and D, R, M,
      U, of which D and R act like RETURN and M and U redraw the list), mapped on both
      keyboards (`src/common/kbd.h`, the ST scancode and the Amiga raw code) and in the Amiga
      debug build's serial key bytes; the help line names it
- [ ] The screen: a title, one line per test (later stories fill them in), a summary, ESC to the
      list; fits 80 columns by 25 rows and the mono screen; `@@ selftest start` and
      `@@ selftest end ok|fail` in debug builds; `src/common/selftest.c` with the platform facts
      (image size, address lines) passed in
- [ ] Harness: every session opens the screen, waits for the end line, checks each test's line,
      and returns with ESC, on all three images
- [ ] `README.md` "How it works" describes the self-test

## Acceptance

The screen runs from the chooser on all three images in the emulators, and the harness sessions
pass.

## Notes
