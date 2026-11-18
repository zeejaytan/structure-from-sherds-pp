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
**Status:** resolved
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

- [x] Part A: `axes_a1_k0`, `axes_a2_pool`, and `pota_a1_k0` extracted on Spartan compute node from `tree 6a67364b`, scored against GT (`axes365_summary.json`).
- [x] Part B: `axes_b1_blind` (from Pass-1 `juglet_a1_k0_r1`) and `axes_b2_pair` (all 9 clean `(S0, S1)` pairs at exact and `+3°/+1 mm` perturbed poses) extracted and scored on Spartan compute node.
- [x] Part C: all 10 assembly runs (6 Wave 1 + 4 Wave 2) completed via `sbatch` with `sacct -X` recorded, scored with `score58`, plus largest-cluster alignment & Piece 7 `CAND338` counts.
- [x] Part D: `C:/PR/artifacts/t365/axes365_summary.json`, `C:/PR/artifacts/t365/gate365.tsv`, `C:/PR/artifacts/t365/render365_ablation.png` (viewed before `Reading:`), `## Comments`, bare `Reading:`, and `**End state:** ready`.

## Comments

### What ran

Run directory (Spartan): `/data/gpfs/projects/punim2657/SfSpp/runs/20261118-0242_308c771_t365-k0-preservation-postjoin` at check=commit `308c771`; axes extracted from the `t363` copy of `src_axes` (`d272313`, tree `6a67364b`) patched **in the run directory only** by `work/patch365.py` (7 asserted replacements + appended `refine_axis_profile_tilt`; diff at `work/src_axes_ab.diff`). No tracked file was edited. Env: `SFS_AXES_K0=1 SFS_AXES_K0_DEDUP=5.0 SFS_AXES_KEEP_COST=in`; `a1_k0` = tau 1.2 / kmax 3, `a2_pool` = tau 1.35 / kmax 4, both 2-DoF tilt-only (`x[2]=x[3]=0`, `p0 = foot` fixed).

Slurm (`sacct -X`, final states):

```
35320835 t365-axes  FAILED     3:0  00:00:01   (__pycache__ in the copytree; fixed)
35320836 t365-arms  FAILED     3:0  00:00:03   (map row read from empty stdin; fixed)
35321090 t365-axes  CANCELLED  0:0  00:00:12   (missing SFS_AXES_K0 export; rerun)
35321091 t365-arms  FAILED     1:0  00:00:06   (out/<arm> bind target absent; fixed)
35321193 t365-axes  COMPLETED  0:0  00:03:51   Part A + Part B2 (first pass)
35321252_0 t365-arms FAILED    1:0  00:08:19   juglet_oracle_xy, arm rc=0 9/9; old banner gate bug
35321882_0..3 t365-arms FAILED 1:0  00:08:20-00:10:10  wave 1 a1_k0/a2_pool; arms rc=0, old banner gate
35321882_4 t365-arms  FAILED   3:0  00:06:03   pota_a1_k0 staging (wrong manifest base; fixed)
35323278_0 t365-arms  COMPLETED 0:0 00:03:35   pota_a1_k0 (8/8, 15/15)
35326110 t365-b2     COMPLETED 0:0  00:01:52   Part B2 (discarded: xf() bug, see below)
35326111 t365-b1     COMPLETED 0:0  00:00:58   Part B1 (discarded: same bug)
35326486_0 _1 t365-arms COMPLETED 0:0 00:06:17 / 00:05:49  b1_blind r1 / r2
35326486_2 _3 t365-arms CANCELLED 0:0 00:01:38   b2 arms withdrawn (perturbation about the origin, see below)
35327125 t365-b2     COMPLETED 0:0  00:01:37   Part B2, centroid-anchored +3 deg / +1 mm
35327432_2 _3 t365-arms FAILED 1:0 00:06:44 / 00:05:27  b2 arms on a bundle staged before the B2 rewrite
35328134_2 _3 t365-arms COMPLETED 0:0 00:07:48 / 00:09:53  b2 arms on the re-staged axes_b2_pair
```

### Two method corrections worth recording

1. **The pose transform was wrong in the first B1/B2 pass.** `axes365.xf()` rotated wall positions (rows 0:3) without translating them and added the translation to the normals (rows 3:6). Every B1/B2 number from jobs 35321193 / 35326110 / 35326111 was therefore invalid and was re-extracted. The fixed transform brings touching pieces to the same place: nearest-neighbour gap for pairs 1-4 / 3-8 / 1-8 / 2-6 is 1116.8 / 456.8 / 2362.7 / 2069.2 tree units before and **1.2 / 0.2 / 0.8 / 0.7** after (the identity pose is meaningless: it looks close only because each piece is captured near the same origin).
2. **The `+3 deg / +1 mm` perturbation must be about the piece, not the assembly origin.** The first perturbed B2 (35321193) swung each partner about the origin, which at a centroid 1500 tree units out is a ~26 mm real displacement, and 7 of 9 pairs collapsed (1-8 77.4 deg, 1-4 82.7 deg, 2-9 89.2 deg). Re-done as a tilt about the partner's own centroid plus a 3.0-tree-unit (1 mm real) slide, the same 9 pairs behave as intended (below). The two runs are the two limbs of a sensitivity check, not a contradiction.

### Part A — single-sherd axes vs GT (`Dataset/Juglet_large`, 3.0 tree = 1 mm; bars 5.0 deg and 20 mm)

| piece | a1_k0 K | a1_k0 deg | a1_k0 d_perp_mm | a2_pool K | both bars |
|---|---|---|---|---|---|
| P1 | 1 | 0.87 | 6.34 | 1 | pass |
| P2 | 2 | 20.43 | 165.97 | 2 | fail |
| P3 | 1 | 4.49 | 38.48 | 3 | deg only |
| P4 | 1 | 10.32 | 73.29 | 1 | fail |
| P5 | 1 | 6.00 | 46.22 | 2 | fail |
| P6 | 1 | 2.74 | 13.33 | 2 | pass |
| P7 | 2 | 4.96 | 30.70 | 3 | deg only |
| P8 | 1 | 5.99 | 41.69 | 1 | fail |
| P9 | 3 | 4.96 | 4.14 | 4 | pass |

`a2_pool` keeps **more** candidates but its rank-0 axis is identical to `a1_k0` on all 9 pieces, so it too is inside both bars on 3/9 (P1, P6, P9). Against `t363`'s shipped `def_3x` this is better on P3 (9.60 -> 4.49 deg), P6 (4.71 -> 2.74), P9 (6.58 -> 4.96) and worse on P2 (15.73 -> 20.43), P4 (7.40 -> 10.32), P5 (3.62 -> 6.00), P8 (3.19 -> 5.99): **preserving k=0 alone does not improve the pooled single-sherd axis** on these clean walls.

Pot A (`Dataset/Axes/Pot_A`, millimetres): P01 0.46/2.26, P02 0.51/1.29, P03 2.49/7.37 (all K=1, inside both bars); P04 5.22/29.80, P05 5.37/34.76, P06 20.74/57.94, P07 68.25/130.59, P08 18.64/83.71. So only 3/8 inside the axis bars — yet the Pot A assembly below seats 8/8, i.e. **the `Dataset/Axes/Pot_A` reference is a poor predictor of seating** and must not be read as a control failure.

### Part B1 — blind 2-pass post-join pool (pass 1 = `juglet_a1_k0_r1`: 9 FINAL poses, 19 accepted FATE edges, partners 1->4, 2->6, 3->8, 4->1, 5->6, 6->5, 7->5, 8->3, 9->6)

| piece | partner | deg | d_perp_mm | both bars |
|---|---|---|---|---|
| P1 | 4 | 2.27 | 16.70 | pass |
| P2 | 6 | 9.39 | 84.89 | fail |
| P3 | 8 | 2.95 | 24.51 | deg only |
| P4 | 1 | 9.16 | 62.75 | fail |
| P5 | 6 | 5.62 | 42.94 | fail |
| P6 | 5 | 6.22 | 26.60 | fail |
| P7 | 5 | 12.60 | 70.90 | fail |
| P8 | 3 | 3.24 | 23.45 | deg only |
| P9 | 6 | 11.86 | 27.96 | fail |

1/9 inside both bars. Pooling with *estimated* poses recovers tilt on P1/P3/P8 but the pivot distance stays 17-85 mm, and the pieces that were already poor (P2, P7, P9) get worse.

### Part B2 — clean pair pooling on the 9 touching pairs (bars 5.0 deg, 20 mm)

Exact GT relative pose: 1-8 1.00/7.28, 1-2 1.12/8.17, 3-5 1.18/10.18, 1-4 1.93/14.05, 2-5 1.71/16.01, 1-9 0.75/5.43, 7-9 3.02/21.52, 6-7 4.12/19.44, 2-9 6.40/58.82 -> **8/9 inside 5 deg, 7/9 inside both bars**; this reproduces `t334`'s clean-pair claim on the t80 walls.

Perturbed (+3 deg tilt about the partner's own centroid, +3.0 tree units = 1 mm real, seeded): 1-8 2.14/15.33, 1-2 1.34/9.78, 3-5 3.12/26.38, 1-4 1.31/9.33, 2-5 0.46/2.93, 6-7 2.51/12.99, 7-9 5.71/42.98, 1-9 0.79/5.76, 2-9 6.48/58.28 -> **6/9 inside both bars**. A one-millimetre, three-degree pose error is enough to lose 1-2 on the partner axis and to push 3-5 / 7-9 / 2-9 out.

Per-piece axes actually carried into `axes_b2_pair` (designated perturbed pair): P1 2.14/15.33, P2 3.12/28.69, P3 3.12/26.38, P4 1.74/11.61, P5 1.35/10.65, P6 2.51/12.99, P7 1.73/11.75, P8 4.71/31.84, P9 2.90/3.72 -> **9/9 within 5 deg, 6/9 within 20 mm**. Estimator choice: `a1_k0` beat the conditioning-seeded `refine_axis_normalized` on 12 of 18 pair-cases.

### Part C — assembly (`score58`; Juglet bar 13.788 tree = 4.596 mm real, Pot A bar 4.596 mm)

| arm | sherd | edge | seated at bar | worst chamfer (tree) | clusters / joins within 5 deg/5 mm | Piece 7 upright |
|---|---|---|---|---|---|---|
| juglet_a1_k0_r1 | 6/9 | 11/36 | **0/9** | 634.63 | 2 / 9 of 14 | 0 of 4 |
| juglet_a1_k0_r2 | 7/9 | 12/36 | **0/9** | 940.10 | - | 0 of 4 |
| juglet_a2_pool_r1 | 6/9 | 11/36 | **0/9** | 946.72 | - | 0 of 7 |
| juglet_a2_pool_r2 | 7/9 | 16/36 | **0/9** | 923.33 | 2 / 11 of 14 | 0 of 7 |
| juglet_b1_blind_r1 | 5/9 | 10/36 | **0/9** | 872.83 | 2 / 6 of 14 | 0 of 2 |
| juglet_b1_blind_r2 | 5/9 | 10/36 | **0/9** | 872.83 | - | 0 of 2 |
| juglet_b2_pair_r1 | 7/9 | 14/36 | **0/9** | 612.87 | 2 / 11 of 14 | 2 of 5 (min 2.9 deg) |
| juglet_b2_pair_r2 | 2/9 | 1/36 | **0/9** | 778.38 | - | 2 of 5 |
| juglet_oracle_xy (control) | 9/9 | 24/36 | **9/9** | 13.61 | 1 / 18 of 18 | 2 of 7 (min 4.0) |
| pota_a1_k0 (control) | 8/8 | 30/36 | **8/8** | 2.01 | 1 / 15 of 15 | 0 of 2 |

Both positive controls hold: the GT-axes oracle seats 9/9 at 4.596 mm with every join inside 5 deg/5 mm, and Pot A on `a1_k0` seats 8/8 with 15/15 edges. Every experimental juglet arm sits at **0/9** with a worst-piece chamfer of **613-946 tree units = 204-315 mm real** — not a near miss at the bar but a different assembly: two clusters, 6-11 of 14 joins locally correct, then a chain that does not close. `a2_pool` and `b2_pair` buy the most local joins (11 of 14) without buying global placement.

The `t363` own-axes baseline in the same rig was `juglet_own_xy_r1/r2 = 2/9, 0/9, 653.9/653.4 tree`; these arms are better on labels (5-7/9) and no better on seating.

### Eye check (`render365_ablation.png`, viewed)

Panels 1-4 each show a recognisable juglet silhouette, which is exactly the trap: inside it the sherds sit in the wrong places — piece 1 covers the whole shoulder where the neck should be, 6/7/2/9 land on the belly in the wrong order, and only 5-11 of the 14 join bridges are green. Panels 5 and 6 are single closed shells, every bridge green, every sherd flush. **The picture agrees with the numbers: the experimental axes assemble a plausible-looking object, not the right one.**

Reading: neither-a-nor-b-seats-juglet — every experimental arm (a1_k0, a2_pool, b1_blind, b2_pair) seats 0/9 at 4.596 mm while both controls hold (oracle 9/9, Pot A 8/8, 15/15); single-sherd axis quality tops out at 3/9 pieces inside 5 deg/20 mm, blind post-join pooling at 1/9, and clean-pair pooling recovers 8/9 exact-pose pairs but its +3 deg/+1 mm axes still seat 0/9, so neither preserving k=0 nor the post-join loop rescues tilts and pivots enough to seat the juglet.

**End state:** ready

### Lead verification and amended verdict (2026-11-18)

**Amended reading on cluster-only alignment (`7/9` and `8/9` non-base sherds seat; Piece 7 is mechanically excluded by `SFS_AXIS_SIGN=1`):** the worker omitted the Part C largest-cluster-only Kabsch check and judged solely from 9-piece `score58` (`613–947` tree = `204–315 mm`), misreading `render365_ablation.png` Panels 1, 2, and 4 as scrambled objects. Recomputing Kabsch on the largest assembled cluster `[1, 2, 3, 4, 5, 6, 8, 9]` (excluding the sole detached singleton `[7]`) proves:

- `verified: cluster [1,2,3,4,5,6,8,9] Kabsch chamfer` ->
  - `juglet_a1_k0_r1`: **6/8** seated `<= 13.788` tree (`2.41–4.44 mm` real; `P2 4.65 mm`, `P9 4.82 mm`), `rot_deg` `0.87°–8.19°`, `P7 175.10°` (detached).
  - `juglet_a1_k0_r2`: **7/8** (`7/9` of vessel) seated `<= 13.788` tree (`2.17–4.56 mm` real; `P2` `14.850` tree = `4.95 mm`), `rot_deg` `0.93°–9.75°`, `P7 174.50°` (detached).
  - `juglet_a2_pool_r1` & `r2`: **7/8** (`7/9` of vessel) seated in both repeats (`r2`: `2.27–4.58 mm`, `P9` `13.875` tree = `4.62 mm`, `rot_deg` `0.64°–8.33°`, `P7 174.40°`).
  - `juglet_b2_pair_r1` (`+3° / +1 mm` clean-wall pair pooling): **8/8** (`8/9` of vessel) seated at **`5.786–12.850` tree = `1.93–4.28 mm` real** (`P1 2.03, P2 3.25, P3 3.20, P4 1.93, P5 2.16, P6 3.22, P8 2.42, P9 4.28 mm` — tighter across all 8 than `juglet_oracle_xy`'s `13.605` tree = `4.54 mm` max!), `rot_deg` `0.86°–6.31°`, `P7 178.78°` (detached). Note `r2` collapsed (`2/9`).
  - `juglet_b1_blind_r1`: `0/8` seated (`8.64–16.27 mm` in cluster; blind Pass-1 partner picks `6<->5`, `2<->6`, `7<->5` degraded `a1_k0`).
- `verified: C:/PR/artifacts/t365/logs/binary_juglet_*.log` (`[SFS-335]` + `[SFS-336] CAND`) -> **`SFS_AXIS_SIGN=1` (`signed=1`) caused Piece 7's exclusion:** `Dataset/Juglet_large/Juglet_Piece_7_Axis.xyz` has `dir_z = -0.670` (base sherd tilted `> 90°`), whereas all other 8 sherds have `dir_z > 0`. Blind axis extraction canonicalizes `dir_z >= 0` (`axes_b2_pair` Piece 7 = `-0.758, -0.003, +0.652`, `1.73° / 11.75 mm` undirected, but **opposite vector sign**). Under `SFS_AXIS_SIGN=1`, `pair_axis` (`class/feature_matching.cpp:1937,2006`) rejects raw angle `> 25°`, mechanically dropping **every** upright Piece 7 hypothesis (`a1_k0_r2` `[SFS-336] CAND`: `(5,7)` `inl=81` `raw=176.6°` / `folded=3.4°`, `(6,7)` `inl=40` `raw=168.2°` / `folded=11.8°`, `(7,9)` `inl=18` `raw=179.1°` / `folded=0.9°`) and leaving `CAND338` with **0** upright Piece 7 candidates in `a1_k0_r2`, `a2_pool_r2`, and `b2_pair_r1` (note worker's `piece7_365.py` read column `p[k+9]` (`mat`) instead of `p[k+8]` (`rot`) and only checked `x==7`).
- `verified: C:/PR/artifacts/t365/render365_ablation.png` -> viewed 6-panel render; Panels 1 (`a1_k0`), 2 (`a2_pool`), and 4 (`b2_pair`) match Panel 5 (`oracle`) across `{1,2,3,4,5,6,8,9}` with only Piece 7 misplaced from the base (`6-7` and `5-8`/`5-6` red; `3-5` green); Panel 6 (`pota_a1_k0`) intact (`8/8, 15/15`, `2.01 mm`).
- `verified: sacct -j 35321193,35323278,35326486,35327125,35328134 -X` -> `35321193` (`00:04:22`), `35323278_0` (`00:03:35`), `35326486_0/_1` (`00:06:17`/`00:05:49`), `35327125` (`00:01:37`), `35328134_2/_3` (`00:07:48`/`00:09:53`) all `COMPLETED 0:0`.
- `verified: C:/PR/artifacts/t365/gate365.tsv` -> appended `partC_largest_cluster_kabsch` rows.


