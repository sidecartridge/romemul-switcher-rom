# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository. It is the single source of project context for coding agents; there is no separate `AGENTS.md`.

## What this is

The **bare-metal rescue ROM switcher** for the **SidecarTridge ROM Emulator** family (TOS Emulator and Kickstart Emulator). It is a ROM image that the cartridge's Pico firmware (`../sidecartos`) boots *instead of* the system ROM when the device is in **Rescue Mode**. It runs on the retro computer (Atari ST/Mega ST, Atari STE/Mega STE, Amiga 500/2000), lists the ROM images stored in the cartridge's flash, lets the user pick one, tells the firmware to make it the boot ROM, and hard-resets the machine.

It is **fully freestanding**: no TOS, GEMDOS, BIOS, XBIOS, Kickstart or AmigaOS calls anywhere in the runtime; no libc; `LIBS`/`LDLIBS` must stay empty (the Makefiles error out otherwise). All platform code talks to the hardware registers directly with interrupts masked and everything polled.

Sibling repos (same protocol, different roles):

- `../sidecartos` — the Pico firmware. Its `CLAUDE.md` memory-map table is the **authority** for flash offsets, the catalog/parameters layout and the protocol version.
- `../sidecartos-config` — the *host-side* switcher (`SWITCHER.TOS` / AmigaOS `SWITCHER`) that runs under TOS/AmigaOS with a full FatFS and can upload/delete/rename ROMs. This repo is the read-only, OS-less counterpart; do not import its FatFS/libcmini/VBCC approach here.

### The three repos are developed in sync

`sidecartos`, `sidecartos-config` and this repo form one product. **Any change to what they share must land in all three, coordinated, never in one alone.** The shared contract is:

- the protocol version (`kSwitcherTosProtocolVersion` here, `SWITCHER_TOS_PROTOCOL_VERSION` in `usbdrive/src/include/constants.h` and `sidecartos-config/src/include/helper.h`), plus the copy in `src/common/test.c` byte 512;
- the flash offsets (`FLASH_CATALOG_START`, `FLASH_PARAMS_START`) and the firmware linker scripts behind them;
- `parameters_t` and `rom_catalog_t` from the firmware's `usbdrive/src/include/builder.h` (this repo hard-codes byte offsets 70, 256+70 and 512);
- the bus protocol: magic-sequence offsets, nonce and checksum masks, command codes in the firmware's `emul/src/include/tosemul.h`, the Hamming encoding of `CMD_SELECT_ROM`, and the reply layout;
- the release version (`version.txt` here; the firmware and host switcher ship as the same `v<x.y.z>`) and the published image names, which the firmware's `tools/image/make_images.py` and `DEFAULT_ROM/MANIFEST.txt` consume.

How to keep them in sync:

1. Before changing anything in that list, check the other two repos' current source (read-only) and agree the change with them.
2. If Claude sessions are running in the sibling repos, use `ListAgents` to find them and `SendMessage` to tell them what changes, with exact values and offsets. Expect the same from them, and verify what they report against their source before acting on it.
3. If no sibling session is running, tell the user which sibling files must change and leave those repos untouched.
4. The firmware pins this repo's images by SHA-256 in `DEFAULT_ROM/MANIFEST.txt` and downloads them from this repo's GitHub release. Images are padded with random bytes, so every build changes the hashes. Tell the firmware side before a rebuild lands in `dist/`, and after publishing a release ask it to re-pin from the release assets, not from a local `dist/`.
5. Cite the other repos' backlog items as "firmware EPIC-NN / D-NN" and "sidecartos-config EPIC-NN"; they cite this one as "romemul-switcher-rom EPIC-NN" (see Backlog).

Facts the siblings rely on that are easy to break here:

- The firmware zero-fills the whole 60 KB catalog after the last record and caps it at 64 records (`ROM_MAX_FILES`). `parse_rom_description()` relies on that: it walks records until one starts with a zero byte, with no upper bound.
- `blocks` is a u32 at record byte 64. This ROM reads only its low 16 bits, which is enough up to 65535 blocks.
- Parameters-page bytes 516 to 532 hold the `CONFIG.TXT` words that only the host switcher reads. Bytes 536 to 767 may hold stray data on a device.
- The firmware rejects a rescue ROM above 512 KB. Debug firmware holds at most 496 KB in SRAM (firmware D-16), so test the 512 KB Amiga image with release firmware.

One source tree produces **three ROM images**, selected by `ROM_BASE_ADDR_UL` (no Makefile default; `tools/dev/build.sh` passes it):

| platform | `ROM_BASE_ADDR_UL` | startup asm | image | notes |
|---|---|---|---|---|
| `st` | `0x00FC0000UL` | `src/st/startup_st.s` | `RESCUE_SWITCHER_v<ver>_192KB.img` | Atari ST / Mega ST (192 KB TOS slot) |
| `ste` | `0x00E00000UL` | `src/st/startup_ste.s` | `RESCUE_SWITCHER_v<ver>_256KB.img` | Atari STE / Mega STE (256 KB TOS slot) |
| `amiga` | `0x00F80000UL` | `src/amiga/startup.s` | `RESCUE_SWITCHER_v<ver>_512KB.img` | Amiga 500/2000, full 512 KB Kickstart-replacement ROM |

`st` and `ste` share `src/st/`; `src/st/switcher.c` derives the displayed model name from `ROM_BASE_ADDR_UL` (`0x00FC0000UL` → "Atari ST", `0x00E00000UL` → "Atari STE"). The ST build also emits a `ROMSWITC.PRG` (linked via `start.s`) that runs the same code as a TOS program for quick testing in Hatari. `start.s` is the **only** place allowed to touch the OS: it calls GEMDOS `Super(0)` when not already in supervisor mode, then never returns to TOS.

`version.txt` is the single version source (`v4.0.0`; a leading `v` is stripped by the build scripts and both Makefiles before it becomes `-DAPP_VERSION_STR` and the artifact names). `CHANGELOG.md` is read by the release workflow up to the first `---` line.

## Common commands

```bash
tools/dev/build.sh st release      # out of tree into tools/dev/builds/st-release/; never touches dist/
tools/dev/build.sh ste debug-test  # types: release | debug | test | debug-test; platforms: st | ste | amiga
./build.sh st                      # release build, published to dist/st/ (st | ste | amiga | all)
./build.sh all                     # replaces dist/, so tell the firmware first (C-06)
tools/dev/romtool.sh info <img>    # the pinned romtool (amitools 0.8.1), repo-relative paths
tools/dev/all_harness.sh           # build the 3 debug-test images, run 9 Hatari/FS-UAE sessions (~1 min)
python3 tools/dev/hatari_harness.py --image ste --machine megaste   # one session; also --monitor mono, --corrupt
python3 tools/dev/fsuae_harness.py # the Amiga session over the serial port; also --corrupt, --no-warp
tools/dev/measure_builds.sh        # payload and free space of every image, as a Markdown table
make clean                         # removes build/st* and build/amiga* (outputs of a direct make)
make format-check                  # clang-format dry run over src/**/*.{c,h}
make tidy                          # clang-tidy over src/**/*.c
make check                         # format-check + tidy
make tag                           # git tag + push v<version.txt>; triggers the release workflow
./docs/epics/cockpit.sh            # regenerate docs/epics/STATUS.md after editing an epic or story
```

There is no unit-test suite. The gate is the emulator harnesses (D-06; `tools/dev/README.md`): **every code change runs `tools/dev/all_harness.sh`** and builds the release images with `tools/dev/build.sh <platform> release`, before it reaches Diego's bench. The harnesses boot the debug+test images as the system ROM of Hatari (ST, Mega ST, STE, Mega STE, colour and mono) and FS-UAE (A500), drive the chooser and read the `@@` trace. A PASS says nothing about the bus, the flash, the selection on a real device or power-on timing (C-03). Run `./build.sh` only when the firmware session has been told (C-06).

## Code organisation

Three directories under `src/`, one rule: **`src/common` is platform-agnostic** and may only include the four interface headers it owns (`glyph.h`, `kbd.h`, `platform.h`, `term.h`); `src/st` and `src/amiga` implement those interfaces and own every hardware detail. Each platform has its own `Makefile`, `mem.h` (RAM layout constants; the ST one has `_Static_assert` guards), and `rom_abs.ld.S` (linker script template preprocessed with `ROM_BASE_ADDR_UL`).

The pieces that take several files to understand:

- **Boot path**: `startup_st.s` / `startup_ste.s` / `amiga/startup.s` (ROM entry) or `st/start.s` (PRG entry) → `switcher.c` (`rom_switcher_main`: IPL mask, screen, `palloc` heap, trace) → `rom_check.c` (integrity screen) → `chooser.c` (`chooser_loop`).
- **Protocol stack**: `chooser.c` asks `commands.c` for pages; `commands.c` builds commands and drives retries; `platform.c` on each side emits the actual magic-address reads (`platform_send_magic_sequence`) and the hard reset.
- **Text stack**: `text.c` (`text_printf`, VT52-like escapes) → `glyph.c` per platform (font from `font8x8.h`) → `screen.c` per platform.
- **Debug plumbing** (ST only): `natfeat.s` + `htrace.c` send traces to Hatari NatFeats; compiled only with `_DEBUG`.
- **Test data**: `test.c` holds the raw params/catalog/storage arrays and is only added to `SRCS_C` when `TEST=1`.

File map, for orientation:

| path | role |
|---|---|
| `common/chooser.c` | catalog + parameters parsing, pagination, menu flow, ROM selection |
| `common/commands.c` | bus protocol: magic sequence wrapper, sync commands with retries, block/page reads |
| `common/rom_check.c` | full-ROM additive checksum with progress callback |
| `common/palloc.c` | freestanding bump allocator |
| `common/text.c` | `text_printf` and VT52-like control handling |
| `common/font8x8.h` | shared 8x8 font |
| `st/start.s` | PRG entry: supervisor transition, stack, jump to `rom_switcher_main` |
| `st/startup_st.s`, `st/startup_ste.s` | ROM entry (`_os_entry`) per model |
| `st/screen.c`, `st/palette.h` | shifter, video mode, palette, screen memory |
| `st/kbd.c` | IKBD ACIA polling, filters non-keyboard packets |
| `st/glyph.c` | ST glyph renderer |
| `st/platform.c` | magic-sequence inline asm per ROM base, hard reset |
| `st/natfeat.s`, `st/htrace.c` | Hatari NatFeats trace (`_DEBUG` only) |
| `st/mem.h`, `st/rom_abs.ld.S` | RAM layout with `_Static_assert` guards; ROM linker script template |
| `amiga/startup.s` | ROM bootstrap: overlay off, code/`.data` copy to RAM, `.bss` clear, ROM-resident `_platform_hard_reset` |
| `amiga/screen.c` | hires 2-bitplane display and framebuffer, DiagROM-inspired structure |
| `amiga/kbd.c` | CIAA keyboard polling and handshake |
| `amiga/glyph.c` | bitplane-aware glyph renderer |
| `amiga/platform.c` | magic-sequence transport with `0x00F8xxxx` packed register args |
| `amiga/hw.h`, `amiga/mem.h`, `amiga/rom_abs.ld.S` | custom chip/CIA registers; RAM layout; fixed 512 KB Kickstart ROM layout |

## How it talks to the cartridge — the bus command protocol

The firmware cannot be addressed as a device: the host can only *read* ROM. Commands are therefore encoded as a **sequence of reads at magic addresses** that the Pico's DMA IRQ handler recognises on the address bus (see `../sidecartos` `dma_irq_handler_lookup`).

- **ROM base**: `ROM_BASE_ADDR_UL` (above). `init_rom_address()` (`commands.c`) must run before any command; it records `magic_number_address` = base, `scratch_rom_address` = base and `remote_rom_address` = base + `kRomInRamSwapOffset` (4).
- **Magic sequence** (`platform_send_magic_sequence`, one implementation per platform in `platform.c`): 8 byte reads at fixed offsets from the base (`+0x1234, +0xFC42, +0x6452, +0xCDE0, +0x5CA2, +0x8CA4, +0x1F94, +0xE642`), then 5 reads at `base + msb`, `base + lsb`, `base + command`, `base + param`, `base + checksum`, where `msb/lsb` come from a random 32-bit nonce masked to even words (`& 0xFFFEFFFE`) and `checksum` is the even-masked 16-bit sum of the four. The ST version hard-codes the two ROM bases in separate inline-asm blocks (`0xFC....` and `0xE0....`); the Amiga version uses `tst.b 0xF8....` and passes args in `a0–a4`. The RNG seed is `platform_get_system_time_seed()` (MFP timer bytes on ST, the long at address 4 on Amiga).
- **Reply channel**: the firmware overwrites the first bytes of the emulated ROM. Base+0 echoes the nonce when the command completed (`0xFFFFFFFD` = checksum error); base+4 is where block data, checksum and status appear (`kReadRomBlockSize` 512 B + checksum word). `send_sync_rom_command` spins on base+0 with a retry budget, calling `platform_poll()` each iteration.
- **Commands used here** (`commands.c` enum): `kCmdSelectRom 0x0000`, `kCmdUnlockReadBlockMemory 0x0008`, `kCmdLockReadBlockMemory 0x000A`. The parameter is always shifted left one bit (addresses must be even); `kCmdSelectRom` additionally **Hamming-encodes** the ROM index (`hamming_encode`). The host-side tool has many more commands (write, erase, rename…); this ROM is read-only by design.
- **Flash reads**: `read_flash_block` (512 B, verified against the remote checksum, `kMaxCommandRetries` 16) and `read_flash_page` (4 KB = 8 blocks; the `endian` flag word-swaps because the Pico stores little-endian). Flash **offsets** are relative to the Pico's `XIP_BASE`: `FLASH_CATALOG_START 0xFF0000`, `FLASH_PARAMS_START 0xFFF000` (`chooser.c`). They **must match the firmware's linker scripts**.
- **Selecting a ROM** = `send_change_rom_command_and_hard_reset(index)`: one async `kCmdSelectRom` then `platform_hard_reset()`. **`SELECT_ROM` is never echoed**: the firmware leaves its command loop, `write_rom_list()` erases and rewrites the parameters page, then `watchdog_reboot()`, and the ROM is off the bus for about 26 to 29 ms until the emulator is up again. On ST the reset path masks interrupts, busy-waits about 2.9 s at 8 MHz (covering that gap), clears the three memvalid magics (`0x420`, `0x43A`, `0x51A`) so TOS does a cold start, and jumps through the reset vector at address 4. On Amiga the stub lives in ROM `.boot` because RAM code is gone after reset; it runs with **no delay**, which is an open issue (C-07, and the candidates in `docs/epics/ITERATIONS.md`).

## Catalog and parameters

`chooser_loop()` reads the **parameters page** (4 KB at `0xFFF000`) and the **catalog** (up to `MEMORY_EXCHANGE_SIZE` 61440 B at `0xFF0000`) over the bus, then parses:

- protocol version word → compared with `kSwitcherTosProtocolVersion` (`0x0040`, same as the host-side `sidecartos-config` tool). Mismatch shows an incompatibility message and waits for a key. **Bump it in lockstep with the firmware.**
- default ROM index at byte offset `kDefaultRomIndex` 70 and rescue ROM index at `256 + 70`, both little-endian words. These offsets are hard-coded to the firmware's `parameters_t`; a layout change lands in both repos.
- the catalog is a sequence of 256-byte `rom_catalog_t` entries (64-byte name + metadata). Entries matching the default/rescue index get metadata flags and are marked in the list (`R` for rescue).

Menu keys (`kbd.h`, raw IKBD-style scancodes shared by both platforms): arrows move/paginate, **RETURN/ENTER** selects, **ESC** exits/cancels. `D`, `R`, `M`, `U` are recognised but only return to the caller (no delete/rename/upload in the ROM). Selection asks for confirmation ("any key, ESC to cancel") before `send_change_rom_command_and_hard_reset`.

## Program flow and platform split

1. Startup asm (`startup_*.s`) enters supervisor mode, sets the stack, (Amiga) fixes the overlay and relocates code/`.data` to RAM, clears `.bss`, calls the C entry.
2. `switcher.c` masks interrupts (IPL 7 on ST), initialises the screen, `palloc` heap (ST: from `_end`, inside `ST_RAM_VARS_*`), keyboard and (debug builds) trace.
3. **ROM integrity check** (`rom_check.c`): full-ROM additive big-endian 16-bit sum with a spinner. On mismatch the stored and computed values are shown and boot continues only after a key press. ST/STE checksum field = last 4 bytes of the image. Amiga field sits **immediately before the Kickstart footer** (offset `0x7FFE4`), and the runtime excludes both that field and the `romtool`-maintained Kickstart checksum longword right after it.
4. `chooser_loop()` as above.

Platform-facing interfaces are `glyph.h`, `kbd.h`, `platform.h`, `term.h`; `src/common` must not know anything else about the platform. Where the platforms differ:

- **Display**: ST uses the shifter directly (screen memory at `ST_SCREEN_BASE_ADDR`, 32000 B). Amiga uses a hires 2-bitplane copper/framebuffer setup written in a DiagROM-inspired structure; do not reintroduce ad hoc diagnostic branches (`SCREEN_DIAG_STAGE` and friends were deliberately removed) and do not regress `glyph.c` to a single-plane renderer.
- **Keyboard**: ST polls the IKBD ACIA and filters non-keyboard packets. Amiga polls CIAA with the handshake; `kbd_wait_for_key_press()` must react to *any* key, not only mapped ones.
- **Memory layout**: constants live in `src/st/mem.h` (with `_Static_assert` guards) and `src/amiga/mem.h`. On Amiga, `startup.s`, `mem.h` and `rom_abs.ld.S` must stay in sync; runtime code executes **from RAM** after boot, so never assume ROM addresses for code except `_platform_hard_reset`.
- **Text**: `text_printf()` implements cursor movement, erase/clear, save/restore cursor, wrap control, reverse video and foreground colour. `ESC c n`, `ESC e`, `ESC f` are parsed but intentionally not implemented. `text_run_feature_tests()` must stay in the tree even though nothing calls it.

## Build internals

The toolchain is Docker-based and pinned in `tools/dev/toolchain.sh`, the same arrangement as sidecartos-config's (EPIC-00 STORY-01). `logronoide/atarist-toolkit-docker-x86_64:1.4.0`, pinned by digest, provides GCC 4.6.4 and binutils 2.30; `m68k-atari-mint-gcc` compiles **all three** images with `-m68000 -std=gnu99 -ffreestanding -nostdlib -Wall -Wextra -Werror`. `romemul-switcher-rom/romtool:1` (amitools 0.8.1 on a pinned `python:3.12-slim`, built from `tools/dev/docker/romtool/` on first use) runs `scripts/finalize_rom.py` and `romtool`. Nothing compiles natively, and the host needs only Docker, git and bash.

`tools/dev/build.sh <platform> <type>` empties `tools/dev/builds/<platform>-<type>/` inside the container, runs `make -C src/st|amiga` with `BUILD_DIR` there and the build ID, then finalizes in the romtool container. The output folder holds the finalized `RESCUE_SWITCHER_v<ver>_<size>KB.img`, `ROMSWITC.PRG` for ST/STE, the link maps, and `obj/`. `./build.sh` is the release path: it calls `tools/dev/build.sh <platform> release` and copies the published files to `dist/<platform>/`; it refuses `debug` and `test`.

The **build ID** is `<sha7>`, then `-dirty.<diff7>` when `src/`, `Makefile` or `version.txt` differ from HEAD, then `+debug`, `+test` or `+debug+test`. It is computed on the host by `rescue_build_id` and reaches C as `BUILD_ID_STR`; a direct `make` gets `unknown`.

Internal 8.3 names are enforced: `ROMSWITC.PRG/.BIN/.MAP` (release), `ROMSWDBG.*` (debug), `ROMSWROM.MAP` (ROM link map), `ROMSWAMI.MAP` / `ROMAMDBG.MAP` (Amiga). A direct `make` writes to `build/st`, `build/st-debug`, `build/st-test`, `build/st-debug-test` and the `amiga*` equivalents, unfinalized.

| mode | flags | effect |
|---|---|---|
| default | `-O2` | real hardware path |
| `debug` | `-O0 -g -D_DEBUG=1` | Hatari NatFeats tracing on ST (`htrace.c`), extra `text_printf` diagnostics |
| `test` | `-D_TEST=1 -DTEST=1` | catalog/params reads come from `src/common/test.c` arrays instead of the bus. The arrays are `const`, which this a.out toolchain puts in `test.c.o`'s `.text`, and both `rom_abs.ld.S` route that file to a `.romdata` section in ROM, so the RAM layout matches a release build. The embedded params carry the protocol version at byte 512, so bump it together with `kSwitcherTosProtocolVersion` |

**Image finalisation** (`scripts/finalize_rom.py`, run in the romtool container): pad to the exact size (192/256/512 KB) with **random bytes**, never zeros; then write the 32-bit big-endian additive checksum. ST/STE: field at `size-4`. Amiga: payload must end before the **kickety-split** marker at `0x40000` (checked against `__rom_payload_end` in the map), random fill on both sides of it, checksum field at `0x7FFE4` excluding the 4-byte Kickstart checksum at `0x7FFE8`, then `romtool copy -c` fixes the footer and `romtool info` validates. Published artifacts use only canonical names (`RSWIT192.PRG`, `RSWIT256.PRG`, `RESCUE_SWITCHER_v<ver>_<size>KB.img`); do not reintroduce timestamped copies.

A direct `make` needs the variables `tools/dev/build.sh` passes, for example:

```bash
STCMD_NO_TTY=1 ST_WORKING_FOLDER=$PWD stcmd make st ROM_BASE_ADDR_UL=0x00E00000UL STARTUP_ROM_ASM=startup_ste.s
```

Adding a `.c` file: append it to `SRCS_C` in `src/st/Makefile` and/or `src/amiga/Makefile` (explicit lists, `VPATH` covers `../common`). Header dependencies are generated by the compiler (`-MMD -MP`, included at the end of each makefile), and the build-config stamp carries the version and the build ID, so changing either rebuilds everything.

**Code style checks**: `make format`, `make format-check`, `make tidy`, `make check` run `scripts/clang-checks.sh` over `src/**/*.{c,h}` with the repo `.clang-format` (Google-based, 2-space) and `.clang-tidy`. It looks for LLVM under `/usr/local/opt/llvm/bin` first, else `PATH`, and needs `rg`. `.clang-tidy-ignore` was copied from the firmware repo and lists paths that do not exist here.

**Release**: `make tag` pushes `v<version.txt>`; `release.yml` then runs `./build.sh all` and uploads only `dist/{st,ste,amiga}/*.img` to the GitHub Release with the top `CHANGELOG.md` section as body. No S3/AWS step.

## Conventions and gotchas

- **Source header block**: every `.c`, `.h`, `.s` under `src/` starts with the `File / Author / Date / Copyright / Description` comment block (see `src/common/rom_check.c`). Keep it on new files.
- **Freestanding discipline**: no libc, no OS traps, no `LIBS`, and **no libgcc**. A 32-bit `/` or `%` that GCC cannot fold becomes a call to `___udivsi3` / `___umodsi3` and the link fails with "undefined reference" (at `-O2` it is usually folded away, at `-O0` it is not, so this shows up only in `debug` builds). Use shifts/masks for powers of two or the hand-written `div_u32_u16` in `commands.c`; likewise `copy_bytes` and the byte-swap helpers instead of `<string.h>`. Build every change with `debug-test` too, which compiles at `-O0`.
- **No named sections**: the objects are a.out, so `__attribute__((section(...)))` fails with "section attributes are not supported for this target". Place data by file name in the linker scripts instead, as `.romdata` does for `test.c.o`.
- **Trace lines** (`src/common/trace.h`): `TRACE("fmt", ...)` prints `@@ <line>` in debug builds and compiles to nothing in release. The harnesses match these lines exactly, so changing one means changing `tools/dev/harness_common.py` too. Every blocking key read is preceded by `TRACE("waitkey...")`.
- **Docker file sharing on macOS**: a file the host writes into a folder a container has just recreated can stay invisible to the next container for a while. Keep write-then-read steps on the container side, as `tools/dev/build.sh` does with the finalizer.
- `dist/` keeps images of earlier versions; `./build.sh` replaces only the current version's files.
- **Word alignment**: the 68000 faults on odd word access; keep buffers that are read as `unsigned short`/`unsigned long` aligned (the `palloc` heap base is rounded to 4).
- **Endianness**: the Pico stores catalog/params little-endian; `read_flash_page(..., ENDIAN_BIG)` swaps words, and `chooser.c` still reads multi-byte fields byte-wise. Mirror what the firmware expects when adding a field.
- **Memory constants**: changing anything in `mem.h` means updating the startup asm and `rom_abs.ld.S` together, on both platforms if shared.
- **Names that must not change**: `rom_check.c/.h` stays as is (it was renamed from `rom_crc` when the algorithm became an additive checksum; do not rename it back). `text_run_feature_tests()` stays in the tree.
- **ST linker script** is generated into `build/<mode>/rom_abs.ld` through a `.tmp` file and then renamed, so a failed preprocess never leaves a stale script behind. Keep that pattern if you touch the rule.
- **Amiga reset**: keep the true cold-reset stub in `startup.s` ROM code; nothing in relocated RAM may be relied on after `platform_hard_reset()`.
- Naming inside files is mixed (`snake_case` in `common/`, some `camelCase` and `kConstant` enums in `switcher.c`). Match the file you edit; do not reformat wholesale.
- `build/`, `dist/`, `*.o`, `*.map`, `*.bin` are generated and git-ignored; exclude them from searches.

## Backlog

Work is tracked in `docs/epics/`, the same procedure as the firmware's and the host SWITCHER's
(`../sidecartos/docs/epics/`, `../sidecartos-config/docs/epics/`). `ITERATIONS.md` is the plan
and its order, `EPIC-NN-<slug>/epic.md` and `STORY-NN-<slug>.md` hold the work, `DECISIONS.md`
holds the `D-NN` decisions and `C-NN` constraints, and `STATUS.md` is generated by
`./docs/epics/cockpit.sh`, run after editing any epic or story. The rules are in
`docs/epics/README.md`: one epic at a time, each ending in a verification story that is Diego's
checkpoint, and a task is checked only when observed. The backlog is committed (D-01); code is
not committed until Diego says so. Branches (D-05): the version lives on `release/v4.0.0`, each
epic on `epic-NN-<slug>` cut from it and merged back by pull request, and the release branch is
merged into `main`, where the `v4.0.0` tag is pushed.

Iteration 1 is v4.0.0: EPIC-00, the Hatari and FS-UAE harness that boots the three images as
system ROMs (D-06), done; EPIC-01, Diego's look at every screen with `--interactive`; EPIC-02,
the release gate (notes, CI, merges, the `v4.0.0` tag, the firmware's re-pin). The open defects found on 2026-10-09 are listed as
candidates in `ITERATIONS.md`: the constant nonce, the Amiga reset racing the firmware reboot
(C-07), the mono cold boot on ST/STE, the unbounded catalog walk, and a debug image on hardware.
EPIC-00 STORY-01 moved every build out of tree (`tools/dev/build.sh`), which ended the two
`build.sh` flaws: debug images published to `dist/`, and stale images re-finalized.

## Working style

These behavioral guidelines bias toward caution over speed. For trivial tasks, use judgment.

### 1. Think before coding

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them — don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity first

Minimum code that solves the problem. Nothing speculative.
- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical changes

Touch only what you must. Clean up only your own mess.
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it — don't delete it.
- When your changes orphan an import/variable/function, remove it. Don't remove pre-existing dead code unless asked.

The test: every changed line should trace directly to the user's request.

### 4. Goal-driven execution

Define success criteria. Loop until verified.
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan with a verification check per step. For this repo the check is `tools/dev/all_harness.sh` (it builds the debug-test images itself) plus `tools/dev/build.sh <platform> release` for `st`, `ste` and `amiga`. `./build.sh` replaces `dist/`, so tell the firmware first (C-06).

### 5. No AI attribution

Never add AI-tool attribution to commits, PR descriptions, code comments,
docs, or any other artifact. This means **no**:
- "Generated with Claude Code", "Co-authored by Claude", "Made with ChatGPT",
  or any similar phrasing.
- `Co-Authored-By: Claude …`, `Co-Authored-By: ChatGPT …`, or any other
  AI co-author trailer.
- "AI-assisted", "written with the help of an LLM", etc., as comments or
  changelog entries.

Write the message as the human author. Do not mention AI tools used to
produce the work.
