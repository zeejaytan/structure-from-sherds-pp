# Ticket 334 — is the right-way-up fit ever proposed, and if proposed, what outranks it?

**Status:** ready-for-agent
**Working in:** (claimed by the worker)
**Branch:** (set by `ticket_start.sh`)
**Answers:** S1
**Depends on:** 331 (3x input), 332 (dense 1x control), 333 (the 3x assembly run and its per-pair dump)
**Blocks:** nothing
**Worker:** unassigned
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Scale and dot density are ruled out as the blocker (331, 332, 333 agree): the hopeless step
> is the pairwise pose. This ticket measures whether a right-way-up pose is ever *offered* for each of the nine
> true seams, and if it is offered, which step throws it away or outranks it — because "never proposed" and
> "proposed then outscored" lead to different fixes (the fitting versus the ranking).

## What is already measured (do not re-measure)

- **1x, dense:** all nine touching seams are enumerated, **1 of 9 pairs passes**, score **2/9 sherds, 1/18
  edges**, 5-8 of the pairs fit upside down (>90 deg). Run `20261112-0245_30b80ae_t332-dense-window12`,
  `score/acc.tsv`.
- **3x:** 180 candidates (32 at 1x), **30 of 36 pairs admitted**, still **1 correct pair** — 1-8 at
  1.99 deg / 53.5 mm — score **0/9, 0/18** at the shipped gate and **2/9, 1/18** at the scale-equivalent
  4.5 mm gate. Five true seams are admitted **upside down**: 1-4, 2-5, 3-5, 6-7, 7-9. Run
  `20261112-1238_55473e9_t333-juglet-large`, `score/summary.json`.
- **Pot A is unaffected:** 8/8 sherds, 15/15 edges, twice, on the same binary — so the bias is specific to this
  vessel's breaklines, not to the code being broken.

## The code path this ticket is about (read these before writing anything)

All in `class/feature_matching.cpp` and `class/reconstruction.cpp` on `origin/main` (`700f5c7`):

- `class/feature_matching.cpp:1615` `PairwisePruning` — for **every** LCS candidate of a pair it calls
  `Icp(...)` at :1650, stores the fitted transform in `iter->trans_` and the score, then filters in a basket:
  erase if `axis_angle_ > 0.436` (:1715), erase the whole basket if its best score is `> 1.5` (:1726),
  erase if the pieces overlap (:1729); then `MergeSimPair` (:1739, :1589) collapses transforms within
  **0.175 rad / 20 mm** keeping the lower score (:1596).
- `class/feature_matching.cpp:1751` `RegistrationPruning` — the same shape during graph growth, with a
  looser score cut at :1860 (`score_ > 10`).
- `class/reconstruction.cpp:1151` `Icp` — the pose is a **local refinement seeded by the LCS correspondence**
  (Ceres, axis and rim terms), with `MatchingScore` and `inlierCalculate` at :1339-1340. There is no global
  search for the pose: what the seed says, the refinement believes.
- `class/feature_matching.cpp:979` `MatchingScore` = `(sum |p_A - p_B| + sum (1 - n_A·n_B)) / count^1.2`
  — a mean, divided by a count weight, and it is **this** number the basket ranks on.
- **The axis test is sign-blind:** at :1667 and :1817, `axis_angle = min(angle, pi - angle)`. A piece turned
  180 deg about an in-plane axis has its axis normal reversed, `acos(dot) = pi`, and the test reads **0**.
  A flipped pose therefore cannot be caught by the 0.436 rad gate. Check this holds numerically; if it does,
  say so with the number.

## Part A — does a right-way-up candidate exist?

**Definition.** For a pair (i, j) the GT relative transform is `T_gt = T_i^{-1} * T_j` from
`Dataset/Juglet_large/Juglet_Piece_N_T.txt`. A candidate is **right-way-up** when the rotation between its
`trans_` and `T_gt` is **<= 15 deg** and the translation is **<= 10 mm** (generous: the merge radius itself is
10 deg / 20 mm). Report the distribution, not only the cut: for each pair give the *closest* candidate's
rotation and translation, so a 20 deg near-miss is visible.

Instrument a copy (do not edit the shipped tree): for each of the nine true seams and for every candidate that
reaches the basket, dump

```
pair  seed_id  rot_vs_GT_deg  trans_vs_GT_mm  score  axis_angle_rad  overlap  area_mm2  erased_by
```

where `erased_by` is one of `survived / axis_gate / basket_score / overlap / mergesimpair / nooverrange /
registration_pruning`. A candidate can be erased by more than one; print the first that fires in program order
and note the others. Include candidates the basket would erase — the whole point is to see what is thrown away.

## Part B — if it is proposed, who outranks it?

Three separate questions, each answered with numbers:

1. **Score:** the surviving flipped candidate's `score` against the right-way-up candidate's `score` — and
   which term differs (distance sum, normal sum, or the `count^1.2` denominator). One flipped candidate with
   more matched dots and a smaller mean distance will beat a correct one; say whether that is what happens.
2. **Gate:** if the right-way-up candidate is erased, name the gate and its margin (e.g. "axis_angle 0.98 rad
   vs the 0.436 cut" — this would be the sign-blind case; or "score 1.62 vs the 1.5 basket cut").
3. **Selection:** if both survive, which one the assembly actually uses (the `[T333-PAIR]` list from ticket
   333's diagnostic shim, plus the cluster dump).

## Part C — if it is *never* proposed

Only if Part A finds no right-way-up candidate within 15 deg / 10 mm for a pair, characterise the seed:

- the winning LCS alignment: how many dots it pairs, in which order along the two lines, and the mean gap;
- **which wall each matched dot sits on.** Ticket 328 established that `Surface_0` is the outer wall on pieces
  2, 6, 9 and the inner wall on pieces 1, 3, 4, 5, 7, 8 — so "A's outer rim paired with B's inner rim" is a
  measurable, physically wrong pairing of the two sides of one crack. Report the wall split per matched dot for
  the winning candidate of each pair.
- the angle of the closest candidate: if a 141-175 deg family appears, say how many pairs share it.

## Pre-registered reading (fix it now, apply unchanged)

1. **Right-way-up proposed and surviving, but beaten on score** -> the ranking/scoring is the binder. Name the
   term and the margin. The fix is in `MatchingScore`/ranking, not in the fit.
2. **Right-way-up proposed but erased by a gate** -> that gate is the binder. Name it and its margin. If it is
   the sign-blind axis test, the fix is to make it direction-aware.
3. **Right-way-up never proposed (< 15 deg / 10 mm) for a majority of the nine** -> the fitting is the binder:
   the LCS seed cannot reach the correct pose, and the fix is in the correspondence/seed, not the ranking.

A shortfall is a shortfall. If the three cases split across pairs, say which pair falls in which case; do not
average them into one verdict.

## Acceptance

- **Fidelity first.** The instrumented pass must reproduce ticket 333's arm-1 admitted list exactly (30 unique
  pairs from 180 candidates) and the Pot A control must stay 8/8 sherds, 15/15 edges. If the instrumented
  binary changes any of that, the numbers are void — say so and fix the instrument.
- The per-pair table for all nine true seams, every number traceable to a named file in the run dir.
- The classification (reading 1, 2 or 3 per pair) with the term, gate or seed named.
- The axis sign-blindness check, with the number for at least one admitted upside-down pair.
- Every job id with its final `sacct` State/ExitCode.

## Must not

- **No change to any gate or constant in the shipped path** — no edit to 0.436, 1.5, 10, 0.175/20, `MINIMUM_NUMBER`,
  `INLIER_THRESHOLD`, `ANGLE_THRESHOLD`. The instrument is a diagnostic copy or a separate harness.
- No re-run of the preprocessing chain, no re-run of the four scored arms (a diagnostic pass is enough).
- No corpus write; `Dataset/` and `runs/` stay read-only apart from this run's own directory.
- No commit, no push, no branch or worktree command; the ticket is edited only by appending
  `## Comments (worker, <date>)` and flipping `Status:` to `results-in`.
- Do not judge the pictures; export what the lead needs and say what it shows numerically.

## Where it lives

- Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t334-upsidedown/`.
- Read first: ticket 333's run `20261112-1238_55473e9_t333-juglet-large` (tree, shim, `[T333-PAIR]` dump) and
  ticket 326's diagnostic harness `hpc/tools/diag326_pairs.cpp` + `hpc/scripts/diag326.sh` on branch
  `326-scale-free-assembly` (it already links pristine objects and replays the pair stage with logging — reuse
  it rather than starting from scratch).
- The 3x tree: `.../20261112-1238_55473e9_t333-juglet-large/work/trees/px3/juglet` (GT axes from
  `Dataset/Juglet_large`).
