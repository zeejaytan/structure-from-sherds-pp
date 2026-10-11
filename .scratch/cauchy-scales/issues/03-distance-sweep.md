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

**Status:** ready-for-agent (rewritten 2026-10-12 after E-2; previous
sweep version superseded — see "Why rewritten")

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

- [ ] Step 0 — scorer read, ZERO new runs (lead or worker, before the
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
- [ ] Hunk: replace the `GraduatedDistScale(site, iter)` call at exactly
      the three hardcoded distance sites (`Icp`, `Registration` 5-arg,
      `Registration` 6-arg — current lines `:1486`/`:1735`/`:1951`, verify
      at implementation time) with the fixed literal `119.0`. The helper
      itself stays (E-2 record); the call sites stop calling it. Norm /
      axis / rim scales keep their literals 1.8. `IcpIncGraphAxis` (env)
      and dead `IcpFine` (5.0) untouched, as in 02.
- [ ] Dump calls pass `119.0` (same live-value rule as 02 — the dump's
      `a` column must show 119 at iter 0, or it lies).
- [ ] Weights, iteration counts, gates, solver options, caps, staging
      untouched. Six cap invariants still hold (100 ×5, 1000 ×5).
- [ ] Batch arm `hpc/sbatch/run_e3_p50.sbatch` modelled on
      `run_e2_graduated.sbatch`: marker grep (three sites `CauchyLoss(119.0)`,
      untouched-site checks, dump-live-value checks, cap checks, POT_A
      check) + freshness + one Pot_A full assembly, pinned
      `--nodelist=spartan-bm065`, dump enabled. Single run citing the
      existing solver-01 control per 02's recorded deviation (dump
      outcome-neutrality already proven, job 32696452 A vs B).
- [ ] **Kill criterion stated before running:**
      1. Primary read — dist `s/a²` p50 ≈ 1, `ρ′` p50 ≈ 0.5 at iter 0
         (arithmetic prediction from E-0 `fnorm` p50 111–119 mm at
         `a = 119`). If the term is still suppressed or fully online,
         the arithmetic is wrong — report, do not tune.
      2. Divergence stays 0 (cap work holds; anything above is a STOP).
      3. Accuracy vs control (2/8, 1/15): HOLDS → distance at p50 is
         neutral, direction stays open; GROWS → follow it (ticket for
         the winning value); FALLS to 0 → distance direction CLOSED,
         route to 04/05 with the numbers.
      4. Score vs accuracy read SEPARATELY (E-2 lesson): report both,
         never let a score rise overrule an accuracy fall.
      5. Pair 1-4 still SURVIVED; SURVIVED ≥ 14.
      - Do not tune the value in place whatever moves. Revert, record,
        report.
- [ ] Report: primary read before/after, outcome lines, diverged count,
      step-0 scorer answer, verdict in ticket 11 + S1 Where-it-stands.

## Out of scope (unchanged from spec.md)

Weights, iteration counts, gates, caps/anchoring, correspondence gate,
Juglet half, "the paper's" values. Ticket 06 (shape diagnostic) still
runs last.
