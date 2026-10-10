---
id: STORY-02
epic: EPIC-02
title: The build path on CI
status: in-progress
---

## Goal

`build.yml` (pull requests) and `release.yml` (tags) build the three images with the build
path of EPIC-00 STORY-01, which has not run on CI yet.

## Tasks

- [x] The pull request of `epic-00-emulator-harness` into `release/v4.0.0` runs `build.yml`
      green: `./build.sh all` through `tools/dev/build.sh`, the pinned Atari image pulled by
      digest, the romtool image built on the runner
- [ ] The workflows' `stcmd` install and unpinned `pip install amitools` steps removed, since
      nothing uses them any more; `build.yml` still green
- [ ] The published files of the CI build checked as in STORY-03 (names, sizes, the v4.0.0
      title, both checksums), from the run's artifacts or a local `./build.sh all` in a
      scratch copy of the same commit

## Acceptance

`build.yml` green on a pull request with the workflows cleaned up.

## Notes

- 2026-10-10: `build.yml` ran green on pull request #3 (EPIC-00, 52 s) and #4 (EPIC-03, 46 s),
  both through `tools/dev/build.sh`, with the old install steps still in place.
- Both workflows now only check out and run `./build.sh all`: the `stcmd` install (a `curl |
  bash` of the toolkit's `latest` release), the unpinned `pip install amitools`, the Python setup
  and `STCMD_NO_TTY` are gone, since the build runs in the pinned images alone. `build.yml` also
  keeps the three images as the run's `rescue-images` artifact, so a CI build can be checked and
  compared with a local one.
