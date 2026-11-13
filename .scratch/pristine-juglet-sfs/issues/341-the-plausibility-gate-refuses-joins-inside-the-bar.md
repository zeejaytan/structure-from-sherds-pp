# Ticket 341 — the plausibility gate refuses joins that are inside the bar

**Status:** needs-info — waits for ticket 340 (the applied candidate must be named before a veto can be called wrong)
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`, only after 340 lands
**Answers:** S1
**Depends on:** 340 (which candidate is applied), 339 (the veto reasons are already printed), 338 (the candidate table)
**Blocks:** any change to the plausibility gate or its bars
**Worker:** unassigned
**Budget:** small (4 failed acceptance runs)
**Hand-over:** DeepSeek: numbers behind an existing `fail_reason`; the hidden check is the veto table in Part B.

> **In one line.** Three pairs whose best candidate is inside the 5 deg / 5 mm bar are offered at
> `j = 0`, tried, and refused eight, seven and five times by `CheckGraphPlausibility` — the gate says
> no to joins the answer key says are nearly right. This ticket records the numbers the gate is using
> when it says no, and nothing else.

## What is already measured (do not re-measure)

`SFS_ADMIT_TRACE=1` (ticket 339) already prints `[SFS-339] FATE ... veto <reason> <value>` for every
refusal, with the `fail_reason` string. Read out of the five trace-on logs of the 339 run
(`C:/PR/artifacts/t339/logs/binary_juglet_signed_on_r{1,2,3,6,7}.log`), 45-48 vetoes per run:

| pair | best candidate (CAND338) | in bar? | offers | minimum j | refusals | reasons |
|------|--------------------------|---------|--------|-----------|----------|---------|
| 5-7 | id 160, 4.71 deg / 3.69 mm, inlier 72 | yes (4.71 < 5) | 10, 8, 10, 10, 10 | 0 | 8, 6, 8, 8, 8 | `PC_` every time |
| 4-7 | id 145, 3.35 deg / 2.68 mm, inlier 28 | yes | 22, 20, 20, 20, 20 | 0 | 7, 7, 6, 6, 6 | `PC_` every time |
| 6-9 | id 170, 2.35 deg / 1.72 mm, inlier 18 | yes | 10, 14, 9, 9, 9 | 0 | 3, 5, 3, 3, 3 | `Overlap_73.70`, `Overlap_76.06`, `Overlap_72.87` |

Across a whole run the reason census is `PC_` 30-34 of 45-48 refusals, the rest `Overlap_<n>_` with
one `Score_` in r2. So the gate is not refusing a rare case: it is refusing most attempts, and three
of the refusals it makes are on pairs whose best candidate is inside the bar.

Seam 1-2 is the other side of the same coin: it *is* kept (4 keeps in r1, 2 in r2) and its final pair
still reads 84.40 deg / 93.99 mm (338). Ticket 340 is what says whether the kept edge is the in-bar
candidate.

## Part A — the instrument

Extend the existing veto print; do not add a new switch and do not re-implement the reason string.
Inside `CheckGraphPlausibility` (`class/ranking_system.cpp:2211-2292`), at the point the reason is
set, add the numbers that produced the verdict:

```
[SFS-341] VETO <step> <state> <j> <y> <x> <reason> <value_a> <value_b> <value_c>
```

- `PC_` — the point-cloud inlier count that failed, the distance bar it was compared against, and
  the rim-gap value (report `-1` for a value the code does not compute at that site; do not invent
  one).
- `Overlap_<n>_` — `<n>` as printed, the bar, and the number of edges in the chunk.
- `Score_` — `graph_score_`, the bar, and the chunk's `inlier` sum.

Gated by `SFS_VETO_TRACE=1` (exact string, default off, banner `veto trace: on|off`). Additions only;
no threshold, gate, `TOP_k`, `BRANCH_b` or ordering change; GT stays out of `class/`.

## Part B — the table (the deliverable)

One row per distinct refusal of the three pairs above, per trace-on repeat:

| pair | repeat | step/state/j | reason | value_a | value_b | value_c | the candidate's own error (CAND338) | reading |
|------|--------|--------------|--------|---------|---------|---------|------------------------------------|---------|

Reading per pair, exactly one of: `refused-while-in-bar` (the numbers show the gate rejecting a pair
whose candidate is inside the bar), `refused-on-its-own-data` (the PC inlier count is near zero or
the rim gap is empty, so the edge's own point data is weak and no bar change would help),
`refused-on-pose` (the edge's applied pose is not the candidate's — hand this to 340's finding).

## Part C — arms

As 339 and 340: 3x tree, `SFS_TOPK`/`SFS_BRANCH` unset.

| arm | switches | repeats | pass |
|-----|----------|---------|------|
| signed 5/3 trace on | `SFS_VETO_TRACE=1` (+ `SFS_EDGE_TRACE=1` if 340's table needs it) | 2 | 2/9 sherds, 1/18 edges |
| signed 5/3 fidelity | all unset | 1 | the same 2/9 and 1/18, else the instrument is void |
| Pot A | unset | 1 | 8/8 and 15/15, else the ticket is void |

## Part D — readings (apply unchanged)

1. **`refused-while-in-bar`.** The gate's bar is refusing a join the answer key calls right, six to
   eight times, on three different pairs. The next ticket is a **bar experiment on the PC test**, and
   it must be bounded: one arm with that bar relaxed, signed 5/3 accuracy must rise above 2/9 sherds
   and 1/18 edges, and Pot A must stay 8/8 and 15/15. No other threshold moves in the same ticket.
2. **`refused-on-its-own-data`.** The pair's own point-cloud evidence is weak at that moment. No gate
   change is justified; the next ticket is where that evidence is computed, not the bar.
3. **`refused-on-pose`.** The edge the gate sees is not the candidate the table lists. Say so and
   stop: 340 must explain the gap first.
4. Instrument void if the fidelity arm differs from 2/9 and 1/18, or Pot A is not 8/8 and 15/15.

## Acceptance

- [ ] `veto trace: on`/`off` in the respective arms; zero `[SFS-341]` lines with the switch off.
- [ ] Additions only in `class/`; no threshold, gate, `TOP_k`, `BRANCH_b` or ordering change.
- [ ] Part B filled for 5-7, 4-7 and 6-9 in both trace-on repeats, with the three numbers per refusal.
- [ ] Signed 5/3 2/9 and 1/18 in the trace-on arms and the fidelity arm; Pot A 8/8 and 15/15.
- [ ] `sacct` State and ExitCode recorded for every job id.
- [ ] `## Comments (worker, 2026-11-13)` section appended, ending with exactly one `**End state:**` line.

**Report, do not conclude.** No gate is changed in this ticket.
