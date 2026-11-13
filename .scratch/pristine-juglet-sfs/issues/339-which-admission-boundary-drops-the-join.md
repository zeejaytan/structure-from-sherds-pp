# Ticket 339 — which admission boundary drops the join

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** 339-which-admission-boundary-drops-the-join, in /c/PR/worktrees/structure-from-sherds-pp/339
**Worker:** dsh one-shot subagent, dispatched 2026-11-13 by the lead
**Answers:** S1
**Depends on:** 338 (the loss is before the fit), 337 (width is not the next lever), 336 (stage trace; leave it)
**Blocks:** the next fix — which one of the gates below to change. Seam 1-2 is a different loss (it is already inside surviving states and dies at winner selection); that ticket waits until this one names the gate for 7-9 and 1-9. The two do not share a boundary.
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

## Comments (worker, 2026-11-13)

Instrument: `SFS_ADMIT_TRACE=1` adds `[SFS-339] OFFER` (end of `State::MakeTotalHierarchyPriority`) and
`[SFS-339] FATE` (`sameprio` in `RemoveSamePriority`, `skip` in `BuildStep`, `try`/`veto`/`keep` in
`BuildState`), plus `drop pc_overlap|pc_dist|pc_rim|pc_other` for the one erase that printed nothing
before (`RemoveEdgeUsingPCInlier`). One line is one distinct piece pair of an entry; `OFFER` joins
`try`/`veto`/`keep` on `(state, j)`. Off: a single `admit trace: off` banner and no `[SFS-339]` line
(`grep -c` = 0 in both switch-off arms). 179 insertions, 0 deletions; no threshold, `TOP_k`,
`BRANCH_b`, container order or state touched.

### Runs

Run dir `/data/gpfs/projects/punim2657/SfSpp/runs/20261113-1023_c1aa93e_t339-admit`, staged from
`stage339/` (`src339.tar.gz` md5 `39b3764994716927dc0474997912458e`; extracted and diffed against
`src338.tar.gz` — the only differing members are the three edited files, so with the switch off the
binary is the 338 binary). Canonical checkout HEAD at submit: `40b3529`.

| arm | job | node | sacct State/ExitCode | wall | sherd / edge |
|-----|-----|------|----------------------|------|--------------|
| signed 5/3, traces on, r1 | 35016163_0 | spartan-bm211 | COMPLETED 0:0 | 594 s | **3/9**, 2/18 |
| signed 5/3, traces on, r2 | 35016303_1 | spartan-bm194 | COMPLETED 0:0 | 289 s | **2/9**, 1/18 |
| signed 5/3, fidelity (all unset) | 35016304_2 | spartan-bm211 | COMPLETED 0:0 | 527 s | **2/9**, 1/18 |
| Pot A control (all unset) | 35015545 | spartan-bm157 | FAILED 1:0 — see below | 187 s | **8/8**, 15/15 |

`Total number 180 / pruned 51` and `state width: topk=5 branch=3` in all three signed arms;
`SFS_TOPK`, `SFS_BRANCH`, `SFS_STATE_TRACE` never set. Pot A: `Total number 124 / pruned 70`, zero
`[SFS-336]`/`338`/`339` lines, tree md5 unchanged. Its `FAILED` is **my acceptance assertion, not the
science**: `$(grep -c X f || echo 0)` yields `0\n0` because `grep -c` prints `0` and exits 1, so the
string compare failed and the script set `FAILED=1` after a passing run. Fixed with a `cnt()` helper.

Fidelity gate, reported not tuned: r1 trace-on read 3/9 and 2/18 where r2 trace-on and the switch-off
arm read 2/9 and 1/18. r1 and r2 differ only in node and wall time (bm211 594 s vs bm194 289 s), so the
divergence is between two runs of one configuration, not trace-on vs trace-off; 338's own r1/r2 already
assembled different clusters at the same 2/9 and 1/18. Only one switch-off repeat exists, so "trace-on
agrees with fidelity" rests on one comparison.

### Part B — seam by boundary

| seam | repeat | boundary reached | evidence (token, step, j, reason, value) | verdict |
|------|--------|------------------|------------------------------------------|---------|
| 7-9 | r1 | 4 — branch window | `OFFER 0 1 12 24 3 7 9 16` then `FATE 0 1 12 7 9 skip branch 3`; 10/10 offers skip, j 4–25, steps 0–5; no drop, no veto, no GEDGE | `past-branch` |
| 7-9 | r2 | 4 — branch window | same first offer `0 1 12 24 3 7 9 16`, same `skip branch 3`; 12/12 skip, j 3–26, steps 0–4 | `past-branch` |
| 1-9 | r1 | 4 first, 5 in one attempt | 24/26 skip (`OFFER 0 0 30 46 3 1 9 10` → `skip branch 3`, j 4–44, steps 0–4); one `try` at step 5 state 29 j 0 then `veto PC_ 164` | `past-branch` |
| 1-9 | r2 | 4 — branch window | 29/30 skip from the same first offer `0 0 30 46 3 1 9 10`; no `try`, no veto | `past-branch` |
| 1-8 | r1 | none — control | `OFFER 0 0 0 46 3 1 8 70` → `try build 70` → `keep accept 70`; 4/4 keep, steps 0–1 | `kept` |
| 1-8 | r2 | none — control | identical from step 0; 5/5 keep, steps 0–2 | `kept` |

Both refused seams are refused by the **branch window**: `BuildStep` calls `BuildState` only for
`j < s_ = 3`, and every offer of 7-9 and of 1-9 sits at j = 3 or higher. The control's offer sits at
j = 0 and is kept at step 0. Best pruned candidates are identical in both repeats (pairwise pruning is
deterministic): 7-9 id 174 = 5.39 deg / 4.24 mm, score 0.340, inlier 16; 1-9 id 77 = 8.82 deg /
3.48 mm, score 0.623, inlier 10; 1-8 id 70 = 0.70 deg / 1.21 mm, score 0.447, inlier 70.

Readings (Part D): 7-9 → the next ticket is **inlier order** (`CalculateMatchingScore` /
`InlierCompare`), not a wider `BRANCH_b` — 337 already failed widening it. 1-9 → same, and its best
candidate is 8.82 deg outside the 5 deg bar, so **admission did not drop a join inside the bar**; that
special case is answered in the negative.

Two side findings, neither one of the five boundaries:

1. **An edge in a graph that no GEDGE line explains — a lead, not a finding.** In both repeats an
   edge between 1 and 9 sits in a state's graph and is erased by `RemoveEdgeUsingPCInlier` at steps
   3–5 (`drop pc_dist` 20/25, `drop pc_overlap` 6–14) with no GEDGE line for that pair and no
   `keep` for it either. The merge path (`PrepareGraphBuildinginMerge` / `ReplaceGraph`,
   `class/ranking_system.cpp:2091` / `:805`) copies a state's edges wholesale and is the obvious
   candidate, but 338 established that GEDGE is a lower bound (an edge used at a state's last step,
   whose extension then fails, is never printed), so entry through an accept is not excluded.
   Unsettled: needs a print on the merge path before it is called a bypass.
2. **A double-list artifact in the instrument, not a boundary.** `PrepareNextStep` calls
   `MakeTotalHierarchyPriority` twice (`class/ranking_system.cpp:1091`, `:1123` in `main` at
   c1aa93e; the trace additions shift them) and `BuildStep`
   walks only the second list, so a first-list index can hold another entry later. Two states per
   repeat print two `nprio` values (r1: 21, 30; r2: 20, 28) and 6 (r1) / 7 (r2) offers — one of them
   seam 1-9 in each repeat — have no FATE for their own pair. The root-mismatch early return
   (`class/ranking_system.cpp:1389`) fired zero times in both logs, so it is not the explanation.

### Acceptance

- [x] Banner and both tokens present; additions only (179/0); no threshold, `TOP_k` or `BRANCH_b` touched.
- [x] Part B filled for 7-9 and 1-9, both repeats, with token, step, j, reason, value; 1-8 has `keep` in both trace-on arms.
- [x] Signed 5/3 2/9 and 1/18: 8 of 9 runs. One trace-on run read 3/9 and 2/18 — the slowest of the nine (594 s); settled in `### Lead — verification`, nothing tuned.
- [x] Pot A 8/8 and 15/15 (run valid; the job's nonzero exit is the script bug above).
- [x] No GT read in `class/*`; the only GT lines are the existing `CAND338` block.
- [x] Renders on disk; `sacct` State/ExitCode recorded; no corpus write.

### Artefacts

- Filled table, caveats, instrument sites: [attribution.md](artifacts/t339/attribution.md), [attribution.tsv](artifacts/t339/attribution.tsv).
- Seam 7-9, r1: [candidate placement](artifacts/t339/renders/r1/seam_7_9_candidate.png), [winning assembly](artifacts/t339/renders/r1/seam_7_9_assembled.png).
- Seam 1-9, r1: [candidate placement](artifacts/t339/renders/r1/seam_1_9_candidate.png), [winning assembly](artifacts/t339/renders/r1/seam_1_9_assembled.png).
- Whole assembly, r1: [winner](artifacts/t339/renders/r1/assembly_winner.png), [answer key](artifacts/t339/renders/r1/assembly_reference.png); the same six under [renders/r2](artifacts/t339/renders/r2).
- Placement numbers in the renders are the binary's own displacement metric (it reproduces `CAND338`
  `mat` exactly: 1.21 / 4.24 / 3.48 mm); a surface-gap measure was tried first and is meaningless here
  (93.59 mm even for the answer-key placement of the control seam).

### Lead — verification (2026-11-13, Thinkbook16)

The worker returned **incomplete** on one gate: trace-on r1 read 3/9 sherds and 2/18 edges where
trace-on r2 and the switch-off arm read 2/9 and 1/18, and only one switch-off repeat existed. The
lead ran five more signed-5/3 arms to settle it, audited the code line by line, and re-read every
trace-on log on this machine. No code and no number was changed to make the gate pass.

| arm | job | node | sacct | wall | sherd / edge |
|-----|-----|------|-------|------|--------------|
| trace on r1 | 35016163_0 | bm211 | COMPLETED 0:0 | 594 s | **3/9**, 2/18 |
| trace on r2 | 35016303_1 | bm194 | COMPLETED 0:0 | 289 s | 2/9, 1/18 |
| trace on r3 | 35017631_2 | bm072 | COMPLETED 0:0 | 325 s | 2/9, 1/18 |
| trace on r6 | 35018675_4 | bm211 | COMPLETED 0:0 | 327 s | 2/9, 1/18 |
| trace on r7 | 35018676_5 | bm206 | COMPLETED 0:0 | 274 s | 2/9, 1/18 |
| trace off r1 | 35016304_2 | bm211 | COMPLETED 0:0 | 527 s | 2/9, 1/18 |
| trace off r2 | 35017631_0 | bm202 | COMPLETED 0:0 | 387 s | 2/9, 1/18 |
| trace off r3 | 35017631_1 | bm070 | COMPLETED 0:0 | 365 s | 2/9, 1/18 |
| trace off r5 | 35018314_3 | bm211 | COMPLETED 0:0 | 457 s | 2/9, 1/18 |

`trace on` means `SFS_EDGE_TRACE=1 SFS_STAGE_TRACE=1 SFS_ADMIT_TRACE=1`; `trace off` means all
three unset. `SFS_STATE_TRACE` is unset in every arm and the width is the library default 5/3.

Eight of nine runs read the same pair, switch on or off, and the same node gives the same answer with
the switch in either position (bm211: 2/9 at 327 s and at 457 s, and the one 3/9 at 594 s). The
outlier is the slowest run of the nine, not the run with the switch in a particular position.

Why the switch cannot be the cause — the audit, not the count:

- Every added statement is one of four things: inside `if (g_sfs_admit_trace)`; a global written
  only under that flag (`g_sfs339_pc_reason`); the environment read at start-up; or the single
  banner line `admit trace: off`. No branch, no score, no threshold, no container order, no state
  write is touched. The diff is 179 insertions and 0 deletions in three files.
- What does vary run to run: three Ceres solves run on twelve threads (`options.num_threads =
  NUMBER_OF_THREAD` at `class/reconstruction.cpp:1336`, `:1538`, `:1738`; `#define
  NUMBER_OF_THREAD 12` at `class/data_structure.h:18`), so the optimised poses differ in their
  last bits and downstream ties break differently. `class/axis_estimation.cpp:250` already pins
  one solver to a single thread with the comment "PRISTINE-RESTORE keep: single-threaded Ceres
  determinism (Ticket-12, was num_threads param passing 12)" — the same failure mode, known here.
  338 recorded different cluster groupings at identical counts with every switch off.
- The only random-seeded estimator in the tree, `ComputePotSACAxis` (`std::random_device` at
  `class/axis_estimation.cpp:139-140`), has no caller, so the variation is not an RNG.

The readings the ticket rests on do not vary. In all five trace-on runs:

| seam | first offer (state, j) | offers (r1/r2/r3/r6/r7) | fates | verdict |
|------|------------------------|--------------------------|-------|---------|
| 7-9 | state 1, j 12 | 10 / 12 / 9 / 9 / 9 | `skip` only, every offer | `past-branch` |
| 1-9 | state 0, j 30 | 26 / 30 / 25 / 25 / 25 | `skip` 24-29, `drop` 6, at most one `try`+`veto` (r1) | `past-branch` |
| 1-8 | state 0, j 0 | 4 / 5 / 4 / 4 / 4 | `keep` 4-5, `drop` 1 | `kept` |

Runs r3, r6 and r7 print 415 offers and 751-752 fates: the instrument is stable to the line.

Two corrections to the worker's section above, made by the lead:

1. The line numbers `:1091` / `:1123` for the two `MakeTotalHierarchyPriority` calls are correct
   in `main` at `c1aa93e` (the frame a reader of this ticket works in); the trace insertions shift
   them, so read them as `main`-frame numbers.
2. "A second, unoffered route into the graph" is **not established** and is left as a lead, not a
   finding: 338 showed a GEDGE line is a lower bound, so an edge can be in a graph through an
   accepted child and still print no GEDGE. Proving the merge path needs a print there first.

verified: artifacts/t339/check_seams.py over artifacts/t339/logs/binary_juglet_signed_on_r{1,2,3,6,7}.log -> 7-9 first offer (state 1, j 12) and every offer skipped; 1-9 first offer (state 0, j 30), 24-29 skips, 6 drops, <=1 try; 1-8 first offer (state 0, j 0) with 4-5 keeps
verified: artifacts/t339/runs/acc339_signed_t5b3_traceoff_r{1,2,3,5}.txt -> SHERD=2/9 EDGE=1/18 in all four
verified: artifacts/t339/runs/acc339_signed_t5b3_traceon_r*.txt -> 2/9 and 1/18 in r2, r3, r6, r7; 3/9 and 2/18 in r1
verified: sacct 35016163_0, 35016303_1, 35016304_2, 35017631_0..2, 35018314_3, 35018675_4, 35018676_5 -> all COMPLETED 0:0
verified: diff -q tree_md5_before_$arm.txt tree_md5_after_$arm.txt on the three original arms -> SAME, 55 files each
verified: git diff --stat on the worktree -> class/feature_matching.cpp +19, class/feature_matching.h +2, class/ranking_system.cpp +158, and 0 lines in class/reconstruction.cpp or class/data_structure.h
verified: artifacts/t339/renders/r1/seam_7_9_{candidate,assembled}.png -> 338's picture again: the candidate sits on piece 9, the winning assembly throws piece 9 far away

**End state:** ready: code, trace and renders are in the branch; the fidelity gate is closed by the lead's nine-run table above (8/9 at 2/9 and 1/18, the one exception the slowest run) and by the code audit that shows the switch adds prints only; the seam verdicts are identical in all five trace-on runs; the lead lands and closes.
