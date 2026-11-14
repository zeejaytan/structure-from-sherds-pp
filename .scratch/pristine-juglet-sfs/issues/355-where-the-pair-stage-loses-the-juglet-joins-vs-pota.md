# Ticket 355 — where the stock pair stage (`LCS -> Icp`) loses the Juglet joins vs Pot A

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 355-where-the-pair-stage-loses-the-juglet-joins-vs-pota, in /c/PR/worktrees/structure-from-sherds-pp/355
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 349 (180 candidates, only 4/14 measured joins in bar), 350 (after removing tilt the remaining error is a pure spin `5.7–165.7 deg` + translation), 353 (breakline carries spin signal along steep seams), 354 (oracle-pool caveat and conservator decision not to replace `LCS` with a 2-DoF grid search)
**Blocks:** any pair-stage change (`FeatureComp`/`LCS`, `Icp` iter=0 `UsePreCorres`, or `Icp` iter>=1 `MakeCor`/`P2L`)
**Budget:** small (4 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: one read-only switch (`SFS_PRECOR_TRACE=1`) printing the `LCS` segment interval and the post-`iter=0` `UsePreCorres` pose inside `Icp`, one Spartan batch run on Juglet (signed 5/3) and `Pot_A` (default 8/8, 15/15), and an offline comparison script + render in `C:/PR/artifacts/t355/`.

> **In one line.** The stock SfS++ assembler already solves `Pot_A` (`8/8` sherds, `15/15` edges) without a 2-DoF grid search because `CalculateFeatureAxisless` -> `LCS` matches spin- and height-invariant differential features and hands `[start_..end_]` to `Icp` at `iter = 0`. This ticket measures whether the 10 missed Juglet joins are already lost at `LCS` + `iter = 0` (`UsePreCorres`) or are dragged off later by `Icp` `iter = 1..49` (`MakeCor` + `P2LConstraint`).

## Why this replaces a locked 2-DoF search

Ticket 354 ran a 2-DoF `(theta, h)` grid search on all 36 Juglet pairs. Re-examination establishes two blockers and a direct answer from `Pot_A`:

1. **Claim 2 (Oracle dot-pool confound in 354).** `C:/PR/artifacts/t354/locked354.py:103-105` selected the contact subsets `self.ia, self.ib` (`d <= 3.0 mm`) **at the ground-truth pose** (`theta = 0, h = 0`) and froze them across the entire `360 deg x 30 mm` sweep (`:115-130`). Moving away from `(0, 0)` pulls those ground-truth dots past each other's ends into the `3.0 mm` cap, and 15 non-joins with `0+0` dots at ground truth remain flat at `3.0000 mm` everywhere. Without the ground-truth mask (`C:/PR/artifacts/t354probe/sweep_true.py`), spinning `5 deg` moves the all-dot capped score on `1-2` by `+0.008 mm` (`2.517 -> 2.525`) and lowers it on `1-9`, `2-6`, and `5-8`.
2. **Claim 1 (Ranking fails even with the oracle pool).** All 7 non-joins that have dots within `3.0 mm` at ground truth score at or below the worst join (`1.8839 mm` on `1-9`); 3 beat the best join (`2-5` `0.3010`, `6-8` `0.4038`, `2-8` `0.5650` vs `3-5` `0.6738`); and `4-7` (`0.8218`) + `7-9` (`0.8202`) beat 5 real joins with full-sized dot pools.
3. **Why building a 2-DoF grid search is rejected (`Pot_A` control).** The stock assembler already solves `Pot_A` (`8/8 sherds, 15/15 joins`) without knowing ground-truth dot pools and without a 2-DoF brute-force grid search:
   - `CalculateFeatureAxisless` (`class/filter.cpp:161-219`) computes cylindrical derivative features `(dr/ds, dz/ds, r*dtheta/ds, thickness)` relative to each piece's axis; these derivatives are analytically invariant to both spin (`theta`) and vertical shift (`z`).
   - `FeatureComp` / `LCS` (`class/feature_matching.cpp:386-496`, `:1602-1721`) reverses `shard_x_` (`ChangeOrder()`), quantizes the 4 rows, and matches contiguous segments `[start_.y..end_.y]` to `[start_.x..end_.x]` (length `size_ > windowsize`).
   - `PairwisePruning` (`class/feature_matching.cpp:1851-1966`) runs `Icp` (`class/reconstruction.cpp:1190-1397`) in two phases: at `iter = 0` (`pre_cor = true`), `UsePreCorres` (`:1102`) + `P2PConstraint` (`:1301`) locks point-to-point onto the `LCS` segment; at `iter = 1..49`, `MakeCor` (`:290`, `:1263`) + `RejectOutlier(..., 20, 0.7)` (`:203`, `:1264`) switches to nearest neighbours across the entire breakline with `P2LConstraint` (`:1298`) and `AxisConsistency` (`w_a = 0.1`, `:1207`, `:1308`).

What is **not** yet measured is which of those two phases fails on the 10 missed Juglet joins while both succeed on all 15 joins of `Pot_A`:

- Does `LCS` on the Juglet propose the wrong dot segment (or shifted along the seam so `iter = 0` `UsePreCorres` already lands off in spin/tilt/position)?
- Or does `LCS` + `iter = 0` land near the true join, after which `iter = 1..49` (`MakeCor` across the whole closed loop + `P2LConstraint` + weak `w_a = 0.1`) drifts the pair away into `5.7–165.7 deg` spin and `0.2–179 deg` tilt?

## Part A — the read-only switch (`SFS_PRECOR_TRACE=1`)

One switch, exact string, default off, additions only in `class/`:

- `SFS_PRECOR_TRACE=1`, read once in `InitSfsSwitches()` (`class/feature_matching.cpp:93`, declared in `class/feature_matching.h:34`), default `false`. Banner line printed at startup alongside the existing switches: `precor trace: on` / `precor trace: off`. With it off, **zero** `[SFS-355]` lines may appear in stdout.
- Inside `Icp` (`class/reconstruction.cpp:1190-1397`), at `iter == 0`:
  - Immediately after `UpdateTrans` (`lines 1341-1344`), capture the post-`iter=0` relative transform `M0` placing `shard_x_` (moving) into `shard_y_` (fixed), in the exact same convention `PairwisePruning` uses at `class/feature_matching.cpp:1891-1896`:
    `R_fix_inv = R[index_f].inverse()`, `t_fix_inv = -R_fix_inv * t[index_f]`, `R_rel = R_fix_inv * R[index_m]`, `t_rel = t_fix_inv + R_fix_inv * t[index_m]`.
  - If `miss_cor` fires at `iter == 0` (`COR[0].cor.size() < MINIMUM_NUMBER`, `lines 1274-1288`, before Ceres solves), still print the line with `cor0_size` (`< 6`) and `M0 = Identity` (`1 0 0 0 0 1 0 0 0 0 1 0`).
  - Extract the printing into one small helper (e.g. `PrintPrecorTrace(...)` in `class/reconstruction.cpp`) rather than duplicating the formatting block across the `miss_cor` and post-`UpdateTrans` paths.
  - Format (exact tab-separated line):

        [SFS-355]\tPRECOR\t<index_>\t<y>\t<x>\t<start_y>\t<end_y>\t<start_x>\t<end_x>\t<size_>\t<cor0_size>\t<12 floats of M0 row-major 3x4>

    where `<index_>`, `<y>` (`shard_y_`), `<x>` (`shard_x_`), `<start_y>`, `<end_y>`, `<start_x>`, `<end_x>`, `<size_>` come from `m_LCS[cycle.edges[0]]`.
- **Print-only and GT-free constraints:**
  - `git diff HEAD -U0 -- class` must show **0 deletions** (additions only).
  - Do not touch any threshold, loss, weight, solver option, `MINIMUM_NUMBER`, `w_a`, `LCS` quantization, `UsePreCorres`, `MakeCor`, `RejectOutlier`, `TOP_k`, or `BRANCH_b`.
  - `class/` stays GT-free.

## Part B — measurements, tables, and visual check (`C:/PR/artifacts/t355/`)

Do not copy `.obj` meshes or dense `.xyz` surface clouds off Spartan (`AGENTS.md`); fetch only `logs/`, `runs/`, and the small `Axes/`, `Breaklines/*.pcd`, `Ground Truth/` files needed for offline pose scoring into `C:/PR/artifacts/t355/`.

Write an offline analysis & rendering script in `C:/PR/artifacts/t355/` that reads the trace logs and scores **both** `M0` (`iter = 0`) and `M` (`final`, from `[SFS-336] CAND`) against `G_axis = Ay * GTy^-1 * GTx * Ax^-1` (as in `C:/PR/artifacts/t350/replay350.py`), in **real millimetres** (`length / 3` because both `px3/juglet` and `px3/pota` run at `3x` scale where 1 tree unit = 3 mm):

1. **Sanity anchor check (mandatory before trusting any number):**
   Reproduce the logged `[SFS-338] CAND338` `rot` and `mm/3` on `M` across `juglet_signed_on_r1` (anchors: `1-2` candidate `1` final `2.138 deg / 0.734 mm`, `1-8` candidate `70` final `0.701 deg / 0.403 mm`) and check `pota_on` candidates (and `G_axis` on `Pot_A`) against the `Pot_A` `Breaklines` + `Ground Truth`. Report the max diff and fail/stop if `> 0.01 deg` or `> 0.01 mm`.
2. **Per-candidate decomposition (`precor355_all.tsv`)** for all 180 Juglet candidates (`r1` and `r2`) and all `Pot_A` candidates (`pota_on`):
   - Stage 0 (`iter = 0` `UsePreCorres` P2P): `(rot0_deg, tilt0_deg, spin0_deg, mm0)` — where `tilt0` is `angle(R_M0 * z, z)` and `spin0` is the signed spin after removing tilt about the moved piece's aligned breakline centroid, matching `replay350.py:147-161`.
   - Stage 1 (final `CAND` after `iter = 1..49`): `(rotF_deg, tiltF_deg, spinF_deg, mmF)`.
   - `LCS` seam accuracy (`lcs_hit_y`, `lcs_hit_x`, `lcs_overlap_frac`): fraction of dots in the `LCS` segment `[start_y..end_y]` on `y` (0-based `start_y - 1 .. end_y - 1`) and `[start_x..end_x]` on `ChangeOrder()`-reversed `x` (0-based `N_x - end_x .. N_x - start_x` in `Breakline_0.pcd`) that lie within `3.0 mm` real (`9.0` tree units) of the partner sherd's breakline at the ground-truth pose.
   - `lcs_shift_mm`: Euclidean distance at the ground-truth pose between the midpoint of `LCS` segment `y` and the midpoint of `LCS` segment `x` (measures whether `LCS` matched the right indices vs shifted tangentially along a uniform rim/wall or picked the opposite side of the sherd).
   - `in_bar_0` and `in_bar_F`: `rot <= 5.0 deg` **and** `mm <= 5.0 mm` real (`15` tree units).
3. **Summary tables:**
   - `precor355_juglet.tsv`: **16 rows** — the 14 measured joins of ticket 348 + controls `4-7, 4-6`. For each pair, report both best-at-final and best-at-iter0 candidate rows: candidate `index_`, `size_`, `cor0_size`, `lcs_overlap_frac`, `lcs_shift_mm`, `(rot0, tilt0, spin0, mm0)`, `(rotF, tiltF, spinF, mmF)`, `d_rot = rotF - rot0`, `d_mm = mmF - mm0`, `in_bar_0`, `in_bar_F`, and candidate fate (`surv` / `pair_axis` / `pair_overlap` / `pair_basket` / `merged`). Repeat-check `r1` vs `r2`.
   - `precor355_pota.tsv`: **15 rows** — the 15 ground-truth joins of `Pot_A` (`GT_graph` pairs), same columns, showing how `LCS`, `iter = 0`, and `iter = 1..49` behave on the pot where `8/8 sherds, 15/15 edges` succeeds.
4. **Render (`C:/PR/artifacts/t355/render355_iter0_vs_final.png`):**
   - Visual comparison of Juglet's 14 joins vs `Pot_A`'s 15 joins across `LCS -> iter=0 -> final` (spin error, tilt error, mm error, and `lcs_shift_mm`), plus 3D breakline placement strips showing `LCS` segment intervals vs ground-truth seam vs `iter=0` vs `final` for representative Juglet joins (e.g. passing `1-8`, `1-2`, `6-9` and failing `5-6`, `6-7`, `2-6`) and a `Pot_A` join.

## Part C — readings (name exactly one)

- **`lcs-misses-the-seam`** — On >= 7 of the 10 Juglet joins that never reach the `5 deg / 5 mm` bar at final, **no** `LCS` candidate starts within `5 deg / 5 mm` even at `iter = 0` (`in_bar_0 = 0`, and `lcs_overlap_frac < 0.25` or `lcs_shift_mm > 5 mm`), whereas `Pot_A`'s joins enter `iter = 0` inside or close to the bar via `UsePreCorres`. The bottleneck is upstream of `Icp` (feature quantization, Lanczos derivative smoothing over 15 dots, or `LCS` correspondence offsets), not `Icp` iteration 1..49.
- **`icp-p2l-drifts-off-lcs`** — A majority (`>= 6`) of the 10 missed Juglet joins DO have a candidate that lands inside `5 deg / 5 mm` at `iter = 0` (`in_bar_0 = 1`), and `iter = 1..49` (`MakeCor` across the whole breakline + `P2LConstraint` + weak `w_a = 0.1`) pushes them out (`in_bar_F = 0`), while `Pot_A` stays inside. The bottleneck is `Icp` iterations `1..49`.
- **`stage-split-between-lcs-and-icp`** — Neither mechanism alone accounts for most of the 10 missed joins: some (specify count, e.g. `>= 3`) have zero good `LCS` segment (`in_bar_0 = 0`) and others (`>= 3`) land in-bar at `iter = 0` and are dragged out by `Icp` `iter = 1..49`. Report the exact split per join.
- **`trace-inconclusive`** — The `M0` sanity check fails (`> 0.01 deg` / `> 0.01 mm` mismatch against `CAND338`), `Pot_A` loses `8/8` / `15/15`, or Juglet loses `2/9` / `1/18`.

## Part D — what this cannot settle

- Print-only measurement: cannot show whether fixing `LCS` or `Icp` recovers full assembly (priority ordering and `BuildStep` admission still gate later steps).
- Compares only two objects (`Juglet` 9 sherds vs `Pot_A` 8 sherds) at their existing `px3` extraction.
- `M0` assumes `UsePreCorres` + `P2PConstraint` ran at `iter = 0`; when `COR[0].cor.size() < MINIMUM_NUMBER` (`miss_cor`), `M0` is recorded as Identity and `cor0_size` flags that `UsePreCorres` never ran.

## Part E — arms (Slurm `sbatch` only, private tree copy per arm)

Verify `SOURCE_COMMIT`, run preflight + checkbuild (`make -j8`), copy private `px3/juglet` and `px3/pota` trees, record tree md5 before/after each run, poll every job (`scripts/slurm_poll.sh`), and record `sacct` State/ExitCode in the write-back.

| arm | data | switches | expected acc & trace counts |
|---|---|---|---|
| `juglet_signed_on_r1` | `px3/juglet` | `SFS_AXIS_SIGN=1 SFS_PRECOR_TRACE=1 SFS_STAGE_TRACE=1 SFS_EDGE_TRACE=1 SFS_WIN_TRACE=1` | `2/9 sherds, 1/18 edges`; `180` `[SFS-355] PRECOR` rows; winner in 342's set |
| `juglet_signed_on_r2` | `px3/juglet` | same as `r1` | `2/9 sherds, 1/18 edges`; `180` `[SFS-355] PRECOR` rows; winner in 342's set |
| `juglet_signed_unset_r1` | `px3/juglet` | `SFS_AXIS_SIGN=1`, all trace switches unset | `2/9 sherds, 1/18 edges`; `0` `[SFS-355]` rows |
| `pota_on` | `px3/pota` | `SFS_PRECOR_TRACE=1 SFS_STAGE_TRACE=1 SFS_EDGE_TRACE=1` | `8/8 sherds, 15/15 edges`; `> 0` `[SFS-355] PRECOR` rows (equals `Total number :`) |
| `pota_unset` | `px3/pota` | all trace switches unset | `8/8 sherds, 15/15 edges`; `0` `[SFS-355]` rows |

## Acceptance

- [ ] `SFS_PRECOR_TRACE` read once in `InitSfsSwitches`, default off, banner `precor trace: on` / `precor trace: off`; zero `[SFS-355]` lines when off (`juglet_signed_unset_r1` and `pota_unset`).
- [ ] `git diff HEAD -U0 -- class` has 0 deletions (additions only), `class/` stays GT-free, and no threshold, weight, or heuristic is changed.
- [ ] All 5 arms meet their accuracy targets (`2/9` & `1/18` on all 3 Juglet arms; `8/8` & `15/15` on both `Pot_A` arms); tree md5 before/after identical in all 5 arms; `sacct` State and ExitCode recorded for every job ID.
- [ ] Anchor validation reproduced to `<= 0.01 deg` and `<= 0.01 mm`; `180` `PRECOR` rows on Juglet `r1` & `r2`; `pota_on` `PRECOR` count equals `Total number :`.
- [ ] `C:/PR/artifacts/t355/precor355_juglet.tsv` (16 rows), `precor355_pota.tsv` (15 rows), `precor355_all.tsv`, offline scoring script, and `render355_iter0_vs_final.png` produced and inspected.
- [ ] Exactly one reading slug from Part C named in `## Comments (worker, ...)` with supporting counts for Juglet (14 joins) vs `Pot_A` (15 joins), ending with `**End state:**`.
