# AGENTS.md

This file documents the current project context for coding agents working on this repository.

## Project Purpose
- Atari ST ROM switcher UI and keyboard control logic.
- Built as a freestanding m68k binary (`.PRG` + `.BIN`) using `atarist-toolkit-docker` via `stcmd`.

## Build And Run
- Preferred build command:
  - `./build.sh`
- Equivalent command used by the script:
  - `STCMD_NO_TTY=1 ST_WORKING_FOLDER=/Users/diego/mister_wkspc/romemul-switcher-rom stcmd make`

## Repository Layout
- `src/st/`
  - ST-specific startup and hardware access.
  - `start.s`: entry point, supervisor transition, stack setup, jump to `rom_switcher_main`.
  - `screen.c/.h`: shifter/video mode/palette/screen memory register-level operations.
  - `kbd.c/.h`: IKBD ACIA polling (keyboard-only filtering).
  - `glyph.c/.h`: glyph fetch + glyph plot (ST bitplane renderer).
  - `rom_switcher.c`: main loop and menu rendering.
  - `term.h`: `TERM_*` aliases mapping terminal concepts to ST memory/register storage.
- `src/common/`
  - Cross-module/common logic.
  - `text.c/.h`: text output, printf, VT-52 escape handling.
  - `font8x8_basic.h`: font table.
  - `romemul_hw.h`: ROM emulator hardware definitions.

## Runtime And Memory Model
- Current stack top: `0x00038000` (see `start.s` and `screen.h` constants).
- Screen base: `0x00038000`.
- Screen size: `32000` bytes.
- RAM vars block: base `0x0003FD00`, size `768` bytes.
- Layout guards are compile-time checked in `screen.h`.
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
- Atari ST 8.3 naming requirement is enforced in ST Makefile:
  - `ROMSWITC.PRG`, `ROMSWITC.BIN`, `ROMSWITC.MAP`.

## Change Guidelines
- Prefer register-level operations; avoid TOS/GEMDOS runtime dependencies unless explicitly required.
- Keep `src/st` for hardware/platform-specific code and `src/common` for reusable logic.
- If changing memory constants, update both startup and screen/term assumptions together.
- Always validate with `./build.sh` after edits.
