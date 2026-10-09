# Epics, Stories & Tasks

The backlog for romemul-switcher-rom, the bare-metal rescue ROM switcher that the SidecarTridge
boots in Rescue Mode on the Atari ST, the STE and the Amiga 500/2000. It follows the same
procedure as the firmware's (`../sidecartos/docs/epics/`) and the host SWITCHER's
(`../sidecartos-config/docs/epics/`), so the three repositories track work the same way (D-03).
**This backlog is committed**, in a public repository, so the plan travels with the code
(D-01).

See `STATUS.md` for the live cockpit (generated; don't edit it by hand).

## Hierarchy

```
docs/epics/
  cockpit.sh          # regenerates STATUS.md from the files below
  STATUS.md           # generated dashboard (epics grouped by iteration)
  ITERATIONS.md       # iteration narrative: goal, order and why, outcome
  DECISIONS.md        # D-NN decisions + C-NN constraints
  templates/          # copy these when adding new work
    epic.md
    story.md
  EPIC-00-<slug>/
    epic.md           # the epic (carries `iteration: N`)
    STORY-01-<slug>.md
    STORY-02-<slug>.md
```

- **Iteration**: a pass with one overarching goal, grouping several epics; one iteration per
  release. Each `epic.md` carries `iteration: N`; the cockpit groups epics under it.
- **Epic**: a folder `EPIC-NN-<slug>/` with an `epic.md`. A coarse capability.
- **Story**: a file `STORY-NN-<slug>.md` inside an epic folder. A shippable slice of that epic.
- **Task**: a checkbox line inside a story, `- [ ]` open or `- [x]` done. The checkboxes drive the
  percentages in the cockpit.

**Numbers are the execution order**: epics within an iteration, and stories within an epic, are
done in number order, and `ITERATIONS.md` says why the order is what it is. Prefer adding over
renumbering once an epic has started.

Every work epic has the same shape: code-and-build stories, then **one hardware-verification
story, which is Diego's checkpoint**. Its `## Platforms` section says which of the three images
that story must cover (`st` 192 KB, `ste` 256 KB, `amiga` 512 KB), on which machine and with
which SidecarTridge board.

## Status field

```yaml
---
id: STORY-01
epic: EPIC-01
title: <short story title>
status: todo   # todo | in-progress | done | blocked | deferred
---
```

Keep `status` honest relative to the checkboxes: `todo` = no tasks done, `done` = all done,
`in-progress` = some, `blocked` = waiting (say why in the body), `deferred` = consciously postponed
to a later iteration.

## Working rules

- **Strictly sequential**: one epic at a time.
- **Checkpoints**: after each epic, work stops. Diego verifies on a real SidecarTridge in Rescue
  Mode, with the image booted on the machine it is built for, and approves before the next epic
  starts. Code in `src/common` is verified on **every image**: ST, STE and Amiga (C-02), and
  against every supported board, the RP2350A and the RP2350B.
- **A task is checked only when it was observed**, by whoever the task names. "Built" is not
  "verified on the ST".
- **Before and after numbers** for every size-, memory- or timing-related story (image payload,
  free ROM space, boot time, retries).
- **Emulators first** (D-06): once EPIC-00 is done, every code story runs the harnesses before
  it reaches Diego's bench. A harness PASS says nothing about the bus (C-03).
- **Branches** (D-05): the version lives on `release/vX.Y.Z` (today `release/v4.0.0`); each epic
  is developed on its own branch `epic-NN-<slug>`, cut from the release branch, and merged into
  it by pull request once the epic is done, verification story included; when the version is
  done the release branch is merged into `main` and the `v4.0.0` tag is pushed from `main`.
- **Nothing is committed until Diego says so.** Work is left in the working tree; he sequences the
  history. Pull requests only when he asks. The backlog itself (`docs/`) is committed (D-01).
- **The three repositories move together** (D-03): a change to the shared contract (protocol
  version, flash offsets, parameters page and catalog layout, bus commands, release names) is
  agreed with the firmware and the host SWITCHER first. Sibling items are cited with their
  repository: `firmware EPIC-11`, `firmware D-16`, `sidecartos-config EPIC-03`.
- **Tell the firmware before a rebuild lands in `dist/`** (C-06): its release images pin our
  images by SHA-256.
- **Run `./docs/epics/cockpit.sh`** after every change to a story or epic file: `STATUS.md` is the
  dashboard Diego reads.
- Copying an image onto the `ROMEMUL` volume, pointing `RESCUE.TXT` at it and booting it on the
  machine is Diego's step.
- No AI attribution anywhere (`CLAUDE.md`).

## Adding work

1. Copy `templates/epic.md` to `EPIC-NN-<slug>/epic.md`; give it an `iteration` and its platforms.
2. Copy `templates/story.md` into the epic folder once per slice, in execution order.
3. End the epic with a hardware-verification story covering every affected image and board.
4. Run `./docs/epics/cockpit.sh`.
