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
**Status:** resolved
**Branch:** 367-isolate-base-sign-on-shipped-vs-4dof-cin-and-k1, in /c/PR/worktrees/structure-from-sherds-pp/367
**Worker:** dsh one-shot subagent w367
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

- [x] Part A: `<run>/work/` builds all 5 axis sets (`axes_shipped_base`, `axes_pota_shipped_base` `8/8` byte-identical by `cmp` vs `axes323`, `axes_a1_4dof_cin`, `axes_pota_a1_4dof_cin` with `K=1` check, `axes_a1_4dof_k1`) without touching tracked files; `axes367_summary.json` scores every candidate (`unsigned_deg`, `signed_deg`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `SignAxisGuard` = `0/18` across all Juglet sets).
- [x] Part B: all 8 assembly runs (`--array=0-7`) complete via `sbatch` with `sacct -X` showing `COMPLETED 0:0`; scored with `score58` origin Kabsch, `cluster366` vertex Kabsch, stock & sherd-centroid `CountResult`, `CANDTOTAL`, `SIGN-AXIS`, and Piece 7 upright `CAND`/`CAND338` audit.
- [x] Part C: `C:/PR/artifacts/t367/axes367_summary.json`, `C:/PR/artifacts/t367/gate367.tsv`, `C:/PR/artifacts/t367/render367_ablation.png` (visually inspected), `runs/index.tsv` row appended, `## Comments` complete, bare `Reading:` line, `**End state:** ready` — *this run ends `blocked` instead, see the Comments' Reading line: the controls for the promotion do not hold*.

## Comments

**Run dir:** `/data/gpfs/projects/punim2657/SfSpp/runs/20261119-1023_273b8f9_t367-shipped-vs-4dof-cin/` (git7 `273b8f9`, tag `t367-shipped-vs-4dof-cin`). All prototype code lives under `<run>/work/`: `axes367.py` (Part A), `base_axis.py` copied verbatim from `20261118-1435_78581fb_t366-base-sign-4dof/work/base_axis.py`, `src_axes_367/` copied from `20261118-0247_4beb57f_t363-own-axes/src_axes` (tree `6a67364b`). Job helpers under `<run>/tools/` (`arm335.sh`, `score58.py`, `cluster366.py`, `p7audit366.py`, `stage367.sh`, `checkacc333.py`). **No tracked file touched**, nothing committed. `runs/index.tsv` row appended with `verdict` and `human_note` blank.

### Slurm (`sacct --array -j <id> -X`)

```
35381843     t367-axes     COMPLETED  0:0  00:05:34   8
35382995_0   t367-arms     COMPLETED  0:0  00:08:28  16
35382995_1   t367-arms     COMPLETED  0:0  00:08:16  16
35382995_2   t367-arms     COMPLETED  0:0  00:08:28  16
35382995_3   t367-arms     COMPLETED  0:0  00:09:50  16
35382995_4   t367-arms     COMPLETED  0:0  00:09:35  16
35382995_5   t367-arms     COMPLETED  0:0  00:09:09  16
35382995_6   t367-arms     FAILED     1:0  00:03:33  16
35382995_7   t367-arms     FAILED     1:0  00:03:53  16
35384083_6   t367-arms     COMPLETED  0:0  00:03:32  16
35384083_7   t367-arms     COMPLETED  0:0  00:02:46  16
35384247     t367-cluster  COMPLETED  0:0  00:00:14   8
35384770_8   t367-arms     COMPLETED  0:0  00:02:50  16
35384770_9   t367-arms     COMPLETED  0:0  00:03:14  16
35384770_10  t367-arms     COMPLETED  0:0  00:04:16  16
35384770_11  t367-arms     COMPLETED  0:0  00:03:08  16
```

Two honest deviations from the ticket's plan, both recorded here rather than hidden:

1. **`35382995_6/_7` exited `1:0`, not `0:0`.** The first `--array=0-7` run made the Pot A seat/SHERD/EDGE check *fatal* in `submit367.sbatch`; a TEST arm scoring badly is a result, not a rig failure, so that gate was mine and it was wrong. The block now only reports ("`=== Pot A arm $ARM seated at 4.596 mm = $N/8 (reported, not gated)`") and the two Pot A tasks were resubmitted with `sbatch --array=6-7` (`35384083`) → both `COMPLETED 0:0`. All eight arms therefore have a completed run; the `FAILED` rows above are kept because they are what `sacct` says.
2. **Four extra arms (`35384770_8-11`)** — `pota_shipped_base_r1/r2` and `pota_a1_k0rig_r1/r2` — were added beyond the ticket's eight. Without them a short Pot A seating count cannot be told apart from a broken harness; with them the rig is exonerated and the loss is pinned to the axis rule (§ Part B).

### Part A — five axis sets (`out/axes/axes367_summary.json`)

Set headers as printed by the job:

```
axes_shipped_base        (shipped)  K=[1,2,1,2,1,1,3,1,3]  guard=0/18  cmp vs t363 shipped = 8/9
axes_pota_shipped_base   (shipped)  K=[1,1,1,1,1,2,1,3]    guard=6/15  cmp vs axes323      = 8/8
axes_a1_4dof_cin         (extract)  K=[1,2,2,1,1,1,1,1,3]  guard=0/18
axes_pota_a1_4dof_cin    (extract)  K=[1,1,1,1,1,1,1,1]    guard=6/15
axes_a1_4dof_k1          (k1)       K=[1,1,1,1,1,1,1,1,1]  guard=0/18
axes_a1_base (t366 file, re-scored) K=[1,2,1,1,1,1,2,1,3]  guard=0/18
```

Rank-0 unsigned error, degrees (per-candidate `signed_deg`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `z_wall_minus_z_pole`, `orient_flipped` for every candidate are in the JSON):

| piece | shipped_base | pota_shipped_base | a1_4dof_cin | pota_a1_4dof_cin | a1_4dof_k1 | t366 a1_base |
|---|---|---|---|---|---|---|
| 1 | 0.95 | 0.34 | 0.95 | 0.34 | 0.95 | 0.87 |
| 2 | 15.73 | 2.25 | 15.73 | 0.79 | 15.73 | 20.43 |
| 3 | 9.60 | 0.41 | 4.92 | 0.81 | 4.92 | 4.49 |
| 4 | 7.40 | 4.07 | 7.40 | 4.07 | 7.40 | 10.32 |
| 5 | 3.62 | 2.03 | 1.06 | 2.03 | 1.06 | 6.00 |
| 6 | 4.71 | 11.35 | 2.58 | 19.90 | 2.58 | 2.74 |
| 7 | 4.69 | 68.38 | 4.44 | 68.38 | 4.44 | 4.96 |
| 8 | 3.19 | 1.20 | 3.80 | 18.98 | 3.80 | 5.99 |
| 9 | 6.58 | — | 6.58 | — | 6.58 | 4.96 |

Reading of that table, in order of what the ticket asked:

- **`orient_base_axis` alone on the untouched shipped axes changes exactly one thing on Juglet: piece 7's sign.** `cmp` against `20261118-0247_4beb57f_t363-own-axes/out/axes_partA/axes_def3x` is **8/9**, and the only differing file is piece 7, whose three lines `orient_base_axis` negated (`orient_flipped=3`, `z_wall_minus_z_pole=-28.64` before the flip). Every other juglet file is byte-identical. Piece 7's own direction error does not move (4.69° before and after); what changes is which way round it is placed, which is the whole 175.31° inverted problem the ticket came from.
- **`axes_pota_shipped_base` is `8/8` byte-identical to `axes323`, 0 lines flipped** — the expected control, so the base rule is inert on the Pot A corpus and the shipped Pot A axes are trustworthy as-is.
- **The 4-DoF pre-polish `c_in` ranking helps Juglet and hurts Pot A.** On Juglet it tightens five pieces against t366's `a1_base` (P5 6.00→1.06, P8 5.99→3.80, P4 10.32→7.40, P2 20.43→15.73, P7 4.96→4.44 with K 2→1) and loses a little on two (P3 4.49→4.92, P9 4.96→6.58); P2 stays the hard piece (15.73° rank-0, second candidate 9.51°). On Pot A it keeps `K=1` on all eight pieces as expected, but **P06 goes 11.35°→19.90° and P08 goes 1.20°→18.98°**. The direct axis comparison against `axes323` gives P01 0.00°/0.00 mm, P02 1.50°/6.02 mm, P03 0.47°/0.03 mm, P04 0.00°/0.00, P05 0.00°/0.00, **P06 31.11°/77.32 mm, P07 0.00°/0.00, P08 19.49°/39.62 mm**.
- **Why Pot A regresses, mechanically:** `meta.polish_shift_deg` says the 4-DoF polish moves the chosen Pot A line by only 2.105° on P06 and 0.299° on P08 — the line is already ~19-20° off when it reaches the polisher, i.e. **the pre-polish `c_in` ranking picked the wrong basin**. Our P08 rank-0 (18.98°) is literally the *shipped* set's second candidate (`c1 = 18.98°/16.80 mm`); the shipped ranking (post-polish cost re-sort, i.e. `rank_by_profile`) puts the 1.20° line first. Same candidate pool, different ORDER — that is the entire Pot A difference, and it is exactly the behaviour ticket 367 forbade me to reproduce.
- **`axes_a1_4dof_k1` = `a1_4dof_cin` column 0 is identical to `a1_4dof_cin` on every piece and on all 9 pieces `K=1`** (`flip_at_k1=false` everywhere): on Juglet the `keep_lines` extra lines never win, so K>1 is not where the 4-DoF gain comes from — the gain is the 4-DoF polish itself.
- **`SignAxisGuard` = `0/18` (0.00 %) on every Juglet set**, as the ticket requires. Both Pot A sets report `6/15` (40 %) — but those are the *shipped* axes that score `8/8` and `15/15`, so the guard's 25 % bound does not transfer to Pot A and is reported as information only, never as a failure.

### Part B — eight arms plus four controls

`score58` origin-gauge seating recomputed as `chamfer_mm <= 13.788` tree (Juglet) / `<= 4.596` mm (Pot A); `score58`'s own `seated_count` field is against `4.596` in **tree** units and is always 0 on Juglet, so it is never quoted. `cluster` = largest-subset vertex-centroid Kabsch. `centroid` = stock CountResult reproduced at the sherd centroid after the `k=n` piece-centroid gauge (below).

| arm | CR directed | CR sherd | CR centroid | centroid strict | 9pc seated / worst mm | cluster seated / worst mm |
|---|---|---|---|---|---|---|
| juglet_shipped_base_r1 | 17/36 | 9/9 | 18/18 | 18/18 | 6/9 · 5.56 | 9/9 · 0.48 |
| juglet_shipped_base_r2 | 17/36 | 9/9 | 18/18 | 17/18 | 5/9 · 6.07 | 9/9 · 0.47 |
| juglet_a1_4dof_cin_r1 | **25/36** | 9/9 | 18/18 | 18/18 | **9/9 · 4.06** | 9/9 · 0.47 |
| juglet_a1_4dof_cin_r2 | 22/36 | 9/9 | 18/18 | 18/18 | 8/9 · 4.77 | 9/9 · 0.48 |
| juglet_a1_4dof_k1_r1 | 22/36 | 9/9 | 18/18 | 18/18 | 9/9 · 4.15 | 9/9 · 0.48 |
| juglet_a1_4dof_k1_r2 | 17/36 | 9/9 | 18/18 | 18/18 | 8/9 · 4.77 | 9/9 · 0.47 |
| juglet_a1_base_r1 (t366) | 23/36 | 9/9 | 18/18 | 18/18 | 8/9 · 4.88 | 9/9 · 0.47 |
| juglet_a1_base_r2 (t366) | 23/36 | 9/9 | 18/18 | 18/18 | 8/9 · 4.65 | 9/9 · 0.48 |
| juglet_oracle_xy (t366) | 26/36 | 9/9 | 18/18 | 18/18 | 9/9 · 4.19 | 9/9 · 0.47 |
| **pota_a1_4dof_cin_r1** | 26/36 | **7/8** | 13/15 | 0/15 | **0/8 · 75.38** | 7/7 · 1.30 |
| **pota_a1_4dof_cin_r2** | 26/36 | 7/8 | 13/15 | 0/15 | 0/8 · 75.50 | 7/7 · 1.30 |
| **pota_shipped_base_r1** | 30/36 | **8/8** | 15/15 | 15/15 | **8/8 · 2.06** | 8/8 · 1.29 |
| **pota_shipped_base_r2** | 30/36 | 8/8 | 15/15 | 15/15 | 8/8 · 2.02 | 8/8 · 1.28 |
| **pota_a1_k0rig_r1** | 30/36 | 8/8 | 15/15 | 15/15 | 8/8 · 2.01 | 8/8 · 1.27 |
| **pota_a1_k0rig_r2** | 30/36 | 8/8 | 15/15 | 15/15 | 8/8 · 2.01 | 8/8 · 1.27 |
| pota_a1_k0rig (t366 ref) | 30/36 | 8/8 | 15/15 | 15/15 | 8/8 · 2.01 | 8/8 · 0.42 |

- **The Pot A loss is one piece, and it is the axes, not the harness.** `pota_a1_4dof_cin` seats 7 of 8 (`7/8`, `13/15` edges) and reproduces t366's `pota_a366_4dof` (7/8, 13/15, cluster 24.13 mm vs 24.127). In the **same** rig, `pota_shipped_base` (the shipped axes + `orient_base_axis`, byte-identical to `axes323`) and `pota_a1_k0rig` (t366's 2-DoF axes) both seat **8/8 at 2.0 mm and `15/15` edges**. So the harness seats Pot A when the axes are right; the 4-DoF `c_in` ranking is what puts piece 08 19.49° / 39.62 mm off.
- **On Juglet the 4-DoF `c_in` rule is the best of the three candidate rules** (25/36 directed, 9/9 seated at 4.06 mm, the best arm on both gauges), ahead of t366's `a1_base` (23/36, 8/9 at 4.65 mm) and far ahead of `shipped_base` (17/36, 6/9). `k1` is indistinguishable from `a1_4dof_cin` in outcome (`K=1` on Juglet anyway).
- **Piece 7 upright audit (Juglet).** `shipped_base`: CANDTOTAL 448, `CAND336` 39/157 upright, `CAND338` 4/23 upright (9 on a GT edge), `[SFS-335]` SIGN-AXIS lines 1. `a1_4dof_cin`: 431, 17/58 (min rotation 2.201°), 3/13 (5 on a GT edge). `k1`: 240, 13/44, 3/11 (4 on a GT edge). t366 `a1_base_r2`: 361, 27/100, 8/17. Oracle: 230. Accepted joins `3-7, 6-7, 7-8, 7-9` = `4/4` on every Juglet arm, `3/3` on every Pot A arm.
- **Two tool incidents, recorded so the numbers can be trusted.** (i) The `cluster366.py` in the t366 run and in my `tools/` was a **stale copy** (md5 `0f8be557…`) that reported `cluster=[1] seated=0/1 max_chamfer≈53.6` on every arm while `score58` showed per-piece chamfers of 6.5-16.7 mm; the real largest-subset tool is the laptop copy (md5 `81f347cc…`). Re-run as job `35384247` over all eight arms and five reference arms; it reproduces t366 exactly (`juglet_a1_base_r2` 9/9 at 1.445 = 0.482 mm, `juglet_oracle_xy` 9/9 at 0.467 mm) and is what the table above uses. (ii) My first sherd-centroid rows compared raw estimated vs GT relative positions and read `0/18` for every arm **including the oracle**; the estimated poses live in a global frame ~1.6e3 tree units from the vessel, so the lever arm swamped the residual. The rows above fit the `k=n` piece-centroid gauge first (as `cluster366` does) and then read `18/18` for every Juglet arm.

### Figure

`C:/PR/artifacts/t367/render367_ablation.png` (2415×3045, six labelled panels, `[SFS-336] FINAL` poses with green bridges = joins within 5° / 5 mm and red dashes = outside). Inspected before this line was written: panels 1-5 all show a **closed Juglet vessel with all nine sherds in place and every bridge green**; panel 1 (`shipped_base`) has 6 sherds inside the bar, panels 2-3 nine, panel 4 (t366 `a1_base_r2`) eight, panel 5 (oracle) nine. Panel 6 (Pot A, `pota_a1_4dof_cin`) shows **one large arc plus a detached outlier — 2 clusters, largest 7 of 8 placed, 13 of 13 joins within 5°/5 mm, 0 of 8 seated** against the origin gauge; the picture agrees with the table that the Pot A loss is the single piece 08, not a shattered vessel.

### Deliverables fetched to `C:/PR/artifacts/t367/`

`axes367_summary.json`; `gate367.tsv` (487 data rows + header, exact 7 columns `group piece metric value reference verdict note`); `render367_ablation.png`; `logs/`; `score/` (16 arms); `result/` (per-arm `1. Acc.txt` + `T_*.txt`); `work/` (all prototype scripts + `p7_<arm>.json`); `build_gate367.py`, `render367.py`, `add_centroid367.py` (row builders).

### Verdict of this run

Ticket 367 asked which of three unconfounded rules is clean enough to promote into `SfSpp_preprocessing/sfs_axes/`. The answer is **none of them as it stands**:

- `orient_base_axis` over the shipped axes is a **pure piece-7 sign fix on Juglet** (8/9 files byte-identical, 0/8 changed on Pot A) and keeps Pot A perfect (`8/8`, `15/15`, 2.0 mm), but as a Juglet ranker it is the **worst** arm (6/9 seated, 17/36 edges).
- The 4-DoF `c_in` rule is the **best Juglet rule** (9/9 at 4.06 mm, 25/36 edges) and `K=1` on Pot A as designed — but its pre-polish `c_in` order picks the wrong Pot A basin on P06/P08, dropping the promotion control to `7/8`, `13/15`.
- `k1` is not a separate rule: it is `a1_4dof_cin` and inherits its Pot A failure.

Reading: blocked: control-or-input-failed — `pota_a1_4dof_cin` does not hold the Pot A positive control (`7/8`, `13/15`, piece 08 axis 19.49 deg / 39.62 mm off `axes323`) although the rig controls do: `axes_pota_shipped_base` is `8/8` byte-identical and seats `8/8` `15/15` at 2.06 mm in the same rig, t366's 2-DoF `a1_k0` seats `8/8` `15/15` at 2.01 mm, and `juglet_oracle_xy` is `9/9`. The 4-DoF `c_in` ranker cannot be promoted as-is; the fix is candidate ORDER on Pot A (post-polish cost re-sort, the shipped rule), which ticket 367 outlaws by construction and therefore hands back to the lead.

**End state:** blocked

### Lead verification (2026-11-19)

- `verified: C:/PR/artifacts/dsh-handover/367/accept.py -> ACCEPTANCE PASSED (exit 0); C:/PR/artifacts/t367/render367_ablation.png inspected visually (Panels 1-5 show closed 9/9 Juglet vessels with Base Piece 7 upright at the foot and 18/18 green bridges within 5 deg / 5 mm; Panel 6 pota_a1_4dof_cin_r1 shows 7/8 Pot A cluster with Piece 08 misaligned).`
- `verified: C:/PR/artifacts/t367/axes367_summary.json & C:/PR/artifacts/t367/gate367.tsv:1-269 -> axes_shipped_base is 8/9 byte-identical to t363 axes_def3x (only Piece 7's 3 lines negated by orient_base_axis, z_wall_minus_z_pole=-28.64, SignAxisGuard 0/18 = 0.00%); axes_pota_shipped_base is 8/8 byte-identical by cmp to t323 axes323 (0 lines flipped); axes_a1_4dof_cin has K=[1,2,2,1,1,1,1,1,3] on Juglet (SignAxisGuard 0/18, rank-0 deg_signed=[0.95, 15.73 (cand1 9.51), 4.92, 7.40, 1.06, 2.58, 4.44, 3.80, 6.58], d_perp_sherd_mm=[0.12, 0.68, 0.60, 0.95, 0.39, 0.89, 0.33, 0.28, 3.58]) and K=[1,1,1,1,1,1,1,1] on Pot A (pre-polish c_in order kept top[0]=19.90 deg on P06 and 18.98 deg on P08 instead of post-polish c_ref winners 11.35 deg [shift 0.926 deg] on P06 and 1.20 deg [shift 0.831 deg] on P08). Note: ticket prose line 162 wrote K=1 on all 9 pieces by slip; actual K from line 136 and axes367_summary.json is [1,2,2,1,1,1,1,1,3] (CANDTOTAL=431) vs [1,1,1,1,1,1,1,1,1] (CANDTOTAL=240) for a1_4dof_k1.`
- `verified: C:/PR/artifacts/t367/score/*.json & C:/PR/artifacts/t367/gate367.tsv:270-488 -> juglet_a1_4dof_cin_r1 achieves 9/9 sherds, 12/18 stock edges (25/36 directed), 18/18 centroid edges (<= 2.13 deg / 0.91 mm), 9/9 seated <= 4.596 mm even on score58 origin gauge at max 4.060 mm (12.18 tree, tighter than juglet_oracle_xy's 4.191 mm = 12.57 tree), and 9/9 at 0.468 mm on vertex-centroid Kabsch (r2: 8/9 at 4.774 mm origin gauge, 9/9 at 0.483 mm vertex gauge, 11/18 edges); juglet_a1_4dof_k1_r1 achieves 9/9 at 4.149 mm origin gauge (0.479 mm vertex gauge, 11/18 edges; r2: 8/9 at 4.773 mm, 0.472 mm vertex gauge); juglet_shipped_base_r1/r2 achieves 9/9 sherds in 1 cluster at 0.476 / 0.466 mm vertex gauge and 18/18 centroid edges (<= 1.73 deg / 0.86 mm in r1), with 6/9 (5.559 mm) / 5/9 (6.067 mm) on score58 origin gauge and 8/18 stock edges; pota_shipped_base_r1/r2 and pota_a1_k0rig_r1/r2 both pass 8/8 sherds, 15/15 edges, 8/8 seated at 2.011-2.064 mm, while pota_a1_4dof_cin_r1/r2 stops at 7/8 sherds, 13/15 edges (Piece 08 out, 75.38 / 75.50 mm) because pre-polish c_in order dropped P08's 1.20 deg post-polish c_ref winner (eliminating upright CAND338 on pair 4-8).`

