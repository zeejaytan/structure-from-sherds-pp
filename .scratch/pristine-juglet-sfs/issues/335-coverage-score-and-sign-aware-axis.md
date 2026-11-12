# Ticket 335 — two ranking levers, measured separately: a coverage-aware score and a sign-aware axis test

**Status:** landed
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

### Interim 5 — design change applied before the assembly half: coverage ranks, gates keep the legacy score

Pulled b9e5b93 into the worktree (the only commit taken). The switch is now ranking-only. Nothing was submitted to the assembly half.

**Site enumeration, done before wiring (file:line):**

| site | what it does | verdict |
|---|---|---|
| `class/feature_matching.cpp:1656` (`MergeSimPair`) | the only place two survivors compete for one slot: keeps the lower-scoring twin of an `isSimilar(0.175, 20)` pair | **wired to rank_score** |
| `class/feature_matching.cpp:1726` | basket cut, `lowest_score > 1.5` erases the basket | gate, keeps `score_legacy` |
| `class/feature_matching.cpp:1860` | registration cut, `score_ > 10` erases the candidate | gate, keeps `score_legacy` |
| `class/reconstruction.cpp:1567`, `:1776` | `MatchingScore(lcs.score_, ...)` writes `score_` | no ordering |
| `class/ranking_system.cpp:908`, `:928`, `:1348`, `:1550` | state ranking by `state_score_`/`graph_score_`, both derived from `inliner_` | never reads `LCSIndex::score_` |

No sort or order-by-score exists over `LCS_out`; its order is insertion order. No ambiguous competition site was found, so nothing was left on the legacy score for that reason.

**Code:** `RankScore()` (`class/feature_matching.cpp:53-61`) returns `score_legacy / max(c, 0.05)` and is called only at `:1658`; both gates read `score_` (`:1790`, `:1945`). The survivor line prints `score_legacy`, `c`, `rank_score`. The three counters now count **legacy-gate** erasures, so the assembly half can compare them against the legacy arm mechanically.

**Instrument:** `GateScore335` returns `score_legacy` unless `SFS_COVERAGE_AT_GATES=1`, which restores the pre-amendment shape as the `coverage-at-gates` diagnostic cell; `RankScore335` is used in the harness `MergeSimPair` copy; `MERGE` gained `kept_rank`/`dropped_rank`; `SURVIVOR335` prints `rank_score`; `BASKET`/`BASKETCAND` unchanged in meaning. `invariance335.py` compares the DROP verdicts (candidate + gate name) and every basket's `(n_cand, erased_1.5)` between `legacy/legacy` and `coverage/legacy`.

**Jobs:** 34942720 (pairs3) FAILED 5:0 — my own compile error, `cov_by_idx` declared after its first use; no result. Resubmitted as run 20261112-2211_23ca16b_t335-pairs4, job **34943031**, five cells (four passing + the diagnostic) plus the invariance check.

### Interim 6 — first pair run with the amended shape: fidelity PASS, invariance FAILed on my own reporting block

Run 20261112-2211_23ca16b_t335-pairs4, job **34943031** COMPLETED 0:0, elapsed 00:02:42. Five cells ran clean (rc=0 each, 21-22 s):

| cell | SURVIVOR lines |
|---|---|
| legacy/legacy | 79 |
| coverage/legacy | 79 |
| legacy/signed | 51 |
| coverage/signed | 51 |
| coverage-at-gates (diagnostic) | **22** |

**Fidelity: PASS.** `legacy/legacy` gives 79 SURVIVOR lines, 30 unique pairs, and `fidelity335.py` reports PASS against ticket 334 (`logs/fidelity335.txt`). The instrument reproduces 334.

**Invariance: FAIL, and the failure is in the instrument, not the switch.** The DROP verdicts — the shipped path — are identical: 97 vs 97, zero differing candidates, zero differing gate names (`logs/invariance335.txt`). What differed was the `erased_1.5` column of my own `BASKET` report: its lambda still divided by `c` when the coverage switch was on, i.e. it reported the pre-amendment number. That is exactly the 24-vs-1 basket count in the report, and it is a reporting bug: the candidate-level DROP lines show the gates never moved.

Fixes, then resubmitted: the `BASKET` lambda now obeys the same rule as the gates (legacy unless the diagnostic cell asks otherwise), and `invariance335.py` now derives the erasure verdict from the DROP `basket_score` lines — the shipped path — instead of trusting the report column, and requires the erased-basket sets to be equal. Run 20261112-2229_23ca16b_t335-pairs5, job **34943439**.

**Part A, already measured (run pairs4):**
- 3x Juglet: 18 GT-joined pairs, **0** beyond 90 deg raw, worst raw 1.5e-05 deg, guard **ACCEPTED** (`logs/diag335_legacy_legacy.log`, AXSUMMARY).
- Pot A: 15 GT-joined pairs, **6** beyond 90 deg raw (40%), worst 177.94 deg, guard **REFUSED** (`logs/diag335_pota.log`, AXSUMMARY) — the released pot axes are lines, as the ticket warned.

**The diagnostic cell is the calibration evidence the amendment predicted:** with coverage through the gates, the Juglet keeps 22 of 79 survivors and 24 of 36 baskets are erased by the unchanged 1.5 cut; with coverage ranking only, 79 survive and the DROP verdicts match legacy exactly.

### Interim 7 — pair level is green; Part A and the nine-seam table; assembly submitted

Run 20261112-2229_23ca16b_t335-pairs5, job **34943439** COMPLETED 0:0, elapsed 00:02:55. Five cells rc=0.

| cell | SURVIVOR lines | unique pairs | seams with a correct pose admitted | seams where the correct pose is the best-ranked survivor |
|---|---|---|---|---|
| legacy/legacy | 79 | 30 | 4/9 | **1/9** |
| coverage/legacy | 79 | 30 | 4/9 | **2/9** |
| legacy/signed | 51 | 29 | 4/9 | **4/9** |
| coverage/signed | 51 | 29 | 4/9 | **4/9** |
| coverage-at-gates (diagnostic) | 22 | 9 | 1/9 | 1/9 |

"correct pose" = rotation error <= 15 deg from the GT pose (`work/seams335.py`, ROT_OK = 15.0); "best-ranked survivor" = lowest rank_score among the survivors of that pair, i.e. the candidate the pipeline would carry forward.

**Fidelity: PASS** (`logs/fidelity335.txt`): legacy/legacy reproduces 334 — 79 SURVIVOR lines, 30 unique pairs, identical score multiset.
**Gate-invariance: PASS** (`logs/invariance335.txt`): legacy/legacy vs coverage/legacy — DROP dispositions identical, 77 axis-gate / 1 basket-cut / 18 overlap in both cells, 36 baskets, 0 baskets with a different verdict.

**Part A — is a raw-angle gate admissible?** (`work/analysis335.py`, AXPAIR/AXSUMMARY; angles in degrees)

| dataset | GT-joined pairs | raw angle > 90 deg | worst raw | guard |
|---|---|---|---|---|
| 3x Juglet | 18 | **0** (0.0%) | 1.5e-05 | **ACCEPTED** |
| Pot A | 15 | **6** (40.0%) | 177.9 | **REFUSED** |

Pot A pairs beyond 90 deg: 1-5 177.9, 2-5 177.4, 3-5 177.9, 1-7 119.8, 4-7 121.6, 6-7 121.5. Every juglet GT pair reads 0.0 deg raw: the nine juglet axes are one line, and the GT poses make them coincide as directions.

**One sentence per dataset.** 3x Juglet: a raw-angle gate is admissible — all 18 GT-joined axes agree as directions to within 1.5e-05 deg, so "up" and "down" are the same line and the 180-deg flip the gate must catch is a property of the candidate pose, not of the reference. Pot A: a raw-angle gate is NOT admissible — 6 of 15 GT-joined pairs have axes that are anti-parallel or 120 deg apart in the released files, so an unfolded comparison would reject the correct pose itself; the guard refuses, exactly as designed.

**The nine seams, per cell** (`logs/seams335.txt`; rank = rank_score, rot in deg):

| seam | legacy/legacy | coverage/legacy | legacy/signed | coverage/signed | diagnostic (gates) |
|---|---|---|---|---|---|
| 1-2 | flip 173.2 wins (0.4356 vs 0.4472) | flip wins (2.323 vs 2.385) | **correct 2.1 wins** | **correct 2.1 wins** | correct pose erased at the 1.5 cut |
| 1-4 | no correct-pose candidate | none | none | none | none |
| 1-8 | flip 171.3 wins (0.3581 vs 0.4048) | **correct 1.6 wins** (0.693 vs 3.760) | **correct 0.7 wins** | **correct 1.6 wins** | correct 1.6 survives |
| 1-9 | flip 167.8 wins (0.4785 vs 0.6234) | flip wins (2.632 vs 4.800) | **correct 8.8 wins** | **correct 8.8 wins** | correct pose erased at the 1.5 cut |
| 2-5 | no correct-pose candidate (best 45.9) | none | none | none | none |
| 2-9 | no correct-pose candidate (15.1, just outside the 15 deg bar) | none | none | none | none |
| 3-5 | no correct-pose candidate (48.3) | none | none | none | none |
| 6-7 | lost before ranking (1 candidate, 165.7, erased by overlap) | lost | lost | lost | lost |
| 7-9 | **correct 5.4 wins** | **correct 5.4 wins** | **correct 5.4 wins** | **correct 5.4 wins** | correct pose erased at the 1.5 cut |

**Calibration of the unchanged cuts under the old design** (`work/calib335.py`, `logs/calib335.txt`) — this is the evidence the amendment asked for:

| cell | axis-gate drops | 1.5-cut drops | overlap drops |
|---|---|---|---|
| legacy/legacy | 77 | 1 | 18 |
| coverage/legacy | 77 | 1 | 18 |
| legacy/signed | 116 | 2 | 6 |
| coverage/signed | 116 | 2 | 6 |
| coverage-at-gates | 77 | **75** | 3 |

And the correct pose's gate score under the old design: 1-2 0.4472 / c 0.1875 = **2.385** (above 1.5), 1-9 0.6234 / 0.1299 = **4.800** (above 1.5), 7-9 0.3400 / 0.2078 = **1.636** (above 1.5); 1-8 0.4342 / 0.6270 = 0.693 (below). The diagnostic cell loses all three of those seams and keeps only 22 survivors — the prediction in the amendment, confirmed to the seam.

**Reading against the pre-registered bars (no retuning):** coverage alone moves correct-win 1/9 -> 2/9, short of the "3/9 -> >=6/9" bar; sign alone moves it 1/9 -> 4/9 and removes the upside-down winners of 1-2, 1-8 and 1-9; both together are 4/9, i.e. no interaction at the pair level. Note the baseline here is 1/9, not the 3/9 the ticket quotes: my definition is the winner among the admitted survivors, which is stricter than "a correct candidate is admitted" (4/9 in the legacy cell).

**Instrument bug, recorded:** the BASKET/BASKETCAND reporting columns are wrong — the captured per-basket index list accumulates across baskets, so the rows are cumulative and cannot be read per basket. The DROP lines are the shipped path and are correct (they are what the invariance check compares); the per-seam calibration above was computed from CAND and SURVIVOR335 instead, so nothing in the numbers above depends on the broken column.

**Assembly submitted** (each job runs its two repeats sequenced inside one job):

| lane | run | job |
|---|---|---|
| Pot A control, 4 cells x 2 repeats (8 arms) | 20261112-2235_23ca16b_t335-pota | **34943588** |
| 3x Juglet legacy/legacy x2 | 20261112-2238_23ca16b_t335-juglet-legacy_legacy | **34943640** |
| 3x Juglet coverage/legacy x2 | 20261112-2239_23ca16b_t335-juglet-coverage_legacy | **34943641** |
| 3x Juglet legacy/signed x2 | 20261112-2239_23ca16b_t335-juglet-legacy_signed | **34943642** |
| 3x Juglet coverage/signed x2 | 20261112-2239_23ca16b_t335-juglet-coverage_signed | **34943643** |

### Interim 8 — Pot A control lane green so far; the signed arms refuse, as required

Run 20261112-2235_23ca16b_t335-pota, job **34943588** (still running; 6 of 8 arms done at 22:57).

| arm | coverage | sign | rc | wall | sherds | edges |
|---|---|---|---|---|---|---|
| pota_legacy_legacy_r1 | - | - | 0 | 163 s | 8/8 | 15/15 |
| pota_legacy_legacy_r2 | - | - | 0 | 179 s | 8/8 | 15/15 |
| pota_coverage_legacy_r1 | 1 | - | 0 | 165 s | 8/8 | 15/15 |
| pota_coverage_legacy_r2 | 1 | - | 0 | 192 s | 8/8 | 15/15 |
| pota_legacy_signed_r1 | - | 1 | 0 | 206 s | 8/8 | 15/15 |
| pota_legacy_signed_r2 | - | 1 | 0 | 202 s | 8/8 | 15/15 |

Both Pot A signed arms print the guard line verbatim and fall back for the whole run:

    SIGN-AXIS: refused, axes are sign-arbitrary (40% of pairs > 90 deg)

(`logs/binary_pota_legacy_signed_r1.log`, `..._r2.log`; 6 of 15 GT-joined pairs beyond 90 deg = 40.0%, matching the AXSUMMARY above). No Pot A arm ran with the fold removed.

**Constants untouched, mechanically:** `t335_class.patch` removes 6 lines and each is replaced by a switch-guarded equivalent — `inliner_(0)` (initialiser, `coverage_(1.0)`/`axis_raw_(0)` added beside it), the merge comparison `if (iter->score_ < comp->score_)`, the two `axis_angle = min(axis_angle, 3.14159 - axis_angle);` fold lines, the basket `lowest_score = min(lowest_score, iter->score_);`, and the `if ((iter->score_ > 10) || (iter->overlap_))` gate (re-emitted unchanged plus a counter). No removed or added line alters 0.436, 1.5, 10, 0.175, 20, MINIMUM_NUMBER, INLIER_THRESHOLD or ANGLE_THRESHOLD.

**Reading 2, recorded with both numbers (wins / admitted, never one alone):** coverage alone reaches **wins 2/9** (from 1/9) and **admitted 4/9** (unchanged from 4/9); the pre-registered coverage bar was wins 3 -> >= 6, so it **MISSES**, and twin ranking in `MergeSimPair` is not the binder. Sign alone reaches **wins 4/9 / admitted 4/9**; both together **wins 4/9 / admitted 4/9** — the two levers do not interact at the pair level.

3x Juglet jobs still PENDING (34943640-43) behind a congested sapphire queue.

### Interim 9 — Pot A lane complete (8/8 arms green); first Juglet arm reads 0/9

Job **34943588** COMPLETED 0:0, 00:26:14. All eight Pot A arms: **8/8 sherds, 15/15 edges**, rc=0 — legacy/legacy r1 163 s, r2 179 s; coverage/legacy r1 165 s, r2 192 s; legacy/signed r1 206 s, r2 202 s; coverage/signed r1 217 s, r2 209 s. `Pot A lane failures: 0`. Both signed arms print `SIGN-AXIS: refused, axes are sign-arbitrary (40% of pairs > 90 deg)` and score 8/8.

First Juglet arm, job 34943640 (`juglet_legacy_legacy_r1`, 549 s): **0/9 sherds, 0/18 edges**, three clusters `{6,9} {2,4,5,7} {1,3,8}`. This is a valid run, not an instrument failure: 9 pieces loaded, `LCS window: 6`, both switches off.

Why 0/9 and not the 2/9 in the ticket: ticket 333's own Juglet arms were `arm1` (inlier gate unset = shipped 1.5 mm) **0/9, 0/18** and `arm2` (SFS_INLIER_MM=4.5) **2/9, 1/18** (`runs/20261112-1238_55473e9_t333-juglet-large/out/arm1_r1/1. Acc.txt`, `arm2_r1/1. Acc.txt`). My arms run with the shipped gate, so 0/9 is the correct legacy baseline here; the 2/9 quoted in the ticket belongs to the 4.5 mm arm.

Part D machinery ready: `work/fate335.py` reads each arm's `T_<i>to_<i>.txt`, the `[T333-CLUSTER]` lines and the GT transforms, and prints per GT edge the cluster of each piece plus the rotation and translation error of the final relative pose. Tested on Pot A: all 15 edges in one cluster, rotations 0.9-3.8 deg, translations 4.9-26.4 mm.

### Interim 10 — final: assembly (Part C), seam fate (Part D), render; both switches measured, bars stated plainly

All jobs are closed. `sacct -X -n`, every one COMPLETED 0:0:

| job | run | State / ExitCode | elapsed |
|---|---|---|---|
| 34943439 | 20261112-2229_23ca16b_t335-pairs5 | COMPLETED 0:0 | 00:02:55 |
| 34943588 | 20261112-2235_23ca16b_t335-pota | COMPLETED 0:0 | 00:26:14 |
| 34943640 | 20261112-2238_23ca16b_t335-juglet-legacy_legacy | COMPLETED 0:0 | 00:21:02 |
| 34943641 | 20261112-2239_23ca16b_t335-juglet-coverage_legacy | COMPLETED 0:0 | 00:23:08 |
| 34943642 | 20261112-2239_23ca16b_t335-juglet-legacy_signed | COMPLETED 0:0 | 00:12:55 |
| 34943643 | 20261112-2239_23ca16b_t335-juglet-coverage_signed | COMPLETED 0:0 | 00:13:30 |

Earlier jobs of this ticket, for the record: 34942542 CANCELLED (pending, my grep bug), 34942621 CANCELLED (pending), 34942720 FAILED 5:0 (my harness did not compile: `/panel/work/diag335_pairs.cpp:543:25: error: cov_by_idx was not declared in this scope`), 34943031 COMPLETED 0:0 00:02:42 (pairs4: fidelity PASS, my reporting block wrong). Every run above has its row in `runs/index.tsv` (lines 810-819; `verdict` and `human_note` left blank for the lead).

#### Part C — 3x Juglet assembly, four cells, two repeats each

Each job ran its two repeats sequenced inside it. "sherds" and "edges" are the two `Sherd accuracy` lines of `out/<arm>/1. Acc.txt` (first = 9 pieces, second = 18 GT edges).

| cell | arm | sherds | edges | clusters (`[T333-CLUSTER]`) |
|---|---|---|---|---|
| legacy/legacy | r1 | 0/9 | 0/18 | {6,9} {2,4,5,7} {1,3,8} |
| legacy/legacy | r2 | **2/9** | **1/18** | {6,9} {2,4,5,7} {1,3,8} |
| coverage/legacy | r1 | 0/9 | 0/18 | {6,9} {1,3,8} {2,4,5,7} |
| coverage/legacy | r2 | 0/9 | 0/18 | {6,9} {1,3,8} {2,4,5,7} |
| legacy/signed | r1 | **2/9** | **1/18** | {2,4,5,7,9} {1,3,8} {6} |
| legacy/signed | r2 | **2/9** | **1/18** | {5,7} {1,3,8} {2,4,6,9} |
| coverage/signed | r1 | **2/9** | **1/18** | {5,7} {1,3,8} {2,4,6,9} |
| coverage/signed | r2 | **2/9** | **1/18** | {2,4,5,7,9} {1,3,8} {6} |

**The 4/9-sherd bar is MISSED in every arm** (best 2/9 = 22%), and nothing was retuned to chase it. Read with the noise in view: the legacy/legacy cell itself reads 0/9 in r1 and 2/9 in r2 on the same binary, so **the assembly is not repeat-stable** and a one-repeat difference of 2/9 sits inside run-to-run variation. The signed cells read 2/9 in all four repeats, which the legacy cell cannot claim, but the clusters still differ between the two repeats of a cell ({2,4,5,7,9} vs {5,7}), so I do not claim a signed gain from this table.

**Pot A control (the acceptance bar):** run 20261112-2235_23ca16b_t335-pota, job 34943588 — **8 arms, 8/8 sherds and 15/15 edges each, rc=0, "Pot A lane failures: 0"**; wall 163 s, 179 s (legacy/legacy), 165 s, 192 s (coverage/legacy), 206 s, 202 s (legacy/signed), 217 s, 209 s (coverage/signed). **PASS, twice per cell.** The two signed arms print `SIGN-AXIS: refused, axes are sign-arbitrary (40% of pairs > 90 deg)` and fall back; no Pot A arm ran with the fold removed.

#### Part D — where each of the nine true seams was lost

`work/fate335.py` reads each arm's `T_<i>to_<i>.txt` (0-based), the 1-based `[T333-CLUSTER]` lines and the GT transforms; for each GT edge it prints whether the two pieces ended in the same cluster (**J**) or not (**S**) and the rotation error of the final relative pose in degrees (`fate_juglet_all.txt`, compressed by `work/fate_table.py`). A correct join is J with a small angle; **J with 90-180 deg is a wrong join, worse than S**, because the assembly asserts it.

| arm | 1-2 | 1-4 | 1-8 | 1-9 | 2-5 | 2-9 | 3-5 | 6-7 | 7-9 |
|---|---|---|---|---|---|---|---|---|---|
| legacy/legacy r1 | S46 | S161 | **J2** | S34 | J95 | S69 | S137 | S156 | S165 |
| legacy/legacy r2 | S68 | S164 | **J2** | S23 | J95 | S69 | S160 | S156 | S165 |
| coverage/legacy r1 | S46 | S161 | **J2** | S34 | J95 | S69 | S137 | S156 | S165 |
| coverage/legacy r2 | S46 | S161 | **J2** | S34 | J95 | S69 | S137 | S156 | S165 |
| legacy/signed r1 | S86 | S108 | **J1** | S92 | J168 | **J9** | S55 | S158 | J159 |
| legacy/signed r2 | S84 | S111 | **J1** | S89 | J171 | **J9** | S51 | S155 | J163 |
| coverage/signed r1 | S84 | S111 | **J1** | S89 | J171 | **J9** | S51 | S155 | J163 |
| coverage/signed r2 | S86 | S108 | **J1** | S92 | J168 | **J9** | S55 | S158 | J159 |

Stage at which each seam is lost, read off this table together with the pair instrument (Interim 7):

- **1-8** — joined correctly (1-2 deg) in **all eight arms**: never lost.
- **7-9** — the pair instrument carries the correct pose to the top in every cell (5.4 deg), but the assembly joins it at **159-165 deg** in the signed cells and splits it in the legacy cells. Lost **after** pairwise ranking: the pose that reaches the top of the basket is not the pose written into the final transform.
- **1-2, 1-9** — the pair instrument admits a correct pose and even ranks it first in the signed cells, yet the assembly splits them (S23-S92). Also lost downstream of pairwise ranking.
- **2-9** — joined correctly (9 deg) in the signed cells, split (69 deg) in the legacy cells; the pair instrument never ranks a correct candidate for it (best 15.1 deg, just outside the 15 deg bar), so this join comes from the graph/state layer, not from a pair I can point at.
- **2-5** — joined at **95-171 deg** in every arm: a wrong join, not a miss.
- **1-4, 3-5, 6-7** — split everywhere and no correct candidate is ever admitted (best 18.6 / 48.3 / 165.7 deg); 6-7 has a single candidate and is erased by overlap before ranking.

So pairwise ranking is not the binder for six of the nine seams. Both switches change *which* pose wins a pair (Part B: correct-winner 1/9 -> 4/9 with the sign switch), but the assembly does not inherit those wins: it makes its own choice at the state/graph stage. That is the measurement this ticket set out to make.

#### Render

[artifacts/t335/render/t335_juglet_legacy_signed_r1_vs_gt.png](artifacts/t335/render/t335_juglet_legacy_signed_r1_vs_gt.png) — left the GT juglet, right the legacy/signed r1 arm coloured by its three final clusters and aligned on pieces 1 and 8 (the pair it places correctly, 1.2 deg). Per-piece displacement after that alignment, mm: 1:10, 2:114, 3:45, 4:109, 5:54, 6:97, 7:72, 8:52, 9:109. The 5-piece cluster sits inside the vessel body but turned; the handle cluster (1,3,8) is displaced downward and outward. Render inputs in `C:/PR/artifacts/t335/render/{arm,gt}`; final script `work/render_run4.py`.

#### What the two switches do, in one place

| lever | pair level (correct pose is the best-ranked survivor) | assembly level (3x Juglet, sherds / edges) | Pot A control |
|---|---|---|---|
| coverage alone | 1/9 -> **2/9** (bar was 3 -> >=6: **MISSED**) | 0/9, 0/18 and 0/9, 0/18 | 8/8, 15/15 twice |
| sign alone | 1/9 -> **4/9** | 2/9, 1/18 in both repeats | 8/8, 15/15 twice (guard refuses, falls back) |
| both | 1/9 -> **4/9** (no interaction) | 2/9, 1/18 in both repeats | 8/8, 15/15 twice |

Neither switch is default-on, both banner themselves at start-up, both fall back to legacy on an empty or unparseable value, and with both unset the shipped path is unchanged — fidelity PASS against ticket 334 (79 SURVIVOR lines, 30 unique pairs, identical score multiset) and gate-invariance PASS (identical DROP dispositions, 96 candidates, 0 differing verdicts).

**Files:** code in the worktree (`class/feature_matching.cpp`, `class/data_structure.h`, `class/reconstruction.cpp`, `hpc/tools/main_pristine_batch.cpp`), patches in `C:/PR/artifacts/t335/work/{t335_class.patch,t335_shim.patch}`; instrument `work/diag335_pairs.cpp`; analysis `work/{analysis335.py,seams335.py,calib335.py,fate335.py,fate_table.py}`; logs under each run's `logs/` and `out/`. Uncommitted: no commit, no push, no branch or worktree command was run.


## Lead verification (2026-11-12, after the worker's Interim 10)

**All numbers below were re-read by me from the runs, not taken from the worker's report.**

- **Jobs**, `sacct -X`: 34943439 pairs5 COMPLETED 0:0 00:02:55; 34943588 Pot A COMPLETED 0:0 00:26:14;
  34943640 legacy/legacy 00:21:02; 34943641 coverage/legacy 00:23:08; 34943642 legacy/signed 00:12:55;
  34943643 coverage/signed 00:13:30. Earlier: 34942542 and 34942621 CANCELLED while PENDING (nothing ran);
  34942720 FAILED 5:0 (the worker's own harness compile error); 34943031 pairs4 COMPLETED 0:0.
- **Pot A control**, `out/*/1. Acc.txt` for all eight arms: **8/8 sherds and 15/15 edges, both repeats, every
  cell**. Both signed arms print `SIGN-AXIS: refused, axes are sign-arbitrary (40% of pairs > 90 deg)` and fall
  back — verified in `logs/binary_*.log` (4 refusals, 4 `coverage score: coverage`, 4 `legacy`). No Pot A arm
  ran with the fold removed.
- **Pair level**, my independent recompute from the `SURVIVOR335` lines
  (`C:/PR/artifacts/t335/lead_verify_pairs.py`; rank = `rank_score`): legacy 79 survivors / 30 pairs / **1 win**,
  4 admitted; coverage 79/30/**2**, 4; legacy+signed 51/29/**4**, 4; coverage+signed 51/29/**4**, 4;
  coverage-at-gates 22/9/**1**, 1. Matches the worker's table exactly. Winners' rotation errors — legacy:
  1-2 173, 1-4 19, 1-8 171, 1-9 168, 2-5 46, 2-9 15, 3-5 48, 7-9 5; coverage turns 1-8 into 2 deg and 1-4 into
  166; signed turns 1-2 into 2 and 1-9 into 9, keeps 1-8 at 1 and 7-9 at 5, leaves 2-9 at 15.1 (just outside the
  15 deg bar), 2-5 at 46, 1-4 at 19, 3-5 at 48.
- **Assembly**, `out/*/1. Acc.txt`: legacy 0/9 0/18 then 2/9 1/18; coverage 0/9 0/18 twice; legacy+signed
  2/9 1/18 twice; coverage+signed 2/9 1/18 twice. **The 4/9-sherd / 6/18-edge bar is missed in every cell.**
- **Fidelity and invariance** (`logs/fidelity335.txt`, `logs/invariance335.txt`): legacy/legacy reproduces 334's
  79 survivor lines, 30 unique pairs and the score multiset; 96 DROP verdicts identical in legacy/legacy and
  coverage/legacy, 0 differing.
- **The code**, read in the worktree: the only competition site is `class/feature_matching.cpp:1658`
  (`RankScore` in `MergeSimPair`); the basket cut reads `iter->score_` (:1790) and the registration cut reads
  `score_` (:1945). `git diff -U0 | grep '^-'` shows six removed lines, each replaced by a switch-guarded
  equivalent; 0.436, 1.5, 10, 0.175/20, MINIMUM_NUMBER, INLIER_THRESHOLD and ANGLE_THRESHOLD are unchanged.
  `SignAxisGuard` (class/reconstruction.cpp:111) takes the GT transforms from its caller, so the library stays
  GT-free and only the batch shim supplies them.

### One correction to the worker's Part D

7-9 is **not** "split in the legacy cells and joined flipped in the signed cells". Read from
`artifacts/t335/fate_juglet_all.txt`: in **every** cell the final relative pose for 7-9 is **159-165 deg** — in
legacy/legacy r1 it is cluster 1/0 (split) *and* 165 deg; in the signed cells cluster 0/0 (joined) at the same
angle. So the assembly writes a flipped 7-9 everywhere, including the cell where the pair instrument had the
correct 5.4 deg pose ranked first. That is stronger than the report says: the final transform is not the pairwise
winner at all; it is re-derived downstream.

### What the ticket establishes

1. **The separation was necessary.** With coverage through the gates the unchanged 1.5 cut erases 75 candidates
   (vs 1) and kills the correct poses of 1-2 (2.385), 1-9 (4.800) and 7-9 (1.636) — measured, and the reason the
   gate split exists.
2. **Coverage ranks, and it is weak.** 1 -> 2 of 9 wins, exactly one seam, because the twin merge is the only
   competition site. Pre-registered reading 2 (twin ranking is not the binder) is the one that holds.
3. **The sign test works at pair level** (1 -> 4 of 9 wins) **and cannot be used on the pots**: 40% of Pot A's
   true joins read beyond 90 deg because the released pot axes are lines, not directions. The guard refuses
   correctly and Pot A stays green.
4. **Neither lever reaches the assembly bar.** With the pair-level choice improved fourfold the assembly stays
   inside 1-2 of 9 — and the legacy cell itself moves 0/9 to 2/9 on the same binary, so that band is noise. The
   pairwise choice is not the binder for six of nine seams: the pose written into the final transform is
   re-derived at the state/graph stage (7-9 is the clean case).

**Claim type: the method failed at the stage this ticket could reach.** Not a broken measurement (fidelity and
invariance pass; Pot A green) and not a wrong reference (the 3x GT poses come from the released T matrices). The
next question is the state/graph stage, not the pair score.
