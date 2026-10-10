---
id: STORY-12
epic: EPIC-03
title: Every line within 80 columns
status: done
---

## Goal

No text runs past column 80 on any screen. A line that does wraps to column 0 of the next row,
and the next result line writes over it. Diego, 2026-10-10, after the first run on hardware:
"some text lines are longer than 80 chars and are not displayed properly. fix."

## Tasks

- [x] Find the lines: the Mega ST machine line (62 characters of details), the build line of a
      debug build, and failure lists (data lines, stress reads, soak, catalog with bad records,
      flash reads with large counts) can pass column 80; in the chooser, the protocol mismatch
      message (81), the out-of-memory messages (84) and "ROM image selected:" with a long name
      (up to 83)
- [x] The self-test writes every detail through one writer that wraps at spaces onto the next
      rows, at column 28; failure lists are built in a buffer first; the formatter into a
      buffer (`text_vsnprintf()`) is in every build, not only debug ones
- [x] The configuration's second and third rows go through the same writer; the soak's live
      line is bounded at 80 even with ten-digit counts; its result clears three rows first
- [x] The bottom of the self-test screen moved up a row (the summary, the S line, the soak) and
      the soak's row is capped at row 22, so a wrapped result stays on screen
- [x] The chooser's messages shortened: "This rescue switcher is not compatible with the
      device's firmware.", "Out of memory for the ... buffer.", "Selected ROM: " (14 characters
      and a name of at most 63)
- [x] A check: debug builds trace `@@ overflow row N` when a character follows an automatic
      wrap with no cursor move in between, and every harness session fails on it ("80 columns")

## Acceptance

Every harness session passes its "80 columns" step. The check fails when it should: with the
wrapping switched off, the Mega ST session failed with `@@ overflow row 3`.

## Notes

**Done 2026-10-10.**

- A line of exactly 80 characters is fine: the cursor wraps but nothing follows it before the
  next cursor move. The ROM list relies on that (69 + 11 characters).
- The check sees only the screens the harness walks: the boot screen, the list, the self-test,
  the soak, the confirmation. The chooser's error messages and a 63-character ROM name are
  bounded by construction, not by a session.
