# 365: Preserve `sfs_axes` `k=0` (`a1_k0` / `a2_pool`) and test 2-pass post-join `(S0, S1)` consensus (`b1_blind` / `b2_pair`) on `def_3x` Juglet

**What to build:** a **measurement and prototype ablation run** (zero edits to tracked `class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`; all prototype axis logic lives only in the run directory's copy of `src_axes` and driver scripts) evaluating **both** Option (a) (preserving `sfs_axes`'s `k=0` accurate candidates lost inside `profile.py` / `select.py`) and Option (b) (2-pass post-join pooled `(S0, S1)` axis consensus) on Ticket 80's clean `def_3x` lanes and Ticket 363's assembly rig:

1. **Part A — Option (a): Single-sherd `sfs_axes` ranking/refinement fixes (`axes_a1_k0`, `axes_a2_pool`, `pota_a1_k0`):**
   - `axes_a1_k0`: in a run-dir copy of `d272313`'s `src_axes` (`tree 6a67364b`), apply two zero-constant changes:
     1. In `refine_axis_profile(vt, walls, centre, size)` (`profile.py:59`), optimize **only the 2 direction-tilt DoFs (`x[0], x[1]`) holding `p0 = foot` fixed (`x[2] = x[3] = 0`)** so `profile_cost` cannot walk a good normal-refined axis away from the piece (`P4`: `2.73°/16.45 mm -> 7.40°/50.79 mm` in `t364`).
     2. In `rank_by_profile` (`select.py:55`), **preserve the pre-polish `cost_in` ordering** before dedup (do not let far-start candidates `k >= 1` jump above and delete `k=0` via `fin.sort` + `15.0°` `_distinct`), and dedup survivors at **`5.0°`** (instead of `15.0°`) before `keep_lines(tau=1.2, kmax=3)`.
   - `axes_a2_pool`: from the same `2-DoF`-polished 8 distinct candidates per piece (`5.0°` dedup), keep up to `kmax=4` candidates within `tau=1.35` so the assembler also sees sub-5° lines like `P2` `k=1` (`8.45°`), `P3` (`4.40°/4.92°`), `P5` (`0.69°/1.06°`), `P6` (`1.84°/2.58°`), `P8` (`2.74°`), `P9` (`6.41°/6.58°`).
   - Run Pot A through `axes_a1_k0` -> `pota_a1_k0` assembly (`8/8, 15/15` required positive control so `a1_k0` does not break corpus plates).

2. **Part B — Option (b): Post-join pooled `(S0, S1)` axis consensus (`axes_b1_blind`, `axes_b2_pair`):**
   - `axes_b1_blind` (100% blind 2-pass pipeline — **no Ground Truth**): take Pass 1's best blind assembly (`juglet_a1_k0_r1`); for each piece `i = 1..9`, select its best partner `j` from the final assembly (`[SFS-336] FINAL` cluster neighbor with the highest `[SFS-339] FATE keep accept` / shortest breakline distance, falling back to single-sherd `a1_k0` if piece `i` is an isolated singleton), transform partner `j`'s `t80` `def_3x` `(S0, S1)` walls (strided to `<= 80,000` points/wall to avoid `t334`'s full-density 4h timeout) into piece `i`'s local frame using Pass 1's estimated pose `T_i^-1 * T_j`, pool `(S0_pooled, S1_pooled)`, extract the pooled axis via `a1_k0` + `refine_axis_normalized` seeded from the higher-`s2/s3` member, write `axes_b1_blind/Juglet_Piece_N_Axis.xyz`, score against `Dataset/Juglet_large`, and run Pass-2 assembly (`juglet_b1_blind_r1`, `juglet_b1_blind_r2`).
   - `axes_b2_pair` (clean `(S0, S1)` 2-piece pooling ceiling with `+3° / +1 mm` pose noise): pool `t80`'s clean `def_3x` `(S0, S1)` walls (`<= 80,000` points/wall) across the 9 touching Juglet pairs (`1-8, 1-2, 3-5, 1-4, 2-5, 6-7, 7-9, 1-9, 2-9`) at both **exact** GT relative poses and **perturbed** (`+3°` rotation, `+1 mm` real = `+3.0` tree units translation) poses; score all 9 pairs `(deg, d_perp_mm)` for pooled `a1_k0` and conditioning-seeded `refine_axis_normalized` (`tree 6a67364b` + `a1_k0`). Map the perturbed (`+3° / +1 mm`) best pair axis for each piece (`1-8` -> P1,P8; `1-2` -> P2; `3-5` -> P3,P5; `1-4` -> P4; `6-7` -> P6,P7; `1-9` -> P9) into piece `i`'s frame as `axes_b2_pair/`, and run Pass-2 assembly (`juglet_b2_pair_r1`, `juglet_b2_pair_r2`).

**Answers:** S1
**Blocked by:** None (Tickets 363 and 364 resolved 2026-11-18).
**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-18)
**Branch:** 365-sfs-axes-k0-preservation-and-postjoin-loop, in /c/PR/worktrees/structure-from-sherds-pp/365
**Worker:** dsh one-shot subagent w365
**Budget:** medium 5
**Hand-over:** DeepSeek — Spartan compute node (read-only over `class/`, `sfs_axes/`, `Dataset/`, and existing `runs/`). Check path: the lead re-reads `C:/PR/artifacts/t365/axes365_summary.json` and `C:/PR/artifacts/t365/gate365.tsv`, verifies `sacct -X`, checks `pota_a1_k0` (`8/8, 15/15`), verifies `score/*.json` (and cluster-only Kabsch if stray pieces skew 9-piece Kabsch), and looks at `C:/PR/artifacts/t365/render365_ablation.png` before closing.

## Why this ticket exists (verified in Tickets 363–364)

- Ticket 364 proved **direction tilt (`1°–16°`) is the primary assembly breaker** (`2/9 -> 8/9` in one `2.09°–4.30°` cluster when GT direction is given, vs `2/9` total collapse when only GT pivot is given), and **pivot offset (`7–135 mm`) blocks tight seating (`7.4–29.1 mm` slack inside `{1..8}` and detached `P9`)**.
- `basins364.json` showed **two concrete paths** forward without inventing ad-hoc heuristics:
  - **Option (a):** `refine_axis_normalized` had already placed `k=0` accurately on **`P1 (1.44°), P4 (2.73°/16.45 mm), P5 (2.13°/17.31 mm), P6 (3.20°/15.19 mm)`**, yet `refine_axis_profile`'s 4-DoF Nelder-Mead walked `P4` out to `7.40°/50.79 mm`, `fin.sort` + `15°` dedup let far-start candidates overwrite `k=0` on `P3/P5/P6/P7`, and `15°` dedup stripped `P2`'s `8.45°` `k=1` candidate.
  - **Option (b):** Ticket 334 only tested 3× pair-pooling on `t331`'s unsegmented `cloud_mesh` (with fracture-face contamination and unstrided 4h timeout). Ticket 364 showed `P2` and `P9` have near-spherical normal-residual degeneracies individually, so clean `(S0, S1)` pair-pooling (`b1_blind` and `b2_pair`) must be measured on `t80`'s separated walls.

## Part A — Option (a) Single-sherd `a1_k0` and `a2_pool` axes + Pot A gate

1. Create run dir `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t365-opt-ab/` and copy `20261118-0247_4beb57f_t363-own-axes/src_axes` (`d272313` `tree 6a67364b`) into `<run>/work/src_axes_ab/`.
2. Write run-dir wrapper/patch module (no edits to tracked `SfSpp_preprocessing/sfs_axes/`):
   - **`a1_k0`:** `refine_axis_profile` holds `p0 = foot` fixed (`2-DoF` tilt only); `rank_by_profile` preserves `top`'s pre-polish `cost_in` order before `_distinct(..., 5.0°)` and `keep_lines(tau=1.2, kmax=3)`.
   - **`a2_pool`:** same `2-DoF` tilt polish + `cost_in` order + `_distinct(..., 5.0°)` + `keep_lines(tau=1.35, kmax=4)`.
3. On a Spartan compute node, run `a1_k0` and `a2_pool` on all 9 Juglet `t80` `def_3x` `(S0, S1)` walls (`/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0430_d272313_t80-default-on/out/cell_def_3x/geom/`) and run `a1_k0` on all 8 Pot A corpus `(S0, S1)` walls.
4. Score all three axis sets (`axes_a1_k0`, `axes_a2_pool`, `axes_pota_a1_k0`) vs GT (`Dataset/Juglet_large` and `/data/gpfs/projects/punim2657/Dataset/Axes/Pot_A/`): report `K`, `deg` (`rank0`, `bestK`), `d_perp_mm` (`tree / 3.0` for Juglet, `1x mm` for Pot A) and save into `axes365_summary.json`.

## Part B — Option (b) Clean `(S0, S1)` post-join pair consensus (`b1_blind` and `b2_pair`)

1. Run Pass-1 assemblies first (from Part C below): `juglet_a1_k0_r1`, `juglet_a1_k0_r2`, `juglet_a2_pool_r1`, `juglet_a2_pool_r2`, `pota_a1_k0`, `juglet_oracle_xy`. Wait for completion so Pass 1 poses exist for `b1_blind`.
2. **`axes_b1_blind` (blind 2-pass loop, zero GT):**
   - Parse `[SFS-336] FINAL` poses and `[SFS-339] FATE keep accept` / cluster edges from `juglet_a1_k0_r1`.
   - For each piece `i = 1..9`: if piece `i` belongs to a multi-piece cluster, select its best joined partner `j` (most inliers accepted / shared-cluster nearest seam in Pass 1). Transform `j`'s `t80` `(S0, S1)` walls (`<= 80,000` points/wall strided deterministically) into `i`'s frame via `T_i^-1 * T_j`, concatenate `(S0_i + S0_j, S1_i + S1_j)`, and run `a1_k0` (plus `refine_axis_normalized` seeded from higher-`s2/s3` piece, selecting lowest pooled `profile_cost` among seeded vs pooled top basin). If `i` is singleton, keep `a1_k0[i]`.
   - Score `axes_b1_blind` (`deg`, `d_perp_mm`) vs `Dataset/Juglet_large`.
3. **`axes_b2_pair` (clean `(S0, S1)` 2-piece ceiling, exact & `+3° / +1 mm` perturbed):**
   - For all 9 touching pairs (`1-8, 1-2, 3-5, 1-4, 2-5, 6-7, 7-9, 1-9, 2-9`), load `t80` `(S0, S1)` walls (`<= 80,000` points/wall), transform partner into anchor frame at exact GT pose AND at `+3°` rotation / `+1 mm` real (`+3.0` tree units) perturbation (`fixed deterministic seed`).
   - Run pooled `a1_k0` and conditioning-seeded `refine_axis_normalized` (`tree 6a67364b` + `2-DoF` polish). Record `(deg, d_perp_mm)` for every pair.
   - Using the **perturbed (`+3° / +1 mm`)** pooled axes, map each piece's designated pair axis (`1-8` -> P1,P8; `1-2` -> P2; `3-5` -> P3,P5; `1-4` -> P4; `6-7` -> P6,P7; `1-9` -> P9) into `axes_b2_pair/`, score vs `Dataset/Juglet_large`, and run Pass-2 assembly.

## Part C — `t363` assembly rig runs (exact container, shims, `t80` lanes)

- Reuse `20261118-0247_4beb57f_t363-own-axes` / `t364` container & shims (floor 10 `0c216463ae27fe4571b200cc2292da99` for Juglet; floor 50 `daa9d614cfa78de79584cd26933a2057` for Pot A), `XY_BREAK=1`, `XY_STATE=1`, `SFS_AXIS_LOCK_ICP` unset.
- **Wave 1 (array `--array=0-5`):**
  - `0: juglet_a1_k0_r1`
  - `1: juglet_a1_k0_r2`
  - `2: juglet_a2_pool_r1`
  - `3: juglet_a2_pool_r2`
  - `4: juglet_oracle_xy` (positive control, `9/9` bar; if `8/9` due to `P2` stochasticity allow 1 repeat as in `t364`)
  - `5: pota_a1_k0` (positive gate: `axes_pota_a1_k0` on Pot A must hit `8/8, 15/15` at `4.596 mm` bar)
- **Wave 2 (array `--array=0-3` after Part B produces `axes_b1_blind` & `axes_b2_pair`):**
  - `0: juglet_b1_blind_r1`
  - `1: juglet_b1_blind_r2`
  - `2: juglet_b2_pair_r1`
  - `3: juglet_b2_pair_r2`
- Score every arm with `score58` (`13.788` tree units = `4.596 mm` real for Juglet; `4.596 mm` for Pot A), record `CountResult`, cluster partitions, per-piece chamfers (`_tree` and `/ 3.0 = mm`), largest-cluster-only Kabsch error if any stray piece splits off, and Piece 7 upright `CAND338` counts.

## Part D — Deliverables and visual check

- `C:/PR/artifacts/t365/axes365_summary.json` — per-piece `(deg, d_perp_mm, K)` for `a1_k0`, `a2_pool`, `b1_blind`, `b2_pair`, `pota_a1_k0`, plus all 9 `b2_pair` pairs (exact & perturbed).
- `C:/PR/artifacts/t365/gate365.tsv` — exact 7-column header `group\tpiece\tmetric\tvalue\treference\tverdict\tnote` with explicit `_tree` vs `_mm` unit suffixes.
- `C:/PR/artifacts/t365/render365_ablation.png` — 6 labelled panels at `[SFS-336] FINAL`: best `juglet_a1_k0`, best `juglet_a2_pool`, best `juglet_b1_blind`, best `juglet_b2_pair`, `juglet_oracle_xy`, and `pota_a1_k0` (`PlaceErr` bridges drawn). Look at it via file viewer before writing `Reading:`.
- Append one `runs/index.tsv` row (verdict and `human_note` blank), record every Slurm job ID & `sacct -X` in `## Comments`, write bare `Reading: <slug> — ...` line, and end with `**End state:** ready`.

## Readings (pick one verbatim on a bare `Reading:` line)

- `single-sherd-a1-or-a2-seats-juglet` — `juglet_a1_k0` or `juglet_a2_pool` seats `>= 7/9` at `13.788` tree (`4.596 mm`) with `pota_a1_k0` holding `8/8, 15/15`.
- `blind-postjoin-loop-seats-juglet` — single-sherd arms stay `< 7/9`, while `juglet_b1_blind` (blind 2-pass loop) seats `>= 7/9` and `pota_a1_k0` holds `8/8, 15/15`.
- `paired-clean-walls-resolve-axes-and-seat` — `juglet_b2_pair` (`+3° / +1 mm` perturbed 2-piece pooled `(S0, S1)` axes) brings all 9 pieces within `<= 5°` and `<= 20 mm` `d_perp` and seats `>= 7/9`, proving clean `(S0, S1)` pair-pooling solves both tilt and pivot once pairs form, while naming which Pass-1 pairs failed in `b1_blind`.
- `neither-a-nor-b-seats-juglet` — all experimental arms stay `< 7/9`; exact axis `(deg, d_perp_mm)` and assembly deltas vs `t363` are recorded.
- `blocked: control-or-input-failed` — `pota_a1_k0` or `juglet_oracle_xy` fails positive control, or a named input is missing.

## Out of scope

- Editing tracked files under `structure-from-sherds-pp/class/`, `SfSpp_preprocessing/sfs_axes/`, `AxisExtraction/*.m`, or `Dataset/`.
- Committing, pushing, merging, or running `ticket_close.sh` from the worker.

## Acceptance

- [ ] Part A: `axes_a1_k0`, `axes_a2_pool`, and `pota_a1_k0` extracted on Spartan compute node from `tree 6a67364b`, scored against GT (`axes365_summary.json`).
- [ ] Part B: `axes_b1_blind` (from Pass-1 `juglet_a1_k0_r1`) and `axes_b2_pair` (all 9 clean `(S0, S1)` pairs at exact and `+3°/+1 mm` perturbed poses) extracted and scored on Spartan compute node.
- [ ] Part C: all 10 assembly runs (6 Wave 1 + 4 Wave 2) completed via `sbatch` with `sacct -X` recorded, scored with `score58`, plus largest-cluster alignment & Piece 7 `CAND338` counts.
- [ ] Part D: `C:/PR/artifacts/t365/axes365_summary.json`, `C:/PR/artifacts/t365/gate365.tsv`, `C:/PR/artifacts/t365/render365_ablation.png` (viewed before `Reading:`), `## Comments`, bare `Reading:`, and `**End state:** ready`.
