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
**Status:** ready-for-agent
**Worker:** unassigned
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

- [ ] Part A: `<run>/work/` builds all 8 axis sets (`bounded6_d15`, `shiftgate6_d15`, `bounded6_d5`, `shiftgate6_d5` for Juglet and Pot A) cleanly from one captured pool without touching tracked files; `axes368_summary.json` records per-candidate `unsigned_deg`, `signed_deg`, `d_perp_origin_mm`, `d_perp_sherd_mm`, `is_base_fragment`, `SignAxisGuard` = `0/18` across all Juglet sets, Pot A against corpus/`axes323`/`Ground Truth Axes`, and `top[0..7]` diagnostics.
- [ ] Part B: all 8 core assembly runs (`--array=0-7`, plus `8-11` if `d5` triggers) complete via `sbatch` with `sacct -X` showing `COMPLETED 0:0`; scored with `score58` origin Kabsch, fixed `cluster366` vertex Kabsch (`md5 81f347cc...`), stock & centroid-gauge `CountResult`, `CANDTOTAL`, `SIGN-AXIS`, and Piece 7 upright audit.
- [ ] Part C: `C:/PR/artifacts/t368/axes368_summary.json`, `C:/PR/artifacts/t368/gate368.tsv`, `C:/PR/artifacts/t368/render368_ablation.png` (visually inspected), `runs/index.tsv` row appended, `## Comments` complete, bare `Reading:` line, `**End state:** ready` (or `blocked`).

## Comments

*(Worker records run dir, Slurm IDs / `sacct -X`, Part A/B tables, `top[0..7]` audit, visual inspection of `render368_ablation.png`, and `Reading:` here.)*
