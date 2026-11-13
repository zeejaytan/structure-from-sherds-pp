# Ticket 342 — which join does the winning assembly carry

**Status:** ready-for-agent
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`; work only in the worktree it names
**Answers:** S1
**Depends on:** 340 (which candidate the search accepts), 338 (the winner and the assembled pair errors)
**Blocks:** whether the next change is in the search or in the assembly — nothing should be changed in either until this is answered
**Worker:** unassigned
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

- [ ] `win trace: on`/`off` in the respective arms; zero `[SFS-342]` lines with the switch off.
- [ ] Additions only in `class/`; no threshold, gate, `TOP_k`, `BRANCH_b` or ordering change; no GT in `class/`.
- [ ] Part B filled for both trace-on repeats, and the two repeats compared.
- [ ] Signed 5/3 reads 2/9 and 1/18 in the trace-on arms and in the fidelity arm; Pot A 8/8 and 15/15.
- [ ] Tree md5 identical before/after per signed arm; `sacct` State/ExitCode recorded.
- [ ] `## Comments (worker, <date>)` appended, ending with exactly one `**End state:**` line.

**Report, do not conclude.**
