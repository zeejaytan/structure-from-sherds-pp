# Ticket 336 — which stage writes the final pose? Trace each true join from the candidate list to the output transform

**Status:** in-progress
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
