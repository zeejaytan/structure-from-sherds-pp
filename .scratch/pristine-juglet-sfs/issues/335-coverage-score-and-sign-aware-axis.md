# Ticket 335 — two ranking levers, measured separately: a coverage-aware score and a sign-aware axis test

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-12)
**Branch:** 335-coverage-score-and-sign-aware-axis, in /c/PR/worktrees/structure-from-sherds-pp/335
**Answers:** S1
**Depends on:** 334 (the nine-seam diagnosis), 333 (the 3x assembly run), 332 (the 1x control), 328 (wall side), 331 (3x input)
**Blocks:** nothing yet
**Worker:** 29ecce19-6640-406d-8276-8a600584a35a (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-12 by the lead)
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

## Part B — lever 1: coverage-aware score, `SFS_SCORE_COVERAGE` (RANKING ONLY)

> **Amended by the lead on 2026-11-12 at the conservator's direction, before the assembly half was submitted.**
> The first draft let the coverage-adjusted score through the absolute cuts. That is a calibration error of the
> same family this project keeps finding: `score' = score_legacy / c` rescales by 1/c (c is typically 0.1-0.6),
> so the unchanged 1.5 and 10 cuts become 2-10x stricter. On ticket 334's own numbers the correct **1-9** pose
> (0.6234, c = 21/77) becomes **2.29**, above the 1.5 basket cut — the seam would be erased, not outscored.
> **The cuts keep the legacy score; coverage only decides who ranks first.**

Default **off** (legacy score). When set to `1`:

```
c         = matched_dots / min(n_dots_i, n_dots_j)       // dimensionless, in dots
rank_score = score_legacy / max(c, 0.05)                // ranking ONLY
```

- read **once** at start-up, banner `coverage score: <legacy|coverage>`, unset/empty/unparseable -> legacy;
- **gates are untouched:** the basket cut (1.5, :1726) and the registration cut (10, :1860) compare
  `score_legacy`, exactly as shipped. With the switch on, **every erasure decision must be identical to the
  legacy arm** — and the run must prove it: the per-basket erasure verdict is logged for both cells and compared
  (same baskets erased, same count). A single differing verdict voids the cell;
- `rank_score` is used **only where candidates compete for the same slot**: the `MergeSimPair` twin choice
  (:1596 family) and any other ordering that picks a winner among survivors. The worker must **enumerate those
  sites with file:line first** and list them in the ticket before wiring the switch; if a site is ambiguous, leave
  it on the legacy score and say so;
- the survivor line must print `score_legacy`, `c` and `rank_score`, so the audit can recompute the ordering;
- no other constant moves. Not the 1.5 cut, not the 10 cut, not 0.436, not `MINIMUM_NUMBER`, not
  `INLIER_THRESHOLD`.

**Retained as a documented diagnostic (not a passing cell):** the first implementation (coverage through the
gates) is kept as cell `coverage-at-gates` for the pair level only, because it measures the calibration effect
the amendment is about — it is reported as evidence, never as a lever result.

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

1. **Pair level, the 334 harness** (extend the ticket-334 instrument): the nine true seams, four passing
   cells — `legacy/legacy`, `coverage/legacy`, `legacy/signed`, `coverage/signed` — plus one **diagnostic**
   cell, `coverage-at-gates` (the pre-amendment behaviour), reported as evidence only. Per cell and per seam:
   closest candidate to GT (rot, trans, dots, c, score_legacy, rank_score), the erasure verdict of every basket,
   and the winner the ranking would keep. **Fidelity check first:** the legacy/legacy cell must reproduce 334's
   79 SURVIVOR lines, 30 unique pairs and identical score multiset, or the instrument is void. **Gate-invariance
   check:** the `coverage/legacy` cell must have byte-identical erasure verdicts to `legacy/legacy`; if any
   differs, the switch is touching a gate and the cell is void.
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

1. **Coverage alone** (ranking only, gates unchanged) lifts the pair-level correct-win count from 3 of 9 to
   **>= 6 of 9** with identical erasure verdicts — the ranking was the binder, and coverage is the fix.
2. **Coverage changes which twin survives but the correct-win count stays 3 of 9** — twin-level ranking is not the
   binder; report that and move the question to the ordering of accepted pairs, not the twin merge.
3. **Coverage alone reaches >= 6 of 9 at pair level but the assembly stays at or below 2/9** — the loss is
   downstream of the pairwise stage (the 7-9 shape); report the stage, do not credit assembly to coverage.
4. **Sign alone** removes the upside-down winners; if it changes nothing at assembly, the flips were symptoms,
   not the binder — say so plainly.
5. **Only the two together** reach the bar (>= 4 of 9 sherds, >= 6 of 18 edges) while each alone does nothing:
   report "interaction only" and do **not** credit either lever separately.
6. **Neither, nor both,** reaches 4/9 while Pot A stays green: the pairwise ranking is not the last binder;
   report the seam fate table and stop — do not tune the formulas to find a pass.

A lever that shows no single-lever effect must be reported as such. Do not present a combined number as if both
levers contributed.

## Acceptance

- The two switches, each read once at start-up, defaulting to legacy, with the banners; `grep -n` proof that no
  other constant moved and that the legacy path is byte-for-byte unchanged when both switches are unset
  (diff the unset-path sources, and show the Pot A control reproduces 8/8 + 15/15).
- **The enumerated ranking sites** (file:line) where `rank_score` is used, and the proof that every gate still
  reads `score_legacy`: same baskets erased in `legacy/legacy` and `coverage/legacy`, with the count.
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


## Comments (worker, 2026-11-12)

### Interim 1 — code in place, pair instrument submitted

**The two switches, as built** (worktree files, uncommitted; shipped to the job as patches on the pristine source):

- `SFS_SCORE_COVERAGE` / `SFS_AXIS_SIGN` are read once by `InitSfsSwitches()` (`class/feature_matching.cpp`, called at the top of `main` in `hpc/tools/main_pristine_batch.cpp`). Only the exact string `1` turns a switch on; unset, empty or unparseable stays legacy and prints `[SFS-335] <NAME> ... is not 1; using legacy`. Banners: `coverage score: legacy|coverage`, `axis sign: legacy|signed (pending sign guard)`.
- Coverage: `LCSIndex` gains `coverage_` and `axis_raw_` (`class/data_structure.h`); `coverage_ = cycle.inlier / min(dots on the two breaklines)` is recorded for every candidate where the score is computed (both pruning passes in `class/feature_matching.cpp`); the basket's 1.5 gate and the registration 10 gate read `GateScore()` = `score_ / max(c, 0.05)` when the switch is on and `score_` unchanged when it is off. The survivor line prints `score_legacy`, `c`, `score_gate`, `axis_raw`, `axis_angle` when either switch is requested.
- Sign: the fold `min(a, pi-a)` now runs only when the switch is off, at both sites (PairwisePruning and RegistrationPruning), against the same 0.436 cut. Precondition guard `SignAxisGuard()` (`class/reconstruction.cpp`, declared in `class/reconstruction.h`) carries each piece's stored axis into the vessel frame with that piece's own GT transform, counts the GT-joined pairs reading beyond 90 deg, and refuses when more than 25% do (`SIGN-AXIS: refused, axes are sign-arbitrary (<pct>% of pairs > 90 deg)` / `SIGN-AXIS: accepted, ... n=<np> GT pairs`).
- **Stated plainly, because it is a design compromise:** the guard cannot be GT-free. `AxisAlignment` maps every piece's stored axis to +z, so once a piece is loaded its axis sign is gone and the only place the sign survives is the relative pose; measuring "is this axis a line or a direction" therefore needs the GT poses the shim already loads. The guard is a measurement of the corpus, not a runtime decision made blind.
- No shipped constant touched: 0.436, 1.5, 10, 0.175/20, `MINIMUM_NUMBER`, `INLIER_THRESHOLD`, `ANGLE_THRESHOLD` all still read their literals in the built source (the job log prints the grep).

**Pair instrument (Parts A and B):** `work/diag335_pairs.cpp` is ticket 334's harness with three additions only — the two switches drive the fold and the gate, `c` is recorded, and a Part A section `AXPAIR` prints the raw and folded angle of every piece pair with its GT edge, plus `AXSUMMARY` with the guard's verdict. Survivor lines keep 334's exact shape (`SURVIVOR`) so the fidelity gate can compare them; the new numbers go on a separate `SURVIVOR335` line.

**Submitted:** run `20261112-2149_23ca16b_t335-pairs`, job **34942542** (`sapphire`, 16 cpus, 64 G, 2 h); `runs/index.tsv` row written by `scripts/new_run.sh`. It builds the shipped source (pristine + `gate333.patch` + `t335_class.patch` + `t335_shim.patch`), runs the four cells on the 3x juglet tree, the Part A dump on Pot A, then the fidelity gate (legacy/legacy vs ticket 334's 79 SURVIVOR lines). State at submission: `PENDING`.

**Next:** read Part A and the four cells when 34942542 finishes; stage the assembly job (Pot A controls + 8 juglet runs) while it waits.


### Interim 2 — assembly half staged, not submitted

Staged in `/data/gpfs/projects/punim2657/SfSpp/stage335` (md5s in the job logs), every path verified to exist on the cluster:

- `arm335.sh` — one assembly arm inside the image; shim source is now `/panel/psrc/hpc/tools/main_pristine_batch.cpp` (the shipped source), so the switches are the real code path; it prints `SFS_SCORE_COVERAGE`/`SFS_AXIS_SIGN`, the two banners and the `SIGN-AXIS` line, then Sherd/Edge accuracy.
- `submit_pota335.sbatch` — Pot A control lane, 8 arms (4 cells x r1/r2), `NP=8`, `inc_pota`, accuracy read by `checkacc333.py` on `1. Acc.txt`; bar 8/8 sherds and 15/15 edges in every run.
- `submit_juglet335_<cell>.sbatch` (4 files) — one cell per job, two repeats sequenced, `NP=9`, `inc_jug` with `-DJUGLET_ORIG`, on a private writable copy of 333's 3x tree (41 MB; Pot A tree 100 MB).
- All six scripts pass `bash -n`. Nothing submitted: the pair-level fidelity gate must pass first.

**Job 34942542** (pair instrument) still `PENDING` on `sapphire` at this point; estimated start was 22:03.

Also staged: `analysis335.py` (reads `logs/diag335_*.log` into the Part A table, the nine-seam per-cell table and the admitted-list sizes; the correct-pose criterion is rot <= 15 deg with the same 15 deg / 10 mm convention ticket 334 used). Job 34942542 `PENDING` at 22:12.


### Interim 3 — a parsing bug in my own job, caught before it ran

The stage-1 script extracted ticket 334's reference list with `grep '^SURVIVOR\t'`. GNU grep does not expand `\t`, so that matched nothing: on 334's log it returns 0 lines where `awk -F'\t' '$1=="SURVIVOR"'` returns 80 (79 rows + header). The job would have died at step 0 with "FAIL no 334 SURVIVOR lines". Fixed to awk before it started.

- Job **34942542** (run `20261112-2149_23ca16b_t335-pairs`) — `CANCELLED` by me at 22:15 while still `PENDING`, before it ran a single step. Its `runs/index.tsv` row stays; nothing in it is a result.
- Resubmitted as run `20261112-2154_23ca16b_t335-pairs2`, job **34942621**, same script with the awk fix.

The fidelity script was self-tested on the cluster before the job: `awk` extracts 80 lines (79 rows + header) from ticket 334's `logs/diag_juglet.log`, and `fidelity335.py --ref <that> --log <same log>` reports "79 SURVIVOR lines, 30 unique pairs" and `FIDELITY: PASS` (rc=0). That confirms both the reference extraction and 334's two numbers the gate rests on.


**Interim 4 (basket-level audit, as asked in m00299).** Measurement only; no formula, cut, floor or guard threshold was touched. Three things were added and the pair job was resubmitted:

- Pair instrument: a BASKET section (basket id, y, x, n_cand, n_axis_ok, lowest_legacy, lowest_gate, erased_1.5) and a BASKETCAND section (per candidate: idx, y, x, gt_edge, score_legacy, c, score_gate, rot_err_deg, axis_deg, erased_1.5). erased_1.5 is computed exactly as the shipped code does: the basket's lowest *gate* score over the candidates that survived the 0.436 axis cut, compared with the unchanged 1.5.
- Shipped code: three counters (g_sfs_basket_erased_15, g_sfs_cand_erased_15, g_sfs_cand_erased_10) incremented where the unchanged cuts fire, printed once per assembly run as "[SFS-335] gates: baskets_erased_1.5=... cands_erased_1.5=... cands_erased_10=... coverage=... signed=...", only when a switch is on. The pair instrument has no registration pass, so the 10-cut count comes from the assembly logs.
- Jobs: 34942621 (pairs2) was cancelled while still PENDING and superseded by run 20261112-2201_23ca16b_t335-pairs3, job **34942720**, which carries the BASKET sections. Nothing in either cancelled job was a result; both index rows stay.

Still waiting on the queue; nothing submitted to the assembly half yet.
