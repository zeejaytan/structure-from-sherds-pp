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

- 2026-09-19: job 30711534 FAILED after 4 min — naming workaround held
  (all 9 pieces segmented, correct `Juglet_Piece_N` names), but Step 2
  died on piece 1: `[pcl::PCDWriter::writeASCII] Input point cloud has
  no data!`. Diagnosed, all ruler-side (measurement #2, not method):
  (a) quarantine dirs sat INSIDE the recursive scans — `backup_02digit/`
  under `Dataset/Point/Juglet/` got re-processed (18 PCDs, Surfaces: 20
  with 2 stale `Juglet_Piece_0_*`); `Temp/stale_obj_backup/` would likewise
  be re-scanned. Both moved under `${OUTPUT_BASE}/_quarantine/` (outside
  every scanned tree, untracked, kept).
  (b) boundary radius: `sqrt(1/N)` assumes points over 1 m^2, pinning r at
  the 15 mm clamp; on the 65 mm / 1.8 mm-wall Juglet that swallows every
  fracture edge (Pot_A is ~120 mm with 843k-point pieces, where 15 mm
  resolves). Mesh sample verified clean (11028 pts, no NaN/inf, bbox
  33x37x33 mm — the -494 Z is scan-table offset, scale is true mm).
  Fix as versioned `SfSpp_preprocessing@patches/juglet_boundary_radius.patch`
  (bbox sheet-area spacing, same 6x/clamp intent; also lowers Pot_A
  re-runs 15 mm -> ~6 mm, noted): job applies it `--forward` to the
  nested copy and rebuilds `EdgeLineExtractionHeadless` in-container.
  Resubmitted as 30752971; polled.

- 2026-09-19: job 30752971 FAILED after 5 min in the NEW Step 1b, on two
  script bugs (patch itself applied cleanly, no .rej — verified in the
  nested copy): (a) Step 1b `cd`s into the nested source dir, so the
  relative container path no longer resolved (`pcl_191_nurbs.sif: no such
  file`) — image path is now absolute; (b) the 2-digit-PCD quarantine
  used a top-level glob, but 30711534's `backup_02digit/` subdir sat
  inside the recursive scan (Surfaces: 20 again) — now a `find` catches
  2-digit PCDs at any depth, and the leftover backup dir is removed.
  Resubmitted as 30761823; polled.

- 2026-09-19: job 30761823 FAILED after 2 min in Step 1b rebuild:
  `CMake Error: source directory "/workspace/original_nurbs_preprocessing"
  does not exist` — the Nov-2025 CMakeCache pins the source dir to
  `/workspace/...`, shadowed in-container like everything else
  (CMakeLists.txt itself is clean). Step 1b now drops the cache and
  re-runs `cmake ..` before `make` (idempotent). Same failure also
  exposed the watcher gap (see below). Resubmitted as 30762110.
- Watcher fix: background polls launched via WSL `bash` never completed
  (WSL ssh has no cluster key; old script also lacked BatchMode so a
  keyless ssh could hang on a password prompt instead of failing).
  `scripts/slurm_poll.sh` now passes `-o BatchMode=yes` (fail fast, never
  hang); polls launched with Git Bash so they inherit the working Windows
  ssh. Verified with a 10 s-interval poll of finished job 30761823.

- [ ] Meshes on Spartan under `sfs_preprocessing/Dataset/Mesh/Juglet/`
- [ ] OBJ→PCD (`ObjToPcd`), `MeshPreprocessingHeadless` → Surface_0/1 per piece
- [ ] `EdgeLineExtractionHeadless` → Breakline_0/1 + Surface_F per piece
- [ ] MATLAB PotSAC → Axis per piece (note handle-piece axis failures as scope, S2)
- [ ] Bundle organized as `Juglet_Dataset_YYYYMMDD/SfS_pp/`; spot-check units in mm
