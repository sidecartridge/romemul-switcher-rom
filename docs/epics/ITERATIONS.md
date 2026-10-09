# Iterations

Epics are grouped into iterations via the `iteration: N` frontmatter field; the cockpit groups the
dashboard by it. This file is the narrative: what each iteration sets out to do, the order it is
done in and why, and how it ends. One iteration per release.

| Iteration | Theme | Status |
| --- | --- | --- |
| 1 | **v4.0.0**: the rescue ROM that ships in firmware v4.0.0's release images, protocol 0x40, exercised in Hatari and FS-UAE and seen on hardware | in progress |

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
| EPIC-01 | The three images by eye, before the release | Diego, 2026-10-09: the `--interactive` looks, taken out of EPIC-00, come before the release gate. The harnesses check the trace, not the picture, and type on the Amiga through the serial port; only this sees every screen and the Amiga's CIA keyboard path. |
| EPIC-02 | The v4.0.0 release gate | Last: the release notes, the first CI runs of EPIC-00's build path, the merges and the `v4.0.0` tag (Diego's step), and the firmware's re-pin from the published images (C-06). |

Any candidate below that Diego chooses for v4.0.0 becomes an epic that runs before EPIC-01 and
EPIC-02; since neither has started, they are renumbered then rather than leaving the numbers
out of execution order.

### Candidates, not yet epics

Found on 2026-10-09 in the review of the ST video path, the sync with the two sibling sessions,
and EPIC-00. None is approved yet; each waits for Diego to choose it into an epic.

- **The nonce never changes during a run.** In ROM mode the ST seed samples MFP timer C and D,
  but `startup_st.s`/`startup_ste.s` execute `reset`, which stops the MFP timers, and nothing
  restarts them; the Amiga seed is the long at address 4, a fixed RAM value. So every frame of a
  run carries the same nonce. The lock after each `UNLOCK_READ_BLOCK` restores base+0 and hides
  most of it, but a retry after a checksum mismatch, or a read after a failed restore, can take a
  stale echo, and with it the previous block's data, which carries a valid checksum of its own.
  The firmware never checks the nonce, so the fix is ours alone. The host SWITCHER has the same
  bug (sidecartos-config EPIC-03).
- **The Amiga reset races the firmware's reboot** (C-07). `_platform_hard_reset` resets and
  jumps to ROM offset 2 microseconds after `SELECT_ROM`, so the Amiga re-enters this switcher
  from the old image and meets the undriven bus partway through its boot. The ST waits about
  2.9 s first and is fine. A fix needs a wait of at least 0.5 s run from RAM and then a reset
  that does not depend on the old image, which contradicts `CLAUDE.md`'s rule to keep the reset
  stub in ROM: Diego's call.
- **ST/STE cold boot on a monochrome monitor** can come up in medium resolution: `screen_init()`
  reads MFP GPIP bit 7 once, right after startup, and never again. A warm reset works. Fix:
  sample after a short delay until the bit is stable. Hatari can check the mono render path, not
  the power-on race (C-03).
- **The catalog walk is unbounded.** `parse_rom_description()` stops only at a record whose first
  byte is 0x00; an erased catalog (all 0xFF, a flash never reindexed) makes it walk past its
  61,440-byte buffer. Mirror sidecartos-config `448fd03`: stop at 240 records and at a first
  byte of 0x00 or 0xFF.
- **The ST keyboard driver drops keys on an ACIA overrun.** `src/st/kbd.c` discards any byte
  read with an error flag set, overrun included, although on an overrun the byte in the
  receive register is valid. Its empty-poll delay of 2,500 loops makes overruns likely: about
  28 ms between polls in a debug build, measured in Hatari (EPIC-00 STORY-02). A key pressed
  within a poll gap of the previous release is lost. Fix: keep the byte on overrun, and poll
  without the long delay.
- **A debug image must not reach hardware.** `natfeat.s` detects Hatari's native features with
  the illegal opcode 0x7300, which Hatari intercepts; on a real ST in ROM mode it traps through
  an exception vector nothing has set. Debug images are for Hatari only, or the debug build
  installs a vector that answers "no native features".
- **Hardware verification of v4.0.0**: the three images on the RP2350A and the RP2350B, each
  booted in Rescue Mode, listing, selecting and booting a ROM. No epic yet; if chosen, it runs
  before EPIC-02. The release notes and the release gate itself are EPIC-02.

**Outcome:** _pending._
