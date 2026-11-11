---
id: EPIC-07
iteration: 2
title: The v4.0.0 re-issue gate
status: todo
---

## Goal

Publish iteration 2's images as the GitHub release `v4.0.0` again, replacing the images of
`561b13d`, and hand their checksums to the firmware, which re-pins them in its own re-issued
v4.0.0 (D-09, C-06). This ends iteration 2.

## Scope

- In scope:
  - the release notes;
  - the release branch and the merges;
  - moving the `v4.0.0` tag;
  - checking the published images;
  - the firmware's re-pin;
  - the user docs' screenshots, if the screens changed.
- Out of scope: any code change other than what the release needs.

## Platforms

All published images:
- `RESCUE_SWITCHER_v4.0.0_192KB.img`;
- `RESCUE_SWITCHER_v4.0.0_256KB.img`;
- the Amiga images EPIC-06 decides: `_1024KB.img`, and `_512KB.img` if it stays.

## Stories

- STORY-01: Release notes
- STORY-02: Merge, move the tag and publish (Diego's step)
- STORY-03: The firmware re-pins (sync)

## Notes

`make tag` runs `git tag v4.0.0 && git push origin v4.0.0`, which fails while the tag exists
locally and on GitHub (today at `561b13d`). `release.yml` uploads with `overwrite: true`, so the
new run replaces the assets of the existing release.

The new images carry the build ID of the new commit, which is the only thing on screen that tells
them from `561b13d`'s.
