# 364: Isolate axis tilt (`deg`) vs pivot offset (`d_perp`) in `def_3x` assembly, and trace where `sfs_axes` loses the pivot on Juglet

**What to build:** a **measurement and ablation run** (zero source edits to `class/`, `SfSpp_preprocessing/sfs_axes/`, or `Dataset/`) with two parts:
1. **Part A (`t363` rig ablation):** Keep everything in `t363` identical (`def_3x` lanes, floor-10 shim, `XY_BREAK=1`, `XY_STATE=1`) and swap only the two halves of the `Axis.xyz` line (`pos` vs `dir`) between `axes_def3x` and `Dataset/Juglet_large`:
   - `juglet_own_dir_gt_pos`: direction from `axes_def3x` (`0.95°–15.73°` tilt, keeping `K` lines per piece), paired with `Dataset/Juglet_large`'s GT point `pos` (`d_perp = 0`, removing the `7–135 mm` pivot offset while keeping the `1°–16°` tilt).
   - `juglet_gt_dir_own_pos`: exact GT direction from `Dataset/Juglet_large` (`deg = 0`), paired with `axes_def3x` rank-0 `pos` (`d_perp = 6.86–134.62 mm` real / `20.57–403.86` tree units, isolating pure pivot offset).
   - `juglet_own_dir_zsign_flip7` (cheap 1-line check? Optional or replace? Better: test `juglet_own_dir_gt_pos` both as shipped `dir_z >= 0` AND keep `juglet_oracle_xy` + `pota_def_xy` controls; wait — also check `juglet_own_dir_gt_pos` (`r1`, `r2`) and `juglet_gt_dir_own_pos` (`r1`, `r2`) = **6 array tasks total** including the 2 controls!).
2. **Part B (`sfs_axes` sub-stage trace, zero edits):** Instrument via a standalone read-only runner script on a Spartan compute node calling into `d272313`'s `sfs_axes` (`tree 6a67364b`, NOT `4c2aec3`) on `t80`'s `(S0, S1)` walls for all 9 Juglet sherds (and Pot A's 8 sherds as baseline). Measure **at every sub-step inside `run_potsac_union` -> `_cluster_union` -> `refine_axis_normalized` -> `rank_by_profile` -> `refine_axis_profile` -> `keep_lines`**:
   - Does any **raw PotSAC basin** (across all basins, and within `PROFILE_BASINS = 60`) already have BOTH `deg <= 5°` AND `d_perp <= 15 mm` (`<= 45` tree units)?
   - Does `refine_axis_normalized` improve or worsen `(deg, d_perp)`?
   - Does `refine_axis_profile` (`profile.py:59` Nelder-Mead) pull `p0` (`d_perp`) **toward** or **away from** the GT axis (record `before_profile_deg / before_profile_d_perp_mm` vs `after_profile_deg / after_profile_d_perp_mm` for the top-8 distinct lines and rank-0)?

**Answers:** S1
**Blocked by:** None (Ticket 363 resolved `2026-11-18` at `0/9` seated on `axes_def3x` vs `9/9` on `juglet_oracle_xy`).
**Status:** ready-for-agent
**Working in:** unassigned
**Branch:** unassigned
**Worker:** unassigned
**Budget:** medium 5
**Hand-over:** DeepSeek — Spartan compute node (read-only over `class/`, `sfs_axes/`, frozen `Dataset/`, and existing `runs/`). Check path: the lead re-reads `C:/PR/artifacts/t364/gate364.tsv`, verifies `sacct -X`, checks hybrid axis file construction (`deg` and `d_perp` properties), verifies `score/*.json` and `[SFS-338]` Piece 7 candidates in the hybrid arms, and looks at `C:/PR/artifacts/t364/render364_ablation.png` before recording any verdict.

## Why this ticket exists (verified in Ticket 363 — do not re-derive)

- Ticket 363 showed a sharp paradox: on `t80`'s clean `(S0, S1)` walls, `axes_def3x` actually hits **`8/9 <= 10°`** rank-0 directions (`0.95°–9.60°`; only P2 `15.73°`), yet seats **0/9** (`1/18` edges, `191.6–218.0 mm` real chamfer), with Piece 7 producing `0/14` upright poses across all partners and the `{1,2,3,5,6,8}` cluster twisted (`3-5` `52.4°`, `1-2` `127.2°`).
- Inspecting `class/filter.cpp:173-218` shows why `d_perp` matters just as much as `deg`: LCS computes `Dist(i) = sqrt(x^2 + y^2)` and `Theta_Diff = Dist(i) * dtheta/ds` around `axis_point_`. Direct comparison on Spartan showed Pot A (`8/8`!) has `d_perp = 1.82–7.21 mm` on `P01–P03`, whereas Juglet `axes_def3x` has **`d_perp = 22.9–134.6 mm` real (`68.7–403.9` tree units) on `P2–P8`**.
- To design a targeted fix to `sfs_axes`, we must first measure cleanly: (1) whether `0/9` is caused by the **`d_perp` pivot error**, the **`deg` tilt error**, or both together; and (2) whether `potsac` never finds small `(deg, d_perp)` axes on Juglet `(S0, S1)` walls or `profile.py` / `select.py` ruins or discards them.

## Part A — `dir`-vs-`pos` ablation on the `t363` rig (6 array tasks)

1. Reuse `20261118-0247_4beb57f_t363-own-axes` / `t362` container, floor-10 shim (`0c216463ae27fe4571b200cc2292da99` for Juglet; `daa9d614cfa78de79584cd26933a2057` for Pot A), and `t80` `def_3x` Breaklines/Surfaces/Meshes/GT staging. Never edit `class/` or `Dataset/`.
2. Build two hybrid axis directories in `<run>/work/`:
   - `axes_own_dir_gt_pos/`: for each piece `1..9`, for every candidate line `k = 0..K-1` in `.../20261118-0247_4beb57f_t363-own-axes/out/axes_partA/axes_def3x/Juglet_Piece_N_Axis.xyz`, write `pos = gt_pos` (exact XYZ from `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Juglet_Piece_N_Axis.xyz` columns `0:3`), `dir = own_dir[k]` (exact unit direction from `axes_def3x` line `k` columns `3:6`). Verify orthogonality/distance: `d_perp` to the GT axis at `gt_pos` is `0.000 mm` and `deg` equals `axes_def3x`'s exact direction error!
   - `axes_gt_dir_own_pos/`: for each piece `1..9`, write 1 line with `pos = own_pos[0]` (exact XYZ from `axes_def3x` rank-0 line columns `0:3`), `dir = gt_dir` (exact direction from `Dataset/Juglet_large` columns `3:6`). Verify: `deg = 0.00°` and `d_perp_mm` equals `axes_def3x`'s exact `d_perp` (`6.86–134.62 mm` / `20.57–403.86` tree units)!
3. Run 6 arms in one array job (`--array=0-5`, `XY_BREAK=1`, `XY_STATE=1`, `FLOOR=10` for Juglet, `FLOOR=50` for Pot A):
   - `juglet_own_dir_gt_pos_r1`, `juglet_own_dir_gt_pos_r2`
   - `juglet_gt_dir_own_pos_r1`, `juglet_gt_dir_own_pos_r2`
   - `juglet_oracle_xy` (positive control, must seat `9/9`)
   - `pota_def_xy` (positive control, must hit `8/8, 15/15`)
4. Poll every `sbatch` with `scripts/slurm_poll.sh` / `sacct -X` and record final `State/ExitCode`. Score all 6 with `score58` (`13.788` 3× tree bar = `4.596 mm` real for Juglet; `4.596 mm` for Pot A) and check Piece 7's `[SFS-338] CAND338` upright count (`rot < 20°`) in both hybrid arms.

## Part B — Sub-stage trace inside `sfs_axes` (`tree 6a67364b`, zero code edits)

1. Run one compute-node Python job (`t364-basins`) importing the unmodified `src_axes` from `20261118-0247_4beb57f_t363-own-axes/src_axes` (verify md5 matches `d272313`'s `sfs_axes` tree `6a67364b`) on all 9 Juglet `t80` `def_3x` `(S0, S1)` walls and all 8 Pot A `(S0, S1)` walls (`Pot_A` corpus surfaces).
2. Replicate `run_potsac_union(C0, C1, seeds=list(range(10)), step=10, max_candidates=6, normalized=True, line_rule=True)` step-by-step in a read-only diagnostic script (without modifying `sfs_axes/*.py`) and record for every piece (`Pot A P01..08`, `Juglet P1..9`) against GT axes:
   - Total `_cluster_union` basins (`M`), and best `(deg, d_perp_mm)` across **all `M` basins** vs across **top `PROFILE_BASINS=60` basins**.
   - Count of basins in all `M` and top `60` satisfying `(deg <= 5°, d_perp <= 15 mm)` and `(deg <= 10°, d_perp <= 15 mm)`.
   - Effect of `refine_axis_normalized` on those 60 basins (median & min `deg` and `d_perp_mm` before vs after).
   - Effect of `refine_axis_profile` (`profile.py:59`) on the top-8 distinct lines entering `rank_by_profile`: `(cost, deg, d_perp_mm)` **before** `refine_axis_profile` vs **after** `refine_axis_profile`, specifically noting whether `refine_axis_profile` **increases** or **decreases** `d_perp_mm` and `deg` for rank-0 and for the best line.

## Part C — Deliverables and visual check

- `C:/PR/artifacts/t364/gate364.tsv` — exact 7-column header `group\tpiece\tmetric\tvalue\treference\tverdict\tnote`, with unambiguous unit suffixes (`_tree` vs `_mm`, converting Juglet tree units to real mm via `/ 3.0` in `note`) covering Part A controls, both hybrid arms (r1/r2 + per-piece + Piece 7 upright `CAND338` count), and Part B basin/refinement trace for Juglet `P1..9` & Pot A `P01..08`.
- `C:/PR/artifacts/t364/render364_ablation.png` — four labelled panels at `[SFS-336] FINAL` poses: `juglet_own_dir_gt_pos` (best repeat), `juglet_gt_dir_own_pos` (best repeat), `juglet_oracle_xy`, and `pota_def_xy`, bridges labelled with `PlaceErr`. **Look at it before picking the `Reading:` line** and include one sentence describing what the eye sees.
- `C:/PR/artifacts/t364/partB/basins364.json` — raw JSON so the lead can independently recompute Part B stats without Spartan.
- Append one `runs/index.tsv` row (leaving `verdict` and `human_note` blank).

## Readings (pick one verbatim on a bare `Reading:` line)

- `pivot-offset-drives-collapse` — `juglet_own_dir_gt_pos` seats `>= 7/9` (`d_perp = 0`, keeping our `1°–16°` tilts) while `juglet_gt_dir_own_pos` seats `0/9` (`deg = 0`, keeping our `7–135 mm` pivots); removing `d_perp` rescues assembly.
- `tilt-error-drives-collapse` — `juglet_gt_dir_own_pos` seats `>= 7/9` while `juglet_own_dir_gt_pos` seats `0/9`; direction accuracy (`deg`) is the main gate.
- `both-pivot-and-tilt-required` — both `juglet_own_dir_gt_pos` and `juglet_gt_dir_own_pos` fail to seat cleanly (`< 7/9`) or produce contrasting partial clusterings/seating rates; numbers for both and Part B basin root cause are recorded.
- `blocked: control-or-input-failed` — `pota_def_xy` fails `8/8, 15/15`, `juglet_oracle_xy` fails `9/9`, or `d272313` `sfs_axes` cannot be verified.

## Out of scope

- Any edit to `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`.
- Proposing or tuning new axis heuristics in code before this ablation settles `(deg` vs `d_perp)` and `(PotSAC basins` vs `profile.py)`.
- Committing or pushing from the worker.

## Acceptance

- [ ] Part A: both hybrid axis dirs verified by math check (`d_perp == 0` / `deg == 0`), plus 6 arms (`2 + 2 + 2 controls`) run with `sacct -X` `COMPLETED 0:0`, `score58` (`13.788` tree bar = `4.596 mm`), and Piece 7 `CAND338` upright count.
- [ ] Part B: step-by-step `sfs_axes` (`tree 6a67364b`) basin + `refine_axis_normalized` + `refine_axis_profile` trace saved for all 9 Juglet pieces and 8 Pot A pieces (`basins364.json`).
- [ ] Part C: `C:/PR/artifacts/t364/gate364.tsv` (7 columns, clean units), `C:/PR/artifacts/t364/render364_ablation.png` (looked at before `Reading:`), bare `Reading:` line, and `**End state:** ready` (or `blocked:`/`failed:`).
