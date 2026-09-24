# 05: Per-stage failure causes on the Juglet, with validated fixes

**What to build:** for every stage ticket 04 named, the WHY on this object
plus the smallest change that lets true pairs through -- each measured as
true pairs gained vs false pairs admitted against GT, never by assertion.

**Answers:** S1 (touches S2: every gate below is axial-assumption machinery)

**Blocked by:** 04 (resolved -- verdict + evidence in
`artifacts/juglet_run1/`, run logs, ticket 04 comments)

**Status:** resolved

## Verdict (2026-09-24, ticket 05 -- for S1)

Per-stage causes, each with its test (mechanism ranking, strongest first):

1. **Joint refinement diverges placements (operative).** Every merge
   attempt ends with pieces hundreds of meters off (MERGEPOSE prints
   e5-scale translations on a 65 mm object), so post-refinement
   re-matching finds nothing (7/8 merges) or interpenetration (1/8:
   pair 1-6, 67 overlap-area units). Ceres always reports convergence
   (zero "Not converge" prints) -- it converges *somewhere absurd*.
   Refuted as sole cause: axis weight 1.0 -> 0.1 changes nothing, so
   other strict terms (CauchyLoss set, 1e-6 tolerance, rim terms) or bad
   initialization share the blame. NEXT: ticket 06 (fix refinement).
2. **Matcher blindness (contributory).** 12/18 true mates get zero
   feature matches: quantized cylindrical profiles computed in
   51 deg-scattered per-fragment axis frames (27 deg large pieces,
   65 deg small). Consistent-frame counterfactual shifts patterns but
   yields no joins -- frames alone insufficient.
3. **Pruning/merge gates (downstream, mostly unreached).** axis_angle
   0.436, score>10, overlap/intersection, CountPCInlier(7.0,1.5),
   SortRoot zero-exclusion (P2=P9=0 out) -- all named with thresholds,
   but the 0.436/PC sweeps tested dead code (PrepareNextStep path never
   reached on Juglet runs): VOID results, recorded as such. These gates
   matter only once merges get that far.
4. **Scorer (fixed).** Commented-out method-graph requirement restored;
   honest 0/9, 0/18 verified (run 30895279).

**Needs-eye:** viewer bundle TBD at staging time (any retuned assembly that
claims a join is staged correct-vs-attempt and witnessed before it counts)

## Standing evidence (ticket 04)

- 0/18 joins, stably; 12/18 true mates zero feature matches; 5 true pairs
  reach ICP with inliers (1-6: 84) yet none merges.
- Gates, with thresholds: RegistrationPruning axis_angle > 0.436 rad
  (`feature_matching.cpp:2088`); RemoveEdgeUsingPCInlier overlap /
  CountPCInlier(7.0, 1.5); SortRoot zero-exclusion (`ranking_system.cpp:742`,
  measured P2=0, P9=0); honest scorer restored (0/18 verified, run 30895279).

## Guardrail (read before tuning anything)

The Juglet is OUT-OF-SCOPE material (handmade/handled, S2): tuning gates
until IT assembles risks overfitting one object and breaking axial ware.
Every proposed change must ALSO report its effect on Pot_A behavior (no
regression on in-scope material) or be labeled Juglet-only. A threshold
that only works here is a finding about scope, not a fix to ship.

## Checklist (one variable per experiment, GT-scored each time)

- [x] Matcher: frames tested via counterfactual (consistent vessel axis:
  patterns shift, no joins -- frames insufficient); 12/18 zero-match
  stands as features-in-scattered-frames. Q_size sweep DEPRIORITIZED
  with rationale (same values feed both LCS passes; loosening adds
  false matches without fixing merges -- revisit only if refinement
  is fixed and matching becomes the binding constraint again)
- [x] axis_angle gate: named (0.436 rad) with code location; sweeps at
  0.8/1.2 VOID (dead PrepareNextStep path on Juglet runs -- recorded,
  not hidden). Gate relevance re-scoped: matters only once merges
  reach that path
- [x] CountPCInlier(7.0, 1.5) + overlap gates: named with locations;
  same dead-path status as above on current runs
- [x] SortRoot zero-exclusion: evidenced (P2=P9=0 measured); consequence
  of upstream starvation, not an independent cause
- [ ] End state (MOVED to ticket 06): a retuned/fixed Juglet assembly
  run reported as honest X/9 sherds, Y/18 edges; every claimed join
  rendered correct-vs-attempt and witnessed; S1/S2 written back

## Comments

- 2026-09-24: MERGETABLE verdict (run 31200856): 7/8 merges re-match ZERO
  post-refinement (isEdgeRemoved on empty table); 1 merge (pair 1-6: 27
  raw, 14 kept, Table set) dies at Overlap_67.3/score 28 instead. So two
  death modes: scattered-after-refinement vs interpenetrating-after-
  refinement. Both indict the Ceres placements, not the thresholds.
  (w_a=0.1 already refuted as sole cause.) Discriminator ran as
  31203789: refined placements are e5-scale translations -- genuinely
  wrong, Ceres diverges. See Verdict above.

- 2026-09-24: E3b REFUTES the axis-weight theory -- w_a=0.1 (verified
  print) still 0/18. Kill the "refinement scatters placements" story as
  the sole cause. Remaining fork: (a) Ceres placements genuinely far
  apart (something else diverges them: strict CauchyLoss/tolerances?),
  vs (b) re-match thresholds (RejectOutlier 2.0/0.85 + >8) too strict
  for handmade curves. Instrumented MakeMultiCorres to print raw vs
  kept counts per pair (diagnostic run 31200856 rebuilding now): large
  raw + ~0 kept => RejectOutlier is the killer (tune it); ~0 raw =>
  placements scattered (dig refinement inputs).

- 2026-09-21: E3 (30902802) gives the operative gate: all 8 merge attempts
  die at CheckGraphPlausibility FIRST gate (Score_, graph_score_=-1
  sentinel). Traced -1 to IcpIncGraphAxis: NOT non-convergence (zero
  "Not converge" prints) but isEdgeRemoved-true on the post-refinement
  re-match table. MakeMultiCorres re-matches after Ceres moves pieces
  and needs >8 survivors past RejectOutlier(2.0, 0.85) -- gets none.
  Theory with teeth: Ceres carries AxisConsistency at weight 1.0 (10x
  the original 0.1) against 51 deg-scattered axes -- refinement may
  scatter placements past re-matching. Test: SFS_AXIS_WEIGHT=0.1
  (original value) env knob committed (+print). E3a baseline rebuild
  30905301 first; E3b (0.1, no rebuild) follows on the same binary.

- 2026-09-21: E1/E2 VOID -- critical self-correction. RegistrationPruning
  and RemoveEdgeUsingPCInlier (the env-gated functions) run ONLY in the
  PrepareNextStep path, which our runs never reach (no extensions survive
  to select). The 0.8/1.2 sweeps tested dead code; the "no change" result
  proves nothing about the axis gate. Lesson logged: verify the gate
  EXECUTES (prints fire) before varying it -- the [GATE] silence was the
  signal, misread as "empty lists" (also true, but downstream).
- 2026-09-21 redirect: merges ARE attempted (8x "Graph merging step") and
  die inside BuildState's merge path, which ends in CheckGraphPlausibility
  (score<0 / overlap / profile gates -- ALL SILENT: fail_reason recorded
  but never printed). Instrumented it (verdict + score print, 2 lines).
  E3 (rebuild + run, 30902802) reads WHY each merge fails; that names the
  operative gate instead of the candidate list.

- 2026-09-21 (ticket 05 executing): env gates committed directly in
  tracked code (no patch files needed here) + [GATE] once-per-run prints
  (verifiability rule); run scripts pass SFS_* through (empty-safe).
  E0 baseline (rebuild + defaults, 30900057): same zero (4 singletons
  this time -- membership jitters again), honest 0/9 + 0/18 confirmed
  working. NOTE: no [GATE] prints in E0 log is EXPECTED, not stale code
  (prints sit inside per-edge loops; with no edges surviving to
  per-graph filtering there is nothing to print about -- itself a
  finding: per-graph LCS lists are empty).
- Code reading (pinned version): per-graph matching is a SECOND LCS
  variant (LCSGraphBuilding) differing by ONE line (matched_index
  exclusion of already-matched points); RegistrationPruning gates are
  axis_angle 0.436 + score>10 + overlap/intersection; RemoveEdgeUsing-
  PCInlier gates overlap + CountPCInlier(7.0,1.5); SortRoot excludes
  zero-inlier shards from roots (measured P2=P9=0).
- 2026-09-21: E1 (axis gate 0.8) zero change: 0/18, 7 singletons. As
  predicted by the empty-list reading -- the gate never fires because
  per-graph match lists are empty before it. E2 (1.2) running as
  30902120 to complete the sweep; expectation set accordingly.
