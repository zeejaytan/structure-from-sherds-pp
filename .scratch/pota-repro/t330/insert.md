### Bar 2 — the flip table

**Every length literal the patch scales, at s = 1.546** (from the audit's own `SCALE` line):

| literal (mm) | `Gate()` | where it decides |
|---|---|---|
| 1.5 (`INLIER_THRESHOLD`) | 2.32 | CountInlier — feeds `inlier_` and `score_`; does not drop a pair itself |
| 2 | 3.09 | DetectIntersection correspondence length |
| 3 | 4.64 | reconstruction inlier distance |
| 4 | 6.18 | DetectIntersection `cor.len` cap |
| 5 | 7.73 | makeregion length floor, OverlapCheck centre-too-close, CauchyLoss |
| 7 | 10.82 | ProfileChecking |
| 10 | 15.46 | inlierCalculate rim gaps, RejectOutlier(10), isConverge |
| 20 | 30.92 | RejectOutlier(20), `isSimilar`/`isSimilarTrans` translation, isSimilarGraph |
| 50 | 77.31 | PairwisePruning `OverlapCheck_3d(...,50.0)` area floor |
| 100 | 154.6 | `OverlapCheck_3d(...,100)` area floor in graph building |
| 1000 | 1546 | OverlapCheck centre-too-close area sentinel |

**Not scaled:** ANGLE_THRESHOLD 0.262 rad (15°), RejectOutlier's 0.7/0.8 quantiles, the 0.436 rad
sliver gate, the 1.5 basket-score bar, and the scorer's own 0.35 rad / 50 mm.

**What a verdict is here.** The candidate enumeration is identical in both regimes — all 1815
candidates carry the same `(y, x, gt)` at the same index — so a verdict change is the same candidate
being pruned in one regime and kept in the other. MOR1: **792 survivors at S = 60, 816 at
S = 92.77; 81 candidates appear, 57 disappear, net +24 (+3.0%)**.

Where the 81 came from, by the reason they were dropped at S = 60:

| S = 60 drop reason | candidates | of which |
|---|---|---|
| `axis_angle > 0.436` (bar unchanged) | 46 | the pose moved under the rescaled Icp |
| `overlap(area=…)` | 21 | **14 exactly by the widened window** (50 < area ≤ 77.31); 7 because the area itself changed |
| `basket_lowest_score > 1.5` (bar unchanged) | 5 | the scores moved |
| merged away at S = 60 | 9 | the score ordering changed which rival survived |

So of the two *decision* literals that moved, the overlap floor accounts for 14 candidates and the
merge window for 3 new merges wider than 20 mm (merges 30 → 32). Everything else moved because the
rescaled inlier distance, RejectOutlier and CauchyLoss *inside* Icp changed the poses, which then
crossed gates whose values never changed.

**Pairs whose verdict changes (MOR1)** — one row per pair:

| pair | direction | gate whose value moved | before → after | GT edge | inlier | gap_min (mm) | median gap (mm) | score | drop reason on the side that drops it |
|---|---|---|---|---|---|---|---|---|---|
| P24–P27 | drop→keep | overlap area floor | 50 → 77.31 mm | no | 18 | 0.63 | 11.77 | 0.834 | S60: `overlap(area=54.9)` |
| P03–P15 | drop→keep | none (0.436 rad gate unchanged) | — | no | 28 | 0.21 | 21.26 | 0.511 | S60: `axis_angle>0.436` |
| P18–P29 | drop→keep | none (0.436 rad gate unchanged) | — | no | 12 | 0.64 | 14.02 | 0.590 | S60: `axis_angle>0.436` |
| P05–P24 | drop→keep | none (1.5 basket bar unchanged) | — | no | 14 | 0.73 | 15.98 | 0.963 | S60: `basket_lowest_score>1.5(11)` |
| P08–P25 | drop→keep | none (1.5 basket bar unchanged) | — | no | 8 | 0.82 | 14.83 | 0.625 | S60: `basket_lowest_score>1.5(11)` |
| P20–P27 | drop→keep | none (1.5 basket bar unchanged) | — | no | 18 | 0.18 | 19.98 | 0.309 | S60: `basket_lowest_score>1.5(11)` |
| P15–P30 | drop→keep | none (1.5 basket bar unchanged) | — | no | 0 | 66.95 | 72.95 | 11.0 | S60: `basket_lowest_score>1.5(11)` |
| P01–P17 | keep→drop | none (0.436 rad gate unchanged) | — | no | 34 | 0.45 | 26.38 | 0.662 | S92.77: `axis_angle>0.436` |
| P02–P23 | keep→drop | none (0.436 rad gate unchanged) | — | no | 0 | 0.05 | 23.20 | 11.0 | S92.77: `axis_angle>0.436` |
| P04–P27 | keep→drop | none (0.436 rad gate unchanged) | — | no | 8 | 0.42 | 10.91 | 0.591 | S92.77: `axis_angle>0.436` |
| P13–P30 | keep→drop | none (1.5 basket bar unchanged) | — | no | 0 | 0.24 | 13.72 | 11.0 | S92.77: `basket_lowest_score>1.5(11)` |
| P15–P24 | keep→drop | none (1.5 basket bar unchanged) | — | no | 0 | 65.71 | 75.28 | 11.0 | S92.77: `basket_lowest_score>1.5(11)` |
| P22–P26 | keep→drop | none (1.5 basket bar unchanged) | — | no | 0 | 0.87 | 15.98 | 11.0 | S92.77: `basket_lowest_score>1.5(11)` |

All 13 are non-GT, in both directions. **No GT pair flips at all.** (K3R1, secondary lane: 5 gains
P01–P17, P02–P10, P03–P15, P07–P21, P09–P16 and 5 losses P04–P27, P07–P20, P07–P31,
P10–P30, P15–P17 — also all non-GT.)

Whether a flipped pair appears in the patched assembly's final join set cannot be read off the
artefacts: the run saves no merge list. The frame-free proxy is physical contact at the final pose,
and **none of the 13 flipped pairs is in contact (≤ 1 mm) in the patched final state**; nor does any
of them touch the five misplaced sherds except P08–P25 and P05–P24, whose partners P08 and P05 are
both correctly placed in the patched run.

### Bar 3 — attribution of the 16 failed MOR1 joins

"GT-correct candidate" = a candidate for that pair whose pose is within the scorer's bars of the
answer key (rot < 20.05 deg, t < 50 mm).

| failed join | candidates | GT-correct at S60 | at S92.77 | GT-correct candidate surviving both |
|---|---|---|---|---|
| P03–P17 | 18 | 1 | 1 | yes, idx 441 (11.1 deg / 13.5 mm) |
| P03–P24 | 9 | 1 | 1 | yes, idx 498 (18.3 / 3.9) |
| P03–P25 | 6 | 1 | 1 | yes, idx 503 (10.2 / 7.4) |
| P03–P26 | 2 | 1 | 1 | yes, idx 505 (4.1 / 3.7) |
| P05–P25 | 4 | 1 | 1 | yes, idx 722 (10.9 / 11.2) |
| P13–P25 | 1 | 1 | 1 | yes, idx 1382 (6.5 / 5.1) |
| P14–P30 | 4 | 1 | 1 | yes, idx 1458 (2.9 / 4.3) |
| P17–P18 | 8 | 1 | 1 | yes, idx 1561 (2.2 / 0.7) |
| P18–P23 | 2 | 1 | 1 | yes, idx 1640 (12.4 / 14.0) |
| P20–P23 | 7 | 1 | 1 | yes, idx 1701 (5.3 / 2.7) |
| P23–P24 | 4 | 1 | 1 | yes, idx 1766 (8.5 / 10.3) |
| P28–P31 | 3 | 1 | 1 | yes, idx 1810 (3.5 / 1.5) |
| P03–P05 | 5 | 0 | 0 | no — no candidate is GT-correct in either regime |
| P14–P26 | 1 | 0 | 0 | no — its only candidate is 149.8 deg off |
| P24–P31 | 3 | 0 | 0 | no — the two plausible candidates are killed in **both** regimes by the **unscaled** 0.436 rad axis gate (0.842, 0.483 rad) |
| P26–P30 | 0 | 0 | 0 | no — no candidate exists at this stage |

Reading:

