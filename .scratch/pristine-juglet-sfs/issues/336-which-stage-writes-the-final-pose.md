# Ticket 336 — which stage writes the final pose? Trace each true join from the candidate list to the output transform

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** 336-which-stage-writes-the-final-pose, in /c/PR/worktrees/structure-from-sherds-pp/336
**Answers:** S1
**Depends on:** 335, 334, 333, 331, 328
**Blocks:** the next fix (the ranking ticket is spent; the fix depends on where the pose is decided)
**Worker:** dcaadeca-2166-49f0-be57-e3736feca7cf (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-13 by the lead)
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Ticket 335 raised the pair stage's correct-winner rate from 1 of 9 to 4 of 9 and the
> assembly still wrote the wrong pose for six of nine seams — 7-9 ends at 159-165 degrees even in the cell
> where the correct 5.4 degree pose ranked first. This ticket does not change any threshold. It brackets the
> pipeline at each stage boundary and answers, per seam: which stage wrote the pose that is in the output?

## What is already measured (do not re-measure)

**Ticket 334, 3x Juglet, pair level** (`artifacts/t334`, job 34934714): 180 candidates, 79 survivors, 30 admitted
pairs. Three seams (1-2, 1-8, 1-9) had a correct pose that survived every gate and lost on score; five never
produced a correct candidate under the old ranking; 7-9's correct pose was the seam's own lowest score.

**Ticket 335, both levers, pair level and assembly** (job 34943439 and the four assembly jobs): with the
sign-aware axis test the correct pose is the best-ranked survivor for **4 of 9** seams (1-2, 1-8, 1-9, 7-9); with
coverage ranking, 2 of 9. **The assembly bar was missed in every cell** — best 2/9 sherds, 1/18 edges — and the
control cell itself moved 0/9 to 2/9 on the same binary. Pot A control was 8/8 + 15/15 in all eight arms.

**The decisive observation this ticket starts from:** in `artifacts/t335/fate_juglet_all.txt` the final relative
pose for 7-9 is **159-165 degrees in every cell**, including the cell where the pair instrument ranked the correct
5.4 degree pose first; 1-2 and 1-9 are split even in the cells where the correct pose ranks first; 2-9 is joined
at 9 degrees in the signed cells although no correct pair candidate is ever ranked (best 15.1 degrees). So the
final transform is not the pairwise winner for six of nine seams. Where it comes from is unmeasured.

## The question

For each of the nine true seams, name the stage that wrote the relative pose present in the final output:

- **A — the pair stage decided it**: the final relative pose equals a surviving pair candidate (rotation within
  5 degrees, translation within 5 mm), correct or wrong.
- **B — a gate erased the pose that would have won**: the final pose equals an enumerated candidate that was
  erased by the axis gate, the 1.5 basket cut, the overlap erase or the 10 registration cut.
- **C — pruning/merge decided it**: the final pose equals an enumerated candidate that never survived to the
  basket (twin merge, ordering, run-length floor).
- **D — a later stage invented it**: the final pose matches none of the 180 enumerated candidates. Then name
  *which* later boundary first shows it (registration, tree build, or final refinement).

Only D needs the stage instrumentation to be precise; A/B/C can be answered from the candidate list already
produced by 334's instrument. Do all four anyway — a seam that looks like A may be a coincidence of two poses
landing close, and the table has to say which.

## Part A — find the stage boundaries, in the worktree, with file:line

Read the pipeline in `main.cpp` and the class files, and list the boundaries in order with the function and line
where each one is called. Start from what is already known and correct it if it is wrong:

- `class/feature_matching.cpp:1615` `PairwisePruning` (enumerates candidates, ICPs them, applies the axis gate
  :1715, the basket cut :1726 and the overlap erase :1729)
- `class/feature_matching.cpp:1596` `MergeSimPair` (twin merge, now optionally ranked by coverage :1658)
- `NoOverrange` (overlap removal, called after `MergeSimPair` in the same function)
- `class/feature_matching.cpp:1751` `RegistrationPruning` (merges candidates while growing clusters)
- the tree/state build (`MakeCorWOBuildTree` and whatever `ranking_system.cpp` calls) and the refinement that
  produces the final transforms written as `T_<i>to_<i>.txt`

Produce `stage_map.tsv`: stage index, function, file:line, what it consumes, what it writes.

## Part B — the trace

Instrument **without changing behaviour**: a switch `SFS_STAGE_TRACE` (exact string "1", default off, read once,
banner `stage trace: on`), that after each boundary dumps, for the current placement state:

1. **clusters**: piece -> cluster id, and the current relative transform between every pair in the same cluster
   (rotation deg, translation mm, quaternion or 3x3+3 as you prefer, tab-separated);
2. **pair candidates**: for each of the nine true seams, every candidate still alive at that boundary (idx, score,
   rot vs GT, trans vs GT);
3. **the boundary's own decisions**: which candidates it erased and why (the enum the code already uses).

The dump must be cheap enough to run on 9 pieces and must not reorder any container (**no sorting of `LCS_out`;
copy, never mutate**). A trace that changes the result is void: the acceptance includes "the trace-off run and the
trace-on run produce the same `1. Acc.txt` score", which is the fidelity gate for this ticket.

## Part C — the table

For each of the nine true seams and each repeat, report the attribution class A/B/C/D, the stage that decided it,
the rotation and translation error of the final pose, and the margin to the nearest alternative. Summarise:

- how many seams fall in each class;
- **for class D, the stage at which the pose first appears** — that is the name of the next fix;
- for class B, the gate that erased it and by how much (this is the only way a gate can be blamed).

## Part D — runs

- **3x Juglet, two repeats**, on the 3x tree ticket 333/335 used (`stage335` has the private copy; reuse it, do not
  re-stage the corpus), with the shipped gate (1.5 mm) and with the sign switch on (it was accepted on the Juglet:
  0 of 18 GT pairs beyond 90 degrees — that is established, do not re-measure it). Two cells x two repeats.
- **Pot A control**: the eight-arm lane already exists (`stage335/submit_pota335.sbatch`); run at least the
  legacy and legacy+signed cells, two repeats, and report **8/8 sherds and 15/15 edges**, or the instrument is
  not trustworthy.
- Where a run's own numbers differ from ticket 335's, say so plainly; the assembler is not repeat-stable and 0/9
  versus 2/9 has already been seen on one binary.

## Pre-registered readings

1. **Most seams are class A or B** -> the pair stage is the decision point after all, and the fix is in the gate
   order or the ranking, not downstream. Say which seam contradicts 335's conclusion and how the two readings
   are reconciled.
2. **Most seams are class C** -> the decision is made before the basket; the fix is in the enumeration / twin
   merge / ordering, and the ticket must name the container operation that picks the wrong one.
3. **Most seams are class D, appearing first at registration or the tree build** -> the state/graph stage writes
   the pose. Name the function and the numbered boundary. The next ticket targets it.
4. **Class D but different stages for different seams** -> report the split; do not average it into one story.
5. **The trace changes the result** (fidelity gate fails) -> the instrument is void; fix or abandon it, do not
   report a table.

## Acceptance

- `stage_map.tsv` with function, file:line and what each boundary writes, verified in the worktree (not copied
  from this ticket).
- `SFS_STAGE_TRACE` switch, default off, read once, banner, no behaviour change; `git diff` proof that no
  constant moved (`0.436`, `1.5`, `10`, `0.175/20`, `MINIMUM_NUMBER`, `INLIER_THRESHOLD`,
  `ANGLE_THRESHOLD`) and that no container is sorted.
- The trace-off / trace-on fidelity check (same `1. Acc.txt`) and Pot A 8/8 + 15/15.
- The nine-seam attribution table for both repeats, with the class, the deciding stage, the final pose error, and
  the nearest alternative's margin; the class counts; and the class-D stage histogram.
- Every job id with its final `sacct` State/ExitCode; the run directory named by the convention
  (`scripts/new_run.sh`); the ticket updated by appending to `## Comments (worker, <date>)`.

## Must not

- **Change any constant, gate, threshold or ordering.** This is a measurement ticket. If the trace cannot be
  added without changing behaviour, say so and stop.
- Sort, reorder or mutate `LCS_out` or any container to make the trace easier; copy instead.
- Re-run the preprocessing chain, re-stage the corpus, or touch the 1x tree, the pot corpus or the canonical
  checkouts.
- Report a bar (4/9 sherds, 6/18 edges) as a target for this ticket: nothing here is expected to raise the score.
- Commit, push, or run any worktree command.

## Inputs

- 3x tree: `/data/gpfs/projects/punim2657/SfSpp/stage335` (private copy of ticket 333's `work/trees/px3/juglet`),
  Pot A lane in the same directory.
- GT: `/data/gpfs/projects/punim2657/Dataset/Juglet_large/` (`Juglet_Piece_N_T.txt`, `_Axis.xyz`, graph).
- **The pair harness that survives is `hpc/tools/diag334_pairs.cpp` on `main`** (467 lines, committed in
  df4b3d5): it links the pristine class objects, replays `PairwisePruning` / `MergeSimPair` and prints the
  candidate and survivor tables. Ticket 335's five-cell harness (`work/diag335_pairs.cpp`) was **not committed**
  and died with its worktree — do not go looking for it. Extend the 334 one; do not write a third from scratch.
- Previous runs: 34934714 (334), 34943439 (335 pairs), the four 335 assembly jobs 34943640-34943643.

## Comments (worker, 2026-11-13)

### Part A — stage map (done)

`336-stage_map.tsv` is beside this ticket (19 rows: stage, function, file:line, consumes, writes). Every
`file:line` was read back in this worktree after the trace edits, not copied from the ticket; a sample of the
load-bearing ones: `class/feature_matching.cpp:1704` PairwisePruning, `:1827` axis gate, `:1840-1842` basket
cut, `:1849` overlap erase, `:1672` MergeSimPair, `:85` NoOverrange; `class/reconstruction.cpp:1791`
IcpIncGraphAxis; `class/ranking_system.cpp:1053` its call, `:1063` T_[i].Input, `:1672/:1721` the two
PrepareGraphBuilding paths, `:1787` GraphAxisRefinement, `:1833` CheckGraphPlausibility;
`hpc/tools/main_pristine_batch.cpp:398-434` the final write.

Reading it gives the bracket this ticket asks for: the saved pose is `T_axis[i] * graph T_[j][i]`, and inside a
cluster `graph T_` is written by only two places — the chunk average at graph build (rows 12/13) and the joint
`IcpIncGraphAxis` (row 14). `GraphAxisRefinement` applies one `R_a,t_a` to the root and to every other node
(row 15), so it re-frames the cluster and cannot change a relative pose; `CheckGraphPlausibility` only returns a
verdict (row 16).

### Part B — the trace (done)

`SFS_STAGE_TRACE`, exact `"1"`, default off, read once in `InitSfsSwitches()`, banner `stage trace: on|off`.
The trace only prints: nothing is sorted, reordered or mutated; every dump reads the existing containers and
`lcs_reference_` is walked by const_iterator. `git diff --stat` for the whole ticket: **193 insertions, 0
deletions** across `class/feature_matching.cpp`, `class/feature_matching.h`, `class/ranking_system.cpp`,
`hpc/tools/main_pristine_batch.cpp` — no existing line, and so no constant, gate or ordering, is touched.

Lines (all `[SFS-336]\t...`): `AXIS` (per piece after the axis loop); `CAND`/`DROP`/`MERGE`/`SURV`
(pair enumeration, each gate's own erase decision with its value); `PHASE reg`/`RCAND`/`RDROP`
(registration); `PRIO` (the priority entry being built, with its chunk's edges); `NODE init|icp|refine|veto`
(every node's `T_` at each boundary of a build); `INCR` (the ICP increment itself); `VETO`;
`STEP`/`KEEP` (selection); `OUTSTATE`/`CLUSTER`/`FINAL` (winning state, membership, saved poses).

### Runs

Source is a snapshot of this worktree, not the patch chain: `src336.tar.gz`, md5
`68c39abf556ba4f4681534b05f60222c`, built once per job; the same tarball and `t336_worktree.diff` are copied
into each run's `work/` so the binary is reproducible from the run dir.

The ticket's Inputs line names `SfSpp/stage335` as the 3x tree; that directory is the 335 script staging area
and has no tree in it. The tree used is the one 335's own jobs copied from and measured on,
`runs/20261112-1238_55473e9_t333-juglet-large/work/trees/px3/juglet` (and `.../pota` for the control), copied
per job to a private tree and md5-summed before/after.

Submitted 2026-11-13 00:47 UTC from `e444445`:
- 34952628 `20261113-0047_e444445_t336-stagetrace-legacy` — 3 arms: trace off r1, trace on r1, trace on r2
- 34952630 `20261113-0047_e444445_t336-stagetrace-signed` — same three arms, `SFS_AXIS_SIGN=1`
- 34952631 `20261113-0047_e444445_t336-pota-control` — Pot A, legacy and signed, two repeats each, all trace on

Fidelity is read inside each Juglet job: the same binary runs trace off and trace on, and `1. Acc.txt` is
scored by `checkacc333.py` for every arm; repeats are listed separately, never averaged.

### Control and instrument check (interim, 2026-11-13 01:05 UTC)

- Pot A control, arm `pota_legacy_r1`, trace **on**: **8/8 sherds, 15/15 edges**, wall 414 s, binary
  `ee9576e326f705764bc79e2eea18208c`, shim `807d78ede267c31573b0163ec99335cb`. The trace does not move the
  control.
- Trace **off**: the binary prints `stage trace: off` and the log holds **0** `[SFS-336]` lines. Trace on: the
  same arm prints `stage trace: on` and the lines appear by kind (Pot A r1): RCAND 1933, NODE 1760, RDROP 908,
  INCR 440, MERGE 174, CAND 124, VETO 89, PRIO 89, PHASE 87, SURV 70, DROP 47, STEP 8, FINAL 8, AXIS 8, KEEP 7,
  OUTSTATE 1, CLUSTER 1.
- Cost of the trace: the Pot A arm takes 414 s with it on, against 149-167 s for the same two arms in the 335
  runs (no trace) — about 2.5x, all of it writing lines to stdout. Recorded, not tuned.
- The analysis script that reads the trace is `work/trace336.py` in every run dir, md5
  `94692071230ffd14ed1551f29f7c105f` (five-case synthetic self-test passes, including the axis-turned case that
  the earlier draft got wrong).

### Instrument, fidelity and the legacy cell (2026-11-13 01:40 UTC)

Fidelity, same binary per cell, trace off against trace on, `1. Acc.txt` read by `checkacc333.py`:

| job | cell | arm | trace | sherds | edges |
|---|---|---|---|---|---|
| 34952628 | legacy | r1 | off | 0/9 | 0/18 |
| 34952628 | legacy | r1 | on | 0/9 | 0/18 |
| 34952628 | legacy | r2 | on | 0/9 | 0/18 |
| 34952630 | signed | r1 | off | 2/9 | 1/18 |
| 34952630 | signed | r1 | on | 2/9 | 1/18 |

Equal in both cells. In the legacy cell the comparator is coarse (all three arms 0/9); the two trace-on repeats
differ from each other (seam 1-2 is 176.84 deg in r1 and 45.75 deg in r2), so the assembler's own variance is
larger than anything the trace does. Pot A control, all four arms trace on: **8/8 sherds, 15/15 edges**
(`pota_legacy_r1/r2`, `pota_signed_r1/r2`, job 34952631 COMPLETED 0:0).

Job 34952628 ends **FAILED (1:0)** although all three arms are `rc=0` and print their score: the in-job script
counts `acc_rc=1` (returned by `checkacc333.py` for any non-perfect score) as a failure. Nothing in the run
failed to run. Tree verified unchanged after the runs with `diff -r --exclude=Result` against the 333 source
tree (including `Ground Truth`), not from the in-job md5 list, which drops paths containing a space.

### Legacy cell, nine true seams, both repeats

Winning state in both repeats: state 0, score 216.0 / 290.0, **3 graphs**.
r1 clusters `{2,4} {5,7} {1,3,8}` (pieces 6 and 9 unplaced); r2 clusters `{6,9} {2,4,5,7} {1,3,8}` (all nine
placed).

| seam | r1 class | r1 deciding | r1 rot deg | r1 trans mm | r2 class | r2 deciding | r2 rot deg | r2 trans mm |
|---|---|---|---|---|---|---|---|---|
| 1-2 | D | none (pieces 1,2 in different graphs) | 176.84 | 3126.99 | D | none (different graphs) | 45.75 | 265.33 |
| 1-4 | D | none (different graphs) | 11.07 | 195.15 | D | none (different graphs) | 161.30 | 2201.77 |
| 1-8 | **A** (corrected, see the block below) | pair candidate 70 | 1.99 | 54.30 | **A** | pair candidate 70 | 1.99 | 54.31 |
| 1-9 | unplaced (piece 9) | - | - | - | D | none (different graphs) | 34.23 | 918.87 |
| 2-5 | D | none (different graphs) | 178.71 | 2802.04 | D | **icp** at step 3 | 95.40 | 683.68 |
| 2-9 | unplaced (piece 9) | - | - | - | D | none (different graphs) | 68.93 | 1806.58 |
| 3-5 | D | none (different graphs) | 136.99 | 1563.91 | D | none (different graphs) | 136.58 | 1322.28 |
| 6-7 | unplaced (piece 6) | - | - | - | D | none (different graphs) | 155.66 | 1794.26 |
| 7-9 | unplaced (piece 9) | - | - | - | D | none (different graphs) | 165.12 | 3089.50 |

| arm | A | B | C | D | D first stage | D reason |
|---|---|---|---|---|---|---|
| legacy r1 | 1 | 0 | 0 | 4 | none=4 | different graphs=4 |
| legacy r2 | 1 | 0 | 0 | 8 | icp=1, none=7 | different graphs=7, same graph=1 |

Margin to the nearest alternative (smallest rotation from the final pose to any enumerated pair candidate,
over the 180 candidates): r1 29.68, 18.81, 2.10, 75.20, 157.65 deg for 1-2, 1-4, 1-8, 2-5, 3-5; r2 43.99, 18.84,
2.10, 32.95, 13.55, 66.54, 149.61, 43.23, 162.90 deg in seam order. No seam in either repeat matches a surviving
candidate within 5 deg and 5 mm (class A), an erased candidate (class B) or a merge-dropped candidate (class C).

What the trace shows, in the order the seams fall out:

- **Same graph (r1 seam 1-8; r2 seams 1-8 and 2-5).** The pose is already present in the `init` dump — the
  graph's first pose, `PrepareGraphBuilding`, `TransAverage` of the chunk's edges
  (`class/ranking_system.cpp:1672`, merge path `:1721`) — and is carried unchanged through `icp`
  (`IcpIncGraphAxis`, `class/ranking_system.cpp:1053`) and `refine` (`GraphAxisRefinement`, `class/ranking_system.cpp:1787`,
  which moves all nodes of a cluster together and cannot change a relative pose). 1-8 is 1.99 deg / 54.30 mm in
  both repeats, first seen at `init@step0` with 111 and 131 matching phase dumps. r2 seam 2-5 first appears one
  boundary later, at `icp@step3`, at 95.40 deg / 683.68 mm.
- **Different graphs (4 seams r1, 7 seams r2).** The winning state puts the two pieces in different clusters, so
  no phase dump ever contains both: their relative pose is the product of two independently framed clusters and
  no stage wrote it. The numbers are metres apart (195-3127 mm) and 11-179 deg, which is what that composition
  gives, not a pose any stage chose.
- **Unplaced (4 seams r1).** Piece 9 (1-9, 2-9, 7-9) and piece 6 (6-7) are not in the winning state at all, so
  there is no final pose to attribute.

No class A, B or C seam in this cell: the surviving/erased/merged candidate lists never appear again as the
final pose within 5 deg and 5 mm, in either repeat. The stage that first shows each in-graph pose is the graph
build itself (init), with one seam appearing at the first joint ICP.

### Signed cell (`SFS_AXIS_SIGN=1`), both repeats

Winning state: state 0, **3 graphs**, score 260.0 (r1) / 266.0 (r2). r1 clusters `{5,7} {1,3,8} {2,4,6,9}`;
r2 clusters `{1,3,8} {6} {2,4,5,7,9}`. All nine pieces are placed in both repeats.

| seam | r1 class | r1 deciding | r1 rot deg | r1 trans mm | r2 class | r2 deciding | r2 rot deg | r2 trans mm |
|---|---|---|---|---|---|---|---|---|
| 1-2 | D | none (different graphs) | 84.40 | 914.08 | D | none (different graphs) | 106.19 | 881.08 |
| 1-4 | D | none (different graphs) | 110.82 | 1851.28 | D | none (different graphs) | 87.98 | 1641.04 |
| 1-8 | **A** (corrected, see the block below) | pair candidate 70 | 1.24 | 33.06 | **A** | pair candidate 70 | 1.24 | 33.06 |
| 1-9 | D | none (different graphs) | 89.33 | 2227.93 | D | none (different graphs) | 111.78 | 2611.18 |
| 2-5 | D | **init** at step 4 | 171.13 | 1099.69 | D | **icp** at step 4 | 167.76 | 1002.44 |
| 2-9 | D | **init** at step 4 | 9.44 | 264.24 | D | **init** at step 4 | 10.41 | 290.94 |
| 3-5 | D | none (different graphs) | 51.12 | 1027.98 | D | none (different graphs) | 40.22 | 934.89 |
| 6-7 | D | none (different graphs) | 154.63 | 2260.84 | D | none (different graphs) | 137.31 | 2069.48 |
| 7-9 | D | none (different graphs) | 163.18 | 3111.18 | D | **icp** at step 5 | 159.80 | 3071.59 |

| arm | A | B | C | D | D first stage | D reason |
|---|---|---|---|---|---|---|
| signed r1 | 1 | 0 | 0 | 8 | init=2, none=6 | different graphs=7, same graph=1 |
| signed r2 | 1 | 0 | 0 | 8 | init=1, icp=2, none=5 | different graphs=5, same graph=3 |

Margin to the nearest alternative (smallest rotation from the final pose to any enumerated pair candidate):
r1 44.60, 27.60, 1.31, 74.36, 97.46, 7.20, 93.23, 41.24, 159.50 deg; r2 34.50, 45.86, 1.31, 51.16, 100.61, 6.12,
75.30, 58.03, 156.10 deg in seam order 1-2, 1-4, 1-8, 1-9, 2-5, 2-9, 3-5, 6-7, 7-9.

One cross-graph seam in this cell shows a pose that was set when the pair was briefly together: signed r1 2-5
is 171.13 deg, matches 2 `init@step4` dumps, yet the winning state has pieces 2 and 5 in different graphs — the
split clusters inherited the earlier in-graph pose. Every other cross-graph seam has no phase dump containing
both pieces.

### All four trace-on arms

| seam | legacy r1 | legacy r2 | signed r1 | signed r2 |
|---|---|---|---|---|
| 1-2 | D none (diff. graphs) | D none (diff.) | D none (diff.) | D none (diff.) |
| 1-4 | D none (diff.) | D none (diff.) | D none (diff.) | D none (diff.) |
| 1-8 | **A** (pair cand 70) | **A** | **A** | **A** |
| 1-9 | unplaced (piece 9) | D none (diff.) | D none (diff.) | D none (diff.) |
| 2-5 | D none (diff.) | D icp@3 (same) | D init@4 (diff., set before split) | D icp@4 (same) |
| 2-9 | unplaced (piece 9) | D none (diff.) | D init@4 (same) | D init@4 (same) |
| 3-5 | D none (diff.) | D none (diff.) | D none (diff.) | D none (diff.) |
| 6-7 | unplaced (piece 6) | D none (diff.) | D none (diff.) | D none (diff.) |
| 7-9 | unplaced (piece 9) | D none (diff.) | D none (diff.) | D icp@5 (same) |

Class totals over the 36 seam-arm cells (corrected to the physical measure, see the correction block): **A = 4**
(seam 1-8 in every arm), **B = 0, C = 0**, D = 28, unplaced = 4. Class-D first stage: `init` 3, `icp` 3, none
(never jointly posed) 22; reason `same graph` 9, `different graphs` 23 (the counts do not add to 36 because signed r1
2-5 is both `init` and `different graphs`).

### What the pair stage held, per seam

The 180 enumerated candidates and their gate fates are the same in all four arms (same seed `FeatureComp`;
legacy: 96 pair-gate drops — 77 axis, 18 overlap, 1 basket — 79 survivors after twin merging; signed: 124 drops
— 116 / 6 / 2 — 51 survivors). Search and merge counts then differ run to run, so the four final columns
differ. Best candidate = smallest rotation to GT among all 180; final = the saved pose vs GT in each arm.

| seam | best cand | its fate | cand vs GT rot / trans | final rot deg vs GT (Lr1 Lr2 Sr1 Sr2) | final trans mm vs GT (Lr1 Lr2 Sr1 Sr2) |
|---|---|---|---|---|---|
| 1-2 | 3 | merged away (kept 1) | 1.93 / 52.4 | 176.84 45.75 84.40 106.19 | 3127.0 265.3 914.1 881.1 |
| 1-4 | 37 | merged away (kept 38) | 15.78 / 437.5 | 11.07 161.30 110.82 87.98 | 195.2 2201.8 1851.3 1641.0 |
| 1-8 | 67 | merged away (kept 70: 0.70 / 7.9) | 0.49 / 4.3 | 1.99 1.99 1.24 1.24 | 54.3 54.3 33.1 33.1 |
| 1-9 | 77 | survived | 8.82 / 183.9 | - 34.23 89.33 111.78 | - 918.9 2227.9 2611.2 |
| 2-5 | 94 | survived | 45.91 / 286.6 | 178.71 95.40 171.13 167.76 | 2802.0 683.7 1099.7 1002.4 |
| 2-9 | 108 | survived | 15.12 / 424.0 | - 68.93 9.44 10.41 | - 1806.6 264.2 290.9 |
| 3-5 | 118 | survived | 48.31 / 913.7 | 136.99 136.58 51.12 40.22 | 1563.9 1322.3 1028.0 934.9 |
| 6-7 | 166 | erased `pair_overlap` (76.898) | 165.69 / 2205.7 | - 155.66 154.63 137.31 | - 1794.3 2260.8 2069.5 |
| 7-9 | 174 | survived | 5.39 / 134.2 | - 165.12 163.18 159.80 | - 3089.5 3111.2 3071.6 |

Corrected reading of 1-8 (the earlier version of this paragraph applied the 5 mm bar to a translation norm, which is
frame-dependent - see the correction block): candidate 67 is 0.49 deg from GT and its twin 70 is 0.70 deg; the final
pose is 1.24-1.99 deg from GT and, measured as point movement, 1.3-1.6 mm away, and it reproduces surviving candidate
70 to 1.5-2.3 deg and 0.6-0.9 mm. So the graph carries this seam's orientation **and** its placement, and the seam is
class A.

### Where each phase is written

| first stage seen | function | file:line |
|---|---|---|
| registration (before a graph holds both) | `RegistrationPruning` | `class/feature_matching.cpp:1898` |
| init | `PrepareGraphBuilding` -> `TransAverage` (merge path `PrepareGraphBuildinginMerge`) | `class/ranking_system.cpp:1672` / `:1721` |
| icp | `IcpIncGraphAxis`, called from `BuildState` | `class/ranking_system.cpp:1053` |
| refine | `GraphAxisRefinement` (one global move per cluster; cannot change a relative pose) | `class/ranking_system.cpp:1787` |
| veto | `CheckGraphPlausibility` (accept/reject only) | `class/ranking_system.cpp:1833` |

### Instrument, fidelity, control, jobs

- Fidelity gate (same binary per cell, trace off vs on, score on `1. Acc.txt`): legacy 0/9 sherds 0/18 edges off
  and on both repeats; signed 2/9 sherds 1/18 edges off and on both repeats. The scores match. The saved
  solutions do **not** match between any two arms of a cell (legacy placed 9 / 7 / 9 pieces across off, on r1,
  on r2; signed placed 9 in all three with different `T_*` md5s), so the score is a coarse comparator: the
  instrument's inertness rests on the diff shape plus this repeat spread. The code diff is additions only
  (`git diff --numstat`: class/feature_matching.cpp 89/0, class/feature_matching.h 2/0, class/ranking_system.cpp
  72/0, hpc/tools/main_pristine_batch.cpp 30/0 = 193 insertions, 0 deletions), so no constant, gate, ordering
  or container call could have been edited — an edit would show as a `-`/`+` pair. No added line calls a sort.
  What was added is declarations, one banner print, a matrix-print helper and print blocks guarded by
  `if (g_sfs_stage_trace)`. The trace-off repeat (`legacy_traceoff_r2`, `signed_traceoff_r2`, job 34954018) put the
  same spread on the off side: 0/9 0/18 and 2/9 1/18, the same scores as the trace-on arms of each cell.
- Pot A control, all four arms trace on: **8/8 sherds, 15/15 edges** (pota_legacy_r1/r2, pota_signed_r1/r2).
- Jobs: 34952628 legacy (sacct FAILED, ExitCode 1:0), 34952630 signed (FAILED, 1:0), 34952631 Pot A
  (COMPLETED, 0:0), 34954018 trace-off repeat (COMPLETED, 0:0, 00:19:55). The two FAILED states are the run script's own exit
  code: every arm ran rc=0 and printed its score; `checkacc333.py` returns 1 for any non-perfect score and the
  script counted that as a failure.
- Run dirs (named by `scripts/new_run.sh`): `runs/20261113-0047_e444445_t336-stagetrace-legacy`,
  `...-signed`, `...-pota-control`, `runs/20261113-0153_e444445_t336-fidelity-off2`.
- Wall: Pot A 414 / 715 / 263 / 360 s; legacy off 1265, on r1 593, on r2 881; signed off 1773, on r1 960, on
  r2 724. Trace cost is not separable from search variance (legacy on r1 was faster than off).

### What the trace shows

- One seam in all four arms ends at a surviving pair candidate within 5 deg and 5 mm of point movement: 1-8,
  candidate 70. Every other seam reproduces no pair candidate. Where a graph holds both pieces the pose is the
  `TransAverage` blend of that chunk's edges (first seen at `init`), moved afterwards only by the joint `icp`.
- In the 9 cells where both pieces sit in one graph the blend is close to GT for 1-8 (1.24-1.99 deg, 1.3-1.6 mm of
  point movement) and roughly near for signed 2-9 (9.4-10.4 deg, 4.3-4.9 mm), and far from it for 2-5
  (95.4-167.8 deg, 66-114 mm) and 7-9 (159.8 deg, 94-98 mm).
- In the 23 cells where the winning state separates the pair, 22 have no phase dump containing both: their
  relative pose is the composition of two independently framed clusters (195-3127 mm apart), which no stage
  chose. The 23rd (signed r1 2-5) inherited a pose set at step 4 before the split.
- The pair stage held a candidate within 16 deg of GT for 1-2, 1-8, 1-9, 2-9 and 7-9; the final reproduces one of
  them, 1-8, to 1.5-2.3 deg and 0.6-0.9 mm of point movement (class A). The rest are far from GT in rotation, and
  their translation columns in the tables above are not physical distances (see the correction block).

### Correction block after lead review (worker, 2026-11-13)

**1. The "trans mm" column is a transform translation, not a physical gap.** A residual rigid transform whose
rotation origin lies far from the material moves the material very little even when its translation is large.
Measured at seam 1-8: `T_axis[1]` translation is (-587.91, -564.67, 1284.24) mm and the GT-placed piece-8 points
sit 1512-1560 mm from piece 1's local origin. The residual `E(1,8) * GTrel(1,8)^-1` has ||t|| = 53.50 mm yet moves
those points by median 1.61 mm, max 1.92 mm. The physical measure used below is that point movement, RMS over the
moving piece's breakline points (`Breaklines/Juglet_Piece_<i>_Breakline_0.pcd`) - the same quantity ticket 334
prints as `rms_err_mm`. Class rule from here: rotation <= 5 deg AND point movement <= 5 mm, precedence
A (surviving) > B (erased) > C (merge-dropped) > D.

**2. Corrected classes (36 cells): A = 4, B = 0, C = 0, D = 28, unplaced = 4.** A is seam 1-8 in every arm; D first
stage `init` 3, `icp` 3, none 22.

| arm | A | B | C | D | D first stage |
|---|---|---|---|---|---|
| legacy r1 | 1 | 0 | 0 | 4 | none=4 |
| legacy r2 | 1 | 0 | 0 | 8 | icp=1, none=7 |
| signed r1 | 1 | 0 | 0 | 8 | init=2, none=6 |
| signed r2 | 1 | 0 | 0 | 8 | init=1, icp=2, none=5 |

Seam 1-8, matched against surviving candidate 70 (rot deg / point movement mm): legacy r1 2.32 / 0.93, legacy r2
2.32 / 0.92, signed r1 1.52 / 0.61, signed r2 1.52 / 0.62. Its dropped twin 67 is nearer still (2.10 / 0.77 legacy,
1.31 / 0.53 signed) and was erased at registration (`reg_axis` in three arms, `reg_score` in legacy r2), but the same
pose reached the final through the survivor, so that erase cost nothing at this seam. So 1-8 is not "orientation
without placement": the graph build writes this seam's pose at `init` and the pair stage's surviving candidate is
the same pose to 0.6-0.9 mm. The lead's proposed wording ("placement agrees with nothing, 30-50 mm") rests on the
translation norm and is not what the geometry does - the 33-56 mm figures are the lever-arm translation.

Boundary cell: signed 2-9 is 9.44 deg / 4.27 mm (r1) and 10.41 / 4.91 (r2) - inside the 5 mm physical bar, outside
the 5 deg bar, so it stays D on rotation alone, while its placement is already at the reference's own scale (GT
places these two pieces' breaklines a median 20.8 mm apart).

**3. Final pose vs GT, physical point movement (rotation deg / RMS mm), replacing the "trans mm" columns:**

| seam | legacy r1 | legacy r2 | signed r1 | signed r2 |
|---|---|---|---|---|
| 1-2 | 176.84 / 96.61 | 45.75 / 64.01 | 84.40 / 93.99 | 106.19 / 80.38 |
| 1-4 | 11.07 / 43.26 | 161.30 / 139.26 | 110.82 / 94.08 | 87.98 / 64.32 |
| 1-8 | 1.99 / 1.59 | 1.99 / 1.59 | 1.24 / 1.33 | 1.24 / 1.33 |
| 1-9 | unplaced | 34.23 / 58.20 | 89.33 / 102.18 | 111.78 / 83.98 |
| 2-5 | 178.71 / 114.30 | 95.40 / 65.84 | 171.13 / 86.60 | 167.76 / 85.30 |
| 2-9 | unplaced | 68.93 / 57.28 | 9.44 / 4.27 | 10.41 / 4.91 |
| 3-5 | 136.99 / 96.36 | 136.58 / 97.14 | 51.12 / 68.28 | 40.22 / 29.62 |
| 6-7 | unplaced | 155.66 / 78.41 | 154.63 / 79.22 | 137.31 / 84.41 |
| 7-9 | unplaced | 165.12 / 97.73 | 163.18 / 94.46 | 159.80 / 93.88 |

**4. Counts arithmetic (lead point 2).** The 9 same-graph cells are `init` 6 + `icp` 3. The seventh `init` is signed
r1 2-5: pieces 2 and 5 are in different graphs in the winning state, but a step-4 state held them together and the
split clusters inherited that pose, so it is a cross-graph cell with a live phase dump. It was counted once in the
D-first-stage `init` tally and once in the `different graphs` reason tally - that double count is the extra one.
`init` 7 + `icp` 3 = 10 = 9 same-graph + 1 cross-inherited.

**5. Why 4.3 / 7.9 mm disagree with 334's 1.25 / 1.21 mm (lead point 3).** Same candidates, same post-ICP matrices,
four frames:

| quantity | cand 67 | cand 70 |
|---|---|---|
| 334 `t_err_norm` (aligned frame, `T_axis[y] * (C*G^-1) * T_axis[y]^-1`) | 1.250 | 0.993 |
| this ticket's pair table (`C^-1 * G`, original frames) | 4.309 | 7.914 |
| `C * G^-1`, original frames | 10.148 | 15.450 |
| physical point movement, RMS over piece 8 | 1.278 | 1.208 |
| 334 `rms_err_mm` | 1.280 | 1.210 |

Rotations agree to 3e-4 deg (0.486 / 0.700 here vs 0.485738 / 0.701366 in 334), so these are the same candidates in
the same ICP state. The four translations are one residual expressed in different frames; with the rotation origin
~1.5 m from the material their norms differ by an order of magnitude while the point movement agrees to 0.002 mm.
Explained, not unexplained.

**6. Fidelity repeat, job 34954018** (submitted 2026-11-13 01:53 local from e444445; sacct COMPLETED 0:0,
elapsed 00:19:55, ended 2026-11-13T02:13:35; run dir `runs/20261113-0153_e444445_t336-fidelity-off2`). Both arms run
trace off, read from `out/<arm>/1. Acc.txt`: `legacy_traceoff_r2` 0/9 sherds 0/18 edges (wall 765 s),
`signed_traceoff_r2` 2/9 sherds 1/18 edges. Both equal the trace-on scores of the same cell, so the score is
unaffected by the trace; the placements are not repeat-stable (legacy 9 / 7 / 9 pieces across off, on r1, on r2).

**7. Reproducing 2 and 3.** Script `.scratch/pristine-juglet-sfs/issues/trace336_phys.py` (md5
`e37f770dc46efbfc55bc409da7ecb96e`, copy on Spartan at `SfSpp/stage336/trace336_phys.py`) parses the same four
trace logs with `trace336.py` and prints the class per seam; it was run as
`trace336_phys.py --log <run>/logs/binary_<cell>_traceon_r<k>.log --tree <run>/work/trees/juglet`. Its TSVs are in
`SfSpp/stage336/phys/`.

## Lead verification (2026-11-13)

**Verified by the lead before recording** (each line re-read from the artefact it rests on):

- **Scores**, `out/*/1. Acc.txt`: legacy 0/9 sherds 0/18 edges in traceoff_r1, traceon_r1 and traceon_r2; signed
  2/9 1/18 in the same three; fidelity-off2 `legacy_traceoff_r2` 0/9 0/18 and `signed_traceoff_r2` 2/9 1/18.
  Pot A control 8/8 sherds 15/15 edges on all four trace-on arms.
- **Jobs**: 34952628 FAILED 1:0, 34952630 FAILED 1:0, 34952631 COMPLETED 0:0, 34954018 COMPLETED 0:0 00:19:55.
  The two FAILED are the run script's own exit (`checkacc333.py` returns 1 for any non-perfect score); every
  arm's binary ran rc=0. Recorded here so a later reader does not take them for crashed runs.
- **The class table**: I re-ran `trace336_phys.py` on the cluster myself for legacy_traceon_r1 and
  signed_traceon_r1 and got the worker's table (A=1, D=4, unplaced=4; A=1, D=8).
  `verified: trace336_phys.py --log <run>/logs/binary_<cell>.log --tree <run>/work/trees/juglet -> A=4, B=0,
  C=0, D=28, unplaced=4 over 36 cells`.
- **The code**: `git diff --stat` = 193 insertions across the four code files, 0 deletions; the only removed
  line in the whole diff is `**Status:** in-progress`. Nothing added sorts or mutates the traced containers.
- **Script copies**: `trace336_phys.py` on Spartan (md5 e37f770dc46efbfc55bc409da7ecb96e) differs from the
  worktree copy (md5 8129a0f61d6fdaef4aeed66748684ae9) by the final newline only (`diff` -> one line, `main()`,
  no textual change); the Spartan copy is the one that produced the table.

### The ticket's own class-A criterion was wrong, and the error is mine

Part C defined A as "final pose equals a surviving candidate within 5 deg / 5 mm", and I read the 5 mm off the
**translation column of the composed transform**. My first pass therefore read 1-8 as "placement agrees with
nothing, 30-56 mm away". That column is the displacement of the world origin, and the origin sits about 1.5 m
from the material (`T_axis[1]` t = (-587.91, -564.67, 1284.24) mm; the GT-placed piece-8 points lie 1512-1560 mm
from piece 1's origin), so a 2 deg residual shows up there as ~53 mm while the material moves ~1.6 mm.

Measured as material movement (`C:/PR/artifacts/t336/lead/check18.py`, from `T_0to_0`/`T_7to_7` of
legacy_traceon_r1 plus the 3x tree's GT `T` and piece-8 breakline): seam 1-8 final vs GT = **1.99 deg,
breakline RMS 1.59 mm (max 1.92), origin translation gap 54.3 mm**; surviving candidate 70 is 2.32 deg / 0.93 mm
from it (legacy) and 1.52 / 0.61 (signed). So 1-8 is class A in every arm: the pair stage's own winner **is** the
final pose.

Corrected table: **A = 4 (1-8 in every arm), B = 0, C = 0, D = 28, unplaced = 4**; D's first stage: init 3, joint
ICP 3, none 22.

### What the trace answers

1. No gate erases the winner (B = 0), and no candidate is lost at the twin merge and then re-chosen (C = 0).
   The pre-registered readings "most B -> gate order" and "most C -> enumeration/twin merge" are both empty.
2. The binding event is the **state/graph search, not the pair score**: in 22 of the 32 traced cells the two
   pieces are never in one graph in any dumped state, so no stage ever sets their relative pose - the output is
   the composition of two cluster frames 195-3127 mm apart.
3. Where they do share a graph (9 cells) the pose is a **TransAverage blend**:
   `class/ranking_system.cpp:1253` `TransAverage(int current_node, vector<LCSIndex>& edges, Matrix3d& R,
   Vector3d& t)` (overload :1320) averages a node's incident edges, called from :617-618, :1707 and :1768. An
   average of several poses equals none of them, which is why A is rare inside a cluster rather than common.
   `IcpIncGraphAxis` (:1053) and `GraphAxisRefinement` (:1787, rigid whole-cluster motion) cannot change a
   relative pose, consistent with the 3 cells whose pose first appears at the joint ICP.
4. The pre-registered reading "most D at registration or tree build -> that stage is the next fix" does not
   apply: D is mostly the upstream condition that the two pieces never shared a graph, which is decided by merge
   acceptance and the recursive state search.

### Recorded, not tested: the 20 mm thresholds are origin-relative too

The worker raised this; it is arithmetic, not opinion. `class/data_structure.cpp:711`
`isSimilarTrans(T_a, T_b, rad_threshold, t_threshold)` builds `T = T_a * T_b.inverse()` and tests
`t.norm() < t_threshold`; the call sites that decide merges are `class/feature_matching.cpp:1679`
`isSimilar(comp->trans_, 0.175, 20)` and `class/ranking_system.cpp:256` / `:315`
`isSimilarTrans(T_c, T_i, 0.436, 20.0)` (also `class/data_structure.cpp:1009` in scoring). `t.norm()` is the
movement of the world origin, so 20 mm there is not 20 mm at the seam; the 1-8 numbers above are an existence
proof that the two quantities differ by ~30x on this geometry. This ticket did not test it.

**Claim type:** the method fails at the state/graph stage, below the pair score. It is not a broken measurement
of the pose - where a candidate survives, the pose is written correctly.



