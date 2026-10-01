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

## MAPPING WRITE-UP 2026-10-01: REPRODUCIBLE (not structurally different)

Paper cost (appendix Eqs 8–16, verified against the text) beside code
(`reconstruction.{h,cpp}`, every functor read):

| Paper term | Code term | Verdict |
|---|---|---|
| J_R dist P2P, m=ΔRΔt (Eq 10) | `CostFuncDist`: w_d·m + Cauchy(4.0) | MATCH structure |
| J_R dist P2L, 4 terms l̂_A·l̂_B·n̂_A·n̂_B (Eq 11) | `CostFuncLineDist`: w_line·same 4-term set + Cauchy(4.0) | MATCH structure |
| J_R normal λ=0.4, e=\|\|R_An_A−R_Bn_B\|\|² (Eq 13) | `CostFuncNorm`: w_n·(R_An_A−R_Bn_B) + Cauchy(1.8), w_n=3.0 | MATCH structure; VALUE BACKWARDS (paper 0.4× down, code 3× up — 7.5× emphasis flip) |
| J_I axis μ=0.4, Cao + log-sum-exp softmin (Eqs 15–16) | `BiaxialCaoErrorWithFixedAxis`: w_a + softmin + Cauchy(1.8), w_a=0.1 | MATCH structure (code takes sqrt — "different from PotSAC" note; monotonic, absorbed by weight/scale); value same direction (0.1 vs 0.4) |
| J_I rim ν=0.4 | `CostFuncRim`: w_r=w_h=1.0 + Cauchy(1.8) | MATCH structure with SPLIT weights (expressible: set both 0.4); VALUE BACKWARDS (1.0 vs 0.4) |
| LM 100 / ICP 150 (line 240) | ceres 100 ✓ / outer 150 (paper-config) | MATCH |
| Cauchy kernel scales | paper UNSTATED | ours by necessity (4.0/1.8) — recorded, not paper's |

So the ticket's question is answered: settings EXIST (w_d=1.0, w_n=0.4,
w_a=0.4, w_r=w_h=0.4). No recorded deviation, no structural convert —
tuning toward the paper's numbers MEANS something here. The single most
suspicious fact in the table: paper down-weights normal AND rim (0.4×);
code up-weights normal 3× and holds rim at 1×. Emphasis backwards on two
of three weighted terms — and T6's failures are ROTATION errors, i.e.
normal-driven.

Variants liveness (item 3): `IcpIncGraphAxis` LIVE (called
`ranking_system.cpp:1041`, w_a=1.0 variant); `IcpFine` DEAD (decl+def
only, verified — same verdict as the Surface_F audit).

## E1 OUTCOME 2026-10-01 (job 31854704): normal emphasis does NOT recover

0/8 + 0/15, 22 states (vs 21), best 263 (vs 232), profile 83/2 (still
healthy), 13 overlap fails. Same regime — the 7.5× emphasis flip on the
normal term moves scores marginally and recovers nothing. Poses at T6
resolution unavailable for this run (dump env not set in sbatch; T6 needs
a dump-armed rerun to claim pose-level effects either way).
E1 reverted below; E2 (rim) proceeds on the reverted base, one variable.

## E2 OUTCOME 2026-10-01 (job 31881006): rim emphasis does NOT recover

0/8 + 0/15, 23 states (vs 22/21), best 608 (vs 263/232), profile 90/2,
13 overlap fails. Same regime — rim 1.0→0.4 moves scores, recovers
nothing. E2 reverted below; E3 (axis, same direction, 4×) proceeds on
the reverted base, one variable. After E3 the weights hypothesis is
exhausted either way: remaining structural candidates are correspondence
input quality (ticket-18 pollution feeds ICP directly) and the unmapped
Cauchy scales — not a fourth weight.

## EXPERIMENTS (one variable each, this tree onward) — E1 DONE (no recovery, reverted)

Base state, stated once: paper-config values + stddev rule + T0 dump
code, POT_A block. Each experiment changes ONE weight, rebuilds,
re-runs the same paper-config job, and reports states/scores/accuracy +
T6 rel-rot table:
- E1: w_n 3.0 → 0.4 (normal emphasis to paper's). FIRST (7.5× flip on
  the normal term behind rotation errors).
- E2: w_r=w_h 1.0 → 0.4 (rim emphasis to paper's).
- E3: w_a 0.1 → 0.4 (axis emphasis to paper's; same direction, 4× magnitude).
- KNOWN PARTIAL, recorded not hidden: outer-150 reached only `Icp` (:1210).
  The live `Registration` overloads (:1441, :1632) still run 50 outer —
  the paper-config arm was partial on iterations, and E1–E3 inherit that.
  Completing it (Registration → 150) is the next variable after E3, not
  part of E3. (`IcpIncGraphAxis` runs 200/200, `IcpFine` 100/100 —
  lineage values, untouched throughout.)
No prediction on direction (baseline is 0/8 — nothing to regress; watch
for losing the 6-state branching / crashes). Outer-150 stays (already
paper's). Cauchy scales stay (paper-silent).

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
