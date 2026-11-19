# The SfS++ chain, end to end — OBJ to reassembly

One page naming every step from a scanned piece to a seated pot: the tool that performs it, where its
inputs and outputs live, and the gate that says the step worked. Two repos carry the chain —
**SfSpp_preprocessing** (OBJ → clouds, surfaces, breaklines, axes) and **this repo** (assembly) — and
the state described here is as of 2026-10-31. When a step changes, change this page in the same commit.

## 0. Inputs

- Per piece: an OBJ mesh from photogrammetry, **in millimetres**. (The Juglet's raw
  `Dataset/Juglet/PieceNN.obj` scans are inches ×50.8; the mm meshes the pipeline reads live at
  `Dataset/Mesh/Juglet/`. Feed the mm ones, never the raw ones.)
- Spartan layout: preprocessing canonical checkout `/data/gpfs/projects/punim2657/sfs_preprocessing`
  (`Dataset/` = meshes, clouds, axes); the released frozen corpus at
  `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Point/` (143 released + 5 derived
  clouds, `MANIFEST_released.tsv` beside them); this repo's canonical checkout
  `/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker` with working area `sfs_main/`; the shared
  run index and attic at `/data/gpfs/projects/punim2657/SfSpp/runs/` and `SfSpp/attic/`.

## 1. Mesh → point cloud (preprocessing README, step 1)

`cc_mesh_to_pcd.sh -i <mesh>.obj -o <dir>` drives CloudCompare offscreen: sample up to **1 000 000
points on the mesh faces**, octree normals at 0.5, MST orientation (k=6), statistical-outlier removal;
output an ASCII PCD `x y z normal_x normal_y normal_z` at `Dataset/Point/<pot>/<piece>_Point.pcd`
with the tool's own `_cc.log` beside it. The script refuses under 10 000 points.

**The corpus rule:** the sampler is unseeded, so one draw is frozen per piece — md5 and point count in
a MANIFEST, never silently refreshed. This step sets the quality of everything downstream that reads
the input's layout: the 2026-10-31 provenance record
(`SfSpp_preprocessing/docs/notes/2026-10-31-juglet-cloud-provenance.md`) is what happened when a
substitute converter (a mesh-vertex dump) stood in for this step on the Juglet — the curvature angle
came out 4× steep and the failure looked like the method's. A substitute converter is a recorded
deviation in that day's ticket, never a private tool.

## 2. Axis (preprocessing README, step 3)

MATLAB `preprocess_new_pots.m` (vendored twin kept in the preprocessing repo) writes the authors'
baseline `Dataset/Axes/<pot>/<piece>_Axis.xyz` per piece; on our Python path, `sfs_axes`
(`select_union_lines` / `run_potsac_union` with `normalized=True, line_rule=True, base_orient=True`
since `SfSpp_preprocessing` ticket 335, `9e0fe6a`) extracts the normalized profile-ranked candidates
and applies blind base-to-rim sign orientation (`orient_base_axis`, opt-out
`SFS_AXES_BASE_ORIENT=0` / `--no-base-orient`). A defective or upside-down axis seats nothing while
looking exactly like a method failure — Pot D 18/29 (ticket 301) is the standing example (released
axis files pointing away from their own poses), and inverted base scans (`Pot_F_Piece_01`,
`Pot_J_Piece_03`, `Juglet_Piece_7`) failed `SignAxisGuard` until ticket 335 flipped base caps from
rim-to-base to base-to-rim.

## 3. Mesh → surfaces (`mesh_processing_headless` → `MeshPreprocessingHeadless`)

Staging first: mesh + cloud + axis are **symlinked** from the frozen corpus into the stage
(`scripts/stage_piece.sh`; the ticket stagers follow it), and every resolved path, md5 and point count
lands in the stage's `inputs.md5`. No geometry is copied.

The binary then: downsamples the input cloud (adaptive spacing, `[SPACING-22]` log line), region-grows
surfaces from seeds at the **growth angle** theta, clusters the result into
`<piece>_Surface_0.ply` (the interior face — the classify step fixes Surface_0 as interior) and
`<piece>_Surface_1.ply`, and writes `<piece>_SampledWithNormals.ply`. Per-piece status goes to the
status file. The whole control surface is the `SFS_*` environment family — a parameter is varied
through env or arguments, **never** by editing source.

theta itself: the paper ships a flat **3.4°** (calibrated on Pot A). Our extension derives it per
sherd from the sherd's own curvature — **theta = 3.3573 × S / R_fold** (ticket 43; `[GROWTH-45]` log
line) — currently opt-in via `SFS_GROWTH_FORMULA=1`, with `SFS_PAPER_THETA` overriding both for
paper-repro runs. The formula's angle is only valid on corpus-conforming clouds (step 1): on a sparse
layout the fold radius under-reads and theta over-reads by the same factor. Tickets 47 (corpus-conforming
Juglet clouds) and 48 (formula as the no-env default, no densify anywhere) are the two open steps here.

## 4. Surfaces → breaklines (`edgeline_extraction_headless` / EdgeLineExtraction)

Walks the interior surface's boundary and emits `<piece>_CompleteBreaklines.xyz`, which becomes
`Breakline_0` (the boundary of Surface_0, the interior) and `Breakline_1`, converted to the `.pcd`
the assembler loads.

**The one-line rule:** the assembler reads exactly **Breakline_0** — `class/data_path.h` names only
the `*_Breakline_0.pcd` arrays; Breakline_1 appears nowhere in the build. A wrong Bl1 cannot change
a seating result; the extraction quality that matters for reassembly is the **interior** line.

## 5. Assembly (this repo)

`class/data_path.h` selects the pot and lists, per piece, `Breaklines/<piece>_Breakline_0.pcd`,
`Mesh/<piece>_Mesh.obj` and `Axes/<piece>_Axis.xyz` under `/Dataset/SfS_pp` — the sbatch binds the
dataset directory into the container at `/Dataset`. The build is the pristine scorer plus the batch
shim; jobs are submitted from the canonical checkout via `hpc/sbatch/` (e.g.
`run_pristine_pota.sbatch`), never from a copy, and no job edits source. The hierarchy-clear
optimization proposes per-piece poses.

## 6. Scoring and the record

- **Seating bar** (the conservator's ruling, 2026-10-27): *own place* = chamfer to home below 7.07 % of
  pot size (≈4.6 mm on the 65 mm Juglet); *right way round* = own place **and** median point distance
  below the same bar. Pot A rides as positive control — 15/15 or the batch is void. The paper's
  20.05° / 50 mm is a comparability figure, not our bar.
- **Look before reporting:** every result that moves or reassembles geometry is rendered before/after
  at a view that resolves the tested scale, paired with a measurement a bad viewpoint cannot distort;
  the picture travels as a workspace-relative link, and visual-qa/ is staged when the decision needs
  orbit.
- **The record:** every job is a run dir `runs/<YYYYMMDD-HHMM>_<git7>_<tag>/` (`run.json`,
  `submit.sbatch`, `logs/`, `out/`), one append-only `runs/index.tsv` row (verdict and human_note
  written by a person or the coordinating session, never the job), every `sbatch` polled with its
  final `sacct` State/ExitCode recorded.

## 7. The setting that assembles the Juglet (2026-11-17, tickets 79/80/362)

This is the first setting on which the pipeline seats a Juglet piece. Nine pieces, one cluster, all
14 adjacent joins inside 5° / 5 mm at their committed poses, **9 of 9 seated** at the own-place bar,
worst 11.093 tree units = **3.70 mm**; Pot A control 8/8 and 15/15 in the same job set
(`structure-from-sherds-pp` ticket 362, source `4012ca1`, run
`runs/20261117-0512_4012ca1_t362-def-lanes`). Quote the whole chain, not one line of it: every
element below is load-bearing, and the breakline switch is a default rather than an env line.

**Step 3 — surfaces** (`MeshPreprocessingHeadless`, 3× tree, `SFS_TREE_UNITS_PER_MM=3.0`):
```
SFS_NORMAL_MODE=radius SFS_NORMAL_RADIUS_MM=1.0
SFS_MERGE_CLUSTER_DOT=0.887
SFS_GROW_BASE_MM=1.5 SFS_GROW_BASE_THETA_DEG=25
MESH_ENV="" on 1× and Pot A lanes
```
The Gnr/Mnr chord growth is what cures eight of the nine sherds; merge dot 0.887 holds them together.
No `SFS_REFINE_ADD_GATE` and no `SFS_REFINE_WALL_GATE` — both hurt the Pot A control.

**Step 4 — breaklines** (`EdgeLineExtraction`, POT_NAME only):
```
SFS_WALL_PAIR_FILTER=pair      # DEFAULT ON since ticket 80; "off" reverts, nothing else changes
```
`SFS_WALL_PAIR_MM=0.80`, `_ED=-0.70` (the ray march exits at dot ≤ −0.70), `_RADIAL_MIN=0.20`,
`_AXISFIT_MIN=0.80`, `_MEDRAD_MIN=0.40`, `_MAXDROP=0.35` are the overrides and stay at their defaults
unless a lane says otherwise. The rule is deliberately narrow: a point is dropped only when it is
**both** off the interior wall (beyond 0.80 mm of a wall-oriented core) **and** unpaired (no partner
along −n between 1.5 and 10 units within 0.45 units), which is why a Pot A base floor — a real
two-walled shell — survives (P05 refuses 62 of 15,460 = 0.401 %). This step is the one that moved the
Juglet: sherd 7's off-lip boundary went 30 dots / 12.6 mm → **2 dots / 0.60 mm**. The assembler reads
only `Bl0` (§4), so this is the line that matters.

**Step 5 — assembly** (the `358` rig on the `335–338` branch fork, floor-10 shim):
```
SFS_AXIS_LOCK_ICP=0 SFS_XY_BREAK_NORMALS=1 SFS_XY_STATE_NORMALS=1
```
`FLOOR=50` → `10` is the only shim change for the Juglet (Pot A keeps the shipped 50 and must come
back 15/15 or the batch is void). With both `XY` switches off the same lanes already seat **5 of 9**
(5.92 mm worst): the breaklines make the vessel assemble, the switches make it sit.

**Step 2 (axes) — landed default (`SfSpp_preprocessing` ticket 335, `9e0fe6a`, 2026-11-19):**
```
SFS_AXES_BASE_ORIENT=1    # DEFAULT ON in sfs_axes/select.py; 0 or --no-base-orient opts out
```
`orient_base_axis` keeps `170/172` benchmark pieces byte-identical to `axes323`, flips only genuine
inverted base scans (`Pot_F_Piece_01`, `Pot_J_Piece_03`, and 3× `Juglet_Piece_7` `175.31° → 4.69°`,
taking `SignAxisGuard` from `4/18` to `0/18`), and holds `Pot F 5/5 7/7` and `Pot J 8/11 16/21`
(`20261119-1644_ac60d8d_t335-ship-base-orient`). 2-DoF tilt-only `a1_k0` was validated across all
10 pots in ticket 336 (`e0cfcce`) and **rejected as default** because it drops corpus `K1 ≤ 10°`
from `135/163` to `116/163` and regresses `Pot D, E, H, J` below their `t323` floors. Keep
`4-DoF c_ref + orient_base_axis`.

**Why the setting works, in one number.** At the true 6–7 pose the pair-stage overlap test reads
**14.35** on these lanes against **52.28** on the GMnr lanes and **191.17** on the 355 lanes — same
engine, same 50 bar, three tracings of the same break. The test was counting the off-lip flap that
step 4 removes; nothing in the search had to change.

**The bar has a scale, and it is the trap.** Every seating number must be quoted with its bar.
`score58.py` hard-codes 4.596 mm — a 1× bar — and reports 0 of 9 on a 3× bundle whose real bar is
3 × 4.596 = **13.788 tree units** (proved from the staged mesh: `Juglet_Piece_2_Mesh.obj` is 84.93
tree units across against 28.31 mm for the 1× file, same 2,565 vertices, ratio 3.0006). Until the
scorer derives the bar from `SFS_TREE_UNITS_PER_MM`, read `tables.seating` in tree units beside the mm
bar, and ignore its `seated` boolean.

**What this setting does not do.** It leaves one wrong join inside the right group (4–6, 62.9° /
14.3 mm at its fit; 4–7 on the stock arm) — invisible to chamfer, so read the join table. And the
overlap test's structural defect stands unpatched: a 100-bar sweep over cross pairs under a pose
fitted to one pair, which prints nothing naming the pair. On these lanes it never fires; on others it
still would.

## Corpus caveats that bind scoring

Pot C 01 and Pot J 01 interior edgelines measure the input, not the method; Pot J 09 and Pot C 03 Bl1
are unscored (wrong references); Pot D 18/29 axes are defective; five scored pieces were excluded by
the authors and never sit beside the paper's numbers; Pots A/B ship full-resolution meshes against
decimated C–J, so mesh-side measurements do not compare across those groups; the Juglet clouds are
vertex dumps until ticket 47 lands, so no input-layout-dependent Juglet score until then. Full list:
`SfSpp_preprocessing/AGENTS.md`, "Corpus caveats". Measurement vocabulary (chamfer, part_acc,
best-of-N): the workspace `docs/glossary.md`, which outranks any local redefinition.

