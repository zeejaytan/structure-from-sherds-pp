# Ticket 362 — Assembly on the default-on edgeline: does the search keep the 6–7 pair, and if it drops it, why

**What to build:** Run the **new** edgeline — ticket 79's wall-pair rule, on by default after ticket 80 — through the assembler rig that measured the GMnr lanes (ticket 361), and answer two questions in one run: does the fixed line change the join set, and **does the 6–7 pair that now fits at 3.98° / 1.17 mm still get dropped at pruning?** If it still drops, diagnose from the arm's own trace which stage killed it and whether the fit or the test is wrong. No code change.

**Answers:** S1
**Blocked by:** **nothing for Part D0 and Part A** — the conservator waived the eye-check ordering on 2026-11-17 and asked for the assembly work and the investigation to run while he verifies visual-qa. **Part B/C's arms need ticket 80's `def_3x` lanes and its default-on binary** (`SfSpp_preprocessing/.scratch/juglet-breaklines/issues/80-default-on-and-the-edgeline-for-the-eye.md`); they do not exist yet. Do not fabricate lanes: do D0 and A, then end `waiting: ticket 80 lanes`, and the lead will message you to continue.
**Status:** resolved
**Branch:** 362-assembly-on-the-default-on-edgeline, in /c/PR/worktrees/structure-from-sherds-pp/362 (merged, 73e11f6)
**Worker:** dsh one-shot subagent w362
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

## Part D — the diagnosis

Two halves. **D0 runs now**, on ticket 361's own traces, because the drop is already recorded and the question "why" does not need the new lanes. **D1 runs after Part B**, only if 6–7 is still dropped on the fixed edgeline.

### D0 — why 6–7 was dropped at pruning on the GMnr lanes (ticket 361's run)

Ticket 361's run directory holds every arm's binary log: `/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0129_d2fb631_t361-gmnr-lanes/logs/binary_*.log`, and arm `juglet_gmnr_xy` carries `[SFS-339] FATE 7 64 -1 6 7 drop pc_overlap 16` together with `[SFS-339] OFFER` lines and `[SFS-336]` lines where the switch was on. Reconstruct, read-only:

1. **The whole life of the 6–7 candidate in that arm** — its fit (3.978°, 1.174 mm, 16 inliers), every `OFFER` step it appears at and at what priority, the `FATE` line when it dies, and whether it was re-offered later. Quote the lines verbatim, with step numbers.
2. **Why `pc_overlap` fired** — the code path is `PrepareNextStep`'s merge branch (`class/ranking_system.cpp:2218`) and its single branch (`:2260`): both call an overlap test *after moving one shard into the group's frame*, and both erase before `CountPCInlier` ever runs. Read those two test functions and say, in plain words, what they test (how much of one shard's points sit inside the other's surface, at what tolerance), then compute it yourself for 6–7 at the pose that was offered: **how much overlap did the test actually see, against its own threshold**, and the same number for the pair at the **GT pose**.
3. **The one number that settles it** — if 6–7 passes the same test at the GT pose and fails at the fitted pose, the verdict is a **fit** verdict; if it fails at both, it is a **test/threshold** verdict. Give both numbers and say which.
4. **Whether 4–7 explains anything** — 4 and 7 are not adjacent in the ground truth, yet `C1{4,7}` held after the drop. Check whether the 4–7 candidate was offered at a step that made 6–7's overlap test worse (the group frame 7 was already sitting in), and say what the trace shows. If the trace cannot settle it, say that rather than guessing.

### D1 — the same, on the fixed-edgeline arm, only if Part B still drops it

Repeat D0 on this run's own log for `juglet_def_xy`, with the same four items. The point of D0 was to know the mechanism before the new lanes; D1 is whether anything about it changed.

Then make the claim call, once, plainly: (1) the method failed, (2) the measurement is wrong, (3) the reference answer is wrong — with the numbers that force it. **Name what would fix it as a candidate, never build it**: which switch or threshold would have to move, and what would have to be re-measured to prove the fix. No code change in this ticket.

Write `C:/PR/artifacts/t362/diag362.md` with the numbers that settle it, and keep the D0 and D1 sections separate.

1. **Which stage** — refused at the fit, erased in `PrepareNextStep` by `MergeOverlapTest` (`class/ranking_system.cpp:2218`, reason `pc_overlap`) or by `SingleOverlapTest` (`:2260`), erased by `CountPCInlier` (`:2238` / `:2278`, thresholds `7.0` / `1.5`), or never offered because its step came later. Name the line.
2. **How wrong the pose is at the moment of the drop** — take the offered transform, move both shards, and measure: the interpenetration the test saw (number of overlapping points and its own threshold), and the same against the **GT pose** for the same pair. A pair that passes at the GT pose and fails at the fitted pose is a **fit** verdict; a pair that fails at both is a **test/threshold** verdict.
3. **Which of the three claims it is** — (1) the method failed, (2) the measurement is wrong, (3) the reference answer is wrong — stated once, plainly, then the numbers.
4. **What would fix it**, named as a candidate, not built: which switch or threshold would have to move, and what would have to be re-measured to prove it. No code change in this ticket.

If Part B keeps and commits 6–7, skip D1 and say so in one line; still record its committed error, and still file D0 — the mechanism of the 361 drop is worth having either way.

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

---

## Comments (worker, 2026-11-17) - w362

**Part D0 — why 6–7 died on ticket 361's GMnr xy arm** (read-only, log `logs/binary_juglet_gmnr_xy.log`). Full write-up: `C:/PR/artifacts/t362/diag362.md`.

The premise in this ticket needs one correction: **the drop was not `PairwisePruning`**. The candidate passed the pair stage (its overlap at the fitted pose is 47.52 against the 50.0 bar at `class/feature_matching.cpp:1944`) and died in `PrepareNextStep` — the `FATE` line carries `j = -1`, which only that path prints. Verbatim, log line 15618:

```
[SFS-339] FATE 7 64 -1 6 7 drop pc_overlap 16
```

1. **Life of the candidate.** Fit `[SFS-338] CAND338 210 6 7 0.469625 16 3.49641 1 3.97842 3.52091` — 3.97842°, 3.52091 tree units = **1.1736 mm**, 16 inliers, `gt_edge = 1`. (The only other 6–7 candidate is the wrong-registration one, 156.5°.) It was offered **25 times**, steps 0–5, states 1, 2, 3, 5, 6, 13, 14, 18, 20, 27, 29, 31, 36, 44, 45, 46, 50, 52, always `branch 3`; at steps 0–4 all 25 ended `skip branch 3` (offered, never built). From step 5 it was erased 17 times (16 `pc_overlap`, one `pc_dist` at `5 52`). **No re-offer after step 5**: `OFFER` lines for steps 6 and 7 are absent, so the last real offer is `5 52 j9 (30 inl) / j13 (6 inl)`.

2. **What the tests measure.** One function, `OverlapCheck_3d` (5-arg overload, `class/feature_matching.cpp:894-968`): half the summed length of the segments that pierce **both** shards' local surface normals (`DetectIntersection`, `:745-802`), in tree units — pure lip **interpenetration between two traced breaklines**. Three call sites, three bars: pair stage **50** (`:1944`, the entry's own pair only), merge branch **100** (`class/ranking_system.cpp:2218` → `MergeOverlapTest`, `class/feature_matching.cpp:2232-2263`, every cross-group pair under the entry's pose, first offender wins), single branch **100** (`:2260` → `SingleOverlapTest`, `class/ranking_system.cpp:2757-2779`). Both admission tests erase before `CountPCInlier(…, 7.0, 1.5)` can vouch for the entry. At the state that killed 6–7 the partition was `[SFS-337] STEP 7 64 502 0 1,2,3,5,7,8|4,6,9`, so 6 and 7 sat in different graphs → **merge** branch, and 6–7 is the **fourteenth** cross pair tested ((1,4) is the first).

3. **The one number.** Same engine as ticket 359's verified scorer (`gates359.py` with `TREE` forced to the 361 tree; it reproduces all four logged 6–7 areas to 5 decimals): at the **offered** pose **47.5238** (12 crossings, 16 inliers) — **passes** the 50 bar; at the **GT pose** (361 bundle) **52.2773** (10 crossings, 20 inliers) — **fails** it by 2.28 units = 0.76 mm. So neither "fit" nor "fails at both": the fitted pose passes and the **reference placement is refused**. Verdict: **test/threshold**. And at the 100 bar where the erase actually happened, 6–7's own number cannot be the trigger — the `pc_overlap` is collateral from a cross pair dragged in by the entry's pose, which pair the log does not record.

4. **4–7.** The non-adjacent pair (`gt_edge = 0`) fits **better** than the true one: `CAND338 177 4 7 … 57 0.955249 0 2.11883 1.63271` = 2.119° / 0.544 mm / 57 inliers, and its pair overlap is **0.0167** (fitted) / 6.22 (GT), against 6–7's 47.52. Fit error and overlap both prefer the pair that does not exist. But 4–7 was dropped in the **same** two batches as 6–7 (`FATE 7 62 -1 4 7 drop pc_overlap 24`, `… 30`, `FATE 7 64 -1 4 7 drop pc_overlap 28`), and the `C1{4,7}` that survives is the partition the ranking already had at state 60 (`STEP 7 60 644 1 1,2,3,5,6,8,9|4,7`) — it was not "held" by a 4–7 entry through step 7. Structurally it can have made 6–7's test worse (the merge carries 4 and 9 into the main group under a pose fitted only to 6→7, and the sweep reports whichever cross pair comes first), but **the trace cannot settle that**, and I have not pretended otherwise.

**Claim call: (2) the measurement is wrong.** The true pair fits at 3.98° / 1.174 mm with 16 inliers; the same test at the correct pose scores 52.28 and is refused; a false pair scores 0.0167. The metric ranks the false pair clean and the true pair unacceptable, then the 100-bar merge sweep kills the true pair on a cross pair it never fitted. Not (1): the method found the right pose. Not (3): the reference is right — it is the one being refused. (359's 191.17 is not a counter-example: it was measured on the **other** bundle. 361 read `bundle3x/juglet` Bl0 pieces 6/7 `ef41f9af…`/`e519d7da…` (133/172 loaded points) while 359 measured ticket 355's `09ac47ad…`/`3263ab4e…` (141/174); Axes and GT are md5-identical, only the traced lines differ.)

**Candidate fix (named, not built):** confine `MergeOverlapTest` to the entry's own `(y,x)` pair — or re-register each cross pair before testing it — and let `CountPCInlier(7.0, 1.5)` run before the overlap verdict; the threshold is the weaker lever (60 would pass 6–7 at GT, but it widens the gap 4–7 already exploits and does not touch the sweep that erased the entry). To prove it: a per-pair area trace inside the sweep, the GT-pose overlap distribution over the 16 adjacent pairs (here max 52.28, next 25.09), and a re-run showing 6–7 committed **and** Pot A still 8/8, 15/15.

**Part A — lanes (read-only census).** Ticket 80's `def_3x` cells **now exist** (`/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0430_d272313_t80-default-on`, cells written 04:46): `out/cell_def_3x/pieces/Juglet_Piece_1..9`, `geom/` **36/36** files. Every piece's `mesh.log` carries `[SFS-79] wall pair filter: on (default)` — the switch was **unset**, not `=pair` — and the env line is exactly ticket 80's (radius 1.0 / 0.887 / 1.5 / 25 / 3.0 plus `WORNESS=0.7`), with no `SFS_REFINE_*` anywhere. Bl0 off-lip dots per piece (score80's metric, W_MM 0.80, core = S0 with `sgn*(n.u) > 0.20`): **0, 0, 1, 0, 0, 0, 2, 0, 0** — Sherd 7 gives **2 dots / 0.601 mm**, reproducing the ≤ 3-dot / ≤ 0.599 mm reading within the scorer's 0.012 mm tolerance. Binary md5s `MeshPreprocessingHeadless 82dd4d5911fb6832e2889fd696867e30` (the default-on build) and `EdgeLineExtractionHeadless a481f49ec5450adfa0de872fef7ed969`. Sherd 7's wall-pair kept 49307 of 49937 points (`add_refused=732`).

Staging (ticket 67 recipe) is **not yet built** for this ticket — no arm was dispatched, so there is no `Breaklines/ Surfaces/ Mesh/ Axes/` tree and no Pot A hardlink set. The recipe's last instance (the 361 arm inputs) censuses at **9 Breaklines / 18 Surfaces / 9 Mesh / 9 Axes** for juglet and **8 / 16 / 16 / 8** for Pot A. Build route verified **before** dispatch: branch shim `hpc/tools/main_pristine_batch.cpp` md5 **daa9d614cfa78de79584cd26933a2057**, guard step a no-op (ticket 67's guard already inline at `:366-369`, so guard-added is byte-identical), and the floor edit `point_.cols() < 50` → `< 10` at `:142` gives **0c216463ae27fe4571b200cc2292da99** — all three match ticket 361's recorded values, so a different number later means a different source. Driver to copy: `submit361.sbatch` md5 `61b787a9f3d2545d4a530f6f075e770e`; arm runner `tools/arm335.sh` md5 `0ec9942d7a295b7dd65c88b850a64f73`.

**Not done, and why.** Part B/C (arms, scoring), Part D1 (no fixed-edgeline arm exists), Part E (no `FINAL` poses to render): the lead's order for this ticket was D0 then Part A, then stop. The lanes and the default-on binary exist now, so the remaining work before the arms is only the staging step plus one Slurm array; ticket 80's own scored record (`gate80.tsv`) was still unwritten when this census ran, which is why the per-piece dot row is `info` rather than `pass`. No run directory was created and no `runs/index.tsv` row was written, because no arm ran.

**Deliverables:** `C:/PR/artifacts/t362/diag362.md` (D0, with every verbatim line and the claim call), `C:/PR/artifacts/t362/gate362.tsv` (33 rows: lanes + diag, columns `group arm pair metric value reference verdict note`). No code change, no commit, no worktree command.

**Part B/C — the arms that actually ran, on ticket 80's lanes** (supersedes the two "not yet built" paragraphs above; ticket 80's cells landed at 04:46 and the lead ordered the run). Run dir `/data/gpfs/projects/punim2657/SfSpp/runs/20261117-0512_4012ca1_t362-def-lanes`, source `git archive HEAD 4012ca1`, floor-10 shim `0c216463ae27fe4571b200cc2292da99`, env = ticket 80's `def_3x` line exactly, `SFS_AXIS_LOCK_ICP` unset, staged by the ticket 67 recipe from `out/cell_def_3x/geom/` (juglet 9 Breaklines / 18 Surfaces / 9 Mesh / 9 Axes / 10 GT, pota 8/16/8/8/9).

| arm | SHERD / EDGE | clusters | PlaceErr ≤5°/5 mm | seated at 13.788 mm | max chamfer |
|---|---|---|---|---|---|
| `juglet_def_stock` (both switches off) | 8/9, 8/18 | 1 × 9 | 14 of 14 | **5 of 9** | 17.748 mm |
| `juglet_def_xy` (both switches on) | 9/9, 13/18 | 1 × 9 | 14 of 14 | **9 of 9** | 11.093 mm |
| `pota_def_xy` (control) | 8/8, 15/15 | 1 × 8 | 15 of 15 | 8 of 8 | 2.001 mm |

**The Pot A control holds**, so Part B is not void. Against ticket 361's GMnr arm (6/9, 5/18, `C0{1,2,3,5,6,8,9}`+`C1{4,7}`, 10 of 14 in bar, 0 of 9 seated, 172.6 mm) the difference is the lanes, not the rig: **on the default-on lanes both rigs put all nine sherds in one cluster with all 14 adjacent joins inside 5°/5 mm at the committed poses.** What the rig buys is depth — 5 of 9 seated → 9 of 9, 17.748 mm → 11.093 mm worst — and one honest cost stays: the false pair 4–6 (62.9° / 14.3 mm) is still merged into the single cluster. Slurm: `35229861` FAILED 3:0 in 2 s (`LOCK: unbound variable`, a dropped init under `set -u`), `35229902_1` COMPLETED 0:0 00:09:05, `_2` COMPLETED 0:0 00:03:19, `_0` FAILED 3:0 (`flock` over GPFS did not serialise two staging nodes), resubmitted as `35229970_0` COMPLETED 0:0 00:07:30. Both failures are driver bugs, fixed in `submit362.sbatch` before the surviving runs; no arm result is affected.

**D1 — skipped, as the ticket allows (line 77): 6–7 is kept and committed.** One line, as asked: on `juglet_def_xy` it survives pruning with **32 inliers** and is committed at **1.656° / 0.291 mm**, `in_bar_F 0` only because the CAND fit it was scored at (6.674°/1.542 mm) is outside the bar while the committed pose is well inside. Its committed error on the stock arm is 2.613° / 0.538 mm. The reason nothing needed fixing is the bundle, and it is measurable: at the GT pose 6–7's pair overlap is **14.35** here, against **52.28** on 361's GMnr bundle and **191.17** on 359's 355 bundle, and on these lanes **no** adjacent pair exceeds the 50 limit at the GT pose (next: 6–9 21.53, 2–6 10.44). The same engine, the same bar, three segmentations — only the two older ones trip it, which confirms the D0 claim call by construction rather than overturning it.

**Part E — the render.** `C:/PR/artifacts/t362/render362_assembly.png` (2415×2100). What I see: two complete juglets (stock left, xy right) with all nine sherds in place and the side seams visible in both, the xy one with its base ring flush; the Pot A bowl complete with no visible seam step; and in the 6–7 close-up the two breakline point sets meeting along the seam instead of crossing through each other.

`Reading: fixed-edgeline-keeps-6-7`

`Diagnosis: measurement — the pair/merge tests score lip interpenetration of whatever breakline tracing they are handed, and only the GMnr and 355 segmentations push the true 6–7 pose over the bar (52.28, 191.17) where ticket 80's wall-paired lanes measure 14.35 and accept it.`

**Two corrections to my own D0 text above**, both from the lead's re-read: paragraph 1's "offered 25 times" is **37** `OFFER` lines (one per step/state/j, all steps 0–5; the 17 `FATE` drops and the two `STEP` partitions are unchanged); and the 52.28 GT figure should be quoted with its margin — ±0.5° swings it 26.2–58.1 and ±0.1 mm swings it 42.4–51.6, so 47.52 and 52.28 are the same reading inside this metric's own jitter at sub-0.1 mm scale, which is itself part of why the bar cannot certify the true pose. The verdict and mechanism stand.

**Before this ticket lands:** ticket 80's `pins/t80.diff` must land first — the nine `def_3x` cells are built from commit `d272313` plus that diff, and the default-on edgeline is `SfSpp_preprocessing` main `c16a3f0`, neither of which is in this repo's archive, so the lanes are not reproducible from `4012ca1` alone. Also noted for whoever files the fix: `hpc/tickets/58`'s `score58.py` is juglet-named only (`Juglet_*` T/Mesh/Breakline/graph names); scoring Pot A needs a name view, which I built outside the arm's tree (`score_pota2.sh`, `work/view_pota_score/`).

**Deliverables:** `C:/PR/artifacts/t362/gate362.tsv` (54 lines, header + 53 data rows: lanes, shim, diag, arms, D1, render), `diag362.md` (D0 §0–8, Part B/C/D1/E §9–12), `render362_assembly.png`, `pairs362.tsv`, `funnel362.tsv`, `run.json` + one `runs/index.tsv` row in the run dir. No code change, no commit, no push, no worktree command.

**End state:** ready

---

## Verdict (lead, 2026-11-17)

**Reading accepted — `fixed-edgeline-keeps-6-7` — and it carries more than the ticket asked for.
On ticket 80's lanes the Juglet assembles: one cluster of nine, all 14 adjacent joins inside 5° / 5 mm
at their committed poses, and `juglet_def_xy` seats 9 of 9 at the 3× bar, worst piece 3.70 mm real.
That is the first time this project has seated a Juglet piece at all; ticket 67's best was 0 of 9 with
chamfers of 458–475 mm.**

### What I verified myself, not taken from the worker

- **Slurm**, my own `sacct -X`: `35229861_0.._2` **FAILED 1:0** in 3 s (the ticket's table records 3:0 for
  this job — sacct is the record and it is 1:0), `35229902_0` FAILED 3:0 2 s, `_1` **COMPLETED 0:0 09:05**,
  `_2` COMPLETED 0:0 03:19, resubmitted `35229970_0` **COMPLETED 0:0 07:30**. The two failures are driver
  bugs the worker names and fixed before the surviving runs; no arm result depends on them.
- **The traces**, read in the run dir: `[SFS-337] STEP 8 95 859 0 1,2,3,4,5,6,7,8,9` — one cluster holding
  all nine at step 8; `[SFS-339] FATE 7 87 0 6 7 keep accept 852` — 6–7 kept; `[SFS-341] EDGE 7 81 1 6 7 20
  1.23074 34` — the committed edge; and the accuracy lines verbatim: `juglet_def_xy` **Sherd 9/9, Edge
  13/18**, `juglet_def_stock` **8/9, 8/18**, `pota_def_xy` **8/8, 15/15**.
- **The seating numbers, including the bar's scale**, which the ticket does not state and the verdict
  turns on. I pulled score58's own `tables.seating` for the xy arm (chamfers 6.225 / 11.093 / 8.810 /
  6.651 / 6.232 / 9.632 / 6.178 / 6.438 / 10.195, `seat_bar_mm 4.596`, seated 0) and then measured the
  frame rather than assuming it: the staged `Juglet_Piece_2_Mesh.obj` is the 3× file — **84.93 tree units**
  across against **28.31 mm** for the 1× file, the same 2,565 vertices, ratio **3.0006** — and the GT
  translation is 489.89 tree units = 163.3 mm. So the chamfers are in tree units, score58's 4.596 is a
  1× bar, and the right bar is **3 × 4.596 = 13.788**. The 9 of 9 is therefore a real seating in
  millimetres: worst piece **11.093 tree units = 3.70 mm**, 80 % of the own-place bar. `score58.py`'s
  `0 of 9` is its hard-coded 1× bar applied to a 3× bundle — a scorer defect, not an assembly defect.
  `funnel362.tsv` carries both counts, so nothing is hidden, but see the open item below.
- **The three-segmentation table**, recomputed by me on these lanes (`gtareas_def362.py`, ticket 359's
  engine with `TREE` repointed at ticket 80's bundle): 6–7 at the GT pose is **14.3509** with 22 inliers;
  the next largest adjacent pair is 6–9 **21.5287**, then 2–6 10.4433, and nothing else above 8.47. With
  ticket 361's **52.2773** and ticket 355's **191.17** that is the same engine and the same bar on three
  segmentations, and only the two older ones trip it. D0's claim call is confirmed by construction.
- **The render**, looked at before writing this: two complete juglets (stock left, xy right) with all nine
  sherds in place, side seams visible, the xy base ring flush; the Pot A bowl complete with no seam step;
  and in the 6–7 close-up the two breakline sets meeting along the seam instead of crossing through each
  other. It reads as a vessel, not as a knot — against ticket 361's two-lump blob on the same rig.
- **The switch that made the lanes**: `mesh_processing_headless.cpp` on `SfSpp_preprocessing` main at
  `f7fd884` is **f93be3ca…**, the md5 ticket 80 pinned. The worker's "un-pinned lanes" caution is therefore
  satisfied for the source; the pin *file* stays in ticket 80's run dir on Spartan.

### What it means

The 6–7 loss was never a fitting problem. The spurious off-lip flap gave the pair-stage overlap test
something to count: on the GMnr lanes the true placement scores 52.28 against a 50 bar and is refused, on
ticket 355's bundle 191.17. Remove the flap and the same measurement at the same true pose reads **14.35**.
So the metric was reading the tracing it was handed — which is why one fix upstream (ticket 79's rule,
on by default in ticket 80) cured the fit, the drop and the seating at once, and why no pruning code had to
change. D0's structural defect is still real for other lanes — a 100-bar sweep that erases an entry on a
cross pair under a pose fitted to one pair, and prints nothing naming it — but on these lanes the sweep
never fires, so it is not what decides this result.

**The rig buys depth, the lanes buy topology.** Both rigs put all nine in one cluster with 14/14 joins in
bar; the xy switches take the seating from 5 of 9 to 9 of 9 and the worst chamfer from 17.748 to 11.093
tree units. The Pot A control holds at 8/8 and 15/15, so Part B is not void.

### Open, recorded here and not ticketed

1. **`score58.py` must take the bundle's scale.** Its 4.596 bar is 1×; on a 3× bundle it reports 0 of 9 for
   a 3.70 mm assembly. Until it is fixed every juglet seating number has to be quoted with its bar, and the
   bar has to be derived (× `SFS_TREE_UNITS_PER_MM`) rather than typed.
2. **The false pair rides inside the correct cluster.** `4–6` is not adjacent (62.9° / 14.3 mm at its fit)
   and is merged into the single group in both arms; `4–7` too on the stock arm. It costs nothing at the
   seat bar here, and that is the point worth watching: a wrong join inside a right assembly is invisible to
   chamfer, so the join table is what must be read, not the picture.
3. `score58.py` also knows only `Juglet_*` names; scoring Pot A needed a name view built after the
   arm ran (`work/view_pota_score/`). The arm tree was not touched.

**Reading `fixed-edgeline-keeps-6-7` stands as written. What it adds, which the worker could not say
without reading the seating: the fixed edgeline does not merely keep the join, it assembles the vessel.**

