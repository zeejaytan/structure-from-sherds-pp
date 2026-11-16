# Ticket 362 — Assembly on the default-on edgeline: does the search keep the 6–7 pair, and if it drops it, why

**What to build:** Run the **new** edgeline — ticket 79's wall-pair rule, on by default after ticket 80 — through the assembler rig that measured the GMnr lanes (ticket 361), and answer two questions in one run: does the fixed line change the join set, and **does the 6–7 pair that now fits at 3.98° / 1.17 mm still get dropped at pruning?** If it still drops, diagnose from the arm's own trace which stage killed it and whether the fit or the test is wrong. No code change.

**Answers:** S1
**Blocked by:** ticket 80 (`SfSpp_preprocessing/.scratch/juglet-breaklines/issues/80-default-on-and-the-edgeline-for-the-eye.md`) — it must be landed, and **the conservator's eye check in visual-qa must have passed** (recorded in that ticket's Comments). Do not start before both.
**Status:** ready-for-agent
**Worker:** unassigned
**Budget:** medium (one lane bundle, three arms, one control, one gate table, one render; the diagnosis is read-only)
**Hand-over:** DeepSeek: read ticket 361 in full (the rig, the arms, the bridge numbers, the funnel) and tickets 358/359 for what the switches do; read ticket 80's record for the lanes. Stage the nine default-on 3× lanes, run the arms, score joins before seating, and read the 6–7 fate out of the traces. Look at the render before the Reading line. Do not change any pruning code.

---

## Why this ticket exists

Ticket 361 put the GMnr 3× lanes through ticket 358's rig and got a result that split cleanly in two: **the breaklines fixed the 6–7 fit** (3.978°, 1.174 mm, 16 inliers, after ticket 359 had it refused at 166.9°) and the rig raised sherd accuracy from 2 of 9 to **6 of 9** and edges from 0 of 18 to **5 of 18** — while **0 of 9 pieces seated**, chamfers 172.6 mm against a 13.788 mm bar, and the 6–7 pair was **dropped by `PairwisePruning`** (`[SFS-339] FATE 7 64 -1 6 7 drop pc_overlap 16`), leaving `C1{4,7}` held by a pair the ground truth says is **not adjacent**.

Since then the edgeline itself has changed once: ticket 79's wall-pair filter removes the flap that hung off Sherd 7's lip (34 off-lip dots → 2, patch 15.601 mm → 0.599 mm) and **re-traces** the boundary. That is a different input from the GMnr lanes 361 used. Whether the search keeps 6–7 when the line it was measured against is finally on the wall is a question only an assembler run answers.

## Part A — lanes (precondition, read-only)

1. The nine 3× cells of ticket 80's default-on run (`def_3x`), env line exactly `WORNESS=0.7`, `SFS_NORMAL_MODE=radius`, `SFS_NORMAL_RADIUS_MM=1.0`, `SFS_MERGE_CLUSTER_DOT=0.887`, `SFS_GROW_BASE_MM=1.5`, `SFS_GROW_BASE_THETA_DEG=25`, `SFS_TREE_UNITS_PER_MM=3.0`, **no** `SFS_REFINE_*`, and the switch **unset** rather than set to `pair`. If the run directory has been cleaned, re-run the nine cells with `stage66/run_cell66.sh` at exactly those settings — never "close enough".
2. Bl0 dot counts per piece match ticket 80's record, and Sherd 7's Bl0 reproduces the ≤ 3-dot / ≤ 0.599 mm reading. If it does not, `blocked: lanes`.
3. Staging is ticket 67's recipe: `Breaklines/` = the Bl0 lanes, `Surfaces/` = S0/S1 as bare 6-number `.xyz`, `Mesh/` + `Axes/` from `Dataset/Juglet` and `Dataset/Juglet_large`, Pot A hardlinked for the control. Census the file counts into the deliverable.
4. Assembler built in the container from a git archive of `main` at dispatch time, with ticket 358's `tools/arm335.sh`; the run-dir shim carries exactly two literal changes (floor `50`→`10` at `:142` on this branch; ticket 67's guard is already inline at `:366-369`). Record the three md5s — ticket 361 recorded `daa9d614cfa78de79584cd26933a2057` / same / `0c216463ae27fe4571b200cc2292da99`, and a different value means a different source, which must be explained before any arm is read.

## Part B — arms (one Slurm array; reuse ticket 361's driver from its run directory, copy it in and record its md5)

| arm | object | `XY_BREAK` | `XY_STATE` | floor | purpose |
|---|---|---:|---:|---:|---|
| `juglet_def_stock` | Juglet | 0 | 0 | 10 | the 6-DoF raw-normal baseline on the new lanes — context, not the question |
| `juglet_def_xy` | Juglet | 1 | 1 | 10 | **the question** — the 358 rig on the fixed edgeline |
| `pota_def_xy` | `Pot_A` | 1 | 1 | 50 | control: must seat **8/8, 15/15**; if it does not, the Part is void |
| `juglet_def_oracle` | Juglet | 1 | 1 | 10 | **only if** `juglet_def_xy` survives fewer joins than 361's 12 of 14, or 6–7 disappears before pruning. Seed with the assembler's own axis-conjugated pose `G_axis = T_axis[y]·GT[y]⁻¹·GT[x]·T_axis[x]⁻¹` (`hpc/tools/main_pristine_batch.cpp:319`, `:333`) — the `Gy⁻¹·Gx` seed is ticket 78's recorded defect. Label it a diagnostic about the artefacts, never a result about the method. |

Poll every `sbatch` with `scripts/remote/job_status.sh`; record the exact `sacct` `State`/`ExitCode` and the run directory id in this ticket. One `runs/index.tsv` row, verdict and human_note blank.

## Part C — scoring: joins first, seating second, and 6–7 named explicitly

Per ordered pair of the 16 GT-adjacent pairs: `inl_icp`, `joins_inl_ge6`, `joins_surv` through `PairwisePruning`, `step0_kept`, `rotF_deg`, `mmF`, `PlaceErr`, `FINAL` cluster. Then:

1. **The 6–7 pair, in detail** — its fit error, inliers, whether it reached the ranking (`[SFS-339] OFFER`), whether it survived pruning, and the **exact `FATE` line** if it was dropped, verbatim. This is the headline of the ticket: a pair that now fits at 3.98° / 1.17 mm either is or is not kept.
2. **Joins against ticket 361's benchmark** — sherds `6 of 9`, edges `5 of 18`, the four bridges (3–5 37.31°, 5–6 9.23°, 5–8 11.79°, 1–4 11.75°), and the funnel (`place_at_final` 10 of 14). Say whether the fixed line moved any of them and by how much.
3. **Clusters** — does `C1{4,7}` survive, and is the false 4–7 pair still holding it?
4. **Seating** — `score58.py` or a fresh scorer, 3× bar **13.788 mm**, and report the best chamfer; 361's max was 172.6 mm. Seating is the secondary reading and may legitimately stay 0.

Deliverable `C:/PR/artifacts/t362/gate362.tsv`: columns `group arm pair metric value reference verdict note`; every verdict cell must **match its own number** (ticket 78's `pass` on `4/36` was a tagging bug).

## Part D — the diagnosis, only if 6–7 is still dropped

Read-only, from this run's own trace. Answer, in `C:/PR/artifacts/t362/diag362.md` with the numbers that settle it:

1. **Which stage** — refused at the fit, erased in `PrepareNextStep` by `MergeOverlapTest` (`class/ranking_system.cpp:2218`, reason `pc_overlap`) or by `SingleOverlapTest` (`:2260`), erased by `CountPCInlier` (`:2238` / `:2278`, thresholds `7.0` / `1.5`), or never offered because its step came later. Name the line.
2. **How wrong the pose is at the moment of the drop** — take the offered transform, move both shards, and measure: the interpenetration the test saw (number of overlapping points and its own threshold), and the same against the **GT pose** for the same pair. A pair that passes at the GT pose and fails at the fitted pose is a **fit** verdict; a pair that fails at both is a **test/threshold** verdict.
3. **Which of the three claims it is** — (1) the method failed, (2) the measurement is wrong, (3) the reference answer is wrong — stated once, plainly, then the numbers.
4. **What would fix it**, named as a candidate, not built: which switch or threshold would have to move, and what would have to be re-measured to prove it. No code change in this ticket.

If 6–7 is kept and committed, skip Part D and say so in one line; still record its committed error.

## Part E — render and readings

Render `C:/PR/artifacts/t362/render362_assembly.png` (≥1800×1200), four labelled panels from the `FINAL` poses: `juglet_def_stock`, `juglet_def_xy`, the Pot A control, and a close-up of pieces 6 and 7 at their final pose with their edgelines drawn. **Look at it and write one sentence of what you see before the Reading lines.**

Pick exactly one Reading line, verbatim:

- `fixed-edgeline-keeps-6-7` — 6–7 survives pruning and is committed, and the Pot A control holds.
- `fixed-edgeline-still-drops-6-7` — 6–7 is still dropped; Part D is filled in.
- `fixed-edgeline-breaks-controls` — the Pot A control fails.
- `blocked: lanes` — Part A failed.

Plus one line, only when Part D ran: `Diagnosis: <method|measurement|reference> — <one clause>`.

## Out of scope

- Any edit to `class/`, `hpc/`, `main.cpp` or the pristine tree; the two shim literals in the run directory are the only permitted change.
- Changing the pruning code, the overlap tests, the thresholds, or the growth/merge settings.
- The 1× bundle, or any lane other than ticket 80's nine `def_3x` cells.
- Committing, pushing, or opening/removing worktrees.
