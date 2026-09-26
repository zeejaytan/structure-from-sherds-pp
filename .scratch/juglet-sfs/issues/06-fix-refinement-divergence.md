# 06: Fix the Ceres refinement divergence on the Juglet

**What to build:** the reason joint refinement flings sherds hundreds of
meters off (e5-scale translations on a 65 mm object, measured MERGEPOSE
prints, run 31203789) -- plus the smallest change that keeps placements
sane, measured as joins gained vs false joins against GT.

**Answers:** S1 (touches S2: every suspect term below is axial-assumption
machinery)

**Blocked by:** 05 (resolved -- verdict: refinement diverges, thresholds
exonerated; evidence in ticket 05 comments + `artifacts/juglet_run1/`)

**Status:** resolved -- **not the cause.** The e5 divergence was an
artifact of degenerate dot-input (fixed in ticket 02). On real rim curves
there is no divergence. The actual binding constraint is a gate the
Juglet's breaklines cannot pass even at ground truth. Verdict and
evidence in "Verdict" below; fix direction moved to preprocessing.

**Needs-eye:** staged pair `juglet_sfs29` (true 2-9 join at GT vs the
machine's 110-deg-off placement) is **staged but NOT yet witnessed** --
conservator read pending. No join is claimed anywhere in this ticket, so
nothing here depends on it; the visual confirmation of "true placement vs
machine placement" is still owed before the ticket is closed out.

## Verdict (2026-09-26): the refinement was never the problem

**The question this ticket asked is answered NO -- and the honest answer is
that the premise was wrong.** The e5-scale divergence that opened the
ticket came from degenerate dot-input breaklines (ticket 02). With real
rim curves (ticket 07) there is no divergence: merges start sane
(2-15 mm) and the walk is non-convergence, not explosion. Chasing it cost
real time and the trail stays for that reason.

**What the evidence now supports (two independent tests):**

| Test | Pot_A (works) | Juglet (fails) |
|---|---|---|
| 1. Static: true mates at GT vs the strict gate | **15/15 pass** | **0/18 pass** |
| 2. Dynamic: true pair at GT, oracle-init through the binary | **2-5 accepted, kept** (score 732→735) | **6-7 REJECTED**, 0 inliers, Score_ -1 |

Test 2 is the decisive one. It hands the real binary the **exact ground-truth
placement** of a genuinely-touching pair and lets registration, refinement,
gates and scorer run unmodified. Juglet pair 6-7 is rejected with zero
matching points. The same harness, same binary, same code path accepts
Pot_A pair 2-5. **The instrument is validated; the difference is the
material.**

**The reason, in the method's own terms.** The join gate requires a
point on one sherd's breakline within 2 mm of a point on its neighbour's,
with surface normals agreeing (dot > 0.85). On the Juglet:

- **Wall thickness, not wear.** Wall ≈1.8 mm. Extraction traces the
  **inner** face on one sherd and the **outer** face on its neighbour:
  1.7 mm apart, normals **67-129 deg opposed** (means 129 deg apart,
  [0.68,0.43,0.6] vs [-0.66,0.43,-0.62]). Two different surfaces are
  being compared as if they were the same. Pot_A's thin wheel-thrown
  breaks put both traces on one face, 0.09-1.30 mm apart, normals
  genuinely agreeing.
- **Wear: traces miss the seam.** Best per-pair trace gaps 0.17-30 mm
  where mating edges touch at 0.02 mm; 8/18 answer-key edges are not
  physical contacts at all (3.3-18.9 mm mesh gaps).

**Why no threshold fixes it:** the gate rejects truth itself. Widening
(20 mm/0.7) admits points on 110-deg-wrong poses; narrowing keeps
nothing. SFS_NORMAL_ABS=1 (matching robust_icp's own abs(dot)) was
refuted at runtime: 0/18. There is no setting where truth passes and
falsehood fails -- verified, not asserted.

**Honest denominator: 10, not 18.** Eight answer-key edges are not
physical contacts. Real score is **0/10** joinable edges, still zero.

**Fix direction (moved):** **preprocessing, not assembly.** Broken-edge
extraction must trace both faces and pair across the wall, and segments
must cover the seam. Acceptance test for any new bundle:
`artifacts/juglet_run1/gate_probe_b0.py <bundle>` -- true mates must pass
the gate at GT. Until a new extraction passes it, no assembly tuning is
worth attempting.

**Also found, separate defect:** the beam proposes a **different pair
every run** on identical input (6-7, then 5-7, then 3-7, then 7-9). On a
9-sherd pot true neighbours should be stable. Pairing instability, not
yet attributed -- opened as ticket 08.

**Weight this can bear:** one object, one failure mode. It shows the
method fails on **this kind of break** (thick, handmade, eroded). It does
NOT show it fails on all handmade pottery, and a thick wall on an
otherwise clean break is untested. Control caveat: the NURBS Pot_A
bundle is frame-broken (breaklines not in GT frame, 59-pt files) and was
skipped; the control uses `original_samples` via the POT_A_ORIG block.

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

**Guardrail satisfied — and here it doubles as the control.** Pot_A was
re-run twice through the full instrumented binary: baseline 8/8 pieces,
score 732, 1/15 edges, 2/8 sherds; oracle arm 8/8 pieces, score 735,
1/15, 2/8. The **baseline is bit-identical to the pre-instrumentation
Pot_A run**, confirming the prints and the oracle hooks changed no
behavior. Every knob added in this ticket (SFS_REJECT_DIST/NORM,
SFS_NORMAL_ABS, SFS_CAUCHY_DIST/NORM, SFS_ORACLE_*, SFS_ORACLE_INJECT)
**defaults to legacy behavior**, so the no-regression property holds by
construction, not by measurement alone. The +3 score in the oracle arm
is the true 2-5 join being admitted — the expected effect of the
intervention, and the control's whole purpose.

## Checklist

- [x] Characterize: MERGEINIT prints (run 31204332) show initial
  placements ALREADY e5 -- divergence is UPSTREAM of IcpIncGraphAxis,
  in Registration() (Ceres), which overwrites trans_ during pruning.
  Legacy ICP finds 84 inliers on 1-6 (sane); Ceres Registration on the
  same pair diverges. **[SUPERSEDED: dot-input artifact, see Verdict]**
- [x] Registration I/O per edge: TRANSAVG prints added (per-edge t_norm
  entering TransAverage). Sane inputs (7-16 mm) on real curves.
- [x] Ablate objective terms one at a time: axis weight exonerated
  (w_a 1.0 / 0.1 / 0, all identical 0/18); Cauchy widths tested
  (1.0/0.5 and upstream 5.0/2.0, both 0/18 on real curves);
  SFS_NORMAL_ABS=1 refuted (0/18). Rim terms already flag-disabled.
  **No single objective term is the cause.**
- [x] Convergence criteria: not the binding constraint. Refinement
  non-convergence is real but downstream of a gate truth cannot pass.
- [x] **Test 1 (static, decisive):** GT-placed true mates vs the strict
  gate, both bundles, same instrument -- Pot_A 15/15, Juglet 0/18.
  Truth fails the gate. `artifacts/juglet_run1/gate_probe_b0.py`
- [x] **Test 2 (dynamic, decisive):** oracle-init through the real
  binary, M = A_a P_a^-1 P_b A_b^-1 verified to 1e-3 mm. Juglet 6-7
  **rejected from truth** (0 inliers); Pot_A 2-5 **accepted and kept**
  (score 732->735). Harness validated.
- [x] Void arms labelled, not hidden: first 2-9 and 6-7 oracle arms had
  ORACLE-count 0 (pair never proposed) and were discarded; pair
  injection added; driver now prints `ORACLE fired N` and marks voids.
- [x] End state: honest **0/10** joinable edges (0/18 nominal, 8
  non-contacts excluded); S1/S2 written back; Pot_A check recorded
  (no-regression satisfied by construction: every knob defaults to
  legacy behavior). Visual witness for `juglet_sfs29` still pending.

## Comments

- 2026-09-26, TEST 2 RESULT -- the proof, and the control that validates
  it. Oracle 6-7 (TRUE mates, GT-placed, seam 0.921 mm) fired TWICE:
  `ORACLE *** pair 7-6 fired t=[-0.369 0.596 -4.553]` and `pair 6-7
  t=[0.171 0.106 -4.603]`, both `MERGETABLE corPts=0 tablesum=0.000` ->
  `PLAUSIBILITY FAIL: Score_ score=-1`. **Truth handed in, truth
  rejected, twice.** Control Pot_A 2-5 (POT_A_ORIG, seam 0.479 mm)
  fired twice, was ACCEPTED and kept (score 732->735, 8/8 pieces, join
  survives into the final assembly). Same binary, same code path,
  verified intervention on both sides. Harness is sound; the material
  is the difference. Oracle math verified frame-invariant offline to
  1e-3 mm on 5 pairs before use (artifacts/juglet_run1/oracle_M_any.py).

- 2026-09-26, VOID ARMS DISCARDED (recorded, not hidden): first oracle
  attempt (6-7, 2-9) printed ORACLE-count 0 -- the beam never proposed
  those pairs, so the null said nothing. Root cause of the void: the
  override only fires on a single-edge cross merge of the named pair,
  and the beam proposed 5-7 / 3-5 instead. Two fixes: (1) SFS_ORACLE_
  INJECT=1 prepends the named pair to the candidate list (it carries NO
  placement -- identity transform, overridden downstream -- so it
  proposes the pair and nothing else); (2) the driver now prints
  `ORACLE fired N time(s)` and marks N=0 arms VOID. 2-9 remains void
  (injected pair never merged) and is NOT reported as evidence.

- 2026-09-26, PAIRING INSTABILITY (new defect, ticket 08): the beam
  proposes a DIFFERENT pair every run on identical input -- 6-7, 5-7,
  3-7, 7-9 across four runs. On a 9-sherd pot true neighbours should be
  stable. This is a separate defect from the gate finding and is not
  explained by it: it is upstream of the gate entirely.

- 2026-09-26, honest denominator corrected to 10: 8 of the 18
  answer-key edges are not physical contacts (GT mesh gaps 3.3-18.9 mm,
  zero points within 3 mm). Real joinable set is 10 edges; score is
  0/10, not 0/18. Recorded so the metric is not read as 18 misses when
  8 of them were never joinable.

- 2026-09-25: Test-1 CORRECTIONS (pipeline-faithful _0-only rerun,
  gate_probe_b0.py): the pipeline reads ONLY Breakline_0 per piece,
  so _1-inclusive numbers overstated. True inputs: Pot_A-orig 15/15
  strict at GT (6-49 inliers, traces 0.09-1.30 mm, normals agree);
  Juglet 0/18 (raw=0 everywhere; best trace gaps 0.17-30 mm, normals
  opposed; abs rescues only 2-9:6pts, 6-7:19pts). The "6-7 passes"
  claim is WITHDRAWN (its passing segments live in _1 files the binary
  never reads). Second correction: NURBS Pot_A breaklines are NOT in
  GT frame (59-pt files, coords ~(-195,-68,-1644) vs orig (65,-52,401);
  0/15 with km-scale gaps) -- NURBS bundle skipped; control uses
  original_samples via new POT_A_ORIG block. Test-2 harness built
  (commit 69925b9): TAXIS print (main.cpp), SFS_ORACLE_PAIR/M override
  at MERGEINIT (Tp = W_f M W_m^-1, M^-1 on role swap, fires only on
  single-edge cross merges, full downstream unmodified), sbatch
  passthrough. TAXIS harvest running in holder 31295030; then oracle
  run on 6-7 (predicts Score_-1 from truth at default raw gate).

- 2026-09-25: conclusion SUSPENDED by user challenge (rightly: tuning
  failures argue, they don't prove). Two-test design to PROVE
  non-viability and say why, given Pot_A works. TEST 1 (static,
  DONE): same gate probe, both bundles, GT placements. Pot_A: 15/15
  true mates pass the strict gate at GT (12-50 inliers, traces
  0.09-0.95 mm apart, raw==abs: normals genuinely agree).
  Juglet: 1/18 (only 6-7). Same instrument, same gate, opposite
  outcomes -- the gate assumes mating traces coincide <2 mm with
  agreeing surface normals; wheel-thrown thin clean breaks satisfy it
  15/15, handmade thick eroded breaks violate it 17/18 (opposite-face
  traces ~1.7 mm apart, normals 67-129 deg opposed; 8/18 GT edges not
  even physical contacts). Probes: artifacts/juglet_run1/
  gt_gate_probe.py + pot_a_gate_probe.py. TEST 2 (dynamic, designed,
  not yet built): oracle-init merge through the real binary. New knobs
  SFS_ORACLE_PAIR + SFS_ORACLE_P (the two GT vessel-frame matrices);
  at the MERGEINIT site (ranking_system.cpp:1744) replace TransAverage
  output with W_b^-1 * W_a * P_a^-1 * P_b for single-edge merges of the
  named pair only (W from the graphs' T_, all in scope). Refinement,
  gates, scorer run unmodified FROM truth. Predictions: 6-7 should
  keep ~45 pts and merge if downstream viable (blame falls on
  matcher/init alone); 2-9 should keep ~0 even from truth (gate-level
  proof). If Ceres walks AWAY from oracle init (MERGEPOSE final vs
  GT), the objective itself rejects truth -- full incompatibility
  proof. Control: identical harness on a Pot_A pair must merge+score,
  else the harness (not the method) is broken and the test is void.

- 2026-09-25: SFS_NORMAL_ABS=1 REFUTED at runtime (holder run,
  verified print, strict 2.0/0.85): 8 attempts, all pair 6-7, all
  tables empty, Score_ -1, still 0 joins. abs() only matters at GT
  placements, which refinement never reaches -- at refined placements
  (~10 mm off) the 2.0 mm DISTANCE gate keeps nothing regardless of
  normal direction. Distance binds first; normals second. Consolidated
  mechanism for 06: (1) LCS inits land ~10 mm off; (2) refinement never
  closes to <2 mm; (3) re-match keeps nothing; (4) wide gates admit
  points but placements interpenetrate/wrong-pose (2-9 passed at 110
  deg off) and overlap/score kill them; (5) at GT itself 17/18 true
  mates fail the strict gate (normals opposed across wall faces +
  traces floating 0.2-1.8 mm off-seam). No assembly gate setting passes
  truth AND filters falsehood -- the fix lives UPSTREAM in breakline
  extraction (same-face traces, seam-covering segments) and/or LCS
  init quality, both bigger than one variable. GT-gate probe saved as
  artifacts/juglet_run1/gt_gate_probe.py: acceptance test for any new
  bundle (true mates must pass the gate at GT). No Pot_A rerun owed:
  all knobs default to legacy behavior (abs off = raw dot, identical
  code path). Second measurement caveat: scorer denominator 18 counts
  8 GT edges whose meshes never touch at GT (gaps 3.3-18.9 mm, zero
  pts <3 mm) -- honest joinable set is ~10, still 0/10. Holder 31291185
  released after this run.

- 2026-09-25: STRUCTURAL finding -- truth itself fails the re-match
  gate, so no tuning of it can ever join. GT-placed breaklines of TRUE
  mates: best segment pair (2-9 seg4 vs seg5) sits 1.68 mm apart with
  surface normals 67+ deg opposed (means 129 deg apart: [0.68,0.43,0.6]
  vs [-0.66,0.43,-0.62] -- opposite wall faces, ~wall-thickness offset).
  Gate demands <2 mm AND normals agreeing within ~32 deg (0.85) or
  ~46 deg (0.7): 0 inliers at every segment pair of 2-9, and 0/18 mates
  at 3mm/15deg whole-file. At 10mm/30deg false pairs score max 19 mean
  2.5 vs true max 24 mean 1.3 -- no discrimination left either.
  Two internal inconsistencies found: (1) UnifiedPotteryValidation uses
  raw dot (agree-only) while robust_icp's identical check uses abs(dot)
  (robust_icp.cpp:396) -- the pipeline disagrees with itself on
  opposing normals; (2) with abs(), true 2-9 admits 6 inliers at strict
  (2.0/0.85) vs false 1-2 admitting 0 -- discrimination EXISTS under
  abs. Single variable next: SFS_NORMAL_ABS=1 (abs the fallback dot,
  matching robust_icp), strict gate, holder run. Pot_A no-regression
  check still owed before any fix claim. (31291925 Cauchy-restore on
  real curves also completed: verified 5.0/2.0 prints, still 0/18 --
  capture range exonerated on real curves, closing the dot-era thread.)

- 2026-09-25: the ONE passed merge dissected (wide-gate run): pair 2-9
  (TRUE mates) placed 110 deg / ~200+ mm off GT relatives -- right pair,
  wrong pose. GT gap 0.02 mm (touching) vs proposed 11.45 mm (floating):
  a near-miss the gates correctly reject. Refinement starts ~10 mm off
  and never closes (1.0 mm kernels vs upstream 5.0). The Cauchy
  restore (5.0/2.0) was only ever tested on DOTS -- retesting on real
  curves now as 31291925 (knobs already in binary, verified prints).
  If it closes: capture range was binding. Staged pair 2-9
  (correct-touching vs proposed-floating) as juglet_sfs29 for the eye.

- 2026-09-25: w_a=0 (verified print) still 0/18. Axis term FULLY
  exonerated (1.0 / 0.1 / 0 identical) -- remove it from the suspect
  list, not just demote it. Remaining: init quality (~10 mm off vs
  3.6 mm true gap), rim terms (already off by flags), Cauchy scales
  (5.0 tested, negative).
- 2026-09-25: iterative diagnosis moves to a held CPU allocation
  (holder 31291185, sapphire 8cpu/32G, 3h -- NOT the GPU script, which
  would waste a GPU on CPU work). Batch turnaround was the bottleneck:
  every one-line probe cost a queue wait. Ad-hoc holder (documented
  here); file as cpu_session.sh if the pattern repeats.

- 2026-09-25: upstream widths (5.0/2.0, verified print) still 0/18.
  Refined reading of the evidence: on real curves there is NO e5
  divergence (that was dot-era); inits sit ~10 mm off and never close
  to <2 mm. So: non-convergence, not divergence. E3b (axis weight) was
  also dot-era and therefore VOID for real curves -- retesting w_a=0.1
  now as 31290875 (no rebuild needed). If that fails, next: w_a=0,
  then init quality (why 10 mm off when GT gap is 3.6 mm?).

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
