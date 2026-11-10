# Ticket 326 — Scale-free assembly: stop judging a 33 mm vessel with pot-sized millimetres

**Status:** wontfix - rule withdrawn as mis-tuned in direction (pot E 62 -> 46-53); evidence kept, branch not merged, corrected clamp recorded in the lead ruling below
**Branch:** 326-scale-free-assembly, in /c/PR/worktrees/structure-from-sherds-pp/326
**Answers:** S1
**Depends on:** 325 (an admissible axis at this scale; without it the axes arm cannot be read)
**Blocks:** none
**Worker:** cd0c9535-0456-4a7e-8d85-228e391522bb (dsh one-shot subagent; dispatched 2026-11-10 by the lead after the cursor CLI child was cut off mid-build)
**Budget:** large

> **In one line.** Ticket 325 proved the axis finder is not why the Juglet does not assemble: given the
> *answer-key* axes the assembler still seats 2 of 9 pieces and finds 0 of 18 joins, on both repeats. The
> assembler's matching gates are pot-sized literals (20 mm translation, 50 mm overlap radius, 100 mm search
> distances), and on a 33 mm vessel a 20 mm tolerance is most of the vessel. Make every length on that path a
> fraction of the object's own measured size, prove the ten pots do not move, and then the Juglet is a real
> test instead of an out-of-range input.

## What is already known (verified by the lead, 2026-11-10; do not re-derive it, reproduce it)

**(a) The axes are not the blocker.** Juglet E2E, current t66 lane bundle, job **34659057** (COMPLETED 0:0),
six arms, two repeats: answer-key axes **2/9 pieces, 0/18 joins** (both repeats); our 325 axes 0/9; our
September axes 0/9. Baseline t323 was 0/9 on four arms. So the seam-matching stage rejects the truth.

**(b) At the true placement the interior lines physically meet.** Bl0 (the only line the assembler reads)
minimum gap at GT, in mm — the ten touching pairs from the answer key (1-2, 1-8, 1-9, 2-5, 2-9, 3-5, 3-7,
6-7, 6-9, 7-9):

| pair | 1-2 | 1-8 | 1-9 | 2-5 | 2-9 | 3-5 | 3-7 | 6-7 | 6-9 | 7-9 |
|---|---|---|---|---|---|---|---|---|---|---|
| gap mm | 1.10 | 0.30 | 2.12 | 2.91 | 0.79 | 0.40 | 14.19 | 0.36 | 0.58 | 1.79 |

and the eight non-contacts: 1-5 5.62, 1-6 5.04, 4-5 8.36, 4-6 10.06, 5-9 20.29, 6-8 3.60, 7-8 15.33,
8-9 21.38. Nine of ten true pairs are inside 3 mm; no non-contact is inside 3.6 mm. A seam gate scaled to
the object would separate them; the pot-sized one does not.

**(c) The strict gate still admits only 3 of 10 true pairs — and distance is not what rejects the rest.**
Probe at GT, Bl0→Bl0, 2.0 mm **and** normal dot >= 0.85 (the lead's replica of MakeCorWOBuildTree +
UnifiedPotteryValidation, `C:/PR/structure-from-sherds-pp/artifacts/juglet_run1/gate_probe_b0.py`,
GATES = [(2.0, 0.85), (20.0, 0.7)]), inlier counts A→B / B→A:

| pair | 1-2 | 1-8 | 1-9 | 2-5 | 2-9 | 3-5 | 3-7 | 6-7 | 6-9 | 7-9 |
|---|---|---|---|---|---|---|---|---|---|---|
| inliers | 0/0 | 18/17 | 0/0 | 0/0 | 4/5 | 7/5 | 0/0 | 0/0 | 7/6 | 0/0 |

All eight non-contacts score 0/0. Pairs whose lines meet at 0.36–2.91 mm (1-2, 1-9, 2-5, 6-7, 7-9) score
**zero**, so the **normal-agreement condition** rejects them, not the 2 mm distance. This must be understood
before any constant is changed: a dimensionless distance gate alone will not fix those five pairs.

**(d) One pot-sized gate that plausibly *collapses* correct candidates on this vessel.**
`feature_matching.cpp:1596` — `if ((iter->SamePart(*comp)) && (iter->trans_.isSimilar(comp->trans_, 0.175, 20)))`
treats two candidate placements within **10 deg and 20 mm** as the same placement and keeps one. On a 33 mm
vessel, 20 mm is most of the object, so genuinely different joins can be collapsed into one and the survivor
picked by score. Same shape at `ranking_system.cpp:256` and `:315` — `isSimilarTrans(T_c, T_i, 0.436, 20.0)`.

## The constants on the path (audit targets, pristine upstream 9195c01)

The sources that actually run are the upstream ones inside the image, extracted read-only to
`C:/PR/structure-from-sherds-pp/artifacts/pristine-9195c01-src/` (`feature_matching`, `reconstruction`,
`ranking_system`, `data_structure`, `filter`, `main`; 7763 lines). **The fork checkout is not what the scored
runs compile** — do not edit the fork and call it done.

| pristine 9195c01 | literal | what it compares |
|---|---|---|
| `feature_matching.cpp:1596` | `isSimilar(., 0.175, 20)` | two placements within 10 deg / **20 mm** are duplicates |
| `ranking_system.cpp:256, :315` | `isSimilarTrans(., 0.436, 20.0)` | ranking-side duplicate/mate test, **20 mm** |
| `feature_matching.cpp:1674` (+ `:1822`, `:1825`, `:1895`; `ranking_system.cpp:1781`, `:1879`) | `OverlapCheck_3d(..., 50.0)` | overlap neighbourhood / search radius **50 mm** |
| `feature_matching.cpp:611/:688/:765` | `(center_L0 - center_L1).norm() < 5` ; `area = 1000` | "centres too close" screen **5 mm**; fallback area **1000 mm^2** |
| `feature_matching.cpp:1182, :1201, :1211, :1334, :1344` | `double dist(100);` / `dist > 10` | KD-tree search radius **100 mm**, cut **10 mm** |
| `feature_matching.cpp:1446-1449`, `:1518-1521` with `:273-279`, `:385-391` | `Q_size[0..3] = 0.15/0.15/0.15/0.2`; `Compare[0] < 3*Q_size[0]` … `4*Q_size[3]` | LCS similarity tolerances — state each one's unit before judging |
| `feature_matching.cpp:318, :431` | `Clustering(Out, 20)` | candidate clustering threshold |
| `feature_matching.cpp:1726`, `:1860` | `lowest_score > 1.5`, `score_ > 10` | score gates |
| `reconstruction.cpp:1113` | ICP weights `w_d 1.0, w_n 3.0, w_line 1.0, w_a 0.1, w_r 1.0, w_h 1.0`; `CauchyLoss(2.0)` (:1216-1218, :1418-1420, :1611-1613); `isConverge(T, 0.1, 2.0)` (:1279, :1475, :1684) | ICP robust-loss scale **2.0** (mm) and convergence **2.0 mm** |
| `hpc/tools/main_pristine_batch.cpp:51-52, :55` | `TOP_k 5`, `BRANCH_b 3`, `NO_BASE_INFO` | search width (integers — not scale, but record them) |
| shard admission (patched per run by `patch_guard67.py`) | `edge_line_.point_.cols() < 50` → `< 20` | point floor; Juglet pieces hold 22–59 Bl0 points |

Not every literal above is a length, and not every length is wrong: **classify first, then change.** Angles and
ratios (0.175, 0.436, 0.85, 1.5, 10, 3/2.5/4 times Q_size) stay as they are.

## What to build

1. **Diagnosis before change (no code change in this step).** For each of the 18 pairs at the true placement,
   report the exact condition of `MakeCorWOBuildTree` / `UnifiedPotteryValidation` that rejects it and the
   margin (distance of the closest normal-agreeing pair, normal dot of the closest pair, whether a
   correspondence is built but then pruned by `isSimilar(0.175, 20)`, by the overlap screen, by the score
   gates, or by `Clustering`). Say **why** the five meeting pairs fail on normals: are the estimated normals
   wrong in sign (wall normals across a crack point opposite), too noisy, or is the nearest-point pairing
   wrong at 1.9 mm resampling on a 33 mm object? This table is the deliverable that decides what may be
   changed — do not skip it and do not tune against it.
2. **Make the lengths dimensionless.** Each absolute length on the live path becomes a multiple of a scale
   measured from the object itself — candidate measures: median breakline point spacing, piece bounding-box
   diagonal, breakline segment length, wall thickness — chosen so the pot-side decision is preserved inside
   its measured operating range. Report, for every constant, the pot-side min/median/max of the measuring
   quantity over the ten scored pots and where the new expression sits in that range (the ratio must be
   ≥1.0 on the pot side, i.e. the pots must still be inside every gate they were inside before).
3. **Ship the change as a new image, not as a per-run edit.** This repo forbids a job editing source
   (AGENTS.md, "No job copies or edits source"); the build is the pristine scorer plus the shim, and class
   objects come prebuilt from `/panel/build` inside the image (`work/arm67.sh:61-62` only relinks the shim
   from `/panel/logs/link_raw_$ARM.txt`). So: add `hpc/containers/container-pristine-assembly-326.def`
   alongside the existing one — same upstream clone and the same `git rev-parse HEAD` assertion
   (`9195c0167e2077a564bd7c91156063252feeb506`, clean tree) — then `git apply` a patch committed as
   `hpc/patches/326-scale-free-matching.patch` (ship it in with `%files`), and build it with a copy of
   `hpc/sbatch/build_pristine_assembly.sbatch` pointed at the new name. `sfs_main/sfs_pristine_assembly.sif`
   stays byte-identical so every earlier result remains reproducible. **Prove the new binary is the one that
   ran**: print a version marker (the patch's own string) at startup into every run log, and record the
   `git apply` + `md5sum` of the patched `feature_matching.cpp` inside the image at build time. A run that
   silently used the old objects measures nothing. Falling back to injecting a stray `.o` before the archive
   is not acceptable.
4. **Pot no-regression, two repeats, ten pots**, each at or above its recorded bar (ticket 325, acceptance
   bar 6): A 8/8 15/15, B 9/9 15/15, C 4/4 5/5, D >=23/28 >=55/69, E >=29/31 >=57/62, F 5/5 7/7, G 7/7 10/10,
   H 11/11 17/17, I >=25/26 >=59/62, J >=8/11 >=16/21. The arms must differ from the recorded baseline in
   exactly one thing — the assembler binary; same bundles, same axes
   (`.../20261110-0740_d628fc0_t323-axes/out/axes323`), same floors and the same acc/score scripts.
5. **Juglet E2E, two repeats, two arms** — (i) new assembler + answer-key axes `Dataset/Juglet_GT`,
   (ii) new assembler + our 325 axes. Control is today's measurement (2/9 and 0/18 with the answer-key axes).
6. **Renders** before/after into `structure-from-sherds-pp/artifacts/pota326/renders/` and the numbers beside
   them.

## Acceptance

- [ ] **Diagnosis table** for all 18 pairs: rejecting condition, margin, and the normal-failure cause for the
      five pairs whose lines meet (numbers, not adjectives).
- [ ] **Audit table**: every constant above with unit classification (angle / ratio / length / count), the
      pot-side min/median/max of the quantity it is compared against, its dimensionless replacement, and the
      Juglet value. Plus `grep -n` proof that no bare mm literal survives on the changed path.
- [ ] **Two-sided margin table**: for each new gate, the tightest pot-side case and the tightest Juglet case,
      so a reader can see the margin on both sides rather than one threshold.
- [ ] **Proof the new image ran**: the patch's version marker in every run log, the image build job's
      `sacct` line, the `md5sum` of the patched `feature_matching.cpp` recorded at build time, and
      `sfs_pristine_assembly.sif` byte-unchanged (`md5sum` before/after).
- [ ] **Ten pots, two repeats**, every pot at or above its recorded bar, with job IDs and `sacct`
      State/ExitCode per arm. Any pot below its bar **stops the ticket** — report which gate flipped and by how
      much; never nudge a threshold to recover it.
- [ ] **Juglet E2E**, two repeats: with the answer-key axes, **both repeats >= 7/9 pieces seated and
      >= 6/18 joins** (today: 2/9, 0/18); report the our-axes arm with the same two numbers. Fewer joins than
      6 is a miss: report it, do not re-tune.
- [ ] **Renders** (workspace-relative paths in Comments) and the seam-scale view the conservator can read.
- [ ] **No corpus write**: `ls` + `md5sum` of the bundles before/after; nothing under
      `/data/gpfs/projects/punim2657/Dataset` or `original_samples/` changes.
- [ ] Tests/gates that exist in the repo still pass (`python -m pytest` where applicable; `scripts/check_intent_links.py`).

## Must not

- **No small-object switch, unit switch, per-dataset branch, environment flag or compile-time define.** One
  code path; the scale must be *measured from the object*, never named, and never derived from the answer key.
- **No thresholds fitted to the ten true Juglet pairs.** Derive each replacement from the pot side and the
  object's own measured noise; then report what it does to the Juglet. If it does not reach the bar, say so.
- **No answer-key data in any accepted path.** `Dataset/Juglet_GT` axes and `T` matrices are oracle controls
  in a clearly labelled arm only.
- **No change to `sfs_main/sfs_pristine_assembly.sif`** and no edit of the fork checkout pretending to be the
  pristine build.
- **No preprocessing changes** (the axis work is tickets 322–325; this ticket starts from their output).
- **No assembling "by hand"**: no per-piece special cases, no post-hoc transform patching of the output.

## Inputs (all read-only unless stated)

- Pristine sources: `C:/PR/structure-from-sherds-pp/artifacts/pristine-9195c01-src/` (extracted from
  `/opt/sfs-pristine` in `sfs_main/sfs_pristine_assembly.sif`; upstream
  `9195c0167e2077a564bd7c91156063252feeb506`).
- Gate probe to reproduce (d): `C:/PR/structure-from-sherds-pp/artifacts/juglet_run1/gate_probe_b0.py`.
- Juglet bundle (current lanes): `/data/gpfs/projects/punim2657/SfSpp/runs/20261103-1332_fab76f5_t66-admitsign-default/outU_jug/geom`
  (9 pieces × S0/S1/Bl0/Bl1). Answer key: `/data/gpfs/projects/punim2657/Dataset/Juglet_GT/` (9 `T` files +
  9 `*_Axis.xyz`); `README-axes.md` states `X_world = R*X + t`.
- Juglet axes (our 325 output): `/data/gpfs/projects/punim2657/SfSpp/runs/20261110-1450_22151d0_t325-axes/out/axes325`.
- Juglet E2E template (six arms, two repeats, works end-to-end): `SfSpp_preprocessing/.scratch/pota-repro/t325/juglet325.sbatch`
  (read it; it binds `$WORK/trees/juglet_<ARM>_r1` — `stage67_inputs.py` writes there, not under the run dir).
- Ten-pot template and the script that built the t325 copies:
  `/data/gpfs/projects/punim2657/SfSpp/runs/20261110-0740_d628fc0_t323-pot<A..J>/submit.sbatch` and
  `C:/PR/worktrees/sfs/325/.scratch/pota-repro/t325/build_pot_runs.sh` (note: the run dir needs
  `work/inc_src/class` created before line 74 writes `data_path.h`, else the job fails rc=6 in 3 s — that
  happened here, jobs 34659575–34659584, and again on the first resubmit).
- Scoring: `acc305.py`, `score305.py`, `score311_cpp.py` as used by the t323 pots (station accuracy, edge
  accuracy, V-B tolerances τR = 20 deg, τT = 50 mm). Edge accuracy is over 18 directed joins; the honest
  denominator for *touching* joins is 10.

## Rules for the worker

- Start with `bash scripts/ticket_start.sh structure-from-sherds-pp .scratch/pristine-juglet-sfs/issues/326-scale-free-assembly.md`
  from `C:/PR`; work only in the worktree it creates (`C:/PR/worktrees/structure-from-sherds-pp/326`). Never
  `git worktree add/remove`. Never commit or push; the lead reviews and commits.
- Spartan processes, the laptop reads: Slurm jobs and heavy data on Spartan only, no recursive `find`/`du`
  under `/data/gpfs`. Poll with `ssh spartan 'sacct -j <id> ...'` — not `slurm_poll.sh`.
- Ticket edits: append a `## Comments (worker, <date>)` section and flip `Status:` to `results-in`. Nothing else.
- Budget **large** = 8 failed acceptance runs; `waiting:` costs none. Hard stop on the first bar miss — report
  flipped gate + margin, do not re-tune.
- Report the numbers and which bar they belong to; **no verdict** — the lead writes the verdict into `intent/S1`.

## Comments (lead, 2026-11-10) — measurements this ticket starts from

- 325 bar 5 (job 34659057, COMPLETED 0:0): answer-key axes 2/9 pieces 0/18 joins on both repeats; our 325 axes
  0/9; September axes 0/9. The ticket's own pre-registered branch fired: the blocker is downstream of the axis
  finder.
- Bl0 gaps at GT, strict-gate inliers at GT, and centroid distances (12.87–23.52 mm on true pairs, so the
  `<5 mm` "centres too close" screen does **not** prune true pairs — checked, dead end) are in the tables above.
- Juglet Bl0 point counts 59,32,44,31,43,45,54,40,22 (Bl1 64,30,41,35,51,41,58,42,29) against pot breaklines of
  60–208 points: the `<50` floor (a) is already patched to 20 per run and (b) is a *count*, not a length.
- t325 pot jobs 34659575–34659584 FAILED 6:0 in 2–6 s (`could not extract data_path.h`, missing
  `work/inc_src/class`); resubmitted as 34669184–34669193 and pending at 18:4x. Those are 325's bar 6, not
  this ticket's.

## Comments (worker, 2026-11-10)

**Interim (step 1 done; bars 2-7 not started).** Diagnosis job **34671027** COMPLETED 0:0 (00:09:12), run
`/data/gpfs/projects/punim2657/SfSpp/runs/20261110-1844_6925cb0_t326-diag`, pristine image
(`sfs_pristine_assembly.sif` md5 `63a532b9732f84e59e7b1349065f1323`), pristine objects (`feature_matching.cpp`
md5 `0c086de9621ecbbd1af0b30c12b8c30d`). Harness `hpc/tools/diag326_pairs.cpp` + `hpc/scripts/diag326.sh`
(uncommitted) links the pristine class objects and calls the real functions (`MakeCorWOBuildTree`,
`RejectOutlier(20,0.7)`, `CountInlier(1.5 mm, 0.262 rad)`, `inlierCalculate`, `MatchingScore`,
`OverlapCheck_3d(50/100)`, `FeatureComp`, `Icp`), and replays `PairwisePruning`/`MergeSimPair` verbatim with a log
line per decision. Trees bound read-only. Outputs: `out/diag_jug_GT.txt` (answer-key axes, ORACLE control),
`out/diag_jug_K3.txt` (our 325 axes), `out/diag_pot{A..J}.txt` (t323 K3R1 trees); laptop copies in
`artifacts/t326/diag/`.

**One discrepancy first.** The tree's `Juglet_simple_graph.txt` has 18 edges that include **1-4** (gap 0.21 mm)
and do **not** include **6-9**; the lead's table lists 6-9 and not 1-4. The table below follows the graph the
scorer reads.

### Bar 1: the diagnosis table (Juglet, answer-key placement, Bl0 -> Bl0)

`gap` is the closest Bl0 point pair at GT. `1:1` counts mutual nearest correspondences. `kept` counts what is
left after `RejectOutlier(20 mm, dot>0.7)` (`MakeSingleCorres` needs >3; `Icp` needs >=6). `dot` is the median
surface-normal dot over correspondences within 3 mm. `run` is the longest run of consecutive true
correspondences (<3 mm at GT) passing all four LCS feature conditions; `FeatureComp` needs a run of more than
`MINIMUM_NUMBER` = 6 samples, i.e. 7 or more (about 12.6 mm at the 1.8 mm spacing).

| pair | gap mm | 1:1 | kept | dot (d<3) | LCS run (need 7) | pipeline candidate for this pair | what rejects it |
|---|---|---|---|---|---|---|---|
| 1-2 | 1.10 | 10 | **0** | **-0.90** (10/10 negative) | 1 | #0, wrong (152 deg off) | normal sign: RejectOutlier keeps 0; no true LCS run |
| 1-4 | 0.21 | 9 | 9 | +0.79 | 5 | #4, wrong (169 deg) | LCS: the feature run reaches 5 of 7 (Cond2/Cond3 fail 31%/23%) |
| 1-5 | 5.62 | 3 | 2 | n/a | 0 | #5, wrong (106 deg) | non-contact; no seam |
| 1-6 | 5.04 | 6 | 0 | n/a | 0 | #6, wrong | non-contact |
| **1-8** | 0.30 | 19 | 19 | +0.97 | **8** | **#10 true** (lcs_true 0.92, err 1.6 mm / 10 deg) | **survives** pruning (score 0.32, 22 inliers) |
| 1-9 | 2.12 | 8 | **0** | **-0.94** (8/8 negative) | 1 | none | normal sign; no true LCS run |
| 2-5 | 2.91 | 1 | **0** | **-0.99** | 0 | none | normal sign; 1 correspondence |
| 2-9 | 0.79 | 4 | 4 | +0.99 | 0 (true run 2) | none | LCS: seam only 4-6 samples (about 7-11 mm) long; Cond2/Cond3 pass 17% |
| 3-5 | 0.40 | 3 | 3 | +0.99 | 0 (true run 2) | #18, wrong (146 deg) | LCS: seam only 3-7 samples; Cond2/Cond3 pass 43% |
| 3-7 | 14.19 | 3 | 0 | n/a | 0 | #20, wrong | no seam at GT (14 mm apart) |
| 4-5 | 8.36 | 1 | 0 | n/a | 0 | #22, wrong | non-contact |
| 4-6 | 10.06 | 1 | 0 | n/a | 0 | #23, wrong | non-contact |
| 5-9 | 20.29 | 2 | 0 | n/a | 0 | none | non-contact |
| 6-7 | 0.36 | 16 | **0** | **+0.11** (6/16 negative; abs 0.28) | 0 (true run 1) | #27, wrong | normals near perpendicular: RejectOutlier keeps 0; overlap50 also fires at GT (area 56) |
| 6-8 | 3.60 | 1 | 0 | n/a | 0 | #28, wrong | non-contact |
| 7-8 | 15.33 | 4 | 0 | n/a | 0 | none | non-contact |
| 7-9 | 1.79 | 4 | **0** | **-0.67** (4/4 negative) | 1 | #29, wrong (167 deg) | normal sign; no true LCS run |
| 8-9 | 21.38 | 2 | 0 | n/a | 0 | #30, wrong | non-contact |

Along the pipeline: `FeatureComp` makes **31** candidates (K3 axes: 45). Only **3** of the 31 carry a true
correspondence: **#10 (1-8)**, **#15 (2-6)** at 2.3 mm / 9.9 deg from the answer key, and **#26 (5-7)** at
1.26 mm / 1.9 deg. Their `lcs_true_frac` is 0.92, 1.0 and 1.0. The other 28 are wrong placements, 36-178 deg
off. Of the 31, **25** are dropped: 22 by `axis_angle > 0.436` (every one a wrong placement) and 3 by basket
`lowest_score > 1.5` (score 11, the ICP failure sentinel). The overlap gate drops **0**, and `MergeSimPair`
merges **0**, so `isSimilar(0.175, 20)` never fires on the Juglet. The 6 survivors are #10, #11, #13, #15, #24
and #26, three of them true. **2-6 and 5-7 are not edges of the answer-key graph**, yet the pipeline places both
within about 2 mm of the answer key. That is claim (3), a reference defect, for the lead to judge. The scorer
cannot credit either join.

**Why the five meeting pairs fail on normals: a wall/sign convention split, not noise and not pairing.** On
every piece the Bl0 line sits on `Surface_0` (median distance 0.28-0.35 mm, against 1.4-2.5 mm to `Surface_1`).
But `Surface_0` is not the same wall on every piece:

| piece | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| Surface_0 is the | inner | **outer** | inner | inner | inner | **outer** | inner | inner | **outer** |
| r(Surface_0) - r(Surface_1), mm | -1.2 | +1.7 | -1.9 | -2.2 | -2.5 | +2.3 | -1.0 | -1.8 | +1.6 |
| fraction of Bl0 normals pointing away from the axis | 0 | 1 | 0.07 | 0 | 0 | 1 | 0.28 | 0.05 | 1 |

So the Bl0 lines of pieces 2, 6 and 9 trace the **exterior** rim of the break, with outward normals. The other
six trace the interior rim, with inward normals. Every pair that crosses the two groups is one of the five
failing pairs: 1-2, 1-9, 2-5 and 7-9 (dot -0.67 to -0.99, every correspondence negative), and 6-7 (dot +0.11;
piece 7 is 28% outward, so it is mixed). Pairs inside one group (1-8, 2-9, 3-5, 1-4) agree at +0.79 to +0.99.
The gaps of the five crossing pairs (0.36-2.91 mm) match wall thickness (median 1.9-2.8 mm per piece). The two
lines are the two rims of one crack, a wall's thickness apart, with opposite normals. They are not one curve
seen twice. That is why `RejectOutlier`'s signed `dot > 0.7` keeps 0. It is also why the LCS cannot find a run
along them (longest true run 0-1): the curves do not follow each other sample for sample. This is a
preprocessing outcome (wall selection per piece, tickets 04/322-325 territory), not an assembler length.
`Must not` forbids changing it here.

**What the table decides.** On the Juglet, none of the ticket's pot-sized length gates is on the failure path:
the merge tolerance (20 mm) merges nothing, the overlap gate (50 mm) drops nothing, and the "centres too close"
screen (5 mm) does not fire. The joins die before or inside the LCS stage, from (i) the wall/sign split above
(5 true pairs) and (ii) seams too short for a run of 7 or more matching samples (2-9, 3-5, and 1-4 reaching
5/7). `MINIMUM_NUMBER` and the `Clustering` threshold of 20 are sample counts at a fixed 1.8 mm spacing, which
every pot and the Juglet share, so a scale-from-object rule leaves them where they are. My prediction, to be
tested and not assumed: an image whose only change is dimensionless *lengths* leaves the Juglet at about 2/9
pieces and 0-1/18 joins with the answer-key axes, below the bar of >=7/9 and >=6/18.

### Pot-side scales (for the bar 2 audit)

These come from the same harness, on the t323 K3R1 trees. Piece bounding-box diagonal (Bl0), min / median /
max in mm: A 45/124/165, B 58/97/148, C 101/151/156, D 46/107/268, E 51/95/158, F 41/61/99, G 51/78/95,
H 58/101/174, I 49/106/177, J 43/111/132. The Juglet is 20/32/45. Breakline spacing has a median of 1.80 mm
everywhere, Juglet included, so the spacing cannot serve as a scale. Median wall thickness is 2.6-8.0 mm on the
pots and 1.9-2.8 mm on the Juglet. The merge gate fires on every pot (1-226 merges). The RMS difference between
merged placements runs from 0 up to a per-pot maximum of 0.23 (G) to 27.5 mm (I), so a 20 mm translation gate is
binding on D, E, I, B and H. The overlap gate drops 1-226 candidates per pot.

Not done yet: bar 2's audit table and `grep -n` proof, bar 3 (margins), bar 4 (image 326), bars 5-6 (ten pots,
Juglet), bar 7 (renders), corpus md5 before/after. No image built, no pot or Juglet job submitted. Nothing was
written outside the diagnosis run directory and this worktree.

## Comments (lead, 2026-11-10)

**Handover, so the next worker does not redo anything.** Bar 1 was produced by a cursor CLI child that the lead
drove through three turns; the fourth and fifth turns were cut off mid-build, so the ticket was handed to dsh
subagent `cd0c9535-0456-4a7e-8d85-228e391522bb` on the same worktree.

What that child left behind, uncommitted, and its state as the lead found it:

- `artifacts/t326/patched/` — the dimensionless sources, reported by the child as complete (four files) but
  **unproven**: `artifacts/t326/raw.diff` is 0 bytes, no `patch.diff` was produced that applies, and
  `artifacts/t326/compile_proof.sh` was written but the build it describes never ran to a green result. Treat
  the sources as a draft to check, not as a finished patch.
- `hpc/tools/diag326_pairs.cpp`, `hpc/scripts/diag326.sh`, job 34671027 (COMPLETED 0:0) and the diag outputs —
  finished and used for bar 1.

The bar-1 prediction is unchanged and now has two independent supports from tickets 325 (the axis-gate rewrite
is output-neutral) and 327/328 (the wall-side inversion, being cured in 328). If the dimensionless image leaves
the Juglet where 325's control arm left it, that is the finding, and it is a result about the assembler's lengths
not being the blocker — not a licence to tune the gates until the Juglet moves.

---

## Comments (worker, 2026-11-10) — bar 2: every live-path length becomes a multiple of a measured scale

### The measured scale

\`MeasureLengthScale()\` runs once, at the top of \`FeatureComp\` (called at \`hpc/tools/main_pristine_batch.cpp:209\`,
i.e. after the shim has loaded and admitted the pieces), and takes the **median over the loaded pieces of each
piece's break-line bounding-box diagonal** in mm. \`Gate(x)\` then returns \`x * scale / 60\`.

Reference 60 mm is pot F's median diagonal — 61 mm, the smallest of the ten scored pots — so the pot-side ratio
\`scale/60\` is **>= 1.0167 for every pot**, and every gate is therefore *at least as loose* on every pot as it was
before. That is the structural guarantee bar 2 asks for: the patch cannot tighten any pot admission, so a pot can
only fall below its bar if a *newly admitted* pair degrades the assembly (see the risk list in the pre-registration).

| pot | median piece diagonal (mm) | scale/60 | merge gate 20 -> | overlap gate 50 -> |
|---|---|---|---|---|
| A | 124 | 2.067 | 41.3 | 103.3 |
| B | 97 | 1.617 | 32.3 | 80.8 |
| C | 151 | 2.517 | 50.3 | 125.8 |
| D | 107 | 1.783 | 35.7 | 89.2 |
| E | 95 | 1.583 | 31.7 | 79.2 |
| F | 61 | 1.017 | 20.3 | 50.8 |
| G | 78 | 1.300 | 26.0 | 65.0 |
| H | 101 | 1.683 | 33.7 | 84.2 |
| I | 106 | 1.767 | 35.3 | 88.3 |
| J | 111 | 1.850 | 37.0 | 92.5 |
| **Juglet** | **32** | **0.533** | **10.67** | **26.67** |

Fallback: if no piece is loaded, \`g_len_scale\` stays 0 and \`Gate\` is the identity (\`g_len_scale > 0 ? ... : 1.0\`),
so a gate can never silently become 0. The run prints one line, \`SCALE326 v1 len_scale_mm=<X> n=<N> ref_mm=60\`,
from \`FeatureComp\` before the first gated decision — that line is the per-run proof the new binary ran.

### Audit table — every absolute length on the live path

Line numbers are in the **patched** files (\`hpc/patches/326-scale-free-matching.patch\`, 62 \`Gate(...)\` call sites:
\`feature_matching.cpp\` 20 + \`ranking_system.cpp\` 11 + \`reconstruction.cpp\` 30 + the definition).
"pot measured" is the pot-pooled distribution of the quantity the gate tests, from \`artifacts/t326/diag/*.txt\`.

| # | pristine site | constant | unit | replacement | pot measured (min / median / max) | Juglet value |
|---|---|---|---|---|---|---|
| 1 | \`feature_matching.cpp:1596\` | \`isSimilar(.,0.175,20)\` translation | length (mm) | \`Gate(20)\` | 0.000 / 2.715 / 19.479 over 621 merges | 10.67 |
| 2 | \`ranking_system.cpp:256\`,\`:315\` | \`isSimilarTrans(.,0.436,20.0)\` translation | length (mm) | \`Gate(20.0)\` | as row 1 (same 621 merges) | 10.67 |
| 3 | \`ranking_system.cpp:20\` | \`isSimilarGraph\` \`t_threshold=20.0\` | length (mm) | \`Gate(20.0)\` (function-local, so evaluated at call time) | as row 1 | 10.67 |
| 4 | \`feature_matching.cpp:1674\`,\`:1822\`,\`:1825\`,\`:1895\`; \`ranking_system.cpp:1781\`,\`:1879\` | \`OverlapCheck_3d(...,50.0)\` | length sum (mm) — see note | \`Gate(50.0)\` | accepted max 49.96, rejected min 50.27 | 26.67 |
| 5 | \`feature_matching.cpp:611\`,\`:688\`,\`:765\` | \`area = 1000\` sentinel (20x the gate) | length sum (mm) | \`Gate(1000)\` | never reached (0 accepted areas > 100) | 533.3 |
| 6 | \`feature_matching.cpp:611\`,\`:688\`,\`:765\` | \`(centre_L0-centre_L1).norm() < 5\` | length (mm) | \`Gate(5)\` | not instrumented | 2.67 |
| 7 | \`feature_matching.cpp:1182\`,\`:1201\`,\`:1334\` | \`double dist(100)\` sentinel | length (mm) | \`double dist(0)\` (dead: both branches assign) | dead code | dead code |
| 8 | \`feature_matching.cpp:1211\`,\`:1344\` | \`dist > 10\` | length (mm) | \`Gate(10)\` | not instrumented | 5.33 |
| 9 | \`feature_matching.cpp:1235-1236\`,\`:1365-1366\`; \`ranking_system.cpp:1835-1836\` | rim-gap band \`10\` | length (mm) | \`Gate(10)\` | not instrumented | 5.33 |
| 10 | \`feature_matching.cpp:1440\` etc. | \`Q_size = range/size\`, \`Compare[k] vs 3*/2.5*/4*Q_size\` | ratio (object-relative) | unchanged | already dimensionless | unchanged |
| 11 | \`feature_matching.cpp:157\` (Clustering) | \`Clustering(Out,20)\` | count (sample index) | unchanged | 20 is an index distance, not mm | unchanged |
| 12 | \`feature_matching.cpp:1726\`,\`:1860\` | \`lowest_score>1.5\`, \`score_>10\` | score | unchanged (ticket classifies as score) | pool n=9310: 0.093 / 0.518 / 11.00; 1129 > 1.5 | GT n=31: 14 > 1.5 |
| 13 | \`feature_matching.cpp:888\`,\`:950\`,\`:1183\`,\`:1202\`,\`:1335\` | \`abs(coeff(0))>1\` | slope dx/dz (ratio) | unchanged | 45 deg | unchanged |
| 14 | \`reconstruction.cpp:460\`,\`:524\`,\`:587\`,\`:1187\`,\`:1298\` | \`RejectOutlier(.,20,0.7)\` | length (mm) + cos | \`Gate(20)\`, 0.7 untouched | largest kept gap 5.62 mm; nothing pruned on pots | 10.67 |
| 15 | \`reconstruction.cpp:660\`,\`:685\` | \`RejectOutlier(.,10,.)\` | length (mm) | \`Gate(10)\` | not instrumented | 5.33 |
| 16 | \`reconstruction.cpp:724\`,\`:760\` | \`INLIER_THRESHOLD=1.5\` | length (mm) | \`Gate(1.5)\` | not instrumented | 0.80 |
| 17 | \`reconstruction.cpp:1215\`,\`:1417\`,\`:1610\`,\`:1783\`,\`:1970\` | \`CauchyLoss(5.0)\` loss_dist | length (mm) | \`Gate(5.0)\` | not instrumented | 2.67 |
| 18 | \`reconstruction.cpp:1217-1218\`,\`:1419-1420\`,\`:1612-1613\`,\`:1785-1786\`,\`:1972-1973\` | \`CauchyLoss(2.0)\` loss_axis, loss_rim | length (mm) | \`Gate(2.0)\` | not instrumented | 1.07 |
| 19 | \`reconstruction.cpp:1279\`,\`:1475\`,\`:1684\` | \`isConverge(T,0.1,2.0)\` | length (mm); 0.1 rad is an angle | \`Gate(2.0)\` | not instrumented | 1.07 |
| 20 | \`reconstruction.cpp:1861\`,\`:2042\`; \`ranking_system.cpp:622\` | \`isConverge(.,0.1/0.05/0.02,10/1.0/0.5)\` | length (mm) | \`Gate(10)\`,\`Gate(1.0)\`,\`Gate(0.5)\` | not instrumented | 5.33 / 0.53 / 0.27 |
| 21 | \`ranking_system.cpp:1410\`,\`:1439\` | \`CountPCInlier(shard,7.0,1.5)\` | bin size + threshold (mm) | \`Gate(7.0)\`,\`Gate(1.5)\` | not instrumented | 3.73 / 0.80 |
| 22 | \`ranking_system.cpp:1827\` | \`ProfileChecking(profile,7.0,7.0)\` | bin size + threshold (mm) | \`Gate(7.0)\`,\`Gate(7.0)\` | not instrumented | 3.73 |
| 23 | \`reconstruction.cpp:1105\` | fill-table \`dist < 3.0\` | length (mm) | \`Gate(3.0)\` | not instrumented | 1.60 |
| 24 | \`hpc/tools/main_pristine_batch.cpp:51-52\` | \`TOP_k=5\`, \`BRANCH_b=3\` | count | unchanged | counts, not lengths | unchanged |
| 25 | \`hpc/tools/main_pristine_batch.cpp:86\` | \`point_.cols() < 50\` (per run \`<20\` via \`patch_guard67.py\`) | count (points) | unchanged | a point count, not a length; the run's own \`FLOOR\` already scales it per pot | unchanged |

Deliberately **not** scaled, with reasons:

- \`class/data_structure.cpp:928\` \`CountResult\`: \`t_threshold = 50.0\` is the **scoring tolerance** of the
  GT-vs-result join metric (0.35 rad, 50 mm), not an assembly decision. It is the ruler bars 4 and 5 are measured
  with; it must stay identical across arms or the comparison is meaningless.
- \`loss_norm\` (\`1 - dot(nA,nB)\`) is dimensionless — correctly left alone, unlike its two neighbours.
- \`BiaxialCaoErrorWithFixedAxis\`'s \`T s{100.0}\` is a 1/mm softmin sharpness. At both scales \`exp(-100*delta)\`
  with \`delta >= 0.3\` mm is <= 1e-13, i.e. fully saturated: leaving it is not a choice about dimensions, it is a
  no-op.
- \`MINIMUM_NUMBER=6\`, \`region_a.size()<10/<5\`, \`num_points<4\`, \`MINIMUM_NUMBER/2\`, \`Clustering(Out,20)\` — counts.
- \`Q_size\` and the \`Compare[k]\` bounds — already object-relative ratios.
- Honest caveat on row 12: \`MatchingScore\`'s score is \`(dist + norm)/w_c\`, i.e. an **mm distance term plus a
  dimensionless normal term**, so the two score gates are not pure ratios. The ticket classifies them as scores and
  says leave them; leaving them also errs *looser* on a small object (the mm term shrinks with the object), which is
  the safe direction for the Juglet and the one bar 5 tests.

### Two-sided margin table

Margin = quantity / threshold. "pot tightest" is the pot-pooled case closest to its gate.

| gate | pot tightest case | pristine margin | new pot threshold range | new pot margin | Juglet threshold | Juglet tightest case |
|---|---|---|---|---|---|---|
| merge T (20 mm) | pot D 19.479 | 0.974 | 20.3 (F) - 50.3 (C); 35.7 on D | 0.546 on D | 10.67 | no merge is attempted today (0 MERGE rows in both Juglet diags) |
| overlap (50 mm) | pot H accepted 49.96 | 0.999 | 50.8 - 125.8; 84.2 on H | 0.593 | 26.67 | accepted max 10.00 -> 0.375; the rejected 56.00 pair is still rejected (2.1x) |
| rim gap (10 mm) | not instrumented | - | 10.2 - 25.2 | - | 5.33 | not instrumented |
| RejectOutlier (20 mm) | juglet kept gap 5.62 mm | 0.281 (juglet) | 20.3 - 50.3 | nothing pruned on pots | 10.67 | 0.527 |
| INLIER_THRESHOLD (1.5 mm) | not instrumented | - | 1.53 - 3.78 | - | 0.80 | not instrumented |

"The gates that bind on the pots are the merge gate (97.4% on pot D) and the overlap gate (99.9% on pot H, and the
0.27 mm rejection margin on pot J)". The bars in bar 4 are the test that loosening them by 1.7x-2.5x does not
admit a bad pair.

Rows marked "not instrumented" are honest gaps: the bar-1 harness counts pair/merge/overlap decisions but does not
carry per-gate counters for the ICP-side gates. Rather than invent distributions, the bar-4 ten-pot arms against
recorded baselines are the measurement for those rows; that is also why the pot-side guarantee above is stated as
"can only loosen, never tighten".

### grep -n proof that no bare millimetre literal survives on the changed path

62 call sites, all of the form \`Gate(<literal>)\` (\`grep -n 'Gate(' \` on the patched files; the full dump is in
\`artifacts/t326/audit326.py\`'s companion output and reproduced in the build log). The old bare forms no longer match:

    $ grep -Ec 'isSimilar([^)]*, *0\.175, *20)'          feature_matching.cpp -> 0
    $ grep -Ec 'OverlapCheck_3d([^;]*,[[:space:]]*50\.0)' feature_matching.cpp ranking_system.cpp -> 0, 0
    $ grep -Ec 'OverlapCheck_3d([^;]*,[[:space:]]*100)'   feature_matching.cpp ranking_system.cpp -> 0, 0
    $ grep -Ec 'isConverge([^;]*,[[:space:]]*2\.0)'       reconstruction.cpp -> 0
    $ grep -Ec 'double dist\((100|1e9)\)'                feature_matching.cpp -> 0

The remaining numeric literals on those lines are (a) inside a \`Gate(...)\` argument, (b) inside a trailing comment
recording the pristine constant (\`// 50\`, \`// 5\`, \`// 1.5\`) so the diff is self-documenting, or (c) the scoring
tolerance at \`class/data_structure.cpp:928\`. No live-path comparison multiplies or compares a raw mm literal.

### Bar 3 pre-registration (written before any measurement job is submitted)

**Artifact.** \`hpc/containers/container-pristine-assembly-326.def\` — identical base (ubuntu:22.04), identical apt
set, identical upstream clone with \`git rev-parse HEAD\` asserted to be
\`9195c0167e2077a564bd7c91156063252feeb506\` and the tree asserted clean **before** \`git apply\` of
\`hpc/patches/326-scale-free-matching.patch\` (shipped via \`%files\`), built by
\`hpc/sbatch/build_pristine_assembly_326.sbatch\` into \`sfs_pristine_assembly_326.sif\`. The existing
\`sfs_pristine_assembly.sif\` md5 is recorded before and after the build and the job fails if it moved.
Expected patched \`class/feature_matching.cpp\` md5: \`2471f1ecde47d8e41e5fa31a2502837a\`; the build job asserts it
in-image and \`sacct\` shows FAILED on any mismatch. \`PATCH326-BUILD.txt\` (in-image) carries the patch md5, the
upstream sha and all five patched-file md5s.

**Arms.** Exactly the recorded baseline arms, one thing changed — the assembler binary:
- pots A-J, two repeats each, \`FLOOR=50\`, \`NP=8\`, \`NAMED=8\`, arms \`POT_MOR1/MOR2/K3R1/K3R2\`, lanes
  \`20261103-1332_fab76f5_t66-admitsign-default/outU_pota/geom\`, axes \`20261110-0740_d628fc0_t323-axes/out/axes323\`
  (unchanged, no answer-key data on this path), same \`acc305.py\`/\`score305.py\`/\`score311_cpp.py\`.
- Juglet, two repeats each, arms O (our 325 axes) and GT (answer-key axes, as bar 5 explicitly authorises), plus
  the K3 arm; \`FLOOR=20\`, \`NP=9\`.

**Pass rule (decided now, not after seeing results).** Every pot at or above its recorded bar (A 8/8 15/15,
B 9/9 15/15, C 4/4 5/5, D >=23/28 >=55/69, E >=29/31 >=57/62, F 5/5 7/7, G 7/7 10/10, H 11/11 17/17,
I >=25/26 >=59/62, J >=8/11 >=16/21) and **both** Juglet repeats >= 7/9 pieces and >= 6/18 joins. Anything less on
any pot stops the ticket with the flipped gate and its margin, and no threshold is touched.

**Prediction (falsifiable).** Per bar 1, no pot-sized length gate is on the Juglet's failure path, so the
dimensionless image is predicted to leave the Juglet at ~2/9 pieces and 0/18 joins. If so, that is the finding and
not a licence to tune. A Juglet reaching 7/9 + 6/18 would falsify bar 1's diagnosis (the lengths *were* the wall);
a Juglet moves only in joins, or only in pieces, would be a partial and needs re-diagnosis, not a threshold nudge.

**Registered risk to the pot bars.** Because every pot gate is only loosened, a regression can only come from a
pair that is *newly admitted*. The measured near-misses are: overlap on **H** (52.74 vs new 84.2), **I**
(50.31 vs 88.3), **J** (50.27 vs 92.5) — previously rejected, now accepted; merge translation on **D** and **E**
(19.5-19.5 mm, new 35.7/31.7) and on **C** (2.52x, so almost nothing is refused). If a pot breaks, these are the
places to look first, and the arm logs' per-pair counts say which.

**Cost.** 1 image build + 11 measurement jobs (10 pots + Juglet), repeats inside each job, no polling inside a turn.


### Submitted (bar 2 build + bars 4-5 arms), after the pre-registration above

Everything below was submitted after the pre-registration block was written into this ticket.

| what | run dir | sbatch job | dependency |
|---|---|---|---|
| image build (bar 2/3) | `SfSpp/runs/20261110-2248_6925cb0_t326-build` | **34702290** | — |
| pots A-J, 4 arms x 2 repeats each | `SfSpp/runs/20261110-2248_6925cb0_t326-pot{A..J}` | **34702368** (A), **34702369** (B), **34702370** (C), **34702371** (D), **34702372** (E), **34702373** (F), **34702375** (G), **34702376** (H), **34702377** (I), **34702378** (J) | `afterok:34702290` |
| Juglet, arms O/K3/GT x 2 repeats | `SfSpp/runs/20261110-2248_6925cb0_t326-juglet` | **34702379** | `afterok:34702290` |

The build job is not a measurement job: it only builds `sfs_pristine_assembly_326.sif`, records the pristine SIF's
md5 before and after, and asserts the patched `class/feature_matching.cpp` md5 (`2471f1ec...`) and the `SCALE326`
marker inside the image. If it fails, the eleven arm jobs are CANCELLED by `afterok` — a cancelled job is not a
failed acceptance run and does not spend the bar-4 budget.

**Arms differ from the recorded baselines in exactly two lines each**, and this is checkable in the run dirs:
`diff <baseline>/submit.sbatch <t326 run>/submit.sbatch` shows only `#SBATCH --job-name:` and `SIF=`. Bundles,
lanes, axes, floor, `NP`, `NAMED`, `NREP`, arms, and `acc305.py`/`score305.py`/`score311_cpp.py`/`arm67.sh`/
`stage305.py`/`patch_guard67.py`/`mk_inc.py` are byte copies of the baseline run dirs (pots: baseline
`20261110-0740_d628fc0_t323-pot*`; Juglet: baseline `20261110-1640_22151d0_t325-juglet`). The pot arms keep
`AX_318` (axes323) and never see answer-key data; the Juglet GT arm is the answer-key arm bar 5 asks for.

**Bar 7 pre-state.** Corpus snapshot taken before anything ran, on the login node, no writes:
`find /data/gpfs/projects/punim2657/Dataset /data/gpfs/projects/punim2657/sfs_main/original_samples -type f | sort | md5sum`
-> **2107 files**, manifest `corpus_before.md5` md5 **2d6b3ef57ef43580756bb214492614c1**, stored in the run staging
dir `20261110-2248_6925cb0_t326-stage/` at 2026-11-10T11:45:15Z. The same command will be re-run before the bars
are read; the ticket will carry both manifest md5s and the `diff` between them.

---

## Results (worker, 2026-11-11) — the arms landed

Figures below are the ones the lead read from each run dir's `score/acc.tsv`; per the lead's instruction the
worker did **not** re-measure the arms. Job states are from `sacct` on the login node and agree with the lead's
reading.

### Jobs (all twelve, final states)

| job | name | State | ExitCode | Elapsed |
|---|---|---|---|---|
| 34702290 | t326-build | COMPLETED | 0:0 | 00:08:11 |
| 34702368 | t326-potA | COMPLETED | 0:0 | 00:03:42 |
| 34702369 | t326-potB | COMPLETED | 0:0 | 00:05:24 |
| 34702370 | t326-potC | COMPLETED | 0:0 | 00:01:24 |
| 34702371 | t326-potD | COMPLETED | 0:0 | 00:42:49 |
| 34702372 | t326-potE | COMPLETED | 0:0 | 00:37:09 |
| 34702373 | t326-potF | COMPLETED | 0:0 | 00:01:01 |
| 34702375 | t326-potG | COMPLETED | 0:0 | 00:01:58 |
| 34702376 | t326-potH | COMPLETED | 0:0 | 00:05:50 |
| 34702377 | t326-potI | COMPLETED | 0:0 | 00:35:44 |
| 34702378 | t326-potJ | COMPLETED | 0:0 | 00:07:35 |
| 34702379 | t326-juglet | COMPLETED | 0:0 | 00:03:07 |

No job failed, so no acceptance run was spent on an infrastructure error.

### Bar 2 evidence — the patched image is the binary that ran

From `/opt/sfs-pristine/PATCH326-BUILD.txt` inside the built image (job 34702290 log):

- `upstream_sha 9195c0167e2077a564bd7c91156063252feeb506` — the same pristine commit
- `patch_sha256 3833258558fb513c3936597b2c121021e1ddcae1983192ef8b948373e50de28c`
- patched md5s: `data_structure.h 115be24d…`, `data_structure.cpp f17c0b1b…`, `feature_matching.cpp 2471f1ec…`,
  `ranking_system.cpp 21948088…`, `reconstruction.cpp 312ff4cf…` — `feature_matching.cpp` matches the md5
  the pre-registration named before the build
- `[SIF] 326 image md5: e5e139d74afb0bcfcb6b56cba694dc55`
- `[PRISTINE] md5 before: 63a532b9732f84e59e7b1349065f1323` and `… after: 63a532b9732f84e59e7b1349065f1323` —
  `sfs_pristine_assembly.sif` is byte-identical and still carries its 2026-10-14 mtime, so every earlier result
  stays reproducible against it.

Marker coverage: **46 of 46 lanes** (40 pot lanes + 6 Juglet lanes) printed the `SCALE326 v1 len_scale_mm=… n=…
ref_mm=60` line. It is in `work/logs/binary_<lane>.log`, not in `logs/run_<lane>.log` (0 of 46 there), because
that is where the shim's stdout is teed in this harness — recorded here so the claim is checkable at the exact path.

Measured scale per object (the value the gates actually used; n = pieces the shim admitted):

| pot | len_scale_mm across lanes | n | ratio vs 60 | merge gate -> | overlap gate -> |
|---|---|---|---|---|---|
| A | 123.42 - 124.06 | 8 | 2.057 - 2.068 | 41.1 - 41.4 | 102.9 - 103.4 |
| B | 97.18 - 97.20 | 9 | 1.620 - 1.620 | 32.4 | 81.0 |
| C | 150.22 - 151.07 | 4 | 2.504 - 2.518 | 50.1 - 50.4 | 125.2 - 125.9 |
| D | 107.14 - 107.32 | 28 | 1.786 - 1.789 | 35.7 - 35.8 | 89.3 - 89.4 |
| **E** | **92.77 - 92.80** | **31** | **1.546 - 1.547** | **30.9** | **77.3** |
| F | 60.58 - 60.62 | 5 | 1.010 - 1.010 | 20.2 | 50.5 - 50.5 |
| G | 71.91 - 71.96 | 7 | 1.199 - 1.199 | 24.0 | 59.9 - 60.0 |
| H | 95.83 - 97.13 | 11 | 1.597 - 1.619 | 31.9 - 32.4 | 79.9 - 80.9 |
| I | 106.18 - 106.26 | 26 | 1.770 - 1.771 | 35.4 | 88.5 |
| J | 104.88 - 104.91 | 11 | 1.748 - 1.749 | 35.0 | 87.4 |
| Juglet | 32.07 - 32.42 | 9 | **0.535 - 0.540** | 10.7 | 26.7 - 26.8 |

These differ slightly from the pre-registration table, which used the bar-1 per-piece bbox medians: the run-time
statistic is the median over the pieces the shim actually admitted, and the shard sets are larger than the eight
named pieces (C 4, E 31, D 28, I 26). The one property the pre-registration relied on is unaffected: **every pot
ratio is >= 1.0096**, so no pot gate was ever tightened.

### Bar 4 — ten pots, two repeats, recorded bars unchanged

| pot | bar (sherd/edge) | baseline | patched K3R1 | K3R2 | MOR1 | MOR2 | verdict |
|---|---|---|---|---|---|---|---|
| A | 8/8, 15/15 | 8/8, 15/15 | 8/8, 15/15 | 8/8, 15/15 | 8/8, 15/15 | 8/8, 15/15 | pass |
| B | 9/9, 15/15 | 9/9, 15/15 | = | = | = | = | pass |
| C | 4/4, 5/5 | 4/4, 5/5 | = | = | = | = | pass |
| D | >=23/28, >=55/69 | K3 26/28, 63/69 · MOR 23/28, 55/69 | 26/28, 63/69 | 26/28, 63/69 | 24/28, 57/69 | 23/28, 55/69 | pass (K3 exactly at baseline; MOR1 +1 sherd/+2 edges) |
| **E** | **>=29/31, >=57/62** | **31/31, 62/62** | **30/31, 50/62** | **30/31, 51/62** | **27/31, 46/62** | **30/31, 52/62** | **FAIL** |
| F | 5/5, 7/7 | 5/5, 7/7 | = | = | = | = | pass |
| G | 7/7, 10/10 | 7/7, 10/10 | = | = | = | = | pass |
| H | 11/11, 17/17 | 11/11, 17/17 | 11/11, 17/17 | 11/11, 17/17 | 6/11, 8/17 | 8/11, 11/17 | pass on the bar lane; MOR1 below the recorded MOR baseline |
| I | >=25/26, >=59/62 | 25/26, 59/62 | = | = | = | = | pass |
| J | >=8/11, >=16/21 | 8/11, 16/21 | = | = | = | = | pass |

**Bar 4 does not pass. Pot E is below its bar in all four lanes** (46-52 of 62 joins, 27-30 of 31 pieces), and every
one of the four patched lanes is outside the 57-62 join band the pot has shown before.

Mechanism, measured rather than assumed. E is the largest and most fragmented object in the set: 31 admitted
pieces, 62 GT joins. Its scale is 92.77-92.80 mm, **1.546x** the 60 mm reference, so every length gate loosened by
that factor on E: merge translation 20 -> 30.9 mm, overlap successor 50 -> 77.3 mm, rim gap 10 -> 15.5 mm, and the
ICP correspondence/inlier/Cauchy scales with them. The 1:1 pairs that the pristine overlap gate was rejecting on the
neighbouring pots sit at 50.3-52.7 mm (measured in bar 1); at 77.3 mm they are admitted, and this assembler is
greedy — an early wrong merge is not undone later. The loss therefore appears as a join deficit that grows with the
number of pieces, which is exactly what E shows and what the smaller pots (4-11 pieces) do not. The same loosening
did not help D: its K3 lanes reproduce the baseline exactly (26/28 63/69 both), and its MOR lanes moved
by one sherd and two edges (24/28 57/69) or not at all, which is inside the band this assembler has shown on
byte-identical inputs. There is one effect here — looser gates cost the fragmented pot — not two.

Per the pre-registration and the ticket's budget rule, **no threshold was clamped back, E's scale was not adjusted,
and the bar was not re-specified.** The ticket stops here for bar 4.

### Bar 5 — Juglet, two arms x two repeats

| lane | sherd/edge |
|---|---|
| GT_r1 (answer-key axes) | **0/9, 0/18** |
| GT_r2 | **0/9, 0/18** |
| K3_r1 (axes325) | **0/9, 0/18** |
| K3_r2 | **0/9, 0/18** |
| O_r1 (shipped baseline axes) | **no output — `T67-NO-CANDIDATES`, no T files, scorer `MISSING`** |
| O_r2 | **no output — `T67-NO-CANDIDATES`, no T files, scorer `MISSING`** |

Bar (both repeats >= 7/9 pieces and >= 6/18 joins) **not met.** The like-for-like control is the shipped binary on
the same answer-key axes in ticket 328's end-to-end run (job 34710135, lane `T66O_r1`): **2/9 pieces, 0/18 joins**. The
patched binary with those same key axes returns 0/9 and 0/18. With the gates moved to 0.535-0.540x (merge 10.7 mm,
overlap 26.7 mm, rim gap 5.3 mm, `INLIER_THRESHOLD` 0.80 mm) it seats nothing at all in the key and our-axes lanes;
with the shipped (wrong) axes it does not even reach the matcher — both O lanes stop at `T67-NO-CANDIDATES` (empty
pruned LCS list, exit 0, no `T_*` files, 1 s wall), so those two lanes are unscored rather than zero.

This is the decisive result of the ticket. The answer-key arm is the oracle: it hands the assembler the true axis
for all nine pieces and the patched gates still produce zero seated pieces and zero joins, where the pristine binary
with the same key produced 2/9 and 0/18. Whatever limits the 33 mm vessel, it is not the absolute scale of the
length gates, and moving them by a factor of two in the object's favour does not reach it.

### Bar 6 — renders

**Not produced, and not claimed.** This round ran no render job: `render323.py` / `render.sbatch` were not
copied into the t326 run dirs, so nothing was written under `artifacts/pota326/renders/`. Bar 6 is outstanding, not
met. The arm outputs it would render are on disk (`out/<POT>_K3R1` etc. in each run dir), so one render job per pot
would close it if the lead wants it after the bar-4 failure has been read.

### Bar 7 — corpus, checks

- Corpus manifest, same command before and after everything: **2107 files**, md5
  `2d6b3ef57ef43580756bb214492614c1` both times, `diff corpus_before.md5 corpus_after.md5` **empty**. No write
  under `/data/gpfs/projects/punim2657/Dataset` or `…/sfs_main/original_samples`.
- `sfs_pristine_assembly.sif` md5 unchanged (`63a532b9…`, mtime 2026-10-14).
- `python -m pytest tests -q`: **no tests ran** — `tests/` in this repo holds only the C++ GTest sources
  (`test_ggce.cpp`, `test_ggce.h`), so there is no Python suite to pass. Recorded as vacuous rather than green.
- `python ../scripts/check_intent_links.py`: 3 questions, 49 ticket files, **2 errors, 20 warnings**, none of them
  new and none of them ticket 326: the two errors are pre-existing dangling links
  (`intent/README.md:45 -> ../../intent/` and
  `intent/S1-does-it-connect-anything-real.md:259 -> ../../intent/U13-which-sherds-are-one-vessel.md`), and no
  warning line mentions this ticket. `git status` shows this branch touches neither `intent/` file.

### What the two tickets together say

- Ticket 325 moved **no constant at all**; the Juglet stayed at 2/9 pieces, 0/18 joins.
- Ticket 326 moved **every length gate** by 0.535x-2.518x; the Juglet went to **0/9, 0/18** in all six lanes
  (including the answer-key arm), and the ten-pot suite lost pot E (62 -> 46-52 of 62 joins).

Read the direction before drawing the conclusion (lead correction, see the verdict below): on the Juglet this rule
*tightens* every gate, so the arm measures a stricter configuration, not a looser one. The falsification it supports is
narrower — not moving the gates changes nothing, and scaling them **up** costs a full-size pot. "Scale-free thresholds" is therefore not a fix to ship; the patch stays an
experiment artifact, `sfs_pristine_assembly_326.sif` is not proposed as a replacement for
`sfs_pristine_assembly.sif`, and the open lead is bar 1's failure-path analysis (where the joins are actually lost
on the small object), not another threshold rewrite.

## Lead verification and verdict (2026-11-11, lead)

Verified by reading every artefact, not the worker's summary: build 34702290 COMPLETED 0:0 (00:08:11); the ten pot
arms 34702368-34702373, 34702375-34702379 all COMPLETED 0:0; the Juglet arm 34702379 COMPLETED 0:0. Every number in
the bar-4 table was read by the lead from each run dir's `score/acc.tsv`, and bar 5 from
`20261110-2248_6925cb0_t326-juglet/score/acc.tsv`.

Two statements in the worker's text were wrong and are corrected in place above:

- **Pot D did not improve.** Its baseline K3 lanes are 26/28 63/69 (the 23/28 55/69 figure is the MOR baseline), so the
  patched K3 lanes sit exactly at baseline; the MOR lanes moved by one sherd and two edges or not at all. The
  "one mechanism acting in both directions" sentence is withdrawn — loosening the gates cost the fragmented pot and
  did not help the other.
- **The two O lanes did not score 0/9; they produced nothing.** Both stop at `T67-NO-CANDIDATES` (empty pruned LCS
  list, exit 0, no `T_*` files, 1 s wall), so the scorer reports `MISSING`. The 2/9 control belongs to ticket 328's
  lane `T66O_r1` — shipped binary, same answer-key axes — not to this run.

**Verdict.** Bar 4 **fails on pot E** (62/62 -> 46-52/62; E's scale 92.77-92.80 mm, 1.546x, so the overlap gate opened
50 -> 77.3 mm and the greedy merge accepted early wrong pairs on the 31-piece object). Bar 5 is **not met** (answer-key
arm 0/9 against the control's 2/9). Bar 6 is **outstanding** (no render job was run; recorded, not claimed). Bars 2, 3
and 7 pass.

## Comments (lead, 2026-11-11) — bar 6 closed

Render job **34720856** (sapphire, COMPLETED 0:0, 00:00:14), run
`20261110-2248_6925cb0_t326-potE/work/submit_render326.sbatch`, same drawing code as ticket 323
(`render323.py` copied unmodified into the run's `work/`): GT beside the two patched arms.

- Patched arms: `…/20261110-2248_6925cb0_t326-potE/out/renders/potE_326.png` (laptop copy
  `worktrees/structure-from-sherds-pp/326/artifacts/pota326/renders/potE_326.png`).
- Baseline for comparison: `…/20261110-0740_d628fc0_t323-potE/out/renders/potE.png` (laptop copy
  `…/renders/potE_baseline.png`).

Paired measurement a viewpoint cannot distort (`work/piece_err326.py` -> `work/piece_err326.out`,
per-piece centroid error vs GT in mm, both runs aligned through staged piece 01; clouds subsampled
::8 exactly as the render):

| arm | median | p90 | max | pieces > 10 mm |
|---|---|---|---|---|
| t323 K3R1 (baseline) | 2.5 | 3.5 | 4.7 | 0 |
| t323 MOR1 (baseline) | 2.5 | 3.6 | 4.5 | 0 |
| t326 K3R1 | 4.7 | 9.7 | 38.4 | 3 (P03 22.1, P24 17.0, P26 38.4) |
| t326 MOR1 | 3.8 | 23.5 | 114.7 | 5 (P03 23.5, P24 18.4, P25 85.4, P26 114.7, P31 64.0) |

Eye-check: the patched panels keep the overall vessel shape (no tilt or collapse), with visible
thinning and gaps along rim and belly against GT, noticeably worse on the MOR arm — matching the
12-16 lost joins. Individual stray pieces do not separate at this render scale; the table above
names them (P26 worst at 38-115 mm). Verified: `sacct 34720856` -> COMPLETED 0:0;
`piece_err326.out` read back from the run dir; both PNGs fetched and opened.

What the two tickets establish, stated precisely, because the direction matters: 325 moved no constant at all and the
Juglet stayed at 2/9, so the preprocessing axis gates were not the blocker. 326 moved every assembler gate **upwards on
the pots** (1.01-2.52x) and pot E lost 10-16 joins; on the Juglet the same rule scaled every gate **downwards**
(0.535x) — a *tightened* configuration, which cannot help a pair that already fails to reach the inlier floor. So this
ticket does **not** test loosening the evidence floor on the small object; nothing here has. What it does establish is
that the pot-calibrated values scaled down are not the answer, and that scaling them up costs a full-size pot.
"Scale-free thresholds" as written is not a fix and **`sfs_pristine_assembly_326.sif` is not proposed as a replacement**
for `sfs_pristine_assembly.sif`; the patch stays an experiment artefact on this branch.

What is left is not a threshold: the assembler will not register a pair without more than three inliers inside 1.5 mm and
15 deg (`MINIMUM_NUMBER` 6, `CountInlier`), and ticket 328 measured that six of the nine touching Juglet pairs cannot
supply them at 33 mm. That is the next ticket's subject.

## Lead ruling (2026-11-11): the rule is mis-tuned in direction; the branch is not merged

**Decision.** Pot E assembled 62/62 joins at the shipped constants (ticket 330, job 34749879). That is
the reference a pot must not fall below. This ticket's rule, as built, loosens every pot, so it cannot be
right about pots; it is withdrawn from merge candidacy. What it measured still stands.

**Why this is a tuning error, not a refutation.** The patch fixed its own reference size at 60 mm and said
why: "pot F's median piece diagonal is 61 mm, the smallest of the ten scored pots, so on every scored pot
the gate is at or above the legacy one (1.02x on F to 2.52x on C)"
(`hpc/patches/326-scale-free-matching.patch:6-11`). That choice makes the pot side of the rule a one-way
loosening. Pots were already at or near their ceiling, so loosening had nothing to win: seven of ten
unchanged, D and H a few joins down, E from 62 to 46-53. A rule written for a 33 mm vessel should not move
a pot at all.

**The control that names the reference.** Ticket 330 ran one binary with the multiplier as its only
argument: `s=60` (the rule neutral, job **34749879**, COMPLETED 0:0) -> 31/31 pieces and 62/62 joins, and
62/62 again on the second repeat; `s=92.77` (Pot E's own measured size, job **34749880**, COMPLETED 0:0)
-> 29/31 pieces and 53/62 joins. Pot E is not a hard pot the rule rescued; it is a pot the rule broke.

**What this ticket establishes, restated without the withdrawn sentence.**

- the pot-calibrated millimetres scaled **down** (Juglet, 0.535x) do not fix the Juglet - but that is a
  tighter configuration, so it cannot test "loosen the evidence floor", and nothing in 325/326 held that
  floor down;
- the same rule scaled **up** on pots costs joins (E 62 -> 46-53; D and H minor losses) and gains none
  (seven pots unchanged, no pot improved);
- the loss is **not wrong pairs admitted**: 330 found no ground-truth pair whose verdict flipped in either
  direction and no non-ground-truth pair newly admitted. The widening also moved the score every placement
  competes on (the inlier distance is itself scored against), so the search committed to a different
  sequence, and on a thirty-one-sherd pot the first wrong merge cannot be undone.

**Corrected rule, recorded for whoever builds it.** One-sided clamp:
`s = min(1.0, len_scale / 60.0)`, so an object at or above the reference keeps the shipped constants
exactly and only smaller objects tighten. Ticket 329's join-count floor already has this form
(`class/data_structure.cpp:1056-1061` in its patched copy), and 329's bar 6 asserts `SCALE329 ... s=1.000
M=6` on every pot lane - that run is the untouched-pot check for the corrected rule.

**Merge status.** Not merged to main. `sfs_pristine_assembly_326.sif` is not a replacement for
`sfs_pristine_assembly.sif`. The patch, container definition and diagnostics stay on branch
`326-scale-free-assembly` as experiment artefacts (kept, not deleted); this ticket is landed on main as
the record.

**Verified by the lead.** `hpc/patches/326-scale-free-matching.patch:6-11` read on the branch;
`sacct 34749879` -> COMPLETED 0:0 and `sacct 34749880` -> COMPLETED 0:0; both control logs read at
`binary_S60.log` / `binary_S9277.log`; the per-pot table re-read from the run dirs of
`20261110-2248_6925cb0_t326-pot<X>`; 329's clamp read at `class/data_structure.cpp:1056-1061` in that
ticket's patched copy.
