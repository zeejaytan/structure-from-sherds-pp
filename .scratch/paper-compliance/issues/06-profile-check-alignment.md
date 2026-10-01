# 06: Profile check paper alignment (fit, statistic, numbers, input)

**Answers:** S1

**Blocked by:** ticket 18's attribution where bundles are concerned (see
interaction rule below) — otherwise nothing; spike-first

**Status:** resolved 2026-10-01 — statistic fixed and measured; fit
explicitly parked (T2: refit changes nothing — reopens only on new
evidence); input belongs to ticket 18; guard done. Gate now behaves as
the paper intends (below).

**Needs-eye:** none — fit statistics and gate counts, no geometry claim.

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

## RESOLVED 2026-10-01 (fix 1 measured end to end, job 31843205)

Std-dev rule + corrupt-input guard, same paper values, same bundle:
profile 87 PASSED / 2 FAILED (was 4/19); the only 2 kills are [1,5,8]
twice — a NON-TRUE config, correctly killed. Every true merge passes.
The gate now does what the paper intends. Fit stays OLS (parked per T2),
numbers stay 7.0/7.0, input stays full-edge (ticket 18 owns pollution).

Assembly still 0/8 + 0/15 — and that is itself the finding: with
plausibility no longer blocking, beam chains 5–6-piece graphs (21 states,
best 232) that are all wrong. The next layer is poses + scores (tickets
07/08, assembly-09), not the profile gate. Ticket 09's baseline re-run
criterion is NOT met by this run (different code); 09 stays open.

## Acceptance criteria (closed)

- [ ] Spike: per-deviation effect ranking with numbers (which move gates,
      which don't)
- [ ] DIRECTING NUMBERS 2026-10-01 (ticket 10 battery, no re-argument):
      max-rule→stddev flips 20/20 recorded failures with 0 new false
      passes (fix FIRST); TLS refit changes nothing (skip); wall-only
      subset confounded (no signal); garbage-T guard required (bin-count
      overflow vacuous-passes corrupt configs — see ticket 10)

- [ ] Fixes in ranked order, one variable each, per-pair assembly numbers
      after each (Pot_A + Juglet honest + authors' sample)
- [ ] No regression rule: a pair passing before fails after → ticket stays
      open, change reverted in isolation
- [ ] Final state matches paper or records acceptance per deviation, each
      with its measurement
- [ ] Ticket 18's bundle arm stated for every measurement
