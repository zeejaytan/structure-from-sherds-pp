# Ticket 342 — which join does the winning assembly carry

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 342-which-join-does-the-winning-assembly-carry, in /c/PR/worktrees/structure-from-sherds-pp/342
**Worker:** dsh one-shot subagent 5e1768bc, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 340 (which candidate the search accepts), 338 (the winner and the assembled pair errors)
**Blocks:** whether the next change is in the search or in the assembly — nothing should be changed in either until this is answered
**Budget:** small (4 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: one dump of the winning state's edges; the hidden check is the table in Part B.

> **In one line.** Ticket 340 showed that pair 1-2 is accepted with its in-bar candidate (id 1,
> 2.14 deg / 2.20 mm) at step 2 — and that a later state accepts the same pair again at step 3 with a
> different edge (`index_` 3). The pair still reads 84.40 deg in the final assembly. This ticket asks
> the one question that separates the two explanations: which of those edges does the winning state
> actually carry?

## What is already measured (do not re-measure)

- The candidate table: `[SFS-338] CAND338 id y x score_ inliner_ axis_raw_deg gt_edge rot_deg mat_mm`
  (`hpc/tools/main_pristine_batch.cpp:352-355`). Pair 1-2's in-bar row is id 1 at 2.14 deg / 2.20 mm.
- Accepts, from 340's `[SFS-340] PICK` lines (31 per trace-on repeat, identical in both repeats):
  `PICK 2 14 0 1 2 1 0.494465 40 78`, `PICK 2 15 0 1 2 1 0.494447 40 78`,
  `PICK 3 19 0 1 2 3 0.534665 46 84`.
- The assembled pair reads 84.40 deg / 93.99 mm (338, both repeats, the signed 5/3 arm).
- The winner's cluster composition differs between repeats at identical accuracy (338), so the winner
  state id is not stable — join on state composition, not on the id.

## Part A — the instrument

One switch `SFS_WIN_TRACE=1` (exact string, default off, banner `win trace: on` / `win trace: off`,
beside the other banners in `InitSfsSwitches`). At the point the run commits a state as the answer —
the same place that produces the assembly the accuracy lines are counted from — walk **that state's
edge list** and print, GT-free, one line per edge in the state's own order:

```
[SFS-342] WIN <state_trace_id> <rank_in_list> <y> <x> <index_> <score_> <inliner_>
```

and once, before those lines, a header with the state's final score and its edge count:

```
[SFS-342] WINNER <state_trace_id> <state_score_> <n_edges> <n_clusters>
```

Rules: additions only; no sort, no mutation, no reference outliving the call; ground truth stays out of
`class/`; no threshold, gate, ordering, `TOP_k`, `BRANCH_b` or width change; with the switch off the
program executes exactly what it executes today. If the winning state is not expressed as an edge list
at that point, print the closest structure that is (a cluster's edges, the saved graph) and say in the
write-back what you printed and why — do not invent a second notion of "winner".

## Part B — the table (the deliverable)

One block per trace-on repeat:

| repeat | winner state id | state score | n edges | edges for pair 1-2 (index_, score_, inliner_) | id 1 in the winner? | id 3 in the winner? | 338's assembled error for 1-2 |
|--------|-----------------|-------------|---------|-----------------------------------------------|---------------------|---------------------|-------------------------------|

Also state, for each repeat, whether any edge of the winner is for pair 7-9 or 1-9 (expected: none,
because 340 found zero accepts for them).

## Part C — arms

As 340: 3x Juglet tree, `SFS_TOPK` / `SFS_BRANCH` never set.

| arm | switches | repeats | pass |
|-----|----------|---------|------|
| signed 5/3, trace on | `SFS_WIN_TRACE=1` | 2 | SHERD=2/9 EDGE=1/18 |
| signed 5/3, fidelity | all unset | 1 | the same 2/9 and 1/18, else the instrument is void |
| Pot A, default | all unset | 1 | 8/8 and 15/15, else the ticket is void |

Tree md5 before and after per signed arm. `sacct` State and ExitCode for every job id.

## Part D — readings

1. **The winner carries id 1.** The accepted in-bar join survived into the answer, and the 84.40 is
   produced after it — in the assembly step that places the joined pieces. The next ticket is that
   step, not the search.
2. **The winner carries id 3.** A different edge for the same pair won at step 3. The next ticket is
   what chooses among several edges of one pair across states (the selection key), and 340's id 1 was
   a correct accept that was later overwritten.
3. **The winner carries no 1-2 edge.** Pieces 1 and 2 are only joined in states that lost. The next
   ticket is the state score that prefers a split over a correct join.
4. **Instrument void** if the fidelity arm differs from 2/9 and 1/18, or Pot A is not 8/8 and 15/15.

## Acceptance

- [x] `win trace: on`/`off` in the respective arms; zero `[SFS-342]` lines with the switch off.
- [x] Additions only in `class/`; no threshold, gate, `TOP_k`, `BRANCH_b` or ordering change; no GT in `class/`.
- [x] Part B filled for both trace-on repeats, and the two repeats compared.
- [x] Signed 5/3 reads 2/9 and 1/18 in the trace-on arms and in the fidelity arm; Pot A 8/8 and 15/15.
- [x] Tree md5 identical before/after per signed arm; `sacct` State/ExitCode recorded.
- [x] `## Comments (worker, <date>)` appended, ending with exactly one `**End state:**` line.

**Report, do not conclude.**

## Comments (worker, 2026-11-14)

**What changed.** One switch, additions only: 37 insertions / 0 deletions over
`class/feature_matching.cpp` (global `g_sfs_win_trace`, `ReadSfsSwitch("SFS_WIN_TRACE", bad_wt)`, banner
`win trace: on|off` beside the other six), `class/feature_matching.h` (`extern`), and
`class/ranking_system.cpp` (`static void Sfs342Winner(const State&)` after `Sfs338GEdges`; called from
`BuildStep` at class/ranking_system.cpp:1340-1343, right after `RemoveSameState(out_state_)`, on
`out_state_[0]` — the same index the shim places the pot from, `count_move_state = 0`). It prints
`[SFS-342] TAB WINNER <trace_id> <state_score> <n_edges> <n_clusters>` then one
`[SFS-342] TAB WIN <trace_id> <rank> <shard_y> <shard_x> <index_> <score_> <inliner_>` per edge of that
state, in the state's own order, clusters then list. No GT in `class/`; nothing sorted or mutated;
thresholds, gates, `TOP_k`, `BRANCH_b`, widths untouched; no earlier ticket's token re-printed.

**Runs** (Spartan, `/data/gpfs/projects/punim2657/SfSpp/runs/20261114-0139_0229a78_t342-winner`):

| arm | job | State | ExitCode | elapsed | banner | accuracy |
|---|---|---|---|---|---|---|
| signed 5/3, trace on, r1 | 35024613_0 | COMPLETED | 0:0 | 6:25 | on | SHERD=2/9 EDGE=1/18 |
| signed 5/3, trace on, r2 | 35024613_1 | COMPLETED | 0:0 | 6:23 | on | SHERD=2/9 EDGE=1/18 |
| signed 5/3, fidelity | 35024613_2 | COMPLETED | 0:0 | 6:27 | off | SHERD=2/9 EDGE=1/18 |
| Pot A, default | 35024237 | COMPLETED | 0:0 | 6:31 | off | SHERD=8/8 EDGE=15/15 |

First attempt `35024236_0/_1/_2` FAILED 1:0 (5:19 / 0:54 / 5:03) — 340's pota script began with
`rm -rf $RUN/work/trees` and deleted the juglet tasks' private trees mid-run; the pota script now owns
`$RUN/pota_work` and never touches `$RUN/work/trees`. Those failed runs' trace lines are not used here.
Tree md5 identical before/after per signed arm (55 files each) and for Pot A (49).

**Part B — the winner.**

| repeat | winner id | state_score | n_edges | n_clusters |
|---|---|---|---|---|
| r1 | 30 | 260 | 4 | 3 |
| r2 | 33 | 263 | 5 | 3 |

| seam | r1 (index_ / score_ / inliner_) | r2 | 340 accepted | 338 assembled (deg / mm) |
|---|---|---|---|---|
| 1-2 | absent | absent | id 1 @ 0.494465 (40), 0.494447 (40); id 3 @ 0.534665 (46) | 84.40 / 93.99 |
| 1-8 | 67 / 0.440855 / 76 | 67 / 0.440855 / 76 | — | 1.24 / 1.33 |
| 7-9 | absent | absent | none | 163.18 / 94.46 |
| 1-9 | absent | absent | none | 89.33 / 102.18 |

r1 holds 5-7 (160), 1-8 (67), 3-8 (97), 4-6 (77). r2 holds 5-7 (160), 4-7 (86), 2-9 (51), 1-8 (67), 3-8 (97).
No edge on 1-2, 7-9 or 1-9 in either winner. The two repeats share the accuracy line and the
1-8 / 3-8 / 5-7 backbone with identical `index_` and score; they differ in state id, score, edge count and
the remaining joins (4-6 vs 4-7 + 2-9). The winning state is not reproducible between repeats; the
accuracy is.

**Verdict slugs.** r1: **carries-none** — the winner's printed edge list holds no 1-2 edge, and neither
id 1 nor id 3 appears. r2: **carries-none**, same reading.

**Caveats.** `RankingSubgraph::edge_` is the accumulated used-edge record, appended only at
class/ranking_system.cpp:2270 (priority entry) and :2334 (bridge into a merge); the merge path copies the
merged graph's nodes (:2312-2320) but not its `edge_`. r2's winner has all nine shards matched
(`End condition : All shards are matched`) yet prints 5 edges over 3 clusters, where a 3-cluster spanning
forest over nine pieces needs 6 — so the printed list is provably a subset of the joins the state used.
"carries-none" therefore means no 1-2 edge was *used as a priority or bridge entry*; it is not a proof
that pieces 1 and 2 never shared a cluster. The final build step's group is `{1,4,5,6,7,9}` in both
repeats (piece 2 absent), which points the same way but is not the whole state. Nothing here measures the
state score against a counterfactual with the 1-2 edge forced in, so Part D's "the state score prefers a
split" remains unmeasured. Renders are shape comparisons, not registrations: the arm frame and the GT
frame do not share an origin.

Attribution, the full tables and seven renders: `C:/PR/artifacts/t342/`.

### Lead - verification (2026-11-14, Thinkbook16)

The lead re-read the code, the logs and the run records before recording anything here.

- `verified:` `python C:/PR/artifacts/dsh-handover/342/accept342.py` -> 36 cases, 36 pass (the lead relaxed one
  case of its own that demanded the unit word beside every figure; the table's header carries deg / mm).
- `verified:` `git -C C:/PR/worktrees/structure-from-sherds-pp/342 diff HEAD -U0 -- class` -> 37 added, 0 deleted
  (`class/feature_matching.cpp` +11, `class/feature_matching.h` +1, `class/ranking_system.cpp` +25). The call
  site is at `class/ranking_system.cpp:1340-1343`, after the sort and `RemoveSameState(out_state_)` and before
  the shard `Move` loop; `Sfs342Winner` only reads `graph_` and `edge_` and prints.
- `verified:` the winner lines in `artifacts/t342/logs/binary_juglet_signed_on_r1.log` (5 lines) and
  `..._on_r2.log` (6 lines); the switch-off log carries 0 `[SFS-342]` lines and the banner appears once per log.
- `verified:` `artifacts/t342/runs/acc342_*.txt` -> trace-on r1 2/9 and 1/18, trace-on r2 2/9 and 1/18, fidelity
  2/9 and 1/18, Pot A 8/8 and 15/15; `runs/sacct342.txt` -> 35024613_0/_1/_2 COMPLETED 0:0 (6:25 / 6:23 / 6:27),
  35024237 COMPLETED 0:0 (6:31).
- `verified:` tree md5 identical before and after in all three signed arms (55 files each) and for Pot A (49).
- `verified:` the final-step GROUP records in both trace-on logs. In r1 the group that takes piece 2 is
  `{2,4,5,6,7,9}` (`[SFS-338] GROUP 6 0 0 2 6 74 175 1 1`, `GROUPNODE 6 0 0 2` then `4 5 6 7 9`); in r2 the same set
  (`GROUP 6 2 0 2 6 74 177 1 1`). Piece 1 is not in the group that takes piece 2 in either repeat.

**Correction to the worker's caveat.** The caveat says the final group is `{1,4,5,6,7,9}` with piece 2 absent.
The log says the opposite: the group carries piece 2 and omits piece 1. The substance the caveat draws is
unchanged - pieces 1 and 2 are not in the same final group - but the membership was mis-copied and must not be
quoted as written.

**What that leaves the answer able to say.** The winner's own printed edge list holds no 1-2 edge in either
repeat, and the last group that takes piece 2 does not take piece 1. Both facts point the same way, and both
are bounded: `edge_` is a record of priority and bridge entries, not of every join inside a merged group
(`class/ranking_system.cpp:2270`, `:2334`; the merge copies nodes at `:2312-2320` but not `edge_`), and r1's
winner prints 4 edges over 3 clusters (r2's log says 'All shards are matched'; r1 prints no such line). So the reading is `carries-none` in the sense
the ticket defined, not proof that pieces 1 and 2 never shared a cluster.

### What this leaves open

Whether the accepted 1-2 edge (candidate id 1, step 2, ticket 340) is lost by the search or only by the
record cannot be settled with a print that reads `edge_`. It needs a node-level print: the cluster id of
each piece in each state that holds the pair, which survives the merges that drop `edge_` entries.

**End state:** ready - code and write-back on the branch; the lead lands and closes.

