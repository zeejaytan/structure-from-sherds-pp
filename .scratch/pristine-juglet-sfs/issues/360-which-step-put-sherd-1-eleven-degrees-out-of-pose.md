# Ticket 360 - Which step put sherd 1 eleven degrees out of pose

**What to build:** an offline trace, from the two winning-arm logs of ticket 358, of the moment the join 1-4 entered the assembly - which edge carried it, what its relative pose was at three moments (after that edge's own pair fit, after the group step that placed it, and at the end), and whether the 11.75 deg now measured was already there when the edge was chosen.

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-15)
**Branch:** 360-which-step-put-sherd-1-eleven-degrees-out-of-pose, in /c/PR/worktrees/structure-from-sherds-pp/360
**Worker:** dsh one-shot subagent w360, dispatched 2026-11-15 by the lead
**Answers:** S1
**Needs-eye:** no
**Depends on:** 358, 342, 346
**Blocked-by:** none
**Budget:** small (offline on the laptop from logs already in C:/PR/artifacts/t358/logs; no Spartan job)
**Hand-over:** DeepSeek

---

## Why this ticket

Ticket 358's winning arm seats nine of fourteen measured joins inside 5 deg and 5 mm, but piece 1 sits inside the largest cluster, {1,2,3,4,6,8,9}, eleven degrees out of pose (final 11.75 deg / 2.83 mm on the join 1-4 in factorial358_juglet.tsv). A piece can be in the right cluster and in the wrong pose, and this is the only one that is. If the error came in with the edge, the question is why that edge was preferred when a pair-stage candidate for 1-4 existed at 5.76 deg (candidate 37, ticket 350's replay table). If it was added by the group step that placed the new root, the question is the group refinement. The two answers point at different code.

## Part A - find the moment

Sources: C:/PR/artifacts/t358/logs/binary_juglet_xy_full_r1.log and _r2.log, the winning arm, both repeats. Tokens available: [SFS-336] NODE / NODEOUT / STEP / CUT / KEEP / CLUSTER / OUTSTATE, [SFS-338] CAND338, [SFS-342] WINNER / WIN, [SFS-340] PICK, and the Runner's own registration lines.

Establish, for each repeat:

1. the step and state at which piece 1 and piece 4 first share a cluster, and the root of that cluster (the log names cluster roots and scores - in r1 the largest cluster's root is sherd 4 with score 437);
2. the edge that was accepted at that moment (its y, x, index_, score_, inlier_ - the PICK and WIN lines carry them), and whether that edge is the pair-stage candidate 37, another candidate for 1-4, or an edge that is not a 1-4 candidate at all;
3. the relative transform of 1 with respect to 4 at three moments, taken from matrices the log already prints - the pair fit's own answer (the WIN / CAND row for that edge), the state's node or cluster matrices immediately after the step in which 1 and 4 joined, and the final OUTSTATE - converted to a rotation error against the ground-truth relative pose (C:/PR/artifacts/t355/tree/juglet/Ground Truth/Juglet_Piece_N_T.txt; one tree unit = 3 mm).

Compute the same three numbers for one control join in the same cluster that ends inside the bar (1-8 and 6-9 are both convenient), so the table shows what a good edge looks like at the same three moments. Say plainly which matrix in the log each number was read from, or which was derived and how.

## Part B - the table

Write C:/PR/artifacts/t360/pose360.tsv with 8 data rows at minimum (2 pairs x 2 repeats x the moments that exist; all three exist, so the table carries 12) plus a header, columns:
pair, repeat, moment, source_line, rot_deg, mm, in_bar, edge_index, edge_score, edge_inlier, note.
Moments are: edge_fit, after_join, final. Where a moment does not exist for a pair, print na with the reason.

## Part C - readings

- **the-edge-came-in-wrong** - the 1-4 edge's own fit answer is already at or near 11.75 deg, while the candidate table offered a 1-4 candidate at about 5.76 deg. The defect is in which candidate the search keeps (selection key), and the next ticket is about that key - not about the group step.
- **the-group-step-added-it** - the edge's fit answer is inside the bar and the rotation error grows by more than 5 deg between after_join and final. The defect is the group refinement (GraphAxisRefinement / the state's move of the new root), and the next ticket is about that.
- **both-contribute** - the edge is outside the bar and the group step adds a further few degrees. Record both numbers; the lead will split the follow-ups.
- **trace-incomplete** - the log does not carry the matrices needed at one of the moments. Say exactly which, and end incomplete.

One reading, on its own line at the end of the write-back, prefixed Reading:.

## Part D - acceptance

- [x] pose360.tsv has 12 data rows (the 8-row spec assumed two moments; all three exist) and the columns above, every row naming the log line it came from.
- [x] Both repeats are traced, and the two repeats are compared explicitly (do they agree on which edge carried 1-4 and on the moment the error appears?).
- [x] The control join is traced the same way and lands inside the bar at all three moments, or the difference is explained.
- [x] The ground-truth relative pose is computed from the files, not quoted; state the formula and the units (tree units, divide by 3 for mm).
- [x] One render, C:/PR/artifacts/t360/render360_pose_moments.png: for 1-4 and the control, the rotation error at the three moments as three points or bars, in degrees, with the 5 deg bar drawn.
- [x] No Spartan job, no change under class/ or hpc/, no assembler run, nothing committed, no other ticket touched.
- [x] The write-back has the table, the two-repeat comparison, the reading line, and exactly one End state line.

## Comments

- **2026-11-15 (lead):** written after the read-only review of ticket 358 (cursor, claude-opus-5-5, session t358-next), which proposed this as its second-ranked direction: the 1-4 error is the only in-cluster mis-pose left, it is cheap to trace offline, and the answer decides whether the next code change is in candidate selection or in the group refinement. It is independent of ticket 359 (which asks about the four missing bridges), and neither touches class/, so the two may run in either order.

## Comments (worker, 2026-11-15)

**Trace.** Read-only, from C:/PR/artifacts/t358/logs/binary_juglet_xy_full_r1.log and _r2.log (the winning arm, both repeats) and the GT files in C:/PR/artifacts/t355/tree/juglet/Ground Truth. Scripts: C:/PR/artifacts/t360/build360.py (parse + table) and C:/PR/artifacts/t360/png360.py (render); probes probe5..probe13 in the same folder. Nothing under class/ or hpc/ was touched, no job ran, nothing committed.

**Which step first puts 1 and 4 in one cluster.** Step 5, in the build whose PRIO row is "step 5, ext 1, order 2, graph 2, root_num_ 4, inlier_ 52, merge_graph 0, ck.node 1" (r1 L10342, r2 L10296): the child graph is {1,2,4,6,8,9}, root 4. That graph later merges with {3} at step 6 (edge 1-3, index_ 1, chunk 149) into the final cluster {1,2,3,4,6,8,9}, root 4, score 437 (CLUSTER line r1 L11853 / r2 L11850; the step-6 build's own VETO score is the same 437, and its matrices equal the FINAL ones).

**The edge that placed sherd 1: index_ 4, y 1, x 4, score 0.486727, 52 inliers.** PICK "[SFS-340] PICK 5 46 2 1 4 4 0.486727 52 52" (r1 L10379) and "PICK 5 45 2 1 4 4 0.486727 52 52" (r2 L10333) - the step-5 parent state id differs between repeats (46 vs 45), which is why ids are not keys; WIN row 2 under WINNER 53 518 4 2 (r1 L11839, r2 L11836); GEDGE "5 51 2 4 1 4 0.486727" for the child the build produced. It is **not** pair-stage candidate 37. 37 is still the pair stage's best-scoring 1-4 candidate (score .447109, 49 inliers, its own fit 5.7612 deg, r1/r2 L273), but the step does not use the pair-stage list: it regenerates its own candidate rows (RCAND) against the two graphs and applies the one with the most chunk inliers (52 vs 49). The two index_ values live in different lists, so "index 4" there is not "the 5th 1-4 pair candidate".

**The 11.75 deg was not in the edge's own answer.** At phase init - after the edge was applied and before any ICP - the 1-4 relative pose is already 8.0324 deg / 2.0854 mm out. The rest is added inside the assembly: +2.40 deg at the join step's own IcpIncGraphAxis + GraphAxisRefinement (after_join 10.4373 / 2.6290), then +1.32 deg at the step-6 1-3 merge's ICP (11.7539 / 2.8302), which the final answer keeps unchanged.

| pair | repeat | moment | source_line | rot_deg | mm | in_bar | edge_index | edge_score | edge_inlier |
|---|---|---|---|---|---|---|---|---|---|
| 1-4 | r1 | edge_fit | L10346 NODE 5 1 2 2 init | 8.0324 | 2.0854 | no | 4 | 0.486727 | 52 |
| 1-4 | r1 | after_join | L10380 NODE 5 1 2 2 veto | 10.4373 | 2.6290 | no | 4 | 0.486727 | 52 |
| 1-4 | r1 | final | L11854/L11857 FINAL pieces 1,4 | 11.7539 | 2.8302 | no | 4 | 0.486727 | 52 |
| 1-8 | r1 | edge_fit | L1122 NODE 0 0 0 0 init | 1.1250 | 0.4355 | yes | 70 | 0.402177 | 99 |
| 1-8 | r1 | after_join | L1136 NODE 0 0 0 0 veto | 1.0998 | 0.4427 | yes | 70 | 0.402177 | 99 |
| 1-8 | r1 | final | L11854/L11861 FINAL pieces 1,8 | 1.2145 | 0.4573 | yes | 70 | 0.402177 | 99 |
| 1-4 | r2 | edge_fit | L10300 NODE 5 1 2 2 init | 8.0324 | 2.0854 | no | 4 | 0.486727 | 52 |
| 1-4 | r2 | after_join | L10334 NODE 5 1 2 2 veto | 10.4373 | 2.6290 | no | 4 | 0.486727 | 52 |
| 1-4 | r2 | final | L11851/L11854 FINAL pieces 1,4 | 11.7539 | 2.8302 | no | 4 | 0.486727 | 52 |
| 1-8 | r2 | edge_fit | L1122 NODE 0 0 0 0 init | 1.1250 | 0.4355 | yes | 70 | 0.402177 | 99 |
| 1-8 | r2 | after_join | L1136 NODE 0 0 0 0 veto | 1.0998 | 0.4427 | yes | 70 | 0.402177 | 99 |
| 1-8 | r2 | final | L11851/L11858 FINAL pieces 1,8 | 1.2150 | 0.4574 | yes | 70 | 0.402177 | 99 |

Row count: the ticket's Part B says 8 data rows for "2 pairs x 2 repeats x 2 moments that exist". All three moments do exist for both pairs in both repeats, so C:/PR/artifacts/t360/pose360.tsv carries 12 rows (3 x 2 x 2, same columns, every row naming its log line); no moment was dropped, and nothing is "na".

**Which log matrix each number comes from.** edge_fit is the state's own matrices at the init phase of the build that applied the edge ([SFS-336] NODE, P = T_[i] @ AXIS[i]); the edge's own stored answer is the [SFS-336] RCAND row for that edge (r1 L9584, r2 L9538), but for a merge build (is_outside = 0) that row holds the moving chunk's incremental transform, not the pair pose - read as a pair pose it gives 70.93 deg forward / 8.69 deg inverted, so the applied pair pose is taken from the state at init (8.0324) instead. For the control's step-0 build (merge_graph = -1, outside) the row's forward matrix *is* the pair answer and reads 1.1265 deg, matching the state's 1.1250 at init. after_join is the same build at phase veto (end of that step). final is the pair of [SFS-336] FINAL matrices, raw (they already carry the axis). Every WIN/RCAND/index_ field was read with the printers' own field order, not guessed: class/ranking_system.cpp:182 (WIN), class/feature_matching.cpp:2165 (RCAND), class/ranking_system.cpp:903 and :1471 (STEP), :1643 (NODE).

**GT relative pose, formula and units.** rel = inv(pose_y) @ pose_x and relGT = inv(gt_y) @ gt_x, with the GT translations multiplied by 3.0 (the files are in tree units, one tree unit = 3 mm). rot_deg = acos((trace(rel @ inv(relGT)) - 1) / 2); mm = RMS |(rel p + t) - (relGT p + t')| over piece x's breakline points, divided by 3. This is the t358 authority (audit_attrition358.final_pair + analyze355.rms_disp, BAR_DEG = BAR_MM = 5) and it reproduces the t358 table exactly: 1-4 final 11.7539 deg / 2.8302 mm. Note that C:/PR/artifacts/t358/lead_placeerr.py prints a different "mm" (pose-translation norm / 3, 663.95 for 1-4) - that is that script's own normalisation, not the placement metric this ticket's 2.83 mm refers to.

**Two repeats.** They agree on everything that decides the answer: the same edge (index_ 4, score 0.486727, 52 inliers) at the same step-5 build, digit-identical NODE matrices for the 1-4 graph at all four phases (8.0324 at init -> 10.4373 at veto, the same in both repeats), the same final 11.7539 deg / 2.8302 mm, and the same winner (trace 53, score 518, 2 graphs) and cluster root 4 / score 437. Only the line numbers differ, plus the control's final at the 4th decimal (1.2145 vs 1.2150 deg, 0.4573 vs 0.4574 mm).

**Control (1-8, ends inside the bar).** Placed at step 0 by an outside build (PRIO r1/r2 L1119: root 1, 99 inliers, merge_graph -1, ck.node 8, edge index_ 70, y 1, x 8, score .402177; PICK L1135). 1.1250 -> 1.0998 -> 1.2145 deg, 0.4355 -> 0.4427 -> 0.4573 mm: inside the bar at all three moments, and its own fit answer (1.1265) matches the applied one (1.1250) to 0.0015 deg. So a good edge looks like this at the same three moments, and the 1-4 loss is not a property of the trace.

**The render.** C:/PR/artifacts/t360/render360_pose_moments.png: left panel rotation error at the three moments for 1-4 and the 1-8 control with the 5 deg bar drawn, right panel the same for the breakline RMS with the 5 mm bar; repeat 2 overlaid as open circles.

**Reading, with the split.** The 1-4 edge came in at 8.03 deg, already 3.03 deg outside the bar, while the pair stage had offered a 1-4 candidate at 5.76 deg - 8.03 of the final 11.75 deg is therefore the edge's own answer (69%), a selection-key result, not a refinement artefact. The remaining 3.72 deg is added afterwards by group ICP: +2.40 at the join step itself and +1.32 at the step-6 1-3 merge. Neither branch alone explains the final number, so both are recorded for the lead rather than claimed as one defect.

Reading: both-contribute

### Lead - verification (2026-11-15, Thinkbook16)

verified: `cd C:/PR/worktrees/structure-from-sherds-pp/360 && COLUMNS=200 python C:/PR/artifacts/dsh-handover/360/accept360.py` -> 17 cases, 17 pass after I relaxed my own row-case from "exactly 8 data rows" to "8 to 12" (the ticket assumed two moments; all three exist for both pairs in both repeats, so the table rightly carries 12).
verified: an independent recompute of every rotation in the table, with my own parser and my own GT reader (`C:/PR/artifacts/t360/lead_rows360.py`: `P = T @ AXIS` from the log's own [SFS-336] AXIS line, GT from `Juglet_Piece_N_T.txt` with translations x 3, `rot = angle(rel @ inv(relGT))`) -> 12 rows, 0 mismatches above 0.01 deg; the largest deviation is 0.0001 deg (1-4 edge_fit 8.0324, after_join 10.4373, final 11.7539; control 1-8 1.1250, 1.0998, 1.2145 / 1.2150).
verified: the applied edge is `[SFS-340] PICK 5 46 2 1 4 4 0.486727 52 52` at `binary_juglet_xy_full_r1.log:10379` and `PICK 5 45 ...` at `_r2.log:10333`, under `[SFS-342] WINNER 53 518 4 2` at r1 :11839; candidate 37 is a different list - its CAND338 row is `binary_juglet_xy_full_r1.log:794 CAND338 37 1 4 0.447109 49 3.96365 1 5.76136 5.42581`.
verified: the two moments' matrices are the ones cited - `binary_juglet_xy_full_r1.log:10346 NODE 5 1 2 2 init 1 0` and `:10380 NODE 5 1 2 2 veto 1 0`, with piece 4 carrying rootFlag 1 and the identity matrix in both.
verified: the render `C:/PR/artifacts/t360/render360_pose_moments.png` (1840x784) opened - 1-4 8.03 / 10.44 / 11.75 deg against the 5 deg line, control 1-8 1.12 / 1.10 / 1.21 inside it at all three moments; the RMS panel shows 2.09 / 2.63 / 2.83 mm against a 5 mm bar.
verified: `git status --porcelain` in the worktree -> the ticket file only; `git diff HEAD -- class hpc` empty; nothing committed, no job run.

**Adjudications.**

1. The reading is `both-contribute` and the split stands: 8.03 of 11.75 deg (69 %) is the edge's own answer at the moment it was applied, and 3.72 deg is added afterwards by group ICP (+2.40 at the join step, +1.32 at the step-6 1-3 merge). The import for the next step is that the selection key, not the refinement, is where most of this loss is made - a 5.76 deg 1-4 candidate existed and the step regenerated its own candidates and took a 52-inlier one at 8.03 deg instead.
2. The control carries the trace: 1-8 sits at 1.12 / 1.10 / 1.21 deg at the same three moments, so the instrument is not manufacturing the 1-4 number.
3. Two repeats agree on every value that decides the answer (same edge, same step, digit-identical matrices for the 1-4 graph, same winner and cluster root); only line numbers and the control's fourth decimal differ.
4. Nothing in the mm column carries the verdict: 1-4 is out on rotation at all three moments (8.03 > 5), so the "no" does not depend on the breakline-RMS measure, which I did not recompute independently.
5. Citation correction: the write-back cites the pair-stage 5.7612 deg for candidate 37 at "r1/r2 L273", but L273 is the `CAND` row (axis_raw 0.0691787 rad = 3.96 deg); the 5.7612 deg figure is the `CAND338` row at L794. The number is right, the line reference is not.
6. Row count: my Part B asked for 8 data rows assuming only two moments existed; all three do, in both repeats, so the 12-row table is the correct shape and I made the hidden check accept it rather than the worker trimming data.

**End state:** ready
