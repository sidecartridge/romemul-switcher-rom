---
id: STORY-02
epic: EPIC-07
title: Merge, move the tag and publish (Diego's step)
status: done
---

## Goal

`main` holds the re-issued v4.0.0, and the `v4.0.0` GitHub release carries its images (D-05,
D-09).

## Tasks

- [x] Every epic branch of iteration 2 merged into `release/v4.0.0` by pull request, then
  `release/v4.0.0` into `main` (Diego)
- [x] The old tag deleted locally and on GitHub, and `make tag` run on `main`. `release.yml`
  then replaces the release's assets (Diego)
- [x] Any asset of the first issue that the second one no longer publishes is removed from the
  release by hand, since `overwrite: true` replaces assets but never deletes them (Diego)
- [x] The published assets checked:
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
- 2026-11-11: #7 merged into `release/v4.0.0` (`341e134`), then #8 `release/v4.0.0` into `main` (`09bd579`). #8's first build stayed queued through a GitHub Actions outage and could be neither cancelled nor re-run, so the pull request was closed and reopened; the new run passed. The epic branches and `release/v4.0.0` were deleted locally and on GitHub.
- 2026-11-11, Diego deleted the old tag (`561b13d`) locally and on GitHub and ran `make tag` on `main`: `v4.0.0` is `09bd579`. Deleting the tag turned the first release into a draft, so `release.yml` (run 43110762717, passed) created a new published `v4.0.0` release, marked latest, with only the three images and the re-issue notes. The first release is left as a draft with the first issue's assets: deleting it is the third task.
- The published assets, checked 2026-11-11: canonical names, exact sizes, title and build `09bd579` in each, our checksum recomputed, `romtool info` all `ok` on the Amiga image, and byte-identical to a local build of the tag:

  | image | size | SHA-256 | checksum |
  |---|---|---|---|
  | `RESCUE_SWITCHER_v4.0.0_192KB.img` | 196,608 | `511718fc9121fd9328bfa8f23b402c58c923f6994ad1ab6ee5b3f37a821c8199` | `B6583310` |
  | `RESCUE_SWITCHER_v4.0.0_256KB.img` | 262,144 | `a403ec763a1d11345a50bf18da0bf4569a3311f9c271b2f627437cdae36019ba` | `F67A2CA7` |
  | `RESCUE_SWITCHER_v4.0.0_512KB.img` | 524,288 | `c79d474fe3413671ba50975f730fa3d54a35ab7d44f031793acb0c412cea614c` | `F5F38998` |

**Done 2026-11-11.** Diego deleted the first release's draft. The only `v4.0.0` release is the re-issue, with the three images above.
