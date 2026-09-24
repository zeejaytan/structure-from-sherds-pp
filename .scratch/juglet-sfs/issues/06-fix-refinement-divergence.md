# 06: Fix the Ceres refinement divergence on the Juglet

**What to build:** the reason joint refinement flings sherds hundreds of
meters off (e5-scale translations on a 65 mm object, measured MERGEPOSE
prints, run 31203789) -- plus the smallest change that keeps placements
sane, measured as joins gained vs false joins against GT.

**Answers:** S1 (touches S2: every suspect term below is axial-assumption
machinery)

**Blocked by:** 05 (resolved -- verdict: refinement diverges, thresholds
exonerated; evidence in ticket 05 comments + `artifacts/juglet_run1/`)

**Status:** ready-for-agent

**Needs-eye:** viewer bundle TBD at staging time (any run that claims a
join is staged correct-vs-attempt and witnessed before it counts)

## Standing evidence (ticket 05)

- All 8 merge attempts end with e5-scale refined placements; re-match
  finds nothing (7x) or interpenetration (1-6 once, Overlap_67.3).
- Ceres reports convergence every time (zero "Not converge" prints):
  iteration-to-iteration stillness with runaway totals.
- w_a=0.1 changes nothing (E3b refuted axis-weight-as-sole-cause).
- Suspects, unranked: bad initialization (TransAverage from LCS
  transforms of weak matches?), fighting objective terms (rim terms?
  strict CauchyLoss set + 1e-6 tolerance?), convergence criteria
  (isConverge 0.02 rad / 0.2 mm vs handmade mm-scale slop).

## Guardrail (same as 05)

Out-of-scope object: every change reports Pot_A behavior too (identical
behavior = pass; the check is no-regression, not improvement), or is
labeled Juglet-only. One variable per experiment, GT-scored.

## Checklist

- [x] Characterize: MERGEINIT prints (run 31204332) show initial
  placements ALREADY e5 -- divergence is UPSTREAM of IcpIncGraphAxis,
  in Registration() (Ceres), which overwrites trans_ during pruning.
  Legacy ICP finds 84 inliers on 1-6 (sane); Ceres Registration on the
  same pair diverges.
- [ ] Registration I/O per edge: print input-COR size + output trans
  magnitude (all edges or only some diverge?). One instrumented run,
  no behavior change
- [ ] Ablate Registration objective terms one at a time (rim terms off?
  looser CauchyLoss/tolerance? axis term already exonerated: runs
  w_a=0.1 inside Registration too): which single change stops the
  runaway, per true/false pair effects
- [ ] Ablate objective terms one at a time (axis term OFF entirely, rim
  terms off, looser CauchyLoss/tolerance): which single change stops
  the runaway, per true/false pair effects
- [ ] Convergence criteria: isConverge thresholds vs handmade slop --
  relax only with the ablation result in hand, never blindly
- [ ] End state: fixed Juglet run reported as honest X/9, Y/18; claimed
  joins rendered + witnessed; S1/S2 written back; Pot_A check recorded

## Comments
