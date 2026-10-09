---
id: EPIC-01
iteration: 1
title: The three images by eye, before the release
status: todo
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

`--interactive` was written in EPIC-00 STORY-03 and STORY-04 and not yet tried. It opens a
window, sends no keys and gives no PASS or FAIL, but saves the trace to the run folder.
