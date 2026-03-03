# AGENTS.md

This file documents the current project context for coding agents working on this repository.

## Project Purpose
- Atari ST ROM switcher UI and keyboard control logic.
- Built as a freestanding m68k binary (`.PRG` + `.BIN`) using `atarist-toolkit-docker` via `stcmd`.

## Build And Run
- Platform argument is mandatory:
  - `./build.sh [st|ste|amiga] [debug] [test]`
- Also supported:
  - `./build.sh st`
  - `./build.sh ste`
  - `./build.sh ste debug`
  - `./build.sh ste test`
  - `./build.sh ste debug test`
- Platform-specific behavior:
  - `st` sets `ROM_BASE_ADDR_UL=0x00FC0000UL` and `STARTUP_ROM_ASM=startup_st.s`.
  - `ste` sets `ROM_BASE_ADDR_UL=0x00E00000UL` and `STARTUP_ROM_ASM=startup_ste.s`.
  - `amiga` currently exits with `amiga build is not implemented yet`.
- `build.sh` is the required validation gate after code changes.
- `ROM_BASE_ADDR_UL` has no Makefile default and must be provided by caller.

## Repository Layout
- `src/st/`
  - ST-specific startup and hardware access.
  - `start.s`: entry point, supervisor transition, stack setup, jump to `rom_switcher_main`.
  - `screen.c/.h`: shifter/video mode/palette/screen memory register-level operations.
  - `kbd.c/.h`: IKBD ACIA polling (keyboard-only filtering).
  - `glyph.c/.h`: glyph fetch + glyph plot (ST bitplane renderer).
  - `switcher.c`: main loop and menu rendering.
  - `term.h`: `TERM_*` aliases mapping terminal concepts to ST memory/register storage.
- `src/common/`
  - Cross-module/common logic.
  - `chooser.c/.h`: ROM catalog parsing and paginated chooser UI logic.
  - `commands.c/.h`: ROM-emulator command protocol and flash page/block reads.
  - `palloc.c/.h`: freestanding allocator used by ST app code.
  - `text.c/.h`: text output, printf, VT-52 escape handling.
  - `font8x8.h`: font table.

## Runtime And Memory Model
- Memory layout constants are centralized in `src/st/mem.h`.
- Current stack top: `0x00078000` (`ST_STACK_TOP_ADDR_UL`).
- Screen base: `0x00078000` (`ST_SCREEN_BASE_ADDR_UL`).
- Screen size: `32000` bytes.
- RAM vars block: base `0x00058000`, size `124 KB` (`ST_RAM_VARS_SIZE_BYTES`).
- Layout guards are compile-time checked in `mem.h`.
- `rom_switcher_main` masks IRQ levels (`IPL=7`) for polled runtime behavior.

## Input Handling Notes
- Keyboard polling uses IKBD ACIA directly in `kbd.c`.
- ACIA is put in polling mode and mouse is disabled (`IKBD command 0x12`).
- Poller filters non-keyboard IKBD packets (mouse/joystick/report frames).

## Text/Terminal Notes
- `text_putc` includes a VT-52 parser used by both `text_puts` and `text_printf`.
- Supported escape behavior includes cursor movement, clear/erase ops, save/restore cursor, wrap on/off, reverse on/off, and foreground set (`ESC b n`).
- `ESC c n` parameter is consumed but behavior is intentionally not implemented.
- `ESC e` / `ESC f` cursor show/hide are intentionally not implemented.

## Rendering Notes
- Glyph path is split by mode in `glyph_plot`:
  - Medium mode (primary target) optimized and loop-unrolled.
  - Mono mode also loop-unrolled.
- Keep VRAM writes deterministic and freestanding-safe; avoid introducing libgcc helper dependencies unless linked intentionally.

## Versioning
- Program version is sourced from top-level `version.txt`.
- `src/st/Makefile` passes it as `-DAPP_VERSION_STR="..."`.
- Menu title line is centered at runtime using this version string.

## Output Naming Constraint
- Internal Atari ST 8.3 naming requirement is enforced in ST Makefile:
  - `ROMSWITC.PRG`, `ROMSWITC.BIN`, `ROMSWITC.MAP`.
- Release artefacts are published by `build.sh` into `dist/<platform>/`:
  - `dist/st/RSWIT192.PRG`
  - `dist/st/RESCUE_SWITCHER_v<version>_192KB.img`
  - `dist/ste/RSWIT256.PRG`
  - `dist/ste/RESCUE_SWITCHER_v<version>_256KB.img`
- `.img` files are zero-padded to 4KB alignment.

## Change Guidelines
- Prefer register-level operations; avoid TOS/GEMDOS runtime dependencies unless explicitly required.
- Keep `src/st` for hardware/platform-specific code and `src/common` for reusable logic.
- If changing memory constants, update both `mem.h` and startup assumptions together.
- Always validate with `./build.sh <platform>` after edits.
- When edits touch debug/test conditionals or build flags, also validate with `./build.sh <platform> debug test`.
