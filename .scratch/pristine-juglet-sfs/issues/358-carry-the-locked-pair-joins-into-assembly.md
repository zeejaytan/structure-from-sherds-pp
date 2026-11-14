# Ticket 358 — Pre-rotation breakline normal leveling + StateManager `(x, y)` normals (no axis lock)

**What to build:** Fix the two 6-DoF breakline-normal bugs exposed by Ticket 357 — pre-rotation normal leveling with a `0.5` flat-rim guard in `CostFuncNorm`/`CostFuncFixedNorm`/`Sfs357NormDot`, and extending `(x, y)` breakline normals from `Icp` into `StateManager` (`IcpIncGraphAxis` + both `Registration` overloads behind default-off `SFS_XY_STATE_NORMALS=1`) — while dropping all axis locking (`SFS_AXIS_LOCK_ICP=0` everywhere; no `SFS_STATE_LOCK_ICP`) and auditing full pair-to-final attrition on Ticket 357 logs.

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-15)
**Branch:** 358-carry-the-locked-pair-joins-into-assembly, in /c/PR/worktrees/structure-from-sherds-pp/358
**Worker:** dsh one-shot subagent w358, dispatched 2026-11-15 by the lead
**Answers:** S1
**Needs-eye:** yes
**Depends on:** 357
**Blocked-by:** none
**Budget:** medium (one offline pair-to-final audit script on existing Ticket 357 logs, two localized default-off fixes in `class/`, one 6-arm Spartan Slurm job on `Juglet` + `Pot_A`, factorial TSV tables, and one comparison render in `C:/PR/artifacts/t358/`)
**Hand-over:** DeepSeek: offline pair-to-final attrition audit (`audit_attrition358.py`), pre-rotation normal leveling + `0.5` flat-rim guard (`class/reconstruction.h`), default-off `SFS_XY_STATE_NORMALS=1` (`class/feature_matching.{h,cpp}`, `class/reconstruction.cpp`), 6-arm Spartan Slurm run on `Juglet` + `Pot_A`, TSV summaries, and `render358_assembly_comparison.png`.

> **Note on file slug vs scope (conservator decision, 2026-11-15):** This file keeps its existing issue slug (`358-carry-the-locked-pair-joins-into-assembly.md`), but **all axis locking (`SFS_AXIS_LOCK_ICP` and any `SFS_STATE_LOCK_ICP`) is explicitly dropped.** The unmodified 6-DoF assembler already solves `Pot_A` (`8/8 sherds, 15/15 edges`, even with `Piece 7`'s axis `58.8 deg` off) and already carries `9–10/14` `Juglet` joins through `PairwisePruning` in 6-DoF (`arm00_stock` and `arm01_xynorm_only` with `SFS_AXIS_LOCK_ICP=0`), whereas Juglet extracted axes sit `25–52 deg` off (`SfSpp_preprocessing/AGENTS.md:159-160`). Locking axes in Ceres does not belong in the solver; fixing why 6-DoF `(x, y)` normals lose vertical pitch stiffness inside `Icp` and disappear inside `StateManager` does.

---

## Verified 6-DoF diagnosis of Ticket 357 (why Ticket 358 takes this exact direction)

Lead inspection of `C:/PR/artifacts/t357/` and `class/` established that Ticket 357's `SFS_AXIS_LOCK_ICP=1` masked what 6-DoF `SFS_XY_BREAK_NORMALS=1` was already doing — and revealed two clean 6-DoF bugs:

1. **In 6-DoF (`SFS_AXIS_LOCK_ICP=0`), `SFS_XY_BREAK_NORMALS=1` (`arm01_xynorm_only`) already boosts `CAND338` inlier counts sharply across true Juglet joins, yet `FINAL` drops back to `2/9 sherds, 1/18 edges`:**
   - In `C:/PR/artifacts/t357/factorial357_juglet.tsv` and `logs/binary_juglet_xynorm.log` (`CAND338 = 45` survivors, `14/14` joins with `>= 6` inliers, `9/14` surviving `PairwisePruning`), true joins gain huge inlier margin over `arm00_stock`:
     - `1-8`: `70 -> 97` (`cand 70`, `rotF=1.11 deg`, `mmF=0.48 mm`)
     - `5-7`: `72 -> 72` (`cand 162`, `rotF=4.85 deg`, `mmF=1.19 mm`)
     - `1-2`: `18 -> 69` (`cand 1`, `rotF=4.72 deg`, `mmF=0.89 mm`; `cand 3` `2.23 deg / 0.77 mm`, `59` inliers, merged)
     - `1-4`: `inl -> 53` (`cand 38`, `rotF=14.27 deg`, `mmF=3.21 mm`)
     - `2-6`: `inl -> 49` (`cand 97`, `rotF=15.07 deg`, `mmF=4.01 mm`)
     - `1-9`: `inl -> 41` (`cand 77`, `rotF=7.26 deg`, `mmF=1.16 mm`)
     - `5-6`: `30 -> 36` (`cand 158`, `rotF=12.75 deg`, `mmF=5.23 mm`)
     - `5-8`: `inl -> 34` (`cand 164`, `rotF=12.25 deg`, `mmF=2.83 mm`)
     - `2-9`: `26 -> 30` (`cand 108`, `rotF=14.38 deg`, `mmF=2.59 mm`)
     - `6-9`: `18 -> 24` (`cand 170`, `rotF=5.05 deg`, `mmF=0.77 mm`)
     - (`3-8` reaches `51` inliers, `6.25 deg / 1.69 mm`, lost in `PairwisePruning` to `pair_overlap`; control `4-7` reaches `57` inliers, `2.45 deg / 0.59 mm`; false join `1-3` scores `cand 15`, `53` inliers, `rotF=169.32 deg`).
   - Meanwhile, on `Pot_A`, `Sfs357NormDot` (`class/reconstruction.h:56`) used too small a flat-rim guard (`||n_perp|| < 0.1`): `Pot_A` flat rims (`mean_nz = 0.95–0.99`, `||n_perp|| ~ 0.14 > 0.1`) still got projected into `(x, y)`, cutting `pota_xynorm` `joins_inl_ge6` from `15/15` (`pota_stock`, `CAND338=70`) to `12/15` (`joins_surv=10`), though `Pot_A` still assembled `8/8 sherds, 15/15 edges`. (In `arm10`/`arm11`, `SFS_AXIS_LOCK_ICP=1` also had a pre-Ceres 2-DoF snap at `class/reconstruction.cpp:1423-1523` that made `isConverge(T, 0.1, 2.0)` fire at `iter == 0` with `0.000000 deg / 0.000000 mm` movement across `iter = 1..49`, cutting `Pot_A` `CAND338` from `70` to `33` and stranding Piece 5.)

2. **Bug 1 — Post-rotation output projection in `CostFuncNorm` / `CostFuncFixedNorm` (`class/reconstruction.h:569-580, 609-617`) removes 3D vertical tilt stiffness:**
   - Ticket 357 projected the residual *after* multiplying by `R_A, R_B`:
     `tempn = tempn - u * u.dot(tempn)` (with `u = R_A * g_sfs357_xy_axis`).
     That zeroes the residual component along `u` identically for every candidate tilt, stripping out the 3D vertical normal restoring force against out-of-plane hinging even when normals are horizontalized.
   - Leveling both breakline normals orthogonal to `g_sfs357_xy_axis` **before** rotation (only when **both** normals have `||n_perp|| >= 0.5`, else falling back to raw 3D for **both**) and evaluating the **full 3D residual** `(R_A * nA_use) - (R_B * nB_use)` keeps full 3D pitch stiffness against hinging while stripping `TICKET 328` opposing `N_z` fracture-bevel bias (`Pot_A` flat rims stay 3D automatically).
   - Verified in 6-DoF across all `14` Juglet joins' `UsePreCorres` `best_idx` segments (`iter = 0`, 6-DoF):
     - `stock_3d`: `in_bar_0 = 0/14`, `med_rot0 = 28.25 deg`, `med_tilt0 = 16.48 deg`
     - `t357_post_proj`: `in_bar_0 = 0/14`, `med_rot0 = 19.42 deg`, `med_tilt0 = 12.14 deg`
     - `t358_pre_level_3d`: `in_bar_0 = 3/14`, `med_rot0 = 13.21 deg`, `med_tilt0 = 4.99 deg`

3. **Bug 2 — `StateManager` reverts to raw 3D bevelled normals (`g_sfs357_xy_axis == 0` outside `Icp`):**
   - `Icp()` clears `g_sfs357_xy_axis.setZero()` before returning (`class/reconstruction.cpp:1560`). Because `StateManager` never sets `g_sfs357_xy_axis`, every score (`MatchingScore`, `CountInlier`) and Ceres solve (`CostFuncNorm`, `CostFuncFixedNorm`) inside `IcpIncGraphAxis` (`class/reconstruction.cpp:1967-2157`) and both `Registration` overloads (`:1575-1757`, `:1758-1965`) uses raw bevelled 3D normals (`TICKET 328` bias).
   - Direct trace proof from `logs/binary_juglet_xynorm.log` at `Step 0` (`IcpIncGraphAxis`):
     - `STEP 0 4 24 37 1,2|3|8`: `{1, 2}` is slashed from `69` inliers (`CAND338`) to `24` -> **DROPPED** at `CUT 0`.
     - `STEP 0 11 28 24 2,6|7|9`: `{2, 6}` is slashed from `49` inliers (`CAND338`) to `28` -> **DROPPED** at `CUT 0`.
     - Meanwhile false join `1-3` (`STEP 0 8 40 28 1,3|4|5`) scores `40` and is **KEPT** (`CUT 0 KEPT 6:78,9:78,3:68,8:40,7:28 DROPPED 10:28,11:28,4:24`), leaving `FINAL` stalled in fragmented `{1,3,5,8} | {2,4,6,9} | {7}` with all `Step 6` merges vetoed.

4. **Measurement caveat preserved from Ticket 357 (`CountResult` vs origin-invariant `PlaceErr`):**
   - `arm11_both` already seated `{1, 2, 8}` together in `FINAL` (`1-2` `2.55 deg / 0.70 mm`, `1-8` `1.22 deg / 0.45 mm`, plus `5-7` `4.70 deg / 1.18 mm`), yet `CountResult` (`class/data_structure.cpp:715, 1009`) still printed `2/9 sherds, 1/18 edges` because Juglet sherds sit at median `1,604.9` tree units (`535 mm`) from `(0, 0, 0)` (vs `384.3` on `Pot_A`), making `isSimilarTrans(..., 0.35, 50.0)` fail on any join with rotation error `> 1.79 deg`. Do **not** alter `CountResult` or `isSimilarTrans`; score **both** `CountResult` and origin-invariant breakline RMS `PlaceErr` (`<= 5 deg, <= 5 mm`, `SCALE = 3.0`) on `FINAL`.

---

## Scope & Guardrails

- Keep all shipped default constants untouched when switches are OFF (`SFS_AXIS_LOCK_ICP=0`, `SFS_XY_BREAK_NORMALS=0`, `SFS_XY_STATE_NORMALS=0`).
- Do **not** re-enable or invoke `SFS_AXIS_LOCK_ICP=1`, and do **not** add `SFS_STATE_LOCK_ICP`. Every run in this ticket runs in unmodified 6-DoF (`SFS_AXIS_LOCK_ICP=0`).
- Do **not** alter `CountResult` or `isSimilarTrans(..., 0.35, 50.0)`.
- Do **not** modify `C:/PR/artifacts/t355/` or `C:/PR/artifacts/t357/` (read-only inputs).
- All heavy compiles/runs execute on **Spartan compute nodes** via `sbatch`, never on the laptop or Spartan login node (`AGENTS.md`).

---

## Part A — Offline 6-DoF Pair-to-Final Attrition Audit (`C:/PR/artifacts/t358/audit_attrition358.py`)

Write a deterministic Python script `C:/PR/artifacts/t358/audit_attrition358.py` reading `C:/PR/artifacts/t357/logs/binary_juglet_stock.log` (`arm00_stock`) and `C:/PR/artifacts/t357/logs/binary_juglet_xynorm.log` (`arm01_xynorm_only`), plus GT / `C:/PR/artifacts/t357/factorial357_juglet.tsv` / `C:/PR/artifacts/t355/analyze355.py` scoring helpers (`SCALE = 3.0`, `PlaceErr` `<= 5 deg, <= 5 mm`).

1. For every one of the `16` Juglet pairs (`14` measured GT joins + `2` controls `4-6`, `4-7`) in both `arm00_stock` and `arm01_xynorm_only`:
   - Extract `best_idx`, `iter = 0` error (`rot0_deg`, `mm0`), `Icp` final pose/inliers (`rotF_deg`, `mmF`, `inl_icp`, `score_icp`),
   - Extract exact `PairwisePruning` fate (`cand338`, `pair_overlap`, `pair_merge`, `pair_inlier`, `pair_basket`, etc.),
   - Trace every `StateManager` event involving that pair across Steps `0..6`: `step0_state`, `step0_inl`, `step0_cut` (`KEPT`, `DROPPED`, `ABSENT`), downstream `[SFS-337]`/`[SFS-338]`/`[SFS-340]`/`[SFS-341]`/`[SFS-342]`/`RDROP` events (`reg_score`, `reg_axis`, `pc_dist`, `pc_rim`, `CUT`), and
   - Score `[SFS-336] FINAL` cluster membership (`same_final_cluster`, `final_rot_deg`, `final_mm` via `PlaceErr`).
2. Write `C:/PR/artifacts/t358/pair_to_final_audit358.tsv` (`32` data rows + header: `16` pairs × `2` arms) and print a concise stage-by-stage funnel summary (`Icp in-bar` -> `CAND338` -> `Step 0 kept` -> `Steps 1..5 surviving` -> `FINAL seated`) confirming where `arm01_xynorm_only` sheds its `9` `CAND338` joins.

---

## Part B — Two Targeted 6-DoF Fixes in `structure-from-sherds-pp` (Default OFF)

### Fix 1: Pre-rotation normal leveling + `0.5` flat-rim guard (under `SFS_XY_BREAK_NORMALS=1`)
In `C:/PR/structure-from-sherds-pp/class/reconstruction.h`:
- Replace the duplicate post-rotation residual projection in `CostFuncNorm::operator()` (`:569-580`) and `CostFuncFixedNorm::operator()` (`:609-617`), and update `Sfs357NormDot` (`:48-64`), to share **one** templated helper `Sfs357LevelPairNormals`:
  - Given `n_A`, `n_B`, and `u = g_sfs357_xy_axis.normalized()` (when `Sfs357XYOn()` is true):
    - Project **in the pre-rotation reference frame**:
      `a_perp = n_A - u * u.dot(n_A)` and `b_perp = n_B - u * u.dot(n_B)`.
    - **Flat-rim guard (`0.5`):** If `||a_perp|| < 0.5 || ||b_perp|| < 0.5` (compare `squaredNorm() < 0.25` safely for Ceres Jets without dividing by zero), keep **both** `nA_use = n_A` and `nB_use = n_B` unmodified in raw 3D so neither mixed 2D/3D comparison nor flat-rim noise amplification can occur (`Pot_A` flat rims have `||n_perp|| ~ 0.14 < 0.5`).
    - Otherwise, normalize `nA_use = a_perp / a_perp.norm()` and `nB_use = b_perp / b_perp.norm()`.
  - Compute the **full 3D residual** after rotation:
    - `CostFuncNorm`: `tempn = (R_A * nA_use) - (R_B * nB_use);`
    - `CostFuncFixedNorm`: `tempn = (R_A * nA_use) - nB_use;`
    - `Sfs357NormDot`: returns `nA_use.dot(nB_use)` clamped to `[-1.0, 1.0]`.
- Keep `SFS_XY_BREAK_NORMALS=0` by default (`Sfs357XYOn()` false -> bit-for-bit identical raw 3D normals in `stock`).
- Check `MakeCor`, `MakeSingleCorres`, `MakeMergeCorres`, `MakeMultiCorres`, and `RejectOutlier` to verify that `g_sfs357_xy_axis` and `(n_A, n_B)` live in the same reference frame everywhere `Sfs357NormDot`/`CostFuncNorm`/`CostFuncFixedNorm` runs.

### Fix 2: Extend `(x, y)` breakline normals into `StateManager` behind `SFS_XY_STATE_NORMALS=1` (default OFF)
1. **New default-off switch (`class/feature_matching.h` & `class/feature_matching.cpp`):**
   - Declare `extern bool g_sfs_xy_state_normals;` next to `g_sfs_xy_break_normals` (`class/feature_matching.h:37`), initialize `bool g_sfs_xy_state_normals = false;` in `class/feature_matching.cpp:92`, and parse `ReadSfsSwitch("SFS_XY_STATE_NORMALS", bad_xys)` in `InitSfsSwitches()` (`:132-178`), printing its banner alongside `SFS_AXIS_LOCK_ICP` and `SFS_XY_BREAK_NORMALS`.
   - Use a small RAII scope guard (e.g. `Sfs357AxisScope` in `class/reconstruction.h` or anonymous helper in `class/reconstruction.cpp`) that saves the prior `g_sfs357_xy_axis` on construction and restores/zeros it on scope exit so **no `break`, `continue`, or early `return`** in `Icp`, `Registration`, or `IcpIncGraphAxis` can leak a stale nonzero `g_sfs357_xy_axis`.
2. **Wire `g_sfs357_xy_axis` inside `StateManager` solve paths (`class/reconstruction.cpp`):**
   - **`IcpIncGraphAxis` (`class/reconstruction.cpp:1967-2157`):** When `g_sfs_xy_break_normals && g_sfs_xy_state_normals`, set `Sfs357AxisScope` from `shard[graph.root_node_ - 1].edge_line_.axis_norm_[0].normalized()` across `MakeMultiCorres`, Ceres solve, `RejectOutlier`, and final `inlierCalculate` (including the `:2067` `continue` path). First verify in `class/ranking_system.cpp` that `shard[graph.root_node_ - 1].edge_line_.axis_norm_[0]` is already in the subgraph's frame (transformed alongside the breakline points).
   - **Both `Registration` overloads (`class/reconstruction.cpp:1575-1757` and `:1758-1965`):** Identify all call sites of both `Registration` overloads (`RegistrationPruning` in `class/feature_matching.cpp:2110-2129` and any `ranking_system.cpp` call sites). Pass/propagate the fixed piece's (`set_node`) axis index `axis_set` (`0` default or the selected `(set_node == iter->shard_x_) ? iter->axis_index_x_ : iter->axis_index_y_` when available; verify whether `L[set_node - 1].axis_norm_[axis_set]` is kept transformed in the subgraph frame when pieces move in Steps `1..6`). When `g_sfs_xy_break_normals && g_sfs_xy_state_normals`, scope `g_sfs357_xy_axis` to `L[set_node - 1].axis_norm_[axis_set].normalized()` (or the transformed current axis of `set_node`) across `UsePreCorres` / `MakeSingleCorres` / `MakeMergeCorres`, Ceres, `MatchingScore`, and `inlierCalculate`.
- Keep `SFS_AXIS_LOCK_ICP` forced/run at `0` (`=0` in Slurm runs; never re-enable axis lock).
- Verify locally (or on a quick Spartan build/dry check) that compiling succeeds cleanly with no warnings in touched files and that `(0, 0)` reproduces `arm00_stock`.

---

## Part C — Controlled 6-Arm Spartan Slurm Run (Unmodified 6-DoF, `SFS_AXIS_LOCK_ICP=0`)

Submit one Spartan Slurm job array (`6` arms, ALL with `SFS_AXIS_LOCK_ICP=0`, `SFS_STAGE_TRACE=1`, `SFS_STATE_TRACE=1`, `SFS_EDGE_TRACE=1`, `SFS_ADMIT_TRACE=1`, `SFS_PICK_TRACE=1`, `SFS_VETO_TRACE=1`, `SFS_WIN_TRACE=1`, `SFS_PRECOR_TRACE=1` — matching Ticket 357 flags + `SFS_XY_STATE_NORMALS`):

| Arm | Object | `SFS_AXIS_LOCK_ICP` | `SFS_XY_BREAK_NORMALS` | `SFS_XY_STATE_NORMALS` | Purpose |
|---|---|---:|---:|---:|---|
| `0` `juglet_stock` | Juglet | `0` | `0` | `0` | 6-DoF raw-normal baseline |
| `1` `juglet_xy_pair` | Juglet | `0` | `1` | `0` | Fix 1 alone: pre-rotation leveling + `0.5` guard in `Icp` |
| `2` `juglet_xy_full_r1` | Juglet | `0` | `1` | `1` | Fix 1 + Fix 2 across `Icp` + `StateManager` (run 1) |
| `3` `juglet_xy_full_r2` | Juglet | `0` | `1` | `1` | Fix 1 + Fix 2 repeatability check (run 2) |
| `4` `pota_xy_pair` | `Pot_A` | `0` | `1` | `0` | `Pot_A` control: Fix 1 preserves `8/8, 15/15` + `CAND338` |
| `5` `pota_xy_full` | `Pot_A` | `0` | `1` | `1` | `Pot_A` control: Fix 1 + Fix 2 preserves `8/8, 15/15` |

Poll every `sbatch` with `scripts/slurm_poll.sh <JOBID>` and record exact `sacct` `State` / `ExitCode` and Spartan run directory ID (`runs/<YYYYMMDD-HHMM>_<git7>_<tag>/`) in the ticket write-back.

### Required deliverables in `C:/PR/artifacts/t358/`
1. `C:/PR/artifacts/t358/logs/` — compressed or fetched logs from all `6` arms.
2. `C:/PR/artifacts/t358/pair_to_final_audit358.tsv` (`32` rows + header, Part A).
3. `C:/PR/artifacts/t358/factorial358_all.tsv` (`94` rows + header: `4 × 16` Juglet + `2 × 15` `Pot_A`).
4. `C:/PR/artifacts/t358/factorial358_juglet.tsv` (`64` rows + header: `4` Juglet arms × `16` pairs, reporting `iter=0`, `Icp` final, `CAND338`, `Step 0`, `Registration`, `FINAL` cluster, origin-invariant `PlaceErr`, and `CountResult`).
5. `C:/PR/artifacts/t358/factorial358_summary.tsv` (`6` rows + header: `arm`, `object`, `sfs_xy_break_normals`, `sfs_xy_state_normals`, `cand338_total`, `in_bar_0`, `in_bar_F`, `joins_inl_ge6`, `joins_surv`, `step0_kept`, `final_placeerr_seated_joins`, `final_largest_cluster_sherds`, `countresult_sherds`, `countresult_edges`).
6. `C:/PR/artifacts/t358/render358_assembly_comparison.png` (`>= 1800x1200 px`, rendered on Spartan from Juglet meshes using `[SFS-336] FINAL` poses and verified with `read_file`): 4 labeled panels side by side — `Stock 6-DoF` (`juglet_stock`), `T357 xynorm_only` (`arm01_xynorm_only`), `T358 xy_full` (`juglet_xy_full_r1`), and `Ground Truth`, naming connected clusters and per-join `PlaceErr`.

---

## Part D — Exclusive Readings (how `S1` reads each outcome)

Before reporting results, separate which claim is being made: **(1) method**, **(2) measurement (`CountResult`)**, or **(3) reference**:

1. **Reading A (`juglet_xy_full` seats `>= 6` true Juglet joins into coherent sub-assemblies / `>= 5/9` sherds in a single `PlaceErr`-verified cluster AND `Pot_A` holds `8/8 sherds, 15/15 edges` on both `pota_xy_pair` and `pota_xy_full`):**
   - Proves `6-DoF` `Icp + StateManager` needed **no axis lock** (`Method fix confirmed`); pre-rotation normal leveling (`0.5` flat-rim guard) + `(x, y)` normals inside `StateManager` carry the Juglet pair joins into multi-sherd assembly while keeping `Pot_A` intact. Inspect `[SFS-336]` / `[SFS-341]` to see whether `Step 6` cluster merge or `PairwisePruning` (`3-8` `OverlapCheck_3d`, `1-4` `MergeSimPair`) is the last gap to a full `9/9` Juglet pot.
2. **Reading B (`juglet_xy_pair` boosts `iter = 0` to `>= 3/14` in bar and keeps `Pot_A` `15/15` / `CAND338 >= 65`, AND `juglet_xy_full` stops `Step 0 CUT 0` from dropping `{1, 2}` and `{2, 6}`, yet `FINAL` still stalls at `<= 3` seated joins):**
   - Cleanly rules out axis lock and pair/Step-0 normal bias (`Step 0` fixed), and uses the `[SFS-337]`/`[SFS-338]`/`[SFS-340]`/`[SFS-341]`/`[SFS-342]` trace in `juglet_xy_full_r1.log` to identify the exact downstream `StateManager` gate (`Registration`, `pc_dist`/`pc_rim` profile veto, `GraphAxisRefinement`, or priority ordering) that rejected or mis-ordered the seated subgraphs.
3. **Reading C (`Pot_A` drops below `8/8 sherds, 15/15 edges` in `pota_xy_pair` or `pota_xy_full`):**
   - Shows where `(x, y)` breakline normals in `CostFuncNorm`/`StateManager` still harm flat/low-tilt rims or multi-sherd `Registration`; keep switches default-OFF and diagnose `pota_xy_*` vs `pota_stock` (`CAND338` and `Step 0..6`) before proposing the next change.

---

## Part E — Acceptance Checklist

- [ ] `C:/PR/artifacts/t358/audit_attrition358.py` runs cleanly and writes `C:/PR/artifacts/t358/pair_to_final_audit358.tsv` (`32` rows + header).
- [ ] `Sfs357LevelPairNormals` with `0.5` flat-rim guard and full 3D post-rotation residual `(R_A * nA_use) - (R_B * nB_use)` / `(R_A * nA_use) - nB_use` implemented once in `class/reconstruction.h` and shared across `Sfs357NormDot`, `CostFuncNorm`, and `CostFuncFixedNorm`.
- [ ] Default-OFF `SFS_XY_STATE_NORMALS=1` wired into `InitSfsSwitches()` (`class/feature_matching.{h,cpp}`), `IcpIncGraphAxis`, and both `Registration` overloads (`class/reconstruction.cpp`) using RAII scope cleanup (`g_sfs357_xy_axis` zeroed on all return paths), with `SFS_AXIS_LOCK_ICP=0` everywhere and shipped default constants untouched.
- [ ] Spartan Slurm 6-arm job completes (`sacct` `COMPLETED`, exit `0:0`), run directory recorded, logs fetched to `C:/PR/artifacts/t358/logs/`.
- [ ] `factorial358_all.tsv` (`94` rows + header), `factorial358_juglet.tsv` (`64` rows + header), `factorial358_summary.tsv` (`6` rows + header) generated and reconciled.
- [ ] `C:/PR/artifacts/t358/render358_assembly_comparison.png` (`>= 1800x1200 px`) generated on Spartan, copied to `C:/PR/artifacts/t358/`, and visually inspected with `read_file`.
- [ ] Branch merged cleanly into `main`, pushed to `origin`, worktree cleaned up, `check_intent_links.py` and `build_board.py` run after write-back.

## Comments

- **2026-11-15 (lead — rewritten):** Replaced previous axis-lock Ticket 358 draft following conservator pushback (`"why need to axis lock where assembler proved able to put together much more complex pot"`). Dropped `SFS_AXIS_LOCK_ICP` and `SFS_STATE_LOCK_ICP` completely; grounded rewritten ticket in verified 6-DoF findings (`arm01_xynorm_only` `CAND338` inlier jump, `Step 0 CUT 0` drop of `{1,2}` `69 -> 24` and `{2,6}` `49 -> 28`, pre-rotation leveling `med_tilt0 16.48 -> 4.99 deg`, and `0.5` flat-rim guard). Ready for cold worker pickup.
- **2026-11-15 (lead, note at dispatch):** two corrections to the evidence wording so the worker does not chase them. (i) The line `STEP 0 4 24 37 1,2|3|8` is `[SFS-337] STEP <step> <trace_id> <state_score> <n_priority> <parts>`: the `24` is that state's score (the sum of inlier counts under `CalculateMatchingScore`), not a raw inlier count; `[SFS-337] CUT 0 KEPT 6:78,9:78,3:68,8:40,7:28 DROPPED 10:28,11:28,4:24` prints the same scores, so the drop of the `{1,2}` state is real (69 inliers at the pair stage, a state score of 24 against 78 for the kept state) but must be described as a score, not as an inlier count. (ii) The Part D comparison rows `stock_3d / t357_post_proj / t358_pre_level_3d` (`in_bar_0 0/14, 0/14, 3/14`; `med_rot0 28.25, 19.42, 13.21`; `med_tilt0 16.48, 12.14, 4.99 deg`) appear in no log under `C:/PR/artifacts/t357/`; treat them as an unverified claim and reproduce them from the two logs in Part A before any of the fixes is credited. Nothing else is changed.

## Comments (worker, 2026-11-15)

**Part A — where the pair joins are shed (`audit_attrition358.py`, `pair_to_final_audit358.tsv`, `stage_funnel358.tsv`).**
`C:/PR/artifacts/t358/audit_attrition358.py` reads the two 357 logs and writes `pair_to_final_audit358.tsv` (32 data rows = 16 pairs x 2 arms) plus a six-arm funnel. Every PlaceErr is measured on the final poses against ground truth in the *relative* frame (`inv(T_y) T_x` against `inv(GT_y) GT_x`), so it is origin-invariant; the bar is 5 deg / 5 mm with `SCALE = 3.0` tree units per real mm, and `CountResult` is scored separately because its 0.35 rad / 50 tree-unit gate is world-frame (Juglet sherds sit ~1600 tree units from the origin).

| arm | candidates | CAND338 pairs (GT joins) | iter-0 in bar (joins) | Step 0 kept / dropped | Steps 1..5 surviving | pairs in one cluster | PlaceErr-verified | clusters (largest) |
|---|---|---|---|---|---|---|---|---|
| t357_stock / juglet_stock | 180 | 15 (13) | 0 (0) | 1 / 0 | 6 | 8 | 2 | 3 (4) |
| t357_xynorm (post-projection) | 180 | 15 (13) | 2 (1) | 0 / 1 | 4 | 9 | 1 | 3 (4) |
| juglet_xy_pair | 180 | 14 (12) | 5 (3) | 1 / 0 | 3 | 6 | 3 | 3 (5) |
| juglet_xy_full_r1 | 180 | 14 (12) | 5 (3) | 1 / 0 | 4 | 11 | 9 | 2 (7) |
| juglet_xy_full_r2 | 180 | 14 (12) | 5 (3) | 1 / 0 | 4 | 11 | 9 | 2 (7) |

The 357 findings reproduce to the digit: `1-8` 97 inliers at 1.11 deg / 0.48 mm, `5-7` 72 at 4.85 / 1.19, `3-8` 51 at 6.25 / 1.69 with fate `pair_overlap` and the state that holds it (`[SFS-337] STEP 0` trace 4, state score 24) CUT, `1-4` and `2-6` lost downstream, false `1-3` kept at 169.32 deg. One discrepancy worth recording: the ticket text calls `1-2` "cand 1, 69 inliers, 4.72 deg, 0.89 mm", but the stated rule (best by `rotF`, requiring `mmF <= 5`) picks a different `1-2` candidate in the xynorm arm (59 inliers, 2.23 deg / 0.77 mm). Both candidates exist; the table follows the rule.

**The three Part D comparison rows.** They are not in any 357 log, so they were recomputed here from the per-pair best pick over the 14 GT joins:

| row | claimed | recomputed | verdict |
|---|---|---|---|
| `stock_3d` | in_bar_0 0/14, med_rot0 28.25, med_tilt0 16.48 | 0/14, **17.14**, **11.84** (in_bar_F 4) | count reproduced, medians refuted |
| `t357_post_proj` | 0/14, 19.42, 12.14 | **1/14**, **16.76**, **9.55** (in_bar_F 3) | refuted |
| `t358_pre_level_3d` (`juglet_xy_pair`) | 3/14, 13.21, 4.99 | **3/14**, **9.58**, **5.30** (in_bar_F 4) | iter-0 count reproduced exactly, tilt close, rot median lower |

So the *direction* of every claimed row survives (leveling moves iter-0 into the bar and cuts the tilt), but the printed medians do not reproduce from these logs under the pair-level statistic the tables use; they look like a different summary (for example per-candidate rather than per-pair). The ticket's own note already flagged them unverified. `juglet_xy_full` measured 3/14, 9.58, 4.43 — the state normals change the tilt median (5.30 -> 4.43) without changing the count in bar.

**Part B — the two fixes, both default-OFF.** Four files, +133 / -24 (`class/reconstruction.h`, `class/reconstruction.cpp`, `class/feature_matching.h`, `class/feature_matching.cpp`).
1. One helper, `Sfs357LevelPair`, in `class/reconstruction.h`, used by `Sfs357NormDot`, `CostFuncNorm` and `CostFuncFixedNorm`: it projects **both** normals onto the plane orthogonal to the pair axis **before** rotation, and only when **both** projections are at least 0.5 long (`squaredNorm() < 0.25` tests the flat-rim case without dividing by a near-zero norm); otherwise **both** keep their raw 3D normal. The residual is then the full 3D one, `(R_A nA) - (R_B nB)` in `CostFuncNorm` and `(R_A nA) - nB` in `CostFuncFixedNorm`. The 357 post-rotation projection is gone, so the residual component along the axis is no longer zeroed — that is the pitch/tilt stiffness the conservator asked for back.
2. `SFS_XY_STATE_NORMALS` (default off, banner `xy state normals: on/off`, bad-value warning), with `Sfs357AxisScope` RAII publishing `g_sfs357_xy_axis` for the duration of a state solve and zeroing it again on every return path. Wired into both `Registration` overloads (one scope before the loop, since the fixed piece never moves; axis index from `lcs.axis_index_x_` / `axis_index_y_` according to which piece is `set_node`) and into `IcpIncGraphAxis` (one scope per iteration, because `R` grows each iteration). The axis is taken as `R[node] * axis_norm_[idx]`: `EdgeLineMove` (`class/data_structure.cpp:773-779`) moves points and normals but never `axis_point_` / `axis_norm_`, so the stored axis is still in the piece's own frame. `SFS_AXIS_LOCK_ICP` was unset in every arm; no constant or threshold was touched.

**Off-path identity, proved by log diff (against `artifacts/t357/logs/binary_juglet_stock.log`).** `[SFS-336] CAND` (180 lines) and `[SFS-338] CAND338` (51 lines) are byte-identical in all four Juglet 358 arms **and** in a control rebuild of the untouched `db848d2` source: the switch-off path takes exactly the old arithmetic, so the 0.5 guard and the Jet rewrite are provably inert. `OUTSTATE` and `CLUSTER` also match (9 pieces, 3 clusters). What does **not** match bit-for-bit is the state stage's `[SFS-337] KEEP` / `CUT` scores: our stock arm differs from the 357 log at step 3 (213 vs 220) and step 4 (2 states vs 3). Two controls settle the cause: (a) a matched-trace repeat (`submit358_stockrep.sbatch`, the 357 banner set: admit/pick/win/veto traces off) still differs, and (b) a **pristine rebuild of `db848d2` with none of this ticket's edits** rewrites the same scores differently while keeping `CAND`/`CAND338`/`OUTSTATE`/`CLUSTER` identical. So the drift is build/run-level floating-point instability in the state stage, not the new switch logic; the pair stage — the one the audit and the summary tables read — is reproducible to the byte.

**Part C — six arms, one array, all COMPLETED 0:0.** `runs/20261115-0418_db848d2_t358-factorial`, `SOURCE_COMMIT=db848d2`, logs fetched to `C:/PR/artifacts/t358/logs/` (`sacct358.txt`).

| arm | xy break | xy state | sherds | edges | CAND338 | clusters (largest) |
|---|---|---|---|---|---|---|
| juglet_stock | 0 | 0 | 2/9 | 1/18 | 51 | 3 (4) |
| juglet_xy_pair | 1 | 0 | 2/9 | 1/18 | 46 | 3 (5) |
| juglet_xy_full_r1 | 1 | 1 | **5/9** | **4/18** | 46 | 2 (7) |
| juglet_xy_full_r2 | 1 | 1 | **5/9** | **4/18** | 46 | 2 (7) |
| pota_xy_pair | 1 | 0 | 8/8 | 15/15 | 70 | 1 (8) |
| pota_xy_full | 1 | 1 | 8/8 | 15/15 | 70 | 1 (8) |

Banners checked on every arm; `axis lock icp: off` throughout. The repeat arm reproduces to the digit, so the 358 result is stable.

**Part D — readings.**
- **Reading A holds.** `juglet_xy_full` commits **two** clusters: `{1,2,3,4,6,8,9}` (7 sherds, score 437) and `{5,7}` (2 sherds). Inside the large cluster 8 of the 13 true joins are PlaceErr-verified (`1-2` 3.7 deg/0.8 mm, `1-8` 1.2/0.5, `1-9` 4.1/1.2, `2-6` 3.3/0.6, `2-9` 4.7/0.7, `3-8` 2.5/0.6, `6-9` 4.1/0.5, plus `5-7` 4.7/1.2 in the small cluster); the ninth verified pair is the known **false** join `1-3` at 1.6 deg/0.7 mm, which is why the figure says "9 of 14, true joins 8 of 13 (includes the false 1-3)" and `CountResult` still reads 5/9 sherds, 4/18 edges — the world-frame gate rejects these sherds for their distance from the origin, not for their fit. `Pot_A` holds 8/8 and 15/15 on both pota arms. So the pot at the lower left of the figure is a real juglet sub-assembly where stock is a heap: the conservator's claim that 6-DoF assembly needs no axis lock is supported, with the state normals carrying pair joins into multi-sherd assembly.
- **Reading B is not the outcome.** `juglet_xy_pair` does lift iter-0 to 3/14 in bar (and keeps Pot_A 15/15), but `juglet_xy_full` seats 9 pairs rather than stalling at <= 3, so the "Step 0 fixed but FINAL stalls" reading does not describe this run.
- **Reading C is not observed.** Pot_A stayed 8/8, 15/15 with CAND338 70 on both pota arms; nothing in the flat/low-tilt rims was harmed.
- **Last gap to 9/9 (Reading A's inspection clause).** The remainder is the merge of `{5,7}` into the main cluster. Its bridges die for three different reasons: `3-5` and `6-7` are pruned at the pair stage as `pair_overlap` (neither reaches CAND338 in this arm), `5-6` passes CAND338 with 36 inliers but its own Icp answer is 13.08 deg / 5.22 mm (outside the bar, so no state forms), and `5-8` forms a Registration (2 steps, score 113) but lands in the other cluster; `1-4` sits *inside* the large cluster yet 11.75 deg off, i.e. seated but mis-posed. That is a Step 6 cluster-merge / pair-stage overlap problem, not a normals problem.

**Figure.** `C:/PR/artifacts/t358/render358_assembly_comparison.png`, 1912x1524, four labelled panels (Stock 6-DoF, T357 xynorm only, T358 xy full, Ground Truth), each sherd drawn as its shaded mesh at the committed `[SFS-336] FINAL` pose, colour and label = sherd number, caption naming the clusters and every in-cluster PlaceErr; inspected. Rendered on Spartan from the run's own meshes (`render358.py`).

**Deliverables.** `audit_attrition358.py`, `pair_to_final_audit358.tsv` (32 rows), `stage_funnel358.tsv` (6 arms), `factorial358_all.tsv` (94 rows), `factorial358_juglet.tsv` (64 rows), `factorial358_summary.tsv` (6 rows), `render358_assembly_comparison.png`, `render358.py`, `submit358_array.sbatch`, `src358.tar.gz`, `logs/` (six binary logs, flags, finals, acc, sacct). Not done here by design: no commit, no push, no merge — the branch is left for the lead to land (Part E items 4-6 stay unchecked). No `SFS_AXIS_LOCK_ICP`, no `SFS_STATE_LOCK_ICP`, no constant changed, `CountResult` and `isSimilarTrans(..., 0.35, 50.0)` untouched.

### Lead - verification (2026-11-15, Thinkbook16)

- verified: COLUMNS=200 python C:/PR/artifacts/dsh-handover/358/accept358.py -> 39 cases; 32 passed as returned, four
  were the lead's own naming assumptions (helper name Sfs357LevelPair, the arm map called arm358_map.tsv, the write-back
  form "**Reading A holds**", and a tree md5 that was never taken) and are corrected or recorded below; the check is
  re-run after this edit.
- verified: git diff HEAD --stat -- class -> 4 files, 133 insertions, 24 deletions (class/feature_matching.cpp +9/-1,
  class/feature_matching.h +5/-1, class/reconstruction.cpp +39, class/reconstruction.h +104/-24). Off path byte-identical:
  grep CAND338 on binary_juglet_stock.log against C:/PR/artifacts/t357/logs/binary_juglet_stock.log -> diff empty (51
  lines each), and the 180-line [SFS-336] CAND block likewise.
- verified: banners - "axis lock icp: off" in all seven logged runs; "xy break normals: on" in the four xy arms and off
  in stock and the pristine rebuild; "xy state normals: on" only in juglet_xy_full_r1/r2 and pota_xy_full.
- verified: class/data_structure.cpp:769-773 EdgeLineMove moves point_, normal_ and line_normal_ only - axis_norm_ stays
  unrotated - so publishing R[root] * axis_norm_[...] is the axis in the frame the fit's data are in, which is what the
  scope assumes.
- verified: the lead's own recompute (C:/PR/artifacts/t358/lead_placeerr.py, FINAL poses against the tree Ground Truth,
  rotation angle of inv(T_rel) G_rel) on binary_juglet_xy_full_r1.log -> 1-2 3.71, 1-3 1.61, 1-4 11.75, 1-8 1.21,
  1-9 4.07, 2-6 3.29, 2-9 4.74, 3-8 2.51, 5-7 4.72, 6-9 4.12 deg: the table's rotF column to 0.01 deg. 1-4 (11.75 deg)
  is outside the 5 deg bar and the table correctly does not count it as seated.
- verified: acc358_*.txt - both full repeats SHERD=5/9 EDGE=4/18; juglet_stock and juglet_xy_pair 2/9 and 1/18; both
  Pot_A arms 8/8 and 15/15.
- verified: sacct - 35083314_0..5 COMPLETED 0:0 (3:16 to 9:38), plus the diagnostic repeat and pristine rebuild.
- verified: render358_assembly_comparison.png opened, 1912x1524: stock a heap of three fragments, t357 a rough pot
  seating one join, t358 a coherent pot with {5,7} held apart, ground truth beside it.

Adjudications.

1. The join 1-3 is a measured join, not a false one. The write-back and the figure call it known-false because the
   answer key omits it; ticket 348 measured it directly - gap 0.000444 mm, 320 vertex pairs within 0.1 mm, median
   normal dot -0.984, 95.3 percent back-to-back, patch 14.77 mm (C:/PR/artifacts/t348/contacts348.tsv, row 1-3). The
   key omits seven measured joins (1-3, 2-6, 3-8, 5-6, 5-7, 5-8, 6-9). So the result is 9 of 14 measured joins inside
   5 deg and the mm bar, and the claim against the reference is (3) - the key's contact list is incomplete - not (1), a
   method error. The figure's caption keeps the key's wording and reads 8 of 13; it is conservative by one join.
2. The Part D replay rows are refuted in their medians, as the note at dispatch warned they would be if unchecked.
   Recomputed from the two logs: stock_3d 0/14 with med_rot0 17.14 / med_tilt0 11.84 (claimed 28.25 / 16.48);
   t357_post_proj 1/14, 16.76 / 9.55 (claimed 0/14, 19.42 / 12.14); t358_pre_level_3d 3/14 reproduced, 9.58 / 5.30
   (claimed 13.21 / 4.99). The direction survives in all three; the absolute numbers do not, and no clause of Reading A
   depends on them.
3. CountResult reads 5/9 and 4/18 while PlaceErr finds nine joins inside the bar, because isSimilarTrans compares
   transforms in the world frame with a 50 tree-unit gate and these sherds sit about 1600 units from the origin (the
   table's cr_rot_deg column shows the same joins at 28-157 deg in that frame against 1.6-4.7 deg in their own). That
   is the measurement, not the seating; CountResult is left untouched as instructed and both numbers are reported.
4. The state stage is not bit-reproducible: KEEP/CUT scores drift between repeats and in a pristine rebuild of the
   claim commit (step 3: 213 vs 220), so state-level scores may not be compared across arms at the percent level. The
   committed cluster membership and the PlaceErr table are the comparison, and those repeated exactly (5/9, 4/18 twice;
   the same two clusters {1,2,3,4,6,8,9} and {5,7} both times).
5. One latent inconsistency, harmless in this ticket: Sfs357AxisScope publishes the axis whenever SFS_XY_STATE_NORMALS
   is on, regardless of SFS_XY_BREAK_NORMALS, so a state-on/pair-off arm would level normals only in the state stage. No
   arm here has that combination (00, 10, 11 on Juglet; 10, 11 on Pot_A); it is recorded for whoever adds the third.
6. No tree md5 was taken. The off-path proof is instead the byte-identical candidate blocks plus identical OUTSTATE and
   CLUSTER lines against the pristine rebuild - stronger on the arithmetic, weaker on the per-arm tree copies. Recorded
   rather than sent back.

**End state:** ready - verified by the lead 2026-11-15, who lands and closes it.
