---
id: STORY-13
epic: EPIC-03
title: The rescue image's size and the machine's RAM
status: done
---

## Goal

The screens say how large the rescue image is, which is the ROM window the machine model gives
the cartridge (192 KB on an ST, 256 KB on an STE, 512 KB on an Amiga), and how much RAM the
machine has. Diego, 2026-10-10: "We need to know the size of the itself image and show it" and
"show not only the ROM image size supported in the machine, but the available RAM".

## Tasks

- [x] The boot screen's build line: "Rescue Switcher v4.0.0, 192 KB image, build ..."
- [x] A first self-test line, "Rescue image": size, base address, version and build;
      `@@ selftest image size=<KB> base=<hex>`
- [x] ST and STE: size the two RAM banks in the ROM startup (`src/st/memconf.inc`, included by
      both startup files), with the algorithm of TOS 1.6 and EmuTOS (`bios/memory.S`,
      `memconf`). The MMU is set to two 2 MB banks, a 252-word pattern goes at offset 8 of each
      bank, and where it repeats gives the bank size. No RAM may be used meanwhile, not even a
      stack, so it runs before the copies and the `.bss` clear, with registers only; the result
      is stored after the clear. The MMU then goes back to `0x4`, the value the switcher has
      always run with
- [x] The PRG build (`start.s`) takes TOS's own result instead (`phystop` and `memctrl`), since
      probing would wreck the running TOS
- [x] Amiga: chip RAM and the slow RAM at `0xC00000`, found the way DiagROM V2 finds memory
      (`srcs/asm/initcode.s`, `DetectMemory`). A block is RAM when patterns read back, each after
      other data went on the bus. A block is a shadow when a tag written there shows up lower
      down. Chip RAM is probed at 512 KB steps from a word of ours, slow RAM at 256 KB steps up
      to `0xD80000`
- [x] A last self-test line, "RAM": "1024 KB: bank 0 512 KB, bank 1 512 KB" or "1536 KB: chip
      1024 KB, slow 512 KB"; `@@ selftest ram total=<KB> parts=<KB>+<KB>`
- [x] Harness: `--memsize KB` (Hatari) and `--chip KB --slow KB` (FS-UAE), checked against the
      trace; five new sessions in `all_harness.sh`

## Acceptance

The probes find what the emulators were given:

| emulator | given | found |
|---|---|---|
| Hatari ST, Mega ST, STE | 512 KB (every default session) | bank 0 512, bank 1 0 |
| Hatari ST, STE | 1024 KB | 512 + 512 |
| Hatari Mega STE | 2048 KB | 2048 + 0 |
| Hatari STE | 4096 KB | 2048 + 2048 |
| Hatari ST, by screenshot | 2048, 2560, 4096 KB | 2048 + 0, 2048 + 512, 2048 + 2048 |
| Hatari Mega ST, by screenshot | 4096 KB | 2048 + 2048 |
| FS-UAE A500 | chip 512, slow 512 | 512 + 512 |
| FS-UAE A500 | chip 1024, slow 512 | 1024 + 512 |
| FS-UAE A500 | chip 2048 | 2048 + 0 |
| FS-UAE A500 | slow 1536 | 512 + 1536 |

On the bench: STORY-15.

## Notes

**Done 2026-10-10.**

- On the ST image, Hatari cannot trace with 2 MB in bank 0. The switcher runs with the MMU set
  to 512 KB banks, so the ST's MMU moves every address of a 2 MB bank. Hatari's native features
  read RAM without that translation (`STMemory_STAddrToPointer`), so the trace comes out as
  garbage while the screen is right. The STE's MCU keeps the addresses in place.
  `hatari_harness.py` refuses those sizes on the ST image. They were checked by screenshot:
  T pressed blind, the RAM line read off the picture.
- The MMU stays at `0x4` after the probe on purpose. Setting the probed value, as TOS does,
  would make the 2 MB banks traceable in Hatari, but it changes how the switcher runs on
  every ST, and `0x4` is what the bench has verified.
- The probe writes RAM at `0x000008` to `0x0001FF` (the exception vectors, not yet set at that
  point) and at `0x200008` to `0x2001FF` as the MMU maps them.
- FS-UAE gives the A500 an ECS Agnus (ID `0x20`) when chip RAM passes 512 KB; the harness
  expects it.
- Amiga fast RAM on a Zorro II card (A2000) is not counted: it appears only after AutoConfig,
  which this ROM does not run.
- The epic's scope says no RAM test of the computer. This is a size, not a test.
