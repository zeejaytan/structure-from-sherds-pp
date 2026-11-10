# Ticket 326 — Scale-free assembly: stop judging a 33 mm vessel with pot-sized millimetres

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-10)
**Branch:** 326-scale-free-assembly, in /c/PR/worktrees/structure-from-sherds-pp/326
**Answers:** S1
**Depends on:** 325 (an admissible axis at this scale; without it the axes arm cannot be read)
**Blocks:** none
**Worker:** unassigned
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
