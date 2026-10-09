---
id: STORY-02
epic: EPIC-02
title: The build path on CI
status: todo
---

## Goal

`build.yml` (pull requests) and `release.yml` (tags) build the three images with the build
path of EPIC-00 STORY-01, which has not run on CI yet.

## Tasks

- [ ] The pull request of `epic-00-emulator-harness` into `release/v4.0.0` runs `build.yml`
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
