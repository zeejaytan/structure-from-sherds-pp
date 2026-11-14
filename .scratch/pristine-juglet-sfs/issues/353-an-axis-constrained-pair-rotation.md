# Ticket 353 — an axis-constrained pair rotation

**Status:** needs-info
**Waits for:** ticket 350 (how much of the pair error is tilt)
**Branch:** to be created at start
**Worker:** unassigned
**Answers:** S1
**Depends on:** 350, 349, 341/345
**Blocks:** nothing yet
**Budget:** medium (5 failed acceptance runs)
**Hand-over:** DeepSeek: one switch, four arms; only after 350's number is in.

> **In one line.** If ticket 350 shows the tilt is what costs the joins, form the pair rotation with
> the axis held instead of free, and see whether the joins land inside the bar.

## The design decision waits for 350

Ticket 350 measures what happens if the tilt is removed *after* the fact. If it shows three or more
joins coming inside the 5 deg bar, this ticket builds the same constraint *during* the fit. If it
shows the spin dominates, this ticket is re-aimed at the spin hypothesis instead and is not built as
written.

## Shape (to be finalised once 350 reports)

- one switch `SFS_AXIS_PAIR` (default off, banner `axis pair: off|on`), additions only;
- where the candidate rotation is formed (`class/feature_matching.cpp:2102-2107` for the
  registration path and the feature-cycle path before it), apply the minimal rotation that brings
  `L[index_m].axis_norm_[axis_m]` onto `L[index_f].axis_norm_[axis_f]` after the existing fit, or
  re-run the fit with that rotation fixed — the cheaper of the two that still keeps the
  correspondence's spin;
- no change to any gate, threshold, `TOP_k` or `BRANCH_b`; the 25 deg axis gate and the overlap rule
  stay exactly as they are, so the arms differ only in the candidate's rotation;
- arms: on r1, on r2 (signed 5/3, everything else unset), off r1 (must reproduce 2/9 sherds and the
  same 51 survivors), Pot A (8/8 sherds, 15/15 edges else the arm is void); tree md5 before/after;
- readings: `constrained-pairing-joins-more` / `constrained-pairing-changes-nothing` /
  `constrained-pairing-loses-the-spin`.

## Why it may not transfer

This run carries the key's own axes. Our own estimate is 1.5-87.8 deg off per piece (ticket 349), so
forcing the axis with *our* axes would bake that error in. The deployable version needs the axis
estimate settled first; the 350 result says only what the tilt is worth when the axes are right.

## Acceptance (provisional)

- [ ] the arm writes a `CAND`-comparable trace, so the change can be read candidate by candidate;
- [ ] the off arm reproduces the 51 survivors of the current tree, exactly;
- [ ] the accuracy line and `Pot A` control recorded; no gate touched;
- [ ] one reading; one `**End state:**` line.