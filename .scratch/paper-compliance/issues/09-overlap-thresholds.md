# 09: Overlap area thresholds — unify or justify per stage

**Answers:** S1

**Blocked by:** nothing — read the gates, then assembly A/B if any value moves

**Status:** ready-for-agent

**Needs-eye:** none — thresholds and pair movement, no geometry claim.

## Why this ticket exists

Paper: one overlap area value, 50mm² (lines 548/565). Code: FIVE per-stage
values — 50.0 `RegistrationPruning` (:2060–2064, "Restored to normal",
matches), 120.0 `PairwisePruning` (:1828, "ORIGINAL THRESHOLD" comment),
10 `CheckGraphPlausibility` (:1869, "REDUCED 50→10"), 100
`MergeOverlapTest` (:2134–2138), 20 `SingleOverlapTest` (:1973). Ticket 05
item 11. Possibly each stage legitimately needs its own gate (early
pruning loose, plausibility tight) — but nothing says so, and two
comments claim opposite histories ("original" 120 vs "normal" 50). At
least one of those comments is wrong about what the paper specifies.

## What to build

1. Read each of the five gates and write down what it protects against
   (what passes if loosened, what dies if tightened) — one line each,
   with cites. The `Enhanced3DOverlapDetector` chain (2.0mm/500mm³ live
   values, ticket-05 liveness table) is evidence, not a sixth gate; cite
   it where a stage duplicates its job.
2. Decide per stage: paper 50, current value with measurement, or a third
   value with numbers. Assembly A/B per pair for every stage that moves
   (Pot_A + Juglet honest + authors' sample), one stage per experiment.
3. Fix the comments: each surviving value gets a measurement-or-reason
   cite. "Original"/"normal"/"reduced" without a pointer is how five
   values grew where one was specified — no new bare claims.

## Acceptance criteria

- [ ] Per-stage protection write-up (5 lines + cites)
- [ ] DIRECTING NUMBERS 2026-10-01 (ticket 10 T5): true merges span
      35–297mm²; false [5,6] at 40–60mm² sits INSIDE that range; paper's
      own 50 kills true [1-5] carrying the run's top scores (144, 155).
      No threshold separates — the verdict is SHAPE, not value: overlap
      fires on pose error (T6's 62–179° interpenetrate), not pair validity.
      Any surviving threshold must pair with score or pose sanity, never
      stand alone.

- [ ] End value per stage with measurement or reason; A/B per pair for
      every moved stage; no-regression rule
- [ ] Comments cite measurements; the two contradictory history claims
      resolved (one corrected, both corrected, or both struck)
