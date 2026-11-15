# Ticket 360 - Which step put sherd 1 eleven degrees out of pose

**What to build:** an offline trace, from the two winning-arm logs of ticket 358, of the moment the join 1-4 entered the assembly - which edge carried it, what its relative pose was at three moments (after that edge's own pair fit, after the group step that placed it, and at the end), and whether the 11.75 deg now measured was already there when the edge was chosen.

**Status:** ready-for-agent
**Worker:** unassigned
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

Write C:/PR/artifacts/t360/pose360.tsv with 8 data rows (2 pairs x 2 repeats x 2 moments that exist) plus a header, columns:
pair, repeat, moment, source_line, rot_deg, mm, in_bar, edge_index, edge_score, edge_inlier, note.
Moments are: edge_fit, after_join, final. Where a moment does not exist for a pair, print na with the reason.

## Part C - readings

- **the-edge-came-in-wrong** - the 1-4 edge's own fit answer is already at or near 11.75 deg, while the candidate table offered a 1-4 candidate at about 5.76 deg. The defect is in which candidate the search keeps (selection key), and the next ticket is about that key - not about the group step.
- **the-group-step-added-it** - the edge's fit answer is inside the bar and the rotation error grows by more than 5 deg between after_join and final. The defect is the group refinement (GraphAxisRefinement / the state's move of the new root), and the next ticket is about that.
- **both-contribute** - the edge is outside the bar and the group step adds a further few degrees. Record both numbers; the lead will split the follow-ups.
- **trace-incomplete** - the log does not carry the matrices needed at one of the moments. Say exactly which, and end incomplete.

One reading, on its own line at the end of the write-back, prefixed Reading:.

## Part D - acceptance

- [ ] pose360.tsv has 8 data rows and the columns above, every row naming the log line it came from.
- [ ] Both repeats are traced, and the two repeats are compared explicitly (do they agree on which edge carried 1-4 and on the moment the error appears?).
- [ ] The control join is traced the same way and lands inside the bar at all three moments, or the difference is explained.
- [ ] The ground-truth relative pose is computed from the files, not quoted; state the formula and the units (tree units, divide by 3 for mm).
- [ ] One render, C:/PR/artifacts/t360/render360_pose_moments.png: for 1-4 and the control, the rotation error at the three moments as three points or bars, in degrees, with the 5 deg bar drawn.
- [ ] No Spartan job, no change under class/ or hpc/, no assembler run, nothing committed, no other ticket touched.
- [ ] The write-back has the table, the two-repeat comparison, the reading line, and exactly one End state line.

## Comments

- **2026-11-15 (lead):** written after the read-only review of ticket 358 (cursor, claude-opus-5-5, session t358-next), which proposed this as its second-ranked direction: the 1-4 error is the only in-cluster mis-pose left, it is cheap to trace offline, and the answer decides whether the next code change is in candidate selection or in the group refinement. It is independent of ticket 359 (which asks about the four missing bridges), and neither touches class/, so the two may run in either order.
