# Ticket 339 — which admission boundary drops the join

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** 339-which-admission-boundary-drops-the-join, in /c/PR/worktrees/structure-from-sherds-pp/339
**Answers:** S1
**Depends on:** 338 (the loss is before the fit), 337 (width is not the next lever), 336 (stage trace; leave it)
**Blocks:** the next fix — which one of the gates below to change. Seam 1-2 is a different loss (it is already inside surviving states and dies at winner selection); that ticket waits until this one names the gate for 7-9 and 1-9. The two do not share a boundary.
**Worker:** dsh one-shot subagent 2239fe5e, dispatched 2026-11-13 by the lead
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: the two print tokens and the switch; the hidden check is the seam-by-boundary table in Part B.

> **In one line.** Ticket 338 put seams 7-9 and 1-9 after the pruned candidate list and before a state-graph edge. This ticket names which admission boundary drops each one, with one new gated print, and does not change a gate.

## What 338 already measured (do not re-measure)

Source: `C:/PR/artifacts/t338/attribution.md` and `attribution.tsv`. Relative pose, 5 deg and 5 mm bar. Signed 5/3, jobs `35008961_0` (trace on r1), `35008961_1` (trace on r2), `35008961_2` (trace off), `35008962` (Pot A), all `COMPLETED` ExitCode `0:0`.

| seam | best pruned candidate | edge in a state graph? | final pair | verdict |
|------|----------------------|------------------------|------------|---------|
| 1-8 | id 70, 0.70 deg / 1.21 mm, score 0.447, inlier 70 | yes, from step 0 (71 lines r1, 82 r2) | r1 1.24 deg / 1.33 mm; r2 1.27 / 1.38; one cluster | joined (the control) |
| 7-9 | id 174, 5.39 deg / 4.24 mm, score 0.340, inlier 16 | no line, either repeat | r1 163.18 deg / 94.46 mm; r2 141.23 / 89.88; split | never attempted |
| 1-9 | id 77, 8.82 deg / 3.48 mm, score 0.623, inlier 10 | r1 none; r2 one edge at step 6, state 34 | r1 89.33 deg / 102.18 mm, split; r2 9.12 / 3.62, one cluster | lost; the candidate is already past 5 deg |
| 1-2 | id 1, 2.14 deg / 2.20 mm, score 0.447, inlier 18 | yes, from step 2 (states 0, 2, 22, 24, 25, 28; 13 lines both arms) | r1 84.40 deg / 93.99 mm; r2 32.79 / 41.76; split | out of scope: lost after the fit, at winner selection |

Where an edge did reach the fit, the fit added at most **0.55 deg and 0.15 mm** (`attribution.md`). Accuracy was 2/9 sherds and 1/18 edges on all three signed arms; Pot A was 8/8 and 15/15.

**Instrument caveat, carried forward.** `[SFS-338] GEDGE` prints `LCSIndex::index_` from `Sfs338GEdges`. That value is **not** the `CAND338` id. It prints on `StateManager::BuildState` entry (the parent's edges, before the new edge is applied) and in `BuildStep` only after `BuildState` returns true. An edge that is applied and then vetoed, or applied at a state's last step, can be missing. Presence counts are a **lower bound**. Follow pairs by `shard_y_` / `shard_x_`, never by equating the two ids. `CAND338` itself is printed only when `SFS_EDGE_TRACE=1` (`hpc/tools/main_pristine_batch.cpp`, the block after `PairwisePruning`).

Ticket 337 classed signed 5/3 seams 7-9 and 1-9 as W (a priority entry at index ≥ 3). That run is not this binary: 338 r2 did apply one 1-9 edge. Re-measure. Do not run another width sweep.

## The boundaries in scope

All of these sit after `PairwisePruning` (the candidate is already in that list) and before an accepted state's used-edge list. Read from `class/ranking_system.cpp`, `class/feature_matching.cpp`, `class/ranking_system.h`, `class/reconstruction.h`, `hpc/tools/main_pristine_batch.cpp`. Do not move or retune any of them.

1. **Eligibility (`PickEdges`).** A root graph is constructed from the full pruned list (`RankingSubgraph(list<LCSIndex>&, int)` copies it into `lcs_reference_`). `Initialize` keeps at most `s_` roots per state and `N_` states (`TOP_k` 5 and `BRANCH_b` 3 in `main_pristine_batch.cpp`, unless `SFS_TOPK` / `SFS_BRANCH` are set — this ticket leaves them unset). `PickEdges` then keeps an edge only when exactly one end is already in the graph. Both ends out, or both ends in, means no offer. No erase line exists for that skip; absence of `OFFER` is the evidence.
2. **Named erases, from step 1 only** (`PrepareNextStep`). The step-0 list is not reused. `FeatureCompGraphBuilding` is called with `MINIMUM_NUMBER` (6, `reconstruction.h`). Then `RegistrationPruning` erases `axis_angle_ > 0.436`, or `score_ > 10`, or `overlap_`. Then `RemoveEdgeUsingPCInlier` erases on `SingleOverlapTest` / `MergeOverlapTest`, else on `CountPCInlier(..., 7.0, 1.5, ...)` returning false (`dist > 10`, or both rim gaps positive). Step 0 does not call these.
3. **Same-priority drop** (`RemoveSamePriority`). Only a reciprocal *inside* merge (`graph_index >= 0`) whose two poses pass `isConverge(R, t, 0.1, 10)` is removed, and only the lower `inlier_`. An outside edge is not removed here. `CombineChunk` / `CombineGraphChunk` (`isSimilarTrans(..., 0.436, 20.0)`) group edges; they do not delete a lone edge.
4. **Branch window.** `BuildStep` calls `BuildState` only for priority index `j < s_` (`s_` is `BRANCH_b`, default 3). A later entry is offered and never attempted.
5. **Plausibility veto** (`CheckGraphPlausibility`, the return of `BuildState`). `graph_score_ < 0` sets `Score_`; a pairwise `OverlapCheck_3d` at distance 50 sets `Overlap_`; when `step_counter > 0`, a failed `ProfileChecking(profile, 7.0, 7.0)` or the rim-gap test sets `PC_`. A false return does not keep the child, and GEDGE will not show the new edge.

`PrepareNextStep` also adds a root when `total_priority_.size() < num_init_prio_ * 0.1`. That is a replenish, not a drop.

## Part A — one switch, two tokens

Switch **`SFS_ADMIT_TRACE=1`** (exact string, default off, banner `admit trace: on`). Additions only. Print in the order the containers already have. Do not sort, erase, or rewrite a score, an inlier, a pose, or a list in order to print it. Leave `[SFS-336]`, `[SFS-337]`, and `[SFS-338]` as they are. `class/*` stays free of ground truth.

After `RemoveSamePriority`, one line per distinct pair in each surviving `total_priority_` entry (`shard_y_`, then `shard_x_`, unchanged):

```
[SFS-339] OFFER <step> <state> <j> <nprio> <branch> <y> <x> <inlier>
```

`step` is `g_sfs_tr_step`, `state` is `trace_id_`, `j` is the index `BuildStep` will test, `nprio` is `total_priority_.size()` after the same-priority drop, `branch` is `s_`, `inlier` is that entry's `inlier_`.

**Three of the erases are already printed — do not write them again.** With `SFS_STAGE_TRACE=1`
the existing trace already emits `[SFS-336] DROP pair_axis | pair_basket | pair_overlap`
(`class/feature_matching.cpp:1849-1871`) and `[SFS-336] RDROP reg_axis | reg_score | reg_overlap`
(`class/feature_matching.cpp:2042-2052`). The trace arms therefore run with `SFS_STAGE_TRACE=1`
and read those lines for boundaries 2 and 3 below. Duplicating them would double the log and add
nothing. If the worker finds that a boundary is **not** covered by an existing line, that is the
only place a new `drop` verb is allowed.

What 336 does **not** print, and what `SFS_ADMIT_TRACE=1` must add — the branch window, the
same-priority drop, and the plausibility reason (336's `VETO` prints the 0/1 verdict but not
the `fail_reason` string):

```
[SFS-339] FATE <step> <state> <j> <y> <x> <verb> <reason> <value>
```

| verb | where | reason | value | j |
|------|--------|--------|-------|---|
| `drop` | `RemoveSamePriority`, before the erase | `sameprio` | that entry's `inlier_` | the index being removed |
| `skip` | `BuildStep`, an offered entry whose `j >= s_` is never handed to `BuildState` | `branch` | `s_` | the index skipped |
| `try` | `BuildState` entry, each pair in the chunk, **before** the new edge is applied | `build` | the entry's `inlier_` | `order_index` |
| `veto` | `BuildState` when `CheckGraphPlausibility` returns false | the `fail_reason` string it already sets (`Score_`, `Overlap_…`, `PC_`; `class/ranking_system.cpp:2211-2292`) | `graph_score_` | same `j` as `try` |
| `keep` | `BuildState` when it is about to return true | `accept` | `graph_score_` | same `j` as `try` |

`state` on `try` / `veto` / `keep` is the parent state's `trace_id_` (the copy `BuildStep` passes in). Join a `try` to the earlier `OFFER` with the same `state` and `j`, not to the `OFFER` lines `PrepareNextStep` prints afterwards. Do not print a `FATE` for the `PickEdges` skip.

## Part B — the table (the deliverable)

For seams **7-9** and **1-9**, each trace-on repeat, one row. Seam **1-8** is the control row, not a subject. Fill from the log; do not inherit 337's W.

| seam | repeat | boundary reached | evidence (token, step, j, reason, value) | verdict |
|------|--------|------------------|------------------------------------------|---------|
| 7-9 | r1, r2 | | | |
| 1-9 | r1, r2 | | | |
| 1-8 | r1 | must be `keep` | | control |

Verdict is exactly one of: `ineligible` (no `OFFER`, no `drop`), `dropped:<reason>`, `past-branch` (`OFFER` with `j >= branch`, no `try`), `vetoed:<reason>`, `kept`. A seam may show an earlier `drop` and a later `OFFER` from a regenerated edge; report the fate of every offer, and mark the earliest drop that removes the pruned candidate's pair.

## Part C — arms that keep the run honest

3x tree only. Do not set `SFS_TOPK` or `SFS_BRANCH`.

| arm | switches | repeats | pass |
|-----|----------|---------|------|
| signed 5/3 trace on | `SFS_EDGE_TRACE=1`, `SFS_STAGE_TRACE=1`, `SFS_ADMIT_TRACE=1` | 2 | 2/9 sherds, 1/18 edges, as 338; both repeats recorded even if they match |
| signed 5/3 fidelity | all three unset | 1 | the same 2/9 and 1/18, else the instrument is void |
| Pot A | default, all three unset | 1 | 8/8 sherds and 15/15 edges, else the ticket is void |

`SFS_STATE_TRACE` stays unset (337's `STEP` lines are not needed here). The three trace
switches are independent; turning all three on in the trace arms is what makes the drop lines
readable without new code. If the trace arms' accuracy differs from the fidelity arm, the
instrument is void — 338 already showed `SFS_EDGE_TRACE` alone is inert, and this arm is what
proves `SFS_STAGE_TRACE` and `SFS_ADMIT_TRACE` are too.

## Part D — readings (apply unchanged)

1. **`ineligible`.** No `OFFER` and no `drop` for that pair. The edge never had exactly one end in a graph. The next ticket is root choice / when a piece enters a graph, not a numeric threshold.
2. **`dropped:<reason>`.** The named erase removed it before any offer (`reg_axis`, `reg_score`, `reg_overlap`, `pc_overlap`, `pc_dist`, `pc_rim`, `pc_other`, `sameprio`). The next ticket is that gate, and no other.
3. **`past-branch`.** Offered at `j >= s_` and never `try`. The next ticket is the inlier order (`CalculateMatchingScore` / `InlierCompare`), not a wider `BRANCH_b`: ticket 337 already bound the seams by widening and still misplaced them.
4. **`vetoed:<reason>`.** `try` then `veto`, no `keep`. The next ticket is `CheckGraphPlausibility` for that `fail_reason`.
5. **`kept`.** The edge entered an accepted child. 338's empty GEDGE column was the lower bound. This seam leaves this ticket and joins the 1-2 winner-selection follow-up. Do not open a gate change for it.
6. **1-9 special case.** Its best candidate is already 8.82 deg / 3.48 mm (`attribution.md`), outside the bar before admission. If its verdict is `kept` and the saved pair stays near that candidate (338 r2 was 9.12 deg / 3.62 mm), say so: admission did not drop a join that was inside the bar. Still record the boundary. Do not propose a gate change whose only effect is to admit an 8.82 deg candidate.
7. **Trace-on accuracy differs from the fidelity arm, or 1-8 has no `keep` line.** The instrument is void. Stop. Do not tune.

A split between 7-9 and 1-9, or between the two repeats, is reported as a split.

Match a seam with `y`/`x` in either order. 338's two trace-on repeats assembled different clusters at the same 2/9 and 1/18 (`attribution.md`), so fidelity is those two counts, not cluster membership. One repeat's clusters are not evidence on their own.

How a row is read, before any run:

- `OFFER` absent and `drop` absent → `ineligible`.
- `drop` with `reg_axis` and no later `OFFER` of that pair → `dropped:reg_axis` (the same shape for the other drop reasons).
- `OFFER` at `j >= branch` and no `try` at that `state` and `j` → `past-branch`.
- `try` then `veto` and no `keep` → `vetoed:<reason>`.
- `keep` → `kept`. Seam 1-8 must land here, from step 0, or the print is void.

## Where the lines go

- Declare the flag beside `g_sfs_edge_trace` (`class/feature_matching.h`, `class/feature_matching.cpp`) and read `SFS_ADMIT_TRACE` the same way `SFS_EDGE_TRACE` is read. Anything other than `1` prints a single `admit trace: off` and no `[SFS-339]` line.
- `OFFER` at the end of `State::MakeTotalHierarchyPriority`, after `RemoveSamePriority`.
- `FATE` `sameprio` in `State::RemoveSamePriority` and `skip branch` in `BuildStep`; `try`, `veto` and `keep` in `StateManager::BuildState` (starts `class/ranking_system.cpp:1272`; the plausibility call is at `:1453` and the return at `:1486`). To print `fail_reason`, capture the string `CheckGraphPlausibility` already builds; do not change its return value. The `reg_*` and pair-level drops are **not** re-implemented — they come from `SFS_STAGE_TRACE=1`.
- Files a worker may edit: `class/feature_matching.h`, `class/feature_matching.cpp`, `class/ranking_system.cpp`. Nowhere else.
- Write the filled Part B table to `C:/PR/artifacts/t339/attribution.md` and `C:/PR/artifacts/t339/attribution.tsv`.

## Part E — run, renders, write-back

- SSH `Host spartan`. Stage under `/data/gpfs/projects/punim2657/SfSpp/stage339/` the way ticket 338 staged under `stage338/` (source tarball, submit script, arm map). Tree: `/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1238_55473e9_t333-juglet-large/work/trees/px3/juglet` (`Mesh/`, `Breaklines/`, `Axes/`, `Ground Truth/`). Private copy per arm. `sbatch` only, from the canonical checkout; do not run the binary on the login node.
- Run directory: `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t339-admit/`. Record every job's final `sacct` State and ExitCode. Fetch logs to `C:/PR/artifacts/t339/runs/`.
- Renders, under `C:/PR/artifacts/t339/renders/`: seam 7-9 and seam 1-9, each as the pruned candidate places it and as the winning assembly places it, plus one view of the whole assembly. The conservator reads the pictures. In the write-back, cite each as a Markdown link relative to `C:/PR`.
- Append only under `## Comments (worker, <date>)`. The last line of that section is exactly one `**End state:** ready | waiting: <ids> | blocked: <reason> | incomplete: <what>`.

## Acceptance

- [ ] `SFS_ADMIT_TRACE=1` banner; both tokens present; a `git diff` whose only deletions are none (additions only) and that touches no threshold, `TOP_k`, or `BRANCH_b`.
- [ ] Part B filled for 7-9 and 1-9, both repeats, with the token, step, j, reason, and value that decide each row. 1-8 has a `keep` line in each trace-on arm.
- [ ] Signed 5/3 trace-on: 2/9 sherds, 1/18 edges, both repeats. Fidelity arm matches. Pot A 8/8 and 15/15, else void.
- [ ] `class/*` has no ground-truth read. The only GT lines in the log are the existing `CAND338` block.
- [ ] Renders on disk. `sacct` State and ExitCode written down. No corpus write.

## Questions this answers

S1 — whether a true join of the Juglet is offered to the state graph at all, and which single gate refuses seams 7-9 and 1-9.

## Must not

- Do not change `0.436`, `20.0`, `1.5`, `10`, `7.0`, `0.1`, `50`, `MINIMUM_NUMBER`, `INLIER_THRESHOLD`, `TOP_k`, or `BRANCH_b`.
- Do not add a pruning rule. Do not sort a container to print it. Do not mutate state while tracing.
- Do not investigate seam 1-2 past the one-line exclusion above.
- Do not commit, push, or run `git worktree`. The lead reviews and commits.
