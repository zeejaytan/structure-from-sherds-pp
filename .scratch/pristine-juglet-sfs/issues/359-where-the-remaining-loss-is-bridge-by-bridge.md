# Ticket 359 - Where the remaining loss is, bridge by bridge

**What to build:** an offline classification of the four missing bridges between the two clusters of ticket 358 - 6-7, 3-5, 5-6, 5-8 - by measuring, at the true relative pose and at the pose the search found, the three quantities that can refuse a pair: the overlap area (limit 50.0), the axis gate (0.436 rad) and the pair's own inlier count, with the gate replay anchored on the logged candidate rows before it is trusted.

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-15)
**Branch:** 359-where-the-remaining-loss-is-bridge-by-bridge, in /c/PR/worktrees/structure-from-sherds-pp/359
**Worker:** dsh one-shot subagent, dispatched 2026-11-15 by the lead
**Answers:** S1
**Needs-eye:** no
**Depends on:** 358, 348, 347
**Blocked-by:** none
**Budget:** small (offline on the laptop from files already fetched; no Spartan job unless a file is missing, and then a single short fetch)
**Hand-over:** DeepSeek

---

## Why this ticket

Ticket 358 landed a coherent seven-sherd cluster {1,2,3,4,6,8,9} and the measured join {5,7} as a separate two-sherd cluster, with nine of the fourteen measured joins inside 5 deg and 5 mm. Four measured joins still do not appear as bridges between the two clusters, and they fail in three different places. The next decision - whether to spend effort on the measurement, the fit, or the merge - depends on which of those three is true for each bridge, and nobody has measured it yet.

The four, with what is already known:

| bridge | what ticket 358 showed | what is not known |
|---|---|---|
| 6-7 | candidate 166 formed and was dropped as pair_overlap with area 77.3877 against the 50.0 limit in this run's log (76.898 is the stock arm's row for the same candidate index, 16 inliers against 22 here) | whether the overlap area at the true pose is also over 50, or whether the fit's own pose inflated it |
| 3-5 | the best-by-rotation row is candidate 120, a failed fit (0 inliers, score 11.0) that the basket rule removes; candidate 118 does reach CAND338 at 48.26 deg / 44.19 tree units with 24 inliers | whether the true pose is inside the axis gate and would be kept if offered, and whether 118's 48 deg is a fit failure or a bad candidate |
| 5-6 | passes CAND338 with 36 inliers, then its own Icp answer is 13.08 deg / 5.22 mm | whether the data at the true pose are good enough that a fit starting there would hold; i.e. is the fit or the data the limit |
| 5-8 | registers, then its state lands in the other cluster | whether its pose at the true relative transform passes the pair's inlier count, which decides fit vs data |

The claim being separated is (1) method, (2) measurement, (3) reference: if the gates pass a true join, the refusal is a measurement or method defect; if they refuse a true join on their own numbers, the measurement they act on is what must change; the reference is already known to omit these joins (ticket 348).

## Part A - the replay and its anchor

Write C:/PR/artifacts/t359/gates359.py. It must reproduce, in python and from the files already on the laptop, the two measures that the C++ pair stage uses before a candidate can survive:

1. **Overlap area** - class/feature_matching.cpp, OverlapCheck_3d at ~:1912, called with a 50.0 limit; the CAND line's area_ field is the number the rule compares.
2. **Axis angle** - the axis gate at class/feature_matching.cpp:1847 (axis_angle_ > 0.436 rad), with axis_raw_ and axis_angle_ on the CAND line.
3. **Inlier count** - the pair's own scoring, i.e. MakeCor + RejectOutlier(20, 0.7) + MatchingScore + inlierCalculate as used at class/reconstruction.cpp:1542-1554, evaluated at a pose handed to it.

**Anchor, before anything is believed:** for every [SFS-338] CAND338 row of the four bridges and of the two controls in C:/PR/artifacts/t358/logs/binary_juglet_xy_full_r1.log, the replay must reproduce the logged area_ and axis_raw_ from the logged 3x4 matrix - area within 2 percent, axis within 0.01 rad. Print the comparison for each row. If the anchor misses on more than one row, stop: the replay is the finding, and the ticket ends incomplete saying which measure could not be reproduced.

## Part B - the same three measures at the true pose

True relative poses: C:/PR/artifacts/t355/tree/juglet/Ground Truth/Juglet_Piece_N_T.txt (the tree the run binds; one tree unit = 3 mm). Breaklines: the tree's Breakline files under C:/PR/artifacts/t355/tree/juglet/ or the per-piece 1x originals if the tree copies are not on the laptop; state which set was used and why.

For each of the six pairs - the four bridges plus controls 1-8 and 5-7 (accepted joins) and 4-7 (a measured non-join, ticket 348) - report at the true relative pose: overlap area against 50.0, axis angle against 0.436 rad, and the inlier count the pair scoring gives. For 5-6 and 5-8 also report the inlier count at the pose the search actually found, so the two are comparable.

## Part C - the table

C:/PR/artifacts/t359/bridges359.tsv, 12 data rows (6 pairs x 2 poses: found and true; where a pair has no found pose, print na and say why), columns:
pair, measured_join, gate_that_refused, logged_area, logged_axis_rad, replay_area, replay_axis_rad, found_inliers, true_area, true_axis_rad, true_inliers, area_limit, axis_limit, verdict.

## Part D - readings

- **gates-refuse-a-true-join** - for at least one bridge, the true pose passes the inlier count the accepted joins get AND the overlap area at the true pose is over 50.0 or the axis angle over 0.436 rad. The gate is refusing a real join on its own numbers: the measure is what must change, and the next ticket is about that measure, not about loosening the number.
- **gates-innocent-the-fit-is-the-gap** - the true pose passes all three, and the pair still was not carried. The fit is the limit for that bridge; the next ticket is a fit question (for 5-6, why 13 deg from 36 inliers; for 3-5, why 48 deg).
- **gates-innocent-the-merge-is-the-gap** - the true pose passes all three and a carried edge exists, yet the clusters stay apart. Then the merge or the plausibility veto is refusing, and the veto line names it.
- **mixed** - the four bridges land in different rows of the above. Name which is which; the lead will split the follow-ups.
- **replay-not-faithful** - the anchor of Part A fails. Stop and report.

One reading, on its own line at the end of the write-back, prefixed Reading:.

## Part E - acceptance

- [x] gates359.py exists, runs from the laptop with no Spartan job, and prints the anchor comparison row by row.
- [x] The anchor reproduces every logged area within 2 percent and every logged axis within 0.01 rad, or the write-back says exactly which rows it could not and ends incomplete.
- [x] bridges359.tsv has 12 data rows and the columns above, with no blank verdict.
- [x] Each of the four bridges has a verdict naming one of gate / fit / merge, with the number it rests on.
- [x] The controls behave: 1-8 and 5-7 show their gates passing, 4-7 is reported and not used to conclude anything about the bridges.
- [x] One render, C:/PR/artifacts/t359/render359_bridge_gates.png, showing for each pair the overlap area at the found pose and at the true pose against the 50.0 line (a bar chart is enough; the point is the two poses side by side).
- [x] No change under class/ or hpc/, no assembler run, nothing committed, no other ticket touched.
- [x] The write-back in the ticket has the table, the anchor printout, the reading line, and exactly one End state line.

## Comments

- **2026-11-15 (lead):** written after a read-only review of ticket 358 (cursor, claude-opus-5-5, session t358-next) which found that the ticket's own summary sentence "3-5 never reaches CAND338" is wrong - candidate 118 is in the log at 48.26 deg - and that only four of the nine seated joins (1-2, 1-8, 5-7, 6-9) were inside the bar at the end of the pair fit, the other four coming inside during assembly. Both are corrected in S1. This ticket exists because the remaining decision - measurement, fit or merge - cannot be taken without the three numbers measured at the true pose, and that measurement is offline and cheap.

## Comments (worker, 2026-11-15)

**Run shape.** Offline python only: `C:/PR/artifacts/t359/gates359.py` with four commands (`anchor`, `probe`, `table`, `plot`), numpy + matplotlib, no Ceres, no compile, no Spartan job, no assembler run. `git status` in the worktree is clean apart from this ticket file; nothing under `class/` or `hpc/` was touched; nothing was committed.

**Inputs.** Log: `C:/PR/artifacts/t358/logs/binary_juglet_xy_full_r1.log`. True poses: `C:/PR/artifacts/t355/tree/juglet/Ground Truth/Juglet_Piece_N_T.txt`. Breaklines: **the tree's own copies**, `C:/PR/artifacts/t355/tree/juglet/Breaklines/Juglet_Piece_N_Breakline_0.pcd` — used because the run itself binds the tree (`hpc/tools/main_pristine_batch.cpp:130-154` loads the tree's Breakline, Axis and Ground Truth files), so the replay reads the same files the search read; the per-piece 1x originals were therefore not needed and are not in `C:/PR/artifacts/t355`. One tree unit = 3 mm. Points per piece after the loader's unique filter: 1:185, 2:96, 3:125, 4:102, 5:150, 6:141, 7:174, 8:126, 9:77.

**Part A — the anchor holds.** Row by row, printed by `gates359.py anchor` (also saved as `C:/PR/artifacts/t359/anchor359.txt`):

```
index pair  logged_area  replay_area  d_area%   logged_axis_rad  replay_axis_rad  d_axis_rad  area_ok  axis_ok
  118 3-5     0.000000     0.000000    0.000         0.221887         0.221887    0.000001  ok      ok
  158 5-6    10.447200    10.451339    0.040         0.148349         0.148352    0.000003  ok      ok
  164 5-8     8.000000     8.000000    0.000         0.140803         0.140800    0.000003  ok      ok
   70 1-8     1.125130     1.125122    0.001         0.019648         0.019647    0.000001  ok      ok
  162 5-7     2.000000     2.000000    0.000         0.077877         0.077879    0.000002  ok      ok
  145 4-7     0.000000     0.000000    0.000         0.030966         0.030953    0.000013  ok      ok

anchor rows: 6, reproducing: 6, missing: 0
VERDICT: anchor reproduces every required row (need at most one miss to continue)
```

Six of six required rows reproduce: worst area error 0.040 percent (row 158), worst axis error 0.000003 rad. The replay is faithful, so this ticket does not end incomplete; the measures below are the C++ pair stage's own numbers.

One further check, not asked for but it closes the same door: recomputing the CAND338 `rot_deg` and `mat` fields from my `G_axis` reproduces all six logged rows — 70: 1.12497/1.30957 vs logged 1.12578/1.30959; 118: 48.26006/44.18872 vs 48.26010/44.18870; 158: 13.08188/15.67464 vs 13.08180/15.67460; 164: 8.31785/5.78287 vs 8.31780/5.78286; 162: 4.63845/3.48098 vs 4.63843/3.48097; 145: 3.47962/2.65491 vs 3.47972/2.65493 — so the true-pose composition `T_axis[y] · G[y]⁻¹ · G[x] · T_axis[x]⁻¹` is the one the run used, not a look-alike.

**Part B — the three measures, found pose and true pose.** Full column set in `C:/PR/artifacts/t359/bridges359.tsv` (header + 12 data rows); `area_limit` is 50.0 and `axis_limit` 0.436 rad on every row, and `measured_join` is 1 for all six pairs (ticket 348, column 9).

| pair | pose | gate_that_refused | logged_area | logged_axis_rad | replay_area | replay_axis_rad | found_inliers | true_area | true_axis_rad | true_inliers |
|---|---|---|---|---|---|---|---|---:|---:|---:|
| 6-7 | found | pair_overlap, 77.3877 > 50.0 (cand 166) | 77.3877 | 0.170908 | 77.3878 | 0.170906 | 22 | na | na | na |
| 6-7 | true | pair_overlap, 191.1738 > 50.0 | na | na | na | na | na | 191.1738 | 0.000000 | 6 |
| 3-5 | found | none, candidate 118 survived | 0.0000 | 0.221887 | 0.0000 | 0.221887 | 24 | na | na | na |
| 3-5 | true | none | na | na | na | na | na | 0.0000 | 0.000000 | 12 |
| 5-6 | found | none, candidate 158 survived | 10.4472 | 0.148349 | 10.4513 | 0.148352 | 36 | na | na | na |
| 5-6 | true | none | na | na | na | na | na | 4.0000 | 0.000029 | 26 |
| 5-8 | found | none, candidate 164 survived | 8.0000 | 0.140803 | 8.0000 | 0.140800 | 34 | na | na | na |
| 5-8 | true | none | na | na | na | na | na | 0.0000 | 0.000024 | 30 |
| 1-8 | found | none, candidate 70 survived | 1.1251 | 0.019648 | 1.1251 | 0.019647 | 99 | na | na | na |
| 1-8 | true | none | na | na | na | na | na | 0.0000 | 0.000037 | 70 |
| 5-7 | found | none, candidate 162 survived | 2.0000 | 0.077877 | 2.0000 | 0.077879 | 85 | na | na | na |
| 5-7 | true | none | na | na | na | na | na | 0.0000 | 0.000000 | 20 |

Rotation and translation of each found pose away from the true pose (from the logged `rot_deg`/`mat`): 6-7 166.9 deg; 3-5 48.26 deg / 44.19 units; 5-6 13.08 deg / 15.67 units; 5-8 8.32 deg / 5.78 units; 1-8 1.12 deg / 1.31 units; 5-7 4.64 deg / 3.48 units.

**4-7 (reported, not used to conclude).** Candidate 145: true area 6.227, true axis 0.00002 rad, true inliers 53; found area 0.000, axis 0.031 rad, 55 inliers. All three gates pass on a pair that ticket 348 measured as *not* a join, which is exactly why it can decide nothing about the bridges — a passing gate on 4-7 is a false positive, not a lost join.

**Why the true-pose areas are mostly zero.** The median nearest-neighbour distance between the two breaklines at the true pose is 1-8 3.63, 6-7 21.01, 5-7 29.64, 5-8 32.57, 3-5 38.17, 5-6 38.30 units. Only 1-8's mating edges are nearly coincident; the others are complementary edges of the fracture, so a small overlap area at the true pose is the normal case and 6-7's 191.17 is the outlier, not the baseline. The 6-7 figure is also stable — perturbing the true pose by +0.25 deg, +0.5 deg, +0.5 unit moves it only to 190.05, 188.00, 190.00 — so 191 is a property of that pair at that pose, not a knife-edge of my replay.

**Verdicts.**

| bridge | verdict names | number it rests on |
|---|---|---|
| 6-7 | **gate** | pair_overlap area 191.17 at the true pose, against the 50.0 limit (and 77.39 at the found pose) — the gate refuses a ground-truth edge on its own measure. Honest second number: only 6 inliers at the true pose, against 85-99 for the accepted joins, so the measure is refusing a pair the fit also barely scores. |
| 3-5 | **fit** | the fit lands 48.26 deg / 44.19 units from the true pose (candidate 118: 24 inliers, area 0.00, axis 0.2219 rad — every gate passed), while at the true pose all three pass (area 0.00, axis 0.00000 rad, 12 inliers). Candidate 120, the picker's best by rotation, was a failed fit: 0 inliers, score 11.0, area 198.72. |
| 5-6 | **fit** | the fit lands 13.08 deg / 15.67 units off and keeps 36 inliers; at the true pose all three pass (area 4.00, axis 0.00003 rad, 26 inliers). The pair was not refused — it was carried at a wrong pose. |
| 5-8 | **fit** | the fit lands 8.32 deg / 5.78 units off and keeps 34 inliers; at the true pose all three pass (area 0.00, axis 0.00002 rad, 30 inliers). |

**Controls behave.** 1-8 and 5-7 pass at both poses, so the gates do not refuse accepted joins: 1-8 area 1.13 → 0.00, axis 0.01965 → 0.00004 rad, 99 → 70 inliers; 5-7 area 2.00 → 0.00, axis 0.07788 → 0.00000 rad, 85 → 20 inliers.

**A caveat that must travel with this table.** In `C:/PR/artifacts/t355/tree/juglet/Ground Truth/Juglet_simple_graph.txt` the ground-truth edges include 1-8, 3-5 and 6-7 but **not** 5-6, 5-8 or 5-7 (piece 5 is the mixed sherd). Those three are contact-level measured joins from ticket 348. For 5-6 and 5-8, "the true pose passes" therefore means the pair passes at the ground-truth-consistent placement — it does not prove a ground-truth join was lost, only that the pair is not refusable on its own numbers. Their fit-is-the-gap verdict rests on the search having formed the pair and then landing 13.08 / 8.32 deg away, not on the reference graph.

**Two discrepancies with the ticket text above.** (1) The 6-7 logged `pair_overlap` area is **77.3877**, not 76.898: both the CAND row and the DROP row in `binary_juglet_xy_full_r1.log` print 77.3877, and the replay gives 77.3878. The ticket's 76.898 comes from the ticket 350 section and does not appear in this run's log. (2) For 5-6 the ticket says "13.08 deg / 5.22 mm"; the log's `mat` field for candidate 158 is 15.6746 tree units (= 47.02 mm), and 5.22 mm would be 1.74 units, so the second number in that line does not match this run — the 13.08 deg does.

The four bridges do not land in one row. 6-7 is refused by a gate on the gate's own number — that one is a measurement question. 3-5, 5-6 and 5-8 all pass all three measures at the true pose and are lost in the fit — those three are fit questions: for 3-5 48 deg, for 5-6 13 deg from 36 inliers, for 5-8 8 deg from 34. Nothing here points at the merge: no bridge reached a carried edge and was then vetoed.

Reading: mixed

### Lead - verification (2026-11-15, Thinkbook16)

verified: `cd C:/PR/worktrees/structure-from-sherds-pp/359 && COLUMNS=200 python C:/PR/artifacts/dsh-handover/359/accept359.py` -> 19 cases, 19 pass, 0 fail.
verified: `C:/PR/artifacts/t359/anchor359.txt` -> 6 anchor rows, 6 reproducing, 0 missing; worst area error 0.040 percent (row 158, logged 10.447200 against replay 10.451339), worst axis error 0.000003 rad.
verified: `C:/PR/artifacts/t359/bridges359.tsv` -> header plus 12 data rows, 14 columns, no blank verdict.
verified: `C:/PR/artifacts/t358/logs/binary_juglet_xy_full_r1.log:531` -> `CAND 166 6 7 0.604461 22 0.170908 0.170908 1 77.3877 0.156028`, identical at :531 of `binary_juglet_xy_full_r2.log`; `binary_juglet_stock.log:531` -> `CAND 166 6 7 0.548109 16 0.148914 0.148914 1 76.898 0.113475` (the same row at `binary_juglet_signed_on_r1.log:355`). So 76.898 is the stock arm's row for candidate index 166 and 77.3877 is this arm's; the ticket body above is corrected.
verified: candidate 158's `mat` field is 15.6746 tree units, and 15.6746 / 3 = 5.2249 mm - so the ticket's "13.08 deg / 5.22 mm" for 5-6 was right and the write-back's parenthetical "(= 47.02 mm)" divides the wrong way: one tree unit is 3 mm, so a length in units is divided by three, not multiplied.
verified: `class/feature_matching.cpp:885-888` -> `size = overlap_counter / 2; area = overlap_area / 2;` then `if (area > threshold) return true;` - the "area" is half the summed length of the intersecting correspondence segments (`CorIndex.len`), not a surface area, which is why the values are quantised (2.00, 4.00, 8.00, 10.45) rather than continuous.
verified: render `C:/PR/artifacts/t359/render359_bridge_gates.png` (1330x728) opened - 6-7 is the only pair above the 50.0 line at both poses (77.39 found, 191.17 true); every other pair is at or below 10.45.
verified: `git status --porcelain` in the worktree -> the ticket file only, and `git diff HEAD -- class hpc` is empty; nothing committed by the worker.

**Adjudications.**

1. The reading is `mixed` and the split is one gate and three fits, but the 6-7 half must travel with its second number: at the true pose that pair yields **6 inliers** against 20-99 for the pairs the search accepts (1-8 70, 5-7 20) and its two breaklines are a median **21.0 tree units (7.0 mm)** apart. Removing the 50.0 limit would therefore not carry 6-7 by itself - the pair's curves also do not agree closely enough for the correspondence stage. The measurement question here is the curve agreement for pieces 6 and 7, not only the threshold.
2. 3-5 has the same shape at 12 inliers and 38.2 units median separation, so its `fit` verdict also rests on thin correspondence: the 48.26 deg answer is not a fit that had good data to work with.
3. 5-6 (26 inliers) and 5-8 (30) are comparable to an accepted join (5-7, 20); those two are where the ticket's `fit` verdict is strongest.
4. The 4-7 control passing all three gates at the true pose (area 6.227, 53 inliers, axis 0.00002 rad) is what the table needs: "the true pose passes" is necessary, not sufficient, for a join - consistent with ticket 348's frac_btb 0.047.
5. The near-zero true-pose areas are the normal case, not a defect: median nearest-neighbour separation at the true pose runs 3.63 (1-8) to 38.30 (5-6) tree units because the two breaklines are complementary edges of the fracture, not coincident curves. 6-7's 191.17 is the outlier, and the worker's perturbation check (190.05 / 188.00 / 190.00 at +0.25 deg / +0.5 deg / +0.5 unit) shows it is not a knife-edge of the replay.
6. Of the worker's two flagged discrepancies, the first (6-7 area) is accepted and corrected above; the second (5-6 mm) is not a discrepancy - 5.22 mm is the right figure.

**End state:** ready — the replay is anchored on all six logged rows, `gates359.py`, `anchor359.txt`, `bridges359.tsv` (12 data rows) and `render359_bridge_gates.png` are in `C:/PR/artifacts/t359/`, no `class/` or `hpc/` file changed and nothing committed; the lead splits the follow-ups (one measurement ticket for 6-7's overlap measure, one fit ticket for 3-5, 5-6 and 5-8) and lands this ticket.

