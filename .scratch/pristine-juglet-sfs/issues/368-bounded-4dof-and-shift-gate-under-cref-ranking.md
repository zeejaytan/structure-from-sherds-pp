# 368: Bounded 4-DoF and shift-gated post-polish `c_ref` ranking under `orient_base_axis` on `def_3x` Juglet & Pot A

**What to build:** a **measurement and prototype ablation run** (zero edits to tracked `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`; all prototype code lives strictly inside `<run>/work/`) testing whether bounding or gating 4-DoF direction drift under shipped post-polish `c_ref` ranking (`fin.sort()`, `keep_lines(c_ref, tau=1.2, kmax=3)`, `orient_base_axis`) simultaneously:
1. keeps **Pot A** at `8/8` sherds and `15/15` edges (`<= 4.596 mm`, preserving `P08 = 1.20°` and `P06 = 11.35°` where `polish_shift_deg < 1°` under `c_ref` sorting); AND
2. achieves **`9/9` closed Juglet assembly** matching `a1_4dof_cin` / `a1_base` by stopping unconstrained 4-DoF direction walks (`10.1°–40.8°` on Juglet `P3, P5, P6`).

Two geometric guards (zero per-pot constants, zero GT, plus `15.0°` shipped dedup vs `5.0°` dedup in Part A) are compared over the **exact same captured 60-basin pool**:

1. **`bounded6_d15`** (smooth direction-tilt cap inside Nelder-Mead, `dedup = 15.0°`):
   - Replace `refine_axis_profile` with `refine_axis_profile_bounded(vt, walls, centre, size, max_tilt_deg=6.0, maxfev=400)`: keep axial pivot shifts `(x[2], x[3])` completely free, while smoothly clamping the tilt radius `r = hypot(x[0], x[1])` to `r_max = tan(radians(6.0))` (if `r > r_max`, scale `(x[0], x[1])` by `r_max / r`).
   - Run shipped `rank_by_profile` (`dedup = 15.0°`, `top[:8]`, post-polish `c_ref` sort, `dedup = 15.0°`, `[:6]`), `keep_lines(c_ref, tau=1.2, kmax=3)`, then `orient_base_axis`.
2. **`shiftgate6_d15`** (post-polish direction-excursion gate, `dedup = 15.0°`):
   - Run unmodified `refine_axis_profile(v_in, walls, centre, size)`; if `direction_angle_deg(v_ref[:3], v_in[:3]) > 6.0`, reject the drifted `v_ref` and emit `(c_in, v_in)` instead of `(c_ref, v_ref)`.
   - Run shipped post-polish sort (`fin.sort()`), `dedup = 15.0°`, `[:6]`, `keep_lines(c, tau=1.2, kmax=3)`, then `orient_base_axis`.
3. **Part A `d5` audit (`bounded6_d5` & `shiftgate6_d5`) and `top[0..7]` diagnostics:**
   - Run identical variants with `dedup = 5.0°` on the same captured pool, and dump full `top[0..7]` tables (`c_in`, `c_ref_unbounded`, `shift_unbounded_deg`, `c_ref_bounded`, `shift_bounded_deg`, `deg_in`, `deg_ref_unbounded`, `deg_ref_bounded`) on every Juglet and Pot A piece.

**Answers:** S1
**Blocked by:** None (Tickets 363–367 resolved 2026-11-18..19).
**Status:** resolved
**Branch:** 368-bounded-4dof-and-shift-gate-under-cref-ranking, in /c/PR/worktrees/structure-from-sherds-pp/368
**Worker:** dsh one-shot subagent w368
**Budget:** medium 5
**Hand-over:** DeepSeek — Spartan compute node (read-only over `class/`, `sfs_axes/`, `Dataset/`, and existing `runs/`). Check path: `C:/PR/artifacts/dsh-handover/368/accept.py` + lead verification of `C:/PR/artifacts/t368/axes368_summary.json`, `C:/PR/artifacts/t368/gate368.tsv`, `sacct -X` (all jobs `COMPLETED 0:0`), 9-piece origin & vertex-centroid Kabsch, centroid-gauge `CountResult`, and visual inspection of `C:/PR/artifacts/t368/render368_ablation.png`.

## Why this ticket exists (from Ticket 367)

- Ticket 367 (`runs/20261119-1023_273b8f9_t367-shipped-vs-4dof-cin`) established both halves of the ranking dilemma:
  - **`shipped_base`** (`sfs_axes` `tree 6a67364b` + `orient_base_axis`) is `8/8` byte-identical to `axes323` on Pot A (`8/8, 15/15` at `2.02–2.06 mm`) and closes Juglet `9/9` (`0.47–0.48 mm` vertex gauge), but drops origin-gauge seating to `6/9` (`5.56 mm`, `8/18` stock edges).
  - **`a1_4dof_cin`** (pre-polish `c_in` order + 4-DoF polish, no `L_pre`) achieves our best Juglet (`9/9` seated at `4.060 mm` origin gauge, `0.468 mm` vertex gauge, `12/18` stock edges — tighter than `4.191 mm` oracle), **yet** drops Pot A Piece 08 (`7/8, 13/15`) because `c_in` ordering puts the `18.98°` second basin first instead of `1.20°` (`0.831°` shift under 4-DoF polish).
  - Meanwhile in `t367/axes367_summary.json`, Pot A true `c_ref` winners (`P06`, `P08`) shifted direction by **`< 1°`** (`0.926°`, `0.831°`), whereas false `c_ref` basins on Juglet `P3, P5, P6` walked **`10.1°–40.8°`** away from PotSAC normals. Stopping those `> 6°` 4-DoF direction drifts while keeping post-polish `c_ref` ordering should unify Pot A (`8/8, 15/15`) and Juglet (`9/9`).

## Part A — Axis extraction, `top[0..7]` diagnostics, and scoring (Spartan compute node)

1. Create `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t368-bounded-4dof-cref/` (`work/`, `out/axes/`, `logs/`). Copy `src_axes` (`tree 6a67364b`) from `/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0247_4beb57f_t363-own-axes/src_axes` into `<run>/work/src_axes_368/` and copy `base_axis.py` from `20261118-1435_78581fb_t366-base-sign-4dof/work/base_axis.py`.
2. On a Spartan compute node via `sbatch` (polled via `scripts/slurm_poll.sh`), run `run_potsac_union(..., seeds=10, step=10, max_candidates=6, normalized=True, line_rule=True)` and `build_pool` ONCE per piece on all 9 Juglet `t80` `def_3x` `(S0, S1)` walls (`/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0430_d272313_t80-default-on/out/cell_def_3x/geom/`) and all 8 Pot A corpus `(S0, S1)` walls (`/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Surfaces/`).
3. From that captured pool, build 8 axis directories under `<run>/out/axes/`:
   - `axes_bounded6_d15` & `axes_pota_bounded6_d15`: `dedup = 15.0°`, `top[:8]`, `refine_axis_profile_bounded(max_tilt_deg=6.0)`, post-polish `c_ref` sort, `_distinct(..., 15.0)`, `[:6]`, `keep_lines(c_ref, tau=1.2, kmax=3)`, `orient_base_axis`.
   - `axes_shiftgate6_d15` & `axes_pota_shiftgate6_d15`: `dedup = 15.0°`, `top[:8]`, unmodified `refine_axis_profile`, emit `(c_in, v_in)` if `direction_angle_deg(v_ref[:3], v_in[:3]) > 6.0` else `(c_ref, v_ref)`, post-polish cost sort, `_distinct(..., 15.0)`, `[:6]`, `keep_lines(c, tau=1.2, kmax=3)`, `orient_base_axis`.
   - `axes_bounded6_d5` & `axes_pota_bounded6_d5`: identical to `bounded6_d15` with `dedup = 5.0°`.
   - `axes_shiftgate6_d5` & `axes_pota_shiftgate6_d5`: identical to `shiftgate6_d15` with `dedup = 5.0°`.
4. Score all 8 sets into `C:/PR/artifacts/t368/axes368_summary.json`:
   - Juglet vs `/data/gpfs/projects/punim2657/Dataset/Juglet_large` (`scale=3.0`) + simulated `SignAxisGuard` (`0/18` required on all 4 Juglet sets).
   - Pot A vs **all three references** (`Dataset/Axes/Pot_A`, `t323` `axes323`, and true `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Ground Truth Axes/` — check existence on Spartan first and record exact path/availability).
   - Record full `top[0..7]` diagnostics (`c_in`, `c_ref_unbounded`, `shift_unbounded_deg`, `c_ref_bounded`, `shift_bounded_deg`, `deg_in`, `deg_ref_unbounded`, `deg_ref_bounded`, plus Pot A vs `axes323` angle/offset) for every piece.

## Part B — Assembly runs (Spartan compute node via `sbatch`)

- Reuse the exact `t363`/`t367` container, shims (`0c216463ae27fe4571b200cc2292da99` Juglet floor 10; `daa9d614cfa78de79584cd2057` Pot A floor 50), fixed banner check (`grep -Eq "xy break normals: (1|on)"`), non-fatal Pot A reporting so every completed task exits `0:0` (`COMPLETED`), and **the fixed vertex-centroid `cluster366.py` (laptop md5 `81f347cc...`, NOT the stale `0f8be557...` copy)**.
- **Slurm array `--array=0-7` (plus `8-11` if Part A triggers the `d5` arm):**
  - `0: juglet_bounded6_d15_r1` (`axes_bounded6_d15`)
  - `1: juglet_bounded6_d15_r2` (`axes_bounded6_d15`)
  - `2: juglet_shiftgate6_d15_r1` (`axes_shiftgate6_d15`)
  - `3: juglet_shiftgate6_d15_r2` (`axes_shiftgate6_d15`)
  - `4: pota_bounded6_d15_r1` (`axes_pota_bounded6_d15`)
  - `5: pota_bounded6_d15_r2` (`axes_pota_bounded6_d15`)
  - `6: pota_shiftgate6_d15_r1` (`axes_pota_shiftgate6_d15`)
  - `7: pota_shiftgate6_d15_r2` (`axes_pota_shiftgate6_d15`)
  - **Conditional `--array=8-11`:** if either `bounded6_d5` or `shiftgate6_d5` reduces Juglet rank-0 error by `>= 2.0°` on `>= 1` piece relative to its `d15` twin while keeping Pot A `P01..08` within `2.0°` of `axes323`, also run 2 Juglet + 2 Pot A repeats for that `d5` winner (`8-9` Juglet, `10-11` Pot A).
- Score every arm (plus include `t367` `juglet_a1_4dof_cin_r1`, `juglet_shipped_base_r1`, `pota_shipped_base_r1`, and `t366` `juglet_a1_base_r2`, `juglet_oracle_xy`, `pota_a1_k0rig` reference rows in `gate368.tsv`):
  - `score58` origin-gauge Kabsch (`13.788` tree = `4.596 mm` on Juglet; `4.596 mm` on Pot A),
  - `cluster366` vertex-centroid Kabsch (`13.788` tree = `4.596 mm` on Juglet; `4.596 mm` on Pot A),
  - Stock `CountResult` AND centroid-gauge `CountResult_centroid` / `CountResult_centroid_strict` (`<= 5.0° / 5.0 mm`),
  - `CANDTOTAL`, `[SFS-335] SIGN-AXIS`, and Piece 7 upright `CAND336` / `CAND338` / accepted joins (`3-7, 6-7, 7-8, 7-9`).

## Part C — Deliverables, Visual Check, and Hygiene

- Fetch to `C:/PR/artifacts/t368/`:
  - `C:/PR/artifacts/t368/axes368_summary.json`
  - `C:/PR/artifacts/t368/gate368.tsv` (exact 7-column header `group\tpiece\tmetric\tvalue\treference\tverdict\tnote`)
  - `C:/PR/artifacts/t368/render368_ablation.png` (6 labelled front/top panels: best `juglet_bounded6_d15`, best `juglet_shiftgate6_d15`, best `d5` arm or `t367` `juglet_a1_4dof_cin_r1`, `t367` `juglet_shipped_base_r1`, `pota_bounded6_d15_r1`, and `pota_shiftgate6_d15_r1`; inspected visually before writing `Reading:`).
  - `C:/PR/artifacts/t368/logs/`, `score/`, `result/`, `work/`.
- Append one row to Spartan `runs/index.tsv` (`verdict` and `human_note` blank).
- Record every Slurm job ID & `sacct -j ... -X` in `## Comments` (`COMPLETED 0:0`), write a verbatim bare `Reading: <slug> — ...` line, and end with `**End state:** ready` (or `blocked`).

## Readings (pick one verbatim on a bare `Reading:` line)

- `guarded-4dof-cref-passes-both-juglet-and-pota` — at least one guarded rule (`bounded6_d15`, `shiftgate6_d15`, or its `d5` twin) passes Pot A `8/8, 15/15` (`<= 4.596 mm`) on both repeats AND seats Juglet `9/9` on vertex cluster (`<= 4.596 mm`) with `8–9/9` at `<= 4.65 mm` on `score58` origin gauge (beating `shipped_base`'s `6/9` / `5.56 mm`), ready for clean promotion into `SfSpp_preprocessing/sfs_axes/`.
- `guarded-4dof-holds-pota-matches-shipped-on-juglet` — Pot A passes `8/8, 15/15` on both repeats and Juglet closes `9/9` on vertex gauge (`0.47–0.50 mm`), but origin-gauge seating does not improve on `shipped_base` (`6/9`, `5.56 mm`) or `t366` `a1_base` (`8/9`, `4.65 mm`).
- `guarded-4dof-still-regresses-pota` — even with the `6°` direction guard under `c_ref` ranking, Pot A drops below `8/8, 15/15` on one or both rules (exact piece and `top[0..7]` shift/cost cause recorded).
- `blocked: control-or-input-failed` — a named input or harness prerequisite failed.

## Out of scope

- Editing tracked files under `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`.
- Committing, pushing, merging, or running `ticket_close.sh` from the worker.

## Acceptance

- [x] Part A: `<run>/work/` builds all 8 axis sets (`bounded6_d15`, `shiftgate6_d15`, `bounded6_d5`, `shiftgate6_d5` for Juglet and Pot A) cleanly from one captured pool without touching tracked files; `axes368_summary.json` records per-candidate `unsigned_deg`, `signed_deg`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `SignAxisGuard` = `0/18` across all Juglet sets, Pot A against corpus/`axes323`/`Ground Truth Axes`, and `top[0..7]` diagnostics.
- [x] Part B: all 8 core assembly runs (`--array=0-7`, plus `8-11` if `d5` triggers) complete via `sbatch` with `sacct -X` showing `COMPLETED 0:0`; scored with `score58` origin Kabsch, fixed `cluster366` vertex Kabsch (`md5 81f347cc...`), stock & centroid-gauge `CountResult`, `CANDTOTAL`, `SIGN-AXIS`, and Piece 7 upright audit.
- [x] Part C: `C:/PR/artifacts/t368/axes368_summary.json`, `C:/PR/artifacts/t368/gate368.tsv`, `C:/PR/artifacts/t368/render368_ablation.png` (visually inspected), `runs/index.tsv` row appended, `## Comments` complete, bare `Reading:` line, `**End state:** ready` (or `blocked`).

## Comments

**Run dir:** `/data/gpfs/projects/punim2657/SfSpp/runs/20261119-0412_bf3fc0b_t368-bounded-4dof-cref` (worktree `C:/PR/worktrees/structure-from-sherds-pp/368` at `bf3fc0b`). Prototype only: all code under `<run>/work`; `class/`, `sfs_axes/`, `AxisExtraction/`, `Dataset/` and existing runs untouched (tree md5 identical before/after every arm).

**Slurm** (`sacct -X`):

```
35393460      t368-axes  COMPLETED      0:0   00:13:14
35393947_0..7 t368-arms  FAILED         1:0   00:00:00    (misfire: RUN_DIR unbound, no run, no output)
35393971_0    t368-arms  COMPLETED      0:0   00:12:25
35393971_1    t368-arms  COMPLETED      0:0   00:11:35
35393971_2    t368-arms  COMPLETED      0:0   00:13:42
35393971_3    t368-arms  COMPLETED      0:0   00:11:00
35393971_4    t368-arms  COMPLETED      0:0   00:05:44
35393971_5    t368-arms  COMPLETED      0:0   00:07:01
35393971_6    t368-arms  COMPLETED      0:0   00:06:26
35393971_7    t368-arms  COMPLETED      0:0   00:06:00
```

35393947 was submitted without `--export=RUN_DIR`, so every task died on `RUN_DIR: unbound variable` at line 23 before doing anything; it is kept here for the record. 35393971 is the real array (`--export=ALL,RUN_DIR=<run>`, shim md5 checked per arm: `0c216463ae27fe4571b200cc2292da99` floor 10 juglet, `daa9d614cfa78de79584cd26933a2057` floor 50 Pot A; axes provenance `all_equal=yes`; `tree unchanged by the run` on all 8).

**Inputs.** `src362.tar.gz` md5 `89444649dc074658101ecd1876dc0f72`, `base_axis.py` `a4d5178f62ca71c8f82c879068146294`, `cluster366.py` `81f347cc1f80b7b84a163337959ec073` (the fixed vertex-centroid Gauge). Pool captured ONCE per piece: 17 pieces (9 Juglet `t80 def_3x`, 8 Pot A corpus), 60 basins, 120 lines each, 24-54 s per piece, 9 min total; 68 piece-rows written across the 8 axis sets.

## Part A — 8 axis sets (one pool)

Juglet rank-0 unsigned angle vs `Dataset/Juglet_large` (deg, P1..P9; `SignAxisGuard` 0/18 on all four sets):

| set | K | P1 | P2 | P3 | P4 | P5 | P6 | P7 | P8 | P9 |
|---|---|---|---|---|---|---|---|---|---|---|
| `axes_bounded6_d15` | 1,3,3,3,1,1,2,1,3 | 0.95 | 15.73 | 4.92 | 8.73 | 1.06 | 3.13 | 4.44 | 3.80 | 3.82 |
| `axes_shiftgate6_d15` | 1,1,1,1,1,1,2,1,1 | 0.95 | 15.73 | 4.92 | 7.40 | 1.06 | 2.58 | 4.44 | 3.80 | 6.58 |
| `axes_bounded6_d5` | 1,3,2,3,3,2,2,2,3 | 0.95 | 15.73 | 12.85 | 8.73 | 1.06 | 3.13 | 4.44 | 2.97 | 13.48 |
| `axes_shiftgate6_d5` | 1,1,1,1,1,1,2,1,1 | 0.95 | 15.73 | 4.92 | 7.40 | 1.06 | 2.58 | 4.44 | 3.80 | 6.58 |

Pot A is the same four ways (`axes323` deviation identical in all four): P01..P08 = `0.00, 1.50, 0.47, 0.00, 0.00, 31.11, 0.00, 19.49` deg; rank-0 vs `Dataset/Axes/Pot_A` = `0.34, 0.79, 0.81, 4.07, 2.03, 19.90, 68.38, 18.98`; `SignAxisGuard` 6/15 (Pot A is not guard-scored; its shipped axes score the same 6/15). Corpus `Ground Truth Axes` present (8 files) and read, `max_dev` rows in `gate368.tsv`.

**Cause (top[0..7] diagnostics, `work/diag/pota/`).** Pot A's two hard pieces are exactly the two whose correct axis is a *large-shift* basin, so a 6° cap cannot reach it:

- `P06` idx2: `shift_unbounded = 18.41°` -> `11.35°` (the value the ticket must preserve); bounded caps the shift at `6.00°` -> `27.23°` (cost 0.037 -> 0.071), so rank 0 stays idx0 at `19.90°` (shift `2.10°`, bounded cost 0.040).
- `P08` idx2: `shift_unbounded = 35.06°` -> `1.20°`; bounded `5.45°` -> `29.48°` (cost 0.031) while idx0 at `18.98°` needs only `0.30°` and costs 0.028, so it keeps rank 0.
- The shift gate fails identically by construction: it discards any `v_ref` that moved >6°, i.e. exactly the correct P06/P08 solutions, so `shiftgate6_d15` and `bounded6_d15` emit the same Pot A axes (identical numbers above).
- `P07`'s `68.38°` is a reference artifact, not our error: `t367`'s shipped axes have the same `68.38°` and differ from `axes323` by `0.00°`. `Dataset/Axes/Pot_A` is degenerate for P07.

**d5 trigger — not fired.** Best Juglet rank-0 gain d15 -> d5 is `0.83°` (bounded6, P8) and `0.00°` (shiftgate6); no piece reaches 2°, and Pot A's d5 deviation from `axes323` is `31.11°` (> 2°). `--array=8-11` was therefore not submitted (`d5_trigger` in `axes368_summary.json`, `array_8_11_triggered=false`).

## Part B — 8 assembly arms (best repeat shown; 2 repeats each)

| arm | cluster Kabsch (vertex) | origin gauge `score58` | worst chamfer | stock edges | P7 upright CAND336 |
|---|---|---|---|---|---|
| `juglet_bounded6_d15_r1` | 9/9, 0.48 mm | 9/9 at <= 4.596 mm | 13.604 tree = 4.53 mm | 10/18 | 141 CAND, 31 upright (min 2.20°) |
| `juglet_bounded6_d15_r2` | 9/9, 0.484 mm | 7/9 | 16.619 tree = 5.54 mm | 8/18 | - |
| `juglet_shiftgate6_d15_r2` | 9/9, 0.491 mm | 9/9 at <= 4.596 mm | 13.311 tree = 4.44 mm | 9/18 | 94 CAND, 23 upright |
| `juglet_shiftgate6_d15_r1` | 9/9, 0.461 mm | 5/9 | 18.082 tree = 6.03 mm | 8/18 | - |
| `pota_bounded6_d15_r1/r2` | 7/7 (P8 out), 1.305 mm | 0/8 | **75.38 mm** (P8) | 13/18 | (Pot A, not gated) |
| `pota_shiftgate6_d15_r1/r2` | 7/7 (P8 out), 1.305 mm | 0/8 | **75.38 mm** (P8) | 13/18 | (Pot A, not gated) |

So the Juglet leg of the ticket *is* met on the vertex gauge (9/9, 0.46-0.49 mm, same as shipped) and once on the origin gauge (`bounded6_d15_r1`: 9/9 at 4.53 mm vs shipped `6/9` at 5.56 mm; 1 of 2 repeats — `r2` is 7/9 at 5.54 mm), but it does not beat `t367` `a1_4dof_cin_r1` (9/9 at 4.060 mm, 12/18) and both guarded rules lose Pot A: P8 sits 75 mm from its home position, chamfers 6.89-16.85 mm, and 2 of 15 joins (P8's) are gone. Piece-7 audit unchanged in kind: 666 [SFS-336] CAND lines, 31 upright on P7, 5 upright CAND338 of which 1 on a GT edge.

**Visual inspection** of `C:/PR/artifacts/t368/render368_ablation.png` (2415x3045, 6 panels) before writing this Reading: panels 1-4 Juglet are one intact jug each, pieces 1-9 in place with green join bridges, panel 4 (`shipped_base`) visibly the loosest of the four; panels 5-6 Pot A are split into 2 clusters with P07 detached to the lower right and the body sherds fanned out, matching the 75 mm P8 residual — consistent with the numbers, not a rendering artifact.

Verification recomputed here, not copied: `sacct -X` rows above; `grep -c` of the 8 `COMPLETED 0:0`; seating from `score/<arm>.json` `tables.seating[].chamfer_mm` re-thresholded at 13.788 tree (Juglet) / 4.596 mm (Pot A); cluster rows from `score/<arm>_cluster.json`; `cluster366.py` md5 re-checked on Spartan; P06/P08 shift/cost pairs re-read from `work/diag/pota/Pot_A_Piece_06.json` and `_08.json`; Figure opened and inspected.

Net: the 6° direction guard is the wrong instrument for Pot A. Its two reference-critical axes are 18.4° and 35.1° away from their basin starts, so any cap or gate at 6° deletes the correct answer and the `c_ref` cost ordering then keeps the 19.9° / 19.0° false basins. Pot A lands at 7/7 and 13/18, so this rule cannot be promoted; the Juglet gain it buys is the same 9/9 vertex seating the shipped axes already have.

Reading: guarded-4dof-still-regresses-pota

**End state:** ready

### Lead verification (2026-11-19)

- `verified: C:/PR/artifacts/dsh-handover/368/accept.py -> ACCEPTANCE PASSED (exit 0); C:/PR/artifacts/t368/render368_ablation.png inspected visually (Panels 1-4 juglet_bounded6_d15_r1, juglet_shiftgate6_d15_r2, juglet_a1_4dof_cin_r1, juglet_shipped_base_r1 all close 9/9 with Base Piece 7 upright at the foot and 18/18 green bridges; Panels 5-6 pota_bounded6_d15_r2 and pota_shiftgate6_d15_r2 show 7/8 Pot A dome with 2 clusters). Note on worker line 152: "P07 detached" is a 0-based/1-based slip — pota_*_cluster.json has cluster_pieces=[1,2,3,4,5,6,7] and score58 per_piece has 1-indexed Piece 8 at 68.43 deg / 182.75 mm / 75.378 mm chamfer.`
- `verified: C:/PR/artifacts/t368/axes368_summary.json -> SignAxisGuard 0/18 on all 4 Juglet sets; Juglet rank-0 deg_signed (bounded6_d15)=[0.95, 15.73, 4.92, 8.73, 1.06, 3.13, 4.44, 3.80, 3.82] and (shiftgate6_d15)=[0.95, 15.73, 4.92, 7.40, 1.06, 2.58, 4.44, 3.80, 6.58]; top[0..7] explains why all 4 Pot A guarded sets drop to axes323_deg=[0.00, 1.50, 0.47, 0.00, 0.00, 31.11, 0.00, 19.49]: Pot_A_Piece_06 idx 2 starts 18.41 deg from axes323 (c_in=0.10383) and reaches 11.35 deg (0.00 deg vs axes323, 3.88 deg vs Ground Truth Axes, c_ref=0.03739) only via an 18.408 deg 4-DoF polish walk (capped at 6.0 deg -> 27.23 deg, c_ref=0.07062), and Pot_A_Piece_08 idx 2 starts 35.06 deg from axes323 (c_in=0.05181) and reaches 1.203 deg (0.00 deg vs axes323, c_ref=0.02756) only via a 35.056 deg 4-DoF polish walk (capped at 5.45 deg -> 29.485 deg, c_ref=0.03103). Because bounded6 caps the walk at 6 deg and shiftgate6 discards moves > 6 deg, both leave idx 0 (19.90 deg on P06, 18.98 deg on P08) at rank 0. d5_trigger=False (Juglet max gain 0.825 deg on P8; Pot A max dev vs axes323 31.107 deg).`
- `verified: C:/PR/artifacts/t368/score/*.json & C:/PR/artifacts/t368/gate368.tsv -> juglet_bounded6_d15_r1: 9/9 sherds, 10/18 stock edges (21/36 directed), 9/9 seated <= 4.596 mm at 4.535 mm origin gauge, 9/9 at 0.480 mm vertex cluster (r2: 8/18 edges, 7/9 at 5.540 mm origin gauge, 0.484 mm vertex); juglet_shiftgate6_d15_r2: 9/9 sherds, 9/18 edges (19/36 directed), 9/9 at 4.437 mm origin gauge, 0.491 mm vertex (r1: 8/9 sherds, 8/18 edges, 5/9 at 6.027 mm origin gauge, 0.461 mm vertex); pota_bounded6_d15_r1/r2 and pota_shiftgate6_d15_r1/r2 all stop at 7/8 sherds, 13/15 edges (26/36 directed), 0/8 seated origin gauge (75.378 / 75.366 / 75.379 / 75.376 mm on Piece 8), 7/7 vertex cluster [1..7] at 1.305 / 1.305 / 1.305 / 1.302 mm. Sacct 35393460 & 35393971_0..7 all COMPLETED 0:0 (35393947_0..7 initial UNBOUND RUN_DIR misfire noted).`
