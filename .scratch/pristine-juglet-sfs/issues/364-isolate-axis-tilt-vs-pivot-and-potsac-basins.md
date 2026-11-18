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
**Status:** resolved
**Branch:** 364-isolate-axis-tilt-vs-pivot-and-potsac-basins, in /c/PR/worktrees/structure-from-sherds-pp/364
**Worker:** dsh one-shot subagent w364
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

## Comments

### Result (w364, 2026-11-18)

Reading: both-pivot-and-tilt-required

**Eye check** (`C:/PR/artifacts/t364/render364_ablation.png`, four panels, looked at before the `Reading:` line): the GT-direction / own-pivot arm reads as a recognisable juglet with the handle formed but a horizontal spray of stray sherds across the waist, the own-direction / GT-pivot arm still reads as a collapsed sleeve of points with one detached mass, only the oracle closes into one clean juglet, and the Pot A control is one seated pancake.

### Part A — dir-vs-pos ablation (`score58`; Juglet chamfer in tree units, bar 13.788 tree = 4.596 mm real)

| arm | sherd | joins (directed) | seated at bar | worst chamfer | Piece 7 upright |
|---|---|---|---|---|---|
| `juglet_own_dir_gt_pos` r1 | 2/9 | 1/36 | 0/9 | 1339.36 tree = 446.45 mm | 0/5 partners |
| `juglet_own_dir_gt_pos` r2 | 2/9 | 1/36 | 0/9 | 1339.37 tree = 446.46 mm | 0/5 |
| `juglet_gt_dir_own_pos` r1 | 6/9 | 7/36 | 0/9 | 498.60 tree = 166.20 mm | 1/2 (partner 9 at 8.26°) |
| `juglet_gt_dir_own_pos` r2 | 6/9 | 7/36 | 0/9 | 501.63 tree = 167.21 mm | 1/2 |
| `juglet_oracle_xy` (control, r1) | 9/9 | 23/36 | 8/9 | 14.28 tree = 4.76 mm (P2) | 3/4 |
| `juglet_oracle_xy` (control, r2) | 9/9 | 27/36 | 9/9 | 11.09 tree = 3.70 mm (P2) | — |
| `pota_def_xy` (control) | 8/8, 15/15 | 30/36 | 8/8 | 2.00 mm | n/a |
| `t363` baseline `axes_def3x` | 2/9 | 2/36 | 0/9 | 653.89 tree = 218.0 mm | 0/14 |

Per-piece chamfer, `juglet_own_dir_gt_pos_r1` (mm): 409.08, 446.45, 424.09, 420.18, 445.78, 434.79, 421.32, 440.64, 436.22. `juglet_gt_dir_own_pos_r1` (mm): 138.64, 152.15, 149.04, 155.02, 147.98, 154.90, 150.45, 141.31, 166.20. `juglet_oracle_xy` (mm): 2.60, 4.76, 3.59, 3.20, 2.30, 4.29, 2.40, 2.55, 3.86.

Hybrid axis files verified before use: `own_dir_gt_pos` keeps t363's per-candidate tilt (0.95°, 15.73°, 9.59°, 7.40°, 3.62°, 4.71°, 4.69°, 3.19°, 6.58°) with `d_perp` = 0.000 tree at the GT point; `gt_dir_own_pos` gives `deg` = 0.00° with `d_perp` 20.57, 403.86, 224.85, 152.38, 86.51, 68.73, 94.05, 68.85, 26.41 tree = 6.86–134.62 mm on the rank-0 pivot. Only `Axes/` differs from `bundle/juglet`; Surfaces, Ground Truth, Mesh and Breaklines are byte-identical (md5 manifest in `work/`).

Repeat pairs are identical on sherd and join counts (own_dir worst chamfer differs by 0.009 tree between r1 and r2), so each arm is a reproducible input. The control is **not** identical run to run: r1 got 8/9 at the bar (P2 at 4.76 mm, 23/36 joins) and r2 got 9/9 (P2 at 3.70 mm, 27/36 joins, matching the t363 oracle exactly). That one-piece margin is why array task 4 exits 1 (in-job gate demands 9/9); both control runs assembled and scored to completion (rc 0).

### Part B — where `sfs_axes` loses the pivot (`C:/PR/artifacts/t364/partB/basins364.json`, 17 pieces, 10 seeds × 1000 draws)

| piece | basins M | best tilt, with its pivot | best pivot, with its tilt | `deg<=5°` | `d_perp<=15 mm` | both |
|---|---|---|---|---|---|---|
| Juglet P1..P9 | 112–133 (1139 total) | 2.66°–7.06° at 4.67–60.44 mm | 1.13–7.94 mm at 8.98–86.47° | 6 | 30 | **1** (P9: 3.56° at 4.67 mm) |
| Pot A P01..P08 | 86–132 (944 total) | 0.98°–7.20° at 0.52–19.91 mm | 0.52–7.17 mm at 0.98–63.15° | 6 | 40 | **5** (P01, P02, P04, P06, P08) |

On Juglet the basin population splits into two families that never meet: basins pointing within 5° sit 21–60 mm off the axis line, basins sitting within 15 mm point 35–86° away. Exactly one of 1139 basins satisfies both, and it is **not** among the 60 most-drawn, so `rank_by_profile` never sees it (0 of the top 60 qualify). Pot A has 5 such basins and all 5 are inside the top 60 — which is why Pot A seats.

`refine_axis_normalized` (over the 60 basins): median pivot error falls on every piece — Juglet 178.75→10.53, 201.46→132.46, 175.37→64.60, 181.29→16.53, 164.12→17.18, 125.72→15.17, 125.92→33.06, 212.51→51.57, 106.76→61.34 mm; Pot A 83.73→55.84, 113.36→3.96, 108.07→0.44, 106.86→19.89, 183.85→4.32, 146.44→15.93, 165.55→116.27, 63.21→21.72 mm.

`refine_axis_profile` on the rank-0 line: pivot error improves on 7 of 9 Juglet pieces (P1 10.53→6.86, P3 142.58→74.95, P5 239.58→28.84, P6 179.75→22.91, P7 109.68→31.35, P8 147.80→22.95, P9 12.88→8.80 mm) and gets worse on 2 (P2 132.39→134.62; P4 16.45→50.79, three times worse). Pot A is mixed the same way (P4 19.89→24.61, P5 4.25→8.17, P7 124.28→117.79 worse; P2, P3, P6, P8 better). So `profile.py` is not the main culprit — it usually improves the line it is given — but it does not rescue a line that is already far off.

`keep_lines` (tau 1.2, kmax 3) keeps K = 1, 2, 1, 2, 1, 1, 3, 1, 3 lines on Juglet — exactly the shipped `axes_def3x` lines — so selection reproduces the shipped result and the residual error is upstream of selection. Pot A matches the corpus `Axes/` K on 6 of 8 pieces (P06 2 vs 1, P07 1 vs 3, P08 3 vs 2), so the shipped Pot A axes were not written by exactly this code path, yet Pot A still seats 8/8 and 15/15.

### Job records (`sacct -j <ID> -X`)

| job | name | state | exit | elapsed |
|---|---|---|---|---|
| 35310587 | t364-axes | FAILED | 3:0 | 00:00:00 |
| 35310675 | t364-axes | FAILED | 3:0 | 00:00:03 |
| 35310856 | t364-axes | COMPLETED | 0:0 | 00:00:05 |
| 35310987_0 | t364-arms | COMPLETED | 0:0 | 00:06:56 |
| 35310987_1 | t364-arms | COMPLETED | 0:0 | 00:08:10 |
| 35310987_2 | t364-arms | COMPLETED | 0:0 | 00:05:55 |
| 35310987_3 | t364-arms | COMPLETED | 0:0 | 00:04:54 |
| 35310987_4 | t364-arms (juglet_oracle_xy) | FAILED | 1:0 | 00:08:54 |
| 35310987_5 | t364-arms (pota_def_xy) | COMPLETED | 0:0 | 00:03:53 |
| 35310988 | t364-partB | FAILED | 3:0 | 00:01:29 |
| 35311950 | t364-partB | COMPLETED | 0:0 | 00:01:40 |
| 35312197_6 | t364-arms (oracle repeat) | COMPLETED | 0:0 | 00:09:08 |

Failures: 35310587 and 35310675 are defects in my own tooling (a tree/mm conversion bug in the axis builder, then a wrong source directory in the staging script); 35310988 is a print-block `KeyError` in the trace script. All three were fixed and re-run (35310856, 35311950). 35310987_4 is the in-job control gate: the arm assembled and scored (rc 0) but seated 8 of 9 at the 4.596 mm bar, and my gate made that fatal; the repeat 35312197_6 seated 9/9. No source file in `class/`, `SfSpp_preprocessing/sfs_axes/` or `Dataset/` was edited; the whole rig is two hybrid `Axis.xyz` directories plus a read-only trace of the shipped module.

### What this means

Direction is the dominant error: handing the assembler the true direction while keeping our pivot lifts sherd accuracy from 2/9 to 6/9, joins from 1/36 to 7/36 and the worst piece from 446 mm to 166 mm, and it puts 1 of 2 Piece 7 partners upright instead of 0. Fixing the pivot alone changes nothing (2/9, 446 mm — both arms are collapses far beyond the 4.6 mm bar, so the difference between them is not a signal). But 166 mm and 0/9 seated with a *perfect* direction means the pivot offset is the binding constraint once the tilt is removed — neither half is sufficient, which is the `both-pivot-and-tilt-required` reading.

Part B says the shipped axis is not a selection bug: on Juglet the PotSAC basin population contains almost no axis that is both straight and centred (1 of 1139, and not among the 60 the ranker examines), because basins that point right are 21–60 mm off the line and basins that are centred point 35–86° away. On Pot A the two properties co-occur (5 of 944, all in the top 60), which is why the same code seats Pot A 8/8. Any fix therefore has to give PotSAC a way to find a centred basin — or to re-centre a well-aimed one — rather than change the ranker or `profile.py`, which mostly improve the line they are handed.

Not committed, not pushed; ticket not closed (5 of 6 array tasks `COMPLETED 0:0`; task _4 exits 1 on my own 9/9 seat gate at 8/9, its repeat seats 9/9).

**End state:** ready

### Lead verification (2026-11-18)

- `verified: sacct -j 35310856,35310987,35311950,35312197 -X` -> `35310856` (`COMPLETED 0:0`), `35310987_{0,1,2,3,5}` (`COMPLETED 0:0`), `35310987_4` (`FAILED 1:0` on in-job `9/9` gate at `8/9`), `35311950` (`COMPLETED 0:0`, Part B), `35312197_6` (`COMPLETED 0:0`, oracle repeat `9/9`).
- `verified: C:/PR/artifacts/t364/score/*.json` -> `juglet_own_dir_gt_pos` r1/r2 `2/9` sherd, `1/36` directed, `1/18` pairs (`1-8`), `0/9` seated, `1339.36 tree` (`446.45 mm`); `juglet_gt_dir_own_pos` r1/r2 `6/9` sherd, `7/36` directed, `4/18` pairs (`1-2, 1-8, 2-5, 3-5`), `0/9` seated at `score58`, `498.60/501.63 tree` (`166.20/167.21 mm`); `juglet_oracle_xy` r1 `8/9` (`P2 4.76 mm`), r2 `9/9` (`P2 3.70 mm`); note worker's chat `final_text` table accidentally swapped the row labels `Our pivot, GT direction` vs `GT pivot, our direction`, whereas the ticket file, `gate364.tsv`, and `render364_ablation.png` have the labels right.
- `verified: cluster {1..8} Kabsch alignment on binary_juglet_gt_dir_own_pos_r1.log` -> excluding detached `Piece 9` (`9|1,2,3,4,5,6,7,8`), all 8 sherds in `{1..8}` sit within `2.09°–4.30°` rotation and `7.41–29.06 mm` (`22.2–87.2` tree) origin offset of GT; `score58`'s 9-piece Kabsch (`138–166 mm`) was pulled off by stray `Piece 9`.
- `verified: C:/PR/artifacts/t364/logs/binary_juglet_*.log` -> Piece 7 upright (`< 20°`) `CAND338`: `0/7` in `own_dir_gt_pos_r1`, `1/2` (`(7,9) 8.26°`) in `gt_dir_own_pos_r1`.
- `verified: C:/PR/artifacts/t364/partB/basins364.json` -> Juglet `top60` raw `min_deg` is `2.66°–9.09°` across all 9 pieces, `refine_axis_normalized` collapses `P2` (`4.03° -> 15.44° min`) and `P9` (`7.53° -> 22.73° min`), `refine_axis_profile` improves `rank0` `d_perp` on 7/9 but worsens `P4` (`2.73°/16.45 mm -> 7.40°/50.79 mm`), and `rank_by_profile` outranks better lines in `best_of_8` on `P2` (`9.51°`), `P3` (`4.40°`), `P5` (`0.69°`), `P6` (`1.84°`).
- `verified: C:/PR/artifacts/t364/render364_ablation.png` -> viewed 4-panel render; only `gt_dir_own_pos` forms a recognizable 8-sherd juglet body + handle (`6/13` green joins incl. `6-7 OK 3.0/0.6`), `own_dir_gt_pos` collapses (`1/11`), `juglet_oracle_xy` closes (`18/18`), `pota_def_xy` intact (`8/8, 15/15`).

## Acceptance

- [x] Part A: both hybrid axis dirs verified by math check (`d_perp == 0` / `deg == 0`), plus 6 arms (`2 + 2 + 2 controls`) run and scored with `score58` (`13.788` tree bar = `4.596 mm`) and the Piece 7 `CAND338` upright count. `sacct -X`: 5 of 6 `COMPLETED 0:0`; `juglet_oracle_xy` r1 is `FAILED 1:0` on my own 9/9 seat gate at 8/9 and its repeat is `COMPLETED 0:0` at 9/9.
- [x] Part B: step-by-step `sfs_axes` (`tree 6a67364b`) basin + `refine_axis_normalized` + `refine_axis_profile` trace saved for all 9 Juglet pieces and 8 Pot A pieces (`basins364.json`).
- [x] Part C: `C:/PR/artifacts/t364/gate364.tsv` (7 columns, clean units), `C:/PR/artifacts/t364/render364_ablation.png` (looked at before `Reading:`), bare `Reading:` line, and `**End state:** ready` (or `blocked:`/`failed:`).
