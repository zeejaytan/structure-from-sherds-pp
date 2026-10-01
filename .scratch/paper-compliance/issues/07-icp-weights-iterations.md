# 07: ICP weights mapping + outer iteration count

**Answers:** S1

**Blocked by:** nothing — methods comparison first (no assembly runs
until the mapping question is answered)

**Status:** ready-for-agent

**Needs-eye:** none — weights and iteration counts, no geometry claim.

## Why this ticket exists

Paper (lines 240/491/519): LM max 100 inner iterations, ICP max 150
outer; λ=0.4 normal-mismatch weight (Cauchy-scaled); μ=ν=0.4 axis/rim
weights. Code (live `Icp`, `reconstruction.cpp:1191+`): ceres 100 inner
(MATCH), `max_iteration=50` outer (DEVIATION, verified); and NO 0.4
anywhere in the live path (verified by grep) — instead split linear
weights (w_d=1.0, w_n=3.0, w_line=1.0, w_a=0.1, w_r=w_h=1.0) × Cauchy
scales (4.0/1.8/1.8/1.8), with no mapping comment to the paper's
formulation. Ticket 05 items 4–5. This is NOT a value swap ("set w_a to
0.4" would be meaningless across different parameterizations) — it is a
methods comparison first.

## What to build (mapping before measurement)

1. Write down the paper's cost (Eqs. 8/14 + Cauchy kernels ρd/ρe, λ/μ/ν
   scalings) beside the code's cost (linear weights × Cauchy losses per
   residual class) and answer: does ANY (weights, scales) setting
   reproduce the paper's cost behavior, or are they structurally
   different costs (e.g. per-class linear weights the paper has no
   analogue of)? Paper Eqs. 8/14 + supplementary [43] are the source;
   cite lines, not memory.
2. Only then: outer iterations 50-vs-150 and any mapped weight comparison,
   measured as assembly A/B per pair (Pot_A + Juglet honest + authors'
   sample), one variable each. If the costs are structurally different,
   say so and convert this ticket to a recorded deviation (like the
   ordering vote: known, measured, accepted) instead of tuning toward a
   number the code cannot mean.
3. `IcpIncGraphAxis` (w_a=1.0 + env gate) and `IcpFine` (half weights)
   variants: record which runs where (liveness unchecked in the audit);
   do not touch them here.

## Acceptance criteria

- [ ] Mapping write-up: paper cost vs code cost, reproducible-or-
      structurally-different verdict with Eq/line cites
- [ ] DIRECTING NUMBERS 2026-10-01 (ticket 10 T6): candidate relative poses
      62–179° off on TRUE pairs; 150 outer iterations did not prevent it.
      Weights mapping is now URGENT, not academic — unmapped weights are
      the prime suspect for ICP mis-convergence. T6 method (dumped T vs
      GT, frame-free) is the re-measurement protocol after any weights
      change.

- [ ] Mapping write-up: paper cost vs code cost, reproducible-or-
      structurally-different verdict with Eq/line cites
- [ ] If reproducible: A/B per pair per variable; no-regression rule
      (passing pair fails → revert in isolation)
- [ ] If structurally different: recorded deviation with the structural
      reason (not a shrug, not a tune)
- [ ] Outer-iteration count resolved either way (150 measured or 50
      justified)
