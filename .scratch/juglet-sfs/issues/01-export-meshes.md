# 01: Export 9 Juglet meshes from juglet_gt.hdf5 at vessel scale

**What to build:** nine watertight-per-piece OBJ meshes a conservator would
recognise as the Juglet sherds, in millimetres, plus one render proving it.

**Answers:** S1

**Blocked by:** None (can start immediately).

**Status:** resolved

## Result (2026-09-16)

- `artifacts/juglet_input/Mesh/Juglet_Piece_0{1..9}_Mesh.obj`: scan-table
  poses, mm. Vertex counts match hdf5 exactly; faces 1-based.
- `artifacts/juglet_input/Ground Truth/`: `Juglet_Piece_{1..9}_T.txt`
  (SFS++ 4-line format, input-mm → assembled-mm) +
  `Juglet_simple_graph.txt` (GARF contact matrix, 18 mates, cross-checked
  against TORA pairs file — identical).
- Fit quality: one global scale norm→gt = 15.61135, spread 0.0000%; all 9
  pieces exact-order rigid fits, residuals ~1e-14 (≈0.0000 mm — same
  guarantee as `tora/scripts/build_juglet_ground_truth.py`). Conservator
  rotations 26–177°.
- Render `artifacts/juglet_input/preview.png`: scattered inputs vs closed
  vessel — witnessed above. True test poses; the GT closes.
- Derivation: `.scratch/juglet-sfs/derive_gt.py` (fixed-scale fit; free-scale
  ICP degenerates to s→0 — recorded so nobody re-learns it).

- [ ] `artifacts/juglet_input/Juglet_Piece_0{1..9}_Mesh.obj` written from
      `tora/artifacts/juglet_gt.hdf5` at 70.7098803591493 mm/file-unit
      (vessel_scale convention: longest side of reassembly = 65 mm)
- [ ] Faces converted 0-based (hdf5) → 1-based (OBJ); vertex counts match hdf5
- [ ] Combined bounds ≈ 41 × 37 × 65 mm; wall ≈ 1.8 mm (glossary: 1.79 mm)
- [ ] One render of the nine pieces in place, view resolving the mm scale
