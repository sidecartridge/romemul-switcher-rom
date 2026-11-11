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

Diego, 2026-11-11: "keep the prefixes and leave the A600 out. But we have to add now A500+ and
A600. Also, the Large images support up to 1MB is experimental, but it can be tested on A500+
and A600." So:
- the 512 KB image is for an Amiga 500, 500+, 600 or 2000;
- a "New in the re-issue" item adds the A500+ and A600;
- an "Experimental" item says Kickstart images up to 1 MB work on the RP2350B with
  `LARGE_ROMS`, to be tested on an A500+ or A600. It is not marked "New in the re-issue",
  because the setting shipped with firmware v4.0.0.

The A600's need for the re-issued firmware's mirror fix stays out. `README.md` and `CLAUDE.md`
name the four Amigas too. The ROM's own title still says "Amiga 500/2000"
(`src/amiga/switcher.c`), pending Diego.

