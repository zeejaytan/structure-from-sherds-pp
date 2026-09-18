# 02: Preprocess the Juglet on Spartan (surfaces, breaklines, axes)

**What to build:** a complete `Juglet_Dataset_*/SfS_pp/` bundle —
Surfaces, Breaklines, Axes, Mesh — generated from ticket 01's meshes with
the fixed (post-Nov-2025) pipeline, ready for the assembler to consume.

**Answers:** S1

**Blocked by:** 01

**Status:** ready-for-agent

## Comments

- 2026-09-16: job 30637535 FAILED in 4 s — binaries were launched with cwd
  `original_nurbs_preprocessing/build`, but their `Dataset/...` paths are
  repo-root-relative (the Dataset symlink is not under `build/`). Fixed to
  run from repo root; resubmitted as 30637575. Step 0 (OBJ→PCD) had already
  succeeded: 9 PCDs, correct counts.

- 2026-09-17: job 30637575 FAILED the same way (Step 0 ok, Step 1
  `filesystem error: ... cannot open directory [Dataset/Point/Juglet/]`).
  Root cause is the container bind, not the cwd: `--bind ...:/workspace`
  is shadowed because /workspace is already a GPFS mount inside
  `pcl_191_nurbs.sif` (verified: `ls /workspace/Dataset` fails inside,
  stale image content). Fix in `SfSpp_preprocessing@9a65017`
  (`run_juglet_preprocessing.sbatch`): bind `/data:/data`, `cd $REPO_ROOT`.
  Verified the PCDs are visible with that bind. Resubmitted as 30694326
  (RUNNING); laptop-side poll started per workspace rule 4.

- 2026-09-18: job 30694326 FAILED after 2 min — bind fix worked, but two
  deeper Pot_A-era assumptions surfaced (both are measurement-#2 issues,
  broken ruler, not a method failure):
  (a) every binary truncates stems to 14 chars (`stem.substr(0, 14)`,
  tuned for `Pot_A_Piece_01`). `Juglet_Piece_01` is 15 chars, so all 9
  pieces collapsed to `Juglet_Piece_0`, each overwriting the last
  (Surfaces: 2, piece 09's only). (b) breakline discovery scans `Temp/`
  for `*.obj`; in OUTPUT_BASE mode our meshes never land there, so it
  found stale Pot_A meshes and crashed on `Pot_A_Piece_01_*` names.
  Workaround in `SfSpp_preprocessing@0968a0a` (binaries untouched):
  single-digit `Juglet_Piece_1..9` (= exactly 14 chars, truncation is a
  no-op, all stages agree), quarantine stale `Temp/*.obj` to
  `Temp/stale_obj_backup/`, stage our 9 OBJs under `Temp/Data/Juglet/`.
  Proper fix (suffix-strip helper + rebuild) is deferred — the two
  headless sources live in a nested upstream clone
  (`original_nurbs_preprocessing/`, diverged tuning vs tracked root
  copies), so a rebuild needs care. Resubmitted as 30711534; polled.

- [ ] Meshes on Spartan under `sfs_preprocessing/Dataset/Mesh/Juglet/`
- [ ] OBJ→PCD (`ObjToPcd`), `MeshPreprocessingHeadless` → Surface_0/1 per piece
- [ ] `EdgeLineExtractionHeadless` → Breakline_0/1 + Surface_F per piece
- [ ] MATLAB PotSAC → Axis per piece (note handle-piece axis failures as scope, S2)
- [ ] Bundle organized as `Juglet_Dataset_YYYYMMDD/SfS_pp/`; spot-check units in mm
