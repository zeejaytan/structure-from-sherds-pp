# Ticket 361 — Do the GMnr 3x breaklines close the joins the 358 rig leaves open?

**What to build:** Run the **GMnr 3× breakline lanes** (the bundle ticket 78 measured at 8 of 9 inner-wall sherds, with the 6–7 seam on the inner wall at 0.15 mm) through **ticket 358's 6-DoF assembler** (`SFS_AXIS_LOCK_ICP=0`, `SFS_XY_BREAK_NORMALS=1`, `SFS_XY_STATE_NORMALS=1`), and report the result join by join against ticket 359's bridge numbers — so that "do cleaner inner-wall breaklines close the remaining Juglet joins?" gets a measured answer instead of an arm that re-tests a known failure.

**Answers:** S1
**Blocked by:** none
**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-17)
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
