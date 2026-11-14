# Ticket 357 — Does locking the candidate axis at `iter = 0` and removing vertical breakline-normal bevel bias recover the Juglet pair stage?

**Status:** ready-for-agent
**Branch:** to be created at dispatch
**Worker:** unassigned
**Answers:** S1
**Needs-eye:** yes
**Depends on:** 355 (11/14 `LCS` seams on target `frac = 1.000`, yet `0/14` in bar at `iter = 0` and `4/14` at `final`), 348 (`14` measured joins + `4-7, 4-6` controls), 350 (`G_axis` frame & `rot / tilt / spin / mm` decomposition)
**Blocked-by:** 355 (landed at `ee55ce2`)
**Budget:** medium (two small default-off switches in `class/`, one Spartan `sbatch` 2×2 factorial array on `Juglet` + `Pot_A`, one offline audit script + render in `C:/PR/artifacts/t357/`)
**Hand-over:** DeepSeek: deterministic GT normal audit (`gt_normal_audit357.tsv`), two default-off switches in `class/` (`SFS_AXIS_LOCK_ICP`, `SFS_XY_BREAK_NORMALS`), 2×2 Slurm factorial on `Juglet` + `Pot_A` control, per-join table (`factorial357_juglet.tsv`, `factorial357_summary.tsv`), and visual render (`render357_hinge_vs_fixed.png`).

> **In one line.** Lead verification on ticket 355 proved that `UsePreCorres` is **not** spin-free on the pot axis — fitting stock `LCS` dot pairs in `2-DoF` around `+z` puts **10 of 14 Juglet joins inside `5 deg / 5 mm` at `iter = 0`** — and exposed why stock `PairwisePruning` loses them: `axis = false` at `iter = 0` lets opposing vertical (`N_z`) boundary-bevel normals (`18.2 deg` median / up to `34.9 deg` at GT vs `3.3 deg` on `Pot_A`) hinge the sherds by `9–33 deg` off-axis AND fail `CountInlier`'s `15 deg` gate even at ground truth (`5/14` joins rejected at GT, `0/14` failing in `(x, y)`).

## Why this follows directly from ticket 355

Ticket 355 (`C:/PR/artifacts/t355/precor355_juglet.tsv`, `precor355_pota.tsv`) established that `FeatureComp / LCS` already locates the right breakline patch (`lcs_overlap_frac = 1.000`, midpoint shift `0.46–3.92 mm`) on **11 of the 14 measured Juglet joins**, missing the seam itself on only **3 joins** (`1-3` `0.000 / 12.98 mm`, `3-5` `0.500 / 14.90 mm`, `6-7` `0.500 / 27.13 mm`). Yet stock `Icp` scored **0 of 14** joins inside `5 deg / 5 mm` at `iter = 0` and only **4 of 14** at `final` (`2/9 sherds, 1/18 edges`), whereas `Pot_A` started with `6/15` at `iter = 0`, ended with `10/15`, and assembled `8/8 sherds, 15/15 edges`.

Post-land verification by the lead against `C:/PR/artifacts/t355/logs/binary_juglet_signed_on_r1.log`, `class/reconstruction.cpp`, `class/reconstruction.h`, and `class/feature_matching.cpp` separated two compounding mechanisms that ticket 355's trace blurred together:

1. **`UsePreCorres` (`iter = 0`) is NOT spin-free around the pot axis; stock `iter = 0` loses the pose to an off-axis hinge (`tilt0 = 9.16–33.25 deg`).**
   - Unlike whole-breakline `MakeCor` + `P2LConstraint` (`iter = 1..49`), `UsePreCorres` (`MakeSection`, `class/reconstruction.cpp:237-254, 1102`) builds a **1-to-1 point-to-point** pairing between `Py[sy-1..ey-1]` and reversed `Px[[nx-i for i in range(sx, ex+1)]]`. When both pieces' aligned `+z` pot axes passing through `(0, 0)` are held fixed (`2-DoF`: `spin` about `+z` + vertical shift `dz` along `+z`, zero `x/y` shift and zero tilt — no oracle, no grid search), fitting those **exact stock `LCS` dot pairs** lands **10 of the 14 measured Juglet joins inside `5 deg / 5 mm` at `iter = 0`** (`1-2` `0.26 deg / 0.76 mm`, `1-4` `2.52 / 0.72`, `1-8` `0.38 / 0.73`, `1-9` `0.33 / 0.66`, `2-9` `3.19 / 0.97`, `3-8` `2.23 / 0.57`, `5-6` `2.95 / 3.31`, `5-7` `1.08 / 0.43`, `5-8` `2.36 / 0.64`, `6-9` `2.04 / 0.94`; median `2.13 deg / 0.72 mm`), spanning **all 9 of 9 sherds**. Allowing free `(dx, dy)` in the local patch (`4-DoF`) drops that yield because the radius `R` lever arm to the pot axis is lost over a 7–14 dot segment.
   - Stock `Icp` sets `axis = false` at `iter = 0` (`class/reconstruction.cpp:1258`) and uses weak `w_a = 0.1` against `w_n = 3.0` (`:1242`) at `iter = 1..49`. Consequently, stock `iter = 0` hinges the sherds off-axis (`1-4` `tilt0 = 33.25 deg` while `spin0 = -1.69 deg`, `2-6` `29.84 deg`, `1-9` `27.65 deg`, `2-9` `15.55 deg`, `1-2` `13.47 deg`, `6-9` `11.59 deg`, `5-8` `11.40 deg`, `3-8` `9.16 deg`), and even at `final`, `1-9` (`spinF = -1.89 deg, tiltF = 8.62 deg`) and `5-8` (`spinF = +1.71 deg, tiltF = 6.79 deg`) miss the `5 deg` bar purely from residual tilt.
2. **Opposing vertical (`N_z`) boundary-bevel bias in `Juglet_Piece_X_Breakline_0.pcd` normals drives BOTH the hinge in `CostFuncNorm` AND gate rejection in `CountInlier` even at exact ground truth.**
   - Evaluated across mutual nearest neighbours (`< 1.5 mm` real) at the **exact ground-truth pose (`0.00 deg, 0.00 mm`)**:
     - On `Pot_A`, 3D normals agree to `1.0–8.4 deg` (median `3.3 deg`), and **15 of 15 joins** pass `CountInlier` (`min(ang, 180-ang) < 0.262 rad = 15.0 deg`, `>= 6` inliers). Horizontal `(N_x, N_y)` projections also pass **15 of 15** (`medXY = 5.0 deg`).
     - On the `Juglet`, the horizontal `(N_x, N_y)` normals at GT have folded median `5.1 deg` and **14 of 14 joins pass `CountInlier`**, **but `N_z` from the `5 mm` boundary patch tilts oppositely into the upper vs lower bevelled fracture edge** (`1-4`: `N_z = -0.370` vs `+0.222 -> 34.9 deg`; `2-6`: `-0.271` vs `+0.221 -> 30.5 deg`; `1-9`: `-0.106` vs `+0.242 -> 26.9 deg`; `1-3` `19.8 deg`, `1-2` `19.2 deg`, `3-5` `18.5 deg`, `3-8` `17.9 deg`, `5-8` `14.1 deg`; `14`-join unfolded 3D median `18.2 deg`, folded `16.0 deg`; plus `6-7` shows opposite-sign radial normals `+0.36` vs `-0.51`, `175.6 deg` unfolded / `4.4 deg` folded). Because of that `18–35 deg` opposing `N_z` bevel bias, `CostFuncNorm` (`w_n = 3.0`) hinges the unconstrained `iter = 0` solve off-axis AND `CountInlier` (`ANGLE_THRESHOLD = 0.262 rad`) rejects **5 of the 14 Juglet joins even at exact ground truth** (`1-4` `0/19`, `2-6` `1/26`, `1-9` `5/17`, `1-3` `5/22`, `3-5` `4/10` -> `score = 11.0`).

What we must test cleanly in a controlled **2×2 factorial** is whether:
- locking the candidate axes `(axis_point_[axis_index], axis_norm_[axis_index])` from `iter = 0` through `iter = 49` of `Icp` (`SFS_AXIS_LOCK_ICP=1`), AND/OR
- comparing breakline normals orthogonally to the candidate axis (dropping vertical `N_z` bevel bias while keeping `R_A, R_B` 3D rotation, `SFS_XY_BREAK_NORMALS=1`) in `CostFuncNorm`, `RejectOutlier`, and `CountInlier`,
unlocks both the `Juglet` pair-stage yield AND end-to-end assembly without regressing `Pot_A` (`8/8 sherds, 15/15 edges`).

---

## Part A — Ground-Truth Seam Normal Audit & Stock Anchor Reproduction

1. **Deterministic script `C:/PR/artifacts/t357/audit_normals357.py`:**
   - Using the existing `tree/juglet` and `tree/pota` inputs under `C:/PR/artifacts/t355/tree/` (copied or referenced read-only; never alter `C:/PR/artifacts/t355/`), evaluate every join (`14` measured Juglet joins + `2` controls `4-7, 4-6`, and all `15` `Pot_A` GT joins) placed at the **exact ground-truth pose (`0.00 deg, 0.00 mm`)** in the axis-aligned reference frame of piece `1` (`T_i = axis[1] * GT[1]^-1 * GT[i]`).
   - Find mutual nearest neighbours `< 1.5 mm` real (`4.5` tree units) between `p_1` and `p_2`, normalize `n_1, n_2`, compute both folded (`min(a, 180-a)`) 3D normal angles and folded horizontal `(N_x, N_y)` angles (plus `mean_nz_A`, `mean_nz_B`, `med_ang3d_unfolded`, `med_ang3d_folded`, `med_ang_xy_folded`, `inl_3d_lt15`, `inl_xy_lt15`, `pass_3d_ge6`, `pass_xy_ge6`).
   - Write `C:/PR/artifacts/t357/gt_normal_audit357.tsv` (**31 rows**: `16` Juglet + `15` `Pot_A`).
   - **Mandatory check:** confirm `Pot_A` passes `15/15` in 3D (`med_ang3d_folded` median `~3.3 deg`) and `15/15` in `(x, y)`, while Juglet passes `9/14` in 3D (failing `1-4`, `2-6`, `1-9`, `1-3`, `3-5`) and `14/14` in `(x, y)`.

2. **Stock baseline anchors:**
   Re-run stock `arm00_stock` (`SFS_AXIS_LOCK_ICP=0 SFS_XY_BREAK_NORMALS=0`, with `SFS_SIGNED_K2=1` on Juglet and default on `Pot_A`, plus `SFS_PRECOR_TRACE=1 SFS_STAGE_TRACE=1 SFS_EDGE_TRACE=1 SFS_STATE_TRACE=1`) to anchor the exact baseline in this binary (`Juglet`: `180` candidates, `0/14` at `iter = 0`, `4/14` measured joins at `final`, `2/9 sherds, 1/18 edges`; `Pot_A`: `124` candidates, `6/15` at `iter = 0`, `10/15` at `final`, `8/8 sherds, 15/15 edges`).

---

## Part B — Two Default-Off Switches & 2×2 Factorial (`class/` + Spartan Slurm)

Add **two** default-OFF switches in `InitSfsSwitches()` (`class/feature_matching.cpp`, declared in `class/feature_matching.h` / `class/reconstruction.h`) printed in the startup banner alongside existing switches (`axis lock icp: on/off`, `xy break normals: on/off`). With **both switches OFF**, behaviour and outputs must be **bit-identical** to HEAD `ee55ce2`. `class/` must remain strictly GT-free.

### Switch 1: `SFS_AXIS_LOCK_ICP=1` (candidate-axis-locked `Icp` from `iter = 0`)

Why candidate-axis-aware locking (NOT naive `+z`-only locking) is mandatory: on `Pot_A`, `Piece 5`'s axis points upside-down (`-z`, `178 deg` flipped) and `Piece 7` has `3` candidate axes where valid joins (`1-7, 4-7, 6-7`) use a secondary candidate axis tilted `~120 deg` from axis `0`! Using each candidate's own stored axes from `lcs = m_LCS[cycle.edges[0]]` handles **both** single-axis signed `Juglet` **and** multi-axis / unsigned `Pot_A` cleanly:

Let `(o_y, u_y)` = `(L[shard_y_-1].axis_point_[lcs.axis_index_y_], L[shard_y_-1].axis_norm_[lcs.axis_index_y_].normalized())` and `(o_x, u_x)` = `(L[shard_x_-1].axis_point_[lcs.axis_index_x_], L[shard_x_-1].axis_norm_[lcs.axis_index_x_].normalized())`.

1. **Closed-form `2-DoF` initializer at `iter = 0` (`pre_cor` + `UsePreCorres`, before Ceres):**
   - For allowed axis sign(s) (`s_sign in {+1}` when `g_sfs_axis_sign` is true; `s_sign in {+1, -1}` when legacy unsigned axis is active):
     - Let `R_al` be the minimum-angle rotation aligning `s_sign * u_x` to `u_y`, shifting `o_x` to `o_y`.
     - Using the `COR[0]` `UsePreCorres` point pairs `(p_A, p_B)`: let `p_B_rot = R_al * (p_B - o_x)`, `p_A_rel = p_A - o_y`, and project both onto the plane orthogonal to `u_y`:
       `q_perp = p_A_rel - (p_A_rel . u_y) * u_y`, `r_perp = p_B_rot - (p_B_rot . u_y) * u_y`.
     - Closed-form spin angle around `u_y`: `theta = atan2(sum((r_perp x q_perp) . u_y), sum(r_perp . q_perp))`, giving `R_cand = AngleAxis(theta, u_y) * R_al`.
     - Closed-form axial shift along `u_y`: `dz = mean((p_A - (R_cand * (p_B - o_x) + o_y)) . u_y)`, giving `t_cand = o_y + dz * u_y - R_cand * o_x`.
     - Pick the `s_sign` minimizing `sum ||p_A - (R_cand * p_B + t_cand)||^2` over `COR[0]` (`s_sign_best`), and initialize `R_i[shard_x_-1] = R[shard_x_-1] = R_cand`, `t_i[shard_x_-1] = t[shard_x_-1] = t_cand` (with `EdgeLineMove` / local frame update matching `Icp`'s convention: note in `Icp`, `s` and `trans` are incremental updates applied to `p_A, p_B`, or initialized in `R, t` / applied via `EdgeLineMove` on `L[shard_x_-1]` before Ceres — take care that `PrintPrecorTrace` at `iter == 0` prints the post-`iter=0` `M0` accurately in `R_fix_inv * R_mov`!).
2. **Candidate-axis lock during Ceres solves (`iter = 0..49` in `Icp`):**
   - Keep `shard_y_` fixed (`true_node` / fixed parameter block `SetParameterBlockConstant` on `s[shard_y_-1]` and `trans[shard_y_-1]`) OR add a strong `CandidateAxisLockFunctor` tying `(R_A * o_y + t_A, R_A * u_y)` to `(R_B * o_x + t_B, s_sign_best * R_B * u_x)` in direction (`w_axis_dir = 1e3`, `residual[0..2] = w * ((R_A u_y) - s_sign_best * (R_B u_x))`) and lateral axis-line offset orthogonal to `(R_A u_y)` (`w_axis_pos = 10.0`, `d_ax = (R_B o_x + t_B) - (R_A o_y + t_A)`, `residual[3..5] = w * (d_ax - (d_ax . (R_A u_y)) * (R_A u_y))`), without constraining axial spin (`theta` around `u_y`) or axial height (`dz` along `u_y`).
   - Run this `CandidateAxisLockFunctor` from `iter = 0` through `iter = 49` (using `loss = nullptr` i.e. L2, no Cauchy cutoff so large initial tilts are never soft-ignored, and `scale` cleanly in `M0` at `iter = 0`).
   - Log a concise banner at startup and verify `[SFS-355] PRECOR` still prints exactly once per candidate at `iter = 0` with `tilt0_deg ~ 0` on single-axis Juglet!

### Switch 2: `SFS_XY_BREAK_NORMALS=1` (compare breakline normals perpendicular to the aligned axis)

When `SFS_XY_BREAK_NORMALS=1`, remove vertical (`N_z` / along-axis) boundary-bevel bias in all three normal consumers of `PairwisePruning` (while leaving raw stored `.pcd` files and geometry unchanged):

1. **`CostFuncNorm` (`class/reconstruction.h:517`):**
   Compute `diff = (R_A * n_A) - (R_B * n_B)`, and zero out its component along the fixed piece's aligned axis `u_ref` (`u_y`, i.e., `(0, 0, 1)` when `axis_index_y_ == 0`):
   `residual = weight_n * (diff - diff.dot(u_ref) * u_ref)`.
   *(Why not normalize `n_A, n_B` to `(n_x, n_y, 0)` before rotation? Because on `Pot_A` `1-5` / `1-7`, `n_B` starts rotated 120–180 deg before `R_B` aligns it, and flat rims with `N_z = -0.99` should keep small horizontal normal weight rather than amplifying noise by `1/0.14`.)*
2. **`RejectOutlier` (`class/reconstruction.cpp:203-223`, called at `:1299` and `:1414`):**
   Project `n_A` and `n_B` orthogonal to `u_ref`: `na_p = n_A - n_A.dot(u_ref)*u_ref`, `nb_p = n_B - n_B.dot(u_ref)*u_ref`. If both `||na_p|| > 0.1` and `||nb_p|| > 0.1`, use `na_p.normalized().dot(nb_p.normalized()) > angle_TH` (`0.7`); otherwise fall back to 3D `n_A.dot(n_B)`.
3. **`CountInlier` (`class/feature_matching.cpp:1234-1331`) & `MatchingScore` (`:1185-1232`) in pair stage:**
   Using the same orthogonal-to-`u_ref` projection (`||n_perp|| > 0.1`, normalized) before `acos(...)` and `min(angle, 3.14159 - angle) < angle_th` (`0.262 rad = 15.0 deg`) inside `PairwisePruning` / `Icp` (guard with a scoped flag or pass `u_ref = L[shard_y_-1].axis_norm_[lcs.axis_index_y_]` normalized so `Registration` / `MultiRegistration` can also be kept clean or tested consistently — document clearly whether `SFS_XY_BREAK_NORMALS` applies to `Icp` alone vs `Icp` + `Registration`!).

### 2×2 Factorial Arms (Spartan Slurm `sbatch`, private tree copy per arm)

Verify `SOURCE_COMMIT`, build on Spartan compute (`sbatch`), verify tree md5 unchanged before/after each run, poll all jobs (`scripts/slurm_poll.sh`), and record `sacct` State/ExitCode and full logs:

| Arm | `SFS_AXIS_LOCK_ICP` | `SFS_XY_BREAK_NORMALS` | Datasets & Repeats | Notes |
|---|---:|---:|---|---|
| `arm00_stock` | `0` | `0` | `Juglet` (`SFS_SIGNED_K2=1`), `Pot_A` | Baseline anchor (must match Ticket 355: `2/9, 1/18` & `8/8, 15/15`) |
| `arm10_axis_only` | `1` | `0` | `Juglet` (`SFS_SIGNED_K2=1`), `Pot_A` | Isolates `iter=0..49` candidate-axis lock alone |
| `arm01_xynorm_only` | `0` | `1` | `Juglet` (`SFS_SIGNED_K2=1`), `Pot_A` | Isolates horizontal-normal comparison alone |
| `arm11_both` | `1` | `1` | `Juglet` (`SFS_SIGNED_K2=1`, **`r1` + `r2`**), `Pot_A` | Tests both fixes together + bit-exact determinism |

All runs include `SFS_PRECOR_TRACE=1 SFS_STAGE_TRACE=1 SFS_EDGE_TRACE=1 SFS_STATE_TRACE=1` (plus one unset smoke verification or zero-diff check on `arm00_stock` vs EE55CE2).

### Required Tables in `C:/PR/artifacts/t357/`

1. **`factorial357_juglet.tsv` (`64 rows` + header):** `16` pairs (`14` measured joins + `4-7, 4-6` controls) × `4` arms (`arm00_stock`, `arm10_axis_only`, `arm01_xynorm_only`, `arm11_both`), columns:
   `arm  pair  ncand  in_bar_0_any  best_idx  lcs_frac  lcs_shift_mm  rot0_deg  tilt0_deg  spin0_deg  mm0  in_bar_0  rotF_deg  tiltF_deg  spinF_deg  mmF  in_bar_F  inl_F  score_F  in_basket_le1p5  fate`
   *(report the best-by-`rotF` candidate with `mmF <= 5.0` if any, else smallest `rotF`, and also note `in_bar_0_any` across all candidates for that join; verify `arm11_both` `r1 == r2` bit-exact across all rows).*
2. **`factorial357_summary.tsv` (`8 rows` + header):** `4` arms × `2` datasets (`juglet`, `pota`), columns:
   `dataset  arm  n_measured  in_bar_0  in_bar_F  joins_inl_ge6  joins_basket_le1p5  joins_surv  final_sherds  final_edges  best_step_sherds  best_step_edges`

---

## Part C — Visual Verification (`C:/PR/artifacts/t357/render357_hinge_vs_fixed.png`)

Produce a clear visual comparison `C:/PR/artifacts/t357/render357_hinge_vs_fixed.png` (`>= 1800x1200 px`, labeled axes, legend) covering representative bevel-hinged Juglet joins (`1-4` with `34.9 deg` GT normal bevel / `33.25 deg` stock `tilt0`, `1-9` with `26.9 deg` / `27.65 deg`, and `3-8` with `17.9 deg` / `9.16 deg`), comparing side-by-side (`4 columns × 3 rows`):
- **Col 1:** Ground truth (`0.00 deg, 0.00 mm`) showing the fixed & moved breaklines plus 3D vs `(x, y)` projected normal vectors at the seam (making the opposing `N_z` bevel visible).
- **Col 2:** `arm00_stock` (`iter = 0` hinged pose & `final` pose).
- **Col 3:** `arm10_axis_only` (`iter = 0` & `final`).
- **Col 4:** `arm11_both` (`iter = 0` & `final`, annotated with `rot0/mm0`, `rotF/mmF`, `inl_F`, and `score_F`).

---

## Part D — Reading (name exactly one)

- **`axis-and-xynorm-unlock-both-pair-stage-and-assembly`** — On `arm11_both`, **>= 9 of 14** measured Juglet joins land in the `5 deg / 5 mm` bar at `iter = 0` and survive `pair_basket` (`<= 1.5`) & `CountInlier` (`>= 6`), **assembly improves beyond `2/9 sherds, 1/18 edges`** (to `>= 5/9` sherds or `>= 4/18` edges), and `Pot_A` retains `8/8 sherds, 15/15 edges`.
- **`axis-and-xynorm-recover-pair-joins-but-downstream-blocks-assembly`** — `arm11_both` recovers the pair stage (**>= 9 of 14** Juglet joins in bar at `final` and passing `CountInlier` / `pair_basket`), strictly beating `arm10_axis_only` on `CountInlier` / `basket` survival while `Pot_A` holds `8/8, 15/15`, **but end-to-end assembly still stops below `5/9` sherds / `4/18` edges** (pinpointing `StateManager` / scoring / false-join competition `4-7, 7-9` as the next blocker).
- **`axis-lock-alone-recovers-pair-joins-without-xynorm`** — `arm10_axis_only` already recovers `>= 9 of 14` Juglet joins into the `5 deg / 5 mm` bar **and** passes `CountInlier` / `pair_basket` at `final` without needing `SFS_XY_BREAK_NORMALS` (or `SFS_XY_BREAK_NORMALS` hurts `Pot_A` / adds nothing over `arm10`).
- **`neither-axis-lock-nor-xynorm-moves-final-pair-yield`** — `arm11_both` leaves `< 7 of 14` measured Juglet joins inside `5 deg / 5 mm` at `final` (or breaks `Pot_A`'s `8/8, 15/15` control), disproving the `iter=0` hinge / `N_z` bevel hypothesis as the main pair-stage bottleneck.

---

## Part E — Acceptance

- [ ] `C:/PR/artifacts/t357/gt_normal_audit357.tsv` (31 rows) confirms `Pot_A` `15/15` pass 3D GT normal gate (`med3D ~ 3.3 deg`), while Juglet passes `9/14` in 3D (failing `1-4, 2-6, 1-9, 1-3, 3-5`) and `14/14` in `(x, y)`.
- [ ] Both `SFS_AXIS_LOCK_ICP` and `SFS_XY_BREAK_NORMALS` are default OFF in `InitSfsSwitches()`, print startup banners, keep `class/` GT-free, and `arm00_stock` matches Ticket 355 anchors (`Juglet 0/14` iter-0, `4/14` final, `2/9 sherds, 1/18 edges`; `Pot_A 6/15` iter-0, `10/15` final, `8/8 sherds, 15/15 edges`).
- [ ] All Slurm jobs across the 4 factorial arms (`Juglet` + `Pot_A`, plus `arm11_both_r2` repeat) `COMPLETED 0:0`, with tree md5 identical before/after, `sacct` table recorded, and `arm11_both_r1` vs `r2` bit-exact.
- [ ] `C:/PR/artifacts/t357/factorial357_juglet.tsv` (64 rows), `C:/PR/artifacts/t357/factorial357_summary.tsv` (8 rows), and `C:/PR/artifacts/t357/render357_hinge_vs_fixed.png` (`>= 1800x1200 px`) are written and visually inspected.
- [ ] `Pot_A` retains `8/8 sherds, 15/15 edges` under the chosen winning setting, OR any `Pot_A` regression is diagnosed explicitly.
- [ ] Write-back ends with one Part D `Reading:` slug, per-arm counts, `verified:` lines, and `**End state:**`.
