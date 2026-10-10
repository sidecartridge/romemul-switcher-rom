---
id: EPIC-02
iteration: 1
title: The v4.0.0 release gate
status: done
---

## Goal

Publish the three v4.0.0 images as the GitHub release `v4.0.0` of this repository, built by
`release.yml` from `main`, and hand their checksums to the firmware, whose release images
download and pin them (firmware EPIC-11, C-06). This ends iteration 1.

## Scope

- In scope: the release notes; the first CI runs of the new build path (EPIC-00 STORY-01);
  the merges and the tag; checking the published images; the firmware's re-pin.
- Out of scope: any code change other than what CI needs to build; the firmware's own release.

## Platforms

All three images, as published: `RESCUE_SWITCHER_v4.0.0_192KB.img`, `_256KB.img`, `_512KB.img`,
plus `RSWIT192.PRG` and `RSWIT256.PRG` in `dist/` (only the `.img` files are uploaded).

## Stories

- STORY-01: Release notes
- STORY-02: The build path on CI
- STORY-03: Merge, tag and publish (Diego's step)
- STORY-04: The firmware re-pins (sync)

## Notes

Runs after EPIC-01 and after any candidate of `ITERATIONS.md` that Diego chooses for v4.0.0.
`release.yml` builds on the tag with `./build.sh all` and uploads `dist/{st,ste,amiga}/*.img`
with the top section of `CHANGELOG.md` as the body.

**Started 2026-10-10** on `epic-02-release-gate`, cut from `release/v4.0.0` at `bff73ec`
(EPIC-03 merged by pull request #4, with EPIC-01's commits). Diego: "start epic 2". The
candidates still open in `ITERATIONS.md` (the Amiga reset race, the unbounded catalog walk, the
ST keyboard overrun, a debug image on hardware) are not in v4.0.0 unless Diego chooses them.

**Released 2026-10-10.** `v4.0.0` on `main` `561b13d`, published by `release.yml` with the three
images, checked and pinned by hash in STORY-03. The release page's text was cleaned up after
publication (one line per paragraph, no `# Changelog` title), and so were `CHANGELOG.md` and
`release.yml`. Only `main` is left as a branch, locally and on GitHub (Diego). Open: the
firmware's re-pin (STORY-04), waiting for Diego's go in the firmware repo.

**Closing, 2026-10-10.** The firmware re-pinned the three release images in its `454abda`
(STORY-04), on its `main` through the merge `70237d3`. Iteration 1 is done.
