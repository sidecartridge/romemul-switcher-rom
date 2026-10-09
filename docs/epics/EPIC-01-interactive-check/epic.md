---
id: EPIC-01
iteration: 1
title: The three images by eye, before the release
status: done
---

## Goal

Diego looks at every screen of the three images in Hatari and FS-UAE and types on them with the
real keyboard, before the release gate (EPIC-02). The harnesses of EPIC-00 check the trace, not
the picture, and on the Amiga they type through the serial port: this is the one place the
Amiga's CIA keyboard path and every screen are seen in an emulator. Diego, 2026-10-09: "you can
leave interactive out and add that tests before the epic of the release gate".

## Scope

- In scope: `--interactive` runs of `tools/dev/hatari_harness.py` and
  `tools/dev/fsuae_harness.py` on the debug+test images built from the commit that will be
  released, and what Diego sees in them.
- Out of scope: the bench (no emulator has a SidecarTridge, C-03); new harness features.

## Platforms

`st` on Hatari's ST in colour and mono, `ste` on Hatari's STE and Mega STE, `amiga` on FS-UAE's
A500.

## Stories

- STORY-01: Hatari, by eye (Diego's checkpoint)
- STORY-02: FS-UAE, by eye (Diego's checkpoint)

## Notes

`--interactive` was written in EPIC-00 STORY-03 and STORY-04. It opens a window, sends no keys
and gives no PASS or FAIL, but saves the trace to the run folder and ends with an `ENDED` line.

**Started 2026-10-09** on `epic-01-interactive-check`, cut from `release/v4.0.0` at `0d827d3`
(EPIC-00 merged by pull request #3). Preparation, by Claude:

- The debug+test images built from `0d827d3`: build ID `0d827d3+debug+test`;
  `tools/dev/all_harness.sh` 9 of 9 PASS on them.
- `--interactive` started once on each emulator and closed after the boot, to check the
  mechanics, not the screens: Hatari opened its window and traced the boot and the screen mode
  within seconds at real speed; FS-UAE reached the list at the A500's own speed in under 40 s.
- Two harness flaws fixed on the way: an interactive run printed "PASS ... 0 steps", which
  claimed a check nobody made, and now prints `ENDED`; and the final read of the trace used a
  pattern that matches an empty buffer at once, so an interactive FS-UAE run saved 0 trace
  lines. `Trace.drain()` replaces it in both harnesses.
- `screencapture` works on this Mac, but it captures the whole desktop, not the emulator
  window, so it was not used to look at the screens.

If a candidate fix is chosen for v4.0.0, it runs before this epic, and these looks must be
taken on images built from the commit that will be released.

**Closing, 2026-10-09.** Diego looked at every screen and agreed: Hatari's ST in colour and mono,
STE, Mega STE and the checksum-fail screen; FS-UAE's A500 and its checksum-fail screen. Each
machine saw a selection and its reset, and the cancel at the confirmation was covered on mono,
STE, Mega STE and the A500. Two harness fixes came out of it: FS-UAE's joystick port 1 is left
empty, so the arrow keys reach the Amiga keyboard; and Ctrl-C ends an interactive run cleanly
with its trace saved. Uncommitted on `epic-01-interactive-check`.
