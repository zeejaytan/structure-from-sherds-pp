# 363: Assemble ticket 80's def_3x Juglet on our own code's axes (retire or isolate the oracle loan)

**What to build:** an end-to-end **measurement run** (no source edits to `class/` or `SfSpp_preprocessing/sfs_axes/`) that stops feeding the `Dataset/Juglet_large` GT×3 oracle into ticket 362's working assembly pipeline and runs **our own shipped `sfs_axes` (`axes323`)** extracted from ticket 80's clean segmented `def_3x` `(S0, S1)` walls instead.

**Answers:** S1
**Blocked by:** None (ticket 362 resolved 2026-11-17 at `9/9` seated on `Dataset/Juglet_large` oracle axes; `SfSpp_preprocessing` `main` unchanged at `78082af` / `4a3b400`).
**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-18)
**Branch:** 363-def3x-assembly-on-own-axes, in /c/PR/worktrees/structure-from-sherds-pp/363
**Worker:** dsh one-shot subagent w363
**Budget:** medium 5
**Hand-over:** DeepSeek — Spartan compute node (read-only over frozen corpora and existing `runs/`). Check path: the lead re-reads `C:/PR/artifacts/t363/gate363.tsv`, recomputes the `axes_def3x` direction errors against `Dataset/Juglet_large`, verifies the Pot A control (`8/8, 15/15`), verifies the `juglet_own_*` and `juglet_oracle_xy` cluster/join/seating numbers from the run logs, and **looks at** `C:/PR/artifacts/t363/render363_assembly.png` before recording any verdict.

## Why this ticket exists (verified — do not re-derive)

- **The Juglet has never been assembled on our current preprocessing + assembly pipeline with our own axes:**
  - In `t320b` (`322`), our axes ran only on **1× broken edgelines** with old `axes320` (`4/9`) / Sept MATLAB (`1/9`).
  - In `t331`–`t362`, every 3× Juglet assembly run used **`AXIS_DIR=/data/gpfs/projects/punim2657/Dataset/Juglet_large`** (the GT answer key scaled ×3). On ticket 80's `def_3x` lanes, that oracle run (`362`) seats **9/9** (`juglet_def_xy`, max chamfer `11.093` tree units = `3.70 mm`) and **5/9** even on `juglet_def_stock` (all 9 in one cluster, 14/14 adjacent joins ≤ 5°/5 mm).
- **We also have not yet measured shipped `sfs_axes` (`axes323`) on clean 3× `(S0, S1)` walls:**
  - All previous 3× axis tests (`332`, `333`, `334`) fed `sfs_axes` the unsegmented watertight mesh cloud `Point.pcd` (including fracture edges) doubled as `(C, C)` instead of the separated interior/exterior walls `(S0, S1)`.
  - Even on `Point.pcd`, `axes323` already hits **7/9 rank-0** and **8/9 best-of-3** (`p3 10.11°` / line 2 = `6.06°`; only `p9 29.81°` misses) — nearly the same single-sherd hit rate as the 10 corpus pots (**135/163 = 82.8% rank-0**, **143/163 = 87.7% best-of-3**), which assemble **20/20** despite 28 pot pieces missing `≤ 10°` at rank 0.
- **What we need to know:** when shipped `sfs_axes` runs on `t80`'s clean `def_3x` `(S0, S1)` walls and feeds `t362`'s assembler (`juglet_def_stock` and `juglet_def_xy`), how many pieces seat, and if any drop out, at which exact stage (`pair_axis`, `XY_BREAK`, `GraphAxisRefinement`, or pruning) does the axis error bite?

## Part A — Extract 3× axes from clean `def_3x` `(S0, S1)` walls (no code edit)

1. Locate ticket 80's `def_3x` segmented walls in `/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0430_d272313_t80-default-on/out/cell_def_3x/` (`geom/` or `pieces/Juglet_Piece_1..9/`, `.S0` and `.S1` PLY/XYZ clouds — verify exact paths and log them; refuse if missing).
2. Using the **unchanged** `/data/gpfs/projects/punim2657/sfs_preprocessing` checkout on `main` (record commit hash; must match `78082af` / `sfs_axes/` unchanged), run the exact shipped `axes323` call (`SfSpp_preprocessing/scripts/run_benchmark_172.py:73`):
   `ps.run_potsac_union(C0, C1, seeds=list(range(10)), step=10, max_candidates=6, normalized=True, line_rule=True)`
   on each of the 9 pieces' `(S0, S1)` walls, writing `<run>/axes_def3x/Juglet_Piece_N_Axis.xyz` in assembly format (`pos_x pos_y pos_z dir_x dir_y dir_z`, canonical `dir_z >= 0`).
3. **Pot A identity sanity check:** run the same script on Pot A's 8 corpus pieces (`Pot_A_Piece_01..08`) into `<run>/axes_pota_chk/` and `cmp` all 8 files against `/data/gpfs/projects/punim2657/SfSpp/runs/20261110-0740_d628fc0_t323-axes/out/axes323/Pot_A_Piece_0*_Axis.xyz` (`8/8` byte-identical required, else stop).
4. **Score `axes_def3x` against `Dataset/Juglet_large/Juglet_Piece_N_Axis.xyz`:** report `K` (lines written), undirected direction error (deg) for every line `0..K-1`, centroid offset (both tree units and real mm = `/ 3.0`), rank-0 `≤ 10°` count, and best-of-3 `≤ 10°` count.

## Part B — Run the `t362` assembly rig on `axes_def3x` (2 repeats)

1. Create a fresh run directory `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t363-own-axes/`.
2. Reuse ticket 362's build/rig (`/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0512_4012ca1_t362-def-lanes`): same container (`sfs_pristine_assembly.sif`), same `main_pristine_batch.cpp` floor-10 shim (verify md5 `0c216463ae27fe4571b200cc2292da99`), same `tools/arm335.sh`, same `def_3x` Breaklines/Surfaces/Meshes/GT staging (9 Breaklines / 18 Surfaces / 9 Mesh / 10 GT for Juglet; 8/16/8/8/9 for Pot A; `SFS_AXIS_LOCK_ICP` unset).
3. Run the following arms (with **2 independent repeats** `_r1`, `_r2` of `juglet_own_stock` and `juglet_own_xy`, since the assembler is non-deterministic):
   - `juglet_own_stock`: `XY_BREAK=0`, `XY_STATE=0`, `FLOOR=10`, `Axes = <run>/axes_def3x/`
   - `juglet_own_xy`: `XY_BREAK=1`, `XY_STATE=1`, `FLOOR=10`, `Axes = <run>/axes_def3x/`
   - `juglet_oracle_xy`: `XY_BREAK=1`, `XY_STATE=1`, `FLOOR=10`, `Axes = /data/gpfs/projects/punim2657/Dataset/Juglet_large/` (positive control reproducing 362's `9/9`)
   - `pota_def_xy`: `Pot_A`, `XY_BREAK=1`, `XY_STATE=1`, `FLOOR=50` (positive control: must hit `8/8, 15/15` or Part B is void)
4. Poll every `sbatch` with `scripts/remote/job_status.sh` and record final `sacct -X` `State` / `ExitCode`. Append one `runs/index.tsv` row (leaving `verdict` and `human_note` blank).

## Part C — Scoring and stage attribution

1. Report per arm & repeat: `Sherd accuracy`, `Edge accuracy`, `FINAL` cluster partition (`[SFS-337] STEP`), count of the 14 GT-adjacent joins inside `PlaceErr ≤ 5° / 5 mm`, and `score58` seating at the **3× bar = `13.788` tree units (`4.596 mm` real)**, listing all 9 piece chamfers (tree units and mm) and best-of-2.
2. **Failure attribution (required whenever any piece fails to join/seat vs `juglet_oracle_xy`):**
   - Inspect `[SFS-338] CAND338`, `[SFS-336] CAND`/`DROP`/`SURV`, `[SFS-337] STEP`, and `[SFS-339] OFFER`/`FATE` in the arm's log for every missing piece/pair.
   - State mechanically which stage dropped or distorted it:
     - Pairwise matching / `XY_BREAK` / `pair_axis` gate (`> 25°`),
     - `GraphAxisRefinement` inside a growing cluster (`ranking_system.cpp:2554`), or
     - `PairwisePruning` / `CheckGraphPlausibility`.

## Part D — Deliverables and visual check

- `C:/PR/artifacts/t363/gate363.tsv` — header `group arm piece metric value reference verdict note`, covering Part A (`axes_def3x` + Pot A `cmp`), Part B controls, Part C arms (both repeats + best-of-2), and Part C attribution rows.
- `C:/PR/artifacts/t363/render363_assembly.png` — four labelled panels at `FINAL` poses: `juglet_own_stock` (best repeat), `juglet_own_xy` (best repeat), `juglet_oracle_xy`, and `pota_def_xy`. **Look at it before writing the Reading line**, and add one sentence describing what the eye sees.

## Readings (pick one verbatim, on a `Reading:` line)

- `own-axes-assemble-the-juglet` — `juglet_own_xy` seats `9/9` at the `13.788` bar (Pot A `8/8, 15/15`), retiring the `Juglet_large` oracle on this lane.
- `own-axes-partial-seating-gap-named` — `juglet_own_xy` seats `1..8` of 9 (or drops joins vs oracle); the dropped piece(s) and exact failing stage are named from the log.
- `own-axes-seat-zero-on-def3x` — `juglet_own_xy` seats `0/9` while `juglet_oracle_xy` seats `9/9` and `pota_def_xy` `8/8, 15/15`; the failing stage is named from the log.
- `blocked: control-or-input-failed` — `pota_def_xy` fails `8/8, 15/15`, `juglet_oracle_xy` fails to reproduce 362, or a named `t80` / `t362` input file is missing.

## Out of scope

- Any edit to `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`.
- Modifying thresholds, overlap gates, `GraphAxisRefinement`, or `sfs_axes` ranking rules (this ticket measures the shipped system as-is first).
- Committing or pushing from the worker; the lead verifies and lands.

## Acceptance

- [ ] Part A: `axes_def3x` generated from `t80`'s clean `def_3x` `(S0, S1)` walls, Pot A `8/8` `cmp` identical to `axes323`, and all 9 pieces scored against `Dataset/Juglet_large` (lines `0..K-1`).
- [ ] Part B: `pota_def_xy` (`8/8, 15/15`), `juglet_oracle_xy` (`9/9` control), and 2 repeats each of `juglet_own_stock` and `juglet_own_xy` completed with `sacct` `State/ExitCode` recorded.
- [ ] Part C: full join/cluster/seating table (`13.788` 3× bar) plus verbatim log-backed attribution for any dropped piece/join.
- [ ] Part D: `C:/PR/artifacts/t363/gate363.tsv` and `C:/PR/artifacts/t363/render363_assembly.png` (looked at before the Reading line).
- [ ] Working tree clean of any `class/`, `hpc/`, or `sfs_axes/` source edits; write-back ends with one `**End state:**` line.
