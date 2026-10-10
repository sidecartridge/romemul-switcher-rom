---
id: STORY-03
epic: EPIC-02
title: Merge, tag and publish (Diego's step)
status: done
---

## Goal

`main` holds v4.0.0 and the `v4.0.0` GitHub release carries the three images (D-05).

## Tasks

- [x] Every epic branch of iteration 1 merged into `release/v4.0.0` by pull request (Diego)
- [x] `release/v4.0.0` merged into `main` (Diego)
- [x] `make tag` on `main` pushes `v4.0.0`; `release.yml` publishes the release (Diego)
- [x] The three release assets checked: canonical names, exact sizes (196,608, 262,144,
      524,288), the "Rescue Switcher - v4.0.0" title in each, our checksum recomputed, and
      `romtool info` all `ok` on the Amiga image

## Acceptance

The release page shows v4.0.0 with the three images and the notes of STORY-01.

## Notes

**2026-10-10.**

- Merged by pull request into `release/v4.0.0`: EPIC-00 (#3), EPIC-01 and EPIC-03 (#4, EPIC-01's
  two commits rode along), EPIC-02 (#5). Each ran `build.yml` green first.
- `release/v4.0.0` into `main` by pull request #6 (`561b13d`), merged by Diego.
- Diego ran `make tag` on `main`: `v4.0.0` on `561b13d`. `release.yml` run `38074163299` built and
  published the release, marked latest, not a draft or prerelease.
- The assets, downloaded with `gh release download`: the canonical names, sizes 196,608,
  262,144 and 524,288, the "Rescue Switcher - v4.0.0" title and build ID `561b13d` in each, our
  checksum recomputed independently, and `romtool info` all `ok` on the Amiga image. A local
  build of `561b13d` gives the same bytes as all three.

  | asset | SHA-256 |
  |---|---|
  | `RESCUE_SWITCHER_v4.0.0_192KB.img` | `0ef464ae791c8737d801f90d3a8461f5b7bf9518c190a394b219271c8942ee74` |
  | `RESCUE_SWITCHER_v4.0.0_256KB.img` | `56a13262012fdc6c3d68bd9a97f30c0229982e73af047db445ee6eadfab66ea5` |
  | `RESCUE_SWITCHER_v4.0.0_512KB.img` | `c7096a351e2fffd970c8211687aa8a682e216c6b591af6bd6c7a5f91e591c9d6` |

- The release page showed the changelog's hard line wraps as line breaks, and its `# Changelog`
  title as a heading. With Diego's go, the published text was replaced by the same notes with
  one line per paragraph and no title (the assets untouched), `CHANGELOG.md` was rewritten the
  same way, and `release.yml` now skips the title.
- Diego then asked for every working branch to be deleted, locally and on GitHub, leaving only
  `main`: the epic branches and `release/v4.0.0`, each checked merged first.

**Done 2026-10-10.**
