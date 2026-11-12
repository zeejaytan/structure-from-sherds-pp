# Ticket 337 — where the search loses the join: offered, explored, or pruned?

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** 337-where-the-search-loses-the-join, in /c/PR/worktrees/structure-from-sherds-pp/337
**Answers:** S1
**Depends on:** 336 (the trace instrument and the corrected class table), 335 (pair-level winners), 334 (nine-seam pair diagnosis), 333 (3x assembly)
**Blocks:** the next fix, which cannot be chosen until we know whether the merge is never offered, never explored, or thrown away
**Worker:** unassigned
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Ticket 336 showed that in 22 of 32 seam-arm cells the two sherds are never in one graph at
> any point, so no stage sets their relative pose. This ticket asks **why** a correct merge is missing: was it
> never in the candidate list of any surviving state, was it offered but outside the three branches each state
> explores, or was it created and dropped at the five-state cut? It then measures the one reversible lever that
> follows — search width — with no threshold change.

## What is already measured (do not re-measure)

- **Ticket 336** (3x Juglet, four arms, jobs 34952628/30/31/34954018): legacy 0/9 sherds 0/18 edges; signed 2/9
  1/18, in every arm and in the trace-off repeat. Pot A control 8/8 + 15/15 on four trace-on arms. Of 36
  seam-arm cells: A = 4 (1-8: the pair candidate *is* the final pose, 1.99° / 1.59 mm), B = 0, C = 0, D = 28,
  unplaced = 4. **22 of 32 traced cells never had the two pieces in one graph.**
- **The instrument** `SFS_STAGE_TRACE=1` (ticket 336) prints `[SFS-336]` lines at every stage boundary. Leave its
  behaviour alone; this ticket adds a second, separate trace.
- **The search shape** (read from the code, main branch): `#define TOP_k 5` and `#define BRANCH_b 3` at
  `main.cpp:30-31` and `hpc/tools/main_pristine_batch.cpp:52-53`; `StateManager manager(TOP_k, BRANCH_b, ...)`
  at `hpc/tools/main_pristine_batch.cpp:290`. `StateManager::BuildStep` (`class/ranking_system.cpp:862`) takes each
  surviving state, extends `s_` = 3 branches from `MakeTotalHierarchyPriority()`, then keeps the top `N_` = 5 by
  `state_score_`. **A merge ranked below the third priority entry of its state is never explored, and the state
  that would explore it can itself be dropped at the five-state cut.**

## Part A — instrument the three refusals (additions only)

A switch **`SFS_STATE_TRACE=1`** (exact string, default off), banner at start, additions only, nothing sorted or
mutated (print a copy). One line per surviving state per step:

```
[SFS-337] STEP <s> STATE <id> SCORE <x> NPRIO <n> PART <p1,p2,...>|<p3,p4,...>
[SFS-337] PRIO <s> <id> <j> <pieceA>-<pieceB> <score> <angle_deg> <t_norm_mm> <material_mm> <GT|not>
[SFS-337] CUT <s> KEPT <id:score,...> DROPPED <id:score,...>
```

- `PART` is the piece partition of that state's graphs (one comma-separated list per graph).
- `PRIO` is the priority entry at index j, with the merge's own numbers: rotation difference (deg),
  `t.norm()` exactly as `isSimilarTrans` computes it (`class/data_structure.cpp:711`), and the **material
  displacement** of the second piece's breakline points under the relative transform — the quantity ticket 336
  established as the physical one.
- `GT|not` tags an entry whose two pieces are joined in the ground-truth graph. **Only the batch shim may read
  GT** (it already passes GT transforms for scoring); `class/*` must stay GT-free.

## Part B — attribution per seam, per arm

Class each of the nine true seams in each arm:

| class | meaning |
|---|---|
| **N** | never co-resident: no surviving state in any step has both pieces in one graph |
| **W** | offered beyond width: a surviving state holds one piece and the true pair sits in its priority list at index >= 3, so it is never explored |
| **O** | explored then pruned: a child state containing the merged pair was created and dropped at an `N` cut |
| **K** | kept: a state containing the pair survived to the end (336's 9 cells) |

For W and O, report the step and the score gap at the deciding cut; for N, report the last step at which the two
pieces were in one state at all (or "never").

## Part C — the width lever, measured at assembly (no threshold touched)

Add **`SFS_TOPK`** (default 5) and **`SFS_BRANCH`** (default 3), read in the batch shim and passed to
`StateManager`. The library's `#define`s are not edited.

Arms, both cells (legacy, signed), **two repeats each**:

| arm | TOP_k | BRANCH_b | role |
|---|---|---|---|
| control | 5 | 3 | must reproduce 336 exactly (legacy 0/9 0/18, signed 2/9 1/18) |
| wide | 10 | 5 | one step out |
| wider | 20 | 10 | does more width help at all |

Report per arm: 9-sherd and 18-edge accuracy, wall time, and the Part B class shift. If wall time grows past what
the queue will admit, the wider arm is submitted at 9 cpus as usual; do not drop repeats.

## Part D — the origin-relative 20 mm test, measured not fixed

For every merge refusal seen in Part A, tabulate (angle, `t.norm()`, material mm) and count how many have
material < 5 mm but `t.norm()` > 20 mm. **Do not change `isSimilarTrans` in this ticket**; the count decides
whether it is worth a ticket of its own.

## Pre-registered reading (fix it now, apply unchanged)

1. Seams move N/W -> K as width rises **and** the assembly score rises: search width is the binder; the next
   ticket is a width policy.
2. Seams stay N even at 20/10: width is not the binder; the pair never reaches a state's priority list at all,
   and the next question is which pairs become candidate edges (LCS generation), not the search.
3. Seams move W -> K at 10/5 but the score does not move: the pose stage (the `TransAverage` blend,
   `class/ranking_system.cpp:1253`) is the next binder.
4. The 5/3 control does **not** reproduce 336 exactly: the instrument perturbed the run. The ticket is **void**;
   report the deviation, do not tune around it.

Mixed outcomes are reported as a split with the counts, not averaged.

## Acceptance

- Part A: the new trace, with a banner; additions only (`git diff -U0 | grep '^-'` must show no removed code
  line), and a run at 5/3 whose scores equal 336's.
- Part B: the class table for nine seams x six arms (three widths x two cells).
- Part C: assembly numbers per arm per repeat, Pot A control 8/8 sherds 15/15 edges with the control arm, and
  every job id with its final `sacct` State/ExitCode.
- Part D: the tally with its threshold line.
- Run dirs under `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_<tag>/`, submitted through
  `scripts/new_run.sh`; logs kept; no corpus write (`ls Dataset/` and `original_samples/` before and after).
- Ticket append only in `## Comments (worker, <date>)`; final turn ends with exactly one
  `**End state:** ready|waiting: <ids>|blocked: <reason>|incomplete: <what>`.

## Must not

- No threshold change: 0.436, 0.175, 20.0, 1.5, 10, 0.262, MINIMUM_NUMBER, INLIER_THRESHOLD, ANGLE_THRESHOLD.
- No change to `isSimilarTrans`, and no GT read inside `class/*`.
- No assembler or writer change that affects Pot A's 8/8 + 15/15.
- No commit, push, or `git worktree` command; no edits outside the worktree; nothing deleted from the corpus.

## Questions this answers

S1 — whether the assembly can seat a true join under the authors' own gates; here, whether the search even
offers the join.
