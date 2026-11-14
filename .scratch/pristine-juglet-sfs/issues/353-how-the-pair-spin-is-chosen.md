# Ticket 353 - how the pair's spin is chosen

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 353-how-the-pair-spin-is-chosen, in /c/PR/worktrees/structure-from-sherds-pp/353
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Waits for:** nothing - ticket 350 landed 2026-11-14 and closed the axis question
**Answers:** S1
**Depends on:** 350 (the tilt buys two joins and the residue is a spin), 349 (the candidate ceiling), 348 (the measured contact set), 347 (the contact-curve runs)
**Blocks:** any change to how a pair's rotation is formed
**Budget:** small (3 failed acceptance runs). Offline arithmetic; no assembler run, no code change.
**Hand-over:** DeepSeek: laptop arithmetic over geometry already fetched.

> **In one line.** After the tilt is removed the pair rotation is still wrong by 3-14 deg about the
> shared axis; measure whether the breakline determines that spin at all, and whether the best spin
> would put the pair inside the bar.

## Why this is not an axis ticket

Ticket 350 closed the axis question: with the key's own axes, rotating each candidate onto its axis
moves 5 of 16 pairs inside the bar to 7, i.e. two joins (1-9, 5-8). The residue on the other nine is
a spin, so the axis is not where the fault lies and no constrained-axis build is worth making. What
is left to decide is whether that spin is *measurable* from the contact curve: if it is, the fit's
spin estimate is the fault and can be fixed in code; if it is not, no code change can recover those
joins and the lever is the data (longer or denser contact evidence, or a coarser object).

## Part A - the spin sweep

For each of ticket 350's 16 pairs (the 14 measured joins plus 4-7 and 4-6 as controls), take the
candidate 350 chose (`best_index` in `replay350.tsv`), reproduce 350's tilt removal, and then vary
**one** parameter: the spin `theta` about the shared piece axis, `theta` in **[-180, +180] deg** in
0.25 deg steps, translation held at the candidate's.

For each `theta` compute the distance between the two pieces' `Breakline_0` polylines with ticket
347's definition (`dist_pt_seg`, point to SEGMENT, both directions) and record the RMS over the dots
within 3.0 mm of the other curve.

Inputs, all already on the laptop:
- `C:/PR/artifacts/t339/logs/binary_juglet_signed_on_r1.log` (the `CAND` and `AXIS` lines);
- `C:/PR/artifacts/t350/tree/Breaklines/Juglet_Piece_N_Breakline_0.pcd` (the tree the run binds);
- `C:/PR/artifacts/t350/gt/Juglet_Piece_N_T.txt` (the key's poses);
- `C:/PR/artifacts/t350/replay350.py` and `replay350_all180.tsv` - reuse 350's parser and its tilt
  removal rather than rewriting them; `theta = 0` must land on 350's numbers.

Nothing here needs Spartan. If a file listed above is missing, say so and stop - do not re-fetch a
tree to work around it.

## Part B - what the best spin would score

Re-score each pair at `theta_opt` against the key exactly as `CAND338` does (`G_axis =
T_axis[y].GT[y]^-1.GT[x].T_axis[x]^-1`, rotation angle and breakline RMS of `M.G_axis^-1`), and put it
beside the candidate's own score from 350.

Table `C:/PR/artifacts/t353/spin353.tsv`, one row per pair, columns:
`pair`, `in_contact_set`, `best_index`, `spin_cand_deg`, `rms_at_cand`, `spin_opt_deg`, `rms_at_opt`,
`rms_at_opt_plus5`, `rms_at_opt_minus5`, `determinable`, `back_to_front`, `rot_at_opt_deg`,
`mm_at_opt`, `rot_cand_deg`, `mm_cand_deg`, `in_bar_cand`, `in_bar_opt`, `note`.
Millimetres are real millimetres (the logged tree-unit RMS / 3, as in 350). Say which unit every
number is in.

Also write `C:/PR/artifacts/t353/spin353_curves.tsv` (`pair`, `theta_deg`, `rms_mm`) for all 16 pairs,
so the reading can be re-checked without re-running.

## Part C - the reading, exactly one

Pre-registered before any number is looked at. A pair's spin is **determinable** iff
(i) within the wrapped grid [-180, +180] the objective has a single local minimum at `theta_opt`,
and (ii) `min(rms_at_opt_plus5, rms_at_opt_minus5) >= rms_at_opt + 0.20 mm`.
0.20 mm is the resolution floor: the breakline dots are about 1.9 mm apart and the RMS values in this
lane are 0.2-0.7 mm. A pair with `|theta_opt| > 90` is flagged `back_to_front`: the best spin is a
half turn, i.e. the correspondence matched the wrong end of the arc, and it is counted separately.

- `spin-is-not-determinable` - at most 4 of the 14 joins are determinable: the contact curve does
  not fix the spin at this resolution, so the lever is the data, not the fit.
- `spin-is-determinable-fit-misses-it` - at least 5 joins are determinable, and for at least 2 of them
  `in_bar_opt` is 1 while `in_bar_cand` is 0: the fit's spin is the fault and the next ticket is the
  spin estimate inside the fit.
- `spin-is-determinable-and-still-out` - at least 5 joins are determinable, but no pair that was
  outside the bar comes inside it at `theta_opt`: the join needs more than the spin.
- `spin-replay-inconclusive` - `theta = 0` does not reproduce 350's numbers, or an input is missing;
  say which and stop.

## Part D - acceptance

- [ ] `spin353.tsv` has 16 rows and the columns above; `spin353_curves.tsv` has the curves;
- [ ] the script is kept under `C:/PR/artifacts/t353/` and reuses 350's parser and tilt removal;
- [ ] at `theta = 0` every row reproduces ticket 350's `total_deg_after` to 0.01 deg - otherwise the
  frame or the tilt removal is wrong, not the data;
- [ ] the determinable / not-determinable count and the back-to-front count are printed, and the
  reading is chosen by the rule above, not by the story;
- [ ] the bar is applied as 5 deg and 5 real mm (state it), and the code's own 1.67 mm allowance is
  mentioned;
- [ ] `git diff --stat` shows this ticket only - no `class/`, no `hpc/`, no threshold, no assembler
  run;
- [ ] one reading, and one `**End state:**` line.

## Limits to state in the write-back

- Translation is held at the candidate's, so this is a counterfactual about the rotation only; a pair
  whose translation is wrong cannot be rescued by any spin and will read `not determinable` for the
  wrong reason. Say which pairs that could be.
- Both repeats of 350 parsed one log, so this tests the arithmetic over one run, not run-to-run
  variation.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there; never edit the main clone, another
  worktree, or `SfSpp_preprocessing/`.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- No Slurm job: everything needed is on the laptop. If you believe a job is needed, say why in the
  ticket and stop rather than submitting.
- Report, do not conclude: the tables, the counts, the one reading slug.
- If the same failure happens twice, stop and report `incomplete`, naming the blocker.

## Comments (worker, 2026-11-14)

