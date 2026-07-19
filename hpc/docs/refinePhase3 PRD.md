# PRD — Hierarchical Archaeological Assembly (Next Steps)

## Purpose and context

Phase 3+ is in and working. I now select ICP connections per phase, propagate world transforms correctly, and drive a three-stage build (rim → base → body). The next increment turns the structural cues into harder rules, smarter gap targeting, and topology controls so larger, messier vessels behave. I’m keeping this PRD short and generic so it drops into any build; I’ll stick to geometry-only logic and reuse the existing optimiser and file layout.

## Objectives

I want ring closure to be real, not implied. I want body placement to actively reduce measured gaps rather than just “fit somewhere good”. I want to stop hub-and-spoke graphs before they form. I also want connection ranking to reward circular vessel structure in a way that is measurable, and I want a pottery-aware validator to catch joins that look numerically fine but break simple fabric rules (height, thickness, curvature continuity). Success means cleaner rings, fewer dangling gaps, healthier degree distributions, and stable trees that survive a global refine.

## Scope

I will add five behaviours without changing the public optimiser entry point. First, stricter ring closure validation with configurable thresholds applied at acceptance time. Second, a gap-targeting scheduler that always chooses the connection that closes the largest current gap by the biggest margin. Third, degree limits and a small regulariser to prevent hubs while still allowing two-degree rings. Fourth, circular topology bonuses that scale with arc coverage and closure progress, not just “rim-to-rim” labels. Fifth, a light pottery-aware validator that rejects or down-weights connections that violate height/thickness/curvature continuity. All of this will operate inside the existing state machine and beam search, and will respect the world-transform propagation already fixed.

## Functional requirements

Ring closure must pass a hard validator before a rim or base is marked “closed”. I will fit a robust circle in the axis-orthogonal plane, compute RMS point-to-circle error and the maximum chord gap between consecutive segments, and accept only when RMS is under 5 mm and the largest gap is under 10 mm by default. These thresholds will be tunable and applied after each proposed ring update. If the ring cannot meet the thresholds within a fixed number of attempts, I will leave it “open” and proceed, but I’ll log the failure with the measured errors.

Gap targeting will maintain a live index of gaps on each ring and in the vertical body. For rings, I will sort gaps by angular width. For the body, I will derive “corridors” by height bands and missing radial coverage. At each placement I will evaluate eligible connections against the largest current gap first and accept only if the placement reduces that gap by a configured fraction (for example at least 20 %). After placement I will update transforms, recompute the gap list, and repeat until no gap can be reduced further.

Hub prevention will enforce a soft degree limit per piece, defaulting to three for body fragments and exactly two for ring membership. I will add a penalty term to the connection score that grows with current node degree and will hard-block any new edge that would push a ring piece beyond degree two. If a body piece genuinely needs a higher degree (rare), I can relax this via config for that dataset.

Circular topology bonuses will no longer be flat. I will scale bonuses with arc completion. A connection that closes a long missing arc gets a larger boost than a short rim-to-rim link that sits on an already dense section. I will also add a one-off ring-closure bonus when a connection brings the ring under both RMS and max-gap thresholds. The same logic applies to bases.

Pottery-aware validation will run as a gate before I accept a connection into the assembly set. I will check height monotonicity against the current axis (no sudden height inversions across a join), local wall-thickness continuity across the fracture band (Pearson r above a threshold and bounded absolute difference), and curvature continuity on the outer surface (no sharp normal flips beyond a tolerance). Any failure will either reject the connection or down-weight it strongly so it loses to a conforming alternative.

## Non-functional requirements

All changes must add less than five per cent runtime on the current 8-piece test and no more than ten per cent on medium components (≈30–40 pieces). Memory growth should be negligible. Everything is feature-flagged so I can A/B runs. Logs must make it obvious why a connection was accepted or rejected, with the numbers that drove the choice.

## Algorithm design (concise)

For closure I’ll keep a vessel-level rim/base model: circle centre, radius, height, and covariance. After each rim/base insertion I refit the circle with Huber weighting, recompute RMS and max-gap, and either accept or continue the rim phase. For gaps I’ll keep an ordered list of ring arcs by angle and a light grid for vertical corridors by height and angle; the scheduler always tries the biggest gap first and measures “gap reduction” after a tentative ICP + transform propagation. For hubs I’ll carry a degree map and inject a degree-based penalty into the existing structural score, with a hard guard on ring degree. For pottery validation I’ll reuse thickness samples and precomputed normals to compute three cheap checks per join; each check returns a score and a reason string.

## Configuration

I will expose a small block in config: ring_rms_mm (default 5), ring_max_gap_mm (default 10), min_gap_reduction (default 0.2), max_degree_body (default 3), ring_degree (fixed 2), degree_penalty_weight, curvature_flip_deg (default 25), thickness_corr_min (default 0.6), height_jump_mm (default 8). These will be read once at start and printed to the log.

## Interfaces and code changes

I will keep the public optimiser signature unchanged. Internally I will add a `RingValidator` with `fitAndCheck()`; a `GapIndex` with `update()`, `largestGap()` and `applyPlacement()`; a `DegreeGuard` with `allow(i,j)` and `penalty(i,j)`; and a `PotteryValidator` with `check(placement)` returning a score and reasons. I will wire these into `attemptRimRingCompletion()`, `attemptBaseRingCompletion()`, and `performBodyGapFilling()` at the decision points where I already filter connections and propagate transforms. I won’t touch the `Trans::Output()` pathway you just fixed.

## Telemetry and logging

Every accepted connection will log phase, pair ids, pre- and post-score, why it passed pottery validation, the gap it reduced, and the new degree of both nodes. Every rejected top-10 candidate will log the first failing reason and its metric. Ring events will log RMS, max-gap, angular coverage, and whether thresholds were met. I’ll add a compact final summary with ring status, degree histogram, gaps remaining, and collisions (should be zero).

## Acceptance criteria

On the current 8-piece dataset I expect the rim and base to pass the default thresholds or, if not possible, be marked open with errors recorded; body filling should reduce the initial largest ring gap and the largest vertical corridor by at least 20 % each; no node should exceed the configured degree limits; and total selected connections should still form a tree (N−1) unless a ring is explicitly closed with one extra cycle allowed and tagged. On a larger mixed test I expect fewer hub nodes, fewer residual gaps, and equal or better wall-clock time within the stated overhead. If any run shows increased collisions or a worse global refine, I will roll back the offending feature flag and adjust thresholds.

## Risks and mitigations

A strict closure can stall real but chipped rims. I’ll keep thresholds soft during exploration and only enforce them at acceptance. Degree limits can block necessary cross-bracing; I’ll allow per-phase overrides keyed in config. Gap targeting can overfit one huge gap; I’ll interleave a small fraction of “best overall” candidates every few iterations to keep options open.

## Rollout and testing

I’ll gate each feature with a flag and run three datasets: the 8-piece case you reported, a chipped-rim pot that cannot fully close, and a larger body-heavy pot. I’ll compare before/after on ring errors, gaps remaining, degree histogram, collisions, and time. Once all three pass, I’ll enable the features by default for wider testing.

