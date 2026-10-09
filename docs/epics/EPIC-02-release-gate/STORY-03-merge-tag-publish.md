---
id: STORY-03
epic: EPIC-02
title: Merge, tag and publish (Diego's step)
status: todo
---

## Goal

`main` holds v4.0.0 and the `v4.0.0` GitHub release carries the three images (D-05).

## Tasks

- [ ] Every epic branch of iteration 1 merged into `release/v4.0.0` by pull request (Diego)
- [ ] `release/v4.0.0` merged into `main` (Diego)
- [ ] `make tag` on `main` pushes `v4.0.0`; `release.yml` publishes the release (Diego)
- [ ] The three release assets checked: canonical names, exact sizes (196,608, 262,144,
      524,288), the "Rescue Switcher - v4.0.0" title in each, our checksum recomputed, and
      `romtool info` all `ok` on the Amiga image

## Acceptance

The release page shows v4.0.0 with the three images and the notes of STORY-01.

## Notes
