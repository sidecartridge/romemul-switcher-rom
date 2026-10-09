---
id: EPIC-00
iteration: 1
title: A test harness on Hatari and FS-UAE that boots the ROM images
status: todo
---

## Goal

Boot each of the three images as the system ROM of an emulator, driven from the host: Hatari for
the ST (192 KB at `0xFC0000`) and the STE (256 KB at `0xE00000`), FS-UAE for the Amiga 500 (the
512 KB Kickstart replacement at `0xF80000`). The harness drives the chooser with keys, reads the
trace and ends with PASS or FAIL and the evidence, so every later story is exercised on all three
images in minutes, before it reaches Diego's bench. Diego is needed only for what no emulator has:
the cartridge, the real flash, the selection and the reset (C-01, C-03).

Asked by Diego, 2026-10-09: follow sidecartos-config's approach (its EPIC-00), which drives a TOS
program in Hatari and an AmigaOS program in FS-UAE. This repository's product is the ROM itself,
so the emulators boot our image directly: no TOS, no AmigaOS, no disk, no GEMDOS drive.

## Scope

- In scope: the groundwork in the ROM (debug builds that link, a debug+test image for each
  platform, a build ID, a trace channel each emulator can capture, fixed-prefix trace lines,
  keys over the serial port on the Amiga); `build.sh` fixes so debug and test output never reach
  `dist/`, and a pinned toolchain image; a Hatari harness, in colour and in mono; an FS-UAE
  harness; one script that runs everything; sizes; documentation.
- Out of scope: emulating the SidecarTridge (C-03); the bus protocol, the selection, the reset
  and power-on timing, which stay hardware-only (C-01); the candidate fixes in `ITERATIONS.md`,
  which later epics will test with this harness; CI.

## Platforms

All three images, as debug+test builds: `st` on Hatari `--machine st`, `ste` on Hatari
`--machine ste` and `megaste`, `amiga` on FS-UAE's A500. Release images change only where a
story says so, with sizes before and after. The verification story is a run of the harnesses on
this Mac that Diego watches and agrees with, not a bench pass.

## Stories

- STORY-01: ROM groundwork for the harnesses
- STORY-02: Builds the harness can trust
- STORY-03: The Hatari harness
- STORY-04: The FS-UAE harness
- STORY-05: Run every image, measure, document
- STORY-06: Verification (Diego's checkpoint)

## Notes

**What this Mac has (2026-10-09):**

- Hatari 2.6.1 at `/usr/local/bin/hatari`, with `--tos`, `--machine st|megast|ste|megaste`,
  `--memsize 0` (512 KB), `--ttram 0`, `--monitor mono|rgb`, `--natfeats`, `--control-socket`,
  `--screenshot-dir`, `--sound off`, `--confirm-quit`, `--fast-forward`,
  `--fast-forward-key-repeat`, `--run-vbls`, `--log-file`.
- FS-UAE 3.2.35 at `/Applications/FS-UAE.app`.
- `romtool` (amitools) at `/usr/local/bin/romtool`, which `build.sh` already uses.
- Python 3.14.6; the harnesses use the standard library only, as the siblings' do.
- Docker with `logronoide/atarist-toolkit-docker-x86_64` tags `1.4.0` and `latest`, two
  different images. `stcmd` uses `latest` unless `STCMD_IMAGE_TAG` is set; sidecartos-config
  pins `1.4.0` (STORY-02).

**What sidecartos-config's harness learned that applies here** (its EPIC-00 STORY-03, STORY-04):

- Hatari: capture output through a pseudo-terminal, not a file, or it arrives kilobytes late.
  Keys go through the control socket as a single `keypress` event with no hold, special keys as
  ST scancodes written `0x1`, not `1`. Run headless with `--sound off` and SDL's dummy video and
  audio drivers. Quit with `hatari-shortcut quit` and `--confirm-quit false`, or a SIGTERM leaves
  a dialog open. Wait for each key's trace line rather than sleeping.
- The program under test flushes the keyboard before each read, so a key sent too early is lost.
  A `@@ waitkey` trace after every flush tells the harness when to press.
- FS-UAE has no key-injection channel. Keys and trace go through Paula's serial port: the debug
  build writes `SERPER` 30 (115,200 baud on PAL), sends on `SERDAT` when `SERDATR` shows TBE, and
  polls `SERDATR` RBF. The harness puts a pseudo-terminal in raw mode (`tty.setraw`) before
  FS-UAE opens it, as `serial_port`. FS-UAE polls for received bytes only after `SERPER` is
  written. Run it with `window_hidden = 1`, `audio_driver = dummy`, `sound_output = none`, and
  stop it with SIGTERM, which it takes cleanly. FS-UAE screenshots are unsolved there too.

**What differs here:**

- No TOS: no `--conout`, no GEMDOS drive, no `--auto`. The ST trace is Hatari's native features
  (`NF_STDERR` through `src/st/htrace.c` and `natfeat.s`), which exist today but have not been
  seen on the host.
- No AmigaOS: no `DH0:` trace file. The serial port is the only channel, for both keys and
  trace, and our Amiga build has no trace at all today.
- The ROM reads the IKBD ACIA and the CIA directly, so on Hatari the control socket's key events
  reach it through the emulated IKBD, as on a real keyboard.
- Open for STORY-03: whether Hatari boots an image it does not know as TOS without patching it.
  Our header claims `os_version` 0x0104. If Hatari applies TOS 1.04 patches, the ROM checksum
  screen will say so.
- Open for STORY-04: whether FS-UAE boots an unknown ROM as Kickstart. `romtool` writes a valid
  footer and checksum.
