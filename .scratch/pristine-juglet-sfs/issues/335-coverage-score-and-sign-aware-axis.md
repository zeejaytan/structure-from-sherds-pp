# Ticket 335 — two ranking levers, measured separately: a coverage-aware score and a sign-aware axis test

**Status:** ready-for-agent
**Working in:** unclaimed
**Branch:** —
**Answers:** S1
**Depends on:** 334 (the nine-seam diagnosis), 333 (the 3x assembly run), 332 (the 1x control), 328 (wall side), 331 (3x input)
**Blocks:** nothing yet
**Worker:** unassigned
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Ticket 334 showed that for three of the nine true Juglet seams the correct pose *is* offered,
> survives every gate, and loses on score; for five it is never offered; for one (7-9) it is the seam's best
> candidate and is lost later in the pipeline. This ticket fixes the score and the axis test, **one switch
> each, default off**, and measures each lever alone and both together — at pair level and at assembly level,
> on Pot A and on the 3x Juglet.

## What is already measured (do not re-measure)

- **334, pair level, 3x Juglet, 180 candidates / 30 admitted pairs:** three seams proposed and outscored on
  `MatchingScore` — 1-2 by **2.6%**, 1-8 by **11.6%**, 1-9 by **23.2%** (margins recomputed on the surviving
  poses, since `MergeSimPair` keeps the lower-scoring twin). Five seams never within 15 deg / 10 mm
  (1-4 at 15.77 deg and 2-9 at 15.12 deg, the rest further). One seam (7-9) is the seam's **best** candidate
  (0.339969) and is lost downstream.
- **333, assembly level:** arm 1 (shipped 1.5 mm gate) **0/9 sherds, 0/18 edges**; arm 2 (4.5 mm) **2/9, 1/18**
  — the 1x score, not a reassembly. Pot A control **8/8, 15/15** twice on the same binary.
- **The score is a mean with a mild count weight:** `class/feature_matching.cpp:979`
  `MatchingScore = (sum |p_A - p_B| + sum (1 - n_A·n_B)) / count^1.2`. It has **no coverage term**: a flip that
  matches 9-18 dots at 0.2-0.8 mm beats a correct pose matching 40-70 dots at 1.2-6x the mean.
- **The axis test is sign-blind:** `class/feature_matching.cpp:1667` and `:1817`,
  `axis_angle = min(angle, pi - angle)`, gated at `> 0.436` (:1715). A piece turned 180 deg about an in-plane
  axis reads ~0 and passes.

## A fact that constrains lever 2, which the worker must confirm first

The released **pot** axis files are **lines, not directions**. Lead measurement (recompute it, do not trust it):
taking each piece's stored axis direction, rotating it by its own GT `T` into the vessel frame, and comparing
GT-joined pairs — Pot A: **9 of 15 within 30 deg, 3 beyond 150 deg, 3 around 120 deg** (all three involve piece 7,
whose axis is known to be defective); Pot B: 9 within 30 deg, 6 beyond 150 deg, 0 in between; Pot J: scattered.
So on pots the same true join appears at 0 deg on one pair and 180 deg on another, which is why the fold exists.

Therefore the un-folded test **cannot** be applied blindly to pots: it would reject true joins (3 of Pot A's 15).
The worker must measure the sign structure on both datasets before implementing, and the switch must refuse
loudly rather than silently mis-score a dataset whose axes are sign-inconsistent. The Juglet GT axes **are**
direction-consistent (334: correct poses 0.5-8.8 deg raw, flips 164-173 deg), so the lever is testable there.

## Part A — measure the axis-sign structure (no code change)

For Pot A (8 pieces) and the 3x Juglet (9 pieces), using the axis files the assembler actually reads and the GT
transforms:

1. per GT-joined pair: the **raw** angle between the two pieces' axes after the GT transform, and the folded one;
2. the same over **all** pairs, so the distribution is visible;
3. a statement: is the sign consistent (bimodal 0/180 is *inconsistent*; a tight cluster near 0 is consistent)?
4. for the Juglet: the raw angle of the candidate poses ticket 334 recorded, to confirm the separation
   (correct 0.5-8.8 deg, flips 164-173 deg).

Deliverable: a table + one sentence stating, per dataset, whether a raw-angle gate is admissible.

## Part B — lever 1: coverage-aware score, `SFS_SCORE_COVERAGE`

Default **off** (legacy score). When set to `1`:

```
c        = matched_dots / min(n_dots_i, n_dots_j)        // dimensionless, in dots
score'   = score_legacy / max(c, 0.05)
```

- read **once** at start-up, banner `coverage score: <legacy|coverage>`, unset/empty/unparseable -> legacy;
- applied **only** where the basket ranks and gates on score (:1726, and :1860 in registration) — every other use
  of `MatchingScore` keeps the legacy value;
- the survivor line must print **both** `score_legacy` and `c`, so the audit can recompute the ranking;
- no other constant moves. Not the 1.5 cut, not the 10 cut, not 0.436, not `MINIMUM_NUMBER`, not
  `INLIER_THRESHOLD`.

## Part C — lever 2: sign-aware axis test, `SFS_AXIS_SIGN`

Default **off** (folded, legacy). When set to `1`:

- the axis gate uses the **raw** angle (no `min(a, pi-a)`) at the same 0.436 rad cut, at :1667 and :1817;
- **precondition guard:** at load, the code measures the raw-angle distribution over the piece set; if more than
  a threshold fraction of pairs (start with **25%**) exceed 90 deg, the axes are line-signed and the switch
  **refuses**: print one line `SIGN-AXIS: refused, axes are sign-arbitrary (x% of pairs > 90 deg)` and fall back to
  legacy for the whole run. The guard threshold is a starting value, stated in the run log, not to be tuned to
  make a result.
- the survivor line must print `axis_raw` next to `axis_angle`.

## Part D — what is measured, and in what order

The queue is congested; order the work so the cheap evidence comes first and each stage can be voided on its own.

1. **Pair level, the 334 harness** (extend the ticket-334 instrument): the nine true seams, four cells —
   `legacy/legacy`, `coverage/legacy`, `legacy/signed`, `coverage/signed`. Per cell and per seam report:
   closest candidate to GT (rot, trans, dots, c, score_legacy, score'), the winner the basket would keep, and
   whether the two agree. **Fidelity check first:** the legacy/legacy cell must reproduce 334's 79 SURVIVOR
   lines, 30 unique pairs and identical score multiset, or the instrument is void.
2. **Assembly, Pot A** (control, 8 pieces): legacy twice; then each lever alone twice; then both twice.
   **Bar: every arm that runs must give 8/8 sherds and 15/15 edges.** A lever that breaks Pot A is rejected
   on the spot, whatever it does to the Juglet.
3. **Assembly, 3x Juglet** (9 pieces): the same four cells, twice each. That is 8 assembler runs
   (333 took 12-42 min per run; sequence them, do not parallelise beyond what the queue allows).
4. **Seam fate table:** for every arm and every one of the nine true seams, the stage at which it was lost —
   never enumerated / enumerated but pruned (name the gate) / admitted but not merged / merged but not seated —
   so 7-9's downstream loss is named in every arm, not averaged away.

## Pre-registered reading (fix it now, apply unchanged)

For each lever **alone**, against ticket 333's arm 1 (0/9, 0/18) and ticket 332's 1x (2/9, 1/18):

1. **Coverage alone** lifts the pair-level correct-win count from 3 of 9 to **>= 6 of 9**, and the assembly score
   rises above 2/9 — the ranking was the binder, and coverage is the fix.
2. **Sign alone** lifts the correct-win count and removes the upside-down winners; if it changes nothing at
   assembly, the flips were symptoms, not the binder — say so plainly.
3. **Only the two together** reach the bar (>= 4 of 9 sherds, >= 6 of 18 edges) while each alone does nothing:
   report "interaction only" and do **not** credit either lever separately.
4. **Neither, nor both,** reaches 4/9 while Pot A stays green: the pairwise ranking is not the last binder;
   report the seam fate table and stop — do not tune the formulas to find a pass.

A lever that shows no single-lever effect must be reported as such. Do not present a combined number as if both
levers contributed.

## Acceptance

- The two switches, each read once at start-up, defaulting to legacy, with the banners; `grep -n` proof that no
  other constant moved and that the legacy path is byte-for-byte unchanged when both switches are unset
  (diff the unset-path sources, and show the Pot A control reproduces 8/8 + 15/15).
- Part A table (raw vs folded angles per dataset) with the one-sentence admissibility statement.
- Part D table: four cells x nine seams at pair level, plus the eight Juglet assembly scores and the nine Pot A
  controls, each with its `sacct` State/ExitCode.
- The seam fate table for every arm.
- A render of the best arm's assembly against the GT layout (workspace-relative link; the lead checks it before
  anything is reported).
- Every job id with final `sacct` state; `runs/index.tsv` row written at submission via `scripts/new_run.sh`.

## Must not

- No change to any shipped constant: not 0.436, not 1.5, not 10, not 0.175/20, not `MINIMUM_NUMBER`,
  `INLIER_THRESHOLD`, `ANGLE_THRESHOLD`. The two new switches are the only new code paths.
- No millimetre scaling of any gate, no size or density change, no preprocessing re-run.
- No corpus write; `Dataset/` and `runs/` are read-only apart from this run's own directory.
- Do not tune the coverage formula, the 0.05 floor, the guard's 25% or the 4/9 bar to make a result. If a number
  looks wrong, say so and stop.
- No commit, no push, no branch or worktree command; the ticket is edited only by appending
  `## Comments (worker, <date>)` and flipping `Status:` to `results-in` at the end.
- Do not idle-poll a PENDING job: check `sacct` at intervals, and use the wait to stage the next cell.

## Questions this answers

S1 — can the assembler be made to seat the Juglet, and if not, which stage is the binder?
