# Ticket 357 — Does locking the candidate axis at `iter = 0` and removing vertical breakline-normal bevel bias recover the Juglet pair stage?

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-15)
**Branch:** 357-what-causes-the-iter0-hinge-and-inlier-rejection, in /c/PR/worktrees/structure-from-sherds-pp/357
**Worker:** dsh one-shot subagent w357, dispatched 2026-11-15 by the lead
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

> **Dispatch note (lead, 2026-11-15).** The signed-axis switch is \`SFS_AXIS_SIGN=1\` (read at
> \`class/feature_matching.cpp:114\`); \`SFS_SIGNED_K2\` does not exist in this tree and was a slip in the draft.
> \`bool axis = false;\` is \`class/reconstruction.cpp:1258\` and \`w_a = 0.1\`, \`w_n = 3.0\` are the \`Icp\` locals at
> \`:1242\`. Everything else in this ticket is as written.

## Part A — Ground-Truth Seam Normal Audit & Stock Anchor Reproduction

1. **Deterministic script `C:/PR/artifacts/t357/audit_normals357.py`:**
   - Using the existing `tree/juglet` and `tree/pota` inputs under `C:/PR/artifacts/t355/tree/` (copied or referenced read-only; never alter `C:/PR/artifacts/t355/`), evaluate every join (`14` measured Juglet joins + `2` controls `4-7, 4-6`, and all `15` `Pot_A` GT joins) placed at the **exact ground-truth pose (`0.00 deg, 0.00 mm`)** in the axis-aligned reference frame of piece `1` (`T_i = axis[1] * GT[1]^-1 * GT[i]`).
   - Find mutual nearest neighbours `< 1.5 mm` real (`4.5` tree units) between `p_1` and `p_2`, normalize `n_1, n_2`, compute both folded (`min(a, 180-a)`) 3D normal angles and folded horizontal `(N_x, N_y)` angles (plus `mean_nz_A`, `mean_nz_B`, `med_ang3d_unfolded`, `med_ang3d_folded`, `med_ang_xy_folded`, `inl_3d_lt15`, `inl_xy_lt15`, `pass_3d_ge6`, `pass_xy_ge6`).
   - Write `C:/PR/artifacts/t357/gt_normal_audit357.tsv` (**31 rows**: `16` Juglet + `15` `Pot_A`).
   - **Mandatory check:** confirm `Pot_A` passes `15/15` in 3D (`med_ang3d_folded` median `~3.3 deg`) and `15/15` in `(x, y)`, while Juglet passes `9/14` in 3D (failing `1-4`, `2-6`, `1-9`, `1-3`, `3-5`) and `14/14` in `(x, y)`.

2. **Stock baseline anchors:**
   Re-run stock `arm00_stock` (`SFS_AXIS_LOCK_ICP=0 SFS_XY_BREAK_NORMALS=0`, with `SFS_AXIS_SIGN=1` on Juglet and default on `Pot_A`, plus `SFS_PRECOR_TRACE=1 SFS_STAGE_TRACE=1 SFS_EDGE_TRACE=1 SFS_STATE_TRACE=1`) to anchor the exact baseline in this binary (`Juglet`: `180` candidates, `0/14` at `iter = 0`, `4/14` measured joins at `final`, `2/9 sherds, 1/18 edges`; `Pot_A`: `124` candidates, `6/15` at `iter = 0`, `10/15` at `final`, `8/8 sherds, 15/15 edges`).

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
| `arm00_stock` | `0` | `0` | `Juglet` (`SFS_AXIS_SIGN=1`), `Pot_A` | Baseline anchor (must match Ticket 355: `2/9, 1/18` & `8/8, 15/15`) |
| `arm10_axis_only` | `1` | `0` | `Juglet` (`SFS_AXIS_SIGN=1`), `Pot_A` | Isolates `iter=0..49` candidate-axis lock alone |
| `arm01_xynorm_only` | `0` | `1` | `Juglet` (`SFS_AXIS_SIGN=1`), `Pot_A` | Isolates horizontal-normal comparison alone |
| `arm11_both` | `1` | `1` | `Juglet` (`SFS_AXIS_SIGN=1`, **`r1` + `r2`**), `Pot_A` | Tests both fixes together + bit-exact determinism |

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
---

## Comments (worker, 2026-11-15)

Worker: one-shot subagent (`w357`), dispatched 2026-11-15 by the lead. Worktree `C:/PR/worktrees/structure-from-sherds-pp/357` at `5b84461`; artefacts in `C:/PR/artifacts/t357/`. Two default-off switches in `class/`; nothing committed, nothing pushed.

### 1. The nine arms, their jobs, and their accuracy

| arm | `SFS_AXIS_LOCK_ICP` | `SFS_XY_BREAK_NORMALS` | dataset | job | `sacct` | sherds / edges |
|---|---:|---:|---|---|---|---|
| `arm00_stock` | 0 | 0 | Juglet (`SFS_AXIS_SIGN=1`) | `35077495_0` | COMPLETED 0:0, 04:42 | `2/9`, `1/18` |
| `arm00_stock` | 0 | 0 | Pot_A (unsigned) | `35077495_5` | COMPLETED 0:0, 03:11 | `8/8`, `15/15` |
| `arm10_axis_only` | 1 | 0 | Juglet | `35077495_1` | COMPLETED 0:0, 13:08 | `2/9`, `1/18` |
| `arm10_axis_only` | 1 | 0 | Pot_A | `35077495_6` | COMPLETED 0:0, 05:50 | `8/8`, `15/15` |
| `arm01_xynorm_only` | 0 | 1 | Juglet | `35077495_2` | COMPLETED 0:0, 06:06 | `2/9`, `1/18` |
| `arm01_xynorm_only` | 0 | 1 | Pot_A | `35077495_7` | COMPLETED 0:0, 04:07 | `8/8`, `15/15` |
| `arm11_both` r1 | 1 | 1 | Juglet | `35077495_3` | COMPLETED 0:0, 08:57 | `2/9`, `1/18` |
| `arm11_both` r2 | 1 | 1 | Juglet | `35077495_4` | COMPLETED 0:0, 07:18 | `2/9`, `1/18` |
| `arm11_both` | 1 | 1 | Pot_A | `35077495_8` | COMPLETED 0:0, 03:27 | `7/8`, `9/15` |

Run dir `/data/gpfs/projects/punim2657/SfSpp/runs/20261115-0038_5b84461_t357-factorial`, `SOURCE_COMMIT=5b84461`. Build gate `35077453_0`/`_1` COMPLETED 0:0 (39 s / 28 s, `=== compile rc=0`); the first attempt `35077400_0` FAILED 3:0 on a staging bug only (`inc_juglet`/`inc_pota`; the R333 include dirs are `inc_jug`/`inc_pota`) and no arm ran before it was fixed. Every arm: run rc 0, tree md5 identical before and after, the startup banner matched its cell, `SFS_PRECOR_TRACE`/`STAGE`/`EDGE`/`STATE` all on, `[SFS-355] PRECOR` exactly 180 (Juglet) / 124 (Pot_A). All jobs and their `sacct` lines: `logs/sacct357.txt`.

### 2. `arm00_stock` reproduces ticket 355 where the switches cannot reach

Juglet 180 candidates, `0/14` in the bar at `iter = 0`, `4/14` at `final`, `2/9 sherds, 1/18 edges`; Pot_A 124 candidates, `6/15`, `10/15`, `8/8`, `15/15` — all four ticket-355 anchors.

Zero-diff against `C:/PR/artifacts/t355/logs/binary_juglet_signed_on_r1.log`, measured line by line: `[SFS-355]` PRECOR 180 vs 180, **0** differing lines; `[SFS-336] CAND` 180 vs 180, **0** differing; `CAND338` 51 vs 51, **0** differing. The assembly-stage traces do move: `NODE` 562 of 800 lines and `RCAND` 1567 of 2652 (2652 vs 2673 lines) differ in the sixth significant digit. That is the multi-threaded `Registration` reduction, not this ticket: `NUMBER_OF_THREAD = 12` (`class/data_structure.h:18`), `options.num_threads = NUMBER_OF_THREAD` in `Icp` (`class/reconstruction.cpp:1499`) and its siblings. The pair stage, the only thing the two switches touch, is byte-identical; no threshold, weight default, `TOP_k`, `BRANCH` or `MINIMUM_NUMBER` was changed.

`arm11_both` r1 vs r2: the 16 rows of the per-join table are byte-identical and the Juglet summary block is identical.

### 3. Part A — the GT seam-normal audit (`gt_normal_audit357.tsv`, 31 data rows)

Pot_A: 15/15 pass in 3D and 15/15 in `(x, y)`; per-join folded medians 1.04-8.41 deg, median of the 15 medians **3.31 deg**. Juglet: **9/14** in 3D, **14/14** in `(x, y)`; the five 3D failures are exactly the ticket's, with its inlier counts — `1-4` 34.86 deg `0/19`, `2-6` 30.45 `1/26`, `1-9` 26.85 `5/17`, `1-3` 19.78 `5/22`, `3-5` 18.50 `4/10`; per-join folded median 16.01 deg and `(x, y)` median 5.04 (ticket: 16.0 / 5.1). Passing joins: `1-2, 1-8, 2-9, 3-8, 5-6, 5-7, 5-8, 6-7, 6-9`.

Controls: `4-7` has 33 mutual-NN pairs and passes 14/33 in 3D but 26/33 in `(x, y)`; `4-6` has **0** mutual neighbours inside 1.5 mm at the exact GT pose, so its medians are `nan` — unmeasurable at GT, reported as such rather than as a normal failure. `SFS_AXIS_SIGN` takes no part in this audit (pure GT geometry; piece 1's axis `0` defines the frame).

### 4. Part B — the two switches as built

Both are read once in `InitSfsSwitches` (`class/feature_matching.cpp:133-134`), print in the banner (`:175-176`) and default to `false` (`:91-92`; declared `class/feature_matching.h:36-37`). With both off every new branch is skipped and the legacy expressions run unchanged, which is what section 2 measures.

- **`SFS_AXIS_LOCK_ICP`.** At `iter = 0` with `UsePreCorres`: minimum-angle rotation of the moving candidate axis onto the fixed one (`Sfs357RotFromTo`, `class/reconstruction.cpp:1232`), closed-form spin about the fixed axis and axial shift from the `COR[0]` pairs, sign chosen by residual (`+1` only when `g_sfs_axis_sign`, otherwise `+1/-1`). The init is applied to the *data*, to `R`/`t` **and** to the correspondence set before Ceres (`:1382-1449`), so `PrintPrecorTrace` at `iter = 0` prints the true post-init `M0`. Then a `CandidateAxisLockFunctor` (`class/reconstruction.h:635`) holds axis direction (weight `1e3`) and lateral axis offset (`10.0`) for that pair at every iteration, plain L2, leaving spin and axial height free (`:1459-1463`).
- **`SFS_XY_BREAK_NORMALS`.** One global axis expressed in the frame the breakline data currently live in (`g_sfs357_xy_axis`, `class/reconstruction.cpp:107`), set per iteration from `R[shard_A-1] * axis_norm_[axis_index_y_]` (`:1329-1333`), refreshed before the final scoring block (`:1537-1540`) and cleared at `:1562`. Consumed by `CostFuncNorm`/`CostFuncFixedNorm` (`class/reconstruction.h:571-620`), `RejectOutlier` (`:217`) and the normal terms of `MatchingScore`/`CountInlier` (`class/feature_matching.cpp:1239, 1259, 1289, 1327`). **Scope, as the ticket asked to have documented: `Icp` only.** The global is nonzero only inside `Icp`, so `Registration`/`MultiRegistration` keep the 3D expression even with the switch on — which is why `arm01_xynorm_only` moves so little: its only effect is inside the pair solve itself.
- **Determinism and GT.** No new switch reads ground truth: the 361 added lines mention no GT path or file (0 hits for `ground`/`gt_` in the added lines), and the pre-existing GT path table in `class/data_path.h:157+` is untouched. Both switches are read once, so the solve is unchanged when they are off (section 2).

### 5. Part B — the factorial (8 summary rows)

| dataset | arm | `in_bar_0` | `in_bar_F` | `joins_inl_ge6` | `joins_basket_le1p5` | `joins_surv` | final |
|---|---|---:|---:|---:|---:|---:|---|
| juglet | `arm00_stock` | 0 | 4 | 14 | 14 | 9 | `2/9`, `1/18` |
| juglet | `arm10_axis_only` | **10** | **10** | 8 | 12 | 8 | `2/9`, `1/18` |
| juglet | `arm01_xynorm_only` | 1 | 3 | 14 | 14 | 9 | `2/9`, `1/18` |
| juglet | `arm11_both` | **10** | **10** | **12** | 12 | 9 | `2/9`, `1/18` |
| pota | `arm00_stock` | 6 | 10 | 15 | 15 | 12 | `8/8`, `15/15` |
| pota | `arm10_axis_only` | 8 | 8 | 7 | 9 | 9 | `8/8`, `15/15` |
| pota | `arm01_xynorm_only` | 4 | 7 | 12 | 12 | 10 | `8/8`, `15/15` |
| pota | `arm11_both` | 8 | 8 | 7 | 9 | 9 | `7/8`, `9/15` |

`n_measured` is 14 / 15. `best_step_sherds` and `best_step_edges` are copies of `final_*`: the shipped binary evaluates accuracy once, at the end (one `Sherd accuracy` line in the log, the second one in the `Acc.txt` is the edge accuracy), so there is no per-step accuracy to report — the columns are filled, explicitly, not silently dropped.

**Juglet, per join.** Stock's four in-bar joins are `1-2` (1.93 deg / 0.64 mm), `1-8` (0.49 / 0.43), `5-7` (4.71 / 1.23), `6-9` (2.35 / 0.57). The axis lock takes that to ten, all at `iter = 0` and all still there at `final`: `1-2` 0.25/0.75, `1-4` 2.52/0.72, `1-8` 0.36/0.73, `1-9` 0.38/0.67, `2-9` 3.24/0.98, `3-8` 2.25/0.58, `5-6` 2.87/3.30, `5-7` 1.02/0.42, `5-8` 2.37/0.64, `6-9` 2.01/0.94 — with `tilt0 = 0.001-0.012 deg` on every one of them, i.e. the hinge is gone at the first solve. The other four do not move: `1-3` 28.35/13.11, `2-6` 14.61/3.88, `3-5` 48.01/11.62, `6-7` 163.61/26.70.

Three of those ten are **pose-correct but discarded**: `1-4` (2.52 deg / 0.72 mm) prints `inl_F = 0`, `score_F = 11.0000`; `1-9` (0.38 / 0.67) prints `inl_F = 4`, `11.0000`; `6-9` (2.01 / 0.94) prints `inl_F = 4`, `11.0000` and is basket-dropped. `11.0000` is `Icp`'s sentinel (correspondences fell below `MINIMUM_NUMBER`, or the final `isConverge` gate failed), so a correct pose is thrown away at the end. Turning the `(x, y)` comparison on rescues exactly these three: `1-4` 32 inliers / 0.7001, `1-9` 14 / 1.0326, `6-9` 14 / 0.5978 (`joins_inl_ge6` 8 -> 12) — at an unchanged 10/14 in bar.

**Pot_A.** The axis lock alone is safe (`8/8`, `15/15`) though it swaps which joins sit in the bar at `final` (10 -> 8 of 15); `(x, y)` alone is safe too and also ends at 8 of 15 in bar; only the **combination** breaks the control to `7/8`, `9/15`. The counts do not explain it — `pota_axis` and `pota_both` carry identical summary rows (8 in bar at both ends, 7 with `>= 6` inliers, 9 basket-passing, 9 surviving) — so the damage is a change of *which* candidates survive into the merge competition, not how many: in `pota_both` piece 5 ends alone in its own cluster (`OUTSTATE 0 548 2`, `CLUSTER 0 ... 1,2,3,4,6,7,8`, `CLUSTER 1 ... 5`), which is the missing sherd and most of the six lost edges. The audit offers the mechanism to check next: on 7 of the 15 Pot_A joins `mean_nz` is 0.95-0.99, i.e. those rims are nearly flat and carry almost no horizontal normal component, so a comparison moved into the `(x, y)` plane is comparing the small part of the vector.

### 6. Where the remaining four Juglet joins are lost — not at the hinge

With the axis locked and the `(x, y)` comparison on, every join still outside the bar is traceable to the seam or to the signed-axis convention, not to the `iter = 0` hinge:

- `1-3` — `lcs_frac = 0.000`, shift 12.98 mm: the preselected dots are not on the seam, so the lock leaves the spin 28.35 deg out.
- `3-5` — `lcs_frac = 0.500`, shift 12.44 mm: half the seam, and the spin stays 48.01 deg out.
- `6-7` — `lcs_frac = 0.500`, shift 27.13 mm; and its GT normals are folded 4.38 deg but unfolded 175.62 deg, so with `SFS_AXIS_SIGN=1` the signed `axis_angle` stays ~164 deg and the pair never enters the bar. A signed-axis artefact, not a switch artefact.
- `2-6` — seam right (`lcs_frac = 1.000`) but the largest shift of the fourteen (3.92 mm); the lock lands it at 14.61 deg / 3.88 mm, close but outside.

So the two switches fix the mechanism they were built for; what is left is the seam selection and the signed-axis choice on `6-7`.

### 7. Reading

The axis lock alone converts the Juglet pair stage: `in_bar_0` 0 -> 10 and `in_bar_F` 4 -> 10, with the hinge gone at the first solve (`tilt0 ~ 0.01 deg`). The `(x, y)` normal comparison adds nothing to that count (10 -> 10), does rescue the three pose-correct-but-discarded joins at the inlier gate (8 -> 12 with `>= 6` inliers), and it is the only setting that breaks `Pot_A` (`7/8`, `9/15` against `8/8`, `15/15` with either switch alone). End-to-end assembly is `2/9`, `1/18` on every Juglet arm, so the bracket that requires assembly to move fails, and the bracket that requires `Pot_A` to hold under the combination fails too.

Reading: `axis-lock-alone-recovers-pair-joins-without-xynorm`

That is the slug whose parenthetical fires twice over: `arm10_axis_only` alone reaches 10 of 14 in the bar at `final` (>= 9), and `SFS_XY_BREAK_NORMALS` both adds nothing over `arm10` on the bar and hurts `Pot_A` when combined with the lock. The honest caveat on "passes `CountInlier`": of arm10's ten in-bar joins, seven clear `>= 6` inliers and the three that do not (`1-4`, `1-9`, `6-9`) are exactly the ones the `(x, y)` comparison rescues — so the choice between "axis lock alone" and "axis lock + `(x, y)`" is not about the bar at all: it is that the extra switch buys 4 more joins at the inlier gate on Juglet and costs the `Pot_A` control `1/8` sherd and `6/15` edges.

### 8. verified: lines

- verified: `C:/PR/artifacts/t357/factorial357_summary.tsv:2` -> `juglet arm00_stock 14 0 4 14 14 9 2/9 1/18 2/9 1/18`
- verified: `C:/PR/artifacts/t357/factorial357_summary.tsv:3` -> `juglet arm10_axis_only 14 10 10 8 12 8 2/9 1/18 ...`; `:5` -> `arm11_both ... 10 10 12 12 9 ...`
- verified: `C:/PR/artifacts/t357/factorial357_summary.tsv:6` -> `pota arm00_stock ... 6 10 15 15 12 8/8 15/15`; `:9` -> `pota arm11_both ... 8 8 7 9 9 7/8 9/15`
- verified: `C:/PR/artifacts/t357/factorial357_juglet.tsv:20` -> `arm10_axis_only 1-4 ... rotF 2.5211 tiltF 0.0123 mmF 0.7233 in_bar_F 1 inl_F 0 score_F 11.0000`; `:52` -> `arm11_both 1-4 ... inl_F 32 score_F 0.7001`
- verified: `C:/PR/artifacts/t357/factorial357_juglet.tsv:22` -> `arm10 1-9 inl_F 4 score_F 11.0000`; `:54` -> `arm11 1-9 inl_F 14 score_F 1.0326`; `:31` -> `arm10 6-9 inl_F 4 score_F 11.0000 fate pair_basket`; `:63` -> `arm11 6-9 inl_F 14 score_F 0.5978`
- verified: `C:/PR/artifacts/t357/factorial357_juglet.tsv:19` -> `arm10 1-3 rotF 28.3460 mmF 13.1055 lcs_frac 0.0000`; `:25` -> `3-5 rotF 48.0092 ... lcs_frac 0.5000 shift 12.4357`; `:30` -> `6-7 rotF 163.6105`; `:23` -> `2-6 rotF 14.6140 mmF 3.8814 shift 3.9168`
- verified: `C:/PR/artifacts/t357/gt_normal_audit357.tsv:4` -> `juglet 1-4 ... med_ang3d_folded 34.857 inl_3d_lt15 0 of 19`; `:7` 30.454 / 1 of 26; `:6` 26.853 / 5 of 17; `:3` 19.778 / 5 of 22; `:9` 18.497 / 4 of 10; `:17` -> `juglet 4-6 True 0 nan`
- verified: `C:/PR/artifacts/t357/gt_normal_audit357.tsv:18-32` -> all 15 `pota` rows `pass_3d_ge6 = 1` and `pass_xy_ge6 = 1`, folded medians 1.040-8.413
- verified: `C:/PR/artifacts/t357/logs/binary_juglet_stock.log:14` -> `axis lock icp: off`, `:15` -> `xy break normals: off`, `:8608` -> `********* Sherd accuracy : 2 / 9 (22.2222%) *********`; `logs/binary_juglet_axis.log:14` -> `axis lock icp: on`; `logs/binary_juglet_both_r1.log:15` -> `xy break normals: on`
- verified: `C:/PR/artifacts/t357/logs/binary_pota_both.log:8744` -> `Sherd accuracy : 7 / 8 (87.5%)`, `:8747-8749` -> `OUTSTATE 0 548 2` then `CLUSTER 0 2 548 1,2,3,4,6,7,8` and `CLUSTER 1 5 0 5` (piece 5 alone); `logs/binary_pota_stock.log:9504` -> `Sherd accuracy : 8 / 8 (100%)`, `:9507-9508` one cluster of `1..8`
- verified: `C:/PR/artifacts/t357/fetch1/out/juglet_stock/1. Acc.txt:2-3` -> `Sherd accuracy : 2 / 9 (22.2222 %)` and `1 / 18 (5.55556 %)`; `fetch1/out/pota_both/1. Acc.txt:2-3` -> `7 / 8 (87.5 %)` and `9 / 15 (60 %)`
- verified: `C:/PR/artifacts/t357/logs/sacct357.txt:5-17` -> the nine array tasks and the two build tasks with State/ExitCode/Elapsed/NodeList as tabulated in section 1
- verified: PRECOR 180/180 and 124/124 lines in every arm log, `grep -c "PRECOR"` per file, and the `[SFS-355]` zero-diff of section 2 (`diff` of the filtered `[SFS-355]` and `CAND`/`CAND338` lines -> 0 lines)
- verified: switch code `class/feature_matching.cpp:91-92` (defaults `false`), `:133-134` (`ReadSfsSwitch`), `:175-176` (banner); `class/reconstruction.cpp:107` (global), `:1232` (`Sfs357RotFromTo`), `:1382` (`iter == 0` lock), `:1459-1463` (functor), `:1329-1333` / `:1537-1540` / `:1562` (axis set, refresh, clear), `:217` (`RejectOutlier`); `class/reconstruction.h:44-67` (global + `Sfs357NormDot/Angle`), `:571-620` (the two norm cost functors), `:635` (`CandidateAxisLockFunctor`); `class/feature_matching.cpp:1239, 1259, 1289, 1327` (score/inlier normals)

### 9. Artefacts in `C:/PR/artifacts/t357/`

`gt_normal_audit357.tsv` (31 rows) and `audit_normals357.py`; `factorial357_juglet.tsv` (64 rows, 21 columns), `factorial357_summary.tsv` (8 rows), `factorial357_all.tsv` (every candidate), `analyze357.py`, the manifests `manifest357*.tsv`; `render357_hinge_vs_fixed.png` (2340x1386) and `render357.py`, plus `crop_row1_gt.png` for the arrow detail; `logs/` with the nine `binary_*` logs, the nine `slurm-arm-35077495_*.out` and `sacct357.txt`; `fetch1/`, `fetch2/`, `fetch3/` with the raw `out/<arm>/1. Acc.txt`; `src357.tar.gz`, `submit357_array.sbatch`, `submit_checkbuild357.sbatch`, `arm357_map.tsv`.

The render is 4 columns (`GT`, `arm00_stock`, `arm10_axis_only`, `arm11_both`) x 3 rows (`1-4`, `1-9`, `3-8`), each a side view in the fixed piece's axis-aligned frame (`r = hypot(x, y)` against `z`), with an equal-aspect seam zoom whose thin arrows are the 3D seam normals and whose bold arrows are their `(x, y)` projections. I looked at it: in row `1-4` the fixed-piece normals point down-left and the GT-moved ones up-left (the opposing `N_z` bevel the audit measures at 34.86 deg while `(x, y)` is 2.40), the stock column starts tens of units away and ends visibly rotated, the axis column lands on the GT seam at `iter = 0`, and the `arm11_both` column keeps the inlier count (`inl_F` 32 against 0) on the same pose.

### Lead — verification (2026-11-15, Thinkbook16)

- verified: COLUMNS=200 python C:/PR/artifacts/dsh-handover/357/accept357.py -> 32 cases, 32 pass (two cases were the
  lead's own and were fixed: the audit count had included the 4-7 control and the 4-6 unmeasurable row; the job scan
  now reads the sacct file and the slurm stdout, not only the ticket).
- verified: git diff HEAD --stat -> class/feature_matching.cpp +22/-1, class/feature_matching.h +3, class/reconstruction.cpp
  +139/-4, class/reconstruction.h +95, ticket +108. Five existing normal comparisons are replaced in place, each by a
  switch-guarded helper whose off path is the legacy expression (class/reconstruction.h:51-68: Sfs357NormDot returns
  n_A.dot(n_B) when off, Sfs357NormAngle returns acos(n_A.dot(n_B)) when off).
- verified: both switches read through ReadSfsSwitch, declared false, banners "axis lock icp: " and "xy break normals:
  "; no SFS_TOPK / SFS_BRANCH / SFS_PC_BAR / ANGLE_THRESHOLD / TOP_k / BRANCH_b / MINIMUM_NUMBER line added; no GT path
  in class/; no other new switch name.
- verified: CAND338 block of the stock arm against C:/PR/artifacts/t355/logs/binary_juglet_signed_on_r1.log -> 0
  differing lines (51 vs 51), so both switches off reproduce ticket 355 exactly.
- verified: the lead's own recomputation from factorial357_juglet.tsv: joins in bar at iter 0 / at final = stock 0/14
  and 4/14, axis-only 10/14 and 10/14, xynorm-only 1/14 and 3/14, both 10/14 and 10/14; in bar AND inl_F >= 6 = 4, 7,
  3, 10. The summary table's joins_inl_ge6 counts all 14 joins (8, 12), so the 7 is the in-bar intersection, not a
  contradiction.
- verified: accuracy from logs/slurm-arm-*.out -> every Juglet arm 2/9 sherds and 1/18 edges (five runs, both repeats
  included); Pot_A stock 8/8 and 15/15, axis-only 8/8 and 15/15, xynorm-only 8/8 and 15/15, both 7/8 and 9/15.
- verified: gt_normal_audit357.tsv 31 rows; Juglet 9 of 14 joins pass the 3D gate at exact GT (failures 1-3 19.78,
  1-4 34.86, 1-9 26.85, 2-6 30.45, 3-5 18.50 deg) and 14 of 14 pass in (x, y); Pot_A 15 of 15 both ways; control 4-6 is
  nan with n_pairs 0 and is reported unmeasurable, which is right.
- verified: render357_hinge_vs_fixed.png opened, 2340x1386. In the stock column the iter-0 piece (blue) is visibly
  rotated off the green ground truth; in the axis-only column it sits on it. The annotations agree with the table:
  1-4 33.29 -> 2.52 deg, 1-9 28.12 -> 0.38, 3-8 10.86 -> 2.25.
- verified: sacct — 35077453_0/_1 and 35077495_0..8 all COMPLETED 0:0; 35077400_0 FAILED 3:0 on a staging path only,
  before any arm ran (recorded by the worker).
- verified: best-per-pair CAND338 rotation agrees exactly between both_r1 and both_r2 (28 pairs, 0 differences), so the
  16-row table identity holds; the raw blocks differ in 12 of 51 lines in the sixth digit, the known 12-thread Ceres
  drift, so "bit-exact" is a table-level statement, not a trace-level one.

Adjudications.

1. The reading fires through its parenthetical, not its main clause. axis-lock-alone-recovers-pair-joins-without-xynorm
   says ">= 9 of 14 in bar AND passing CountInlier / pair_basket at final". arm10 has 10 of 14 in bar but only 7 of
   those 10 pass the inlier gate (1-4 inl 0, 1-9 4, 6-9 4). What does hold is the alternative: the xy-normals switch
   adds no bar entries over arm10 (10 vs 10) and it hurts the Pot_A control when combined (7/8, 9/15). The literal
   clause is not met; the reading is kept with that recorded, because the substantive claim - axis lock alone recovers
   the joins in bar, and xynorm is not needed for that - is what the numbers say.
2. The combination is not a candidate for a fix as implemented: it is the only setting that gives 10 of 10 in-bar
   joins passing the inlier gate, and it is the only setting that breaks Pot_A (piece 5 alone in a cluster, 8/8 -> 7/8
   and 15/15 -> 9/15). Any future ticket must treat the two switches as needing separate scoping for Pot_A, not as a
   pair to switch on together.
3. Assembly is unchanged at 2/9 sherds and 1/18 edges on every Juglet arm. This is the ticket's most important negative
   result: the pair stage is fixed and the result does not move, so the binding constraint is downstream of the pair
   stage - in-bar joins are still dropped by pair_overlap (3-8) and pair_basket (6-9) and the state graph keeps 8 of
   the 10, and the winner still seats two pieces.

**End state:** ready
