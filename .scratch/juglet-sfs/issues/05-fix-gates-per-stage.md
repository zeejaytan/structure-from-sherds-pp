# 05: Per-stage failure causes on the Juglet, with validated fixes

**What to build:** for every stage ticket 04 named, the WHY on this object
plus the smallest change that lets true pairs through -- each measured as
true pairs gained vs false pairs admitted against GT, never by assertion.

**Answers:** S1 (touches S2: every gate below is axial-assumption machinery)

**Blocked by:** 04 (resolved -- verdict + evidence in
`artifacts/juglet_run1/`, run logs, ticket 04 comments)

**Status:** ready-for-agent

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

- [ ] Matcher: sensitivity of feature-match counts to Q_size tolerances
  ({0.15,0.15,0.15,0.2} today) and to input frames (PotSAC vs consistent
  vessel axis) -- report per true/false pair, not totals. WHY do 12/18
  true mates get zero matches: frames, features, or both, with numbers
- [ ] axis_angle gate: sweep 0.436 rad upward on the Juglet run; report
  true pairs admitted vs false pairs admitted at each step, plus Pot_A
  no-regression check. Proposed fix = the knee value, if one exists
  (a gate with no knee is a redesign question, not a tuning one)
- [ ] CountPCInlier(7.0, 1.5) + overlap gates: same sweep treatment on
  the pairs the axis gate admits; name which pairs each sub-gate kills
  (the instrumented prints ticket 04 left open)
- [ ] SortRoot zero-exclusion: with the above fixes in, confirm sherds 2
  and 9 enter root graphs on merit (score > 0 from real inliers), not by
  lowering the bar to zero
- [ ] End state: a retuned Juglet assembly run reported as honest
  X/9 sherds, Y/18 edges with the repaired scorer; every claimed join
  rendered correct-vs-attempt and witnessed; S1/S2 written back with
  what transferred to axial ware and what did not

## Comments

- 2026-09-21 (ticket 05 executing): env gates committed directly in
  tracked code (no patch files needed here) + [GATE] once-per-run prints
  (verifiability rule); run scripts pass SFS_* through (empty-safe).
  E0 baseline (rebuild + defaults, 30900057): same zero (4 singletons
  this time -- membership jitters again), honest 0/9 + 0/18 confirmed
  working. NOTE: no [GATE] prints in E0 log is EXPECTED, not stale code
  (prints sit inside per-edge loops; with no edges surviving to
  per-graph filtering there is nothing to print about -- itself a
  finding: per-graph LCS lists are empty).
- Code-reading finds while E0 ran (all pinned version): per-graph
  matching is a SECOND LCS variant (LCSGraphBuilding) differing by ONE
  line -- matched_index exclusion; RegistrationPruning gates are
  axis_angle 0.436 + score>10 + overlap/intersection; SortRoot excludes
  zero-inlier shards from roots (measured P2=P9=0). E1 (axis 0.8,
  30901668) running; E2 (1.2) follows sequentially (timestamped result
  dirs would collide in parallel). (assembly
  sources are tracked -- no patch files needed here): SFS_AXIS_ANGLE_MAX
  (default 0.436), SFS_PC_DIST/NORM (7.0/1.5), each with a once-per-run
  [GATE] print (verifiability rule); run scripts pass them through
  (empty-safe). Slurm exports submit-env by default, so matrix runs set
  vars inline at submit. E0 baseline (rebuild + defaults) as 30900057
  to prove the rebuild changed nothing; then E1 axis 0.8, E2 axis 1.2
  via the no-rebuild rerun script. Pot_A no-regression deferred to the
  final config only (its dataset is unit-broken; check = identical
  behavior, not improved).
