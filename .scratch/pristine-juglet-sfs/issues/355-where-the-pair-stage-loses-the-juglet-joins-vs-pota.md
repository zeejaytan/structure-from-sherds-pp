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

- [x] `SFS_PRECOR_TRACE` read once in `InitSfsSwitches`, default off, banner `precor trace: on` / `precor trace: off`; zero `[SFS-355]` lines when off (`juglet_signed_unset_r1` and `pota_unset`).
- [x] `git diff HEAD -U0 -- class` has 0 deletions (additions only), `class/` stays GT-free, and no threshold, weight, or heuristic is changed.
- [x] All 5 arms meet their accuracy targets (`2/9` & `1/18` on all 3 Juglet arms; `8/8` & `15/15` on both `Pot_A` arms); tree md5 before/after identical in all 5 arms; `sacct` State and ExitCode recorded for every job ID.
- [x] Anchor validation reproduced to `<= 0.01 deg` and `<= 0.01 mm`; `180` `PRECOR` rows on Juglet `r1` & `r2`; `pota_on` `PRECOR` count equals `Total number :`.
- [x] `C:/PR/artifacts/t355/precor355_juglet.tsv` (16 rows), `precor355_pota.tsv` (15 rows), `precor355_all.tsv`, offline scoring script, and `render355_iter0_vs_final.png` produced and inspected.
- [x] Exactly one reading slug from Part C named in `## Comments (worker, ...)` with supporting counts for Juglet (14 joins) vs `Pot_A` (15 joins), ending with `**End state:**`.

## Comments (worker, 2026-11-14)

**Part A — switch + trace (uncommitted working tree, additions only).** `SFS_PRECOR_TRACE` is read once in
`InitSfsSwitches` (`class/feature_matching.cpp`), declared in `class/feature_matching.h`, and prints
`precor trace: on` / `precor trace: off` at `class/feature_matching.cpp:164`. `PrintPrecorTrace` is at
`class/reconstruction.cpp:1197`, with its two call sites at `class/reconstruction.cpp:1323` (iter 0 +
`miss_cor`, prints `M0` = identity) and `class/reconstruction.cpp:1383` (after the iter-0 `UpdateTrans`, prints
the post-iter-0 relative transform in the `PairwisePruning` convention
`R_fix_inv = R[index_f].inverse(); t_fix_inv = -R_fix_inv*t[index_f]; R_rel = R_fix_inv*R[index_m];
t_rel = t_fix_inv + R_fix_inv*t[index_m]`, `index_f = shard_y_-1`, `index_m = shard_x_-1`).
`git diff HEAD --numstat -- class` = 11/0 (`feature_matching.cpp`), 1/0 (`feature_matching.h`), 39/0
(`reconstruction.cpp`); `git diff HEAD -U0 -- class | grep -c '^-'` = 0. Nothing outside `class/` is modified.
No threshold, loss, weight, solver option, `MINIMUM_NUMBER`, `w_a`, LCS quantization, `UsePreCorres`, `MakeCor`,
`RejectOutlier`, `TOP_k` or `BRANCH_b` line changed, and nothing GT-bearing was added to `class/` — the only
`Ground Truth` string in `class/` is pre-existing and sits in the unmodified `class/data_path.h:1497`.

**Arms.** Five arms, private tree copy each, `SOURCE_COMMIT=6bed767`, run dir
`/data/gpfs/projects/punim2657/SfSpp/runs/20261114-2235_6bed767_t355-precor` (`arm/<ARM>` for Juglet,
`pota_work/<ARM>` for `Pot_A`). Jobs are `sbatch` only; tree md5 identical before/after in all five (55 files
on the Juglet tree, 49 on the `Pot_A` tree).

| arm | job (sacct State / ExitCode / Elapsed) | acc sherds / edges | `[SFS-355]` rows | `Total number :` | banner |
|---|---|---|---|---|---|
| `juglet_signed_on_r1` | `35072764_0` COMPLETED 0:0 4:31 | 2/9, 1/18 | 180 | 180 | `precor trace: on` |
| `juglet_signed_on_r2` | `35072764_1` COMPLETED 0:0 6:27 | 2/9, 1/18 | 180 | 180 | `precor trace: on` |
| `juglet_signed_unset_r1` | `35072764_2` COMPLETED 0:0 4:28 | 2/9, 1/18 | 0 | 180 | `precor trace: off` |
| `pota_on` | `35072765_0` COMPLETED 0:0 3:09 | 8/8, 15/15 | 124 | 124 | `precor trace: on` |
| `pota_unset` | `35072819_1` COMPLETED 0:0 3:06 | 8/8, 15/15 | 0 | 124 | `precor trace: off` |
| `submit_checkbuild355` | `35072683` COMPLETED 0:0 0:32 | — | — | — | compile rc=0, link rc=0 |

Trace rows equal the candidate count exactly (`180` Juglet = `Total number : 180`; `124` `Pot_A` =
`Total number : 124`), and switching the trace off leaves both accuracies untouched and emits zero
`[SFS-355]` lines. Every one of the 304 trace rows has `size_ == cor0_size` and no candidate had
`cor0_size == 0`, so the `miss_cor`/identity branch never fired in practice and is verified by inspection only.
`M0` rotation determinants are 0.999999-1.000001 over all rows.

Two defects in my own harness, both fixed before the numbers above (recorded for the audit trail):
`35072684_{0,1,2}` and `35072685_{0,1}` FAILED 1:0 in 1-3 s on `SOURCE_COMMIT: unbound variable` (`set -u`);
resubmitted as `35072764` and `35072765`. `35072765_1` then FAILED 3:0 in 1 s because the two `Pot_A` array tasks
raced on a shared `cp .../checkacc333.py`; that task was resubmitted alone as `35072819`.

**Part B — anchors (mandatory, checked before any interpretation).**
- `juglet_signed_on_r1` and `_r2`: 51 `CAND338` rows, `max|drot| = 0.001445 deg`, `max|dmm| = 0.002132 mm`.
  Candidate 1 (1-2) 2.139 deg / 0.734 mm vs logged 2.138 / 0.734; candidate 70 (1-8) 0.701 / 0.402 vs 0.701 / 0.403.
- `pota_on`: 70 `CAND338` rows, `max|drot| = 0.001366 deg`, `max|dmm| = 0.000370 mm` (candidate 70, 3-6:
  3.423 / 1.361, exact). `pota_unset` has no `CAND338` rows because the 338 trace was unset there.
- One caveat on the 342 cross-check: this build's `[SFS-342] WIN` line carries eight fields, not ticket 342's four,
  so the expected `30 260 4 3` / `33 263 5 3` cannot be compared field-for-field. Recorded as printed:
  r1 `WIN 30 3 4 6 77 0.597422 25`, r2 `WIN 29 3 4 6 77 0.597422 25`.

**Part B — per-candidate decomposition (Juglet r1 and r2, 180 candidates each).** r1 and r2 select the same
best-final candidate on 16 of 16 joins (same `idx`, same `rotF`), so the two seeds are not independent evidence here.
Candidate-level medians are dominated by wrong pairs and are useless as a headline; per join, the best candidate at
iter 0 already sits at `rot0` median 16.4 deg / `mm0` 3.31 mm (Juglet) against 5.1 deg / 1.42 mm (`Pot_A`).
The LCS segments themselves are simply smaller on Juglet: median 8 dots (range 6-21) against 11 dots (range 6-35)
on `Pot_A`, and `cor0_size` tracks the segment exactly.

**Where the two datasets part company at the bar.** Only 1 of 180 Juglet candidates is inside 5 deg / 5 mm at
iter 0 (`idx 86`, a 2-3 candidate, 3.94 deg / 1.82 mm — not a GT join), i.e. **0 of 16 joins have any candidate in
bar at iter 0**. `Pot_A` has a candidate in bar at iter 0 on 6 of 15 joins. At final, 5 of 16 Juglet joins are in bar
(1-2 1.93 / 0.64, 1-8 0.49 / 0.43, 5-7 4.71 / 1.23, 6-9 2.35 / 0.57 and control 4-7 3.35 / 0.89) and control 4-6
fails (153.46 / 23.09). `Pot_A` lands 10 of 15 in bar at final and still assembles 15/15: four of its joins
(1-5 175.77, 2-5 176.43, 3-5 171.46, 1-7 121.11) carry a ~180 deg tilt from the stored piece-axis sign while
their `rotF` stays 1.75-9.07 deg, so the bar's tilt component is sign-sensitive and those joins are correctly seated
regardless. `Pot_A`'s five out-of-bar joins are 1-4 (21.19 / 10.24), 1-6 (5.90 / 1.30), 1-7 (5.26 / 0.68),
3-5 (9.07 / 2.44), 4-7 (5.26 / 1.04).

**Exact split per missed Juglet join** (the 10 ticket-348 joins that never reach the bar; `frac` = `lcs_overlap_frac`,
`shift` = `lcs_shift_mm`, both at the GT pose; metrics for the join's best-final candidate, `b0` values are in the tsv):

| join | frac | shift | rot0 | rotF | mm0 | mmF | fate | attribution |
|---|---|---|---|---|---|---|---|---|
| 1-3 | 0.000 | 12.98 | 33.96 | 30.42 | 12.23 | 12.09 | pair_overlap | LCS seam |
| 3-5 | 0.500 | 14.90 | 47.04 | 48.31 | 14.92 | 14.67 | surv | LCS seam |
| 6-7 | 0.500 | 27.13 | 153.33 | 165.69 | 26.98 | 26.82 | surv | LCS seam |
| 1-4 | 1.000 | 0.78 | 17.90 | 15.78 | 4.08 | 3.42 | merged | pose |
| 1-9 | 1.000 | 0.74 | 28.12 | 8.82 | 3.43 | 1.16 | surv | pose |
| 2-6 | 1.000 | 3.92 | 32.57 | 26.21 | 8.11 | 6.38 | surv | pose |
| 2-9 | 1.000 | 1.01 | 19.32 | 15.12 | 3.12 | 2.72 | surv | pose |
| 3-8 | 1.000 | 0.60 | 10.86 | 11.00 | 1.64 | 1.51 | surv | pose |
| 5-6 | 1.000 | 3.33 | 7.63 | 13.00 | 4.24 | 5.22 | surv | pose |
| 5-8 | 1.000 | 0.66 | 12.49 | 7.00 | 3.19 | 1.66 | surv | pose |

**Why the iter-0 pose misses the bar even where the seam is right.** Measuring the same LCS dots against the partner
breakline *at the iter-0 pose* (not at GT) gives a median distance of 0.06-0.25 mm for these joins — tighter than at
GT (0.3-14.0 mm) — so `UsePreCorres` + `P2PConstraint` really does seat the LCS dots; the trace is not a frame error.
The dots, however, cover only a ~3 mm patch of a nearly straight run, so the rotation about the seam is left almost
free: the iter-0 pose is a genuine seam fit that is 7-34 deg off in rotation, and `iter 1..49` then recovers only
1-2 (14.96 -> 1.93), 1-8 (8.07 -> 0.49), 6-9 (14.77 -> 2.35) and 5-7 (6.64 -> 4.71), leaves the 7 above out of bar,
and makes 5-6 and 6-7 slightly worse (7.63 -> 13.00, 153.33 -> 165.69).

**Reading (Part C) — `stage-split-between-lcs-and-icp`.** Neither mechanism alone accounts for the 10 missed joins:
3 of 10 (1-3, 3-5, 6-7) have the LCS segment itself off the seam (`lcs_overlap_frac < 0.25` or
`lcs_shift_mm > 5 mm`), and 7 of 10 (1-4, 1-9, 2-6, 2-9, 3-8, 5-6, 5-8) have an accurate LCS seam
(`frac = 1.000`, `shift = 0.60-3.92 mm`) but a pose that is already out of bar at iter 0 and stays out.
Stated plainly against the ticket's thresholds: the `lcs-misses-the-seam` test fails because only 3 of 10 join-level
seams are defective (it wants >= 7), and the `icp-p2l-drifts-off-lcs` test fails because `in_bar_0 = 1` for
**0** of the 10 (it wants a >= 6 majority that starts in bar and is dragged out) — on Juglet the pair stage never
has an in-bar candidate to drag out, so the second group is "seam right, pose loose from iter 0 on, ICP does not
recover it" rather than "in bar then dragged out". `trace-inconclusive` does not apply: the anchors hold to
0.0015 deg / 0.0021 mm, `Pot_A` keeps 8/8 and 15/15, and Juglet keeps 2/9 and 1/18.

**Artefacts** (all under `C:/PR/artifacts/t355/`): `precor355_all.tsv` (485 lines = header + 360 Juglet rows from
r1+r2 + 124 `Pot_A` rows; per row `cor0`, `size`, `rot0 tilt0 spin0 mm0`, `rotF tiltF spinF mmF`, `hit_y`,
`hit_x`, `frac`, `shift`, `in0`, `inF`, `fate`); `precor355_juglet.tsv` (header + 16 joins, `r1_*` and `r2_*`
blocks each carrying the best-at-final and best-at-iter-0 candidate views plus `drot`/`dmm` and `ncand`/`nin0`);
`precor355_pota.tsv` (header + 15 GT joins); `render355_iter0_vs_final.png`; `partC355_counts.json`;
`analyze355.py`, `tables355.py`, `render355.py`; `tables355_stdout.txt`; `logs/binary_*.log` (5) and the
`tree/{juglet,pota}` GT inputs. The patch is left uncommitted in the worktree; nothing was pushed.

### Lead — verification (2026-11-14, Thinkbook16)

The three check failures at first run were the lead's own cases, not the worker's files (the porcelain slice lost the
leading dot of .scratch/...; the banner case wanted the literal "precor trace: on" although the source prints the
ternary; the reading case counted every slug the write-back names). Fixed in the check; it is 24/24.

- verified: cd worktree && COLUMNS=200 python C:/PR/artifacts/dsh-handover/355/accept355.py -> 24 cases, 24 pass.
- verified: git diff HEAD --numstat -> class/feature_matching.cpp 11/0, class/feature_matching.h 1/0,
  class/reconstruction.cpp 39/0, ticket 112/0; git diff HEAD -U0 -- class | grep -c '^-' (excluding '---') -> 0.
- verified: switch read class/feature_matching.cpp:124, banner :164, printer class/reconstruction.cpp:1197, call
  sites :1323 (identity on miss_cor) and :1383 (after the iter-0 UpdateTrans).
- verified: precor355_juglet.tsv 16 rows, precor355_pota.tsv 15, precor355_all.tsv 484 (360 + 124); trace rows
  180/180/0 and 124/0 against "Total number :" 180 and 124.
- verified: the lead's own re-derivation from precor355_juglet.tsv: of the 10 joins that miss the bar, 3 have a
  defective LCS seam (1-3 frac 0.000 shift 12.98; 3-5 0.500 / 14.90; 6-7 0.500 / 27.13) and 7 have frac 1.000 with
  shift 0.60-3.92 mm (1-4 0.78, 1-9 0.74, 2-6 3.92, 2-9 1.01, 3-8 0.60, 5-6 3.33, 5-8 0.66) — the split the worker
  reports, reproduced independently.
- verified: iter-0 in bar 0 of 10 missed joins (0 of 14 overall) on Juglet against 6 of 15 on Pot_A; final in bar
  4 of 14 against 10 of 15; ICP 1..49 improves 1-2 (14.96 -> 1.93), 1-8 (8.07 -> 0.49), 6-9 (14.77 -> 2.35) and
  5-7 (6.64 -> 4.71) and worsens 5-6 (7.63 -> 13.00) and 6-7 (153.33 -> 165.69).
- verified: accuracy and jobs from the write-back table — 2/9 + 1/18 on the three Juglet arms, 8/8 + 15/15 on both
  Pot_A arms; jobs 35072764_0/1/2, 35072765_0, 35072819_1, 35072683 all COMPLETED 0:0; tree md5 SAME (55/49 files).
- verified: C:/PR/artifacts/t355/render355_iter0_vs_final.png opened — panels (a) and (c) put the iter-0 marks (dark)
  above the final marks exactly on 1-2, 1-8, 1-9 and 6-9, and panel (f) shows the overlap fraction at 1.0 on every
  join except 1-3 (0.0) and 3-5 / 6-7 (0.5).

Adjudications.

1. The Part C vocabulary is the lead's and is too coarse. None of the four slugs fires literally on Juglet:
   lcs-misses-the-seam wants >= 7 defective seams (3 found); icp-p2l-drifts-off-lcs wants >= 6 joins in bar at iter 0
   and then dragged out (0 found — the pair stage never has an in-bar candidate to drag). The worker chose
   stage-split-between-lcs-and-icp and said so in the open. The lead keeps that slug with its meaning narrowed and
   recorded here: a third of the missed joins lose the seam in LCS; the other seven get a correct but short seam whose
   rotation is free at the first solve; the remaining 40 iterations recover four of fourteen. The honest fifth slug
   would be short-seam-leaves-the-spin-free; it is recorded in S1 rather than relabelled here.
2. Not verified by the lead: "the same LCS dots meet the partner curve to 0.06-0.25 mm at the iter-0 pose". The trace
   prints a relative transform in local frames while the breakline files are in tree coordinates, so recomputing it
   needs the pipeline's internal poses. It is consistent with the verified table (frac 1.000 with a 7-34 deg rotation
   error) and is recorded as the worker's measurement.
3. Pot_A control caveat: only 10 of 15 Pot_A joins are inside the bar at their best final candidate, yet Pot_A seats
   15 of 15. The per-candidate bar is not a seat predictor, so the Juglet-versus-Pot_A contrast (0/10 against 6/15 at
   iter 0) is suggestive, not decisive.
4. Format note for the next ticket: this build's [SFS-342] WIN line carries eight fields where ticket 342 recorded
   four. Nothing here reads it, but a ticket that does must re-read the format first.

**End state:** ready
