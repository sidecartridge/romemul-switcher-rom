---
id: STORY-01
epic: EPIC-07
title: Release notes
status: in-progress
---

## Goal

The top section of `CHANGELOG.md` describes the re-issued v4.0.0: what it fixes and adds over
`561b13d`, and that it replaces the first v4.0.0 images.

## Tasks

- [x] Rewrite the v4.0.0 section, or add a "Re-issued" part to it, as Diego prefers. One line
  per paragraph and list item (`CLAUDE.md`).
- [x] Name the images and their sizes. Say that the build ID on screen tells the two issues
  apart.
- [ ] Diego approves the text.

## Acceptance

Diego approves the notes. `release.yml` will read them up to the first `---` line.

## Notes

**Drafted 2026-11-11** on `epic-07-reissue-gate`, uncommitted, for Diego to review (Diego chose
"Draft release notes" from the options). The v4.0.0 section keeps its first-issue content and
adds:
- a paragraph saying these images replace the 2026-10-10 ones, with build `561b13d` to tell
  them apart;
- "Upgrading from the first v4.0.0": copy the new image over the old one, leave `RESCUE.TXT`;
- items marked "New in the re-issue": the bus chip line under New, and under Fixed the error
  screens that try again, the erased catalog, the ST keys, and the Amiga's wait before its reset.

The heading's re-issue date is `YYYY-MM-DD` until the tag; it must be set before
`release.yml` publishes the section. Internal changes are left out: the debug image's probe, the
Amiga frame counter, and the harness.
