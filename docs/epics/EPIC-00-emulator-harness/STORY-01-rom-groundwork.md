---
id: STORY-01
epic: EPIC-00
title: ROM groundwork for the harnesses
status: todo
---

## Goal

A debug+test image of each platform boots in its emulator, tells the host which build it is and
what it is doing through a channel the emulator can capture, lists the fake catalog of
`src/common/test.c`, and on the Amiga takes its keys from the serial port, so a harness can
drive it and read the result.

## Tasks

- [ ] Debug builds link again: `rom_check.c` line 41 computes `% kProgressPollStrideBytes`
      (4096) on a 32-bit value, which at `-O0` becomes a call to `___umodsi3` that no library
      provides (C-05); a mask does the same. `./build.sh all debug` builds; release images
      unchanged apart from the random fill (C-06), payload sizes before and after.
- [ ] A debug+test ROM image for every platform. The ST makefile builds no ROM image when
      `TEST=1`, most likely because `test.c`'s arrays (60 KB catalog, 64 KB storage, 4 KB
      parameters) are `.data`, copied at boot into a RAM area of 124 KB, which they exceed. The chooser reads only the
      catalog and the parameters, so drop the storage array from ROM builds, or keep the test
      arrays in ROM with a ROM-resident section in `rom_abs.ld.S`. Decide, record here, and
      finalize the test image like a release one (exact size, random fill, checksum; `romtool`
      on the Amiga).
- [ ] Build ID in every build: git `<sha7>`, `-dirty.<diff7>` with uncommitted changes, `+debug`
      and `+test` suffixes, passed by both makefiles next to `APP_VERSION_STR`, shown on the
      title line or the checksum screen, and printed first on the trace (sidecartos-config
      EPIC-00 STORY-02 is the model).
- [ ] Trace channel, ST: Hatari's `NF_STDERR` through `htrace.c`. Confirm on the host with
      `--natfeats yes` that the lines arrive, and how (Hatari's stdout or its log).
- [ ] Trace channel, Amiga: Paula's serial port in debug builds, `SERPER` 30 (115,200 baud on
      PAL), `SERDAT` written when `SERDATR` shows TBE, as sidecartos-config's `__amigaemutos__`
      debug build does; hooked into the `trace_fn` that `chooser_loop()` already takes.
- [ ] `@@` trace lines in debug builds, from `src/common` where possible: `@@ rescue v<ver>
      <build> <platform>`, `@@ romcheck ok|fail stored=<x> computed=<y>`, `@@ protocol <found>`,
      `@@ list <N> entries default=<D> rescue=<R>`, `@@ page <P> drawn`, `@@ waitkey` after each
      keyboard flush, `@@ key 0x<sc> index <N>`, `@@ select <N> <name>`, `@@ select cancelled`,
      `@@ reset`. Release builds unchanged.
- [ ] Amiga keys over the serial port in debug builds: `kbd` also polls `SERDATR` RBF and maps
      bytes to the chooser's scancodes (`0x1B` ESC, `0x0D` RETURN, `0x80` to `0x83` for the
      arrows, as sidecartos-config does); the CIA keyboard keeps working.
- [ ] What a test build does after a confirmed selection: today it sends the `SELECT_ROM`
      reads, which no emulator answers, and resets into the same image. Keep that and let the
      harness see the second boot by its build-ID line, or stop after `@@ select`. Decide and
      record here.

## Acceptance

`./build.sh st debug test`, `ste debug test` and `amiga debug test` each produce a finalized
image. Booted by hand in Hatari (`--tos`) and FS-UAE (`kickstart_file`), each prints its build ID
and `@@ list` with `test.c`'s entries on the host-visible channel. The release images build as
before, with sizes recorded.

## Notes

The tables below are filled when the story runs.

| image | release payload before | after | debug+test payload |
| --- | --- | --- | --- |
| `st` 192 KB | | | |
| `ste` 256 KB | | | |
| `amiga` 512 KB | | | |
