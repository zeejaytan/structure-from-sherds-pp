# Ticket 334 — is the right-way-up fit ever proposed, and if proposed, what outranks it?

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-12)
**Branch:** 334-upside-down-proposed-or-outscored, in /c/PR/worktrees/structure-from-sherds-pp/334
**Answers:** S1
**Depends on:** 331 (3x input), 332 (dense 1x control), 333 (the 3x assembly run and its per-pair dump)
**Blocks:** nothing
**Worker:** 981cd347-fb64-4d96-b779-39a3a303d1f1 (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-12 by the lead)
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

## Comments (worker, 2026-11-12)

**What was run.** Job **34934714** (sapphire, 1 node / 16 cpu, 4 h wall) — **COMPLETED 0:0**, elapsed **00:02:37**
(started 18:00:30, ended 18:03:07). Run dir
`/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1735_8560cae_t334-upsidedown/`.
One diagnostic binary (`hpc/tools/diag334_pairs.cpp` — ticket 326's harness extended, instrument only) built in the same
image from the same `/opt/sfs-pristine` + `gate333.patch` source ticket 333 used (`SFS_INLIER_MM` unset, so the shipped
gate), run on ticket 333's 3x juglet tree bound **read-only** and, second, on its Pot A tree. No gate or constant in the
shipped path was touched. The juglet tree's md5 manifest is identical before and after the run
(`logs/tree_juglet_before.md5` vs `logs/tree_juglet_after.md5`), so the diagnostic wrote nothing into the corpus.

### 1. Fidelity (acceptance bar 1) — met

- Juglet: `logs/fidelity_juglet.txt` = **FIDELITY: EXACT** — 79 SURVIVOR lines, **30 unique pairs**, and the score
  multiset identical to ticket 333's `[T333-PAIR]` list (68 distinct scores; no pair present in one list only).
- Pot A control, shipped binary on a writable tree copy: `logs/binary_potA_ctl.log` **SHERD=8/8 EDGE=15/15**. The diag
  replayed on 333's pota tree yields 70 admitted lines / 26 unique pairs, identical to 333; one score prints 0.754789
  against 333's 0.754790 (6th decimal, print rounding), which makes `fidelity334.py` report POTA MISMATCH. That is the
  printed display, not a behavioural difference.
- Instrument self-check: wherever `Icp` returned a real score, my independent `MakeCor` + `RejectOutlier(20,0.7)` +
  `MatchingScore` recomputation reproduces `score_` bit for bit. 48 of the 180 candidates carry `score_ = 11.0` (Icp's
  sentinel for non-convergence or `inlier_cal == false`); they are excluded from the term comparisons and flagged below.

Dispositions over the 180 candidates: axis gate 77, survived 79, MergeSimPair 5, overlap 18, basket score cut 1.
86 candidates lie on one of the nine true seams.

### 2. Part A — per-seam table (all nine true seams)

The harness's own answer-key gap confirms the nine-seam set (GTPAIR `gap_min`): 3-5 0.33, 6-7 0.35, 1-8 0.36, 7-9 0.43,
1-4 0.60, 2-9 1.04, 1-2 1.04, 2-5 1.52, 1-9 1.98 mm, against 5.6-57.4 mm for the other nine graph edges.
Right-way-up (rw) = rotation vs `T_gt` <= 15 deg **and** translation <= 10 mm. `axis` = the shipped, sign-folded angle.

| seam | cands | closest cand (idx) | rot ° | t mm | rms mm | score | its fate | winning cand (idx) | rot ° | score | axis ° | rw found / surviving |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1-2 | 13 | 3 | **1.93** | 1.41 | 1.91 | 0.5391 | MergeSimPair -> 1 | 5 | **173.16** | 0.4356 | 21.47 | 2 / 1 |
| 1-4 | 9 | 37 | **15.77** | 9.52 | 10.25 | 0.5039 | MergeSimPair -> 38 | 38 | 18.57 | 0.3743 | 15.08 | 0 / 0 |
| 1-8 | 12 | 67 | **0.49** | 1.25 | 1.28 | 0.4637 | MergeSimPair -> 70 | 69 | **171.30** | 0.3581 | 19.81 | 3 / 1 |
| 1-9 | 7 | 77 | **8.82** | 4.43 | 3.48 | 0.6234 | survived | 81 | **167.79** | 0.4785 | 12.55 | 1 / 1 |
| 2-5 | 3 | 94 | 45.91 | 3.75 | 32.58 | 0.4872 | survived | 94 (same) | 45.91 | 0.4872 | 14.92 | 0 / 0 |
| 2-9 | 3 | 108 | 15.12 | 5.36 | 8.15 | 0.3336 | survived | 108 (same) | 15.12 | 0.3336 | 14.01 | 0 / 0 |
| 3-5 | 4 | 118 | 48.31 | 16.84 | 44.00 | 0.5769 | survived | 118 (same) | 48.31 | 0.5769 | 13.00 | 0 / 0 |
| 6-7 | 1 | 166 | 165.69 | 7.42 | 80.45 | 0.5481 | **overlap** | none survives | — | — | 8.53 | 0 / 0 |
| 7-9 | 2 | 174 | **5.39** | 4.57 | 4.24 | 0.3400 | survived | 174 (same) | 5.39 | 0.3400 | 4.08 | 1 / 1 |

Reading the middle columns: for **six** of the nine seams the closest-to-ground-truth candidate is a *right-way-up* pose
within the bar (1-2 1.9°, 1-8 0.5°, 1-9 8.8°, 7-9 5.4°) or just outside it (1-4 15.8°, 2-9 15.1° — 0.8° and 0.1° over
the 15° bar, with translations 9.5 and 5.4 mm *inside* the 10 mm bar). Three seams (2-5, 3-5, 6-7) have no candidate
within 45° at all, and 6-7 — the seam 333 found never admitted — produced exactly **one** candidate in the whole run.

No right-way-up candidate anywhere in the run was erased by the axis gate, the basket score cut or the overlap test.
The only erasure that ever touched one was `MergeSimPair` (a dedup: 1-2 candidate 3 -> 1, 1-8 candidates 67 and 71 -> 70,
1-4 candidate 37 -> 38), and in every case the surviving twin sits within 0.2-2.8° of the dropped one, so nothing correct
was removed — 1-4's closest (15.77°) was replaced by an 18.57° twin, and 1-8 kept a 0.70° twin of its 0.49° candidate.

### 3. Part B — what outranks the right-way-up pose

Score terms, same candidates, `(dist + norm) / count^1.2`; `mean` is `(dist + norm) / count` (no count weight).

| seam | idx | pose vs GT | count | dist sum | normal sum | mean | **shipped** | fate |
|---|---|---|---|---|---|---|---|---|
| 1-2 | 1 | correct 2.14° | 40 | 34.589 | 2.822 | 0.9353 | 0.4472 | survived |
| 1-2 | 3 | correct 1.93° | 36 | 37.119 | 2.620 | 1.1039 | 0.5391 | merged into 1 |
| 1-2 | 5 | **flipped 173.2°** | 9 | 6.025 | 0.059 | 0.6759 | **0.4356** | survived, **wins by 2.6 %** |
| 1-8 | 69 | **flipped 171.3°** | 18 | 10.702 | 0.787 | 0.6383 | **0.3581** | survived, **wins by 11.6 %** |
| 1-8 | 70 | correct 0.70° | 55 | 48.228 | 1.397 | 0.9023 | 0.4048 | survived |
| 1-9 | 77 | correct 8.82° | 21 | 22.261 | 1.805 | 1.1460 | 0.6234 | survived |
| 1-9 | 81 | **flipped 167.8°** | 12 | 8.906 | 0.533 | 0.7866 | **0.4785** | survived, **wins by 23 %** |
| 7-9 | 174 | correct 5.39° | 12 | 6.252 | 0.454 | 0.5588 | **0.3400** | survived, **wins** |
| 7-9 | 175 | flipped 169.4° | 14 | 9.252 | 1.180 | 0.7451 | 0.4395 | survived |

**The term that decides it is the distance sum, not the count weight.** In every flipped-wins case the flipped pose
matches a *small, tight* subset of the seam — 9 to 18 dots at 0.2-0.8 mm mean — while the correct pose matches the whole
seam, 40 to 55 dots at a 3-6× larger mean distance. Because `MatchingScore` is a **mean over matched dots with no
coverage reward**, matching fewer dots more tightly is rewarded. The `count^1.2` divisor narrows the margin (without the
count weight 1-2's flip would win by 28 % instead of 2.6 %) but does not close it, and on 1-8/1-9 the flip wins on the
plain mean as well. The normal sum never decides anything here (0.06-2.82 against a distance sum of 6-48).
Percentages in this table are the margin as a fraction of the **correct** candidate's score.

Selection: with both survivors in hand the cluster dump of 333 (arm 1: {6,9} {2,4,5,7} {1,3,8}) shows which seam was
actually used — only **1-8** (both in {1,3,8}) and **2-5** (both in {2,4,5,7}) end in one cluster; 1-2, 1-4, 1-9, 2-9, 3-5
and 6-7 end in different clusters, and **7-9, whose correct pose is that seam's lowest score, also ends apart** ({7} with
{2,4,5,7}, {9} with {6,9}).

### 4. Part C — seeding, for the pairs that never got near

| seam | closest idx | LCS seed (sx-ex / sy-ey) | size | true-fraction | dots | mean gap mm |
|---|---|---|---|---|---|---|
| 1-4 | 37 | 31-44 / 155-168 | 14 | **0.929** | 29 | 0.92 |
| 2-5 | 94 | 118-126 / 28-36 | 9 | 0.000 | 17 | 0.78 |
| 2-9 | 108 | 44-51 / 2-9 | 8 | 0.500 | 15 | 0.56 |
| 3-5 | 118 | 120-126 / 17-23 | 7 | 0.000 | 16 | 0.99 |
| 6-7 | 166 | 19-28 / 75-84 | 10 | 0.000 | 14 | 0.90 |

Rotation families of the closest candidate per seam: <15° four (1-2, 1-8, 1-9, 7-9), 15-45° two (1-4, 2-9), 45-90° two
(2-5, 3-5), 135-180° one (6-7). **No 141-175° family appears among the closest candidates**; the 141-175° band instead
holds the *flipped winners* — three of the four flipped admissions sit at 167.8-173.2°.

**Wall side.** Every matched dot of every true-seam candidate is nearest to `Surface_0` (harness `WallSide`, all dots
labelled inner under the convention below) — consistent with ticket 328's own measurement that the `Bl0` line sits on
`Surface_0` at 0.28-0.35 mm against 1.4-2.5 mm to `Surface_1`. But **ticket 328's split does not reproduce on the 3x
tree**. Re-measured from the GT axis in `Axes/Juglet_Piece_N_Axis.xyz`: `Surface_0`'s normals point *toward* the axis on
91-100 % of its points and `Surface_1`'s away (0-3 %) on all nine pieces, and `r(Surface_1) - r(Surface_0)` = +6.20
+6.81 +6.22 +6.84 +6.80 +7.25 +5.28 +5.79 +6.54 mm for pieces 1..9. `Surface_0` is therefore the **inner** wall on every
piece here (wall 5.3-7.3 mm), where 328 recorded it as outer on 2, 6 and 9. Under 328's split five seams would be
cross-wall (1-2, 2-5, 2-9, 6-7, 7-9 — including every flipped admission); under the 3x measurement all nine are
inner-to-inner and the wall inversion explains nothing. **Unresolved, and it needs the lead**: the two measurements
agree on six pieces and on the magnitude of the gap, and differ only in sign on 2, 6, 9, so one axis source is at fault
(328 used the per-piece estimated axis at 1x, this used the GT axis at 3x). I do not adopt either as settled.

### 5. Axis sign-blindness — confirmed, with the number

51 surviving candidates are more than 90° from the ground-truth pose while their sign-folded axis angle is under the
0.436 rad cut. Worked example, **pair 1-4, candidate idx 30**: rotation vs GT **165.62°**, `axis_raw` **164.40°
= 2.869 rad**, shipped `axis_angle = min(164.40, 180-164.40) = 15.55° = 0.271 rad` — 9.4° **inside** the 25.0° (0.436 rad)
cut, and it is the line ticket 333 recorded as `4 1 ... axis_deg=15.5548`. Two more: idx 92 (2-4) 175.49° with folded
4.63°, idx 179 (8-9) 103.46° with folded 2.87°. The 77 candidates the gate does erase span `axis_raw` 25.75-155.01° —
the gate fires only in the middle band and **never on a true 180° flip**, by construction.

### 6. Classification by the pre-registered readings

- **Reading 1 — proposed, surviving, outscored (3 seams): 1-2, 1-8, 1-9.** Correct pose beats the flipped one on
  neither score nor gate; it loses on score by 0.0117 (2.6 %), 0.0468 (11.6 %) and 0.1448 (23 %). Binder =
  `MatchingScore`/ranking, exactly as reading 1 says.
- **Reading 2 — proposed but erased by a gate: none.** No right-way-up candidate was ever erased by the axis gate, the
  basket cut or the overlap test; `MergeSimPair` only replaced correct candidates by equivalent twins.
- **Reading 3 — never proposed (5 seams): 1-4, 2-5, 2-9, 3-5, 6-7.** 1-4 and 2-9 miss the 15°/10 mm bar by 0.8° and
  0.1°; 2-5 and 3-5 are 46-48° away; 6-7 has a single candidate 165.7° away, which the overlap test then erases.
- **Fourth case, not one of the three: 7-9.** Proposed, surviving, and the correct pose *is* that seam's lowest score
  (0.3400 against 0.4395) — the pair stage ranks this seam correctly and the loss is downstream, in the clustering.
  It should not be folded into reading 1.

Tally: 4 seams have a right-way-up candidate within 16° (3 of them outscored by a flip), 2 miss the bar by under a
degree, 3 have no near-correct candidate at all (one of those with a single candidate in the entire run), and 1 is
already correct at the pair stage.

### 7. Files (all in the run dir)

- `logs/diag_juglet.log` — the full instrumented pass (PIECE, GTPAIR, LCSDIAG, CAND, WALL, DROP, MERGE, SURVIVOR, DISP,
  T334PAIR sections; 180 CAND lines, 180 DISP lines).
- `logs/diag_pota.log`, `logs/binary_potA_ctl.log`, `logs/fidelity_juglet.txt`, `logs/fidelity_pota.txt`, `logs/summary334.txt`.
- `logs/tree_juglet_before.md5` / `logs/tree_juglet_after.md5` — identical, no corpus write.
- `work/diag334_pairs.cpp` (the instrument), `work/diag334.sh` (build/link recipe), `work/fidelity334.py`,
  `work/summarize334.py`, `work/ref333_arm1.txt` (333's admitted list used for the comparison).
- `logs/analysis334.txt` — the per-seam analysis behind sections 2-6 (nine-seam table, classification, axis checks),
  produced by `work/analysis334.py`.
- `logs/wall_normals.txt` — the wall-side re-measure behind section 4 (`work/surf/wall_normals.py`).
- `logs/comment334.md` — this comment as written.
- `logs/slurm-34934714.out`.

### 8. Deviations and caveats

- The wall-side statement contradicts ticket 328 on pieces 2, 6, 9 (section 4). Reported, not resolved.
- 48 candidates carry Icp's `score_ = 11.0` sentinel; they are in the CAND/DISP dump with their recomputed terms, but
  excluded from the comparison table in section 3.
- 6-7 produced one candidate for the whole run, so its "never proposed" is a statement about a single trial, not a
  distribution — 333's per-pair dump agrees (6-7 is the one true seam never admitted).
- Nothing was committed, pushed, or staged; the shipped tree and the corpus are untouched.
---

## Lead verification and ruling (2026-11-12, lead session)

Read back from the run directory, not from the report.

- **sacct:** 34934714 COMPLETED 0:0, 00:02:37, sapphire. Pot A control 8/8 sherds, 15/15 edges; the
  0.754789 vs 0.754790 line is print rounding, not a mismatch.
- **Fidelity:** 30 unique pairs and the 68-value score multiset are identical to ticket 333's arm-1 admitted
  list; 79 SURVIVOR lines match. The harness measured the shipped code, so the table can be read as a
  statement about the assembler and not about a proxy.
- **The margins, recomputed on the surviving poses.** `MergeSimPair` replaces a correct candidate with a
  near-twin of *lower* score, so the pre-merge score is not the one that competes:
  1-2 idx 1 0.447224 vs idx 5 0.435568 = **2.6 %**;
  1-8 idx 70 0.404823 (70 inliers, 0.70 deg, 1.21 mm) vs idx 69 0.358065 (12 inliers) = **11.6 %**;
  1-9 idx 77 0.623368 vs idx 81 0.478527 = **23.2 %**.
  The worker's 11.6 % is right; my first arithmetic used the pre-merge 0.4637 and gave 22.8 %, which is the
  wrong comparison. Worth naming: the strongest correct pose in the run (70 inliers at 0.70 deg / 1.21 mm)
  still loses to a 12-inlier 171 deg flip.
- **Axis gate, measured from the drop log:** 96 drops (77 `axis_angle>0.436`, 18 overlap, 1 basket). The 77
  erased candidates have raw axis 25.75-155.01 deg; since the test is `min(a, 180-a)`, the gate can only fire
  in that band, so a 180 deg flip (folded ~0) can never be erased by it. 131 of 180 candidates are more than
  90 deg from GT and 68 of those survive with the folded axis under the cut.
- **The count in the worker's message is wrong, the ticket's is right.** The message says a right-way-up
  candidate exists for "6 of the 9 seams"; the table and my read give **4** (1-2, 1-8, 1-9, 7-9). 1-4 and 2-9
  miss by 0.8 and 0.1 deg. The classification in section 6 (3 outscored, 5 never proposed, 7-9 a fourth case)
  is the account to keep.
- **Wall labels: the worker's doubt is correct and 328's record does not reproduce.** My own check
  (`C:/PR/artifacts/t334_wall_check.py`: mean perpendicular radius about each piece's own GT axis, plus the
  outward-normal fraction) on both trees: `Surface_0` normals point toward the axis on 91-100 % of points and
  `Surface_1` away, with r(Surface_1) - r(Surface_0) = +1.41 to +2.36 mm at 1x and +4.27 to +7.08 mm at 3x.
  `Surface_0` is the **inner** wall on all nine pieces at both scales, no flips. 328 measured the radius about
  the *vessel* axis after applying gtT, which is a different quantity from the per-piece own-axis radius.
  328's code cure (wall side by curvature at the seam) does not depend on the label and stands; its diagnostic
  paragraph does depend on it and is corrected in that ticket.

### Ruling

The pre-registered readings, applied literally: **reading 1 (proposed, surviving, outscored) holds for three
seams — 1-2, 1-8, 1-9. Reading 2 is empty: no correct pose was erased by a gate. Reading 3 (never proposed)
holds for five — 1-4, 2-5, 2-9, 3-5, 6-7, of which two miss the bar by under a degree. 7-9 fits none of them:
its correct pose is the seam's own lowest score and is lost downstream, in the clustering.**

The mechanism behind reading 1 is in the score, not the gates: `MatchingScore` is a mean over matched dots, so
a flip that matches 9-18 dots at 0.2-0.8 mm beats a correct pose that matches 40-70 dots at 1.2-6x that mean.
The `count^1.2` divisor pushes the right way (without it 1-2's flip would win by 28 %) but does not close a
2.6 % gap. The axis test is sign-blind by construction, so it cannot act as the second line of defence.

Claim type: **method failed** (ranking term + a sign-blind axis test), not a broken measurement and not a wrong
reference. Weight: nine seams, one run, but the run reproduces 333's admitted set exactly, so the table is the
same data 333 scored, read one level deeper.

What this does *not* say: that fixing the score would reassemble the Juglet. 7-9 is a measured counterexample to
"the best pair wins the assembly", so any cure has to be tested at assembly level on the pot control as well as
per pair.
