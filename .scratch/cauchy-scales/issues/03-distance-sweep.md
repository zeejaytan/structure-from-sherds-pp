# 03: Distance at p50 — single fixed arm (rewritten after E-2, 2026-10-12)

**What to build:** ONE fixed distance scale at the measured p50
(`a_dist = 119.0` at the three hardcoded solve sites), weights frozen —
not the 1/2/4/8 sweep this ticket previously proposed. E-2 already ran
the sweep's permissive end as a schedule and its kill criterion fired
against the schedule; this ticket asks the narrower question E-2 left.

**Answers:** S1

**Blocked by:** 01 (measured) + 02 (E-2 verdict — this ticket's premise).
The scorer read (step 0 below) gates the run: if the scorer prefers the
wrong assembly, the run still proceeds (tolerance and scoring are
separate variables) but the verdict must read accuracy and score
separately.

**Status:** done (run `32729140` COMPLETED 2026-10-12; verdict
landed in ticket 11 + S1 — distance CLOSED)

## Why rewritten (E-2 verdict, job 32722789)

The previous version proposed arms at `a/w` ≈ 1/2/4/8 mm to find
"whether the knee brackets the operating point, and 02 is the
principled answer." E-2 measured:

- Permissive end (schedule 260 → 4.0): term online on prediction
  (`s/a²` p50 0.18–0.21, `ρ′` p50 0.83–0.85 at iter 0) AND accuracy
  2/8 + 1/15 → 0/8 + 0/15 while best score ROSE 262 → 266 — false
  matches admitted, the "loose" half of the trade. Kill criterion's
  "large `a` raises false-match convergence" branch fired against 02.
- Tight end (1–2 mm): tightens a term E-0 proved 780× past its knee —
  strangling a switched-off term predicts nothing but 2/8 minus noise.

Re-running fixed large-`a` arms would spend ~20 minutes each re-learning
what 32722789 already says. The open question is the middle: at the
measured p50 the term is half-online (`s/a²` p50 ≈ 1, `ρ′` ≈ 0.5 at
iter 0, arithmetic from the E-0 `fnorm` distribution) — does accuracy
hold 2/8, grow, or follow E-2 to 0? Any of the three is a finding; 0
kills the distance direction entirely and routes to 04 (normals) / 05
(axis) instead.

## Approved hunk spec

- [x] Step 0 — scorer read, ZERO new runs (lead or worker, before the
      arm is submitted): compare the best assemblies of the two logs on
      disk (`sfs_main/e2e_pota_e2_32722789.log`, score 266.000 at 0/8,
      vs the solver-01 control `sfs_main/e2e_pota_fresh_*.log` for job
      32624499, score 262.000 at 2/8, both bm065). Question: are the
      266-assembly's merges false pairs outscoring true ones? If yes,
      even a tolerance that seats true pairs may not win the beam — and
      every downstream sweep is gated on a scorer question, not a
      tolerance question. Record the answer in the verdict either way;
      it does not stop the run (separate variables), but a "scorer
      prefers wrong" finding routes to the scorer's owner alongside —
      not instead of — the tolerance verdict.
      DONE (worker, 2026-10-12, zero new runs, both bm065) — ANSWER YES:
      control `sfs_main/e2e_pota_solver01_32624499.log` (NOTE: actual
      filename is `e2e_pota_solver01_*.log`, not `e2e_pota_fresh_*.log`;
      18M Oct10; Best State #0 7/8 score=262.000 Sherd 2/8 Edge 1/15)
      vs E-2 `sfs_main/e2e_pota_e2_32722789.log` (20M Oct12; Best State
      #0 7/8 score=266.000 0/8 0/15). Tails identical SURVIVED14
      FEATURE_MATCHING1 incl Pieces1-4 SURVIVED; both saved pieces
      1,3,4,5,6,7,8 (missing 2). assembly_data.txt decides: control
      G0={1,4,5,6,7,8} G1={3}, final MERGETABLE [1-3] corObjs=7
      corPts=170 tablesum=14.000; E-2 G0={1,4,5,7,8} G1={3,6}, late
      merges [5-7]/[5-8]/[1-8]. Poses differ (piece6 control
      t=[4.50,1.65,-69.33] vs E-2 near-identity; piece3 identity vs
      rotated). Scorer prefers wrong (266>262 seats 0 vs 2); tolerance
      vs scoring separate; run proceeded.
- [x] Hunk: replace the `GraduatedDistScale(site, iter)` call at exactly
      the three hardcoded distance sites (`Icp`, `Registration` 5-arg,
      `Registration` 6-arg — current lines `:1486`/`:1735`/`:1951`, verify
      at implementation time) with the fixed literal `119.0`. The helper
      itself stays (E-2 record); the call sites stop calling it. Norm /
      axis / rim scales keep their literals 1.8. `IcpIncGraphAxis` (env)
      and dead `IcpFine` (5.0) untouched, as in 02.
      DONE — commit `2098f55`: `:1486`/`:1735`/`:1951` now
      `new ceres::CauchyLoss(119.0)`; helper def `:1356` retained
      (`GraduatedDistScale(` count = 1); `IcpIncGraphAxis` still
      `CauchyLoss(cauchy_dist)` + `SFS_CAUCHY_DIST` gate (`:2142-2144`);
      dead `IcpFine` still `5.0` (`:2365`); 9x `1.8` intact.
- [x] Dump calls pass `119.0` (same live-value rule as 02 — the dump's
      `a` column must show 119 at iter 0, or it lies).
      DONE — `:1540`/`:1786`/`:2009` pass
      `LogE0Residuals(site, iter, 119.0, 1.8, 1.8, 1.8, problem)`; run
      dump confirms a=119 at all three sites at iter 0.
- [x] Weights, iteration counts, gates, solver options, caps, staging
      untouched. Six cap invariants still hold (100 ×5, 1000 ×5).
      DONE — verified at `2098f55`: caps 5+5, SOLVER-01 + E3 REVERTED
      markers, 150/100 x3, POT_A `#define` active,
      `multi_hypothesis_optimizer.cpp:887` untouched.
- [x] Batch arm `hpc/sbatch/run_e3_p50.sbatch` modelled on
      `run_e2_graduated.sbatch`: marker grep (three sites `CauchyLoss(119.0)`,
      untouched-site checks, dump-live-value checks, cap checks, POT_A
      check) + freshness + one Pot_A full assembly, pinned
      `--nodelist=spartan-bm065`, dump enabled. Single run citing the
      existing solver-01 control per 02's recorded deviation (dump
      outcome-neutrality already proven, job 32696452 A vs B).
      DONE — arm shipped in `2098f55` (186 lines); job `32729140`
      COMPLETED 0:0 in 00:20:09 on spartan-bm065; all in-job marker
      greps passed, build exit 0, HEAD `2098f55` confirmed in-job.
- [x] **Kill criterion stated before running:**
      DONE — job `32729140` (sfs_e3_p50), COMPLETED 0:0 in 00:20:09 on
      `spartan-bm065`, commit `2098f55`. Single run citing the existing
      solver-01 control (job 32624499, same node) per 02's recorded
      deviation. STOPs a–e: NONE triggered (details per branch below);
      no tuning performed. Verdict NOT written here — lead lands it in
      ticket 11 + S1 (worker reports evidence only).
      1. Primary read — LANDED ON PREDICTION. `a` confirmed 119 at all
         three sites at iter 0 (Icp n=4628, Reg5 n=141183, Reg6
         n=100777; in-run control `IcpIncGraphAxis` still `a = 1`,
         n=8388):

         | site | n | `s/a²` p50 / p90 / max | `ρ′` p50 / p90 / max |
         |---|---|---|---|
         | Icp | 4,628 | 1.005 / 2.313 / 2.753 | 0.4986 / 0.8581 / 0.9976 |
         | Registration5 | 141,183 | 0.859 / 2.46 / 4.548 | 0.5379 / 0.8669 / 0.9976 |
         | Registration6 | 100,777 | 0.8562 / 2.434 / 3.882 | 0.5387 / 0.8237 / 0.9999 |
         | IcpIncGraphAxis (control) | 8,388 | 4.192 / 48.17 / 547.2 | 0.1926 / 0.7938 / 0.996 |

         Ticket predicted `s/a²` p50 ≈ 1, `ρ′` p50 ≈ 0.5 from
         `(115/119)²`. Landed on it. Arithmetic holds — STOP-a clear,
         term half-online as designed. Before: E-0 at `a = 4.0` was
         `s/a²` p50 774–890, `ρ′` p50 0.00128. 40,121 dump files at
         `/data/gpfs/projects/punim2657/sfs_main/e3_dumps`.
      2. Divergence: `DIVERGED-SOLVE` count 0 (grep -c returned 0) —
         stays 0, STOP-b clear (cap work holds).
      3. Accuracy vs control (2/8, 1/15): E-3 = **0/8 + 0/15** — FALLS
         to 0 branch FIRES: distance direction CLOSED, route to 04/05
         with the numbers (lead to land). Best assembly 7/8 pieces
         score=266.000 — identical score to E-2's 266.000.
      4. Score vs accuracy read SEPARATELY: score 266.000 (= E-2, up
         from control 262) while accuracy 0/8+0/15 (down from 2/8+1/15)
         — repeats the E-2 signature (false matches admitted). PROFILE
         831 (control 783, E-2 855).
      5. Pair 1-4 still SURVIVED (`Pieces 1-4 @ SURVIVED: Feature
         matching succeeded (value=21.000)`); SURVIVED 14 /
         FEATURE_MATCHING 1 — unchanged, STOP-d/e clear.
      - No tuning performed; value stays 119.0 in tree for the record.
- [x] Report: primary read before/after, outcome lines, diverged count,
      step-0 scorer answer, verdict in ticket 11 + S1 Where-it-stands.
      DONE (worker evidence; verdict NOT written — lead lands ticket 11
      + S1): primary before E-0 (`a`=4.0: `s/a²` p50 774–890, `ρ′`
      p50 0.00128) → after E-3 (`a`=119: per-site table in branch 1
      above). Outcome lines: `Best assembly: State #0 with 7/8 pieces
      (score=266.000)`, `Sherd accuracy : 0 / 8 (0.000%)`, `Edge
      accuracy : 0 / 15 (0.000%)`; DIVERGED-SOLVE 0; SURVIVED 14 /
      FEATURE_MATCHING 1; pair 1-4 SURVIVED. Step-0 scorer answer: YES,
      scorer prefers wrong (266 at 0/8 outscores 262 at 2/8 — recorded
      in Step 0 box). Kill-branch reading: FALLS-to-0 → distance
      CLOSED, route 04/05. Commits: `2098f55` (hunk+arm) + this ticket
      write-up. Job `32729140` on spartan-bm065, COMPLETED 0:0,
      00:20:09. Log `sfs_main/e2e_pota_e3_32729140.log`; job stdout
      `sfspreproc-docker/slurm-32729140.out` (85 lines); build log
      `build_sif/e3_build_32729140.log` (exit 0).

## Out of scope (unchanged from spec.md)

Weights, iteration counts, gates, caps/anchoring, correspondence gate,
Juglet half, "the paper's" values. Ticket 06 (shape diagnostic) still
runs last.
