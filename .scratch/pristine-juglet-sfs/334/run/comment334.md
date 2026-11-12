
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
- `logs/slurm-34934714.out`.

### 8. Deviations and caveats

- The wall-side statement contradicts ticket 328 on pieces 2, 6, 9 (section 4). Reported, not resolved.
- 48 candidates carry Icp's `score_ = 11.0` sentinel; they are in the CAND/DISP dump with their recomputed terms, but
  excluded from the comparison table in section 3.
- 6-7 produced one candidate for the whole run, so its "never proposed" is a statement about a single trial, not a
  distribution — 333's per-pair dump agrees (6-7 is the one true seam never admitted).
- Nothing was committed, pushed, or staged; the shipped tree and the corpus are untouched.