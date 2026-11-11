---
id: STORY-03
epic: EPIC-07
title: The firmware re-pins (sync)
status: in-progress
---

## Goal

The firmware's re-issued v4.0.0 release images pin and download the new rescue images
(C-06).

## Tasks

- [x] Send the firmware session the asset names and SHA-256s
- [ ] The firmware's `DEFAULT_ROM/MANIFEST.txt` and `tools/image/make_images.py` pin them. Record its commit hash here
- [x] The user docs' captures: rebuild the screenshot images (`tools/dev/build.sh <platform>
  screenshot`), which show the moved tag's build ID. Tell the firmware session which screens
  changed, so it can retake its captures

## Acceptance

The firmware reports the re-pin with its commit hash, checked read-only in `../sidecartos`.

## Notes

- 2026-11-11: the names, sizes and SHA-256s of the published assets (STORY-02) sent to the firmware session. It reported the re-pin done but uncommitted, waiting for Diego: the three assets downloaded, sizes, hashes and `09bd579` matched, `DEFAULT_ROM/MANIFEST.txt` updated with the URLs unchanged, `./build.sh rp2350a release package` and `rp2350b` built, its image harness 25/25 on each. Checked read-only in `../sidecartos`: its uncommitted `MANIFEST.txt` pins the three hashes. The commit hash is still to come.
- 2026-11-11: the screenshot images rebuilt from the tag (build `09bd579`; ST `6703612e…`, STE `a874938e…`, Amiga `91a878b4…`) and the firmware session told which captured screens changed: the build ID on every image, the Amiga title ("Amiga 16 bit models"), and the Amiga self-test's new "Bus chip" line. The error screens' new last line is not in the key script; its docs already have the text.
