# AGENTS.md

This file documents the current project context for coding agents working on this repository.

## Project Purpose
- Bare-metal ROM switcher UI for:
  - Atari ST
  - Atari STE
  - Amiga 500/2000
- No TOS, GEMDOS, Kickstart, or AmigaOS calls are allowed in the runtime paths.
- All platform code must access hardware directly.

## Current High-Level Architecture
- `src/common/` contains the shared chooser, ROM protocol, allocator, text rendering, and shared platform interfaces.
- `src/common/rom_check.c/.h` contains the shared ROM integrity check implementation.
- `src/st/` contains Atari ST and STE hardware-specific code.
- `src/amiga/` contains Amiga 500/2000 hardware-specific code.
- Shared platform-facing interfaces now live in:
  - `src/common/glyph.h`
  - `src/common/kbd.h`
  - `src/common/platform.h`
  - `src/common/term.h`
- `src/common` should not include platform-private ST or Amiga headers directly.

## Build And Validation
- Main entry point:
  - `./build.sh [st|ste|amiga|all] [debug] [test]`
- Supported examples:
  - `./build.sh st`
  - `./build.sh ste`
  - `./build.sh amiga`
  - `./build.sh amiga debug`
  - `./build.sh amiga test`
  - `./build.sh all`
  - `./build.sh all debug`
  - `./build.sh all test`
- `build.sh all` builds `st`, `ste`, and `amiga` sequentially.
- `build.sh` is the required validation gate after code changes.
- `build.sh` now publishes only canonical artifact names. Do not reintroduce timestamped artifact copies.
- `ROM_BASE_ADDR_UL` has no Makefile default and must be passed by the caller or `build.sh`.
- `version.txt` is the single version source. Both ST and Amiga Makefiles pass it as `-DAPP_VERSION_STR="..."`.
- `version.txt` may include a leading `v`; build logic strips it before composing artifact filenames and `APP_VERSION_STR`.

## Platform Build Parameters
- `st`
  - `ROM_BASE_ADDR_UL=0x00FC0000UL`
  - `STARTUP_ROM_ASM=startup_st.s`
- `ste`
  - `ROM_BASE_ADDR_UL=0x00E00000UL`
  - `STARTUP_ROM_ASM=startup_ste.s`
- `amiga`
  - `ROM_BASE_ADDR_UL=0x00F80000UL`
  - Builds a `512 KB` Kickstart-replacement ROM image

## Repository Layout
- `src/common/`
  - `chooser.c/.h`: ROM catalog parsing, pagination, menu flow.
  - `commands.c/.h`: ROM-emulator command protocol, page/block reads, reset flow.
  - `palloc.c/.h`: freestanding bump allocator.
  - `text.c/.h`: terminal-style text output and VT52-like control handling.
  - `rom_check.c/.h`: shared ROM checksum verification.
  - `font8x8.h`: shared 8x8 font data.
  - `test.c/.h`: embedded test catalog and parameter data used by `_TEST` builds.
- `src/st/`
  - `start.s`: ST entry point, supervisor transition, stack setup.
  - `startup_st.s` / `startup_ste.s`: ROM startup entry for ST and STE.
  - `screen.c/.h`: shifter/video mode/palette/screen memory access.
  - `kbd.c`: IKBD ACIA keyboard polling.
  - `glyph.c`: ST glyph renderer.
  - `platform.c`: ST ROM magic-sequence transport and hard reset path.
  - `htrace.c/.h`: Hatari trace support.
- `src/amiga/`
  - `startup.s`: ROM bootstrap, overlay fix, RAM relocation, and ROM-resident hard reset stub.
  - `screen.c/.h`: Amiga display setup and framebuffer control.
  - `kbd.c`: CIAA keyboard polling and keyboard handshake.
  - `glyph.c`: Amiga glyph rendering.
  - `platform.c`: Amiga ROM magic-sequence transport and platform helpers.
  - `mem.h`: Amiga runtime memory layout.
  - `hw.h`: custom-chip and CIA register definitions.
  - `rom_abs.ld.S`: fixed Amiga Kickstart ROM layout.

## Atari ST And STE Notes
- Internal 8.3 build names are enforced:
  - `ROMSWITC.PRG`
  - `ROMSWITC.BIN`
  - `ROMSWITC.MAP`
- Final ROM image sizes are now exact, not generic 4 KB rounding:
  - `st` is padded to `192 KB`
  - `ste` is padded to `256 KB`
- ST/STE ROM slack is no longer zero-filled. Unused bytes are filled with random data during post-build finalization.
- ST/STE store a 32-bit big-endian checksum in the last 4 bytes of the ROM image.
- The ST linker script generation in `src/st/Makefile` writes through a temp file and then renames it, to avoid stale `rom_abs.ld` content.
- ST runtime remains fully polled and bare-metal.
- Keyboard input uses IKBD ACIA directly and filters non-keyboard packets.
- `src/st/switcher.c` derives the displayed model name from `ROM_BASE_ADDR_UL`:
  - `0x00FC0000UL` -> `Atari ST`
  - `0x00E00000UL` -> `Atari STE`

## Amiga Notes
- Target machines are Amiga 500 and Amiga 2000.
- ROM-emulator protocol is the same as the ST path, but the ROM window is `0xF80000` and magic values use the `F8` prefix.
- The Amiga image is a full `512 KB` Kickstart-replacement ROM:
  - base `0x00F80000`
  - kickety split marker at the midpoint
  - footer/checksum validated with `romtool`
- Amiga ROM slack is filled with random data during post-build finalization.
- Amiga stores a dedicated 32-bit big-endian checksum field immediately before the footer.
- The Amiga runtime checksum excludes both:
  - the dedicated stored checksum field
  - the Kickstart checksum longword maintained by `romtool`
- `startup.s` is critical:
  - it must clear the low-memory ROM overlay before touching RAM
  - it copies runtime code from ROM to RAM
  - it copies `.data` to RAM and clears `.bss`
  - `_platform_hard_reset` lives in ROM `.boot`, not in relocated RAM code
- Amiga runtime code is intentionally relocated to RAM after boot. Do not assume normal code continues executing from ROM after startup.
- The current Amiga screen target is `80` columns in hires mode using `2` bitplanes:
  - `640`-wide display
  - `4` colors
  - palette intent matches Atari ST medium-resolution colors
- Amiga glyph rendering must write the active bitplanes correctly. Do not regress it to a single-plane renderer.
- `screen.c` was rewritten around a cleaner DiagROM-inspired structure. Avoid reintroducing old ad hoc diagnostic branches there.

## Runtime And Memory Model
- ST memory layout constants are centralized in `src/st/mem.h`.
- ST layout guards are compile-time checked in `mem.h`.
- `rom_switcher_main` runs with interrupts masked for fully polled behavior.
- Amiga memory layout constants are centralized in `src/amiga/mem.h`.
- Amiga startup and memory assumptions must stay synchronized between:
  - `src/amiga/startup.s`
  - `src/amiga/mem.h`
  - `src/amiga/rom_abs.ld.S`

## Input Handling Notes
- ST:
  - direct IKBD ACIA polling
  - keyboard-only filtering
- Amiga:
  - direct CIAA keyboard polling and handshake
  - `kbd_wait_for_key_press()` must react to any key press, not only mapped menu keys
- Shared chooser confirmation paths now depend on the common keyboard interface, not platform-private key assumptions.

## Text And UI Notes
- `text_printf()` and the shared text path are still core to the menu UI.
- `text_run_feature_tests()` must remain in the tree. Do not delete it, even if it is not auto-invoked.
- Both ST and Amiga perform a full-ROM checksum verification before entering `chooser_loop()`.
- The integrity screen shows a spinner while the checksum runs.
- On mismatch, the stored and computed values are shown and boot continues only after a key press.
- Supported control behavior includes cursor movement, erase/clear operations, save/restore cursor, wrap control, reverse video, and foreground color selection.
- `ESC c n` is parsed but intentionally not implemented.
- `ESC e` and `ESC f` cursor show/hide are intentionally not implemented.

## Test Build Notes
- `_TEST` builds still compile and link test data from `src/common/test.c`.
- In test mode, chooser/catalog reads are redirected to embedded test data rather than ROM-emulator hardware.
- `build.sh all test` should pass. A previous ST padding issue in test mode was fixed by skipping release-image repadding when `TEST_MODE=1`.

## Output Artifacts
- Release artifacts are published under `dist/<platform>/`.
- `test` builds do not publish to `dist/`; only non-test builds do.
- ST artifacts:
  - `dist/st/RSWIT192.PRG`
  - `dist/st/RESCUE_SWITCHER_v<version>_192KB.img`
- STE artifacts:
  - `dist/ste/RSWIT256.PRG`
  - `dist/ste/RESCUE_SWITCHER_v<version>_256KB.img`
- Amiga artifacts:
  - `dist/amiga/RESCUE_SWITCHER_v<version>_512KB.img`
- GitHub release uploads are driven from `dist/` and currently upload only the canonical `.img` files for `st`, `ste`, and `amiga`.
- The release workflow no longer uploads artifacts to AWS/S3.

## Source Header Convention
- All `.c`, `.h`, and `.s` files under `src/` now carry the standard project header block:
  - file
  - author
  - date
  - copyright
  - short description
- Preserve that format when creating new source files.

## Change Guidelines
- Prefer register-level operations. Do not add TOS, GEMDOS, Kickstart, or AmigaOS runtime dependencies.
- Keep `src/common` platform-agnostic and push hardware details into `src/st` or `src/amiga`.
- If changing memory constants, update startup, linker, and runtime assumptions together.
- For Amiga reset work, keep the true cold-reset stub in `startup.s` ROM code.
- Avoid reintroducing removed diagnostic scaffolding such as `SCREEN_DIAG_STAGE`.
- Do not rename `rom_check.c/.h` back to `rom_crc.c/.h`; the project now uses an additive checksum, not CRC32.
- Always validate with:
  - `./build.sh <platform>`
- When changing shared code, build matrices should include:
  - `./build.sh all`
  - `./build.sh all debug`
  - `./build.sh all test`
- Do not run `st` and `ste` builds in parallel against the same worktree; both publish through `build/st/`.
