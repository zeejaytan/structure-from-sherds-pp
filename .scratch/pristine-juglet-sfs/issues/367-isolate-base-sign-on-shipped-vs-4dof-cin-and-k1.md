# 367: Isolate `orient_base_axis` on shipped `sfs_axes` vs. clean 4-DoF `c_in` and `K=1` rank-0 on `def_3x` Juglet & Pot A

**What to build:** a **measurement and prototype ablation run** (zero edits to tracked `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`; all prototype axis logic lives strictly inside `<run>/work/`) isolating why `t366`'s `a366_4dof` regressed compared to `a1_base` (`juglet_a1_base_r2`: `9/9` sherds, `11/18` edges, `8/9 <= 4.596 mm` / `4.653 mm` origin gauge, `0.482 mm` vertex cluster; `pota_a1_k0rig`: `8/8, 15/15` at `2.011 mm`), and determining which exact rule is clean enough to promote into `SfSpp_preprocessing/sfs_axes/`.

We compare three unconfounded axis configurations, all using blind `orient_base_axis` (`t366/work/base_axis.py`) and **none** using `L_pre` injection or `tau=1.25`:

1. **`axes_shipped_base`** — untouched shipped `sfs_axes` (`tree 6a67364b` from `t363`) + `orient_base_axis` only:
   - Tests whether fixing Piece 7's sign alone fixes shipped `axes323` on Juglet (`t363` `axes_def3x` before `orient_base_axis` had `P1=0.95°, P2=15.73°, P3=9.60°, P4=7.40°, P5=3.62°, P6=4.71°, P7=4.69° [175.31° inverted], P8=3.19°, P9=6.58°`).
   - On Pot A (`axes_pota_shipped_base`), `orient_base_axis` must flip `0` lines and remain **8/8 byte-identical (`cmp`) to `axes323`**.
2. **`axes_a1_4dof_cin`** — `t365`'s exact `a1_k0` ranking rule (`pre-polish c_in order`, `5.0°` dedup, `keep_lines(c_in, tau=1.2, kmax=3)`, **NO `L_pre` injection**), replacing 2-DoF `refine_axis_profile_tilt` with **4-DoF `refine_axis_profile`** + `orient_base_axis`:
   - In Ticket 366, `a366_4dof` injected pre-polish `L_pre` at `min(c_in, 1.05 * c_ref)` and raised `tau` to `1.25` using `c_ref`, flooding Pot A Piece 06/07/08 to `K=3` (admitting `35°/41°` outliers on Piece 08) and Juglet to `CANDTOTAL = 204` (`11/18 -> 8/18` edges).
   - Keeping `c_in` order and `keep_lines(c_in, tau=1.2, kmax=3)` preserves `K = 1` on all 8 Pot A pieces (`P01..05` `0.34°–4.07°`, tighter than 2-DoF `a1_k0`'s `0.46°–5.37°`) while keeping 4-DoF's tighter tilts on Juglet (`P5 6.00° -> 1.06°`, `P8 5.99° -> 3.80°`, `P4 10.32° -> 7.40°`, `P2 20.43° -> 15.73° [cand1=9.51°]`, `P7 4.96° -> 4.44°`).
3. **`axes_a1_4dof_k1`** — column 0 (`kmax=1`, `K=1` rank-0 line only on all 9 pieces) of `axes_a1_4dof_cin`:
   - Isolates whether multi-candidate `K > 1` under `c_in` (`P2` `K=2`, `P7` `K=2`, `P9` `K=3`) helps via `P2`'s `9.51°` candidate or dilutes matching vs. single-line `K=1`. (On Pot A, `a1_4dof_cin` already yields `K=1` across all 8 pieces.)

**Answers:** S1
**Blocked by:** None (Tickets 363, 364, 365, and 366 resolved 2026-11-18..19).
**Status:** ready-for-agent
**Worker:** unassigned
**Budget:** medium 5
**Hand-over:** DeepSeek — Spartan compute node (read-only over `class/`, `sfs_axes/`, `Dataset/`, and existing `runs/`). Check path: `C:/PR/artifacts/dsh-handover/367/accept.py` + lead verification of `C:/PR/artifacts/t367/axes367_summary.json`, `C:/PR/artifacts/t367/gate367.tsv`, `sacct -X` (all jobs `COMPLETED 0:0`), recomputed 9-piece & cluster Kabsch + sherd-centroid `CountResult`, and visual inspection of `C:/PR/artifacts/t367/render367_ablation.png`.

## Why this ticket exists (from Ticket 366)

- Ticket 366 established that blind `orient_base_axis` (using `sfs_axes.normalized.is_base_fragment`) flips **only Juglet Piece 7** (`175.04° -> 4.96°` on `a1_base`), reduces `SignAxisGuard` `> 90°` pairs from `4/18 (22.2%)` to `0/18 (0.0%)`, restores `4/4` Piece 7 joins, and brings single-sherd `a1_base_r2` to `9/9` sherds (`11/18` edges, `8/9 <= 4.596 mm` / `4.653 mm` origin gauge, `0.482 mm` vertex cluster), while `pota_a1_k0rig` holds `8/8, 15/15` at `2.011 mm`.
- Ticket 366 left two clean design questions unanswered before promoting any change to `SfSpp_preprocessing/sfs_axes/`:
  1. Was 2-DoF `refine_axis_profile_tilt` + `c_in` (`a1_k0`) even necessary once `orient_base_axis` fixes Piece 7, or does shipped `sfs_axes` (`tree 6a67364b`) + `orient_base_axis` alone (`shipped_base`) already assemble `9/9` Juglet?
  2. When 4-DoF `refine_axis_profile` runs **without** `L_pre` injection or `tau=1.25` bloat (`a1_4dof_cin`, and its `K=1` slice `a1_4dof_k1`), does Pot A stay `8/8, 15/15` while Juglet beats `2-DoF` `a1_base` (`4.653 mm` origin gauge / `0.482 mm` vertex cluster)?

## Part A — Prototype `t367` axis extraction and scoring (Spartan compute node)

1. Create run directory `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t367-shipped-vs-4dof-cin/` (remember: record exact git7 from `HEAD` on Spartan / laptop repo, create `work/`, `out/axes/`, `logs/`).
2. Inside `<run>/work/`:
   - Copy `src_axes` (`tree 6a67364b`) from `/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0247_4beb57f_t363-own-axes/src_axes` into `<run>/work/src_axes_367/` (no edits to tracked repos).
   - Reuse `orient_base_axis` / `write_axes_lines` from `/data/gpfs/projects/punim2657/SfSpp/runs/20261118-1435_78581fb_t366-base-sign-4dof/work/base_axis.py`.
3. Produce 5 axis sets under `<run>/out/axes/` on a Spartan compute node via `sbatch`:
   - `out/axes/axes_shipped_base/`: apply `orient_base_axis` (using `t80` clean `(S0, S1)` walls from `/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0430_d272313_t80-default-on/out/cell_def_3x/geom/`) to `t363`'s shipped `axes_def3x` (`/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0247_4beb57f_t363-own-axes/out/axes/axes_def3x/`).
   - `out/axes/axes_pota_shipped_base/`: apply `orient_base_axis` (using Pot A corpus `(S0, S1)` walls under `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Surfaces/`) to `t363`'s `axes_pota_chk_r1` (`/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0247_4beb57f_t363-own-axes/out/axes/axes_pota_chk_r1/`) and verify **`8/8` byte-identical by `cmp`** against `/data/gpfs/projects/punim2657/SfSpp/runs/20261110-0740_d628fc0_t323-axes/out/axes323/`.
   - `out/axes/axes_a1_4dof_cin/`: run `run_potsac_union(..., seeds=10, step=10, max_candidates=6, normalized=True, line_rule=True)` with `build_pool` (`60` most-drawn basins, raw + normal-refined), sorted by pre-polish `c_in`, `_distinct(..., 5.0)[:8]`, polished by **4-DoF `refine_axis_profile`** (`pf.refine_axis_profile`), keeping **pre-polish `c_in`** in `fin` (NO `L_pre` injection, NO `fin.sort()`), deduping `fin` at `5.0°`, cutting `[:6]`, filtering by **`keep_lines(c_in, tau=1.2, kmax=3)`**, then `orient_base_axis(vt, C0, C1)`.
   - `out/axes/axes_pota_a1_4dof_cin/`: identical `a1_4dof_cin` + `orient_base_axis` extraction on all 8 Pot A corpus `(S0, S1)` walls. Check and log `K` per piece (`[1,1,1,1,1,1,1,1]` expected; verify no `> 10 deg` regression on `P01..05`).
   - `out/axes/axes_a1_4dof_k1/`: exact column `0` (`vt[:, :1]`) of each `axes_a1_4dof_cin/*_Axis.xyz` file (`K=1` on all 9 pieces).
4. Score all 5 sets plus `t366`'s `axes_a1_base` into `C:/PR/artifacts/t367/axes367_summary.json`:
   - Juglet sets scored against `/data/gpfs/projects/punim2657/Dataset/Juglet_large` (`scale=3.0`); Pot A sets scored against `/data/gpfs/projects/punim2657/Dataset/Axes/Pot_A` (`scale=1.0`).
   - Per candidate: `unsigned_deg`, `signed_deg`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `z_wall_minus_z_pole`, `orient_flipped`.
   - Simulated `SignAxisGuard` (`pairs_gt_90 / 18`) for every Juglet set (must be `0/18 (0.00%)` everywhere).

## Part B — `t363` / `t366` assembly rig (`--array=0-7`, Spartan compute node)

- Reuse the exact `t363`/`t365`/`t366` container & shims:
  - Juglet shim md5 `0c216463ae27fe4571b200cc2292da99` (floor `10`), `XY_BREAK=1`, `XY_STATE=1`, `SFS_AXIS_LOCK_ICP` unset, `SFS_AXIS_SIGN=1`.
  - Pot A shim md5 `daa9d614cfa78de79584cd2057` (floor `50`), `XY_BREAK=1`, `XY_STATE=1`, `SFS_AXIS_LOCK_ICP` unset.
- **Fix the banner check in `submit367.sbatch`:** verify `xy break normals: (1|on)`, `xy state normals: (1|on)`, `axis lock icp: (0|off)` via `grep -Eq` so every completed Slurm task exits `0:0` (`COMPLETED` in `sacct -X`).
- **Slurm array `--array=0-7`:**
  - `0: juglet_shipped_base_r1` (`axes_shipped_base`)
  - `1: juglet_shipped_base_r2` (`axes_shipped_base`)
  - `2: juglet_a1_4dof_cin_r1` (`axes_a1_4dof_cin`)
  - `3: juglet_a1_4dof_cin_r2` (`axes_a1_4dof_cin`)
  - `4: juglet_a1_4dof_k1_r1` (`axes_a1_4dof_k1`)
  - `5: juglet_a1_4dof_k1_r2` (`axes_a1_4dof_k1`)
  - `6: pota_a1_4dof_cin_r1` (`axes_pota_a1_4dof_cin`)
  - `7: pota_a1_4dof_cin_r2` (`axes_pota_a1_4dof_cin`)
- **Scoring & audit (on fetched poses & logs):**
  - `score58` 9-piece / 8-piece origin-gauge Kabsch (explicitly count seated pieces against `13.788` tree = `4.596 mm` real on Juglet, `4.596 mm` on Pot A; avoid passing unscaled `4.596` if printing `seated_count`, or report true threshold explicitly in `gate367.tsv` & `axes367_summary.json`).
  - `cluster366` largest-subset & full `k=9` (`k=8` on Pot A) vertex-centroid Kabsch (`13.788` tree = `4.596 mm` real on Juglet, `4.596 mm` on Pot A).
  - Stock `CountResult` (`deg < 20.05°`, `dt_origin < 50.0`) AND **sherd-centroid `CountResult_sherd`** (`deg < 20.05°` and `||E_rel(c_j) - GT_rel(c_j)|| < 50.0` tree on Juglet / `mm` on Pot A, plus strict `< 5.0°` and `< 15.0` tree / `5.0 mm` physical seam accuracy across all `18` / `15` GT edges).
  - `CANDTOTAL`, `[SFS-335] SIGN-AXIS`, and Piece 7 upright `CAND336` / `CAND338` / accepted joins (`3-7, 6-7, 7-8, 7-9`).
  - Include `t366`'s `juglet_a1_base_r1/r2`, `juglet_oracle_xy`, and `pota_a1_k0rig` reference rows in `gate367.tsv` for direct comparison.

## Part C — Deliverables, Visual Check, and Hygiene

- Fetch to laptop `C:/PR/artifacts/t367/`:
  - `C:/PR/artifacts/t367/axes367_summary.json`
  - `C:/PR/artifacts/t367/gate367.tsv` — exact 7-column header `group\tpiece\tmetric\tvalue\treference\tverdict\tnote` with `_tree` and `_mm` unit suffixes covering Part A (`deg_unsigned`, `deg_signed`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `pota_cmp_axes323`, `sign_guard_pct`) and Part B (`CountResult_directed`, `CountResult_sherd`, `CountResult_centroid`, `9pc_Kabsch`, `cluster_Kabsch`, `CANDTOTAL`, `P7_upright_CAND`, `P7_CAND338`, `P7_accepted_joins`).
  - `C:/PR/artifacts/t367/render367_ablation.png` — 6 labelled front/top panels at `[SFS-336] FINAL`: best `juglet_shipped_base`, best `juglet_a1_4dof_cin`, best `juglet_a1_4dof_k1`, `t366` `juglet_a1_base_r2`, `t366` `juglet_oracle_xy`, and `pota_a1_4dof_cin_r1` (`PlaceErr` bridges drawn). **Inspect visually before writing `Reading:`.**
  - Fetched binary logs (`C:/PR/artifacts/t367/logs/`), scores (`C:/PR/artifacts/t367/score/`), and poses (`C:/PR/artifacts/t367/result/`).
- Append one `runs/index.tsv` row (`verdict` and `human_note` blank).
- Record every Slurm job ID & `sacct -j ... -X` in `## Comments` (must show `COMPLETED 0:0` on axis & assembly jobs), write one bare `Reading: <slug> — ...` line, and end with `**End state:** ready` (or `blocked`).

## Readings (pick one verbatim on a bare `Reading:` line)

- `4dof-cin-seats-9of9-and-holds-pota` — `pota_a1_4dof_cin` holds `8/8, 15/15` (`<= 4.596 mm`) on both repeats, `axes_pota_shipped_base` is `8/8` byte-identical to `axes323`, and `juglet_a1_4dof_cin` (and/or `juglet_a1_4dof_k1` / `juglet_shipped_base`) seats `9/9` (`<= 13.788` tree = `4.596 mm` on both vertex-centroid Kabsch and `score58` origin gauge, or improves upon `juglet_a1_base_r2`'s `4.653 mm`).
- `shipped-base-alone-sufficient` — `pota_a1_4dof_cin` and `axes_pota_shipped_base` hold, and `juglet_shipped_base` (`sfs_axes` tree `6a67364b` + `orient_base_axis` alone) equals or beats `a1_base` / `a1_4dof_cin` on Juglet (`9/9` closed, <= `a1_base_r2` max chamfer).
- `2dof-a1-base-remains-tightest` — `pota_a1_4dof_cin` (`8/8, 15/15`) and `axes_pota_shipped_base` (`8/8` `cmp`) hold, while `t366`'s 2-DoF `a1_base` (`4.653 mm` origin gauge, `0.482 mm` vertex cluster, `11/18` edges) remains tighter on Juglet than `shipped_base`, `a1_4dof_cin`, and `a1_4dof_k1`.
- `blocked: control-or-input-failed` — `pota_a1_4dof_cin` or `axes_pota_shipped_base` fails positive control, or a named input is missing.

## Out of scope

- Editing tracked files under `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`.
- Committing, pushing, merging, or running `ticket_close.sh` from the worker.

## Acceptance

- [ ] Part A: `<run>/work/` builds all 5 axis sets (`axes_shipped_base`, `axes_pota_shipped_base` `8/8` byte-identical by `cmp` vs `axes323`, `axes_a1_4dof_cin`, `axes_pota_a1_4dof_cin` with `K=1` check, `axes_a1_4dof_k1`) without touching tracked files; `axes367_summary.json` scores every candidate (`unsigned_deg`, `signed_deg`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `SignAxisGuard` = `0/18` across all Juglet sets).
- [ ] Part B: all 8 assembly runs (`--array=0-7`) complete via `sbatch` with `sacct -X` showing `COMPLETED 0:0`; scored with `score58` origin Kabsch, `cluster366` vertex Kabsch, stock & sherd-centroid `CountResult`, `CANDTOTAL`, `SIGN-AXIS`, and Piece 7 upright `CAND`/`CAND338` audit.
- [ ] Part C: `C:/PR/artifacts/t367/axes367_summary.json`, `C:/PR/artifacts/t367/gate367.tsv`, `C:/PR/artifacts/t367/render367_ablation.png` (visually inspected), `runs/index.tsv` row appended, `## Comments` complete, bare `Reading:` line, `**End state:** ready`.

## Comments

*(Worker records run dir, Slurm IDs / `sacct -X`, Part A/B tables, visual inspection of `render367_ablation.png`, and `Reading:` here.)*
