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

MATLAB `preprocess_new_pots.m` (vendored twin kept in the preprocessing repo) writes
`Dataset/Axes/<pot>/<piece>_Axis.xyz` per piece. A defective axis seats nothing while looking exactly
like a method failure — Pot D 18/29 (ticket 301) is the standing example; those two pieces' released
axis files point away from their own poses.

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

## Corpus caveats that bind scoring

Pot C 01 and Pot J 01 interior edgelines measure the input, not the method; Pot J 09 and Pot C 03 Bl1
are unscored (wrong references); Pot D 18/29 axes are defective; five scored pieces were excluded by
the authors and never sit beside the paper's numbers; Pots A/B ship full-resolution meshes against
decimated C–J, so mesh-side measurements do not compare across those groups; the Juglet clouds are
vertex dumps until ticket 47 lands, so no input-layout-dependent Juglet score until then. Full list:
`SfSpp_preprocessing/AGENTS.md`, "Corpus caveats". Measurement vocabulary (chamfer, part_acc,
best-of-N): the workspace `docs/glossary.md`, which outranks any local redefinition.

