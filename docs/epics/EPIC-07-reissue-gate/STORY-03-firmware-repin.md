---
id: STORY-03
epic: EPIC-07
title: The firmware re-pins (sync)
status: todo
---

## Goal

The firmware's re-issued v4.0.0 release images pin and download the new rescue images
(C-06).

## Tasks

- [ ] Send the firmware session the asset names and SHA-256s
- [ ] The firmware's `DEFAULT_ROM/MANIFEST.txt` and `tools/image/make_images.py` pin them. Record its commit hash here
- [ ] The user docs' captures: rebuild the screenshot images (`tools/dev/build.sh <platform>
  screenshot`), which show the moved tag's build ID. Tell the firmware session which screens
  changed, so it can retake its captures

## Acceptance

The firmware reports the re-pin with its commit hash, checked read-only in `../sidecartos`.

## Notes
