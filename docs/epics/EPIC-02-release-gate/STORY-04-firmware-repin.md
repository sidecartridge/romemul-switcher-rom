---
id: STORY-04
epic: EPIC-02
title: The firmware re-pins (sync)
status: todo
---

## Goal

The firmware's release images carry the published v4.0.0 rescue images, pinned by their real
SHA-256 (C-06, D-03).

## Tasks

- [ ] The SHA-256 of the three release assets sent to the firmware session (or, if none is
      running, to Diego with the file to change: `../sidecartos/DEFAULT_ROM/MANIFEST.txt`)
- [ ] The firmware re-pins and its `tools/image/make_images.py --download` fetches the three
      files from the release; its note that the pins come from an uncommitted tree goes away
- [ ] The Amiga pin of 2026-09-09, whose image shows "v3.1.0" in its title, is gone

## Acceptance

The firmware session confirms the new pins, or Diego does.

## Notes

The 2026-09-09 pins in the firmware's `MANIFEST.txt` match our local `dist/` of that day; the
firmware session was told on 2026-10-09 that they will not match the release and that the
Amiga one shows "v3.1.0".
