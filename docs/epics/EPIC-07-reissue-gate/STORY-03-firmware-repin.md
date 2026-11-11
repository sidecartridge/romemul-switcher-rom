---
id: STORY-03
epic: EPIC-07
title: The firmware re-pins (sync)
status: done
---

## Goal

The firmware's re-issued v4.0.0 release images pin and download the new rescue images
(C-06).

## Tasks

- [x] Send the firmware session the asset names and SHA-256s
- [x] The firmware's `DEFAULT_ROM/MANIFEST.txt` and `tools/image/make_images.py` pin them. Record its commit hash here
- [x] The user docs' captures: rebuild the screenshot images (`tools/dev/build.sh <platform>
  screenshot`), which show the moved tag's build ID. Tell the firmware session which screens
  changed, so it can retake its captures

## Acceptance

The firmware reports the re-pin with its commit hash, checked read-only in `../sidecartos`.

## Notes

- 2026-11-11: the names, sizes and SHA-256s of the published assets (STORY-02) sent to the firmware session. It reported the re-pin done but uncommitted, waiting for Diego: the three assets downloaded, sizes, hashes and `09bd579` matched, `DEFAULT_ROM/MANIFEST.txt` updated with the URLs unchanged, `./build.sh rp2350a release package` and `rp2350b` built, its image harness 25/25 on each. Checked read-only in `../sidecartos`: its uncommitted `MANIFEST.txt` pins the three hashes. The commit hash is still to come.
- 2026-11-11: the screenshot images rebuilt from the tag (build `09bd579`; ST `6703612e…`, STE `a874938e…`, Amiga `91a878b4…`) and the firmware session told which captured screens changed: the build ID on every image, the Amiga title ("Amiga 16 bit models"), and the Amiga self-test's new "Bus chip" line. The error screens' new last line is not in the key script; its docs already have the text.

**Done 2026-11-11.** The firmware committed the re-pin as `68d6fb8` on its `iteration-2` branch (its EPIC-19, the re-issue gate). Checked read-only in `../sidecartos`: `68d6fb8:DEFAULT_ROM/MANIFEST.txt` pins `511718fc…`, `a403ec76…` and `c79d474f…` with the URLs unchanged. `tools/image/make_images.py` builds the names from the version and checks the files against the manifest, so it needed no change. Whether the 13 rescue switcher captures are retaken from the screenshot images is the firmware's question to Diego.
- 2026-11-12: the firmware re-issued its v4.0.0, tag `v4.0.0` on `402fe66` (its PR #5, iteration 2 into `main`), with the re-pin `68d6fb8` and the retaken captures `8e5fb7e`. Checked read-only: both are in the tag, its `MANIFEST.txt` pins our three hashes, and the protocol is still `0x0040`.
