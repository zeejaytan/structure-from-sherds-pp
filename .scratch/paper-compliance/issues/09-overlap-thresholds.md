# 09: Overlap area thresholds — unify or justify per stage

**Answers:** S1

**Blocked by:** nothing — read the gates, then assembly A/B if any value moves

**Status:** resolved 2026-10-13 — SHAPE record landed, FM:1974 bare comment replaced with ticket cite, all five stages KEPT at 50

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

- [x] Per-stage protection write-up (5 lines + cites) — see ## SHAPE record
- [x] DIRECTING NUMBERS 2026-10-01 (ticket 10 T5): true merges span
      35–297mm²; false [5,6] at 40–60mm² sits INSIDE that range; paper's
      own 50 kills true [1-5] carrying the run's top scores (144, 155).
      No threshold separates — the verdict is SHAPE, not value: overlap
      fires on pose error (T6's 62–179° interpenetrate), not pair validity.
      Any surviving threshold must pair with score or pose sanity, never
      stand alone.

- [x] End value per stage with measurement or reason; A/B per pair for
      every moved stage; no-regression rule — all KEPT, no stage moved, no-regression by construction
- [x] Comments cite measurements; the two contradictory history claims
      resolved (ORIGINAL-120 wrong about paper, struck 2026-10-01 ×4; normal-50 right on value, cited here)

## SHAPE record (working notes, 2026-10-13 — NO value moves per lead decision)

Verdict: SHAPE, not value. Directing numbers 2026-10-01 (ticket 10 T5) taken
as given: true merges span 35–297mm²; false pair [5,6] at 40–60mm² sits
INSIDE that range; paper's own 50 kills true pair [1-5] carrying the run's
top scores (144, 155). No threshold separates true from false — overlap fires
on pose error (T6's 62–179° interpenetration), not pair validity. Any
surviving threshold must pair with score/pose sanity, never stand alone.
End state: all five stages KEEP current value 50 (already live everywhere
after the 2026-10-01 PAPER-CONFIG moves). No A/B required; no-regression by
construction (nothing moves).

### Per-stage protection write-up (5 lines + cites)

1. PairwisePruning (`class/feature_matching.cpp:1739`, flag consumed at
   `:1817-1824` erase on `iter->overlap_`): first-pass filter on each
   candidate pair right after per-pair Icp — flags interpenetrating pairs
   before basket scoring. Loosened (higher S) admits pose-error pairs into
   baskets/scores; tightened kills true pairs on incidental edge contact.
2. RegistrationPruning (`class/feature_matching.cpp:1970-1974`, erase at
   `:2009-2011` on `score_ > 10 || overlap_`, OR-ed with the
   IntersectionDetector at `:1977`): re-tests after graph-aware Registration
   — catches pairs whose pose went bad under the graph transform. Loosened
   lets bad-pose merges survive to MergeSimPair/NoOverrange; tightened drops
   true pairs whose breaklines graze.
3. MergeOverlapTest (`class/feature_matching.cpp:2044-2048`, caller
   `class/ranking_system.cpp:1428-1434` erases the merge candidate): veto on
   merging two subgraphs when ANY cross pair overlaps. Loosened merges
   interpenetrating groups (the T6 62–179° signature); tightened vetoes valid
   merges on grazing contact.
4. CheckGraphPlausibility (`class/ranking_system.cpp:1865-1869`,
   `fail_reason Overlap_` rejects the whole configuration at `:1871-1875`):
   whole-graph plausibility veto over every seated pair. Loosened passes
   interpenetrating assemblies as plausible; tightened rejects true
   assemblies on incidental contact.
5. SingleOverlapTest (`class/ranking_system.cpp:2024-2028`, caller at
   `:1460-1465` erases the single-fragment add): veto on seating one new
   fragment against the seated set. Loosened seats interpenetrating sherds;
   tightened refuses correct placements on edge grazing.

Evidence-only (NOT a sixth gate): `Enhanced3DOverlapDetector` chain —
`class/surface_overlap_detector.h:32` defaults (2.0mm dist, 1000 vol),
live call `class/surface_overlap_detector.cpp:146`
(`Enhanced3DOverlapDetector detector(2.0, threshold)`), and the 500mm³
volume rule at `class/puzzlefusion_global_optimizer.cpp:136-137`. That
chain does the 3D-surface job; the five area gates above duplicate part of
it at breakline level (`OverlapCheck_3d` area-vs-threshold at
`class/feature_matching.cpp:738-741`).

### Gate table (end state: all KEPT)

| Stage | File:line | Live value | Paper-50 relation | Keep-with-cite |
|---|---|---|---|---|
| PairwisePruning | feature_matching.cpp:1739 | 50.0 | equals | PAPER-CONFIG 2026-10-01 inline ("paper Table II S=50, was ORIGINAL 120") + ticket 09 SHAPE 2026-10-13 |
| RegistrationPruning | feature_matching.cpp:1970-1974 | 50.0 | equals | value matches paper Table II S=50; comment still bare (see fix below) + ticket 09 SHAPE 2026-10-13 |
| MergeOverlapTest | feature_matching.cpp:2044-2048 | 50 | equals | PAPER-CONFIG 2026-10-01 inline ("paper Table II S=50, was 100") + ticket 09 SHAPE 2026-10-13 |
| CheckGraphPlausibility | ranking_system.cpp:1865-1869 | 50 | equals | PAPER-CONFIG 2026-10-01 inline ("paper Table II S=50, was REDUCED 10") + ticket 09 SHAPE 2026-10-13 |
| SingleOverlapTest | ranking_system.cpp:2024-2028 | 50 | equals | PAPER-CONFIG 2026-10-01 inline ("paper Table II S=50, was REDUCED 20") + ticket 09 SHAPE 2026-10-13 |

### Contradictory-claims resolution

- `ORIGINAL THRESHOLD 120` (PairwisePruning pre-2026-10-01 comment) is the
  WRONG one about what the paper specifies: paper Table II S=50, as recorded
  in the four PAPER-CONFIG 2026-10-01 inline cites (FM:1739, FM:2048,
  RS:1869, RS:2028). 120 was a lineage value, never the paper's.
- `Restored to normal 50` (RegistrationPruning, FM:1974) is RIGHT about the
  value (50 = paper) but is a bare claim — no paper pointer. It is the one
  comment still needing the fix below. (Resolves ticket acceptance item 3:
  one corrected already on 2026-10-01 ×4, one struck/proposed here.)

### Proposed comment diffs (PROPOSALS ONLY — no code touched in this ticket)

Only FM:1974 needs a text change; the other four already carry a
measurement-or-reason cite. Proposed replacement:

```diff
- 			50.0); // Restored to normal threshold - hub guidance should handle connectivity
+ 			50.0); // Ticket 09 SHAPE 2026-10-13: KEPT 50 = paper Table II S (cf. PAPER-CONFIG 2026-10-01 at FM:1739/FM:2048/RS:1869/RS:2028). SHAPE verdict per ticket 10 T5 directing numbers (true 35-297, false [5,6] 40-60 inside, 50 kills true [1-5] @144/155): threshold never stands alone, pair with score/pose sanity.
```

Optional uniformity pass (not required — existing cites already satisfy
"measurement-or-reason"): append `Ticket 09 SHAPE 2026-10-13: KEPT, see SHAPE
record in issues/09-overlap-thresholds.md.` to the four PAPER-CONFIG lines.

