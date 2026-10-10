---
id: STORY-04
epic: EPIC-02
title: The firmware re-pins (sync)
status: done
---

## Goal

The firmware's release images carry the published v4.0.0 rescue images, pinned by their real
SHA-256 (C-06, D-03).

## Tasks

- [x] The SHA-256 of the three release assets sent to the firmware session (or, if none is
      running, to Diego with the file to change: `../sidecartos/DEFAULT_ROM/MANIFEST.txt`)
- [x] The firmware re-pins and its `tools/image/make_images.py --download` fetches the three
      files from the release; its note that the pins come from an uncommitted tree goes away
- [x] The Amiga pin of 2026-09-09, whose image shows "v3.1.0" in its title, is gone

## Acceptance

The firmware session confirms the new pins, or Diego does.

## Notes

The 2026-09-09 pins in the firmware's `MANIFEST.txt` match our local `dist/` of that day; the
firmware session was told on 2026-10-09 that they will not match the release and that the
Amiga one shows "v3.1.0".

2026-10-10: the firmware session (`sidecartos-ce`) told that the v4.0.0 release images are
coming and will not match its pins: what EPIC-03 changed (no contract change; `CMD_PING` and
`CMD_RESTORE_PREVIOUS_ROM` as told on 2026-10-09), that the hashes depend on `main`'s commit, that
CI and local builds of a commit are byte-identical, and that the three assets' SHA-256 follow
the publication.

2026-10-10, after the release: the three assets' SHA-256 sent to `sidecartos-ce` (STORY-03 has
them). Its answer: in a scratch input folder with our hashes on its existing release URLs,
`tools/image/make_images.py --download` fetched the three images from the v4.0.0 release and
they matched; sizes, the v4.0.0 title and build ID `561b13d` right; both boards' release images
built with them, 50 checks passed and 0 failed. The re-pin itself waits for Diego's go in the
firmware repo, where he asked to hold it: `DEFAULT_ROM/MANIFEST.txt` with the three hashes, its
note about the `0d827d3` test images replaced, probably with the host SWITCHER's re-pin
(sidecartos-config `eb1dd0c`). The firmware session sends its commit when that lands.

2026-10-10, the re-pin: firmware commit `454abda`, on the firmware's `main` since Diego merged its
pull request #4 (merge commit `70237d3`, CI green for both boards, release and debug; checked
here on GitHub). Checked here
in `../sidecartos` (read-only): `DEFAULT_ROM/MANIFEST.txt` at `454abda` pins the three assets'
SHA-256 at their v4.0.0 release URLs, its note names our v4.0.0 release tagged on `main` at
`561b13d` and its reproducible builds, and no earlier rescue pin is left (the `0d827d3` ones,
and with them the 2026-09-09 Amiga one). The firmware session reports `make_images.py
--download` fetching the three files from the release into an emptied `DEFAULT_ROM/` and
checking them against the pins, and both boards' release images passing its 50 harness checks.

Also from the firmware session (its `4339cc6`): firmware EPIC-03 moved to its iteration 2
(Diego, 2026-10-10), D-20's refusal of a large ROM whose slot is not staged included; that
refusal boots the rescue ROM instead. Firmware v4.0.0 ships the slot faults as a known issue,
seen only with `LARGE_ROMS = 1`, off by default.

**Done 2026-10-10.**
