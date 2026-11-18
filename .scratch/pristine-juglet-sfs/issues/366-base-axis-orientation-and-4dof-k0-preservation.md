# 366: Base-to-rim axis sign orientation (`orient_base_axis`) + 4-DoF `k=0`-preserved profile refinement + centroid-height `d_perp_sherd_mm` on `def_3x` Juglet

**What to build:** a **measurement and prototype ablation run** (zero edits to tracked `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`; all prototype axis logic lives strictly in the Spartan run directory's `<run>/work/src_axes_366/` and driver scripts) testing two targeted axis-extractor fixes established from Tickets 364–365 and the SfS++ paper (`papers/text/sfspp-2502.13986v1.md`) & MATLAB code (`AxisExtraction/check_base_and_rim.m`, `sfs_axes/normalized.py`, `sfs_axes/select.py`, `sfs_axes/profile.py`):

1. **Gap 1 — Blind Base-to-Rim Sign Orientation (`orient_base_axis`):**
   - In `Dataset/Juglet_large`, Pieces `1..6, 8, 9` have `GT dir_z > 0`, whereas Base **Piece 7** (`r_min = 0.07 mm`, `28.8%` of surface points with `|n . a| > 0.85`) has `GT dir_z = -0.670 < 0`.
   - PotSAC (`pottmann.py`) returns an undirected line (`+a` and `-a` have identical normal/profile cost), and canonicalizing `dir_z >= 0` leaves Piece 7 pointing **rim-to-base** (`-z_vessel`), 180 deg opposite to all 8 non-base sherds. Because `Piece 7` owns `4/18 = 22.22% <= 0.25` of GT edges, `SignAxisGuard` (`reconstruction.cpp:116`) accepted `SFS_AXIS_SIGN=1` (`22.2222% > 90 deg`), causing `pair_axis` (`feature_matching.cpp:1937,2006`) to drop 100% of upright Piece 7 joins (`(5,7)` 81 inliers `raw=176.6 deg / folded=3.4 deg`, `(6,7)` 40 inliers `168.2 / 11.8 deg`, `(7,9)` 18 inliers `179.1 / 0.9 deg`).
   - **Blind fix:** in `<run>/work/src_axes_366/`, add `orient_base_axis(vt, C0, C1=None)` (using **zero GT**): for every candidate column `k` of `vt`, compute `C_all = C0 if C1 is None else np.concatenate([C0, C1], axis=1)`. If `normalized.is_base_fragment(C_all, v_k)` is `True`, compute signed axial coordinates `z = a @ (C_all[0:3] - p0[:, None])` and `n_ax = np.abs(a @ C_all[3:6])` on base-cap points (`n_ax > 0.85`, with fallback top 10% of `n_ax` if fewer than 50 points) and wall points (`n_ax < 0.50`, with fallback bottom 50% of `n_ax`): if `mean(z_wall) < mean(z_pole)`, negate `a = -a` so the vector points **base -> rim** (`+z_vessel`). Non-base fragments keep `dir_z >= 0` canonicalization.

2. **Gap 2 — Centroid-Height Pivot Offset Metric (`d_perp_sherd_mm`):**
   - Because Juglet scans sit `||c_mean|| ~ 1,600` tree units (`533 mm`) from `(0, 0, 0)`, `io_axes.radial_axial_errors` measures `d_perp` at the coordinate origin (`(273-528 mm) * sin(tilt)` = `7-166 mm` lever-arm amplification).
   - Measure and report **both** `d_perp_origin_mm` (`tree / 3.0`) AND `d_perp_sherd_mm`: project `c_mean` onto both the estimated axis (`p_sherd = p0 + ((c_mean - p0) . d) d`) and GT axis, and compute perpendicular distance `/ 3.0` (which is what `AxisAlignment` actually sees after subtracting `Q_z_mean`).

3. **Gap 3 — 4-DoF `k=0`-Preserved Profile Refinement (`a366_4dof`) vs `a1_k0` 2-DoF (`a1_base`) & `b2_pair` (`b2_base`):**
   - Locking `refine_axis_profile` to `2-DoF` (`a1_k0` in Ticket 365) degraded `P5` (`1.06 -> 6.00 deg`), `P8` (`3.80 -> 5.99 deg`), `P4` (`7.40 -> 10.32 deg`), `P2` (`15.73 -> 20.43 deg`), and `P7` (`4.44 -> 4.96 deg`) because `foot` could not shift axially with the tilted cone/wall center of curvature.
   - Test **`a366_4dof`** (4-DoF `refine_axis_profile` + `k=0` `cost_in` order preserved + pre-polish retention when 4-DoF shifts `>= 4.0 deg` + `5.0 deg` dedup + `keep_lines(tau=1.25, kmax=3)` + `orient_base_axis`), **`a1_base`** (`t365` `a1_k0` + `orient_base_axis`), **`b2_base`** (`t365` `b2_pair` + `orient_base_axis`), and **`pota_a366_4dof`** (Pot A positive control).

**Answers:** S1
**Blocked by:** None (Tickets 363, 364, and 365 resolved 2026-11-18).
**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-19)
**Branch:** 366-base-axis-orientation-and-4dof-k0-preservation, in /c/PR/worktrees/structure-from-sherds-pp/366
**Worker:** dsh one-shot subagent w366
**Budget:** medium 5
**Hand-over:** DeepSeek — Spartan compute node (read-only over `class/`, `sfs_axes/`, `Dataset/`, and existing `runs/`). Check path: `C:/PR/artifacts/dsh-handover/366/accept.py` + lead verification of `C:/PR/artifacts/t366/axes366_summary.json`, `C:/PR/artifacts/t366/gate366.tsv`, `sacct -X`, recomputed 9-piece & largest-cluster Kabsch, and visual inspection of `C:/PR/artifacts/t366/render366_ablation.png`.

## Why this ticket exists (verified in Tickets 363–365 + Paper vs Code audit)

- In Ticket 365, `juglet_a1_k0_r2` / `juglet_a2_pool_r2` assembled `8/9` sherds (`{1, 2, 3, 4, 5, 6, 8, 9}`) with `7/8` seated `<= 4.58 mm` real (`13.788` tree bar), and `juglet_b2_pair_r1` seated **all `8/8` (`1.93–4.28 mm` real — tighter than `juglet_oracle_xy`'s `4.54 mm` max)** — leaving **only Base Piece 7** detached (`rot = 174–179 deg`).
- Paper-vs-code inspection (`papers/text/sfspp-2502.13986v1.md` §IV-B2, §IV-C1, §IV-D3, Table III; `AxisExtraction/check_base_and_rim.m`; `sfs_axes/normalized.py`; `class/data_structure.cpp`; `class/feature_matching.cpp`; `class/reconstruction.cpp`) established that:
  1. The SfS++ paper performs no base-specific step in PotSAC (`isbase` / `isrim` are post-axis classifiers, and `#define NO_BASE_INFO` is active in assembly); rim masking (`std(h)<=1mm, std(r)<=1mm`) would misclassify Juglet's horizontal mid-body fracture seams, so `isrim=0` is intentional.
  2. `sfs_axes/normalized.py:125` already implements `is_base_fragment(X, vt)` which isolates **Piece 7 alone** (`True` on P7, `False` on P1..P6, P8, P9), but `select_union_lines` (`select.py`) ignores it and leaves Piece 7's base-to-rim vector inverted relative to the remaining 8 sherds under `SFS_AXIS_SIGN=1`.
  3. At sherd centroid height (`d_perp_sherd_mm`), `a1_k0` already has `d_perp_sherd <= 1.25 mm` on 8/9 pieces (`P9 = 4.57 mm`), and `b2_pair` has `0.38–1.03 mm` on all 9 pieces, meaning direction accuracy (`4-DoF` + `k=0` preservation) and base sign consistency (`orient_base_axis`), not origin lever-arm `d_perp_origin_mm`, govern full 9/9 Juglet assembly.

## Part A — Prototype `src_axes_366` extraction, `orient_base_axis`, and `d_perp_sherd_mm` audit

1. Create run dir `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t366-base-sign-4dof/` and copy `/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0247_4beb57f_t363-own-axes/src_axes` (`d272313`, `tree 6a67364b`) into `<run>/work/src_axes_366/`. **Zero edits to tracked files under `structure-from-sherds-pp/`, `SfSpp_preprocessing/`, or `Dataset/`.**
2. Implement two clean prototype helpers inside `<run>/work/` (loaded via driver / run-dir wrapper over `src_axes_366`):
   - **Blind `orient_base_axis(vt, C0, C1=None)`:**
     - Using `C_all = C0 if C1 is None else np.concatenate([C0, C1], axis=1)` (deterministic stride `<= 30,000` points if huge):
     - For each candidate column `k`: check `is_base = normalized.is_base_fragment(C_all, v_k)`. If `is_base`, compute `z = a @ (C_all[:3] - p0[:, None])` and `n_ax = np.abs(a @ C_all[3:6])`. Take base-cap mask `n_ax > 0.85` (fallback top 10% of `n_ax` if count `< 50`) and wall mask `n_ax < 0.50` (fallback bottom 50% of `n_ax`). Record `z_diff = float(np.mean(z[wall_mask]) - np.mean(z[base_mask]))`; if `z_diff < 0`, flip `v_k[0:3] = -a` (keeping `v_k[3:6] = p0` orthogonal to `a`). If not `is_base`, canonicalize non-base `v_k[0:3]` so `v_k[2] >= 0` (flip sign if `v_k[2] < 0`).
   - **`a366_4dof` ranking/refinement (derived from `tree 6a67364b`):**
     - Run `run_potsac_union(..., seeds=10, step=10, max_candidates=6, normalized=True, line_rule=True)` with `select.select_union_lines` / `select.rank_by_profile` modified **only in `<run>/work/src_axes_366/`** as follows:
       - Keep **full 4-DoF `refine_axis_profile(v, walls, centre, size)`** on `top = _distinct([v for _, v in pool], lambda v: v, 5.0)[:8]`.
       - Record polished list `fin` **in `top`'s pre-polish `cost_in` order**: for each `(v_in, c_in)` with polished `(v_ref, c_ref)`: emit `(c_ref, v_ref)`; if `direction_angle_deg(v_ref[:3], v_in[:3]) >= 4.0`, ALSO emit `(min(c_in, c_ref * 1.05), v_in)` immediately after `(c_ref, v_ref)` so normal-refined `k=0` lines like `P4` (`2.73 deg`) are never lost when 4-DoF walks away.
       - Preserve `(c_ref, v_ref)` at `index 0` in **`cost_in` order** (`fin = [fin[0]] + sorted later items` OR pure `cost_in` order? Preserve pure `cost_in` order across `fin`, deduping via `_distinct(fin, lambda x: x[1], 5.0)` so `k=0` stays rank 0 and no far-start candidate jumps above and deletes it!).
       - Apply `keep_lines(c, tau=1.25, kmax=3)`, then apply `orient_base_axis(vt, C0, C1)`.
3. Produce four axis directories on a Spartan compute node (via `sbatch`):
   - `out/axes/axes_a1_base/`: apply `orient_base_axis` (using `t80` clean `(S0, S1)` walls at `/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0430_d272313_t80-default-on/out/cell_def_3x/geom/`) to `t365`'s `axes_a1_k0` (`/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0242_308c771_t365-k0-preservation-postjoin/out/axes/axes_a1_k0/`).
   - `out/axes/axes_a366_4dof/`: run `a366_4dof` + `orient_base_axis` on all 9 Juglet `t80` `def_3x` `(S0, S1)` walls.
   - `out/axes/axes_b2_base/`: apply `orient_base_axis` (using `t80` `(S0, S1)` walls) to `t365`'s `axes_b2_pair` (`/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0242_308c771_t365-k0-preservation-postjoin/out/axes/axes_b2_pair/`).
   - `out/axes/axes_pota_a366_4dof/`: run `a366_4dof` + `orient_base_axis` on all 8 Pot A corpus `(S0, S1)` walls.
4. Score all four axis sets against GT (`Dataset/Juglet_large` with `scale=3.0` and `/data/gpfs/projects/punim2657/Dataset/Axes/Pot_A/` with `scale=1.0`), saving everything to `C:/PR/artifacts/t366/axes366_summary.json`:
   - For every piece and every kept candidate `k`: `unsigned_deg`, **`signed_deg`**, `d_perp_origin_mm`, **`d_perp_sherd_mm`**, `is_base_fragment`, `z_wall_minus_z_pole`.
   - Compute simulated `SignAxisGuard` on each Juglet axis set (`axes_a1_base`, `axes_a366_4dof`, `axes_b2_base`): `pairs_gt_90 / 18`, which **must be `0 / 18 (0.0%)`**.

## Part B — `t363` assembly rig runs on Spartan compute node

- Reuse the exact `t363` / `t365` container & shims (Juglet floor 10 `0c216463ae27fe4571b200cc2292da99`; Pot A floor 50 `daa9d614cfa78de79584cd2057`), `XY_BREAK=1`, `XY_STATE=1`, `SFS_AXIS_LOCK_ICP` unset, `SFS_AXIS_SIGN=1` (enabled as in `t363`/`t365` for Juglet).
- **Slurm array `--array=0-7`:**
  - `0: juglet_a1_base_r1` (`axes_a1_base`)
  - `1: juglet_a1_base_r2` (`axes_a1_base`)
  - `2: juglet_a366_4dof_r1` (`axes_a366_4dof`)
  - `3: juglet_a366_4dof_r2` (`axes_a366_4dof`)
  - `4: juglet_b2_base_r1` (`axes_b2_base`)
  - `5: juglet_b2_base_r2` (`axes_b2_base`)
  - `6: juglet_oracle_xy` (positive control, must seat `9/9` at `13.788` tree = `4.596 mm` bar; allow 1 extra repeat if P2 stochasticity hits `8/9` once)
  - `7: pota_a366_4dof` (positive control on Pot A: must hit `8/8, 15/15` at `4.596 mm` bar)
- Score every arm with `score58` (`13.788` tree = `4.596 mm` for Juglet; `4.596 mm` for Pot A), reporting:
  - **9-piece full Kabsch** (`seated / 9`, `max_chamfer_tree`, `max_chamfer_mm`),
  - **Largest-cluster-only Kabsch** (`cluster_pieces`, `cluster_seated / len`, `cluster_max_chamfer_mm`),
  - `CountResult` sherd & edge accuracy, `[SFS-335] SIGN-AXIS` banner line, **Piece 7 upright `[SFS-336] CAND` & `[SFS-338] CAND338` counts** (checking **both** `y==7` and `x==7` and reading column `rot = p[k+8]`, NOT `mat = p[k+9]`!), and accepted Piece 7 joins.

## Part C — Deliverables, Visual Check, and Hygiene

- Fetch to laptop `C:/PR/artifacts/t366/`:
  - `C:/PR/artifacts/t366/axes366_summary.json`
  - `C:/PR/artifacts/t366/gate366.tsv` — exact 7-column header `group\tpiece\tmetric\tvalue\treference\tverdict\tnote` with explicit `_tree` and `_mm` unit suffixes covering Part A (`deg_unsigned`, `deg_signed`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `sign_guard_pct`) and Part B (`CountResult`, `9pc_Kabsch`, `cluster_Kabsch`, `P7_upright_CAND`, `P7_CAND338`, `P7_accepted_joins`).
  - `C:/PR/artifacts/t366/render366_ablation.png` — 6 labelled panels at `[SFS-336] FINAL`: best `juglet_a1_base`, best `juglet_a366_4dof`, best `juglet_b2_base`, `t365` `juglet_b2_pair_r1` (before base sign fix, showing P7 detached) or second repeat, `juglet_oracle_xy`, and `pota_a366_4dof` (`PlaceErr` bridges drawn). **Inspect visually before writing `Reading:`.**
  - Fetched binary logs (`C:/PR/artifacts/t366/logs/`) and score JSONs (`C:/PR/artifacts/t366/score/`).
- Append one `runs/index.tsv` row (`verdict` and `human_note` blank).
- Record every Slurm job ID & `sacct -j ... -X` in `## Comments`, write one bare `Reading: <slug> — ...` line, and end with `**End state:** ready`.

## Readings (pick one verbatim on a bare `Reading:` line)

- `base-sign-fix-seats-9of9-juglet` — fixing Base Piece 7's sign via blind `orient_base_axis` gives `0/18 (0.0%)` `> 90 deg` pairs and `juglet_b2_base` (and/or `juglet_a366_4dof` / `juglet_a1_base`) seats **9/9** (`4.596 mm` bar, `<= 13.788` tree) on full 9-piece Kabsch while `juglet_oracle_xy` (`9/9`) and `pota_a366_4dof` (`8/8, 15/15`) hold.
- `base-sign-admits-p7-partial-seating` — `orient_base_axis` restores `0/18` `> 90 deg` pairs and Piece 7 upright `CAND` / `CAND338` candidates, seating Piece 7 into the assembly or improving `juglet_a366_4dof` / `juglet_a1_base` / `juglet_b2_base`, while still stopping short of `9/9` at `13.788` tree (exact failure stage on remaining piece(s) recorded).
- `base-sign-insufficient-p7-rejected-downstream` — `orient_base_axis` flips Piece 7 cleanly (`0/18 > 90 deg`), yet Piece 7 still fails to join downstream (state exact gate `[SFS-336]`/`[SFS-339]`/`[SFS-341]` or pose reason).
- `blocked: control-or-input-failed` — `pota_a366_4dof` or `juglet_oracle_xy` fails positive control, or a named input is missing.

## Out of scope

- Editing tracked files under `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`.
- Committing, pushing, merging, or running `ticket_close.sh` from the worker.

## Acceptance

- [ ] Part A: `<run>/work/src_axes_366/` implements blind `orient_base_axis` and `a366_4dof` (`4-DoF` + `cost_in` order + pre-polish retention `>= 4 deg` + `5 deg` dedup + `keep_lines(1.25, 3)`); extracts `axes_a1_base`, `axes_a366_4dof`, `axes_b2_base`, and `axes_pota_a366_4dof` on Spartan compute node; scores `unsigned_deg`, `signed_deg`, `d_perp_origin_mm`, `d_perp_sherd_mm`, and verifies `0/18 (0.0%)` `> 90 deg` pairs across all three Juglet sets (`axes366_summary.json`).
- [ ] Part B: all 8 assembly runs (`--array=0-7`: `juglet_a1_base_r1/r2`, `juglet_a366_4dof_r1/r2`, `juglet_b2_base_r1/r2`, `juglet_oracle_xy`, `pota_a366_4dof`) complete via `sbatch` with `sacct -X` recorded; scored with 9-piece & largest-cluster Kabsch, `CountResult`, `SIGN-AXIS`, and fixed Piece 7 upright `CAND`/`CAND338` audit.
- [ ] Part C: `C:/PR/artifacts/t366/axes366_summary.json`, `C:/PR/artifacts/t366/gate366.tsv`, `C:/PR/artifacts/t366/render366_ablation.png` (viewed visually), `runs/index.tsv` row appended, `## Comments` complete, bare `Reading:` line, `**End state:** ready`.

## Comments
