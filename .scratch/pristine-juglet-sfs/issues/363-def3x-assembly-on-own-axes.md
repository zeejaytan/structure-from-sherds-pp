# 363: Assemble ticket 80's def_3x Juglet on our own code's axes (retire or isolate the oracle loan)

**What to build:** an end-to-end **measurement run** (no source edits to `class/` or `SfSpp_preprocessing/sfs_axes/`) that stops feeding the `Dataset/Juglet_large` GT×3 oracle into ticket 362's working assembly pipeline and runs **our own shipped `sfs_axes` (`axes323`)** extracted from ticket 80's clean segmented `def_3x` `(S0, S1)` walls instead.

**Answers:** S1
**Blocked by:** None (ticket 362 resolved 2026-11-17 at `9/9` seated on `Dataset/Juglet_large` oracle axes; `SfSpp_preprocessing` `main` unchanged at `78082af` / `4a3b400`).
**Status:** resolved
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

- [x] Part A: `axes_def3x` generated from `t80`'s clean `def_3x` `(S0, S1)` walls, Pot A `8/8` `cmp` identical to `axes323`, and all 9 pieces scored against `Dataset/Juglet_large` (lines `0..K-1`).
- [x] Part B: `pota_def_xy` (`8/8, 15/15`), `juglet_oracle_xy` (`9/9` control), and 2 repeats each of `juglet_own_stock` and `juglet_own_xy` completed with `sacct` `State/ExitCode` recorded.
- [x] Part C: full join/cluster/seating table (`13.788` 3× bar) plus verbatim log-backed attribution for any dropped piece/join.
- [x] Part D: `C:/PR/artifacts/t363/gate363.tsv` and `C:/PR/artifacts/t363/render363_assembly.png` (looked at before the Reading line).
- [x] Working tree clean of any `class/`, `hpc/`, or `sfs_axes/` source edits; write-back ends with one `**End state:**` line.

## Comments (worker w363, 2026-11-18)

Reading: own-axes-seat-zero-on-def3x — `juglet_own_xy` seats `0/9` while `juglet_oracle_xy` seats `9/9` and `pota_def_xy` `8/8, 15/15`; the failing stage is named from the log.

**Eye check first** (`C:/PR/artifacts/t363/render363_assembly.png`, looked at before writing the Reading line): with our axes the def_3x juglet is a lumpy, open-ringed body in two clusters with pieces 4, 7 (and 9 in the second repeat) stranded and nothing seated; the oracle panel is a clean closed juglet (18 of 18 GT joins within 5 deg / 5 mm, 9 of 9 seated) and the Pot A control is an intact plate (15/15, 8/8).

### Part A — our `sfs_axes` on t80's clean `def_3x` `(S0, S1)` walls

Run dir `/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0247_4beb57f_t363-own-axes`; job `35293174` `COMPLETED 0:0 00:03:42` (host `spartan-bm176`).

- **Axis-code pin — discrepancy resolved.** The ticket's `78082af` / `4a3b400` do **not exist as commits** in Spartan's `/data/gpfs/projects/punim2657/sfs_preprocessing`. What is verifiable is the *content*: `git -C sfs_preprocessing archive d272313 sfs_axes` (tree `6a67364b477f18769c08834bb2b4fb3622500fd9`) is the sfs_axes that carries `profile.py`, `select.py` and `run_potsac_union(..., line_rule=...)`, and it equals the sfs_axes tree of the laptop checkout `C:/PR/SfSpp_preprocessing` at `78082af`. The checkout HEAD `4c2aec3` (`320-fix-pota-base-axis-regression`, sfs_axes tree `c3e96ae4...`) predates `line_rule`: run on it (job `35293108`, `COMPLETED 0:0 00:01:00`) Pot A reproduces only `Pot_A_Piece_01` (7/8 differ; rank-0 errors 0.26–51.67 deg). That tree is **not** the axes323 ranker; the d272313 tree was therefore the source, extracted to `<run>/src_axes` and md5-listed in `<run>/out/src_axes_md5.txt` and `out/sfs_axes_version.txt`.
- Exact call (as shipped, `SfSpp_preprocessing/scripts/run_benchmark_172.py:73`): `ps.run_potsac_union(C0, C1, seeds=list(range(10)), step=10, max_candidates=6, normalized=True, line_rule=True)`. Walls: `.../20261117-0430_d272313_t80-default-on/out/cell_def_3x/geom/Juglet_Piece_N.{S0,S1}` (ASCII PLY, 6 columns; Piece 1 = 80 475 / 113 234 points).
- **Pot A identity: 8/8 byte-identical** to `.../20261110-0740_d628fc0_t323-axes/out/axes323/Pot_A_Piece_0N_Axis.xyz` (`cmp` per piece), and the job's two repeats are 8/8 identical. This is the shipped-axes323 check the ticket asked for.
- **Our axes vs `Dataset/Juglet_large`:** lines written K = 1,2,1,2,1,1,3,1,3; rank-0 direction error 0.95, 15.73, 9.60, 7.40, 3.62, 4.71, 4.69, 3.19, 6.58 deg. The ranker's best line is **always line 0** (`best_rank=1`, `cand0_vs_best=0.00` for all nine), so no later candidate could have rescued a piece. Rank-0 position offset from the GT axis line: perpendicular 6.86, 134.62, 74.95, 50.79, 28.84, 22.91, 31.35, 22.95, 8.80 mm (tree units /3), signed along-axis -54.52 ... +38.52 mm. **No piece's GT line is reproduced to 0.5 deg** by any written candidate (`ref_lines_matched` false 9/9).

### Part B — the t362 rig on `axes_def3x` (2 repeats, 2 controls)

Same run dir; array job `35294194`, sbatch `submit363_v2.sbatch` (`--array=0-5 --cpus-per-task=16 --mem=64G --time=02:30:00 --account=punim2657 --partition=sapphire`), map `work/arm363_map.tsv`, source `src362.tar.gz`, shim floor 10 (`0c216463ae27fe4571b200cc2292da99`) for the five juglet arms and the **unmodified** shim `daa9d614cfa78de79584cd26933a2057` for `pota_def_xy` (floor 50 makes the sed a no-op — the ticket's single md5 check covers only the floor-10 arms). Every arm printed `=== tree unchanged by the run` and a matching `axes provenance all_equal=yes` (own: `equal=9/9 lines_per_piece=[ 1 2 1 2 1 1 3 1 3 ]`; oracle: 1 line per piece; pota `equal=8/8`).

| arm | sacct -X | sherd | edge | seated at 13.788 | worst chamfer (tree / mm) |
|---|---|---|---|---|---|
| `juglet_own_stock_r1` | COMPLETED 0:0 00:07:40 | 2/9 | 1/18 | 0/9 | 1845.930 / 615.31 |
| `juglet_own_stock_r2` | COMPLETED 0:0 00:17:17 | 2/9 | 1/18 | 0/9 | 1845.965 / 615.32 |
| `juglet_own_xy_r1` | COMPLETED 0:0 00:07:38 | 2/9 | 1/18 | 0/9 | 653.893 / 217.96 |
| `juglet_own_xy_r2` | COMPLETED 0:0 00:07:16 | 2/9 | 1/18 | 0/9 | 653.419 / 217.81 |
| `juglet_oracle_xy` | COMPLETED 0:0 00:09:11 | 9/9 | 13/18 | 9/9 | 11.093 / 3.70 |
| `pota_def_xy` | COMPLETED 0:0 00:06:13 | 8/8 | 15/15 | 8/8 | 1.799 / 0.60 |

Controls hold first: Pot A `8/8, 15/15` and the oracle `9/9, 13/18` with the same per-piece chamfers as t362 (`p1 6.225`, `p2 11.093`), so this is the t362 rig. The two own-axes repeats agree to 0.05 tree units (0.02 mm) — the collapse is not run-to-run luck.

Per-piece chamfer (tree units; mm = /3). `juglet_own_xy` best repeat (r2): p1 595.544, p2 574.714, p3 616.245, p4 644.864, p5 653.419, p6 624.323, p7 600.299, p8 599.553, p9 634.634 — every piece 198–218 mm off. `juglet_oracle_xy`: p1 6.225, p2 11.093, p3 8.810, p4 6.649, p5 6.231, p6 9.632, p7 6.178, p8 6.438, p9 10.194 (2.06–3.70 mm).

### Part C — clusters, joins, and the stage that eats the juglet

- FINAL partition: `juglet_own_xy_r1` `1,2,3,5,6,8,9|4,7`; `juglet_own_xy_r2` `1,2,3,5,6,8|4,7,9`; both stock repeats `2,4,6,7,9|1,3,5,8`; `juglet_oracle_xy` `1,2,3,4,5,6,7,8,9`. GT-adjacent pairs that ever share a cluster: own_xy **16/18** (never (3,7), (7,8)), stock 13/18 and 12/18, oracle **18/18**.
- Rotation error before/after the single global gauge (`score58` `raw_deg`/`gauge_deg`), own_xy r1: p1 121.44/22.64, p2 13.57/115.98, p3 120.64/22.35, p4 142.79/33.33, p5 70.45/51.14, p6 71.73/50.94, p7 167.59/158.77, p8 122.58/24.04, p9 116.61/11.05; oracle 18.96–22.08 raw / 0.31–2.45 gauge. The own-axes failure is not a global gauge artefact (no single rotation fixes it) and is `60x worse in chamfer.
- **Failing stage (verbatim log evidence):** the erosion starts at the **axis input**, not at an assembly gate.
  - For the two GT joins that never form, `[SFS-338] CAND338` records hypotheses at **171.6 / 173.7 deg** from the GT relative pose for (3,7) (offset 86.0 / 94.6 tree units = 28.7 / 31.5 mm) and **170.9 deg** for (7,8) (87.3 tree units = 29.1 mm) — inside-out placements.
  - The >25 deg pair gate is **not** refusing them: there is **no `[SFS-337] REFUSE` row for (3,7) or (7,8)** in either repeat, and the gate does near-identical work in both arms (REFUSE rows own_xy 561 graph / 460 chunk vs oracle 553 / 433; stock 460 / 394).
  - Admission is **not** the discriminator either: the oracle arm drops more at admission (FATE `pc_overlap` 206 vs 134, `sameprio` 58 vs 19; own `pc_dist` 156 vs 89, vetoes 33 vs 20).
  - What differs: with oracle axes the ring closes through other pairs before those flipped hypotheses matter (all nine in one cluster by step 7, trace 91); with our axes the state ends at two clusters with pieces 4 and 7 stranded, the remaining offers into the main cluster are dropped at admission and skipped on the branch budget (`FATE skip branch 3`), and the deepest `[SFS-337] CUT` in own_xy_r1 keeps `74:533,79:520,80:510,77:507,73:500`, dropping `76:499,71:490`.
  - So the named stage is **pairwise hypothesis generation from our axis lines (`[SFS-338] CAND338` to `[SFS-339] OFFER/FATE`)**, fed by axes 0.95–15.73 deg off the GT direction and up to 134.6 mm off the GT line; neither `pair_axis` nor `PairwisePruning` is the differentiator.
  - **All-pair check (not only (3,7)/(7,8)):** piece 7 does form hypotheses with 7 of the other eight pieces (14 `[SFS-338] CAND338` rows), but **none is upright** — the smallest is 133.5 deg (to piece 2), then 148.8 (5), 167.2 (9), 170.9 (8), 171.6 (3), 172.6 (4), 174.7 (6). No partner can carry piece 7 into the body.
  - **And inside the surviving cluster the placement is still mostly wrong:** of the GT joins whose two pieces end in the same cluster, only 1-8 measures 1.60 deg (16.64 mm) — `score58` per-edge `PlaceErr` on `juglet_own_xy_r1` — while 3-5 measures 52.36 deg (278.2 mm) and 1-2 127.24 deg (1540.0 mm). Sharing a cluster is not the same as being correctly placed.
- `XY_BREAK` + `XY_STATE` normals help our axes a lot (worst chamfer 1845.9 to 653.4 tree units) but seat nothing: the switch cannot repair a wrong axis.

### Part D — deliverables

- `C:/PR/artifacts/t363/gate363.tsv` — 206 rows (group `stage_attribution` = the own-axes arm, group `oracle_attribution` = the oracle control, both `score58` `raw_deg`/`gauge_deg`; every Juglet chamfer row is named `*_tree` because 13.788 tree = 4.596 mm real, while Pot A rows stay in mm), header `group piece metric value reference verdict note` (builder `C:/PR/artifacts/t363/gate363.py`). **Deliberate deviation:** the ticket text asks for 8 columns including `arm`; the execution instruction for this ticket asks for the 7 above, so `arm` is folded into `group` and still visible in `note`.
- `C:/PR/artifacts/t363/render363_assembly.png` — four panels at `[SFS-336] FINAL` poses taken from each arm's own run log (own_stock best repeat, own_xy best repeat, oracle, Pot A), bridges labelled with the join's PlaceErr.
- Supporting copies so the lead can recompute without Spartan: `C:/PR/artifacts/t363/partA/partA.json` (every candidate's deg / d_perp / signed-axial per piece), `work/join363.tsv` (Part C relation, 117 rows), `score/*.json`, `logs/binary_*.log`.
- One `runs/index.tsv` row appended (verdict and human_note left blank): `20261118-0247_4beb57f_t363-own-axes  35293174+35294194  4beb57f  t363-own-axes`.

### Failed attempts (all my harness defects; none in the method)

Polled with `scripts/slurm_poll.sh` / `squeue` / `sacct -X` (the ticket's `scripts/remote/job_status.sh` does not exist in this clone).

- `35293913` _0–_5 `FAILED 1:0 00:00:00` — `set -u` with an unset `SOURCE_COMMIT`.
- `35293927` _0–_5 `FAILED 1:0` (00:04–00:11) — my shim-md5 gate grepped the engine log for a line `arm335.sh` prints to its own stdout, and expected one md5 for both floors.
- `35294092` _0–_5 `FAILED 3:0 00:03:33–00:03:43` — stale 0-byte `bundle/.done` plus a `-s` (non-empty) test, so no task staged anything.
- `35294132` _0/_4 `CANCELLED+`, _1/_2/_3/_5 `FAILED 1:0` — unquoted `SGTP=$TREE/Ground Truth` broke the score58 call, and the Pot A provenance loop compared a `Pot_A_Piece_09_Axis.xyz` that does not exist (8 Pot A pieces).
- `35294194` _0–_5 `COMPLETED 0:0` — the reported run.

**What this means:** the t80 def_3x lane is not an assembly problem — same rig, same lanes, oracle axes seat 9/9 and Pot A 8/8. The blocker is the axis line our preprocessing produces on clean 3x walls, which misses the GT direction by 1–16 deg and the GT line by up to 135 mm and leaves the assembler with inside-out hypotheses for the joins that close the ring. The next move belongs to the axes stage (t323's ranker on (S0,S1) walls), not to the assembler's thresholds.

**End state:** ready

### Lead verification (2026-11-18)

- `verified: sacct -j 35293174,35294194 -X` -> `35293174` (`t363-axes`, `COMPLETED 0:0`, `00:03:42`) and `35294194_0..5` (all 6 arms `COMPLETED 0:0`, `00:06:13`–`00:17:17`).
- `verified: cmp -s` on Spartan `axes_pota_chk_r1` vs `axes323` -> `pota_cmp=8/8` byte-identical.
- `verified: C:/PR/artifacts/t363/partA/partA.json` -> Juglet `axes_def3x` vs `Dataset/Juglet_large`: `deg` `[0.95, 15.73, 9.60, 7.40, 3.62, 4.71, 4.69, 3.19, 6.58]` (`8/9 <= 10°`, `best_rank=1` all 9), real `d_perp` `[6.86, 134.62, 74.95, 50.79, 28.84, 22.91, 31.35, 22.95, 8.80] mm` (`20.57–403.86` tree units).
- `verified: C:/PR/artifacts/t363/score/*.json` -> `pota_def_xy` `8/8` (`max 1.799 mm`), `juglet_oracle_xy` `9/9` (`6.178–11.093` tree = `2.06–3.70 mm`), `juglet_own_stock` r1/r2 `0/9` (`549.8–615.3 mm`), `juglet_own_xy` r1/r2 `0/9` (`574.7–653.9` tree = `191.6–218.0 mm`; `1/18` edges `1-8` at `1.60°`).
- `verified: C:/PR/artifacts/t363/logs/binary_juglet_own_xy_r2.log` -> `Piece 7` has `0/14` upright (`rot < 20°`) `CAND338` candidates across all partners (vs 3 in `juglet_oracle_xy`: `(5,7) 4.98°`, `(6,7) 6.67°`, `(7,9) 8.33°`); note also `Piece 7` GT `dir_z = -0.670` while `axes_def3x` canonicalized `dir_z = +0.637`.
- `verified: C:/PR/artifacts/t363/render363_assembly.png` -> viewed 4-panel render; `juglet_oracle_xy` closed juglet (`18/18` in render, `9/9` seated), `pota_def_xy` intact plate (`15/15`, `8/8`), `juglet_own_stock` and `juglet_own_xy` scrambled across 2 clusters with `0/9` seated.
- `verified: C:/PR/artifacts/t363/gate363.tsv` -> 206 data rows, header `group\tpiece\tmetric\tvalue\treference\tverdict\tnote`, `stage_attribution` corrected to `juglet_own_xy_r1` and Juglet chamfer units corrected to `chamfer_tree` with real mm in `note`.
