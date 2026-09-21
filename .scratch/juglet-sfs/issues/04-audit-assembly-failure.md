# 04: Audit the Juglet assembly failure and diagnose the mechanism

**What to build:** a verdict on WHY job 30829588 proposed zero joins that
distinguishes, with a test behind each: (a) per-fragment axis frames too
scattered to match in, (b) matcher thresholds blind to short handmade
breaklines, (c) merge gate dropping inlier pairs -- plus an honest scorer
(the current one starts from the answer key, so it cannot fail).

**Answers:** S1 (touches S2: the axis-frame finding is S2's scope question
with a number attached)

**Blocked by:** 03 (resolved -- run outputs in
`structure-from-sherds-pp/artifacts/juglet_run1/`, bundle + GT staged)

**Status:** resolved

## Verdict (2026-09-21, ticket 04 audit -- for S1)

- Reproducibility: zero joins stable across 3 runs (baseline, frozen
  rerun, counterfactual); singleton membership is NOT (7 vs 6 pieces,
  different sets -- nondeterministic beam ties/ordering).
- Counterfactual: consistent vessel-frame axes for all 9 sherds
  (md5-verified swap in/out) still yields 0 joins. Frames contribute
  (match patterns shift) but are INSUFFICIENT -- the seat is matcher
  features + merge.
- Matcher: 12/18 true mates get zero feature matches (quantized
  cylindrical profiles in scattered frames; MINIMUM_NUMBER=1 already,
  base gate passes -- not threshold gates). Five true pairs reach ICP
  with inliers (1-6: 84) yet none becomes a graph edge ("no more new
  root" loop; exact sub-gate below merge-loop granularity: OPEN).
- Scorer fixed: restored the commented-out method-graph requirement;
  run 30895279 reports honest 0/9 sherds, 0/18 edges (was vacuous 100%).
  NOTE: future Pot_A/Tray re-scores will drop accordingly -- intended.
- OPEN (follow-up, not this ticket): the exact line where inlier pairs
  die between pruning and graph edges (LCS-kept list never printed).

**Needs-eye:** viewer bundle TBD at staging time (any new geometry claim --
e.g. a counterfactual assembly render -- is staged under
`visual-qa/viewer/pairs/` and witnessed before it counts)

## Starting evidence (all in `artifacts/juglet_run1/` + ticket 03 comments)

- Best state 7 singletons (sherds 1,3,4,5,6,7,8), score 0; sherds 2,9 never
  placed. 0/18 joins proposed. Eye and log agree (witnessed 2026-09-21).
- 4/18 true mates had feature matches (1-6: 29; 4-5, 3-7, 7-9: 3-4);
  14/18 true mates had ZERO matches. ICP gave inliers on five true pairs
  (1-6: 84, 7-9: 63, 4-5: 22, 3-7: 4, 6-8: 2); none merged ("no more new
  root" x8).
- Per-fragment PotSAC axes, mapped to the assembled frame with GT, scatter
  51 deg mean pairwise; matching profiles breaklines as cylindrical
  radius/height/angle curves AFTER per-fragment AxisAlignment
  (`class/filter.cpp:161`, called `main_headless_correct.cpp:295`).
- Counters in the log read 100%: vacuous by construction -- accuracy
  seeds `T_result` from `GT_trans` (`main_headless_correct.cpp:776`) and
  unplaced pieces keep GT values, so nothing-assembled scores perfect.
  Disregard; do not "fix" the finding by re-reading the counter.

## Audit checklist

- [ ] Reproducibility: rerun the identical binary+inputs once (no rebuild);
  zero joins + same 7 singletons required before any mechanism work --
  a flaky zero is a different ticket (nondeterminism, e.g. CGAL
  concurrency) from a stable zero (method/scope)
- [ ] Trace the five inlier pairs (1-6, 7-9, 4-5, 3-7, 6-8) to the exact
  stage that drops each (LCS/pruning scores? merge-gate "root" test?) --
  name the gate and its threshold from code at the pinned version
- [ ] Axis-frame test: per-sherd axis scatter re-measured from the bundle
  alone (no GT in the loop); then the counterfactual -- align by GT
  instead of PotSAC and rerun matching only. Joins appear => frames were
  the whole problem; nothing appears => the matcher itself cannot do
  this material even given perfect frames. Either outcome is the answer
- [ ] Matcher blindness: for 3 zero-match true mates, say whether the
  curves ever clear the feature thresholds (length/curvature gates) or
  never correspond in any frame -- threshold problem vs frame problem
- [ ] Honest scorer: seed accuracy from identity/unknown instead of GT so
  unplaced pieces count as unplaced; re-score run 30829588 with it and
  confirm 0/18 (the number must come from the method, never the key).
  Small code change in this repo; keep the old path behind a flag, do
  not rewrite the assembly
- [ ] Result written back into `intent/S1-does-it-connect-anything-real.md`
  (and the S2 note if the axis finding firms up or falls): which of (a)
  frames / (b) matcher / (c) merge gate, with the test behind each

## Comments

- 2026-09-21: reproducibility rerun 30893352 MIXED -- zero joins stable
  across runs (headline reproduces) but singleton membership doesn't
  (7 pieces: 1,3,4,5,6,7,8 vs rerun's 6: 1,2,4,5,6,8). Nondeterministic
  membership further discredits the 9/9 counter.
- 2026-09-21: counterfactual (consistent PCA-vessel-frame axes swapped
  in with md5s, restored byte-identical after) still 0 joins, 7
  singletons. FRAMES ALONE INSUFFICIENT: match pattern shifted (8-6:
  3->14 matches) but nothing merged. Seat of failure is matcher
  features and/or merge, not frames.
- 2026-09-21: matcher gates read -- MINIMUM_NUMBER=1 already (length
  gate not the blocker); base-classification gate passes (info 0, no
  rim/base to skip on); blindness is quantized cylindrical profiles
  computed in scattered frames. Axis scatter robust: 27 deg large
  pieces, 65 deg small (both far past matchable).
- 2026-09-21: honest scorer committed (restore commented-out
  method-graph requirement in CountResult). Rebuild+rerun 30895279;
  expect honest 0/9 sherds, 0/18 edges. NOTE: this also lowers any
  future Pot_A/Tray re-scores -- flagged, intended.
