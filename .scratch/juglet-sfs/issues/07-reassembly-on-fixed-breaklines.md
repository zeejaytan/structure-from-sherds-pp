# 07: Rerun assembly on the fixed breaklines

**What to build:** a fresh Juglet assembly verdict on REAL rim curves
(10-27 mm breaklines, ticket 02 resolved for real this time) to replace
the dot-input 0/18, using the unchanged pipeline + repaired scorer.

**Answers:** S1 (touches S2 if handmade ware assembles -- or doesn't)

**Blocked by:** 02 (resolved -- bundle verified by the extent gate, job
31281384 COMPLETED: 18 surfaces, 18 breaklines, 9 axes, 9 meshes, GT)

**Status:** ready-for-agent

**Needs-eye:** correct-vs-attempt pair staged under
`visual-qa/viewer/pairs/` for EVERY claimed join (none may be reported
on numbers alone -- ticket 03's rule stands); a null result re-stages
the strongest miss as before

## Standing evidence (all conditional on dot inputs until re-earned)

- 0/18 joins, stably; membership jitters (7/6/4/8 singletons).
- Matcher: 12/18 zero matches; 5 pairs with ICP inliers, none merged.
- Merge death: Ceres placements e5 (first-step explosion, deterministic,
  single-threaded); merge table empty; Score_ fail; singletons.
- Ruler repaired mid-stream: honest 0/9 + 0/18 scorer; patch discipline
  (one-hunk files, marker checks); axis scatter 51 deg on independent
  axes (STANDS -- unaffected by dots).
- Open mechanism when this was written: WHY the first Ceres step leaps
  (unit-mixed residuals? unconstrained directions? false LCS matches?).

## Checklist

- [ ] Rerun `run_juglet.sbatch` (rebuild + run) on the fixed bundle; no
  code changes first -- measure what real curves change by themselves
- [ ] Report honest sherd/edge scores + per-pair fate vs GT (which pairs
  match, refine, merge, or die where -- same analysis as ticket 03)
- [ ] If joins are claimed: stage correct-vs-attempt, witness it, then
  report. If zero again: compare WHERE it dies vs the dot-input runs
  (same gate or new one?) before any further tuning
- [ ] Write back into `intent/S1-does-it-connect-anything-real.md`
  (and S2): the dot-conditional verdicts below are either confirmed on
  real curves or retracted with the differing evidence named

## Comments
