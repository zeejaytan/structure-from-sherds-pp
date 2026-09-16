# juglet-sfs: test the 65 mm Palestinian Juglet end to end on SFS++

9 hand-built sherds (tora `juglet_gt.hdf5`, conservator's Blender reassembly
2026-08-10). SFS++ has never seen this object. Two open questions meet here:

- S1 (this repo): the zero-connections Tray-000 result predates the two unit
  fixes — has SFS++ found *any* real join since the ruler was fixed?
- S2 (this repo): the Juglet is handmade + handled, outside the axial-symmetry
  scope. A failure here may be scope, not capability — report it as such.

## Route

1. `issues/01` — export 9 meshes from the hdf5 at vessel scale
   (70.7098803591493 mm/file-unit, longest side = 65 mm), render, stage.
2. `issues/02` — preprocessing on Spartan: OBJ→PCD, surfaces, breaklines,
   axes (MATLAB PotSAC) → `Juglet_Dataset_*/SfS_pp/`.
3. `issues/03` — assembly on Spartan with a JUGLET block in `data_path.h`;
   report connections + render in the viewer (`Needs-eye:`).

## Decisions so far

- 2026-09-16: source is the hdf5 (assembled, normalized), not the scattered
  scan-table OBJs — same triangles (vertex counts match to ±3), known scale.
  Posterity: `C:\Palestinian_Juglet\Piece*.obj` are unscaled Metashape frames.
- 2026-09-16: GT exists after all — the conservator's Blender reassembly
  (`groundtruth.obj` groups + `juglet_gt.hdf5`). SFS++-format GT derived in
  ticket 01 (18 mates, per-piece input→assembled transforms, 0.0000 mm fit).
  Inputs are scan-pose meshes so the test is honest.
- 2026-09-16: upstream SFS++ and SfSpp_preprocessing both quiet since
  2025-03-06 — pinned versions stand, no sync needed.
- 2026-09-16: NURBS_Dataset_20251103 on Spartan still carries the ×1e6 unit
  bug (5.2e7 in Surface_0) — Juglet gets a fresh dataset, never that one.
