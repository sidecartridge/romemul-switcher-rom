---
id: STORY-02
epic: EPIC-00
title: ROM groundwork for the harnesses
status: done
---

## Goal

A debug+test image of each platform boots in its emulator, tells the host which build it is and
what it is doing through a channel the emulator can capture, lists the fake catalog of
`src/common/test.c`, and on the Amiga takes its keys from the serial port, so a harness can
drive it and read the result.

## Tasks

- [x] Debug builds link again: `rom_check.c` line 41 computes `% kProgressPollStrideBytes`
      (4096) on a 32-bit value, which at `-O0` becomes a call to `___umodsi3` that no library
      provides (C-05); a mask does the same. `./build.sh all debug` builds; release images
      unchanged apart from the random fill (C-06), payload sizes before and after.
- [x] A debug+test ROM image for every platform. The ST makefile builds no ROM image when
      `TEST=1`, most likely because `test.c`'s arrays (60 KB catalog, 64 KB storage, 4 KB
      parameters) are `.data`, copied at boot into a RAM area of 124 KB, which they exceed. The chooser reads only the
      catalog and the parameters, so drop the storage array from ROM builds, or keep the test
      arrays in ROM with a ROM-resident section in `rom_abs.ld.S`. Decide, record here, and
      finalize the test image with STORY-01's finalizer, like a release one.
- [x] The build ID from STORY-01 shown on the ROM checksum screen, which stays up when the
      check fails, and printed first on the trace. The title line is left alone: with a dirty
      debug ID it would pass 80 columns.
- [x] Trace channel, ST: Hatari's `NF_STDERR` through `htrace.c`. Confirm on the host with
      `--natfeats yes` that the lines arrive, and how (Hatari's stdout or its log).
- [x] Trace channel, Amiga: Paula's serial port in debug builds, `SERPER` 30 (115,200 baud on
      PAL), `SERDAT` written when `SERDATR` shows TBE, as sidecartos-config's `__amigaemutos__`
      debug build does; hooked into the `trace_fn` that `chooser_loop()` already takes.
- [x] `@@` trace lines in debug builds, from `src/common` where possible: `@@ rescue v<ver>
      <build> <platform>`, `@@ romcheck ok|fail stored=<x> computed=<y>`, `@@ protocol <found>`,
      `@@ list <N> entries default=<D> rescue=<R>`, `@@ page <P> drawn`, `@@ waitkey` after each
      keyboard flush, `@@ key 0x<sc> index <N>`, `@@ select <N> <name>`, `@@ select cancelled`,
      `@@ reset`. Release builds unchanged.
- [x] Amiga keys over the serial port in debug builds: `kbd` also polls `SERDATR` RBF and maps
      bytes to the chooser's scancodes (`0x1B` ESC, `0x0D` RETURN, `0x80` to `0x83` for the
      arrows, as sidecartos-config does); the CIA keyboard keeps working.
- [x] What a test build does after a confirmed selection: today it sends the `SELECT_ROM`
      reads, which no emulator answers, and resets into the same image. Keep that and let the
      harness see the second boot by its build-ID line, or stop after `@@ select`. Decide and
      record here.

## Acceptance

`tools/dev/build.sh st debug-test`, `ste debug-test` and `amiga debug-test` each produce a
finalized image. Booted by hand in Hatari (`--tos`) and FS-UAE (`kickstart_file`), each prints its build ID
and `@@ list` with `test.c`'s entries on the host-visible channel. The release images build as
before, with sizes recorded.

## Notes

**Done 2026-10-09** (`epic-00-emulator-harness`, uncommitted):

- Debug link: `rom_check.c` masks with `kProgressPollStrideBytes - 1` instead of `%`. Every
  build type links on all three platforms (STORY-01's matrix).
- The fake flash: `const`, and kept in ROM. A named section was the first try and failed:
  this toolchain's objects are a.out ("section attributes are not supported for this
  target"). a.out puts `const` data in `.text`, so both `rom_abs.ld.S` exclude `test.c.o` from
  `.ramtext` and route its `.text` to a new `.romdata` section after the ROM copy of `.data`.
  `__rom_payload_end` (Amiga) and `__rom_lma_end` (ST) now end after it, so the random fill
  starts after the fake flash. The ST makefile builds the ROM image in every mode. In the ST
  debug+test image `.data` is 4 bytes and `.romdata` 128 KB at `0xFC54A4`. Release images:
  `.romdata` empty, payload unchanged by this part.
- Trace: `src/common/trace.c`/`.h` (`TRACE()`, debug only, `@@ ` prefix, 128-byte lines)
  formats with `text_printf`'s engine, which now writes through a sink (`gTextEmit`), plus
  `text_vsnprintf()`, compiled into debug builds only. `platform_trace_init()` and
  `platform_trace_write()` in `platform.h`: ST through `hatari_trace_msg()` (NF_STDERR), Amiga
  through Paula (`SERPER` 30, `SERDAT` on TBE; registers added to `hw.h`). The Amiga debug
  build used to print its trace on screen; it now goes to the serial port.
- Amiga keys over serial (debug only): `kbd.c` polls `SERDATR` RBF in `kbd_poll_scancode()`,
  `kbd_wait_for_key_press()` and `kbd_wait_for_key_or_esc()`, and clears RBF through `INTREQ`.
- Selection in test builds: kept as on hardware. The ROM sends `SELECT_ROM`, which no emulator
  answers, and resets; both emulators boot the same image again, and the harness sees a second
  `@@ rescue` line.
- Build ID: "Rescue Switcher v4.0.0, build <id>" on row 6 of the checksum screen, seen in a
  Hatari screenshot of the checksum-fail screen (`build ded4ba0-dirty.047eb48+debug+test`).

Observed 2026-10-09:

- Hatari 2.6.1, ST debug+test image as `--tos`, clean `HOME`: `@@ rescue v4.0.0
  ded4ba0-dirty.047eb48+debug+test st`, `@@ romcheck ok`, `@@ list 33 entries default=17
  rescue=0`, `@@ protocol 0x0040`, `@@ page 1 of 2 drawn`, `@@ waitkey index 0 page 1`. Hatari
  booted the image unpatched, since the checksum passed. NF_STDERR lands on Hatari's stderr.
  A copy with one byte flipped: `@@ romcheck fail stored=AC969057 computed=AC978F57`,
  `@@ waitkey`, then the list after a key.
- FS-UAE 3.2.35, Amiga debug+test image as `kickstart_file`, serial on a raw pseudo-terminal:
  the same lines, and a full session typed over serial passed every step, from boot through
  down, RETURN, `@@ confirm`, ESC, `@@ select cancelled`, RETURN, a key, `@@ select 0`,
  `@@ reset rom 0`, to a second `@@ rescue`. Boot to the list took about 26 s at normal speed.
- `@@ protocol mismatch` and `@@ no roms` exist but no test data reaches them.

Keys on the ST, for STORY-03. The debug build polls the ACIA only about every 28 ms: Hatari's
ACIA trace shows the status reads 1.5 frames apart, from the empty-poll delay of 2,500 loops at
`-O0`. The driver also discards a byte read with any error flag set, overrun included. So a
`keypress` event, make and break 1.3 ms apart, is lost almost every time (1 of 6), and a key
pressed while the previous release is still unread loses both. What works, 20 of 20 with no
retry in 4.6 s: `keydown`, wait for the ROM's `@@ key` line, `keyup`, pause 0.2 s of host
time, which is seconds of emulated time under fast-forward. The driver behaviour is a candidate
in `ITERATIONS.md`.

| image | release payload before | after | debug+test payload |
| --- | --- | --- | --- |
| `st` 192 KB | 15,708 | 15,844 | 153,948 |
| `ste` 256 KB | 15,708 | 15,844 | 153,948 |
| `amiga` 512 KB | 16,008 | 16,144 | 154,364 |

The 136 release bytes are the build ID row and the formatter's sink.
