# Ticket 338 — the join is lost between the candidate list and the group fit

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** 338-the-join-is-lost-between-the-list-and-the-fit, in /c/PR/worktrees/structure-from-sherds-pp/338
**Worker:** f53606b1-0307-4f82-94c6-1ef2c97e3399 (dsh one-shot subagent, qoder-global/qmodel_latest; dispatched 2026-11-13 by the lead — dsh-opencode-go is not an allowed provider in this session)
**Answers:** S1
**Depends on:** 337 (search width), 336 (stage trace), 335 (pair-level winners), 334 (nine-seam pair diagnosis)
**Blocks:** the next fix — whether the binder is the state score or the group pose fit; nothing can be chosen until the two are told apart
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: four gated print blocks plus one shim dump; the hidden check is the per-arm attribution table this ticket exists to produce.

> **In one line.** The reconciliation of 2026-11-13 shows the search *holds* materially good candidates
> for at least four true seams (1-2, 1-8, 1-9, 7-9) and uses none of them. This ticket follows one named
> candidate per seam through every boundary — candidate list, state graph, twin merge, group ICP,
> TransAverage blend, saved pose — and prints the group score before and after the incremental group ICP at
> `class/ranking_system.cpp:1261-1268`, so the loss is placed at one stage with a number.

## What is already measured (do not re-measure)

- **Ticket 337** (3x Juglet, twelve arms, jobs 34959025 / 34959087_0..11): width binds every seam it touches
  and still cannot place them. 20/10 legacy makes one cluster of all nine (score 306) and the rendered result
  is a heap with the neck floating clear. Signed 20/10 = 3/9 sherds, 1/18 edges, two graphs.
- **The 2026-11-13 reconciliation** (lead): ticket 335's `SURVIVOR335` columns are the candidate's own error
  against the answer key (`PlaceErr`, `stage335/diag335_pairs.cpp:91-107`); ticket 336's `nearest_*` columns
  are the *final assembly pose* against that candidate (`artifacts/t336/trace336.py:257-271`). Both are right;
  they measure different things. Recomputed with the code's own convention
  (`GTRel(x,y) = T_axis[y]·T_y⁻¹·T_x·T_axis[x]⁻¹`): candidate **1** (seam 1-2) = 2.14° / 2.20 mm,
  **77** (1-9) = 8.82° / 3.48 mm, **174** (7-9) = 5.39° / 4.24 mm — ticket 335's numbers exactly. Applied to
  the meshes they seat the break: breakline-to-breakline gap 0.19 / 0.67 / 0.42 mm against GT's
  1.04 / 1.98 / 0.43 mm (`artifacts/t338/render/cand_1.png`, `cand_77.png`, `cand_174.png`). Numbering is
  shared between the two scripts; only 174 and 175 join pieces 7 and 9, and 175 is 169° / 2 542 mm of junk.
- **The consequence:** the lead's 337 claim "no good candidate exists for 8 of 9 seams" is withdrawn. It
  rested on the `[SFS-337] PRIO mat_mm` field, printed only for priority-list entries (1 758 of 5 631);
  candidates pruned before that list never appeared in it.
- **One measured suspicion, not yet tested.** The translation test the search uses (`isSimilarTrans`,
  `class/data_structure.cpp:711`) measures **origin** movement. Candidate 174's 5.4° error is 91.6 mm of
  origin translation because the axis-aligned pieces sit about 1 m from the origin, so a 1.2° rotation
  difference already reads 20 mm against the `0.436 rad / 20.0 mm` test at `class/ranking_system.cpp:385` and
  `:452`, and the `0.175 / 20` test at `class/feature_matching.cpp:1679`. This ticket measures both
  quantities side by side; it changes neither.

## Part A — a per-arm candidate table, from the shim (GT allowed)

After `PairwisePruning` and before the `StateManager` is built
(`hpc/tools/main_pristine_batch.cpp:282-290`), dump every surviving candidate once:

    [SFS-338] CAND338 <id> <y> <x> <score> <inlier> <axis_deg> <gt_edge 0|1> <rot_vs_gt_deg> <material_vs_gt_mm>

`material_vs_gt_mm` is the `PlaceErr` RMS of piece x's own breakline points between the candidate pose and the
answer-key pose — the quantity ticket 335 printed as `rms_err_mm`, so the two tickets stay comparable. This
is what names the reference candidate per arm: **ids are not stable across arms** (the signed arm's 174 is not
the legacy arm's 174).

## Part B — the edge trace (class, GT-free)

Switch **`SFS_EDGE_TRACE=1`** (exact string, default off), banner at start, additions only, nothing sorted or
mutated. Leave ticket 336's `[SFS-336]` and ticket 337's `[SFS-337]` lines untouched.

For every surviving state, every step, every graph, list the edges it actually holds:

    [SFS-338] GEDGE <step> <state> <graph> <edge_index> <y> <x> <score>

`edge_index` is `LCSIndex::index_`, which is the `CAND338` id from Part A — that is the join between the two
parts. `RankingSubgraph::edge_` is a `list<LCSIndex>` (`class/ranking_system.h:157`), read out by `EdgeOut()`
(`class/ranking_system.h:123`). Print it for the graph a step is about to fit and once more after the step is
accepted; do not print per ICP iteration.

## Part C — the group pose fit, with the score print at `class/ranking_system.cpp:1261-1268`

This is the half ticket 337 asked for, included because without it Parts A and B cannot tell "the search never
used the join" from "the search used it and the fit averaged it away".

**C1. Around the incremental group ICP.** Before and after the `IcpIncGraphAxis` call
(`class/ranking_system.cpp:1261-1268`, definition `class/reconstruction.cpp:1791`; it writes
`state.graph_[graph_index].graph_score_` by reference):

    [SFS-338] GROUP <step> <state> <graph> <merge> <n_nodes> <score_before> <score_after> <rim 0|1> <axis 0|1>
    [SFS-338] GROUPNODE <step> <state> <graph> <piece> <incr_rot_deg> <incr_t_mm> <incr_material_mm>

`incr_material_mm` is the RMS displacement of that piece's breakline points under its own incremental
transform `R[i], t[i]` — GT-free, and printed beside the origin number so the two can be compared directly.

**C2. Inside the blend.** `TransAverage` (`class/ranking_system.cpp:1461`; the merge variant at `:1528`) is
where a node's pose is averaged over every edge joining it to the group. For each call print the node, the
number of edges, their ids, and how far the blended pose sits from each contributing edge's own pose:

    [SFS-338] BLEND <step> <state> <graph> <node> <n_edges> <edge_ids> <rot_deg_list> <material_mm_list>

Lists are space-separated in edge order; `material_mm` is the same breakline RMS measure. This line, printed
nowhere today, is what shows whether one good edge is being averaged away by two poor ones.

## Part D — runs

3x tree `runs/20261112-1238_55473e9_t333-juglet-large/work/trees/px3/juglet`, staging layout of ticket 336 as
the template:

| arm | setting | repeats | why |
|---|---|---|---|
| signed, default width | 5/3 | 2 | the arm where the good candidates exist (335) |
| signed, 5/3, trace OFF | 5/3 | 1 | fidelity: the instrument must not change the result |
| Pot A control | default | 1 | 8/8 sherds, 15/15 edges, or the ticket is void |

If the two trace-on repeats agree, one extra signed 20/10 arm may be run to ask the same question where the
search does form one cluster; it is optional and does not gate acceptance.

## Part E — the attribution table (the deliverable)

For each of the four reference seams, per arm:

| question | source |
|---|---|
| which candidate is the good one in this arm (id, rot, material) | Part A |
| does that id ever appear as a `GEDGE` in a surviving state? | yes/no, first step |
| does the winning state hold it? | yes/no |
| if held, does the group fit leave its pose within 5° and 5 mm of the candidate? | before/after |
| if it drifted, who moved it — the blend or the ICP increment? | `BLEND` vs `GROUPNODE`, by how much |
| what is the final saved pose for that pair, against the candidate? | deg / mm |

## Pre-registered readings

1. **The id never appears in any state's `GEDGE` list** → the loss is between the pairwise candidate list and
   the state graphs (candidate selection or graph construction), not the fit.
2. **It appears in states but not in the winning one** → the state score (`State::state_score_`, the sum of
   `graph_score_`) is the binder.
3. **It is in the winning state and the final pose is more than 5° / 5 mm from it** → 337's reading holds: the
   fit is the binder; the `BLEND` line then says whether averaging or the ICP increment did it.
4. **It is in the winning state, the pose still matches, yet the ACC line says the join is wrong** → the
   save/score stage is the binder, not the search.
5. **The trace-on result differs from the trace-off repeat** → the instrument is void; stop and report.

A mixed answer is reported as a split, seam by seam.

## Acceptance

- [ ] Part A table per arm, with the reference candidate named per seam.
- [ ] `GEDGE` present for every state/step/graph; totals printed (lines emitted, states traced).
- [ ] `GROUP`/`GROUPNODE` for every group fit, with the score before and after; totals printed.
- [ ] `BLEND` for every `TransAverage` call, with edge ids and the deviation lists.
- [ ] Part E table filled for all four seams, per arm, with the numbers that decide each row.
- [ ] Pot A control 8/8 sherds and 15/15 edges; signed 5/3 recorded as observed (337 had 2/9 and 1/18, both repeats).
- [ ] Fidelity: the trace-off repeat reproduces the trace-on ACC; if not, the ticket is void.
- [ ] Renders: the 7-9 pair as the winning state places it before and after the group fit, and the final
      assembly, under `artifacts/t338/renders/` — the conservator reads pictures, not only tables.
- [ ] No threshold, gate, `TOP_k` or `BRANCH_b` change: a `git diff` proof that the only edits are print
      blocks plus switch plumbing.
- [ ] `class/*` stays GT-free: the only GT reads are in the shim and in the analysis script.

## Must not

- Do not change any threshold (`0.436`, `1.5`, `0.175`, `20.0`, `MINIMUM_NUMBER`, `INLIER_THRESHOLD`,
  `ANGLE_THRESHOLD`) or the search width. This ticket measures; the fix is the next ticket.
- Do not add a pruning rule, sort a container to print it, or mutate state while tracing.
- Do not compute GT-dependent numbers inside `class/*`; pass only ids and poses out of the classes.
- Do not write to the corpus. No commit, push, or worktree command: the lead reviews and commits.
- Do not touch the shipped scorer path or any file outside `class/*` and
  `hpc/tools/main_pristine_batch.cpp`.

## Inputs

- 3x tree: `/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1238_55473e9_t333-juglet-large/work/trees/px3/juglet`
  (`Mesh/`, `Breaklines/`, `Axes/`, `Ground Truth/`).
- Ticket 336 log to compare against:
  `/data/gpfs/projects/punim2657/SfSpp/runs/20261113-0047_e444445_t336-stagetrace-signed/logs/binary_signed_traceon_r1.log`.
- Staging templates: `/data/gpfs/projects/punim2657/SfSpp/stage336/` (submit scripts, `src336.tar.gz`,
  `trace336.py`) and `/data/gpfs/projects/punim2657/SfSpp/stage335/` (`arm335.sh`, `new_run.sh`).
- Ticket 337 arms: `/data/gpfs/projects/punim2657/SfSpp/runs/20261113-0421_f0f4fc1_t337-width-sweep`
  (`work/337-arm_map.tsv`, `submit.sbatch`, array 0-11).
- Lead's reconciliation scripts, read-only references: `C:/PR/artifacts/t338/reconcile.py`,
  `cand_table.py`, `placeerr_check.py`, `render_seams.py`.

## Comments

### Lead — dispatch (2026-11-13, Thinkbook16)

Opened with `scripts/ticket_dispatch.sh --worker ...` (dry-run first): worktree
`C:/PR/worktrees/structure-from-sherds-pp/338`, branch
`338-the-join-is-lost-between-the-list-and-the-fit`, claim commit `1ad906c` pushed to `origin`.
The worker edits only that worktree plus `artifacts/t338/`; it does not commit.

Routing deviation, recorded because it changes who did the work: the DeepSeek route named in the
Hand-over line (`dsh-opencode-go`) is **not an allowed provider in this harness session** — the model
list rejects it, and no `dsh_send`/`dsh_status` tools exist here. The ticket went instead to a dsh
one-shot subagent `f53606b1-0307-4f82-94c6-1ef2c97e3399` on `qoder-global/qmodel_latest`, the only
permitted route, with the full task book (Parts A-E, arms, budget of 8 failed checks, four end states).
The lead's own acceptance check was written and smoke-run **before** the hand-over, per
`docs/agents/delegation.md`; the worker is not told it exists.

Arms the worker must return: signed 5/3 trace-on x2, signed 5/3 trace-off x1 (fidelity), Pot A default
x1 as the 8/8 + 15/15 control (else the ticket is void); signed 20/10 optional and non-gating.
Verdict stays with the lead: nothing is recorded until the per-seam numbers and the renders are
re-checked here.

