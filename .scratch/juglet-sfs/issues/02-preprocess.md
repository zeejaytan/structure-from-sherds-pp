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

- [ ] Meshes on Spartan under `sfs_preprocessing/Dataset/Mesh/Juglet/`
- [ ] OBJ→PCD (`ObjToPcd`), `MeshPreprocessingHeadless` → Surface_0/1 per piece
- [ ] `EdgeLineExtractionHeadless` → Breakline_0/1 + Surface_F per piece
- [ ] MATLAB PotSAC → Axis per piece (note handle-piece axis failures as scope, S2)
- [ ] Bundle organized as `Juglet_Dataset_YYYYMMDD/SfS_pp/`; spot-check units in mm
