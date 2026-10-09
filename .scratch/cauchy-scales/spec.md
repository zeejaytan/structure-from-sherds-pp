# Spec — Cauchy kernel scales (tolerance, not emphasis)

**Answers:** S1

**Status:** spec for slicing; nothing implemented yet

## Problem statement

From the conservator's side: the pipeline finds every true join on the
sample pot (15/15 through matching) yet seats only 2 sherds out of 8.
The placer hands back placements 62–179° off on true pairs, and every
fourth attempt leaves the map entirely. The E1–E3 weight tests moved
emphasis (how hard each mismatch class pulls) and recovered nothing —
but weights and tolerances are different knobs, and tolerance was never
turned. The solver's outlier bowls may be starving true fits of any pull
at the start: the live distance bowl is 4 mm against 5–12 mm
iteration-0 errors the code itself says must be refined through, and at
100 mm starts the pull-weight is 0.0016 — nearly no gradient to follow.

## Solution

Measure first (log per-class residual norms and pull-weights at
iteration 0 — zero behavior change), then test tolerance one class at a
time at fixed weights: graduated large→small schedule (the one move
Ceres affirmatively recommends), distance sweep, angle sweep, axis/rim
after units are measured, loss-shape swap as diagnostic. Read
divergence rate and join quality separately throughout — tight trades
starvation against rejection, loose the reverse. One variable per
experiment, same machine per pair, Pot_A per-pair scoring throughout.

## User stories

1. As the conservator, I want true joins to keep enough pull at the
   start to converge, so that the placer stops starving on far starts.
2. As the conservator, I want false joins to still be rejected, so that
   loosening tolerance doesn't trade garbage for joins.
3. As the conservator, I want the 2-of-8 to grow with the same pair plus
   new ones, so that progress is directional, not a reshuffle.
4. As the lead researcher, I want tolerance measured before it is
   changed, so that each sweep starts from the actual residual
   distribution, not a guess.
5. As the lead researcher, I want weights frozen during every scale
   experiment, so that the tolerance direction E1–E3 never explored is
   isolated.
6. As the lead researcher, I want every run same-machine pinned, so that
   numbers are comparable, not draws across CPU architectures.
7. As the lead researcher, I want the paper's silence on scales recorded
   as a fact, so that no value is ever called "the paper's" without a
   line cite that doesn't exist.

## Implementation decisions

- The E-0 hunk is log-only (per-block `‖f‖`, `s/a²`, `ρ′` at iteration
  0): zero behavior change by construction; verified by a default-path
  run before any sweep.
- Sweep hunks touch only the scale argument at the relevant solve
  sites — same value everywhere the class appears on the live path.
  Weights, iteration counts, gates, descriptors untouched throughout.
- The graduated schedule uses `LossFunctionWrapper` (mutate-after-build),
  not a rebuild per phase — the tuning practice Ceres documents.
- Scale ≈ 2–3× the measured inlier σ per class (curve-fitting
  precedent); paper thresholds (5 mm, 30°) are experiment anchors, not
  derivations — stated as interpretation wherever used.
- Control tree for all sweeps: the current tree at slice time (rows
  decided so far in ticket 12); each experiment states its control job
  explicitly. Same-node A/B per the ticket-12 protocol (marker +
  freshness + POT_A verification in-job).
- No cap/anchor changes in this feature. Solver setup is owned by
  `solver-convergence/` (cap ticket 01 in flight, anchor 02 queued);
  this feature never touches the solver-options block or pose
  freeze/unfreeze. Interface: that feature's control tree is ours when
  it lands; until then each ticket names its own control job.

## Testing decisions

- What makes a good test here: per-pair assembly numbers on Pot_A
  (accuracy + states + best score), diverged-solve counts, whether the
  same pair passes, and sane-path behavior — never exit codes or log
  lines alone.
- Prior art: the ticket-12 same-node A/B protocol, the ticket-11
  forensics table (26k attempts, uniformity verdict), the E1–E3 weight
  series (null results that motivate — but do not substitute for — the
  scale direction).
- Each experiment states its kill criterion before running (what number
  must move, and what staying identical would mean). Divergence rate and
  convergence quality are read separately: a scale that improves one
  while harming the other is the expected outcome, not a contradiction.

## Out of scope

- Trust-region cap and anchoring (owned by `solver-convergence/`
  tickets 01/02 — interface, not duplication).
- Weights, iteration counts (exhausted, ticket 07).
- Correspondence gate at formation (tested hurting, V2a).
- Opposing veto, adaptive inlier, priority scoring (ticket 12 rows 4/5/7).
- Juglet extraction or assembly (no solver fix transfers until Pot_A
  moves; Juglet half stays untouched).
- Calling any scale value "the paper's" — the paper states none; every
  ticket records chosen values as implementer choices with measurement
  cites.

## Further notes

- Research: `cauchy-scale-research.md` in this directory (Ceres
  formula + scale semantics, weight-vs-scale proof, effective-knee
  table, ranked E-0…E-F with predicted effects, paper-silence
  statement). Primary sources only, every claim cited.
- Grill decisions: measure-before-change (E-0 gates all sweeps);
  weights frozen throughout; schedule before sweeps (Ceres'
  recommendation outranks our ordering intuition); shape-swap last as
  diagnostic, never as fix; divergence and convergence read separately.
