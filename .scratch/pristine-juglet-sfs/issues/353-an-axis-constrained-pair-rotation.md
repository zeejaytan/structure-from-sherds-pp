# Ticket 353 - how the pair's spin is chosen

**Status:** ready-for-agent
**Waits for:** nothing - ticket 350 landed 2026-11-14 and says the tilt is not the lever
**Branch:** to be created at start
**Worker:** unassigned
**Answers:** S1
**Depends on:** 350 (the tilt buys two joins), 349 (the four-of-fourteen candidate ceiling), 348 (the measured contact set), 347 (the contact-curve runs)
**Blocks:** any change to how a pair's rotation is formed
**Budget:** small-medium (3 failed acceptance runs).
**Hand-over:** DeepSeek: offline arithmetic over geometry already fetched, plus one small geometry job.

> **In one line.** Ticket 350 showed the pair rotation is wrong mostly by a spin about the shared
> axis; measure whether the contact curve even determines that spin, and how far the fit's own spin
> sits from the best one.

## Why this replaced the axis-constrained build

The first version of this ticket was to force the pair rotation onto the known axes during the fit,
conditional on ticket 350 finding that the tilt costs the joins. 350 found the opposite: removing the
tilt exactly, after the fact, moves 5 of 16 pairs inside the bar to 7 (only 1-9 and 5-8 cross in).
Building the same constraint into the fit would buy at most those two joins and is not worth a code
change on this evidence. What is left over on every other join is a **spin**, so the spin is the
question, and it can be asked without touching the assembler.

## Part A - the spin, measured against the contact curve

For each of the 16 pairs of ticket 350's table (the 14 measured joins plus 4-7 and 4-6 as controls),
at the true relative pose, take the candidate's rotation, remove the tilt about the piece axis as 350
does, and then vary **one** parameter: the spin `theta` about that axis, `theta` in [-30, +30] deg in
0.25 deg steps, translation held at the candidate's. For each `theta` compute the distance between the
two pieces' `Breakline_0` polylines with ticket 347's own definition (point-to-SEGMENT, both
directions) and record the root mean square over the dots within 3.0 mm of the other curve.

Pre-registered classification, before any number is looked at:

- the spin is **determinable** for a pair iff the objective has a single local minimum on the grid and
  `RMS(theta_opt +/- 5 deg) >= RMS(theta_opt) + 0.20 mm`;
- otherwise it is **not determinable** for that pair.

0.20 mm is the resolution floor: the breakline dots are 1.9 mm apart and the measured RMS values in
this lane are 0.2-0.7 mm.

## Part B - what the best spin would score

Re-score each pair at `theta_opt` against the key exactly as `CAND338` does (`G_axis =
T_axis[y].GT[y]^-1.GT[x].T_axis[x]^-1`, rotation angle and breakline RMS of `M.G_axis^-1`), and put it
beside the candidate's own score. Table `C:/PR/artifacts/t353/spin353.tsv`, columns:
`pair`, `in_contact_set`, `best_index`, `spin_cand_deg`, `rms_at_cand`, `spin_opt_deg`, `rms_at_opt`,
`rms_at_opt_plus5`, `rms_at_opt_minus5`, `determinable`, `rot_at_opt_deg`, `mm_at_opt`, `rot_cand_deg`,
`mm_cand_deg`, `in_bar_cand`, `in_bar_opt`, `note`.

## Part C - the reading, exactly one

- `spin-is-not-determinable` - for at least 9 of the 14 joins the objective is flat by the rule above:
  the contact curve does not fix the spin at this resolution, and the lever is the data (curve length,
  scale, a second feature), not the fit.
- `spin-is-determinable-fit-misses-it` - for at least 5 joins it is determinable, and for at least 2 of
  those the best spin brings the pair inside the 5 deg / 5 mm bar while the candidate's spin does not:
  the fit's spin is biased, and the next ticket is the spin estimate inside the fit.
- `spin-is-determinable-and-still-out` - determinable for at least 5 joins, yet no pair comes inside
  the bar even at `theta_opt`: the join needs more than the spin.
- `spin-replay-inconclusive` - a needed input is missing or the two frames cannot be aligned; say
  which.

## Part D - acceptance

- [ ] the table has 16 rows and the columns above; `spin353.tsv` under `C:/PR/artifacts/t353/`;
- [ ] the script is kept under `C:/PR/artifacts/t353/` and reads the same inputs ticket 350 used
  (the log `C:/PR/artifacts/t339/logs/binary_juglet_signed_on_r1.log`, the tree under
  `C:/PR/artifacts/t350/tree/`, the key under `C:/PR/artifacts/t350/gt/`);
- [ ] at `theta` = 0 the replay reproduces ticket 350's `total_deg_after` for every row to 0.01 deg -
  otherwise the frame or the tilt removal is wrong, not the data;
- [ ] the determinable / not-determinable count is printed, and the reading follows the rule, not the
  story;
- [ ] both the tree frame and the 1x frame are named, and every millimetre says which;
- [ ] `git diff --stat` shows this ticket only - no `class/`, no `hpc/`, no threshold, no run;
- [ ] one reading, and one `**End state:**` line.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there; never edit the main clone, another
  worktree, or `SfSpp_preprocessing/`.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- Spartan: one small read-only geometry job if the breaklines are not already local; every `sbatch`
  gets a poll and its final `sacct` State/ExitCode recorded.
- Report, do not conclude: the table, the counts, the one reading slug.
- If the same failure happens twice, stop and report `incomplete`, naming the blocker.

## Comments (worker, 2026-11-14)

