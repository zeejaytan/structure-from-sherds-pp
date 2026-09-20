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
- Watcher fix (attempt 1, FAILED): background polls launched via WSL `bash`
  never completed (WSL ssh has no cluster key; old script also lacked
  BatchMode so a keyless ssh could hang on a password prompt instead of
  failing). `scripts/slurm_poll.sh` now passes `-o BatchMode=yes`; polls
  launched with Git Bash — but Git Bash has a mangled HOME and its ssh
  is denied too (proven: 170 consecutive ssh failures over 30 min, then
  the shell was reaped). The "verified" claim below was premature.
  Attempt 2: new `scripts/slurm_poll.ps1` runs in the Windows
  ssh-agent context where `ssh spartan` provably works.

- 2026-09-19/20: jobs 30762110 (full, rebuilt v1 binary) + 30762701
  (piece-2 probe) FAILED identically — even at the corrected 2.0-2.8 mm
  radius the boundary cloud is empty on pieces 1 AND 2: systemic, not
  piece-specific. Root cause, one level deeper: the radius was in the
  wrong UNITS, not just the wrong size. The cloud is millimetres, so PCL
  reads 0.015 as 0.015 mm — below the ~0.3 mm point spacing, no neighbour
  is ever found. (v1 "fixed" 15 mm -> 2 mm but kept the metre convention:
  0.002, equally dead.) Same lie in the outlier filter (0.002-0.020),
  which would have emptied the boundary one step later with the identical
  message. Evidence the Nov-2025 adaptive code never succeeded on mm
  data: sane Pot_A breaklines predate it (introduced wholesale in
  nested@3737977). Patch v2 (`patches/juglet_boundary_radius.patch`,
  dry-run verified): mm-unit radii from bbox sheet area + shared
  `g_boundary_radius_mm` so the outlier stage matches the boundary
  stage; Step 1b resets the nested copy to pristine before applying
  (v1 is in the tree). Resubmitted full job as 30822389, watched with
  slurm_poll.ps1 (which doubles as the attempt-2 verification).

- 2026-09-20: job 30822389 (v2 binary) got pieces 1-2 through Step 2
  (12 breakline files) then died on piece 3 — but NOT the boundary:
  piece 3's segmentation made 1 cluster (492/3490), skipped all writes,
  and Step 1 silently copied piece 2's stale tmp files as piece 3's
  surfaces (identical sizes; unclustered.ply missing). Rendered piece 3
  vs 5 (flat, inconclusive), then topology: all meshes closed solids
  (chi=2), so no tube story. Normals coherent, curvature low, neighbour
  angles equal to working piece 9 — growing fragments past the 4.5 deg
  gate on tight bends; survival is marginal luck. Fix: two-pass
  segmentation — new `patches/juglet_mesh_piece_params.patch` makes the
  dead single-file argv mode actually filter + adds SFS_SMOOTHNESS_DEG /
  SFS_CURVATURE_THRESH env overrides (defaults unchanged); Step 1c
  re-runs ONLY piece 3 at 25 deg (rim flips ~180 deg, safe); new Step-1
  gate fails loud on any missing/stale piece output (unclustered.ply +
  cmp check). Resubmitted as 30822854, ps1-watched.

- 2026-09-20: job 30822854 — Step 1c WORKED (piece 3: 2 clusters at 25
  deg; single-file mode + env override proven), but the new gate caught
  a second degenerate piece: piece 9 also makes 1 cluster at defaults
  (missed earlier — only sampling stats were checked, never its
  clusters). Step 1c now loops pieces 3 and 9 with everything else
  quarantined. Resubmitted as 30822989, ps1-watched.

- 2026-09-20: job 30822989 — piece 3 good at 25 deg, but piece 9 still
  1 cluster at 25 deg. Angle CDF for piece 9: frac>25 = 0.33 (still
  fragments), frac>45 = 0.08 (should grow); rim flips ~180 deg keep 45
  deg safe. Step 1c takes per-piece thresholds (3:25, 9:45).
  Resubmitted as 30823406, ps1-watched.

- 2026-09-20: job 30823406 — piece 3 good at 25 deg, but piece 9 still
  1 cluster (92 pts) at 45 deg. Code read explains the 1-iteration exits:
  an explicit `break` on `clusters.size() == 1` skips the built-in retry
  loop (+2 deg/iter, cap 45) — only the 0-cluster case (piece 2) ever
  retries. Piece 9 angle CDF: frac>45 = 0.08, frac>60 = 0.02, so 60 deg
  should grow while rim flips (~180 deg) still stop it. Step 1c now
  3:25, 9:60. Resubmitted as 30823739, ps1-watched.

- 2026-09-20: job 30823739 — piece 9 still 1 cluster (92 pts) at 60 deg.
  Smoothness escalation exhausted (4.5/25/45/60, identical symptom), so a
  new cause is needed. Eliminated: NaN, duplicates, density, topology,
  orientation patches (1 agree-component), curvature, stale files. Local
  replication of region growing on the mesh gives ONE giant region at
  25 deg+ — contradicting the binary's dust — so the binary's growing
  inputs must differ from the model. Added a GROW-DIAG print (cloud /
  normals sizes, true mean neighbour angle and max curvature of the exact
  growing inputs) to the mesh patch. Resubmitted as 30824422 to capture
  it (gate will fail on piece 9 as before; the diagnostic line is the
  product), ps1-watched.

- [ ] Meshes on Spartan under `sfs_preprocessing/Dataset/Mesh/Juglet/`
- [ ] OBJ→PCD (`ObjToPcd`), `MeshPreprocessingHeadless` → Surface_0/1 per piece
- [ ] `EdgeLineExtractionHeadless` → Breakline_0/1 + Surface_F per piece
- [ ] MATLAB PotSAC → Axis per piece (note handle-piece axis failures as scope, S2)
- [ ] Bundle organized as `Juglet_Dataset_YYYYMMDD/SfS_pp/`; spot-check units in mm
