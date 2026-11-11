# Ticket 332 — Density on the cured tree, with the matching window doubled: the control 69 left open

**Status:** landed
**Working in:** main (structure-from-sherds-pp)
**Branch:** 332-dense-cured-window12
**Answers:** S1
**Depends on:** 328 (merged to preprocessing main 2026-11-12, `3ec34bd`) — the wall side is cured, so a density result is no longer confounded by half the joins being filed on the wrong face
**Blocks:** none
**Worker:** e4e2b266-3eb7-4c07-bab3-907a6a5ab1cc (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-12 02:20 by the lead)
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Ticket 69 landed the doubled edgeline density as env switches, default off, because it
> regressed the axis method on the 141 GT files — and it left one control unrun: the density had never been
> tried on a tree whose wall side is correct. Run that now, and at the same time double the matcher's LCS
> window from 6 to 12 dots so the doubling of the sampling does not also halve the physical window.

## Why this is a clean control, in numbers

- The join test is a longest-common-subsequence run along the two edge lines, and the run must exceed
  `windowsize` (`class/feature_matching.cpp:294`), which `main.cpp:211` passes as
  `MINIMUM_NUMBER` = **6** (`class/reconstruction.h:24`). So a join needs a run of **7 dots**.
- At the shipped spacing that is 7 x 1.9 = **12.6 mm** of edge. With the density doubled (0.95 mm spacing)
  the same 12.6 mm is **13 dots** — so the window must go from 6 to **12** (runs must exceed 12) for the
  physical requirement to stay put at ~12.4 mm.
- On the cured, sparse tree six of the nine touching pairs die at exactly this floor or just after it
  (ticket 329 bar 2, job 34725192): four never form a candidate (matched runs of 1-4 dots) and four form
  only upside-down fits at 141-175 deg.
- The density double was measured before the cure (ticket 68, job 34268623): dot counts went from
  59/32/44/31/43/45/54/40/22 to **123/68/94/66/90/94/112/84/49**, true-join candidates from 12/18 to 18/18,
  and the vessel still seated 0 of 9. Ticket 69 then merged it default off (commit `b8fed69`;
  `SFS_SPHERE_RADIUS_MM`, `SFS_RESAMPLE_D_MM`, `edgeline_extraction_headless.cpp:485` and `:574`).

**So the question is narrow and pre-registerable:** with the wall side fixed, does doubling the *sampling*
(and keeping the physical window) seat the juglet — or was the 2x density result pre-cure a statement about
the walls, not about the sampling?

## Part A — the dense chain run on the cured tree

Run the standard preprocessing chain on the nine Juglet pieces from **preprocessing main** (the cured tree,
preprocessing main at `676def2` or later), with the density switches on and **no code change**:

```
SFS_SPHERE_RADIUS_MM=0.9 SFS_RESAMPLE_D_MM=0.95
```

Everything else stays at shipped defaults. Report:
- dot counts per piece, against 123/68/94/66/90/94/112/84/49 (the pre-cure measurement) — they need not be
  identical, the mesh stage draws 30 unseeded ray-cast points (`mesh_processing_headless.cpp:1526-1527`),
  so state the run-to-run spread rather than claiming byte identity;
- piece 9 must be flagged if it stays under 50 dots: the authors' own rule excludes pieces under 50
  points (`papers/text/sfspp-2502.13986v1.md:329`), so its result is outside the method's stated domain;
- the axis directions against `Dataset/Juglet_GT/Juglet_Piece_<i>_Axis.xyz` (the GT key; the last 6
  numbers of the file are point xyz then direction xyz), because ticket 69's Part A is where the density
  cost appeared on pots and the same must be checked here.

Use your own build/scratch directory under your run directory. Do **not** build in, write to, or leave
artefacts in the canonical checkout `/data/gpfs/projects/punim2657/sfs_preprocessing` — another worker
(ticket 331) is running the same chain with shipped defaults at the same time, and the two must not share a
binary, a build tree or a run directory.

## Part B — the window, switchable, default unchanged

Add one switch in the assembler, one code path:

- `main.cpp:211` currently reads `FeatureComp(shard, LCS_out, 25, MINIMUM_NUMBER, 0);`. Replace the
  `MINIMUM_NUMBER` argument with a value read once from the environment, `SFS_LCS_WINDOW`, defaulting to
  `MINIMUM_NUMBER` when unset or unparseable. Do not change `MINIMUM_NUMBER` itself: it also gates
  registration at `class/feature_matching.cpp:1015` and `:1045` (`dummy_inlier > MINIMUM_NUMBER / 2`),
  and that gate must not move.
- The binary must print the effective window once at start-up (`LCS window: <n>`) so every log states its
  own regime.
- With the variable unset, the binary must produce the same results as today on a pot lane — verify on
  **Pot A** (8/8 sherds, 15/15 edges) rather than assuming it.

## Part C — three arms, two repeats each, on the juglet lane

Arm A — sparse lines, window 6 (today's behaviour; the control).
Arm B — dense lines (Part A), window 6 (the pre-cure density arm, re-measured after the cure).
Arm C — dense lines (Part A), window 12 (the ticket's ask: sampling doubled, physical window preserved).

Submit each arm twice (the assembler is not deterministic: Pot E produced 30/31+57/62 and 31/31+62/62 from
byte-identical inputs). Score with the existing scorer against `Dataset/Juglet_GT/` and report, per arm and
per repeat: sherds seated within **10 mm and 30 deg**, and the number of true joins realised out of the nine
touching pairs. Every job gets its poll and final `sacct` State/ExitCode in the ticket.

## Pre-registered reading (write this into the ticket before the first sbatch)

- **C seats >= 7 of 9** (and beats B and A): the failure was sampling, not window length or the walls.
  Density is the cure, and 69's default-off merge should be revisited for the juglet lane only.
- **B > A but C ~ A**: the window length is what binds; more dots only help when the required run is
  shorter. That is the size hypothesis, and ticket 331 is the direct test of it.
- **C ~ B ~ A**: neither sampling nor window length is the gate on the cured tree. The remaining suspect is
  the registration itself — four pairs fit upside-down at 141-175 deg, which is a transform-estimate
  failure, not an acceptance-threshold one — and the next ticket is about the fit, not the gates.

## Must not

- Change `RESAMPLE_D_MM` or `sphereRadius` in the code, or any constant in `sfs_axes`. The density is
  switched by environment only.
- Scale any millimetre or area gate by object size. Ticket 326 was dropped for exactly that; do not revive
  it here.
- Write into `Dataset/`, `original_samples/` or any existing corpus directory.
- Run a pot lane with `SFS_LCS_WINDOW` set. The pots are the untouched control; Pot A is run only with the
  variable unset.
- Commit or push. The lead verifies from artefacts and lands.

## Deliverables

Dot-count and axis tables, the three-arm score table with job ids and sacct states, the nine-touching-pair
join table per arm, and the Pot A neutrality run — all in the ticket Comments with exact paths.

## Comments (worker, 2026-11-12)

### Pre-registered reading (copied verbatim from the ticket before any sbatch)

- **C seats >= 7 of 9** (and beats B and A): the failure was sampling, not window length or the walls.
  Density is the cure, and 69's default-off merge should be revisited for the juglet lane only.
- **B > A but C ~ A**: the window length is what binds; more dots only help when the required run is
  shorter. That is the size hypothesis, and ticket 331 is the direct test of it.
- **C ~ B ~ A**: neither sampling nor window length is the gate on the cured tree. The remaining suspect is
  the registration itself — four pairs fit upside-down at 141-175 deg, which is a transform-estimate
  failure, not an acceptance-threshold one — and the next ticket is about the fit, not the gates.

### Checkpoint 0 — plan, nothing submitted yet

- Run directory: `/data/gpfs/projects/punim2657/SfSpp/runs/20261112-0245_30b80ae_t332-dense-window12`
  (own clone + own build under it; the canonical preprocessing checkout
  `/data/gpfs/projects/punim2657/sfs_preprocessing` is not written, not built in, not shared — ticket 331 owns it).
- Part A: preprocessing source at main `b1e1580` (code identical to the cured merge `676def2`; b1e1580 only adds
  ticket 331's doc — `git show --stat b1e1580` = 1 file under .scratch), moved into the run dir and built there
  inside `pcl_191_nurbs.sif` (MeshPreprocessingHeadless + EdgeLineExtractionHeadless). Chain runner: the repo's
  dual-arm cell script `.scratch/juglet-breaklines/stage69/run_cell69.sh` (ticket 69), which runs ONE mesh stage per
  piece and then feeds TWO edge stages from one snapshot (shipped 1.8 mm walk in `geom18`, `SFS_SPHERE_RADIUS_MM=0.9
  SFS_RESAMPLE_D_MM=0.95` in `geom09`), so the two line sets are a paired measurement of the same surfaces.
  Juglet lane recipe unchanged from t66/t328: `SFS_WORNESS=0.7 SFS_NORMAL_MODE=radius SFS_NORMAL_RADIUS_MM=1.0`,
  corpus clouds `Dataset/Point/Juglet` gated against `MANIFEST_JUGLET_CC.tsv`, axes `Dataset/Axes/Juglet`,
  meshes `Dataset/Mesh/Juglet`.
- Part B: one logical change mirrored in the two files that carry the call — `main.cpp:211` (repo assembler) and
  `hpc/tools/main_pristine_batch.cpp:209` (the batch shim that actually runs in every pristine-SIF job; the shim is
  upstream main.cpp with only the viewer/display statements removed, so leaving it alone would run window 6 in
  "window 12" clothes). `MINIMUM_NUMBER` (class/reconstruction.h:24) is untouched. Both print `LCS window: <n>`.
- Part C: six assembler runs (A1 A2 B1 B2 C1 C2) on the dismantled juglet trees built from the Part A geometry,
  scored with the existing scorer against `Dataset/Juglet_GT`, plus two Pot A runs with `SFS_LCS_WINDOW` unset.
- Nothing has been submitted at the time of this checkpoint.

### Checkpoint 1 — Part A submitted (chain job 34874212)

- Run dir `/data/gpfs/projects/punim2657/SfSpp/runs/20261112-0245_30b80ae_t332-dense-window12` (dirs `src/ logs/ work/ out/ score/ trees/`). Nothing is written into the canonical preprocessing checkout or into ticket 331's run dir.
- Source moved into the run dir as a tarball, not cloned into the shared checkout: `src/pp_src.tar` = `git archive b1e1580` of SfSpp_preprocessing (58,992,640 B, md5 `b6ba486022c1f9a8b03a6ebfd0b97423`); `src/pp_scripts.tar` = the same commit's `.scratch/juglet-breaklines/{stage69,stage55,stage66}` (1,802,240 B, md5 `3e6fefa1ccad719eb3bc6b738ca76042`), because `.scratch` is excluded from the source archive. Extracted to `src/pp` (723 files, 61 MB); density switches verified present in `src/pp/edgeline_extraction_headless.cpp` (`SFS_SPHERE_RADIUS_MM` parsed at :487, `SFS_RESAMPLE_D_MM` at :576, defaults 1.8 / 1.9 untouched, no source edit).
- Chain script `submit_chain332.sbatch` in the run dir; Slurm job **34874212** (`t332-chain`, sapphire, 16 CPUs, 64 GB, 4 h), submitted 2026-11-11T15:32Z from the run dir, running on `spartan-bm164`. The build of MeshPreprocessingHeadless + EdgeLineExtractionHeadless happens inside the job in `src/pp/build` with `pcl_191_nurbs.sif` and `--bind /data:/data` (the t328 build recipe); binary md5s go to `binaries.md5`.
- The job runs the dual-arm cell script twice — two cells, each carrying the same paired dense/sparse measurement of the same surfaces: cell `gt` with `AXIS_DIR=Dataset/Juglet_GT`, cell `ds` with `AXIS_DIR=Dataset/Axes/Juglet` (our shipped September PotSAC output). Both cells carry `SFS_WORNESS=0.7` and `MESH_ENV="SFS_NORMAL_MODE=radius SFS_NORMAL_RADIUS_MM=1.0"`; arm 18 = shipped 1.8 mm walk, arm 09 = `SFS_SPHERE_RADIUS_MM=0.9 SFS_RESAMPLE_D_MM=0.95`. The two cells see the same clouds and meshes and differ only in the axis file: the ticket asks for the axis directions against `Dataset/Juglet_GT`, and the assembler trees need the axis the cured-tree baseline was assembled on (checkpoint 2).
- Part B/C script `submit_asm332.sbatch` is written and uploaded but **not submitted** — it needs the lanes. It applies `src/asm_patch.diff` (md5 `7dc8da104055fd9e13d3e30d98951c47`) to a `git archive HEAD main.cpp hpc/tools/main_pristine_batch.cpp` copy (`src/asm_src.tar`, md5 `d0293d947474610c40ad09d93efb508c`) and refuses to run if either file lacks `SFS_LCS_WINDOW`, the `LCS window: ` banner, or the `FeatureComp(shard, LCS_out, 25, lcs_window, 0)` call.

### Checkpoint 2 — Part B/C tooling verified offline; chain still running

- `src/asm_patch.diff` was regenerated as a **two-file** diff (the first cut also carried the ticket doc, which made `patch` stop and ask in the run dir): now md5 `c8527b82b09dfad12c702b2f614d4b8d`, exactly 2 `diff --git` entries, `main.cpp` and `hpc/tools/main_pristine_batch.cpp`.
- The patch was replayed by hand against a fresh `src/asm_src.tar` before use: both files take all 4 hunks, the patched shim carries `SFS_LCS_WINDOW` at line 76, the out-of-range message at :82 and `LCS window: ` at :101, and `patch_guard67.py` on top of it returns `GUARD OK` with 2 `T67-` markers and the floor literal still `point_.cols() < 50` (line 115) for the arm helper to sed.
- `submit_asm332.sbatch` therefore `cd`s into the source copy before patching and refuses to start if the diff does not contain `+++ b/main.cpp`.
- Pot A neutrality reference, for the comparison this ticket owes: run `20261111-0230_20b52bf_t329-potA-8c`, labels `A_MOR1` and `A_MOR2`, both `8/8` sherds and `15/15` edges (`score/acc.tsv`). The Pot A arms in `submit_asm332.sbatch` run with `SFS_LCS_WINDOW` unset (the driver unsets every `SFS_*`/`POT_NAME` variable it inherits) and must reproduce that.
- Chain job 34874212 progressed past its corpus-manifest gate (all nine Juglet clouds md5- and count-verified against `MANIFEST_JUGLET_CC.tsv`) at 15:36Z and started piece 1; nothing has failed.

### Checkpoint 3 — Part A cell `gt` finished; assembler job 34874505 submitted

Chain job 34874212 finished its `gt` cell at 15:44Z (`out/gt/pieces69.tsv`, `status: ok`, all nine pieces, both densities exported, every mesh/edge rc=0, every classify line `face0`). Cell `ds` is running.

Breakline dot counts, cell `gt` (`out/gt/pieces69.tsv`; 18 = shipped 1.8 mm walk, 09 = `SFS_SPHERE_RADIUS_MM=0.9 SFS_RESAMPLE_D_MM=0.95`):

| piece | Bl0 18 | Bl1 18 | Bl0 09 | Bl1 09 | ratio Bl0 | vs ticket-68 dense (123/68/94/66/90/94/112/84/49) |
|---|---|---|---|---|---|---|
| 1 | 59 | 64 | 123 | 132 | 2.08 | 0 |
| 2 | 30 | 32 | 63 | 68 | 2.10 | -5 |
| 3 | 44 | 41 | 94 | 86 | 2.14 | 0 |
| 4 | 31 | 35 | 66 | 73 | 2.13 | 0 |
| 5 | 43 | 51 | 90 | 106 | 2.09 | 0 |
| 6 | 41 | 45 | 86 | 94 | 2.10 | -8 |
| 7 | 54 | 58 | 112 | 121 | 2.07 | 0 |
| 8 | 40 | 42 | 84 | 88 | 2.10 | 0 |
| 9 | 29 | 22 | 64 | 49 | 2.21 | +15 |

- Density multiplies by 2.07-2.21x on Bl0, consistent with ticket 68's 2.04x; three pieces move off the ticket-68 numbers by 5-15 dots (2, 6, 9), which is the unseeded mesh-stage spread the ticket warns about, not a different sampling rule.
- **Piece 9 is at 64 dots (Bl0) and 49 (Bl1) on the dense arm** — above the 50-dot paper exclusion on Bl0, one dot under it on Bl1. Piece 2 is at 63/68. No piece falls below 50 on Bl0, so the authors' floor (papers/text/sfspp-2502.13986v1.md:329) does not exclude any piece from this lane.
- `[SPHERE-69]` / `[RESAMPLE-69]` banners per cell are in `out/<cell>_banners_18.txt` and `out/<cell>_banners_09.txt`; nothing was changed in the preprocessing source (`git diff` of `src/pp` against `b1e1580` is empty).

Shipped axes vs `Dataset/Juglet_GT`, the axis question the ticket asks Part A to report (static measurement of the two files, `analyze332.py` section 2):

| piece | axis angle (deg) | point offset (mm) |
|---|---|---|
| 1 | 1.53 | 13.3 |
| 2 | 87.83 | 533.0 |
| 3 | 31.82 | 258.7 |
| 4 | 10.72 | 100.1 |
| 5 | 44.50 | 334.4 |
| 6 | 13.26 | 62.7 |
| 7 | 155.87 | 207.3 |
| 8 | 12.81 | 107.6 |
| 9 | 106.68 | 510.1 |

median 31.8 deg, max 155.9 deg (piece 7 is 21.2 deg off antiparallel — the axis is in effect upside down). The chain runs two cells precisely because of this: `gt` gives the mesh stage the GT axis, `ds` gives it the shipped axis, and the assembler trees use the GT axes in both.

- Assembler job **34874505** (`t332-asm`, sapphire, 16 CPUs, 64 GB, 4 h) submitted 15:46Z with `CELL=gt PFX=gt`: six juglet runs (`gt18_r1/r2` sparse window 6, `gt09_r1/r2` dense window 6, `gt09w12_r1/r2` dense window 12) plus two Pot A runs (`potA_r1/r2`, window unset). The second assembler job for `CELL=ds` will be submitted when cell `ds` lands; its arms are `ds18`, `ds09`, `ds09w12`.

### Checkpoint 4 — chain complete, both cells' Part A recorded; assembler jobs re-submitted

sacct (`sacct -X`):

```
34874212     t332-chain  COMPLETED      0:0   00:06:32 
34874505       t332-asm     FAILED      4:0   00:00:03 
34874551       t332-asm     FAILED      1:0   00:00:03
```

- Chain job **34874212** COMPLETED `0:0`. Cell `ds` produced the same nine-piece table and the same `status: ok`. Its breakline dot counts are **identical to cell `gt`** (123/63/94/66/90/86/112/84/64 on Bl0); the two cells' exported surfaces `S0`/`S1` are byte-identical for piece 1 (`md5 f90290f1748ab8bfc2b4903c1f7dae07` / `e902c5a4ad23810b560578b78e71188c`), so the axis did not change the mesh for these pieces, while the breaklines do differ (`Bl0` md5 `42e93bd5282ad01fdb2b76641aafba82` gt vs `fce994030924ada3451ea05d70291ddb` ds) — the axis enters the edge walk, not the surface extraction. Both cells are kept because the assembler arms must be read in both.
- Assembler job **34874505** FAILED `4:0`: the first script version read the helper scripts from the per-cell scratch dir (`work/gt/patch_guard67.py`), which does not exist; guard never ran.
- Assembler job **34874551** FAILED `1:0` after 3 s: same root cause one level up — a variable for the shared helper dir was referenced but never defined, and `set -u` aborted the shell before the `||` guard could print. Fixed by defining `HELP=$RUN/work` and adding an explicit existence check for the six helpers before any work starts (script md5 now `b9c73ee87aae0b8fc590e6cc85445c4f`).
- Re-submitted 2026-11-12T15:59Z: **34874769** (`CELL=gt`, six juglet runs + two Pot A) and **34874770** (`CELL=ds`, six juglet runs). Both 16 CPUs / 64 GB / 4 h on sapphire.

### Checkpoint 5 — six juglet arms, two repeats each: measured numbers

Jobs **34875033** (`CELL=gt`) and **34875034** (`CELL=ds`) both COMPLETED `0:0`. Every run exited 0. The window banner was read back per run from `work/<cell>/logs/binary_<label>.log` line 1:

```
gt18_r1  LCS window: 6      gt18_r2  LCS window: 6
gt09_r1  LCS window: 6      gt09_r2  LCS window: 6
gt09w12_r1 LCS window: 12   gt09w12_r2 LCS window: 12
ds18_r1  LCS window: 6      ds18_r2  LCS window: 6
ds09_r1  LCS window: 6      ds09_r2  LCS window: 6
ds09w12_r1 LCS window: 12   ds09w12_r2 LCS window: 12
```

`sherd`/`edge` are the CountResult reproduction (rad 0.35, t 50 mm) — the same metric as the cured-tree baseline 2/9 + 1/18 (job 34710135) and as t325 `GT_r1` 2/9 + 0/18. `pairs9` is how many of the nine touching pairs pass that same bar within the arm's own assembly. `cand` is `Total number :` / `Total number pruned :` from the binary log.

| arm | cell | window | r | sherd | edge | pairs9 | cand total/pruned | wall |
|---|---|---|---|---|---|---|---|---|
| gt18 | gt | 6 | 1 | 2/9 | 1/18 | 1/9 | 32/12 | 62 s |
| gt18 | gt | 6 | 2 | 2/9 | 0/18 | 1/9 | 32/12 | 48 s |
| gt09 | gt | 6 | 1 | 2/9 | 0/18 | 1/9 | 307/101 | 186 s |
| gt09 | gt | 6 | 2 | 2/9 | 0/18 | 1/9 | 307/101 | 119 s |
| gt09w12 | gt | 12 | 1 | 2/9 | 0/18 | 1/9 | 116/46 | 128 s |
| gt09w12 | gt | 12 | 2 | 4/9 | 1/18 | 1/9 | 116/46 | 175 s |
| ds18 | ds | 6 | 1 | 2/9 | 1/18 | 1/9 | 30/11 | 31 s |
| ds18 | ds | 6 | 2 | 2/9 | 1/18 | 1/9 | 30/11 | 26 s |
| ds09 | ds | 6 | 1 | 2/9 | 0/18 | 1/9 | 306/108 | 80 s |
| ds09 | ds | 6 | 2 | 2/9 | 0/18 | 1/9 | 306/108 | 100 s |
| ds09w12 | ds | 12 | 1 | 0/9 | 0/18 | 0/9 | 114/47 | 76 s |
| ds09w12 | ds | 12 | 2 | 3/9 | 1/18 | 1/9 | 114/47 | 81 s |

Two numbers that are not in the score table but belong to it:

- **Candidate counts do not grow with the dense lanes.** Sparse lanes give 30-32 candidates; dense lanes at window 6 give 306-307 (a 9.6x rise, exactly what doubling the dots should do); dense lanes at window 12 give 114-116 — i.e. raising the window **cuts the candidate list by 62 %** while leaving the run requirement at the same physical length. The window is a run-length filter, so at fixed physical length a denser lane yields longer enumerated runs and fewer of them.
- Each arm reads its own `Breaklines` tree: the dense trees carry 2.07-2.21x the dots of the sparse ones on Bl0 (Part A table, checkpoint 3), and every arm's binary log records the same dot counts the staging manifest lists.

Nine touching pairs, per repeat (`P` = pass 0.35 rad / 50 mm; the three columns after each `P`/`.` are deg / mm for that arm). Full matrices are in `logs/report332.txt` (`/data/gpfs/projects/punim2657/SfSpp/runs/20261112-0245_30b80ae_t332-dense-window12/logs/report332.txt`, sections 3-4). The one pair that passes in almost every run is **1-8** (5.6-6.9 deg, 27-51 mm); the sparse control's repeat-1 edge pass is also 1-8 and repeat-2's is **1-4** (6.1 deg, 44.9 mm). Window-12 runs add no new passing pair in repeat 1 and none beyond 1-8 in repeat 2.

Scored per-repeat tables with paths: `score/acc.tsv` (one row per arm/repeat, `sherd`/`edge`), `score/<arm>_r<k>.json` (score58 output: `countresult`, `tables.per_edge` with deg/mm/join residual, `seating`, `gauge`), `score/<arm>_r<k>.Acc.txt` (verbatim binary accuracy block).

- **Pot A neutrality runs in job 34875033 are VOID**: they were launched with the juglet include root (`-DJUGLET_ORIG`), so the binary went looking for `Juglet_Piece_*_Axis.xyz` in a Pot A tree, loaded nothing, and exited 0 with `Total number : 0` / `T67-NO-CANDIDATES`. Their `acc_potA_r*.row` rows read `MISSING`. The correct Pot A job (POT_A include root, no `-DJUGLET_ORIG`, `SFS_LCS_WINDOW` unset) is **34875710**, submitted 16:0xZ; it failed once first (34875589, `1:0`, stage helper demanded juglet lanes) and was fixed by pointing `--lanes` at `out/gt/geom18`.

### Checkpoint 6 (final) — Pot A neutrality, the nine-pair table, and the job ledger

All runs are done. Run directory RUN = /data/gpfs/projects/punim2657/SfSpp/runs/20261112-0245_30b80ae_t332-dense-window12; every path below is relative to it unless absolute.

#### Job ledger (sacct -X, asked directly)

| job | name | state | exit | elapsed | what it did |
|---|---|---|---|---|---|
| 34874212 | t332-chain | COMPLETED | 0:0 | 06:32 | preprocessing build + nine-piece chain, cells gt and ds, both edge arms |
| 34874505 | t332-asm | FAILED | 4:0 | 00:03 | first assembler attempt: helper path pointed at the per-cell scratch dir |
| 34874551 | t332-asm | FAILED | 1:0 | 00:03 | second attempt: HELP referenced but undefined, set -u aborted before the guard message |
| 34874769 | t332-asm | COMPLETED | 0:0 | 00:53 | srun ran, but every arm exited 127 (/panel/arm67.sh: No such file) — no results |
| 34874770 | t332-asm | COMPLETED | 0:0 | 00:49 | same 127, no results |
| 34875033 | t332-asm | COMPLETED | 0:0 | 12:36 | cell gt: six juglet runs + two VOID Pot A runs |
| 34875034 | t332-asm | COMPLETED | 0:0 | 06:50 | cell ds: six juglet runs |
| 34875589 | t332-pota | FAILED | 7:0 | 00:03 | Pot A staging asked the stager for juglet lanes |
| 34875710 | t332-pota | COMPLETED | 0:0 | 11:50 | Pot A neutrality, two repeats, window unset |

Five sbatch failures, all in job plumbing (paths and arguments), none in the code change and none in a measurement; each is listed with its exit code above.

#### Deliverable 1 — density on the cured tree (Part A)

Cells gt and ds gave byte-identical surfaces and identical dot counts (checkpoint 4); the table is in checkpoint 3. Headline: Bl0 dots 59/30/44/31/43/41/54/40/29 -> 123/63/94/66/90/86/112/84/64, ratios 2.07-2.21x, piece 9 the largest gain (+15 over ticket 68's 49) and piece 6 the largest loss (-8). No piece under 50 dots on Bl0, so the authors' 50-dot exclusion does not bite. Axis table (shipped Dataset/Axes/Juglet vs Dataset/Juglet_GT): median 31.8 deg, max 155.9 deg (piece 7), min 1.5 deg (piece 1) — nine rows in checkpoint 3.

#### Deliverable 2 — three arms x two repeats, juglet lane

sherd / edge = CountResult reproduction (0.35 rad, 50 mm), the same metric as the cured baseline 2/9 + 1/18 (job 34710135) and t325 GT_r1 2/9 + 0/18. pairs9 = of the nine touching pairs, how many sit in that run's own output at the same bar. cand = the binary's Total number / Total number pruned.

| arm | cell | window | r | sherd | edge | pairs9 | cand | wall |
|---|---|---|---|---|---|---|---|---|
| A gt18 | gt | 6 | 1 | 2/9 | 1/18 | 1/9 | 32/12 | 62 s |
| A gt18 | gt | 6 | 2 | 2/9 | 0/18 | 1/9 | 32/12 | 48 s |
| B gt09 | gt | 6 | 1 | 2/9 | 0/18 | 1/9 | 307/101 | 186 s |
| B gt09 | gt | 6 | 2 | 2/9 | 0/18 | 1/9 | 307/101 | 119 s |
| C gt09w12 | gt | 12 | 1 | 2/9 | 0/18 | 1/9 | 116/46 | 128 s |
| C gt09w12 | gt | 12 | 2 | 4/9 | 1/18 | 1/9 | 116/46 | 175 s |
| A ds18 | ds | 6 | 1 | 2/9 | 1/18 | 1/9 | 30/11 | 31 s |
| A ds18 | ds | 6 | 2 | 2/9 | 1/18 | 1/9 | 30/11 | 26 s |
| B ds09 | ds | 6 | 1 | 2/9 | 0/18 | 1/9 | 306/108 | 80 s |
| B ds09 | ds | 6 | 2 | 2/9 | 0/18 | 1/9 | 306/108 | 100 s |
| C ds09w12 | ds | 12 | 1 | 0/9 | 0/18 | 0/9 | 114/47 | 76 s |
| C ds09w12 | ds | 12 | 2 | 3/9 | 1/18 | 1/9 | 114/47 | 81 s |

Control check: arm A reproduces the cured-tree baseline (2/9 + 1/18 in three of four A runs, 2/9 + 0/18 in the fourth), so the patched binary with SFS_LCS_WINDOW unset behaves as before on the juglet lane; the Pot A runs below say the same for the pot lane.

#### Deliverable 3 — the nine touching pairs

Per arm and repeat: how many of the nine pairs are present in the output graph at all, how many pass the CountResult bar, and how many of the present ones are upside-down (angle > 90 deg):

| arm | r | present | pass | upside-down | angle range of the present pairs (deg) |
|---|---|---|---|---|---|
| gt18 | 1 | 6/9 | 1/9 | 4 | 4.3 - 178.0 |
| gt18 | 2 | 6/9 | 1/9 | 3 | 6.1 - 176.1 |
| gt09 | 1 | 9/9 | 1/9 | 7 | 6.1 - 172.3 |
| gt09 | 2 | 9/9 | 1/9 | 8 | 6.1 - 176.3 |
| gt09w12 | 1 | 9/9 | 1/9 | 8 | 6.1 - 178.2 |
| gt09w12 | 2 | 9/9 | 1/9 | 5 | 5.6 - 177.7 |
| ds18 | 1 | 6/9 | 1/9 | 3 | 4.4 - 175.5 |
| ds18 | 2 | 6/9 | 1/9 | 5 | 3.0 - 163.7 |
| ds09 | 1 | 9/9 | 1/9 | 6 | 6.4 - 179.0 |
| ds09 | 2 | 9/9 | 1/9 | 6 | 6.5 - 178.5 |
| ds09w12 | 1 | 9/9 | 0/9 | 8 | 6.9 - 174.1 |
| ds09w12 | 2 | 9/9 | 1/9 | 7 | 6.1 - 179.5 |

- The three pairs missing on the sparse arms are always the same: 1-9, 2-9, 7-9 (both cells, both repeats).
- On the dense arms all nine pairs exist — density does at pair level what ticket 68 predicted (true-join candidates 12/18 -> 18/18) and moves those three pairs off the floor.
- The pair that passes is 1-8 in eleven of twelve runs (5.6-6.9 deg, 27-51 mm); the twelfth (gt18_r2) passes 1-4 (6.1 deg, 44.9 mm) and misses 1-8 at 10.2 deg / 66.9 mm. No window-12 run passes a pair that arm A did not already pass.
- Per-pair deg and mm for every arm and repeat: logs/report332.txt section 4; machine-readable form is score/<label>.json -> tables.per_edge (i, j, deg, mm, pass, strict, join_residual_median_mm, join_residual_median_mm_rev, contact_pairs_lt2mm).

#### Pot A neutrality (window unset)

| run | tree | window banner | sherd | edge | wall |
|---|---|---|---|---|---|
| potA_r1 | work/pota/trees/potA_r1 | LCS window: 6 | 8/8 | 15/15 | 381 s |
| potA_r2 | work/pota/trees/potA_r2 | LCS window: 6 | 8/8 | 15/15 | 291 s |

Both repeats match t329's A_MOR1/A_MOR2 (8/8 sherds, 15/15 edges), so the switch with the variable unset leaves the pot lane where it was. SFS_LCS_WINDOW was never set on a pot lane.

#### The code change, as it stands in the worktree

- Two files, one change each: main.cpp (fork entry point) and hpc/tools/main_pristine_batch.cpp (the shim every pristine-SIF job actually runs). Both read the window once at start-up from SFS_LCS_WINDOW, default MINIMUM_NUMBER when unset/empty/unparseable/outside 1..1000, print LCS window: <n> once, and pass the value to FeatureComp(shard, LCS_out, 25, <value>, 0).
- class/reconstruction.h:24 is untouched (MINIMUM_NUMBER stays 6), so the registration gate at class/feature_matching.cpp:1015 and :1045 (dummy_inlier > MINIMUM_NUMBER / 2) cannot move.
- Patch src/asm_patch.diff md5 c8527b82b09dfad12c702b2f614d4b8d; per-cell shim after patch: logs/shim332_gt.md5 and logs/shim332_ds.md5; guarded base work/<cell>/shim/G.cpp with the two T67- markers (logs/guard_<cell>.log).
- The window really takes effect: the twelve banner lines are in checkpoint 5 and in work/<cell>/logs/binary_<label>.log line 1.

#### What the numbers say, in the ticket's own terms

- Density: candidates multiply about 9.6x (32 -> 307) and the three missing pairs appear; the pass count does not move (1/9 throughout).
- Window: at window 12 the candidate list shrinks to 116 (a 62 % cut) instead of growing, because the window is a run-length filter and not a search radius — the dense lane enumerates longer runs and fewer of them. The pass count still does not move.
- Against the pre-registered reading: C does not seat 7 or more of 9 (2/9 and 4/9), so outcome (1) is not met; B is not above A (2/9 vs 2/9 in both cells), so (2) is not met. The numbers are the shape of outcome (3): C ~ B ~ A on seating, with the upside-down fits (5-8 of nine pairs above 90 deg on the dense arms) as the remaining measured feature. That states which pre-registered bucket the numbers match; it is not a recommendation.
- Caveat on one column: score58's own-place seating (seated_count, bar 4.596 mm) is 0 in every arm, and the underlying chamfer is 157-472 mm for every piece — the metric is dominated by a global scale/space mismatch on this lane and is not usable as evidence here. The CountResult numbers above do not depend on it.

#### Artefact index

| what | path (under RUN unless absolute) |
|---|---|
| chain outputs, both cells (lanes, surfaces, per-piece status) | out/gt/, out/ds/ (pieces69.tsv, geom18/, geom09/, *_banners_*.txt) |
| assembled results per arm and repeat | out/<label>/ (incl. 1. Acc.txt) |
| CountResult table | score/acc.tsv (14 rows: 12 juglet + 2 Pot A) |
| per-arm scorer output | score/<label>.json, score/<label>.Acc.txt |
| analysis tables | logs/report332.txt (analyze332b.py output, sections 1-5) |
| per-arm binaries, logs, panel rows | work/<cell>/logs/, logs/panel_<label>.row, logs/window_<label>.txt |
| job scripts | submit_chain332.sbatch, submit_asm332.sbatch, submit_pota332.sbatch, analyze332b.py, pairs332.py |
| sources | src/pp_src.tar, src/asm_src.tar, src/asm_patch.diff (md5s in checkpoint 1) |

Nothing was committed or pushed; the worktree carries the two-file change and this ticket only. The canonical preprocessing checkout and ticket 331's run directory were never written to.

## Lead verification (2026-11-12)

Re-read from the run directory, not from the worker's summary. RUN =
`/data/gpfs/projects/punim2657/SfSpp/runs/20261112-0245_30b80ae_t332-dense-window12`.

- `verified: sacct -X -j 34874212,34875033,34875034,34875710,34874505,34874551,34874769,34874770,34875589`
  -> 34874212 COMPLETED 0:0 06:32; 34875033 COMPLETED 0:0 12:36; 34875034 COMPLETED 0:0 06:50;
  34875710 COMPLETED 0:0 11:50; FAILED 34874505 4:0, 34874551 1:0, 34875589 7:0;
  34874769 and 34874770 COMPLETED 0:0 with every arm exit 127 (nothing measured).
- `verified: logs/window_<label>.txt line 1` -> 14/14 read back: 6 for gt18, gt09, ds18, ds09 and
  both Pot A runs; 12 for gt09w12 and ds09w12. Every arm's log states its own regime.
- `verified: score/acc.tsv` -> the twelve juglet rows exactly as reported: gt18 2/9 1/18 and 2/9 0/18;
  gt09 2/9 0/18 twice; gt09w12 2/9 0/18 and 4/9 1/18; ds18 2/9 1/18 twice; ds09 2/9 0/18 twice;
  ds09w12 0/9 0/18 and 3/9 1/18.
- `verified: out/potA_r{1,2}/1. Acc.txt` -> 8/8 sherds, 15/15 edges in both repeats (381 s, 291 s),
  with the switch unset: the default path is unchanged.
- `verified: work/gt/logs/binary_*.log "Total number"` -> 32/12 (gt18), 307/101 (gt09), 116/46
  (gt09w12), and 0/0 for the two VOID Pot A runs inside 34875033 (they are the reason that job's
  `acc_potA_r*.row` reads MISSING; the real Pot A job is 34875710).
- `verified: PCD header POINTS` on `work/gt/trees/base18|base09/juglet/Breaklines/Juglet_Piece_{1,2,9}_Breakline_0.pcd`
  -> 59 / 30 / 29 and 123 / 63 / 64: the Part A dot table is the header, not an estimate.
- `verified: score/gt09w12_r2.json` -> per_edge pair 1-2 reads deg 148.127, mm 428.5987, the same row
  the report prints.
- `verified: git diff` -> two code files only (`main.cpp`, `hpc/tools/main_pristine_batch.cpp`):
  `SFS_LCS_WINDOW` is read once at start-up, defaults to `MINIMUM_NUMBER` when unset, empty,
  unparseable or outside 1..1000, is printed once, and replaces the argument at the single
  `FeatureComp` call site. `class/reconstruction.h` is untouched, so `MINIMUM_NUMBER` is still 6 and
  the registration gate at `class/feature_matching.cpp:1015`/`:1045` cannot move.
- `verified: ls -la --time-style=+%Y-%m-%d Dataset/Juglet` -> all files 2026-05-29, no write today;
  `Dataset/Juglet_GT` unchanged (2026-11-10 / 2026-09-16). The corpus was read, not written.

**What the answer means (lead's reading, not the worker's).** Density does move the first gate: the
sparse lanes never enumerate 1-9, 2-9 or 7-9 at all (6 of 9 pairs present), the dense lanes enumerate
all nine. It does not move the second: the pass count is 1 of 9 in every arm, and on the dense arms the
share of touching pairs whose only solution is 90-180 deg out rises to 5-8 of nine. Window 12 cuts the
candidate list by 62 % and adds no passing pair, so the window is not what binds once the dots exist.
Read together with ticket 331 (the 3x vessel, where 7 of 9 seams carry a 12.6 mm matched stretch at the
1.5 mm bar), the failure has moved: length was necessary to *form* a candidate, and it is not sufficient
to *solve* one - with enough dots on the seam the matcher prefers a flipped pose. The next thing to
measure is the pose estimate, not the sampling.

Landing: the switch is inert by default (Pot A 8/8 + 15/15 with it unset, twice) and ticket 69's
precedent is that such diagnostic switches live on `main` default-off, so this branch is merged.
