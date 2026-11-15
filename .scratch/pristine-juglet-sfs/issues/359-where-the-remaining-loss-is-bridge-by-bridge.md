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
| 6-7 | candidate 166 formed and was dropped as pair_overlap with area 76.898 against the 50.0 limit (ticket 350 section; log line in binary_juglet_xy_full_r1.log) | whether the overlap area at the true pose is also over 50, or whether the fit's own pose inflated it |
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

- [ ] gates359.py exists, runs from the laptop with no Spartan job, and prints the anchor comparison row by row.
- [ ] The anchor reproduces every logged area within 2 percent and every logged axis within 0.01 rad, or the write-back says exactly which rows it could not and ends incomplete.
- [ ] bridges359.tsv has 12 data rows and the columns above, with no blank verdict.
- [ ] Each of the four bridges has a verdict naming one of gate / fit / merge, with the number it rests on.
- [ ] The controls behave: 1-8 and 5-7 show their gates passing, 4-7 is reported and not used to conclude anything about the bridges.
- [ ] One render, C:/PR/artifacts/t359/render359_bridge_gates.png, showing for each pair the overlap area at the found pose and at the true pose against the 50.0 line (a bar chart is enough; the point is the two poses side by side).
- [ ] No change under class/ or hpc/, no assembler run, nothing committed, no other ticket touched.
- [ ] The write-back in the ticket has the table, the anchor printout, the reading line, and exactly one End state line.

## Comments

- **2026-11-15 (lead):** written after a read-only review of ticket 358 (cursor, claude-opus-5-5, session t358-next) which found that the ticket's own summary sentence "3-5 never reaches CAND338" is wrong - candidate 118 is in the log at 48.26 deg - and that only four of the nine seated joins (1-2, 1-8, 5-7, 6-9) were inside the bar at the end of the pair fit, the other four coming inside during assembly. Both are corrected in S1. This ticket exists because the remaining decision - measurement, fit or merge - cannot be taken without the three numbers measured at the true pose, and that measurement is offline and cheap.
