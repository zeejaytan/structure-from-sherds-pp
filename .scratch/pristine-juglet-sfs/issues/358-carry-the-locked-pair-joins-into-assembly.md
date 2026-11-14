# Ticket 358 — Fix the `iter = 0` convergence short-circuit and carry the locked pair joins into assembly

**Status:** ready-for-agent
**Answers:** S1
**Needs-eye:** yes
**Depends on:** 357 (landed at `1141787`: `SFS_AXIS_LOCK_ICP` + `SFS_XY_BREAK_NORMALS` recovers `10/14` Juglet joins in bar with `>= 6` inliers at `Icp` and seats `{1, 2, 8}` + `{5, 7}` in `FINAL`, but breaks `Pot_A` in combination and stops at `3` accurate joins in `FINAL`)
**Blocked-by:** none
**Budget:** medium (small targeted edits in `class/`, offline audit on existing `t357` logs, one 6-arm Spartan `sbatch` array on `Juglet` + `Pot_A`, one visual assembly render in `C:/PR/artifacts/t358/`)
**Hold:** Keep all work inside your worktree branch and `C:/PR/artifacts/t358/`; do not commit or push, do not touch `C:/PR/artifacts/t357/`, and do not alter any shipped constant (`w_a`, `0.436`, `1.5`, `MINIMUM_NUMBER`, `TOP_k`, `BRANCH_b`, `ANGLE_THRESHOLD`) or modify `CountResult` / `isSimilarTrans` in `class/data_structure.cpp`.

> **In one line.** Lead verification on ticket 357 proved three things: (1) `SFS_AXIS_LOCK_ICP` accidentally terminated `Icp` at `iter == 0` before `iter = 1..49` (`MakeCor + P2LConstraint`) ever ran (`max|rotF - rot0| = 0.000000 deg` across all `304` candidates), and `Sfs357NormDot` projected `Pot_A`'s flat rims (`|n_z| = 0.95-0.99`, `||n_perp|| ~ 0.14 > 0.1`) into `(x, y)`, breaking `Pot_A` (`7/8, 9/15`); (2) `arm11_both` already assembled **`{1, 2, 8}`** (`1-2` at `2.55 deg / 0.70 mm`, `1-8` at `1.22 deg / 0.45 mm`) plus `{5, 7}` (`4.70 deg / 1.18 mm`) in `FINAL`, masked by `CountResult`'s `1,605` tree-unit origin lever arm; and (3) `8 of 10` in-bar Juglet joins entered `StateManager` (`CAND338`), where unlocked 6-DoF `Registration` with 3D normals re-hinged or dropped them.

## What post-run verification of ticket 357 established

1. **Bug 1 — `iter == 0` `isConverge` short-circuit (`class/reconstruction.cpp:1423-1523`):**
   Applying the closed-form 2-DoF `(best_R, best_t)` to `L[shard_B - 1]` and rebuilding `COR[0]` before Ceres meant `COR[0]` already sat at its 2-DoF least-squares minimum at `iter = 0`. Ceres took a zero incremental step (`R_i = I, t_i = 0`), and `if (isConverge(T, 0.1, 2.0))` (`:1521`) fired immediately at `iter == 0` (`point_to_line == false`). As a result, **`max|rotF - rot0| = 0.000000 deg` and `max|mmF - mm0| = 0.000000 mm` across all `180` Juglet and `124` `Pot_A` candidates in `arm10` and `arm11` — whole-breakline `MakeCor + P2LConstraint` (`iter = 1..49`) never executed once!** On `Pot_A`, losing `iter = 1..49` cut `CAND338` survivors from `70` (`binary_pota_stock.log`) to `33` (`binary_pota_both.log`).
2. **Bug 2 — flat-rim normal projection noise (`class/reconstruction.h:56`):**
   `Sfs357NormDot` falls back to 3D normals only when `a.norm() < 0.1 || b.norm() < 0.1`. On `Pot_A`, `7` of `15` joins sit on flat horizontal rims (`mean_nz = 0.95-0.99`), giving small horizontal components (`||n_perp|| ~ 0.14 > 0.1`) whose radial noise gets normalized to unit length in `RejectOutlier`/`CountInlier`. Combined with losing `iter = 1..49`, that stranded Piece 5 alone (`7/8 sherds, 9/15 edges` in `arm11_both`).
3. **Real assembly gains masked by `CountResult`'s world-origin lever arm (`class/data_structure.cpp:715, 1009`):**
   Evaluating `[SFS-336] FINAL` in `C:/PR/artifacts/t357/logs/binary_juglet_both_r1.log` against `GT_trans` via origin-invariant `PlaceErr` (`<= 5 deg, <= 5 mm` breakline RMS) shows that `CLUSTER 0 = {1,2,3,4,8}` seats **both `1-2` (`2.55 deg / 0.70 mm`) and `1-8` (`1.22 deg / 0.45 mm`) together (a 3-sherd sub-assembly `{1, 2, 8}`)** plus `CLUSTER 2 = {5,7}` (`4.70 deg / 1.18 mm`) — `3` accurate joins across `5` sherds vs `2` separate pairs (`1-8` and `5-7`) in `stock`. `CountResult` still printed `2/9 sherds, 1/18 edges` because Juglet breakline centroids sit `1,604.9` tree units (`535 mm`) from the world origin `(0, 0, 0)` (vs `384.3` tree units on `Pot_A`), so `isSimilarTrans(..., 0.35, 50.0)` rejects any join rotated more than `1.79 deg` (`1-2` has `tnorm = 68.91 > 50.0`; `5-7` has `125.31 > 50.0`).
4. **Where `8 of 10` in-bar Juglet joins fail downstream (`StateManager`):**
   In `arm11_both`, `3-8` (`cand 134`, `2.25 deg / 0.58 mm`, `48` inliers, `score = 0.3295`) is dropped by `OverlapCheck_3d` (`pair_overlap`, `area = 61.60`), and `1-4` (`cand 37`, `2.51 deg / 0.72 mm`, `32` inliers, `score = 0.7001`) is merged by `MergeSimPair` (`class/feature_matching.cpp:1851`) into `cand 38` (`9.42 deg / 2.40 mm`, `score = 0.4692`). The other **8** in-bar true joins (`1-2` `cand 1`, `1-8` `cand 67`, `1-9` `cand 77`, `2-9` `cand 108`, `5-6` `cand 158`, `5-7` `cand 160`, `5-8` `cand 164`, `6-9` `cand 170`) **all survive `PairwisePruning` into `CAND338`** (alongside false join `7-9` `cand 174`, `3.38 deg / 3.36 mm`). Inside `StateManager`'s `RegistrationPruning` (`class/feature_matching.cpp:2126-2129`), `Registration` (`class/reconstruction.cpp:1575, 1758`) re-solves every candidate **unlocked in 6-DoF with 3D normals** (`UsePreCorres` at `iter = 0`, `MakeSingleCorres`/`MakeMergeCorres` at `iter = 1..49`), re-hinging `5-8` (`axis_angle = 1.426 rad -> RDROP reg_axis`), dropping `1-9` (`RDROP reg_score 11` / `reg_axis 2.87 rad`), and drifting `6-9` from `2.01 deg / 0.94 mm` to `14.16 deg / 1.43 mm`.

---

## Part A — Offline Pair-to-Final Audit on Ticket 357 (`C:/PR/artifacts/t358/pair_to_final_audit358.tsv`)

Write deterministic script `C:/PR/artifacts/t358/audit_attrition358.py` reading **only** Ticket 357's existing logs and TSVs (`C:/PR/artifacts/t357/logs/binary_juglet_stock.log`, `binary_juglet_both_r1.log`, `C:/PR/artifacts/t357/factorial357_juglet.tsv`, and `C:/PR/artifacts/t355/tree/juglet` inputs) to emit **`pair_to_final_audit358.tsv` (16 rows + header)**: `14` measured Juglet joins + `2` controls (`4-7, 4-6`), with columns:

`pair  is_control  cand_idx  rotF_icp  mmF_icp  inlF_icp  scoreF_icp  pair_fate  in_cand338  cand_rot_icp  cand_mm_icp  reg_attempts  reg_kept  reg_axis_drops  reg_score_drops  reg_ovlp_drops  stock_same_cl  stock_final_rot  stock_final_mm  both_same_cl  both_final_rot  both_final_mm  both_isSim_tnorm  pass_place_err_both  pass_count_res_both`

Notes:
- Because `StateManager::PrepareNextStep` (`class/ranking_system.cpp:1375-1385`) re-runs `FeatureCompGraphBuilding` on the moved state and assigns new per-step candidate indices, match `RCAND` / `RDROP` events in `binary_juglet_both_r1.log` to each row by unordered piece pair `(min(shard_y_, shard_x_), max(shard_y_, shard_x_))` (note that in `stock` / `both`, `RCAND`/`RDROP` also record whether any candidate on that unordered piece pair survived `RegistrationPruning` at any step).
- Confirm explicitly in comments that in `binary_juglet_both_r1.log`, `FINAL` has **`3` accurate `<= 5 deg / 5 mm` joins across two clusters** (`1-2` `2.55 deg / 0.70 mm` and `1-8` `1.22 deg / 0.45 mm` in `CLUSTER 0 = {1,2,3,4,8}`; `5-7` `4.70 deg / 1.18 mm` in `CLUSTER 2 = {5,7}`), whereas `isSim_tnorm` rejects `1-2` (`68.91 >= 50.0`) and `5-7` (`125.31 >= 50.0`), leaving `CountResult` at `2/9 sherds, 1/18 edges`.

---

## Part B — Three Targeted Code Fixes in `class/`

Keep all switches default OFF so running with no env vars (or `0`) remains **bit-identical** to `arm00_stock`. Keep `class/` strictly GT-free and print startup banner lines cleanly.

1. **Fix 1 — `Icp` must not terminate at `iter == 0` under `SFS_AXIS_LOCK_ICP=1` (`class/reconstruction.cpp:1521`):**
   Change `if (isConverge(T, 0.1, 2.0))` in `Icp` to:
   `if ((!g_sfs_axis_lock_icp || iter >= 1) && isConverge(T, 0.1, 2.0))`
   so `iter = 0`'s pre-applied 2-DoF `(best_R, best_t)` never skips `iter = 1..49` (`point_to_line = true`, whole-breakline `MakeCor + P2LConstraint`). Also refresh `alock.o_B = R_i[shard_B - 1] * alock.o_B + t_i[shard_B - 1]` and `alock.u_B = R_i[shard_B - 1] * alock.u_B` OR note that at each iteration `CandidateAxisLockFunctor` operates on `L[shard_A - 1]` and `L[shard_B - 1]` in their **current** moved frames: after `UpdateTrans`, `L[shard_A - 1]` and `L[shard_B - 1]` moved by `(R_i[node], t_i[node])`, so update `alock.o_A = R_i[shard_A - 1] * alock.o_A + t_i[shard_A - 1]; alock.u_A = R_i[shard_A - 1] * alock.u_A;` and similarly `alock.o_B, alock.u_B` after `UpdateTrans` in `Icp`!
   *(Wait — verify why! In ticket 357 `UpdateTrans` only ran at `iter = 0` with `R_i ~ I, t_i ~ 0`, so `alock` didn't drift. Once `iter = 1..49` actually moves `L[shard_A - 1]` and `L[shard_B - 1]` to a refined pose via `UpdateTrans`, failing to advance `alock.o_A/u_A/o_B/u_B` by `(R_i, t_i)` would pull Ceres BACK to the `iter = 0` `(z, lateral)` position instead of keeping the two moving axes co-linear in their current moved frames! Updating `alock.o_A, u_A, o_B, u_B` by `(R_i[shard_A - 1], t_i[shard_A - 1])` and `(R_i[shard_B - 1], t_i[shard_B - 1])` right after `UpdateTrans` is essential for `iter = 1..49` to refine `spin` and `dz` cleanly without fighting `iter = 0`'s axial position!)*

2. **Fix 2 — Raise the flat-rim guard in `Sfs357NormDot` from `0.1` to `0.5` (`class/reconstruction.h:56`):**
   Change `if (a.norm() < 0.1 || b.norm() < 0.1) return n_A.dot(n_B);` to `if (a.norm() < 0.5 || b.norm() < 0.5) return n_A.dot(n_B);` so flat horizontal rims/bases (`|n . u_axis| > 0.866`, as on `Pot_A` where `mean_nz = 0.95-0.99`) keep their 3D normals rather than amplifying small horizontal noise components, while vertical/oblique Juglet breaklines (`||n_perp|| ~ 0.90-0.99 >= 0.5`) keep `(x, y)` normal comparison.

3. **Fix 3 — Add default-OFF switch `SFS_STATE_LOCK_ICP=1` for both `Registration` overloads (`class/feature_matching.cpp`, `class/feature_matching.h`, `class/reconstruction.h`, `class/reconstruction.cpp`):**
   - Declare `extern bool g_sfs_state_lock_icp;` (default `false`), read `ReadSfsSwitch("SFS_STATE_LOCK_ICP", bad_sl)` in `InitSfsSwitches()` (`class/feature_matching.cpp`), print `state lock icp: on/off` in the banner (and `[SFS-358] SFS_STATE_LOCK_ICP is not 1; state lock off` on bad non-`"1"` values).
   - Add `FixedAxisLockFunctor` in `class/reconstruction.h` matching `Registration`'s single moving block `(const Type* const s_M, const Type* const trans_M)`:
     ```cpp
     // residual[0..2] = 1000.0 * (R_M * u_M - sign * u_F)
     // residual[3..5] = 10.0 * (d - u_F.dot(d) * u_F), where d = (R_M * o_M + t_M) - o_F
     ```
   - Apply this in **both** `Registration` overloads (`5-arg` at `class/reconstruction.cpp:1575` where `c_node` moves against `set_node`; `6-arg` at `:1758` where `merge_node` moves rigidly with `c_node` against `set_node`):
     - Let `bool mov_is_x = (c_node == lcs.shard_x_);` so `aix_m = mov_is_x ? lcs.axis_index_x_ : lcs.axis_index_y_` and `aix_f = mov_is_x ? lcs.axis_index_y_ : lcs.axis_index_x_`.
     - Remember `UsePreCorres(cor_in, L, p_A, p_B, lcs, c_node, true)` (`:1129-1135`) swaps `cor_in.cor` when needed so **`cor_in.index_A == c_node` (moving `p_A`)** and **`cor_in.index_B == set_node` (fixed `p_B`)**.
     - Remember **`L[set_node - 1]` and `L[c_node - 1]` already have their `axis_point_` and `axis_norm_` moved into the current cluster frame by `Geom::Move`!** (See `RestoreShard` -> `Geom::Move` -> `AxisMove` in `class/data_structure.cpp:500-515` — unlike `PairwisePruning`'s `EdgeLineMove`, `StateManager` moves the whole `Geom` including its axis vectors before calling `RegistrationPruning`.) Double-check whether `EdgeLineMove` inside `RegistrationPruning` (`class/feature_matching.cpp:2174-2179`) resets `point_`/`normal_` without moving `axis_` — yes, `Registration` copies `L = shard.edge_line_` once at `:2095` and restores `L[i]` after each candidate with `EdgeLineMove`, so `L[i].axis_norm_[...]` and `L[i].axis_point_[...]` are intact throughout `RegistrationPruning`, whereas inside `Registration`, `L[c_node - 1]` (and `L[merge_node]` in the 6-arg overload) IS moved by `UpdateTrans` / `EdgeLineMove` (`:1720`, `:1926-1928`).
     - Therefore, when `g_sfs_state_lock_icp && iter == 0` (right after `COR[0]` is built from `UsePreCorres` and passes `MINIMUM_NUMBER`, before `ceres::Problem`):
       - Compute the closed-form 2-DoF `(best_R, best_t)` aligning `L[c_node - 1]`'s axis `(o_m, sg * u_m)` onto fixed `L[set_node - 1]`'s axis `(o_f, u_f)` (using `sg in {+1}` if `g_sfs_axis_sign`, else `sg in {+1, -1}`), picking the spin `theta` and shift `dz` minimizing `sum ||(R_cand * p_A + t_cand) - p_B||^2` over `COR[0]`.
       - Apply `(best_R, best_t)`:
         - In the 5-arg overload: `EdgeLineMove(L[c_node - 1], best_R, best_t); R[c_node - 1] = best_R * R[c_node - 1]; t[c_node - 1] = best_t + best_R * t[c_node - 1];`
         - In the 6-arg overload: update `R[c_node - 1], t[c_node - 1]` AND apply `EdgeLineMove(L[i], best_R, best_t)` to every `i` with `merge_node[i]`!
       - Clear and rebuild `COR[0]` using `UsePreCorres(cor_in, L, L[shard_A - 1], L[shard_B - 1], lcs, c_node, true)`. (If `COR[0].cor.size() < MINIMUM_NUMBER`, handle identically to empty `COR`.)
       - Store `slock.valid = true; slock.sign = best_sign; slock.o_F = o_f; slock.u_F = u_f; slock.o_M = best_R * o_m + best_t; slock.u_M = best_R * u_m;`.
     - In every iteration `iter = 0..49`:
       - When `g_sfs_state_lock_icp && slock.valid`: add `FixedAxisLockFunctor(slock.o_F, slock.u_F, slock.o_M, slock.u_M, slock.sign)` with `NULL` loss on `(s, trans)`. After `UpdateTrans` / `EdgeLineMove`, advance `slock.o_M = R_inc * slock.o_M + t_inc; slock.u_M = R_inc * slock.u_M;`, and guard convergence with `if ((!g_sfs_state_lock_icp || iter >= 1) && isConverge(T, 0.1, 2.0))`.
       - When `g_sfs_xy_break_normals && g_sfs_state_lock_icp`: set `g_sfs357_xy_axis = u_f` at the top of each iteration and before the final `MatchingScore`/`inlierCalculate` block (`:1740`, `:1948`), and `g_sfs357_xy_axis.setZero()` right after the loop (so when `SFS_STATE_LOCK_ICP=0`, `Registration` stays untouched!).

---

## Part C — 6-Arm Spartan Slurm Factorial (`Juglet` + `Pot_A`) & Render (`C:/PR/artifacts/t358/`)

Verify `SOURCE_COMMIT`, build on Spartan compute (`sbatch`), verify tree md5 unchanged before/after each run, poll all jobs (`scripts/slurm_poll.sh`), record `sacct`, and check `Arm 1` (`0,0,0`) still matches Ticket 355/357 anchors (`Juglet 0/14, 4/14, 2/9, 1/18`). Enable `SFS_PRECOR_TRACE=1 SFS_STAGE_TRACE=1 SFS_EDGE_TRACE=1 SFS_STATE_TRACE=1` on all arms (`SFS_AXIS_SIGN=1` on Juglet arms; default unsigned on `Pot_A` arms):

| Arm | Dataset | `SFS_AXIS_LOCK_ICP` | `SFS_XY_BREAK_NORMALS` | `SFS_STATE_LOCK_ICP` | Notes |
|---|---|---:|---:|---:|---|
| `juglet_stock` | Juglet | `0` | `0` | `0` | Anchor (`0/14, 4/14, 2/9, 1/18`) |
| `juglet_pair_fixed` | Juglet | `1` | `1` | `0` | Tests `iter >= 1` + `0.5` rim guard at pair stage |
| `juglet_state_locked_r1` | Juglet | `1` | `1` | `1` | Extends axis lock + `(x,y)` normals into `Registration` |
| `juglet_state_locked_r2` | Juglet | `1` | `1` | `1` | Determinism repeat (`r1 == r2`) |
| `pota_pair_fixed` | `Pot_A` | `1` | `1` | `0` | **Must restore `8/8 sherds, 15/15 edges`** |
| `pota_state_locked` | `Pot_A` | `1` | `1` | `1` | **Must hold `8/8 sherds, 15/15 edges`** |

Write in `C:/PR/artifacts/t358/` via deterministic script `C:/PR/artifacts/t358/analyze358.py`:
1. **`factorial358_all.tsv`**: every candidate row across all 6 arms (same schema as `factorial357_all.tsv` plus `max|rotF - rot0|` per arm).
2. **`factorial358_juglet.tsv`** (`16` pairs × `4` Juglet arms = `64` rows): check `rot0`, `rotF`, `max|rotF - rot0|`, `inl_F`, `score_F`, `fate`, plus final assembly columns (`final_same_cl`, `final_rot_deg`, `final_mm`).
3. **`factorial358_summary.tsv`** (`6` rows + header): columns `dataset  arm  n_measured  in_bar_0  in_bar_F  max_drot_deg  joins_inl_ge6  joins_surv  final_sherds  final_edges  place_err_joins_5_5  place_err_cluster_sherds  place_err_max_cluster`.
   *(Where `place_err_joins_5_5` counts measured GT joins in the same `CLUSTER` within `<= 5.0 deg` and `<= 5.0 mm` breakline RMS on `[SFS-336] FINAL`, and `place_err_max_cluster` names the largest `PlaceErr`-connected real component in `FINAL`.)*
4. **`render358_assembly_comparison.png`** (`>= 1800x1200 px`, labeled panels + legend): before/after 3D/side-view assembly render showing the Juglet breaklines & cluster colouring for **Col 1: Ground Truth (`Juglet`)**, **Col 2: `juglet_stock` (`1-8` and `5-7` separated)**, **Col 3: Ticket 357 `arm11_both` (`{1,2,8}` + `{5,7}`)**, and **Col 4: Ticket 358 `juglet_state_locked_r1`**, annotating exact `PlaceErr` joins (`deg / mm`) and wrongly seated sherds in each cluster. Visually inspect the PNG (`read_file`).

---

## Part D — Reading (name exactly one)

- **`state-lock-carries-pair-joins-into-multi-sherd-assembly`** — Both `pota_pair_fixed` and `pota_state_locked` pass `8/8 sherds, 15/15 edges` (`Pot_A` regression fixed by `iter >= 1` + `0.5` rim guard), `max|rotF - rot0| > 0.05 deg` proves `iter = 1..49` executed, **and** `juglet_state_locked` strictly increases `place_err_joins_5_5` beyond Ticket 357's `3` joins (`>= 5` accurate `<= 5 deg / 5 mm` joins or a `>= 4`-sherd accurate sub-assembly in `FINAL`).
- **`pair-fixes-restore-pota-but-state-selection-still-blocks-juglet`** — `pota_pair_fixed` and `pota_state_locked` both hold `8/8 sherds, 15/15 edges`, `max|rotF - rot0| > 0.05 deg` proves `iter = 1..49` executed and keeps `>= 9/14` Juglet joins in bar, **but** `juglet_state_locked` stays at `<= 4` accurate `PlaceErr` joins in `FINAL` (proving `MergeSimPair`, `OverlapCheck_3d`, false-join competition `4-7 / 7-9`, or `Priority` ranking — not `Registration`'s 6-DoF fit — blocks assembly).
- **`iter1-refinement-hurts-locked-juglet-or-breaks-pota`** — Letting `iter = 1..49` run (`iter >= 1` guard) degrades Juglet `in_bar_F` below `8/14` OR `pota_pair_fixed` / `pota_state_locked` still fails `8/8 sherds, 15/15 edges` (pinpointing where `MakeCor + P2LConstraint` or `Registration` diverges).

---

## Part E — Acceptance

- [ ] `C:/PR/artifacts/t358/pair_to_final_audit358.tsv` (16 rows + header) traces every Juglet join + `4-7, 4-6` controls from `ICP` through `PairwisePruning`, `RegistrationPruning`, and `FINAL` on Ticket 357 logs, explicitly reporting both `PlaceErr` and `isSimilarTrans` world-origin `tnorm`.
- [ ] All three `class/` fixes (`iter >= 1` guard + `alock` propagation in `Icp`, `0.5` guard in `Sfs357NormDot`, default-OFF `SFS_STATE_LOCK_ICP` in `InitSfsSwitches()` and both `Registration` overloads) are strictly GT-free, alter zero shipped thresholds, and `juglet_stock` matches Ticket 355/357 anchors (`0/14, 4/14, 2/9, 1/18`).
- [ ] In `factorial358_all.tsv`, `max|rotF - rot0| > 0.05 deg` on `juglet_pair_fixed` and `pota_pair_fixed` (proving `iter = 1..49` executed).
- [ ] All 6 Spartan Slurm jobs `COMPLETED 0:0`, `sacct` recorded, tree md5 unchanged before/after, and `juglet_state_locked_r1` vs `r2` agree cleanly (`PlaceErr` joins & per-join table).
- [ ] `Pot_A` holds `8/8 sherds, 15/15 edges` under both `pota_pair_fixed` and `pota_state_locked`, OR any failure is diagnosed explicitly.
- [ ] `C:/PR/artifacts/t358/factorial358_summary.tsv` (6 rows), `factorial358_juglet.tsv` (64 rows), `factorial358_all.tsv`, and `render358_assembly_comparison.png` (`>= 1800x1200 px`) are written and visually inspected (`read_file`).
- [ ] Write-back ends with one Part D `Reading:` slug, exact tables, `verified:` lines, and `**End state:**` (`ready-to-land`, `incomplete`, `blocked`, or `stopped`).
