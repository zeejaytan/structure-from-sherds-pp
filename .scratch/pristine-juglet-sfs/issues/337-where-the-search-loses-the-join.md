# Ticket 337 — where the search loses the join: offered, explored, or pruned?

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** 337-where-the-search-loses-the-join, in /c/PR/worktrees/structure-from-sherds-pp/337
**Answers:** S1
**Depends on:** 336 (the trace instrument and the corrected class table), 335 (pair-level winners), 334 (nine-seam pair diagnosis), 333 (3x assembly)
**Blocks:** the next fix, which cannot be chosen until we know whether the merge is never offered, never explored, or thrown away
**Worker:** c42e9a8e-054d-4b32-9db7-f75b53d4881e (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-13 by the lead)
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

## Comments (worker, 2026-11-13)

### 1. Instrument version and the 5/3 reproduction gate (Part A acceptance)

Two instrument versions were built. **v1** printed the priority entry's rotation/translation/material from
each piece's absolute graph pose (`T_a * T_b^-1`), which mixes two graphs' frames — it is not the merge's own
numbers. That version was **withdrawn before any width arm ran**: its Pot A job 34958698 was cancelled by the
worker after 5:51 (its own log: `CANCELLED AT 2026-11-13T04:16:13 DUE to SIGNAL Terminated`; `sacct`:
`34958698 CANCELLED+ 0:0 00:05:51`). Nothing from 34958698 is a result.

**v2** (the version all reported runs use) prints the priority entry's numbers from the entry's own candidate
transform: `cand.Output(T)`, with rotation and `t.norm()` computed by the same formula as
`class/data_structure.cpp:711` (`isSimilarTrans(T, I)`), and the priority line is derived from each edge's
own `LCSIndex` (`shard_x_`/`shard_y_`), so the printed pair is the pair the search actually offers at index
`j`. Diff: 257 insertions, 1 deletion across five files; the single deletion is the mandated `StateManager`
constructor line in `hpc/tools/main_pristine_batch.cpp` (Part C's width seam). `git diff -U0 | grep -c '^-[^-]'`
= 1. Nothing in `class/*` reads GT: the GT table is filled in the batch shim
(`hpc/tools/main_pristine_batch.cpp`) from `GT_graph` and passed down as `g_sfs_tr_gt`.

**The 5/3 control reproduces 336 exactly** — the gate that decides whether this instrument perturbed the run:

| arm | run dir: logs/acc75_*.txt | wall | sherds | edges | 336 |
|---|---|---|---|---|---|
| legacy_t5b3_r1 | `runs/20261113-0421_f0f4fc1_t337-width-sweep/logs/acc75_legacy_t5b3_r1.txt` | 577 s | 0/9 | 0/18 | 0/9, 0/18 |
| signed_t5b3_r2 | `.../logs/acc75_signed_t5b3_r2.txt` | 347 s | 2/9 | 1/18 | 2/9, 1/18 |

(`acc_rc=1` is `checkacc333.py`'s by-design exit for an imperfect score, the same reason 336's jobs show
`FAILED 1:0`; the array script does not propagate it.)

**Pot A control, job 34959025, all four arms 8/8 sherds 15/15 edges** (166-181 s), `sacct` COMPLETED 0:0:

```
pota_legacy_r1 state_trace=on rc=0 acc_rc=0 acc=SHERD=8/8 EDGE=15/15
pota_legacy_r2 state_trace=on rc=0 acc_rc=0 acc=SHERD=8/8 EDGE=15/15
pota_signed_r1 state_trace=on rc=0 acc_rc=0 acc=SHERD=8/8 EDGE=15/15
pota_signed_r2 state_trace=on rc=0 acc_rc=0 acc=SHERD=8/8 EDGE=15/15
```

So the trace is additions-only and does not change a score or a decision on either object.

### 2. A measured limitation of the priority line, stated before the results

The priority line's `rot`/`t`/`mat` **carry the candidate's own frame and are not used for any claim**. Two
measurements establish this. First, on Pot A (which ends 8/8 + 15/15) the search's own candidate transform, put
on the raw breakline points, does not seat the two pieces: candidate 0 of `pota_legacy_r1`
(`[SFS-336] CAND 0 1 2 0.26821 78 ...`) moves piece 2's breaklines to 47.8 mm RMS from piece 1's, and no
composition with the eight `[SFS-336] AXIS` transforms improves it (best 36.1 mm; the four compositions give
36.1/67.4, 43.7/47.4, 305/299, 341/344 mm) although that same run assembles all 8 sherds. Second, and more
fundamentally, a nearest-neighbour distance between the two pieces' breakline point sets is **not** the material
quantity: the two sherds' breakline sets are complementary regions of one fracture surface, not the same points,
so even a perfect placement leaves a point-set distance. 336's own physical number is a
*two-transforms-same-points* displacement (`rms_between(E, G, pts[v])`), not a point-set distance.

Consequence for this ticket: the per-seam physical numbers asked for in Part A are computed **offline** from the
log's `[SFS-336] CAND` + `[SFS-336] AXIS` lines with 336's `cand_orig(c, axis, u, v) = inv(Ty) * M * Tx` and
the GT files — the same method 336 used, and legal because only `class/*` must stay GT-free. What the in-code
trace contributes to Part B is exactly what is frame-free: the priority **index j**, the entry's own **inlier
score**, the **GT tag**, the **step**, the state **partition**, and the **CUT KEPT/DROPPED** lists. Part D's
`[SFS-337] REFUSE` numbers are sound: its `rot`/`t` are `isSimilarTrans(T_c, T_i)` — the gate's own
two-candidate comparison — and its `mat` puts the **same** piece's points through both candidate matrices, a
frame-free displacement.

### 3. Jobs and run dirs so far

| job | what | sacct |
|---|---|---|
| 34958698 | Pot A, v1 (withdrawn) | CANCELLED 0:0 00:05:51 |
| 34959025 | Pot A control, v2, 4 arms | COMPLETED 0:0 |
| 34959087 | 12-arm Juglet width sweep (array 0-11) | running at 04:47Z; 2 of 12 done |

Run dirs: `/data/gpfs/projects/punim2657/SfSpp/runs/20261113-0419_f0f4fc1_t337-pota-control-v2` and
`/data/gpfs/projects/punim2657/SfSpp/runs/20261113-0421_f0f4fc1_t337-width-sweep`, both submitted through
`stage335/new_run.sh` from the canonical checkout at commit `f0f4fc1`; source travels as
`stage337/src337.tar.gz` (md5 `23b036a4e2f9b0f749b4cba59a095811`) plus `stage337/t337_worktree.diff`, so
nothing is committed or pushed. Each array task builds in its own container root
(`$RUN/arm/<arm>/{psrc,build}`) with its own private copy of the 3x Juglet tree, and each arm's tree is
md5-summed before and after.
### 4. Width 5/3 and 10/5 measured: the pairs are built and then lose the ranking

Job **34959087** (12-arm array, run dir `runs/20261113-0421_f0f4fc1_t337-width-sweep`, 24 h limit, 9 cpus per arm,
each arm with its own private copy of the 3x Juglet tree). Eight arms are done; the four 20/10 arms are still at
rung 4-5 of about 7 and are reported when they land. Preliminary aggregate job **34960682** (`sacct` FAILED 1:0 —
by design it exits non-zero while any arm has no log yet, and writes the tables for the arms that are present).

Part C, the accuracy the ticket asks for, from `logs/acc75_*.txt` (`score/337-arms.tsv`):

| arm | wall | sherds | edges |
|---|---|---|---|
| legacy_t5b3_r1 | 577 s | 0/9 | 0/18 |
| legacy_t5b3_r2 | 2609 s | 0/9 | 0/18 |
| legacy_t10b5_r1 | 1404 s | 0/9 | 0/18 |
| legacy_t10b5_r2 | 1221 s | 0/9 | 0/18 |
| signed_t5b3_r1 | 1895 s | 2/9 | 1/18 |
| signed_t5b3_r2 | 347 s | 2/9 | 1/18 |
| signed_t10b5_r1 | 1378 s | 2/9 | 1/18 |
| signed_t10b5_r2 | 1162 s | 2/9 | 1/18 |

Doubling both widths costs 1.7-2.5x the wall time and buys **zero** sherds in either cell, both repeats.
336's numbers are reproduced exactly at 5/3, so the zero is a property of the search, not of the instrument.

Part B, the class table over the eight arms (`score/337-partB-matrix.tsv`, one row per seam, one column per arm;
K = in the winning state's partition, K_end = in a terminal state that lost the ranking, O = explored then dropped
at an N cut, W = offered in a priority list at index >= branch, N = never in one graph):

```
seam  L_5/3r1 L_5/3r2 L_10/5r1 L_10/5r2 S_5/3r1 S_5/3r2 S_10/5r1 S_10/5r2
1-2   O       O       O        K_end    W       W       O        O
1-4   W       W       O        K_end    W       W       O        O
1-8   K       K       K        K        K       K       K        K
1-9   K_end   K_end   K_end    K_end    W       W       K_end    K_end
2-5   K       K       K        K        W       W       K_end    K_end
2-9   W       W       O        K_end    K       K       K        K
3-5   W       W       O        O        W       W       O        O
6-7   W       W       W        K_end    W       W       K_end    K_end
7-9   W       W       W        K_end    W       W       K_end    W
```

Three facts, each readable from that file and from the per-arm `score/<arm>.seams9.tsv`:

1. **No seam is N in any arm.** Every one of the nine true pairs is, somewhere in the search, either offered in a
   priority list or carried by a state. So candidate generation is not what loses these joins.
2. **At 5/3 the window is the first loss for most of them:** 5 of 9 seams are W in legacy and 7 of 9 in signed,
   offered at priority index 3-7 of a 3-entry window, and the inlier gap to the last explored entry is 0 to 15
   points against them, never in their favour (`score/legacy_t5b3_r1.seams9.tsv`: 1-4 at index 3, inlier 37 vs 38
   at index 2; 7-9 at index 5, inlier 14 vs 22. `score/signed_t5b3_r1.seams9.tsv`: 1-4 at index 7, inlier 15 vs 30).
   So the window's own ranking already prefers the third offer to the true pair; the pair is not the best candidate
   squeezed out of a narrow list, it is offered *below* the entries the window already explores.
3. **At 10/5 those seams become O or K_end, and the winner does not change.** Widening moves them out of W into
   states that hold them, but K stays exactly 2 seams in every arm and the accuracy does not move.

The mechanism is visible in one arm's trace. In legacy_t10b5_r2:

```
score/legacy_t10b5_r2.steps.tsv:122   5  75  290.0  13  6,9|1,3,8|2,4,5,7        <- winner, 2 true seams
score/legacy_t10b5_r2.steps.tsv:143   6  88  275.0   4  1,3,8|2,4,5,6,7,9        <- 5 true seams, lost
score/legacy_t10b5_r2.steps.tsv:144   6  89  233.0   4  1,2,3,4,6,8,9|5,7        <- 5 true seams, lost
score/legacy_t10b5_r2.cut.tsv:8       6  88:275.0,87:265.0,89:233.0   (nothing dropped)
```

State 88 is state 75 with `{6,9}` merged into `{2,4,5,7}` — a merge that joins three **true** seams (6-7, 2-9, 7-9)
— and the merge **costs 15 points** (290 -> 275). The arithmetic on the logged numbers: the winner's graph scores
(`[SFS-336] CLUSTER` lines of that run) are 26 for `{6,9}` and 160 for `{2,4,5,7}`, 186 together; state 88's
merged six-piece graph therefore carries 275 - 104 (`{1,3,8}`, untouched by the merge) = **171**, i.e. below the 186
its two parts scored separately. The merge damages the pose of the merged graph (336: the relative pose comes from the
TransAverage blend and only the joint ICP moves it), the damaged pose yields fewer inliers, and the ranking — state
score is the sum of graph scores, `class/ranking_system.cpp:624-630` — therefore prefers the least-merged state.
Widening the window feeds more true pairs into a score that penalises joining them.

This is pre-registered reading 3, with its antecedent only partly met and its mechanism measured: the seams move
**out of W** at 10/5 but they move to **losing terminal states**, not to K, and the pose stage (not the window) is
what keeps them out of the answer. Reported as a split, not an average: W->O in 10 of the 36 (seam, cell, repeat) transitions from 5/3 to 10/5,
W->K_end in 11, W->W in 3, O->O in 1, O->K_end in 1, K->K in 8, K_end->K_end in 2 — i.e. 21 of the 24 W cells leave the
window when it widens, 3 stay W, and not one of the 24 reaches K.

### 5. Part D: the 0.436 / 20 mm gate is not discarding near-identical placements

Every refusal of `isSimilarTrans(T_c, T_i, 0.436, 20.0)` in the eight arms (`score/<arm>.partd.tsv`, pooled in
`score/337-partD.tsv`): 6819 refusals, 4734 with a measurable material displacement (the other 2085 involve a piece
whose breakline points are unavailable at that moment), and

- **material < 5 mm: 0**; the smallest material displacement of any refusal is **16.03 mm**, the 5th percentile
  32.9 mm, the median 71.5 mm;
- refusals with `t.norm() > 20 mm`: 3626 of 6819; refusals with `rot < 25 deg`: 2607; refusals that fail on
  rotation alone (`rot >= 25 deg`, `t <= 20 mm`): 3193;
- so **0** refusals have material < 5 mm while `t.norm() > 20 mm`, i.e. the ticket's threshold case does not occur:
  the gate refuses proposals that differ materially, not proposals that are materially the same and only displaced at
  the graph origin.

The `[SFS-337]` `REFUSE` numbers are the gate's own (`rot`/`t` = `isSimilarTrans(T_c, T_i)`) and its `mat` puts
the **same** piece's breakline points through both candidate matrices, a frame-free displacement — this column is
sound even though the priority line's `mat` is not (see §2 above).
### 6. What the width result means, in code and in millimetres

**The eight losing pairs are offered, and the offer is materially wrong.** Applying 336's own physical rule
(rotation <= 5 deg AND breakline point movement <= 5 mm, `stage336/trace336_phys.py`) to the eight arm logs of this
run — GT read offline in the analysis, no `class/*` change — gives, per seam, the class and, where the class is D, the
best of the transforms the log actually holds (for each arm the least-rotated logged candidate, with **its own** rms;
the ranges are across the eight arms):

| seam | class (all 8 arms) | best logged candidate rot | same candidate rms |
|---|---|---|---|
| 1-2 | D | 41.3 - 50.8 deg | 21.2 - 90.2 mm |
| 1-4 | D | 14.7 - 66.2 deg | 27.0 - 110.4 mm |
| 1-8 | **A** | **1.5 - 2.3 deg** | **0.6 - 0.9 mm** |
| 1-9 | D | 33.0 - 75.3 deg | 42.8 - 71.5 mm |
| 2-5 | D | 13.6 - 110.5 deg | 9.9 - 73.6 mm |
| 2-9 | D | 7.0 - 118.9 deg | 3.2 - 87.7 mm |
| 3-5 | D | 93.2 - 149.7 deg | 67.2 - 100.4 mm |
| 6-7 | D | 17.0 - 43.2 deg | 24.6 - 37.1 mm |
| 7-9 | D | 136.4 - 162.9 deg | 86.1 - 94.7 mm |

The class is the same in all eight arms, so this is stable, not repeat noise. **1-8 is the only seam whose best logged
transform is within the 5 deg / 5 mm rule, and 1-8 is the only seam the search keeps** (K in every arm, §4). For the
other eight the best transform anywhere in the log places the second piece 3-110 mm and 7-163 deg from where it
belongs — 2-9 comes closest at 7.0 deg / 3.2 mm, still outside the rule.
The best candidate does move with the cell and the width — 2-5 is 13.5 deg / 9.9 mm in legacy and 97-111 deg in
signed, 2-9 is 66.5 deg in legacy and 7.0 deg in signed, 6-7 improves 43 -> 21 deg in legacy from 5/3 to 10/5 — but
nothing except 1-8 ever comes inside the rule. The two cells differ in their *candidate transforms*, not only in the
width, which is consistent with signed's 2/9 sherds against legacy's 0/9.
(Their files: `score/<arm>.phys.seams9.phys.tsv`, one per arm, in the 0421 run dir.)

So the eight lost joints are not lost because a correct transform was ranked below the window: there is **no correct
transform in the window** to rank. What widening the window adds is more of these wrong transforms, which is exactly
why 10/5 turns W into O and K_end and buys no sherd.

**A merge that joins true seams costs the winner 15 points, and it costs them in the parents' own edges.** In
legacy_t10b5_r2 the winning state is 75 at 290 (`score/legacy_t10b5_r2.steps.tsv:122`), and the state that carries
five of the nine true seams is 88 at 275 (`steps.tsv:143`, kept at the last cut `cut.tsv:8`). **Same nine pieces, one
extra merge, 15 points lower, and the merged state holds five true seams against the winner's two** — the search is
not short of joins, it is penalising them. The winner's own graph scores are 26 + 104 + 160 = 290
(`[SFS-336] CLUSTER` lines of that arm), so the merged six-piece graph carries 275 - 104 = **171** against the
**186** its two parts scored apart. A merge whose incoming edges were weak but whose parents kept their evidence
would leave the total at >= 186; it is at 171, so the parents' existing edges lost at least 15 inliers (more if the
incoming candidate contributed any), i.e. the merge re-poses the whole graph and erodes the very evidence that
justified it.

In the code: `state_score_` is the plain sum of `graph_score_` over graphs (`State::UpdateStateScore`,
`class/ranking_system.cpp:624-630`, called at `class/ranking_system.cpp:1324`), and `graph_score_` is filled by
`IcpIncGraphAxis` over the re-posed graph (call at `class/ranking_system.cpp:1261-1268`, out-parameter at `:1266`),
whose starting poses are the TransAverage blend. So the pose stage both produces the inlier evidence and, once a
merge is wrong, destroys it — a score that sums one number per graph cannot distinguish "joined correctly" from
"joined and dragged everything".

**Not measured, and named so it can be measured:** the exact split of the merged graph's 171 into "parents' edges
lost" versus "incoming edges contributed little". Per-graph scores are printed only for the final winner (336's
`CLUSTER` block in the batch shim), never for the intermediate states, so the split needs one new line printing
`graph_[graph_index].graph_score_` right after the `IcpIncGraphAxis` call at `class/ranking_system.cpp:1261-1268`,
in a new instrument version, plus one arm run. It was not worth a run here because the direction is already fixed by
the arithmetic above.

**Part D is a negative result and should be recorded as one:** the origin-relative 20 mm hypothesis (§5) is dead —
the gate refuses proposals that would move material by a median 71 mm, never by less than 16 mm. The 0.436 / 20 mm
test is not discarding materially-identical placements.

**Coverage caveat on the Part B table.** In legacy_t10b5_r2 the log holds 5631 `total_priority_` entries but only
1758 `PRIO` pair lines, because the printer emits a line only for an entry whose `graph_index_` and
`priority_index_` are in range and whose chunk holds an edge between two different pieces. Missing lines can only
hide an offer, never invent one, so the W columns are a **lower bound** on how often a true pair was offered — and
since no seam is N anywhere, the "these pairs do reach the search" statement is safe. K, K_end and O come from
`STEP`/`CUT` and are unaffected. K_end is a fifth name the ticket's four classes need: a state holding the pair was
in the last cut's KEPT list (or is the winner) while the winner does not hold it — "it reached the end and lost the
ranking", which is neither K nor O nor W.
### 7. 20/10: the width binds every seam, and the assembly is still wrong

All twelve arms are in (job 34959087, run dir `runs/20261113-0421_f0f4fc1_t337-width-sweep`; final aggregate job
34962555, `COMPLETED 0:0`, tables in that run's `score/`). Numbers per arm, each line also in
`logs/acc75_<arm>.txt` and collected in `score/337-arms.tsv`:

| arm | wall | sherds | edges | K | K_end | O | W | winner score | graphs |
|---|---|---|---|---|---|---|---|---|---|
| legacy_t5b3_r1 | 577 s | 0/9 | 0/18 | 2 | 1 | 1 | 5 | 290 | 3 |
| legacy_t5b3_r2 | 2609 s | 0/9 | 0/18 | 2 | 1 | 1 | 5 | 290 | 3 |
| legacy_t10b5_r1 | 1404 s | 0/9 | 0/18 | 2 | 1 | 4 | 2 | 290 | 3 |
| legacy_t10b5_r2 | 1221 s | 0/9 | 0/18 | 2 | 6 | 1 | 0 | 290 | 3 |
| **legacy_t20b10_r1** | 5311 s | **2/9** | **1/18** | **9** | 0 | 0 | 0 | 306 | **1** |
| **legacy_t20b10_r2** | 6104 s | **2/9** | **1/18** | **9** | 0 | 0 | 0 | 306 | **1** |
| signed_t5b3_r1 | 1895 s | 2/9 | 1/18 | 2 | 0 | 0 | 7 | 260 | 3 |
| signed_t5b3_r2 | 347 s | 2/9 | 1/18 | 2 | 0 | 0 | 7 | 260 | 3 |
| signed_t10b5_r1 | 1378 s | 2/9 | 1/18 | 2 | 4 | 3 | 0 | 302 | 3 |
| signed_t10b5_r2 | 1162 s | 2/9 | 1/18 | 2 | 3 | 3 | 1 | 302 | 3 |
| **signed_t20b10_r1** | 5740 s | **3/9** | 1/18 | 4 | 4 | 0 | 1 | 308 | 2 |
| **signed_t20b10_r2** | 4434 s | **3/9** | 1/18 | 4 | 4 | 0 | 1 | 308 | 2 |

**The decisive line is legacy at 20/10: all nine true seams end in one graph (K = 9, a single graph, score 306), and
the assembly scores 2 of 9 sherds.** The window was the last obstacle the ticket suspected, and clearing it does not
produce the answer — seven of the nine pairs are joined and still misplaced. Every arm's class counts are in
`score/337-arms.tsv`, every seam's decision with its step and gap in `score/<arm>.seams9.tsv`.

The full class matrix, nine seams against the twelve arms (`score/337-partB-matrix.tsv`; K kept, K_end held at the
last cut but lost the ranking, O explored then pruned, W offered beyond the window, N never in one graph):

| seam | L 5/3 r1 | L 5/3 r2 | L 10/5 r1 | L 10/5 r2 | L 20/10 r1 | L 20/10 r2 | S 5/3 r1 | S 5/3 r2 | S 10/5 r1 | S 10/5 r2 | S 20/10 r1 | S 20/10 r2 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1-2 | O | O | O | K_end | K | K | W | W | O | O | K_end | K_end |
| 1-4 | W | W | O | K_end | K | K | W | W | O | O | K_end | K_end |
| 1-8 | K | K | K | K | K | K | K | K | K | K | K | K |
| 1-9 | K_end | K_end | K_end | K_end | K | K | W | W | K_end | K_end | K | K |
| 2-5 | K | K | K | K | K | K | W | W | K_end | K_end | K_end | K_end |
| 2-9 | W | W | O | K_end | K | K | K | K | K | K | K_end | K_end |
| 3-5 | W | W | O | O | K | K | W | W | O | O | K | K |
| 6-7 | W | W | W | K_end | K | K | W | W | K_end | K_end | W | W |
| 7-9 | W | W | W | K_end | K | K | W | W | K_end | W | K | K |

No seam is N in any of the twelve arms, at any width. 1-8 is the only seam kept in every arm.

**The four pre-registered readings, answered.**

1. *Seams move N/W -> K as width rises and the score rises => the width is the binder, a width policy is next.* Half
   met and the conclusion is not: the seams do reach K at 20/10 and the score does rise (290 -> 306 legacy, 302 -> 308
   signed), but the answer barely moves (0/9 -> 2/9 legacy, 2/9 -> 3/9 signed) at 2-9x the wall time. Holding a joint
   and seating it are different events, and the width only controls the first.
2. *Seams stay N even at 20/10 => candidate generation is next.* Does not apply: no seam is ever N.
3. *W -> K at 10/5 with no score change => the pose stage is next.* The pose stage is the binder, and it is the
   **pairwise candidate pose**, not the in-graph blend: at 20/10 the pieces are bound and still misplaced, and the best
   logged candidate for eight of the nine seams is 7-163 deg / 3-110 mm from the GT placement in all twelve arms (§6).
4. *The control does not reproduce 336 => the instrument perturbed the run, the ticket is void.* It reproduces exactly:
   legacy 0/9 0/18 and signed 2/9 1/18 on every 5/3 repeat, Pot A 8/8 15/15 on all four control arms (§1, §3).

So the outcome is a **fourth reading the ticket did not name**: the width is sufficient to bind and insufficient to
place. What limits this assembly is the accuracy of each pairwise transform, and only when a pair's candidate is
right (1-8) does the search keep it and seat it. The next ticket is the pairwise pose — LCS candidate generation and
the TransAverage blend behind it — with the eight D seams as its test set; not a width policy.

**Part D, final pool over all twelve arms** (replaces the eight-arm interim in §5): 17,676 gate refusals, 5,269 with no
seatable points available, **12,407 measurable material displacements**, of which **0 are below 5 mm and 0 below
10 mm**; min 16.03 mm, 5th percentile 31.35, median 70.00, 95th 106.41, max 232.34. Rotation median 152.0 deg
(min 4.7); `t.norm()` median 21.8 mm (min 0.8, max 219.3), and 9,334 of the refusals have `t.norm() > 20 mm`
while 742 have rotation < 25 deg. Per arm: `score/337-partD.tsv`; the ticket's threshold case (material < 5 mm with
`t.norm() > 20 mm`) occurs **zero** times. The origin-relative hypothesis is dead; the 0.436 / 20 mm gate is refusing
proposals that would move material by tens of millimetres.

**The 290 against 275 comparison, in the words it deserves.** In legacy_t10b5_r2 the chosen state scores 290 and the
state that merges one more true pair into it scores 275: **same nine pieces, one extra merge, 15 points lower, and the
merged state holds five true seams against the winner's two** (`score/legacy_t10b5_r2.steps.tsv:122,143`,
`cut.tsv:8`). The mechanism, in code: `state_score_` is the plain sum of `graph_score_` over graphs
(`class/ranking_system.cpp:624-630`, called at `:1324`), and `graph_score_` is the inlier count `IcpIncGraphAxis`
returns for the re-posed graph (call at `class/ranking_system.cpp:1261-1268`, out-parameter at `:1266`), whose
starting poses are the TransAverage blend. Joining a pair re-poses the whole graph, the wrong pose lowers the inlier
count, and the sum therefore prefers the state that joins least. The winner's own graph scores (26 + 104 + 160 = 290)
put the merged six-piece graph at 171 against the 186 its two parts scored apart, so **the parents' existing edges
lost at least 15 inliers** — a merge whose incoming edges were weak but whose parents kept their evidence could not
fall below 186. Splitting that loss between "parents eroded" and "incoming contributed little" needs a per-graph
score line at `class/ranking_system.cpp:1261-1268`; it is **not measured** here.

### 8. Acceptance, as the ticket words it

- **Part A trace, banner, additions only, 5/3 scores equal to 336's.** Banner `state trace: on` and
  `state width: topk=<n> branch=<m>` in every arm log (`logs/binary_<arm>.log`); the `[SFS-337]` kinds are STEP,
  PRIO, CUT, REFUSE, plus a `[SFS-337] SFS_STATE_TRACE is not 1; state trace off` note when the value is not exactly 1.
  Additions only, proved by diff: 493 insertions, **1 deletion**, and that one line is the mandated
  `StateManager manager(state_topk, state_branch, ...)` call (`git diff -U0 | grep '^-[^-]'` → exactly one line,
  `hpc/tools/main_pristine_batch.cpp`). The 5/3 control reproduces 336 to the sherd (legacy 0/9 0/18, signed 2/9 1/18,
  Pot A 8/8 15/15 on all four arms).
- **Part B class table, 9 seams x 12 arms.** §7 above, file `score/337-partB-matrix.tsv`, plus per-seam step and gap.
- **Part C per arm per repeat.** §7 table, `score/337-arms.tsv`, `logs/acc75_<arm>.txt`, Pot A 8/8 15/15 from the
  control arms (job 34959025). Job states: 34959025 `COMPLETED 0:0 00:12:38`; 34959087_0..11 each
  `FAILED 1:0` — **cosmetic, and here is why**: the array script's last line propagates `checkacc333.py`'s exit
  status, which is 1 by design for any non-perfect score (336's jobs 34952628/34952630 were `FAILED 1:0` for the same
  reason). The search itself returned `rc=0` in all twelve arms and every accuracy was read (`rc=0 acc_rc=1` on every
  `acc75` line). A future run that wants a clean `sacct` should not end on that test.
- **Part D tally with the threshold line.** §7 final pool over 12 arms, `score/337-partD.tsv`.
- **Run dirs, logs, corpus.** `runs/20261113-0419_f0f4fc1_t337-pota-control-v2` (Pot A),
  `runs/20261113-0421_f0f4fc1_t337-width-sweep` (12 arms), `runs/20261113-0609_f0f4fc1_t337-analysis-final`
  (aggregate, `COMPLETED 0:0`), all through `scripts/new_run.sh`; the cancelled v1 run
  `runs/20261113-0409_f0f4fc1_t337-pota-control` is kept as the record of the withdrawn instrument. Corpus untouched:
  `find Dataset/Juglet_GT -maxdepth 1 -newermt 2026-11-13` → 0 files, same for
  `sfs_main/original_samples` → 0 files, and every arm's private tree is md5-identical before and after its run
  (`logs/tree_md5_before_<arm>.txt` vs `tree_md5_after_<arm>.txt`, reported "tree unchanged by the run" in
  `logs/submit_<arm>.log`).
- **Ticket append only.** This file's first 124 lines are untouched; everything above lives in this one Comments
  section.
- **Never did.** No threshold changed, `isSimilarTrans` untouched, no GT read inside `class/*` (the GT table is filled
  in the batch shim and consumed by the trace), no commit, no push, no `git worktree`, no edit outside this worktree,
  no corpus write.

## Comments (lead, 2026-11-13) — verified, with one correction to the lead's own read-out

**Verification, line by line (lead re-read the files; nothing here is taken from the worker's prose).**

- `logs/acc75_<arm>.txt`, all twelve arms: legacy 5/3 0/9 0/18 (r1, r2) · legacy 10/5 0/9 0/18 (r1, r2) · legacy 20/10 **2/9 1/18** (r1 5311 s, r2 6104 s) · signed 5/3 2/9 1/18 · signed 10/5 2/9 1/18 · signed 20/10 **3/9 1/18** (r1 5740 s, r2 4434 s). Both repeats of every setting agree exactly.
- `logs/analysis_legacy_t20b10_r1.txt` → `winning state: id=0 score=306 graphs=1`; `final clusters: {1,2,3,4,5,6,7,8,9} score 306.0`; `Part B class counts: {"K": 9}`; trace volume steps=545 prio=4326 cuts=8 refuses=3977.
- `logs/analysis_signed_t20b10_r1.txt` → score 308, graphs=2, `{2,4,6} 78.0 | {1,3,5,7,8,9} 230.0`; K=4, K_end=4, W=1 (6-7 offered at index 10 of 10, gap -2.0); refuses=1477.
- Part D, pooled line and the twelve per-arm lines: 17,676 refusals / 12,407 measurable / 5,269 no-seating / **mat<5 = 0** / **mat<5 and t>20 = 0**; arm-by-arm sum agrees.
- `sacct 34959025` = Pot A control `COMPLETED 0:0 00:12:38`; `34959087_0..11` `FAILED 1:0` for the documented `checkacc333.py` reason, with `rc=0` and a read accuracy in every `acc75` line.
- Code: `git diff --stat` over `class/*` and `hpc/tools/main_pristine_batch.cpp` = **258 insertions, 1 deletion**; `git diff -U0 | grep '^-[^-]'` prints exactly `-	StateManager manager(TOP_k, BRANCH_b, ...)`. Nothing else in the diff matches 0.436 / 1.5 / 0.175 / MINIMUM_NUMBER / INLIER_THRESHOLD / ANGLE_THRESHOLD / `isSimilarTrans(` / `t_threshold` except a comment. No GT symbol appears in the `class/*` diff.

**Correction to the lead's own 05:45 read-out to the conservator.** I said "doubling both widths returns exactly the same scores, not one sherd". That was true of the **10/5 rung only**, which was the rung whose arms had finished when I wrote it. At **20/10** the legacy cell goes 0/9 → 2/9 and, far more importantly, from **3 graphs to 1 graph holding all nine pieces**. The statement "width buys nothing" is withdrawn. The correct statement, and this ticket's answer, is: **width is sufficient to bind every true seam and still insufficient to place them.**

**The decisive observation.** The legacy 20/10 arm puts all nine sherds in one graph with all nine true seams present (K=9), scores 306, and still places **one** seam: 1-8 at 1.40 deg and 37.65 mm of origin movement. The next nearest seam in that same single graph is 6-7 at **41.4 deg and 823 mm**. So the graph holds every piece and every joint, and eight of the nine arrangements inside it are wrong. Whatever is wrong is below the search: it is the pose each accepted merge is fitted with, not the set of merges offered.

**Independent rescore of the 20/10 arms at both scoring bars** (`artifacts/t337/rescore_2010.py`, replicating `CountResult`): legacy 20/10 = 1 pair / 2 sherds at the shipped 50 mm bar and unchanged at the 3x-equivalent 150 mm bar; signed 20/10 = 2 correct pairs, 1-8 (1.51 deg, 40.42 mm) and **1-6 (13.71 deg, 32.57 mm)**, giving 3 sherds — which is what the run reports. Note for whoever reads these files next: the printed edge tally is **directional** (`k/2` over directed passes; this arm printed 1/18 at 8.33% = 3/36), so it can read one lower than the number of correct pairs while the sherd line is right. Use the sherd count as the primary figure.

**Part D, final.** The origin-relative hypothesis raised in ticket 336 is dead: over 12,407 measurable refusals across twelve arms, the smallest material displacement any refused proposal would have caused is **16.03 mm** (median 70.00, max 232.34). Not one refused proposal was a near-correct seam. The 0.436 rad / 20 mm gate is refusing genuine collisions, as it should.

**Consequence for the next ticket.** The eight failing seams fail at the pairwise pose: the best candidate the search ever holds for any of them is 9.7 mm of material misfit and most are 20-50 mm, while 1-8's good final pose is produced by the joint ICP after the merge, not taken from a candidate. The next ticket is therefore about how an accepted merge is fitted (candidate pose generation and the TransAverage blend), with the eight D seams as its test set. Width policy is not the next lever; 20/10 costs 2-9x wall for two sherds.
