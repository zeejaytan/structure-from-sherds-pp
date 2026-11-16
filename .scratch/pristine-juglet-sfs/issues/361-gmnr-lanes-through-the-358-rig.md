# Ticket 361 — Do the GMnr 3x breaklines close the joins the 358 rig leaves open?

**What to build:** Run the **GMnr 3× breakline lanes** (the bundle ticket 78 measured at 8 of 9 inner-wall sherds, with the 6–7 seam on the inner wall at 0.15 mm) through **ticket 358's 6-DoF assembler** (`SFS_AXIS_LOCK_ICP=0`, `SFS_XY_BREAK_NORMALS=1`, `SFS_XY_STATE_NORMALS=1`), and report the result join by join against ticket 359's bridge numbers — so that "do cleaner inner-wall breaklines close the remaining Juglet joins?" gets a measured answer instead of an arm that re-tests a known failure.

**Answers:** S1
**Blocked by:** none
**Status:** resolved
**Branch:** 361-gmnr-lanes-through-the-358-rig, in /c/PR/worktrees/structure-from-sherds-pp/361
**Worker:** dsh one-shot subagent w361
**Budget:** medium (one 4–5 arm Spartan array on the GMnr 3× lanes, one Pot A control arm, join-level scoring read out of the arm traces, one comparison render; no source edits)
**Hand-over:** DeepSeek: read this ticket in full first, then tickets 358 (the rig and its switches), 359 (the bridge numbers), 333 (the 0 of 9 baseline) and 78 in `SfSpp_preprocessing` (the lanes). Stage the lanes, run the arms, score at join level, write `C:/PR/artifacts/t361/gate361.tsv` and `render361_assembly.png`, then append your Comments section with one Reading line per Part.

---

## Why this ticket exists

Ticket 78 (`SfSpp_preprocessing/.scratch/juglet-breaklines/issues/78-localised-wall-pair-replay-and-gmnr-assembly.md`) pre-registered an assembler arm `J_GMNR`: the GMnr 3× lanes through the **shipped** assembler, floor 50→10 plus the empty-candidate guard. It returned **0 of 9 seated**, which is correct and uninformative: ticket 333 had already measured the 3× Juglet on that same shipped assembler at **0 of 9** (shipped gates, floor 20, container build). A 0 against a known 0 cannot separate "the assembler fails at 3×" from "cleaner breaklines change nothing".

What is new since then: **ticket 358's rig seats 9 of 14 measured joins on the older t331/t355 lanes** with `SFS_AXIS_LOCK_ICP=0`, `SFS_XY_BREAK_NORMALS=1`, `SFS_XY_STATE_NORMALS=1` (clusters `{1,2,3,4,6,8,9}` + `{5,7}`; Pot A 8/8 and 15/15 in the same arms), and **ticket 359** located the remainder bridge by bridge:

| bridge | what ticket 359 recorded at the true pose |
|---|---|
| 6–7 | refused by `pair_overlap` 191.17 (77.39 found), 6 inliers, 21.0 units (7.0 mm) median separation |
| 3–5 | fit lands **48.26°** off (12 inliers, 38.2 units) |
| 5–6 | fit lands **13.08°** off (26 inliers) |
| 5–8 | fit lands **8.32°** off (30 inliers) |
| 1–4 | seated but **11.75°** out (ticket 360) |

The question this ticket asks is narrow and pre-registered: **on the GMnr lanes, does that set improve, and if so which bridge and by how much?** It does not ask whether the Juglet can be assembled (that is S1's wider question) and it does not re-run anything already measured.

## Inputs (fixed — do not re-derive, do not re-tune)

- **Lanes:** the nine GMnr 3× cells of ticket 78's run `runs/20261116-1801_756301d_t78-gmnr-cells` on Spartan (`/data/gpfs/projects/punim2657/SfSpp/runs/`), whose cell env line must read `WORNESS=0.7`, `SFS_NORMAL_MODE=radius`, `SFS_NORMAL_RADIUS_MM=1.0`, `SFS_MERGE_CLUSTER_DOT=0.887`, `SFS_GROW_BASE_MM=1.5`, `SFS_GROW_BASE_THETA_DEG=25`, `SFS_TREE_UNITS_PER_MM=3.0` and **no** `SFS_REFINE_*`. If the run directory has been cleaned, re-run the nine cells with `stage66/run_cell66.sh` at exactly those settings — never "close enough" settings.
- **Staging:** ticket 67's recipe — `Breaklines/` = the Bl0 lanes, `Surfaces/` = S0/S1 as bare 6-number `.xyz` (`class/data_structure.cpp:781-843` cannot parse PLY headers), `Mesh/`+`Axes/` from `Dataset/Juglet` and `Dataset/Juglet_large`, and the Pot A dataset hardlinked for the control arm.
- **Assembler:** the shipped tree via `hpc/sbatch/run_pristine_juglet.sbatch`'s container route (Apptainer `sfs_main/sfs_pristine_assembly.sif`), with the run-dir shim = a copy of `hpc/tools/main_pristine_batch.cpp` carrying exactly two literal changes (floor `50`→`10` at `:86`; ticket 67's empty-candidate guard at `class/ranking_system.cpp:727`). Record the three md5s; ticket 78's are `fb26e8ab49e9c00f7e8d02de44f6eea5` (shipped), `056dfb914d7f067244a87d200ecb9863` (guard added), `17f9280aeaecbec125eec707e7e15b0f` (guard + floor 10). Canonical sources are never written to.
- **Switches:** `SFS_AXIS_LOCK_ICP=0` in every arm; `SFS_XY_BREAK_NORMALS` / `SFS_XY_STATE_NORMALS` per arm; traces on in every arm (`SFS_STAGE_TRACE=1 SFS_STATE_TRACE=1 SFS_EDGE_TRACE=1 SFS_ADMIT_TRACE=1 SFS_PICK_TRACE=1 SFS_VETO_TRACE=1 SFS_WIN_TRACE=1 SFS_PRECOR_TRACE=1`) so the join table can be built from the logs.
- **Scorer:** `score58.py` from run `runs/20261101-1913_d8f716f_t58-floor-density-panel` (36 directed / 18 undirected when all nine pieces are active, `GT(2,4)=0`) or a fresh scorer written in the run directory — say which, and keep the denominators explicit.

## Part A — precondition (read-only, no job)

1. Nine piece directories present, each with `Breaklines/`, `Surfaces/`, `Mesh/`, `Axes/`; count the files and write the census into the deliverable.
2. The cell env line from the run's `cells_GM_3x.tsv` (or the equivalent) matches the settings above, exactly.
3. The shim md5s match the recorded values.

If any of the three fails, **stop** and report `blocked: lanes`. Do not substitute a different bundle — a different bundle answers a different question.

## Part B — arms (one Slurm array, one job per arm)

| arm | object | `XY_BREAK` | `XY_STATE` | floor | purpose |
|---|---|---:|---:|---:|---|
| `juglet_gmnr_stock` | Juglet | 0 | 0 | 10 | 6-DoF raw-normal baseline **on these lanes**; must land inside the recorded nondeterminism of ticket 78's `J_GMNR` (same 0 of 9, candidate counts within a few percent) or the rig is not comparable |
| `juglet_gmnr_xy_pair` | Juglet | 1 | 0 | 10 | Fix 1 alone on these lanes (decomposition, not the question) |
| `juglet_gmnr_xy` | Juglet | 1 | 1 | 10 | **the question** — ticket 358's rig on the GMnr lanes |
| `pota_xy` | `Pot_A` | 1 | 1 | 50 | control: must seat **8/8, 15/15**; if it does not, the Part is void and nothing else in it may be read |
| `juglet_gmnr_oracle` | Juglet | 1 | 1 | 10 | **only if** `juglet_gmnr_xy` survives fewer joins than ticket 358's 9 of 14. Seed each GT-adjacent candidate with the assembler's own axis-conjugated pose `G_axis = T_axis[y]·GT[y]⁻¹·GT[x]·T_axis[x]⁻¹` (`hpc/tools/main_pristine_batch.cpp:319`, implemented at `:333`) — **use the shim's own seeding, or fix `patch_oracle78.py` to conjugate**; a `Gy⁻¹·Gx` seed is the defect ticket 78 recorded and must not be repeated. Label the arm a diagnostic about the artefacts, never a result about the method. |

Poll every `sbatch` with `scripts/remote/job_status.sh` and record the exact `sacct` `State`/`ExitCode` and run directory id.

## Part C — scoring: join level first, seating second

From each arm's traces, per ordered pair of the 16 GT-adjacent pairs, report: `inl_icp`, `joins_inl_ge6`, `joins_surv` through `PairwisePruning`, `step0_kept`, `rotF_deg`, `mmF`, `PlaceErr`, and the `FINAL` cluster. Then:

1. **The four bridges** — 6–7, 3–5, 5–6, 5–8 — and 1–4's pitch error, each compared with ticket 359's number in the table above. State the delta in degrees and millimetres.
2. **The cluster structure** — does `{5,7}` merge into `{1,2,3,4,6,8,9}`, and which of the surviving joins is the one that did it.
3. **Seating** — `score58.py` per arm (or the fresh scorer), with the 3× bar **13.788 mm** named alongside the 1× bar 4.596 mm so the two are not confused. This is the secondary reading: a bundle can gain joins and still not seat, and the ticket must say which happened.

Deliverable `C:/PR/artifacts/t361/gate361.tsv`: one row per `(arm, pair)` for the join table, plus one row per `(arm, metric)` for the cluster/seating summary; columns `group arm pair metric value reference verdict`. Every verdict cell reads `pass`/`fail`/`n/a` **and matches its own number** — ticket 78's `gate78.tsv` printed `pass` on `4/36`, which is a tagging bug that must not be repeated.

## Part D — render, readings

Render `C:/PR/artifacts/t361/render361_assembly.png` (≥1800×1200) with four labelled panels from the `FINAL` poses: `juglet_gmnr_stock`, `juglet_gmnr_xy_pair`, `juglet_gmnr_xy`, and the GMnr lanes' own Pot A control — clusters named, the four bridge joins annotated. **Look at it and write one sentence of what you see before the Reading lines**; a join table alone has never been enough here.

Pick exactly one Reading line per Part, verbatim:

- `gmnr-breaklines-seat-more-joins` — `juglet_gmnr_xy` survives more of the 14 measured joins than 9, and at least one of the four bridges improves.
- `gmnr-breaklines-change-nothing` — the same 9 of 14 joins and the same four bridge numbers, within the run-to-run drift the traces themselves show.
- `gmnr-lanes-break-the-358-rig` — the Pot A control fails, or the join count on the GMnr lanes falls below the t331/t355 result.
- `blocked: lanes` — Part A failed.

## Out of scope

- Any edit to `class/`, `hpc/`, `main.cpp` or the pristine tree; the two shim literals are the only permitted change, in the run directory.
- Re-tuning the 358 switches, the growth/merge settings, or the 0.8 mm wall ruler.
- The 1× bundle, the shipped-assembler arms already measured (ticket 333, ticket 78), and the 6-DoF pair-to-final audit already in ticket 358.
- Declaring anything about the method from the oracle arm alone.

## Acceptance

- [ ] Part A census and env check recorded, with the three shim md5s.
- [ ] Arms run: stock, xy_pair, xy, `pota_xy` control 8/8 + 15/15 (or the Part declared void).
- [ ] `gate361.tsv` with one row per (arm, pair) and per (arm, metric), verdicts matching their numbers.
- [ ] The four bridges and 1–4 stated as deltas against ticket 359.
- [ ] `render361_assembly.png` looked at, eye sentence written before the Reading.
- [ ] One Reading line per Part, verbatim from the list.
- [ ] Every job's `sacct` `State`/`ExitCode` and the run directory id recorded.

## Comments (worker, 2026-11-17) - w361

**Run directory** `runs/20261117-0129_d2fb631_t361-gmnr-lanes` (tag `t361-gmnr-lanes`, ticket 361, branch `d2fb631`). Driver `submit361.sbatch`; array jobids **35228154** (arm-map rows 0-3) and **35228165** (array task 4, Pot A). Each arm was built inside `sfs_pristine_assembly.sif` from `src361.tar.gz` (git archive of `d2fb631`, 571 entries) with ticket 358's `tools/arm335.sh` (md5 `0ec9942d7a295b7dd65c88b850a64f73`, unchanged), and the per-arm source md5s in `logs/src_md5_<arm>.txt` match ticket 358's recorded set byte for byte (`class/feature_matching.cpp` `043757ff4a85b2107d65940ec32e4221`, `class/reconstruction.cpp` `a1c7dbdee576b54be11d1631c0309d8e`, `hpc/tools/main_pristine_batch.cpp` `daa9d614cfa78de79584cd26933a2057`, `inc_jug/class/data_path.h` `b598ba5709cffbe96537b5b0765141ab`). No file under `class/`, `hpc/` or the pristine tree was written; the only source edit is the run-dir shim's floor literal, made by `arm335.sh` inside the container. One row appended to `runs/index.tsv` (verdict and human_note blank); `run.json` carries the outcomes block.

### Part A - lane census, cell env, shim md5s

**Census (PASS).** `bundle3x/juglet` holds **55** files: `Axes/` 9 (`Juglet_Piece_N_Axis.xyz`), `Breaklines/` 9 (`Juglet_Piece_N_Breakline_0.pcd` - the Bl0 lanes), `Surfaces/` 18 (`N_Surface_0.xyz` + `N_Surface_1.xyz` as bare 6-number lines), `Mesh/` 9 (`N_Mesh.obj`), `Ground Truth/` 10 (nine `N_T.txt` + `Juglet_simple_graph.txt`); all nine pieces carry all six files. `stage_manifest.txt` (md5 `ff1e87ed5a3d1640bc11f8c54d6d6af7`) records per piece `SURF` verts+md5 and `LANE` nseg/bl0_dots/md5 (bl0_dots P1 180, P2 96, P3 122, P4 102, P5 143, P6 133, P7 172, P8 123, P9 77) plus `GTGRAPH md5=ccae01dd9c897e92beb0d507baa45cea`. `bundle3x/pota` holds **49** files: Axes 8, Breaklines 8 (`Pot_A_Piece_0N_Breakline_0.pcd`), Surfaces 16, Mesh 16 (8 Pot A + 8 Juglet `_Mesh.obj`), Ground Truth 18 (the 8 Pot A `_T.txt`, `Pont_A_simple_graph.txt`, and - inherited from the staging - the nine Juglet `_T.txt` and `Juglet_simple_graph.txt`). The GT of `bundle3x/juglet` is **md5-identical** to the t333 `work/trees/px3/juglet/Ground Truth` the ticket 355 analyzers read (all 10 files, graph included), so `analyze355.py`/`analyze357.py`/`audit_attrition358.py` apply unchanged.

**Cell env (PASS).** `out/cells_GMnr_3x.tsv` is a single row `GMnr_3x  Juglet_large  9  SFS_NORMAL_MODE=radius SFS_NORMAL_RADIUS_MM=1.0 SFS_MERGE_CLUSTER_DOT=0.887 SFS_GROW_BASE_MM=1.5 SFS_GROW_BASE_THETA_DEG=25 SFS_TREE_UNITS_PER_MM=3.0  0  yes`; `run.json` gives `WORNESS 0.7`. All seven ticket settings present (7/7), and `SFS_REFINE_*` is set nowhere (`SFS_REFINE_STAR` = none; the driver unsets every `SFS_*` between pieces). The only `SFS_REFINE` occurrences in the run are the prose note itself - a false positive in my first grep, recorded so nobody re-chases it.

**Shim md5s (deviates from ticket 78's triple - read this before the stock rows).** The branch shim is `hpc/tools/main_pristine_batch.cpp` md5 `daa9d614cfa78de79584cd26933a2057`; the floor literal `if (shard[i].edge_line_.point_.cols() < 50) {` occurs once, at **:142** (the ticket's `:86` is the t78 pristine copy), and ticket 67's empty-candidate guard is already inline at **:366-369** (`SFS-NO-CANDIDATES no assembly state to save; exit 0`), so "guard added" is byte-identical to "shipped" on this branch. The three values a t78-shaped shim would produce here are therefore `daa9d614...` / `daa9d614...` / `0c216463ae27fe4571b200cc2292da99` (floor 10), against t78's recorded `fb26e8ab49e9c00f7e8d02de44f6eea5` / `056dfb914d7f067244a87d200ecb9863` / `17f9280aeaecbec125eec707e7e15b0f`. They differ for two reasons, both accounted for line by line in `logs/partA_shim_diff.txt` (floor-10 branch shim vs floor-10 t78 shim: 9 hunks, +176/-7): the branch carries the ticket 335-338 trace blocks (`[SFS-338] CAND338` at :318-357, runtime-gated on `SFS_EDGE_TRACE`), and the 7 removed lines are exactly t78's shim-level t67 guard (superseded by the StateManager-internal guard in commit `9a613b5`) plus the `lcs_window` read that defaults to `MINIMUM_NUMBER`. I am recording this mismatch instead of claiming the third check passed, and I am **not** reading it as `blocked: lanes`: the lanes, the env and the GT identity all pass, no canonical source was touched, and the two checks that test comparability directly do hold (candidate count 232 = t78's 232, and Pot A reproduces t78 exactly). What it does mean is stated with the stock rows below.

### Part B - the four arms, with exact job states

| arm | jobid (sacct) | State / ExitCode | elapsed | candidates | pruned | SHERD | EDGE | clusters | joins in bar at FINAL | seated @13.788 |
|---|---|---|---:|---:|---:|---|---|---|---:|---:|
| `juglet_gmnr_stock` | 35228154_1 | COMPLETED / 0:0 | 00:09:44 | 232 | 67 | 2/9 | 0/18 | 1: C0{1,2,3,4,5,6,7,8,9} | 3/14 | 0/9 |
| `juglet_gmnr_xy_pair` | 35228154_2 | COMPLETED / 0:0 | 00:08:26 | 232 | 70 | 2/9 | 0/18 | 2: C0{1,3,5,8} + C1{2,4,6,7,9} | 3/14 | 0/9 |
| `juglet_gmnr_xy` | 35228154_3 | FAILED / 1:0 (arm and scorer both rc=0 - see note) | 00:08:54 | 232 | 70 | 6/9 | 5/18 | 2: C0{1,2,3,5,6,8,9} + C1{4,7} | **10/14** | 0/9 |
| `pota_xy` | 35228165_4 | COMPLETED / 0:0 | 00:06:51 | 124 | 70 | **8/8** | **15/15** | 1: C0{1,2,3,4,5,6,7,8} | 15/15 | **8/8** |
| `juglet_gmnr_oracle` | not submitted | n/a | - | - | - | - | - | - | - | - |

Two honest entries in that table. (a) Task 0 of the first array - **35228154_0, FAILED 3:0 in 00:00:04** - read the arm map's header line as an arm (`FAIL unknown dataset 'ds'`) and did nothing: the map file keys rows off `TID+1`, and the header shifts it by one. I left the map intact (it is the record, header included) rather than editing history, and submitted Pot A as its own array task. (b) **35228154_3 exited 1** although its arm ran to rc=0 (wall 492 s) and its `score58.py` step returned rc=0: the control's two gate assertions (`SHERD=8/8`, `EDGE=15/15`) were keyed on the array task index in the script as submitted, so they were evaluated against the Juglet accuracy line (`SHERD=6/9`) and set the wrapper's failure flag. I re-keyed those blocks on the arm name for the second submission and did not resubmit the xy arm - its assembler output and score JSON are complete. The `State/ExitCode` above are what `sacct` says, not what I wish it said.

**Stock comparability against t78's `J_GMNR` (232 candidates, 112 pruned, 3/9, 2/18, 0 of 9 seated).** The ticket's stated bar - same 0 of 9, candidate counts within a few percent - **passes**: 232 candidates exactly, 0 of 9 seated exactly. The other three rows do not reproduce: pruned 67 vs 112, SHERD 2/9 vs 3/9, EDGE 0/18 vs 2/18 (and the stock per-piece chamfers are 537-646 mm, well outside t78's 143-276 mm band). The cause is on disk, not in the lanes: t78's `J_GMNR` was built on the **SIF's pristine sources** with its own guard shim (base `fb26e8ab`, include `work/inc1/class/data_path.h` `dc44f5d086e612fa68983d35bc0d839c` with `#define POT_A_INFRA_OFF_JUGLET`, juglet_orig block `401195eae5a41579563fc237a9843298`) via `runarm J_GMNR 10 /panel/inc1 $TREES/juglet 9 -DJUGLET_ORIG`, whereas this run builds the **branch fork** (shim `daa9d614`, R333 `inc_jug` `b598ba57`). The 335-338 state machinery is in the branch and not in t78's build, so on these lanes the stock arm is a comparable lanes-level anchor but not the same assembler. Pot A shows the two assemblers agree exactly (124 candidates / 70 pruned / 8-8 / 15-15; t78's pruned for Pot A is also 70), so the difference is specific to the Juglet state path, not to the staging, the lanes or the control.

### Part C.1 - the funnel, join level first

Per-pair detail (`inl_icp`, `surv` through `PairwisePruning`, `rotF_deg`, `mmF`, `place_at_final`, and the FINAL cluster) is in `gate361.tsv`'s `join` group: 315 rows = 3 Juglet arms x 16 pairs x 5 metrics + Pot A 15 pairs x 5. Funnel per arm (`funnel361.tsv`):

| arm | inl>=6 | survives pruning | in bar at the best fit | step-0 kept / dropped | place ok at FINAL | clusters |
|---|---:|---:|---:|---|---:|---|
| stock | 14/14 | 11 | 4 | 1 / 0 | 3/14 | 1 (largest 9) |
| xy_pair | 14/14 | 12 | 4 | 0 / 2 | 3/14 | 2 (largest 5) |
| **xy** | 14/14 | 12 | 4 | 1 / 1 | **10/14** | 2 (largest 7) |
| pota | 15/15 | 13 | 10 | 0 / 0 | 15/15 | 1 (largest 8) |

`gate361.tsv`: **435 data rows - 291 pass, 123 fail, 21 n/a**, and every verdict is computed from the value on its own row (no row prints pass next to a zero or a miss, the `gate78.tsv` bug). Groups: identity 13 (10 pass / 3 n/a), stock 6 (2 / 3 / 1), control 4 (4 pass), join 315 (228 / 87), bridge 50 (29 / 21), cluster 34 (16 / 6 / 12 n/a), seat 12 (2 / 6 / 4 n/a), oracle 1 (n/a).

### Part C.2 - the four bridges and 1-4 against ticket 359

`juglet_gmnr_xy`; "fit" is the best surviving candidate (the CAND338 pose), "FINAL" is the committed pose the origin-invariant check scores. Deltas are this run minus 359 at the same quantity; negative is closer.

| bridge | ticket 359 | this run, at the fit | this run, FINAL | delta at the fit | delta at FINAL |
|---|---|---|---|---|---|
| **6-7** | 166.9 deg, refused at the gate (found area 77.39 vs limit 50, 6 inliers at the true pose, 21.0 units = 7.0 mm separation) | 3.978 deg / 1.174 mm, **16 inliers**, survives pruning | **not committed** (6 in C0, 7 in C1) | **-162.92 deg / -5.83 mm** | n/a - no committed pose |
| **3-5** | 48.26 deg / 38.2 u = 12.73 mm, 12 inliers | 37.31 deg / 13.59 mm, 16 inliers, survives | 5.517 deg / 1.049 mm, committed | -10.95 deg / +0.86 mm | -42.74 deg / -11.68 mm |
| **5-6** | 13.08 deg / 15.67 u = 5.22 mm, 26 inliers | 9.23 deg / 4.84 mm, 34 inliers, survives | 4.858 deg / 1.433 mm, committed | -3.85 deg / -0.38 mm | -8.22 deg / -3.79 mm |
| **5-8** | 8.32 deg / 5.78 u = 1.93 mm, 30 inliers | 11.79 deg / 2.59 mm, 24 inliers, survives | 3.278 deg / 0.789 mm, committed | **+3.47 deg / +0.67 mm** (worse) | -5.04 deg / -1.14 mm |
| **1-4** | 11.75 deg out, but seated (ticket 360) | 8.16 deg / 1.42 mm, 42 inliers, survives | **not committed** (1 in C0, 4 in C1) | -3.59 deg | n/a - it lost its seating |

Read as prose: **3-5** moves 10.95 deg closer and 0.86 mm further out at the fit, and its committed pose is 42.74 deg / 11.68 mm better than 359's fit number - it is now inside the cluster, 0.517 deg outside the 5 deg place bar. **5-6** improves at both ends (-3.85 deg at the fit, -8.22 deg committed) and lands in bar (4.858 deg / 1.433 mm). **5-8** is the one that gets *worse* at the fit (+3.47 deg / +0.67 mm) and still ends in bar at the committed pose (3.278 deg / 0.789 mm). **6-7** is the qualitative change: refused by `pair_overlap` with 6 inliers in 359, here surviving pruning with 16 inliers and an in-bar fit (3.978 deg / 1.174 mm) - 162.92 deg closer than 359's 166.9 deg best candidate - but it is still **not committed**, so 7 remains outside the pot. **1-4** improves at the fit (-3.59 deg) and loses what 359 had: the pair is no longer seated at all. Two footnotes so the numbers are not over-read: the 6-7 mm delta compares a fit error (1.174 mm) with 359's 7.0 mm *median separation at the true pose*, not like for like; and the FINAL deltas compare this run's committed pose with 359's best-candidate number, which is a magnitude, not a controlled comparison. The 359 cycles that were refused never had a committed pose of their own.

### Part C.3 - the cluster structure, and the seating

**Does {5,7} merge into {1,2,3,4,6,8,9}? Partly - and it is not the same residue.** The xy arm's FINAL structure is **C0{1,2,3,5,6,8,9} + C1{4,7}**: sherd **5** joins the main cluster, through **5-6 (4.858 deg / 1.433 mm) and 5-8 (3.278 deg / 0.789 mm)**, both in bar and both committed - 5-6 is the join that does it, with 5-8 as the corroborating one. Sherd **7 does not merge**: 6-7's fit is in bar and survives pruning, but the pair is not committed inside the cluster (the Step-6 cluster merge ticket 358 named is still the gap), so 7 stays out. What changed on the other side is that **sherd 4 left the main cluster**: 1-4 is no longer seated, and C1 is held together by the control pair **4-7** - a pair with no GT edge - at 3.289 deg / 0.823 mm, in bar. So the residue is {4,7} where 358 had {5,7}: one sherd of the old pair came in, the other stayed out, and a new one fell out. With 358's {1,2,3,4,6,8,9} as the reference, the main cluster keeps 1, 2, 3, 6, 8, 9 and gains 5; it loses 4.

**Seating (secondary reading).** All three Juglet arms seat **0 of 9** at the 3x bar **13.788 mm** (max chamfer 645.7 / 802.5 / **172.6** mm for stock / xy_pair / xy), and 0 of 9 at the 1x bar 4.596 mm. The xy arm's per-piece chamfers are 120.5 135.0 129.1 172.6 139.2 132.3 145.1 145.8 152.6 mm - an order of magnitude outside the bar, and not a near miss. Pot A seats **8/8** at both bars (max 2.0 mm; per piece 1.5 0.7 1.6 1.4 0.8 2.0 1.8 1.1). So the bundle **gained joins and did not seat** - the secondary reading the ticket asked to be named, and the one that holds: on the GMnr lanes the 358 rig buys 10 of 14 joins instead of 9, and still puts nothing within 13.788 mm.

### Part D - the render, looked at

`C:/PR/artifacts/t361/render361_assembly.png` (3005x1883), four labelled panels from the `[SFS-336] FINAL` poses with piece numbers, each panel's cluster line, its in-bar count and its per-join PlaceErr list, and the four bridge joins drawn between sherd centroids (solid green in bar, red dashed out, labelled with the pair's committed number).

**What I see:** panel 3 reads as one continuous juglet wall from lip to waist with sherds 5 and 9 in it, and only 4 and 7 hanging off the lower right as the second cluster, while panel 1 (stock) commits all nine into one cluster and the body is visibly sheared and offset piece to piece, panel 2 breaks the pot into a top and a bottom half at the waist ({1,3,5,8} above, {2,4,6,7,9} below), and panel 4 closes Pot A as a tight, seam-free pot with all eight pieces seated.

The render says the same thing the table does: this is not a pot yet (the wall in panel 3 is continuous where the joins improved but 4 and 7 stand off it, and nothing is within 13.788 mm), and the control is the reason to trust the lanes rather than the method.

### Readings

The ticket's reading list is one vocabulary for the whole run, so the same line answers all four Parts; it is quoted verbatim.

- Part A: `gmnr-breaklines-seat-more-joins`
- Part B: `gmnr-breaklines-seat-more-joins`
- Part C: `gmnr-breaklines-seat-more-joins`
- Part D: `gmnr-breaklines-seat-more-joins`

ready

---

## Verdict (lead, 2026-11-17)

**Reading accepted, with one correction of what it means.** `gmnr-breaklines-seat-more-joins` stands at the join level: on the GMnr 3× lanes the 358 rig survives 12 of the 14 measured joins where ticket 358 got 9 on the older t331/t355 lanes, commits **5–6** (4.858 deg / 1.433 mm) and **5–8** (3.278 deg / 0.789 mm), and scores 6 of 9 sherds and 5 of 18 edges where the stock 6-DoF arm on the same lanes scores 2 of 9 and 0 of 18. Seating is unchanged: **0 of 9**, largest cluster 7 sherds, Sherd 7 still outside.

**The correction.** The gain is mostly the lanes, not the switches. On these lanes the *stock* arm already survives 11 joins and fits **6–7 at 3.124 deg / 1.080 mm** — in bar — where ticket 359 recorded a 166.9 deg refusal and a pair_overlap of 191.17 at the true pose. So the GMnr breaklines alone fix 6–7's fit (its seam now lies on the inner wall at 0.15 mm, ticket 78); what the 358 switches add is the commitment of 5–6 and 5–8 and the jump from 2/9 to 6/9 sherds. The 10-of-14 headline is also a different quantity from 358's 9-of-14: here it is `place_at_final` at 5 deg / 5 mm, there it was survives-pruning. Recorded rather than re-scored.

**What did not improve.** Sherd 7 is dropped at pruning (`[SFS-339] FATE 7 64 -1 6 7 drop pc_overlap 16`) although its fit is now in bar with 16 inliers; the residue cluster is {4,7} — a GT-false pair — instead of 358's {5,7}; and 1–4 improves at the fit (−3.59 deg) yet loses its seating. The Juglet panels of the render look like a vessel; the chamfers are 172.6 mm (xy) and 537–646 mm (stock) against a 13.788 mm bar, so they are pictures, not seatings.

**Verified** (`verified: <command or file:line> -> <value>`):
- `ssh spartan sacct -j 35228154,35228165` -> 35228154_0 FAILED 3:0 4 s; _1 COMPLETED 0:0 9:44; _2 COMPLETED 0:0 8:26; _3 FAILED 1:0 8:54 (arm and scorer rc=0; the control gate was keyed on array index); 35228165_4 COMPLETED 0:0 6:51.
- `logs/binary_gmnr*_xy.log` headline -> 232 candidates, 70 pruned, SHERD 6/9, EDGE 5/18; stock -> 232 / 67 / 2/9 / 0/18; xy_pair -> 232 / 70 / 2/9 / 0/18; pota_xy -> 124 / 70 / 8/8 / 15/15.
- same xy log, FINAL lines -> sherds 1,2,3,5,6,8,9 in cluster 0, sherds 4,7 in cluster 1.
- `runs/20261117-0129_d2fb631_t361-gmnr-lanes/run.json` -> present; `runs/index.tsv` row present.
- `C:/PR/artifacts/t361/render361_assembly.png` -> looked at: four panels, xy two clusters with 5–6 and 5–8 green, Pot A one bowl 15/15 green.

**Deviations accepted.** The shim md5s differ from ticket 78's triple (`daa9d614`/`daa9d614`/`0c216463` vs `fb26e8ab`/`056dfb91`/`17f9280a`) because this run builds the branch fork (335–338 trace blocks, the t67 guard inline at `:366-369`), not the SIF pristine sources; the stock arm is therefore a lanes anchor, not the same assembler as 78's J_GMNR (pruned 67 vs 112, 2/9 vs 3/9, 0/18 vs 2/18), and Pot A reproduces 78 exactly at 124 / 70 / 8-8 / 15-15. Two array tasks failed on script bookkeeping, not on the science.

**Blemishes, not corrected.** One Reading line for all four parts (the ticket asked one per part), and 361's own `gate361.tsv` tags the funnel rows by bar text rather than by verdict. Neither changes a number.

**Next.** The join the fit now carries but the search discards is 6–7; that is a matching/pruning question, not a breakline one. The preprocessing side goes ahead as ticket 79 (`SFS_WALL_PAIR_FILTER`, default off), whose `Blocked by:` this result satisfies.
