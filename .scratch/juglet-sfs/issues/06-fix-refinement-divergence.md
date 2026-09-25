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

- 2026-09-25: cause NARROWED to capture range. MERGEINIT translations
  sane (2-15 mm); MERGEPOSE finals drifted tens of mm off; post-refinement
  re-match ~empty. Init error (~10 mm from LCS feature matches) EXCEEDS
  the fork-tightened robust kernels (Cauchy 1.0/0.5 vs upstream 5.0/2.0):
  saturated losses, weak gradients, 200 iterations of drift instead of
  convergence. Upstream scales restored behind SFS_CAUCHY_DIST/NORM
  (defaults keep current behavior; +print). Running 5.0/2.0 as 31290446
  (rebuild): joins forming = capture range was the binding constraint;
  still zero = drift has another driver (rim terms? init quality?).

- 2026-09-24: trajectory verdict (run 31209597): accumulated motion is
  e5 ALREADY at iteration 0, then wanders (652K->622K; 571K->460K over
  23 iters, never converging sanely). Not gradual drift: a first-step
  explosion. Trans/s zeroed per iteration, so each solve leaps from
  zero to hundreds of meters with true correspondences present.
  Suspect: unit-mixed residuals or unconstrained directions in the
  Ceres problem, NOT thresholds/weights. Running per-edge COR counts
  (31209882) to see input sizes at explosion time.

- 2026-09-24, scale mismatch (user's point, verified): Pot_A piece 1 =
  843k points; Juglet pieces 2k-24k (30-300x sparser). Paper + upstream
  tuned on Pot_A-scale dense data; the fork TIGHTENED fixed thresholds
  (see three-way table in ticket comments/thread) instead of scaling
  them. Adaptivity exists ONLY in preprocessing segmentation
  (Nov-2025 dynK/dynMinCluster -- fork-only, absent upstream); assembly
  gates stayed fixed. So thresholds assume dense data the Juglet
  doesn't have. Fix direction implication: scale-aware gates (or
  upstream restores), not blind loosening.
- Fork-vs-pristine scope: only 10 tracked files differ
  (data_path/data_structure/feature_matching/ranking_system/
  reconstruction + headers, main.cpp); rest is NEW fork subsystems
  (agglomerative, physics, GGCE, validators) not on the Juglet path.

- 2026-09-24: the LCS-only change went into the WRONG overload (6-arg
  Registration; the pruning path uses the 5-arg one with MakeSingleCorres
  -- caught because the confirmation print never appeared, exactly the
  discipline that caught the silent-patch saga). Same treatment applied
  to the 5-arg else branch. Running as 31206929 with SFS_LCS_ONLY=1
  (rebuild + run). If pair 1-6 merges: first Juglet join, mechanism and
  fix proven together.

- 2026-09-24: feedback theory + first fix attempt. After axis-alignment
  centers ALL sherds at origin, dense nearest-neighbor re-matching
  manufactures false correspondences everywhere; refitting them each
  Ceres iteration feeds divergence (e5 endpoints vary run to run =
  random walk, not a wrong minimum). SFS_LCS_ONLY=1 reuses the LCS
  index pairs every iteration instead (fixed pairings tracking moving
  pieces -- proper coarse-to-fine). Submitted 31206521 WITHOUT the
  container passthrough (would have run at defaults); caught,
  cancelled, fixed scripts, resubmitted as 31206551. If 1-6 merges:
  first Juglet join ever, mechanism proven.

- 2026-09-24, three-way audit (paper + upstream pristine + our fork)
  pinpoints the tampering. Paper: P2P-then-P2L ICP, Cauchy kernels,
  30-deg normal pruning, grouping at 25 deg / 20 mm. Upstream code:
  Cauchy(5.0/2.0...), RejectOutlier(20, 0.7), cor > MINIMUM/2 (=0),
  w_a 0.1, isConverge(0.05, 1.0)/(0.1, 2.0). Our fork tightened nearly
  everything ("FIXED" comments): (2.0, 0.85), >8, w_a 1.0 (in one of
  two spots; upstream IcpFine already 1.0), Cauchy down to
  (1.0/0.5/1.0/1.0), isConverge (0.02, 0.2). On precise thrown ware
  that still passes; on handmade mm-slop it cannot.
- KEY REFINEMENT from the numbers: re-match knobs are MOOT, not next.
  1-6's re-match already keeps 17-27 (passes >8!); other pairs have
  raw=0 because pieces sit kilometers apart POST-refinement. And the
  e5 MERGEINIT translations prove pairwise Registration() outputs --
  upstream of everything -- already diverge, while legacy ICP on the
  SAME pair finds 84 sane inliers. So: legacy ICP sane, Ceres
  Registration divergent, joint step inherits garbage. Running now
  (31206095): per-edge Registration I/O print (input COR size is in
  the existing COR prints; output t_norm + axis_angle added) to see
  whether it diverges on all edges or only some.
