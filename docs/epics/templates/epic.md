---
id: EPIC-NN
iteration: 1   # which iteration this epic belongs to (see ITERATIONS.md)
title: <short capability name>
status: todo   # todo | in-progress | done | blocked | deferred | dropped
---

## Goal

What capability this epic delivers and why it matters for the rescue ROM switcher.

## Scope

- In scope: ...
- Out of scope: ...

## Platforms

Which of the three images the change reaches (`st` 192 KB at `0xFC0000`, `ste` 256 KB at
`0xE00000`, `amiga` 512 KB at `0xF80000`), and therefore which machines and SidecarTridge boards
(RP2350A, RP2350B) the hardware-verification story must cover (C-02). Code in `src/common`
reaches all three.

## Stories

Stories live as `STORY-NN-<slug>.md` files in this folder. List them here for
context (the cockpit derives the real status from those files):

- STORY-01: ...
- STORY-02: ...

## Notes

Open questions, design decisions, links to code (`file:line`), the sibling items it depends on
(`firmware EPIC-NN`, `sidecartos-config EPIC-NN`).
