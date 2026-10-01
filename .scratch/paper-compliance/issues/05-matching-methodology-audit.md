# 05: Assembly-matching methodology audit — paper vs matcher

**Answers:** S1

**Blocked by:** nothing — audit only; each gap below names its own fix ticket
where one exists

**Status:** resolved 2026-10-01 — audit complete; three mapping briefs +
lead verification of every deviation and liveness claim. Deviations fanned
out to tickets 06–10 (created same day); matches/acceptances/silent
inventory below are the standing record.

**Needs-eye:** none — text-vs-code comparison, no geometry claim.
Renders enter only if a traced path turns out to depend on geometry
nobody has looked at.

## Why this ticket exists

Ticket 16 audited preprocessing (§IV-B1), the axis, and assembly *inputs*
(descriptors, flags, files, precedence). The assembler's *matching
methodology* — what the paper says matching does vs what
`feature_matching` / `ranking_system` / the optimizers do — was never
compared. That is now the largest unaudited surface in the gap-closure
project. Paper source: `papers/text/sfspp-2502.13986v1.md` (matching +
validation passages, e.g. lines 249, 519–550 — cite by line, not memory).

## How to run it (ticket-16 rules apply)

- Every item verified against BOTH the paper text and the code; line
  numbers cited, not summaries. Anything touched later cites the lines.
- Anything the audit cannot decide from reading becomes a measurement
  proposal, not a guess. Do not let "already verified" override cheap
  local evidence checks.
- Nothing here re-argues settled measurements — cite ticket + line.

## Checklist (seeded from a 2026-10-01 spot-check, not exhaustive)

1. **Profile-curve validation.** Paper: rz-plane projection, 7mm bins,
   orthogonal regression, 7mm rejection threshold (lines ~249, ~550).
   Code: `ProfileChecking(profile, 6.5, 6.0)` at `ranking_system.cpp:1913`
   ("RELAXED", was 6.0/5.5, original 7.0/7.0), plus callers in
   `multi_hypothesis_optimizer.cpp:618,973` and
   `puzzlefusion_global_optimizer.cpp:434`. For each: (a) is
   `CalculateLeastSquare` the paper's orthogonal regression or ordinary
   least squares; (b) which callers run in the sbatch binary
   (`main_headless_correct` path) vs dead/legacy optimizers
   (`multi_hypothesis_optimizer_backup.cpp` exists — check before citing
   either); (c) where the relaxed numbers came from (measurement or drift).
2. **Overlap region detection.** Paper line ~249 (physical compatibility
   via overlapping areas). Code: `Enhanced3DOverlapDetector`
   (`surface_overlap_detector.h:26`, defaults 2.0mm/1000.0) read at
   `puzzlefusion_global_optimizer.cpp:321`. Same liveness question: does
   this execute in a run that matters, and do the thresholds match any
   paper value.
3. **Ranking/plausibility thresholds.** `CheckGraphPlausibility`, the
   rim-height bands in `CountPCInlier`, the 0.4/0.4 μ/ν weights (paper
   line ~519) — each compared to its paper value or recorded as ours.
4. **Match search itself.** What the paper specifies (LCS? beam search?
   pairwise pruning order) vs `FeatureComp` /
   `FeatureCompGraphBuilding` / `PairwisePruning` structure. Structural
   comparison only — no re-tuning inside this ticket.
5. **Ticket-18 interaction.** Face-crossing traces feed profile curves and
   descriptor series. Whatever this audit says about profile/descriptor
   sensitivity must cite ticket 18's pollution finding, not assume clean
   rims.

## Out of scope (recorded, not ducked)

- Re-tuning any threshold found different. New values get their own
  tickets with per-pair measurements, one variable each — bundling them
  here would make every result unattributable.
- The base-half removal (assembly ticket 02, closed): the paper HAS base
  machinery; we run rim-only by explicit decision. This audit records that
  as the standing deviation wherever rim/base appears, it does not
  re-litigate it.
- MLESAC-vs-RANSAC, LM-vs-trust-region, SG window: owned by ticket 16's
  notes and assembly ticket 04 respectively. Cross-cite, don't duplicate.

## Acceptance criteria

- [x] Each checklist item resolved to match / deviation-with-ticket /
      accepted-with-reason, with paper-line + code-line cites (below,
      2026-10-01; three mapping briefs + lead verification of every
      deviation and liveness claim)
- [x] Every deviation names its fix ticket (06–10, created same day) or
      records why it is accepted (beam k/b, dead-by-default optimizers)
- [x] Live-vs-dead verdict for each optimizer call path in the sbatch
      binary (table below)
- [x] Nothing in this ticket re-argues settled measurements

## FINDINGS 2026-10-01

Conventions: "verified" = lead read the lines; "spike-reported" = mapping
brief cite, lead did not re-open the function. Paper lines refer to
`papers/text/sfspp-2502.13986v1.md`.

### Matches (structure confirmed)

- LCS initial search, multi-cluster candidates (paper :203/:207 →
  `FeatureComp` all-pairs × `LCS()` :1607–1622; graph-loop variant).
- Two-round inverted-descriptor matching + rim-interval discard (paper
  :209 → `ChangeOrder` reversal `data_structure.cpp:446–455`, applied
  round-trip; rim zeroing :285–290). Verified reversal machinery.
- P2P first ICP iter → P2L (paper :228 → :1223–1238 switch). Verified.
- LM 100 inner iterations (paper :240 → ceres 100 :1348–1349). Verified.
  (This SETTLES the LM-vs-trust-region open item: the paper names LM.)
- Overlap region conditions d=5mm/θ=30° (paper :545–546 →
  `MakeCorWOTree` len<5 / dir≥0.86=cos30 :529–535). Verified.
- Overlap area 50 (paper :548 → `RegistrationPruning` arg 50.0 :2060–2064,
  "Restored to normal"). Verified this stage.
- Rim median (paper :533 → `MakeRadiusHeight` middle element,
  spike-reported :468–471).
- Beam semantics: expand b, verify, top-k by score, dedup (paper :259–306
  → :841–849, :1374). Verified structure; k/b VALUES are run config
  (paper prescribes semantics, uses k=5,b=3 / k=20,b=10 experimentally;
  ours TOP_k=15/BRANCH_b=8 :47–48 — paper-silent, accepted as config).

### Deviations (each owns a fix ticket)

1. **ICP outer iterations 50 vs 150** (paper :240 → `max_iteration=50`
   :1210). Verified. → Ticket 07.
2. **Weights λ/μ/ν.** Paper: 0.4 Cauchy-scaled (lines :491/:519). Code:
   NO 0.4 anywhere in the live path (verified by grep); split linear
   weights (w_d=1.0, w_n=3.0, w_line=1.0, w_a=0.1, w_r=w_h=1.0 :1208–1209)
   × Cauchy scales (4.0/1.8/1.8/1.8 :1309–1312). No mapping comment
   anywhere. → Ticket 07 (methods comparison, not a value swap).
3. **Normal gate 15° at inlier-counting vs 30° at correspondence.**
   Paper :215 prunes correspondences >30°. Code: `MakeCor` has NO angle
   pruning (mutual-nearest only); angle appears only in inlier counting
   at 0.262rad≈15° (:1079, `reconstruction.h:26`). Verified both halves.
   → Ticket 08.
4. **Grouping rotation 10° vs 25°.** Paper :294 (25°/20mm). Code:
   `isSimilar(0.175, 20)` :1745 = 10.0°/20mm. Translation matches; rotation
   does not. Verified. → Ticket 08.
5. **Profile check: wrong fit, wrong statistic, relaxed numbers, full-edge
   input.** Paper: 7mm bins, ORTHOGONAL regression, reject on STD-DEV
   >7mm of INNER-surface edge points (:249/:550, Table :566–567). Code:
   `CalculateLeastSquare` is ORDINARY least squares of r on z (verified
   :492–506 — design matrix [z,1], pseudoInverse; "orthogonal" is only the
   solver name); rejection is max-absolute-deviation, any-point-any-bin
   (:951–954, verified — no std-dev computed anywhere on the profile
   path); callers pass 6.5/6.0 (all four sites); input is full `edge_line_`
   (rim+fracture), and the puzzlefusion caller feeds PIECE CENTERS, not
   edge points (:432–434, spike-reported). → Ticket 06 (the big one).
6. **Overlap area: one paper value, five code values.** Paper: 50mm².
   Code: 50.0 RegistrationPruning (:2060, matches) BUT 120.0
   PairwisePruning (:1828, "ORIGINAL THRESHOLD" comment), 10
   CheckGraphPlausibility (:1869, "REDUCED 50→10"), 100 MergeOverlapTest
   (:2134–2138), 20 SingleOverlapTest (:1973). Verified 120/50 pair.
   → Ticket 09 (unify-or-justify).
7. **CountPCInlier bands.** Caller bins 7.0 match ω=7; threshold 1.5,
   `dist>10` hard reject, rim ±10mm both-sides rule, ×(ratio²·0.2+1)
   multiplier — all paper-silent (ours). Recorded; no ticket unless
   implicated (belongs to 06/07 as supporting evidence if needed).

### Accepted (recorded, not re-litigated)

- Beam k/b values (config, above). Dead-by-default optimizers
  (multi-hypothesis, puzzlefusion, two-phase run only under ENABLE_*
  flags the sbatch never sets — verified gating :465–522/:613/:629/:571
  and sbatch exports nothing but CC/CXX). Their ProfileChecking sites
  don't execute in production. Base-half removal (assembly ticket 02).

### Paper-silent inventory (ours — cite as such, never as paper)

LCS multipliers 3/2.5/2.5/4; `Clustering(Out,20)` overriding header
default 5; `windowsize`=MINIMUM_NUMBER=1; mode-1 Q_size branch (all three
callers pass mode=0 — live-dead); hub-guidance disabled comment block
(:442–456); dead `EnhancedStateManager manager` (:260); hub-guided
scoring; IntersectionDetector ratios; `RejectOutlier(20,0.65)`;
`isConverge(0.1,2.0)`; disabled `CheckOpposingNormals` (:1915–1919).

### Dead code (removal ticket 10)

- `ProfileCheckingWithInlier`: def + decl only, zero callers (verified).
- `multi_hypothesis_optimizer_backup.cpp`: same entry symbol, NOT in
  CMakeLists (verified absent), never included. Duplicate-symbol
  collision if ever compiled — removal is load-bearing hygiene.
- `multi_hypothesis_optimizer_test_patch.cpp`: patch draft, unbuilt.
- `auto_agglomerative_assembler.{cpp,h}`: absent from CMakeLists, never
  included. `physics_based_optimizer.{cpp,h}` + `physics_integration.cpp`:
  unbuilt shadows. Root `surface_overlap_detection.{cpp,h}`: already
  recorded superseded in AGENTS.md.
- `main.cpp` / `main_headless.cpp`: alternate mains, unbuilt (only
  `main_headless_correct.cpp` in CMakeLists :156–158). Left alone (they
  are binaries-that-aren't, not dead weight in the live one) — recorded.

### Liveness table (default sbatch run, no env flags)

| Path | Verdict |
|---|---|
| `StateManager::BuildStep` (ranking_system :900 via enhanced :45/:47 ← main :680) | LIVE — the production matcher |
| `FeatureComp` :342 → `PairwisePruning` :384 → beam loop | LIVE |
| `CheckGraphPlausibility` (:1840, profile :1913, overlap-10 :1869) | LIVE |
| multi-hypothesis / puzzlefusion / two-phase optimizers | Dead by default (env-gated, flags never set) |
| `Enhanced3DOverlapDetector` chain (:321→:139→ctor) | Dead by default (puzzlefusion-only); exercised thresholds would be 2.0mm/500mm³ (hardcoded :137), NOT header 2.0/1000.0 |
| Breakline-side overlap (`OverlapCheck_3d`, merge/single tests) | LIVE |

### Ticket-18 interaction (cited, not re-measured)

Face-crossing traces feed `ToCylindricalInterpolation` profile input and
descriptor series. Any future profile/descriptor measurement (tickets 06,
07) must state which bundle arm it ran on (pre/post-18-fix); a
measurement on a polluted bundle cannot attribute profile failures to the
fit statistic.
