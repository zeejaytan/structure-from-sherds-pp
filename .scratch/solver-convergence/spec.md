# Spec — solver convergence (trust-region bound, then anchor)

**Answers:** S1

**Status:** spec for slicing; nothing implemented yet

## Problem statement

From the conservator's side: the pipeline finds every true join on the
sample pot (15/15 through matching) yet seats only 2 sherds out of 8.
The placer — the solver that fits two sherd edges together — hands back
placements that are catastrophically wrong (62–179° off on true pairs),
and every fourth attempt leaves the map entirely, landing trillions of
mm away on a 300 mm pot. The guard catches the garbage loudly, but
catching is not fixing: nothing converges that did not converge before.

## Solution

Bound how far one solver step can walk (a trust-region cap, paper-silent
so ours to choose), measure whether the garbage stops without sane
placements changing path; only then consider holding one sherd fixed
while solving the other (the paper's Eq. 5 mechanism, a method change
that needs its own justification). One variable per experiment, same
machine per pair, Pot_A per-pair scoring throughout.

## User stories

1. As the conservator, I want the placer to stop landing trillions of mm
   away, so that wrong answers are at least on the map and judgeable.
2. As the conservator, I want sane placements byte-identical after the
   fix, so that nothing that worked is quietly traded away.
3. As the conservator, I want pair 1-2 (eye-witnessed genuine) to survive
   to a merge attempt, so that the thinnest true join stops dying
   silently.
4. As the conservator, I want the 2-of-8 to grow with the same pair plus
   new ones, so that progress is directional, not a reshuffle.
5. As the lead researcher, I want each experiment to change one mechanism,
   so that a failure attributes cleanly (cap vs anchor vs input).
6. As the lead researcher, I want every run same-machine pinned, so that
   numbers are comparable, not draws across CPU architectures.
7. As the lead researcher, I want the paper's Eq. 5 mechanism tested as a
   method change with its own A/B, never by appeal to the paper.

## Implementation decisions

- The cap hunk touches only the solver-options block at each solve site:
  same field and value everywhere, including the never-called site (the
  uniformity precedent for shared hunks). Still one variable.
- Values are scene-scale (initial 100, max 1000 — against GT max 427 mm
  and ~100 mm correspondence distances), not tuned: the fact under test
  is the mechanism (a bound exists), not the number.
- The anchor hunk freezes one sherd's pose blocks before the solve and
  unfreezes after (existing freeze/unfreeze calls, no new machinery).
  Which sherd is fixed follows the code's own fixed/moving convention.
- No Cauchy-scale, weight, iteration, gate, or descriptor changes in this
  feature. Those hypotheses are exhausted or owned elsewhere.
- Row 4 veto work (the opposing-faces check) belongs to the other thread;
  this feature never touches the veto branch or its call sites.

## Testing decisions

- What makes a good test here: per-pair assembly numbers on Pot_A
  (accuracy + states + best score), diverged-solve counts, whether the
  same pair passes, and sane-path byte-identity — never exit codes or
  log lines alone.
- Prior art: the ticket-12 same-node A/B protocol (control arm + one
  variable, pinned machine, freshness + marker verification in-job),
  the ticket-11 forensics table (26k attempts mined for preconditions),
  the guard's loud-reject lines as the divergence counter.
- Each experiment states its kill criterion before running (what number
  must move, and what staying identical would mean).

## Out of scope

- Cauchy loss scales (unmapped, unowned — separate future work).
- Iteration counts, weights (exhausted, ticket 07).
- Correspondence gate at formation (tested hurting, V2a — needs a new
  reason before revisit).
- Juglet extraction or assembly (no solver fix transfers until Pot_A
  moves; Juglet half stays untouched).
- The veto rows (other thread), descriptor smoothing, base flags,
  fractured surface (all resolved or owned elsewhere).

## Further notes

- Research fact sheets: `.scratch/solver-convergence/ceres-options-research.md`
  (Ceres fields, version pins, per-site map, 14-row experiment table) and
  `paper-anchoring-research.md` (paper quotes with lines, NOT-specified
  list, SfM practice, no-duplication note vs tickets 07/11).
- Grill decisions: cap first (paper-silent, one hunk), all five sites same
  hunk, initial+max together as one mechanism.
