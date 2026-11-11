---
id: STORY-02
epic: EPIC-07
title: Merge, move the tag and publish (Diego's step)
status: todo
---

## Goal

`main` holds the re-issued v4.0.0, and the `v4.0.0` GitHub release carries its images (D-05,
D-09).

## Tasks

- [ ] Every epic branch of iteration 2 merged into `release/v4.0.0` by pull request, then
  `release/v4.0.0` into `main` (Diego)
- [ ] The old tag deleted locally and on GitHub, and `make tag` run on `main`. `release.yml`
  then replaces the release's assets (Diego)
- [ ] Any asset of the first issue that the second one no longer publishes is removed from the
  release by hand, since `overwrite: true` replaces assets but never deletes them (Diego)
- [ ] The published assets checked:
  - the canonical names and exact sizes;
  - the title and build ID in each;
  - our checksum recomputed;
  - `romtool info` `ok` on every Amiga half;
  - a local build of the tagged commit giving the same bytes.

## Acceptance

The release page shows v4.0.0 with the new images and STORY-01's notes. The hashes are recorded
here.

## Notes

- `release/v4.0.0` and the epic branches were deleted after iteration 1. They are cut again from
  `main` (D-05).
- 2026-11-11: one pull request for the whole iteration (Diego: "we can have a single one for this"), #7, `epic-07-reissue-gate` into `release/v4.0.0`. The epic branches 04 to 06 are all inside it. The firmware session was told the bench check passed, and that the hashes come after the release.
