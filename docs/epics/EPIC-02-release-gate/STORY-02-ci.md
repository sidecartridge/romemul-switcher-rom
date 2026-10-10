---
id: STORY-02
epic: EPIC-02
title: The build path on CI
status: done
---

## Goal

`build.yml` (pull requests) and `release.yml` (tags) build the three images with the build
path of EPIC-00 STORY-01, which has not run on CI yet.

## Tasks

- [x] The pull request of `epic-00-emulator-harness` into `release/v4.0.0` runs `build.yml`
      green: `./build.sh all` through `tools/dev/build.sh`, the pinned Atari image pulled by
      digest, the romtool image built on the runner
- [x] The workflows' `stcmd` install and unpinned `pip install amitools` steps removed, since
      nothing uses them any more; `build.yml` still green
- [x] The published files of the CI build checked as in STORY-03 (names, sizes, the v4.0.0
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
- `build.yml` green on pull request #5 with the trimmed workflow (36 s, run `38073603415`). It
  built GitHub's merge commit `10074e2`, whose images carry that build ID.
- The run's `rescue-images` artifact: the three canonical names, exact sizes, the "Rescue
  Switcher - v4.0.0" title in each, our checksum recomputed independently, all right.
- The same commit built locally in a scratch worktree (`tools/dev/build.sh <platform> release`)
  gives the CI's bytes exactly: 192 KB `f1c1348a…`, 256 KB `183891e9…`, 512 KB `84a4fef0…`.
  Builds are reproducible across this Mac and GitHub's runner, as the release notes say.

**Done 2026-10-10.**
