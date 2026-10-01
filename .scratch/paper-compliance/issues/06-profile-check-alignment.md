# 06: Profile check paper alignment (fit, statistic, numbers, input)

**Answers:** S1

**Blocked by:** ticket 18's attribution where bundles are concerned (see
interaction rule below) — otherwise nothing; spike-first

**Status:** ready-for-agent

**Needs-eye:** none — fit statistics and gate counts, no geometry claim.
A re-stage enters only if rim content changes (ticket 18's rule).

## Why this ticket exists

Paper (lines 249/550, Table II): inner-surface edge points → rz-plane →
7mm z-bins → ORTHOGONAL regression per bin → reject on STD-DEV of
orthogonal errors > 7mm. Code (`CheckGraphPlausibility`, the live
production gate): full `edge_line_` points (rim + fracture, and ticket 18
shows face-crossing traces ride along) → same bins → ORDINARY least
squares of r on z (`CalculateLeastSquare`, verified — "orthogonal" is
only the Eigen solver's name) → reject on ANY single point > threshold,
with 6.5/6.0 in place of 7.0/7.0. Four independent deviations in one gate
(fit, statistic, numbers, input) plus a fifth in the dead-by-default
puzzlefusion caller (piece CENTER positions instead of edge points —
do not "fix" that path; it doesn't run; record it). Ticket 05 item 5.

## Interaction rule (binding)

Ticket 18 owns face-crossing traces. Any measurement here states its
bundle arm (pre/post-18-fix). A profile failure measured on a polluted
bundle cannot attribute fit-vs-input — run post-18-fix bundles wherever
the fix lands first, and say which.

## What to build (spike first, one variable each)

1. Attribute before changing: on a post-18 bundle, which of the four
   deviations moves any live gate (Pot_A pairs, Juglet honest pairs)?
   Orthogonal-vs-OLS on real bin data (laptop-recomputable from saved
   profiles); max-vs-stddev rule (same); 6.5/6.0-vs-7/7 (assembly runs);
   inner-only-vs-full-edge input (assembly runs). Rank by measured effect,
   fix in that order, re-measure after each — never as a bundle.
2. The puzzlefusion piece-centers input: record-only (dead by default).
   If that branch ever activates, its profile call is meaningless on
   centers — file it then, not here.
3. Provenance: the 6.5/6.0 drift has a partial trail (7.0/7.0 → 5.0/4.0 →
   6.0/5.5 in tuning logs; 6.5/6.0 itself unsourced). Whatever value wins
   gets a measurement cite in the code comment, ending the drift.

## Acceptance criteria

- [ ] Spike: per-deviation effect ranking with numbers (which move gates,
      which don't)
- [ ] Fixes in ranked order, one variable each, per-pair assembly numbers
      after each (Pot_A + Juglet honest + authors' sample)
- [ ] No regression rule: a pair passing before fails after → ticket stays
      open, change reverted in isolation
- [ ] Final state matches paper or records acceptance per deviation, each
      with its measurement
- [ ] Ticket 18's bundle arm stated for every measurement
