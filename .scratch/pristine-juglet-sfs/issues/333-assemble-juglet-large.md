# Ticket 333 — assemble juglet_large at 3x: do the recovered dots become joins?

**Status:** ready-for-agent
**Working in:** (claimed by the worker)
**Branch:** 333-assemble-juglet-large
**Answers:** S1
**Depends on:** 331 (the 3x input is built and measured), 332 (the dense 1x control on the cured tree)
**Blocks:** none
**Worker:** unassigned
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Ticket 331 enlarged the Juglet 3x: every piece now carries 77-185 dots and 7 of 9 seams
> clear the matcher's 7-dot / 12.6 mm window on the same binary. This ticket hands that input to the
> assembler, unmodified, twice, plus one diagnostic arm with the agreement gate at its scale-equivalent
> 4.5 mm - and asks whether the recovered dots become joins.

## Why this is the right next test (facts, all measured)

- **The 1x baseline does not move with density.** Ticket 332 ran twelve arms on the cured 1x tree: sparse
  enumerates 6 of 9 pairs, dense enumerates 9 of 9, and the number of pairs that pass is **1 of 9 in every
  arm** (0 of 9 once), with 5-8 of the pairs fitting upside down (>90 deg). 1x score: **2/9 sherds, 1/18 edges**
  (`work/gt/trees/base18`, run `20261112-0245_30b80ae_t332-dense-window12`).
- **The 3x input clears the window.** Ticket 331 (run `20261112-0218_b1e1580_t331-large`) gives
  `185/96/125/102/150/141/174/126/77` Breakline_0 dots at 1.800 mm spacing, and **7 of 9** touching pairs
  reach >=7 dots and >=12.6 mm at 1.5 mm (8 of 9 at the scale-equivalent 4.5 mm; fail 1-9, 2-5 at 1.5 mm;
  only 2-5 fails at 4.5 mm). Non-touching pairs: 0 dots. The lead reproduced three pairs independently.
- **The assembler's gates are absolute millimetres**, so at 3x three of them are three times stricter
  relative to the vessel: agreement `INLIER_THRESHOLD 1.5` mm (`class/reconstruction.h`), overlap
  `OverlapCheck_3d(50, ...)` mm^2, merge `20` mm; the angular gate `0.262` rad is scale-free. That is
  exactly why this ticket needs the second arm.

## Part A — build the 3x tree (do not re-run the chain)

Use ticket 331's own outputs; do not re-run sampling/meshing/edgeline. Sources:

| tree slot | from |
|---|---|
| `Breaklines/Juglet_Piece_N_Breakline_0.pcd` | `.../20261112-0218_b1e1580_t331-large/outlarge_jug/geom/Juglet_Piece_N.Bl0` (only Bl0 - the assembler never reads Bl1) |
| `Surfaces/Juglet_Piece_N_Surface_{0,1}.xyz` | `.../outlarge_jug/geom/Juglet_Piece_N.S{0,1}` |
| `Axes/Juglet_Piece_N_Axis.xyz` | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Juglet_Piece_N_Axis.xyz` (GT, already 3x) |
| `Ground Truth/Juglet_Piece_N_T.txt` | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Juglet_Piece_N_T.txt` (already 3x) |
| graph | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Juglet_simple_graph.txt` |
| `Mesh/Juglet_Piece_N_Mesh.obj` | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Piece0N.obj` |

Pin md5s of every file you copy into `pins/tree333.md5`, and record the graph md5
(`ccae01dd9c897e92beb0d507baa45cea`, identical to the 1x copy). Follow the tree layout ticket 332's
`work/gt/trees/base18/juglet` uses - read it first and mirror it.

**Use the GT axes for both arms.** The question is whether scale turns dots into joins, not whether the
estimator is good; ticket 331's estimator axes exist (`axes_large/`) and may be recorded as a one-line note,
not used. Do not add a third axis arm.

## Part B — control, same binary, before anything is measured

Pot A through the very same binary and shim, with the new switch unset, must reproduce **8/8 sherds and
15/15 edges, twice**. If it does not, the run is **void** - say so and stop; do not adjust anything.

## Part C — the arms

- **Arm 1 (main): shipped gates, unmodified binary.** Two repeats on the 3x tree.
- **Arm 2 (diagnostic): `SFS_INLIER_MM=4.5`.** Two repeats on the same tree. The switch must be one code
  path: read once at start-up, default `INLIER_THRESHOLD` (1.5) when unset/empty/unparseable/outside a sane
  range, print once as a banner (`inlier gate: <x> mm`), and be used everywhere `INLIER_THRESHOLD` is
  consumed today. Nothing else changes - no overlap, merge or angle constant moves.

Everything else identical between arms: same tree, same binary, same shim, same count, same seeds/floors.
Arm 1 must show the banner value 1.5; arm 2 must show 4.5.

## Part D — measure and report

Score with CountResult (0.35 rad / 50 mm), as in 332. Per arm and repeat, report:

- sherds placed / 9, edges recovered / 18, which pairs, and which admitted joins are **not** GT;
- the upside-down count among admitted pairs, and the candidate count before the gates;
- the counts the arm's own log prints, plus per-pair degrees/mm from the `per_edge` JSON;
- each job id with its final `sacct` State/ExitCode (poll `sacct` directly, never `slurm_poll.sh`).

## Part E — pre-registered reading (before any job is submitted)

1. **Arm 1 >= 4/9 sherds and >= 8/18 edges, no non-GT join** -> the scale cure works at shipped gates; the
   window was the binder, the millimetre gates are not what 3x broke.
2. **Arm 1 ~ 1x but arm 2 markedly better** -> the window is fixed and the agreement gate (0.5 mm of the
   real vessel at 3x) is the remaining binder; the next ticket makes the mm gates scale-aware. **Do not
   edit any constant in this ticket.**
3. **Both arms ~ 1x and upside-down persists** -> scale does not fix assembly; the binder is the pose
   estimate/registration, and the scale line stops here.

Report the numbers first and the reading second. A shortfall is reported as a shortfall.

## Acceptance

- Pot A 8/8 + 15/15 twice, switch unset, same binary.
- Two repeats per arm; every job id with final sacct State/ExitCode.
- The per-arm table against the 1x reference (2/9 sherds, 1/18 edges; 332 `base18`), every number
  traceable to a named file.
- Consolidated assembly meshes / snapshots exported for the lead to eye (do not judge the pictures
  yourself; the lead renders and rules).

## Must not

- No change to any gate in arm 1; no scale-aware gate work here (that was ticket 329, wontfix, and would be
  its own ticket).
- No corpus write (`Dataset/` and `runs/` stay read-only apart from this run's own directory).
- No edit to ticket 331's run dir; no re-run of the chain; no change to the axis estimator.
- Do not quote ticket 331's own control column as the 1x baseline - piece 1 is anomalous there (lead block,
  ticket 331); the 1x reference is ticket 332's `base18` trees.
- No commit, no push, no branch or worktree command; the ticket is edited only by appending
  `## Comments (worker, <date>)` and flipping `Status:` to `results-in`.

## Where it lives

- Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t333-juglet-large/`, built from
  `hpc/scripts/new_run.sh` style layout (`run.json`, `submit.sbatch`, `logs/`, `out/`).
- Templates to read first: `.../20261112-0245_30b80ae_t332-dense-window12/` (tree layout, scoring, arms) and
  `.../20261110-0320_41494d0_t320b-juglet/submit.sbatch` (the juglet assembler job: SIF
  `sfs_main/sfs_pristine_assembly.sif`, `-DJUGLET_ORIG`, `juglet_orig_block.inc`, FLOOR=20, NP=9).
