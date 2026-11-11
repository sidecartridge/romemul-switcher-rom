# Iterations

Epics are grouped into iterations via the `iteration: N` frontmatter field; the cockpit groups the
dashboard by it. This file is the narrative: what each iteration sets out to do, the order it is
done in and why, and how it ends. One iteration per release.

| Iteration | Theme | Status |
| --- | --- | --- |
| 1 | **v4.0.0**: the rescue ROM that ships in firmware v4.0.0's release images, protocol 0x40, exercised in Hatari and FS-UAE and seen on hardware | done 2026-10-10 |
| 2 | **v4.0.0 re-issued** (D-09): the defects carried over from the first issue, the Amiga reset after a selection, and the Amiga's bus chip in the self-test | planned |

---

## Where v3.1.0 left off

The `v3.1.0` tag (`3573c4e`, 2026-03-12; `CHANGELOG.md` dates it 2026-03-11) is the first version
for the Atari ST, the STE and the Amiga 500/2000, including the full-ROM checksum before the
chooser (pull request #2). After it, `main` gained two README commits (`a835b00`, `a67ea86`).

Then, before this backlog existed and therefore not tracked as epics, the working tree of `main`
gained the following, committed on 2026-10-09 as the start of `release/v4.0.0` (D-05):

- Protocol 0x40 and version v4.0.0 (D-04), Diego's request of 2026-09-09.
- `CLAUDE.md` replacing `AGENTS.md`, with the shared contract and the sync rule of D-03.

The `dist/` images built from that tree on 2026-09-09 are the ones the firmware pinned (C-06).
Nothing of v4.0.0 has been seen on a machine yet.

---

## Iteration 1: v4.0.0

**Goal:** Three rescue images, ST 192 KB, STE 256 KB and Amiga 512 KB, that talk protocol 0x40,
are seen working with firmware v4.0.0 on the RP2350A and the RP2350B, are exercised in Hatari
and FS-UAE before every bench pass (EPIC-00), and are published as the `v4.0.0` GitHub release
the firmware's release images download (firmware EPIC-11). This iteration ends before the
firmware's release gate, which waits for our release assets.

### Implementation path

Epic numbers are the execution order. One epic at a time, each on its own `epic-NN-<slug>`
branch merged into `release/v4.0.0` by pull request when done (D-05); each ends with a
verification story that is Diego's checkpoint.

| # | Epic | Why at this point |
| --- | --- | --- |
| EPIC-00 | A test harness on Hatari and FS-UAE that boots the ROM images | Asked by Diego, 2026-10-09, after sidecartos-config's EPIC-00 showed how much a host-driven emulator loop saves. First, as in both siblings: with it, every fix below is exercised on all three images in minutes before it reaches the bench. It also repairs the two things the loop needs and that are broken today: debug builds that link, and a `build.sh` that keeps debug output out of `dist/`. Done 2026-10-09: nine sessions in about a minute (`tools/dev/all_harness.sh`). |
| EPIC-01 | The three images by eye, before the release | Diego, 2026-10-09: the `--interactive` looks, taken out of EPIC-00, come before the release gate. The harnesses check the trace, not the picture, and type on the Amiga through the serial port; only this sees every screen and the Amiga's CIA keyboard path. Done 2026-10-09: every screen agreed by Diego. |
| EPIC-03 | A self-test of the device, from the rescue ROM | Asked by Diego, 2026-10-09: address and data lines, ping and command reliability, flash reads, catalog and configuration, machine information, run from the chooser, plus the nonce fix and a ping at boot. Runs before EPIC-02, which keeps its number because the firmware session already cites it. It changes the images, so it comes before the release gate and the firmware's final pins. Done 2026-10-10: agreed by Diego on both boards, the three machine families and the emulators. |
| EPIC-02 | The v4.0.0 release gate | Last: the release notes, the first CI runs of EPIC-00's build path, the merges and the `v4.0.0` tag (Diego's step), and the firmware's re-pin from the published images (C-06). Done 2026-10-10: released from `main` `561b13d`, re-pinned by the firmware in its `454abda`. |

The table is the execution order; EPIC-03 runs before EPIC-02. A candidate below that Diego
chooses for v4.0.0 becomes an epic, or a story of EPIC-03, that runs before EPIC-02.

### Candidates, not yet epics

Found on 2026-10-09 in the review of the ST video path, the sync with the two sibling sessions,
and EPIC-00. None is approved yet; each waits for Diego to choose it into an epic.

- **The nonce never changes during a run.** Now EPIC-03 STORY-01 (Diego, 2026-10-09).
- **The Amiga reset races the firmware's reboot** (C-07). `_platform_hard_reset` resets and
  jumps to ROM offset 2 microseconds after `SELECT_ROM`, so the Amiga re-enters this switcher
  from the old image and meets the undriven bus partway through its boot. The ST waits about
  2.9 s first and is fine. A fix needs a wait of at least 0.5 s run from RAM and then a reset
  that does not depend on the old image, which contradicts `CLAUDE.md`'s rule to keep the reset
  stub in ROM: Diego's call. Now EPIC-05 (iteration 2, 2026-11-11).
- **ST/STE cold boot on a monochrome monitor** can come up in medium resolution: `screen_init()`
  reads MFP GPIP bit 7 once, right after startup, and never again. A warm reset works. Fix:
  sample after a short delay until the bit is stable. Hatari can check the mono render path, not
  the power-on race (C-03). Now EPIC-03 STORY-14 (Diego, 2026-10-10): the photos showed a
  mono display wrapped by 4 bytes, the resolution set mid-frame after the reset; it is now set
  in the vertical blank, and the monitor line read after two frames.
- **The catalog walk is unbounded.** `parse_rom_description()` stops only at a record whose first
  byte is 0x00; an erased catalog (all 0xFF, a flash never reindexed) makes it walk past its
  61,440-byte buffer. Mirror sidecartos-config `448fd03`: stop at 240 records and at a first
  byte of 0x00 or 0xFF. Now EPIC-04 STORY-02 (iteration 2, 2026-11-11).
- **The ST keyboard driver drops keys on an ACIA overrun.** `src/st/kbd.c` discards any byte
  read with an error flag set, overrun included, although on an overrun the byte in the
  receive register is valid. Its empty-poll delay of 2,500 loops makes overruns likely: about
  28 ms between polls in a debug build, measured in Hatari (EPIC-00 STORY-02). A key pressed
  within a poll gap of the previous release is lost. Fix: keep the byte on overrun, and poll
  without the long delay. Now EPIC-04 STORY-03 (iteration 2, 2026-11-11).
- **A debug image must not reach hardware.** `natfeat.s` detects Hatari's native features with
  the illegal opcode 0x7300, which Hatari intercepts; on a real ST in ROM mode it traps through
  an exception vector nothing has set. Debug images are for Hatari only, or the debug build
  installs a vector that answers "no native features". Now EPIC-04 STORY-04 (iteration 2,
  2026-11-11).
- **The build line collides with the loading line.** Fixed in EPIC-03 STORY-10, on the screen
  that story changed.
- **Hardware verification of v4.0.0**: the three images on the RP2350A and the RP2350B, each
  booted in Rescue Mode, listing, selecting and booting a ROM. No epic yet; if chosen, it runs
  before EPIC-02. The release notes and the release gate itself are EPIC-02. First evidence,
  reported by the firmware session on 2026-10-09: the `0d827d3` images, pinned in firmware
  `16b66e1` for its test release images, ran on both boards on the Atari ST and the Amiga
  (Diego: "works great"), the rescue ROM booted both from the RESCUE input and through
  `RESCUE_TIMEOUT`. Not yet known from that report: whether a ROM was selected and booted on the
  Amiga, which is where the reset race of C-07 would show. Covered since by EPIC-03 STORY-15
  (Diego, 2026-10-10): both boards, an ST, a Mega ST, an STE and an Amiga, a selection booting
  "several times", the Amiga included.

**Outcome:** _pending._

---

## Iteration 2: v4.0.0 re-issued

**Goal:** New images published as `v4.0.0` again, replacing those of `561b13d` (D-09):
- the defects of the first issue fixed;
- an Amiga reset that waits for the firmware;
- a self-test that names the Amiga's bus chip, Gary or Gayle.

Exercised in Hatari and FS-UAE before every bench pass, seen on both boards, and re-pinned by
the firmware's own re-issued v4.0.0. Diego, 2026-11-11: "iteration 2 is going to overwrite
v4.0.0", then "write the epics and stories for the new iteration".

**Done 2026-11-11.** v4.0.0 re-issued from `main` `09bd579` (#7, #8), its images checked and
re-pinned by the firmware in its `68d6fb8` (on its `iteration-2` branch).

### Since v4.0.0

The screenshot build (`tools/dev/build.sh <platform> screenshot`) was added on 2026-10-11,
outside any epic. It came from the firmware session's request for the user docs' captures,
approved by Diego. It is uncommitted. The release images built from that tree are byte for byte
v4.0.0's.

### Implementation path

Epic numbers are the execution order. One epic at a time, each on its own `epic-NN-<slug>`
branch cut from a new `release/v4.0.0` and merged into it by pull request (D-05). Each epic ends
with Diego's checkpoint.

| # | Epic | Why at this point |
| --- | --- | --- |
| EPIC-04 | The defects carried over from v4.0.0 | First: four small fixes, none waiting on a sibling. Each is one file and a harness session: the error screens that now try again, the catalog walk, the ST keyboard overrun, and a debug image on hardware. |
| EPIC-05 | The Amiga reset waits for the firmware | C-07's race, the last open candidate of iteration 1. Amiga only, and needs Diego's call on replacing the ROM-stub rule of `CLAUDE.md`. |
| EPIC-06 | The Amiga's bus chip, Gary or Gayle, in the self-test | Asked by Diego through the firmware session, 2026-11-11, first as an A19 test with a 1 MB rescue image, then as the ROM size the Amiga takes. Neither can be read from the ROM: Gary mirrors the Kickstart at `$E00000` like a 1 MB machine, and only the socket's A19 wiring differs. Diego settled on naming the bus chip. No change to the shared contract. |
| EPIC-07 | The v4.0.0 re-issue gate | Last: the notes, the merges, moving the tag, and the firmware's re-pin. |

### Candidates, not yet epics

- **"Press any key to exit..." exits nowhere.** Now EPIC-04 STORY-01 (Diego, 2026-11-11). The incompatibility screen, "No ROMs found" and
  the flash read errors in `chooser_loop()` wait for a key and return. `rom_switcher_main()`
  then returns too, but every ROM startup enters it with `jmp`, so its `rts` pops whatever long
  sits at the top of the stack: the first long of screen memory on the ST and STE
  (`ST_STACK_TOP_ADDR_UL` is the screen base), the long at `0x80000` on the Amiga. What the
  machine does next is undefined, most likely a hang with the message still on screen. Read from
  the code on 2026-10-11, while answering the firmware session about the v4.0.0 user docs; not
  observed on the bench or in the emulators, where the release image reaches this path because
  no device answers. The v4.0.0 user docs tell users to switch off after any "exit" message.
  Added by Diego, 2026-10-11.
