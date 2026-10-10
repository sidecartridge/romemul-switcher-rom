# tools/dev: out-of-tree builds and the emulator harnesses

Everything here runs on the host with Docker, git, bash and Python 3 (standard library only).
It never touches `build/` or `dist/`. It comes from EPIC-00 of `docs/epics/`, which follows
sidecartos-config's `tools/dev/`.

## Builds

```bash
tools/dev/build.sh <st|ste|amiga> <release|debug|test|debug-test>
```

- The output goes to `tools/dev/builds/<platform>-<type>/`: the finalized
  `RESCUE_SWITCHER_v<ver>_<size>KB.img`, `ROMSWITC.PRG` on ST and STE, the link maps, and the
  objects under `obj/`. The folder is emptied first, inside the container.
- The toolchain images are pinned in `toolchain.sh`. `logronoide/atarist-toolkit-docker-x86_64:1.4.0`,
  by digest, compiles all three images. `romemul-switcher-rom/romtool:1` (amitools 0.8.1 on
  Python 3.12, built from `docker/romtool/` on first use) runs `scripts/finalize_rom.py`.
- Every build carries a build ID: `<sha7>`, then `-dirty.<hash>` when `src/`, `Makefile` or
  `version.txt` differ from HEAD, then `+debug`, `+test` or `+debug+test`. Debug builds show it on
  the ROM checksum screen and print it first on the trace.
- `./build.sh` at the root is the release build: it calls this script and copies the results to
  `dist/`. The firmware pins `dist/`, so tell it first (C-06).
- `romtool.sh` runs the pinned romtool on repository-relative paths, e.g.
  `tools/dev/romtool.sh info tools/dev/builds/amiga-release/RESCUE_SWITCHER_v4.0.0_512KB.img`.
- `measure_builds.sh` builds every platform as release and debug-test and prints the payload
  and free space of each, as a Markdown table.

## Harnesses

```bash
tools/dev/build.sh st debug-test && python3 tools/dev/hatari_harness.py --image st
tools/dev/build.sh amiga debug-test && python3 tools/dev/fsuae_harness.py
tools/dev/all_harness.sh            # builds release and debug+test images, runs 25 sessions, ~5 minutes
```

The harnesses boot our own image as the system ROM: Hatari with `--tos` on an ST, Mega ST, STE
or Mega STE, and FS-UAE with the image as the `kickstart_file` of an A500. Only debug+test
images run there. Their catalog and parameters come from `src/common/test.c`, and their trace
lines start with `@@ `. Every run uses 512 KB of RAM (D-07) unless `--memsize`, `--chip` or
`--slow` say otherwise.

| script | flags |
| --- | --- |
| `hatari_harness.py` | `--image st\|ste`, `--machine st\|megast\|ste\|megaste`, `--monitor rgb\|mono`, `--memsize 512\|1024\|2048\|2560\|4096`, `--corrupt`, `--stuck-line N`, `--stuck-data K`, `--stress-fault K`, `--stress-alias N`, `--soak`, `--interactive` |
| `fsuae_harness.py` | `--chip 512\|1024\|2048`, `--slow 0\|512\|1024\|1536`, `--corrupt`, `--stuck-line N`, `--stuck-data K`, `--stress-fault K`, `--stress-alias N`, `--soak`, `--no-warp`, `--interactive` |

`--stuck-line N` and `--stuck-data K` run a copy of the image where address line AN, or data
line DK, reads as broken (its self-test word changed, the checksums recomputed): the self-test
must name that line and only it. `--stress-fault K` flips data line DK in every pattern word
of the stress region, and `--stress-alias N` makes address line AN read as stuck there: the
stress reads must blame that line alone. `--soak` also presses S for the 30-second soak.

`--memsize` (KB of ST RAM), `--chip` and `--slow` (KB of Amiga chip RAM and slow RAM at
`0xC00000`) give the machine more RAM, and the self-test's RAM line must report it, bank by
bank or part by part (EPIC-03 STORY-13). On the `st` image, Hatari cannot trace with 2 MB in
bank 0 (2048, 2560 and 4096 KB): the switcher keeps the MMU at 512 KB banks, the ST's MMU then
moves every address, and Hatari's native features read RAM without that translation. The
harness refuses those sizes there; the `ste` image has no such limit. FS-UAE fits an ECS Agnus
once chip RAM passes 512 KB, and the harness expects its ID.

The session, the same on both emulators (`harness_common.run_session`):

1. Boot, the build line, and on the ST the screen mode chosen; the boot ping, which no
   emulator answers.
2. The ROM checksum. With `--corrupt`, one byte of the covered fill pattern is flipped, so the
   check must fail and a key must continue.
3. The list with `test.c`'s entry count, default and rescue indices, and protocol 0x40.
4. The self-test (T): the rescue image's size and base, every address line and data line `ok` (or the one a `--stuck-*` run
   broke), the stress reads with no error (or the line a `--stress-*` run broke), with
   `--soak` the 30-second soak, 8 pings unanswered with 8 distinct nonces, the device tests skipped, the catalog
   and the configuration as `test.c` holds them, the machine the emulator was started as
   and, on the Atari, a stable monitor line, the RAM the emulator was given; the summary with
   no failure but the injected fault and three tests skipped (the ping, which a test build
   does not count as a failure, and the two device tests); then ESC back to the list.
5. Down, page 2 and back, RETURN, ESC at the confirmation.
6. Down, RETURN, a key, then the selection and the ROM's own reset.
7. The second boot after that reset.
8. No text ran past column 80 on any of those screens: a debug build traces
   `@@ overflow row N` when it does (EPIC-03 STORY-12).

Every run writes `tools/dev/logs/<time>-<label>/`, with `trace.txt` holding the `@@` lines, the
emulator's log and output, and on Hatari `shots/` with the list, page 2, the confirmation and
any failure. The last line says PASS or FAIL. A FAIL names the step, the trace line it waited
for, and the screenshot when there is one. The exit code is 0 for PASS. Every run checks that
no emulator process is left.

`--interactive` opens a window and hands over: no keys are sent and there is no PASS or FAIL,
but the trace is still saved.

### What a PASS proves, and what it does not

A PASS proves that the image boots on that machine, verifies its own checksum, parses the
catalog and parameters page, draws the list, follows the keys, asks before selecting, and
resets. It says nothing about the bus command protocol, the real flash, what the firmware does
after `SELECT_ROM`, or hardware timing such as the monitor-detect line at power-on: no
emulator has a SidecarTridge (C-03). Those stay in the hardware-verification stories.

### Configuration

`dev.cfg` (git-ignored; copy `dev.cfg.example`) holds the emulator paths when they are not the
defaults, `/usr/local/bin/hatari` and `/Applications/FS-UAE.app/Contents/MacOS/fs-uae`. No ROM
is needed.

### Traps already found

- Hatari loads the user's saved configuration unless `HOME` points elsewhere. Each run gets
  its own `home/`.
- Keys on the ST: a debug build polls the IKBD ACIA only about every 28 ms and drops a byte
  read with an overrun flag. So a `keypress` event, or a press while the previous release is
  unread, gets lost. The harness sends `keydown`, waits for the ROM's trace line, sends
  `keyup`, and pauses 0.2 s (`hatari_harness.py`, EPIC-00 STORY-02).
- Hatari's control socket path must be under 104 bytes on macOS, so it lives in `/tmp/hh*`.
- Under fast-forward Hatari skips rendering frames; a screenshot waits 0.3 s for a fresh one.
- FS-UAE has no key injection and no external screenshot trigger. Keys and trace go through
  Paula's serial port, on a pseudo-terminal put in raw mode before FS-UAE opens it.
- `warp_mode = 1` is safe in FS-UAE and cuts a session from 34 s to 6 s.
- `--corrupt` must flip a byte nothing executes: flipping code makes the run undefined.
- Docker Desktop's file sharing can hide, from the next container, a file the host just wrote
  into a folder a container recreated. Write-then-read steps stay on the container side.
