# S1 — After the unit fixes, does it find any real joins?

**Status:** open — the Juglet arm is now **settled at 0**, with the cause
proven rather than assumed (2026-09-26). **The Tray-000 arm is still
unmeasured and is the one that can answer the capability half.** ·
**Blocked by:** none

**2026-09-26, Juglet arm settled (0 joins, cause proven, not a
measurement error).** Two tests, one static and one dynamic, agree:

- **Static:** placing the sherds at their true ground-truth positions and
  running the pipeline's own join test, **Pot_A passes 15/15 and the
  Juglet passes 0/18.** The gate rejects the truth itself.
- **Dynamic:** handing the real binary the *exact* ground-truth placement
  of a genuinely-touching pair (6-7, seam 0.92 mm) and letting
  registration, refinement, gates and scoring run unmodified, the join is
  **rejected with zero matching points** — twice. The identical harness
  on Pot_A (pair 2-5) **accepts and keeps** the join (score 732→735).

So this is **case 1, the method failing on this material** — not a broken
ruler, not a wrong answer key. Both were already excluded (ticket 01
residuals ~1e-14 mm; the scorer fix in ticket 04), and the Pot_A control
now rules out a broken instrument as well.

**Why, in the method's own terms.** The join test needs a point on one
sherd's breakline within 2 mm of a point on its neighbour's, with
surface normals agreeing. Measured in the preprocessing repo (its ticket
01, 2026-09-26), the Juglet fails on two counts: the extractor traces
**one** wall face per sherd and which one is arbitrary (inner for 1, 5, 9;
outer for 2, 3, 4, 6, 7, 8), so **10 of 18 true mates are inner-vs-outer
and invisible to any 2 mm comparison**; and only **1 of the 8 same-face
mates** has traces within 2 mm at ground truth, the rest sitting 5–30 mm
from the 0.02 mm truth — the segments do not span the seam. Pot_A's thin
wheel-thrown breaks put both traces on one face, 0.09–1.30 mm apart,
with normals agreeing.

*Correction to an earlier statement here:* the "normals 67–129° opposed"
figure came from pair 2-9, which is one of the opposite-face pairs. On
same-face pairs the normals agree at 0.92–1.00 and pass the gate easily.
The opposed normals were a **symptom** of the face confusion, not a
separate defect.

**Consequence for this question:** the honest Juglet score is **0/10**,
not 0/18 — eight of the answer key's 18 edges are not physical contacts
at all (3.3–18.9 mm apart at ground truth). The denominator itself was
partly broken.

**What it does NOT settle.** The gate blocks this object from ever
proposing a join, so the Juglet arm can prove the method works on
suitable material (Pot_A control) but **cannot** prove it does not work
in general — the gate finding is a statement about breakline extraction
on thick, handmade, eroded sherds, not about SfS++ on all handmade ware.
**The capability half of S1 still needs the symmetric arm below.**

**Also unresolved:** the matcher proposes a *different* pair on every
run over identical input (6-7, 5-7, 3-7, 7-9) — ticket 08. Until that
is attributed, any single-run join count on this corpus is a sample from
a distribution, not a measurement.

## Why it matters

The headline finding on this system is stark: **zero pairwise connections** between any of
the forty Tray-000 sherds. Not a poor assembly — no candidate joins at all.

That result was recorded **before** two unit-conversion bugs were found and fixed: a
1,000,000× coordinate mismatch (micrometres against millimetres) and a boundary-detection
radius in the wrong unit. Either alone is enough to produce exactly that symptom, because
a radius in the wrong unit means the search for neighbouring boundary points looks in the
wrong place entirely.

**So the zero result currently means nothing about SfS++'s capability.** It has not been
re-run since the ruler was fixed. Leaving it in the record as a capability finding would
be the same error as the one already logged in `docs/lessons.md`: fixing the ruler,
re-reading the same run, and calling it proof.

## What is in hand

**A valid reference answer for the Juglet exists** (`.scratch/juglet-sfs/` ticket 01,
resolved 2026-09-16). Nine sherd meshes in millimetres at vessel scale — 41 × 37 × 65 mm,
wall about 1.8 mm — with ground-truth transforms in SfS++'s own 4-line format and an
18-mate contact graph cross-checked against TORA's pair list and found identical. One global
scale fits all nine pieces with **0.0000% spread** and residuals around 1e-14 mm, and the
render shows the pieces closing into the vessel.

**That settles nothing about capability, and is not meant to.** It removes the excuse: the
input and the answer key are no longer in question, so whatever the assembly does next is
about the method. Tickets 02 (preprocess) and 03 (assemble) are what produce a result.

**2026-09-20: ticket 02 is done.** The preprocessing bundle exists on Spartan at
`sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp/`: 18 surfaces, 18 breaklines,
9 axes (PotSAC found candidates on all 9 sherds), 9 meshes, ground truth. Along the
way three Pot_A-era assumptions broke and were fixed with the Nov-2025 programs
otherwise untouched (container mount, 14-char filename trim, millimetre search
radii, per-sherd splitting strictness for sherds 3/9, script-file axis call).
Ticket 03 (assembly + witnessed render) is unblocked; nothing here scores the
method yet.

**The arm that is running is the Juglet, not Tray-000.** That matters for reading the
outcome — see the gate below.

## The corpus this method was built for

Conservator, 2026-09-18: **about 99% of the Rabati material is axially symmetric** — round in
section, thrown or otherwise symmetric about a vertical axis.

That is the most consequential fact about SfS++ in this folder, and it cuts against how the
method has been treated here. SfS++ assumes axial symmetry and exploits it; that assumption
has been written up as a *limitation* confining it to the role of comparison partner. On a
corpus that is 99% axially symmetric it is not a limitation — it is a match. On the stated
character of the target material, SfS++ is the method whose assumptions fit best, better than
either diffusion system's.

**The Juglet is the exception, not the sample.** Handmade and handled, it is the 1%. A Juglet
outcome — success or failure — therefore says less about Rabati than a result on an ordinary
round-sectioned body sherd would. That is not an argument against running it: it is the only
object here with a trustworthy answer key, which is why it goes first. It is an argument
against letting the Juglet stand in for the corpus when the finding is written up.

## Done when

- [x] **A reference answer good enough to score against, for at least one real object** —
      the Juglet, 2026-09-16, residuals ~0.0000 mm, render witnessed. Without this the
      re-run would have had nothing to be right or wrong against
- [x] **The Juglet run end to end** (`.scratch/juglet-sfs/` 02 then 03): 0/18
      connections proposed (best state singletons, score 0; sherds 2,9
      unplaced), strongest miss (sherds 1-6, 29 matches/84 inliers)
      rendered correct-vs-attempt and witnessed 2026-09-21 (eye and log
      agree: nothing joined). Null result, fully documented -- no proposed
      join exists to render, so the miss stands in for it.
- [x] **The Juglet audit end to end** (ticket 04, 2026-09-21): zero stable
      across 3 runs (membership jitters 7 vs 6 singletons -- nondeterminism
      noted); consistent-frame counterfactual still 0 joins (frames
      insufficient; seat is matcher/merger); trace closed to named gates:
      RegistrationPruning erases edges over 0.436 rad post-registration
      axis angle, RemoveEdgeUsingPCInlier (overlap/CountPCInlier 7.0,1.5),
      SortRoot excludes zero-inlier sherds from roots (P2, P9 out);
      scorer restored to honest 0/9, 0/18. Residual: per-pair sub-gate
      attribution (erasures print nothing) is follow-up work.
- [x] **Ticket 05 verdict** (2026-09-24): E1/E2 sweeps VOID (dead
      PrepareNextStep path -- recorded, not hidden); merge death pinned
      to Ceres refinement DIVERGENCE (e5 placements, converges-nowhere);
      w_a=0.1 refutes axis-weight-as-sole-cause; thresholds exonerated
      (re-match finds nothing because placements are kilometers off, not
      because gates are strict). Fix direction = refinement robustness:
      ticket 06 opened. S2 note stands.
- 2026-09-25 AMENDMENT (breaklines degenerate -- ticket 02 reopened):
  bundle breaklines are sub-mm dots at x1000 offsets (vs Pot_A's real
  rim curves); the 60 matches / 84 "inliers" are dot-noise overlaps.
  The 03/04/05 "0/18" verdicts stand CONDITIONALLY as dot-input results
  (assembly genuinely found nothing on THESE inputs; scorer fix,
  patch-ordering, axis-scatter on independent axes all stand). Must be
  re-earned on real rim curves; mechanism ranking may shift.
- 2026-09-24, ticket 06 characterization (SUPERSEDED 09-25, kept for
  the trail): merge INITIAL placements looked e5 -- later shown to be
  stale readings; current understanding below. Ticket 06 proceeds on
  refinement capture range.
- 2026-09-25 AMENDMENT (breaklines degenerate -- ticket 02 reopened,
  then RE-RESOLVED same day): bundle breaklines were sub-mm dots at
  x1000 offsets (meter-assumed sphere radii 100-1000x too small, padded
  to 200 by the densifier; writer x1000 on mm clouds). Fixed via
  versioned patches; extent gate (rim scale + vessel scale) green on
  all 9 (10.3-26.8 mm). ALL downstream assembly findings (03-06) were
  dot-input results: the "0/18" must be re-earned (ticket 07 opened
  for the rerun); standing independently: scorer fix, patch discipline,
  axis scatter on independent axes, lessons.md entry ("a count is not
  a shape").
- 2026-09-25, ticket 07 verdict (REAL rim curves, run 31282932): 0/18
  again, but a DIFFERENT zero -- matcher floods (36/36 pairs match,
  all 18 mates with inliers) instead of starving; merges start sane
  (2-15 mm inits) and drift (200 refinement iterations vs 1.0 mm
  robust kernels vs ~10 mm init error: saturated losses, weak
  gradients, no convergence). Fix direction: restore upstream kernel
  scales and/or bound the walk (ticket 06). Existing pair-16 staging
  + witness stand (meshes/GT/scan poses byte-identical, claim null).
- [x] **Juglet arm settled with cause proven** (2026-09-26, ticket 06):
      **0/10** joinable edges (0/18 nominal — 8 answer-key edges are not
      physical contacts). Two tests agree: static GT-placement gate probe
      (Pot_A 15/15 vs Juglet 0/18) and dynamic oracle-init through the
      real binary (Juglet 6-7 rejected from truth with 0 inliers; Pot_A
      2-5 accepted, kept, score 732→735). **The method fails on this
      material, and the harness that showed it is validated by its own
      control.** Cause: breaklines trace opposite wall faces ~1.7 mm
      apart with opposed normals; the gate compares them as one surface.
- [ ] **A new breakline extraction that pairs across the wall and covers
      the seam** — the actual fix, in preprocessing. Acceptance test:
      `python artifacts/juglet_run1/gate_probe_b0.py <bundle>` must show
      true mates passing the gate at ground truth. Until this passes, no
      further assembly tuning is worth attempting.
- [ ] **Tray-000 re-run end to end** **after** both fixes, from the fixed preprocessing output
- [ ] Connection count reported, and if it is still zero, a check that the search radius is
      now in the same units as the point cloud — verified by printing both, not assumed
- [ ] At least one proposed join **rendered**: the two sherds, at a view that shows whether
      the break faces meet. A connection count is not evidence that a connection is real
- [ ] The 0% NURBS-dataset result revisited in the same light: `Surface_F.pcd` files were
      missing and surfaces were 2–6× sparser than the working sample, so that arm was also
      scoring a broken input

## Gate

**On Tray-000 or other axially symmetric material:** if it still finds nothing with correct
units and complete inputs, **that** is a capability finding and can be reported as one.
Until then it is an unmeasured system.

**On the Juglet, the same outcome does not carry that reading.** The Juglet is handmade and
handled — outside the axial-symmetry assumption SfS++ is built on (`.scratch/juglet-sfs/spec.md`).
A failure there is **scope, not capability**, and must be reported as such. The Juglet arm
can therefore prove the method works on hard real material, but it cannot prove the method
does not work.

**Only an axially symmetric arm can do that, and no object is currently assigned to it.**
Tray-000 is parked (conservator, 2026-09-18 — see
[U13](../../intent/U13-which-sherds-are-one-vessel.md)), which leaves the capability arm
empty. That is a gap to fill, not a reason to stop: since ~99% of Rabati is axially symmetric,
one ordinary vessel with a conservator-made answer would serve, and is far cheaper than
establishing trust in a forty-piece tray.

## Source

`ASSEMBLY_FAILURE_ROOT_CAUSE.md`, `ROOT_CAUSE_DIAGNOSIS.md` (2025-11-04),
`BOUNDARY_RADIUS_FIX.md`, `PREPROCESSING_SUCCESS_SUMMARY.md` (2025-11-05),
`NURBS_FIX_RESULTS.md`, `STEP_BY_STEP_COMPARISON.md`. Conservator, 2026-09-18: Rabati is
~99% axially symmetric; Tray-000 set aside for now.

## 2026-09-29 — smoother A/B: scores move, joins don't

Paper-compliance ticket 01 (assembly side): the descriptor smoother differs
from the paper (Lanczos+Gaussian vs Savitzky-Golay+Gaussian(7,2.0)),
2.6–20% descriptor differences measured on a real rim. Full matching A/B on
the authors' Pot_A sample, deterministic matcher verified by control rerun:
same 9 pairs survive both arms, values move ±1–2, assembly score 0→36 with
accuracy 0/6 both arms. No pair gained or lost. Closed as cosmetic for
joins. Found along the way and fixed: d2445d8's host-absolute dataset paths
broke every fresh build's file loads (reverted to container-visible).
Found and recorded: teardown segfault after full output (robustness item,
not a measurement blocker).

## 2026-10-01 — compliance 05: matching methodology audited, fanned to 06–10

Three mapping briefs + lead verification of every deviation and liveness
claim. Matches: LCS search, two-round inverted matching, P2P→P2L, LM-100
inner (settles LM-vs-trust-region), overlap d/θ conditions, area-50 at one
stage, beam semantics. Deviations with fix tickets: ICP outer 50 vs 150 +
weight-mapping (07); normal gate 15°-at-counting vs 30°-at-correspondence
and grouping 10° vs 25° (08); profile check — OLS not orthogonal,
max-rule not std-dev, 6.5/6.0 not 7/7, full-edge not inner-only (06, the
big one); five overlap-area values where the paper has one (09). Dead code
removal (10), incl. the backup file that would collide if compiled.
Paper-silent inventory recorded (ours, never paper's). Liveness: only the
incremental StateManager path runs by default; the three alternative
optimizers are env-gated dead, as are their profile/overlap sites. LM-100
verdict also closes a ticket-16 leftover. Ticket-18 interaction cited into
06/07 (no polluted-bundle measurements without stating the arm).

## 2026-10-01 — E2E baseline: probe 15/15 in, assembly 0/8 out (handoff fired)

Fresh current-code extraction (`pota_e2e_fresh`, probe `e2e_fresh` arm
15/15) through `build_sif/Hierarchy-Clear` with the POT_A block (job
31835065, COMPLETED 0:0): matching finds all 15 true pairs (values
7–148), then a single 8-piece state with score 0.000 "wins" and every
piece lands wrong — 0/8 sherds, 0/15 edges. Ticket 07's handoff clause
triggered exactly as written: the probe passes and the assembler finds
nothing, recorded as a downstream finding. Filed as juglet-sfs ticket 09
(log forensics → owning ticket 06/07/08/09, re-run of this baseline moves
0/8). `POT_A_ORIG` restore left pending in `data_path.h`.

## 2026-10-01 — fork audits accepted (both docs land) + 05 corrected

- Both `FORK_VS_UPSTREAM.md` docs reviewed claim-by-claim and committed.
  Preprocessing doc stands as written (upstream-owned 0.12/4.5/1.5 verified;
  ticket 16 corrected accordingly).
- Assembly doc: 4 surprises verified, 1 corrected before landing —
  ConnectivityOptimizer 0.7/0.3 scoring IS live (my first reading said
  dormant; the scorer runs inside live `MakeHierarchyPriorityList`, only
  the separate GGCE *phase* is default-off). It joins new ticket 12 as
  row 7.
- Ticket 05 amended: LCS multipliers / Clustering(Out,20) / isConverge /
  Q-structure re-attributed to upstream (were "ours"); MINIMUM_NUMBER
  6→1 + axis-gate/lowest-score/RejectOutlier/opposing-ratio/CountInlier
  relaxations recorded as cluster-lineage unknowns → new ticket 12
  (adopt-or-revert, measured; rows 4/5/7 are lineage-ADDED, so
  disable-vs-keep, not value-vs-value).
- F5 fixed (live binary logs `[DROPPED]` shards now — the old logging
  reached only the unbuilt main; standing record corrected); F7 stale
  comment fixed. Both ride the next rebuild (prints/comment + cout only).

## 2026-10-02 — control: iterations load-bearing (0/8 at 50 vs 2/8 at 150)

Symmetric single-run isolation (only the two Registration lines differ).
150 restored (it earned it). Ticket 07 outer-count item conclusive-ish.
`POT_A_ORIG` restore postponed again — ticket 08's A/Bs need the POT_A
block next; restore lands after the last assembly run, and the ticket
says so.

## 2026-10-02 — k=1 bounded; 1-2 never attempted (next slice named)

Seven directed edges transform-similar; MERGETABLE ledger eliminates all
but {1→3, 3→1, 1→4} (pairs 1-2/2-4 never attempted). Pair 1-2 — eye-witnessed
genuine, probe 15/15 member, pieces transform-exact — survived matching
(value 10) yet never reached merging. Dies between matching and merging;
ticket 09 owns locating where (pruning vs beam).

`CountResult` counts DIRECTED edges (total=30): "0/15 (3.333%)" is exactly
one passing directed edge (k=1, display truncates, percent exact). Context:
Registration-150 run (job 31898200) placed pieces 2, 3 exactly and piece 1
within 0.3°/2.0mm — first 2/8 in the series (ticket 07, single-run
evidence, control rerun owed). Sherd 2/8 (poses) is an independent
criterion from edges. Zero undirected pairs pass, so ticket 09's
pair-level "nothing fully recovered" stands. Staged GT graph
byte-identical to the probe's. Open: which directed edge passed.

## 2026-10-01 — fix 1 works as intended; 0/8 stands on poses+scores

Std-dev run (job 31843205): profile 87/2 (was 4/19), only kills a false
[1,5,8] config twice — every true merge passes. Beam chains 5–6-piece
graphs over 21 states (best 232) that are all wrong: 0/8 + 0/15.
Ticket 06 resolved (statistic fixed+measured, fit parked, guard done).
Profile layer cleared; what remains is poses (07/T6: 62–179° off) and
merge scoring/selection (08, assembly-09). Ticket 09 stays open (its
re-run criterion needs the fix that moves 0/8, not this one).

## 2026-10-01 — battery executed: statistic convicted, poses convicted, fit cosmetic

- Replica 30/30 (T0 vindicated as recorder).
- T1: stddev rule flips ALL 20 recorded failures, 0 new false passes —
  the max-deviation STATISTIC kills true merges (fix first, ticket 06).
- T2: TLS refit changes nothing (skip).
- T4: confounded by subset size (no signal; ticket 18 stands on the eye).
- T5: true 35–297mm², false inside it, paper's 50 kills top-scored true
  merges — no threshold separates (ticket assembly-09: shape, not value).
- T6: relative poses 62–179° off on true pairs — registration convicted
  alongside (ticket 07 urgent; T6 is the re-measurement protocol).
- Robustness: garbage T (t_z=-1.48e11) vacuous-PASSES via bin-count
  overflow; 5-8 abort same family. Guard required (ticket 06 input work).
  No-env rerun downgraded to optional (T0 allocates nothing; 30/30 replica).

## 2026-10-01 — cause named: plausibility kills true merges at BOTH gates

- Defaults: overlap-10 kills true merges (35–95mm²); profile never reached.
- Paper values: overlap-50 passes them on; profile 7/7-max-rule kills 8
  all-true configs out of 19 ([1,5]×4, [3,6], [1,3,5]×2, [1,4]) — plus
  correct kills and one false pass ([7,8]).
- Prime suspects by body count: max-rule statistic (outliers — ticket 18
  put face-crossing traces in the input), OLS fit, full-edge input;
  overlap threshold placement (true merges span 35–95, above even paper's
  50 at the top end). Test battery filed as juglet-sfs ticket 10 (T0 dump
  → T1–T6 offline, one variable each, kill criteria stated). ICP poses
  still open (T6); ticket 09 stays open until it lands.

## 2026-10-01 — PAPER-CONFIG arm: values move the search, not the outcome

Same bundle through paper values (profile 7/7, overlap 50 everywhere,
grouping 25°, 30° angle, ICP 150, k=5/b=3; job 31835275): 6 states branch
(vs 1), best score 567.000 (vs 0.000), real 2-piece merges — but still 0/8
+ 0/15, no state joins all 8, no pose right. Values were a lever, not the
gap; structure (weights mapping, OLS/max-rule, correspondence gate, ICP
poses) owns the remainder. Bisect route recorded in ticket 09: paper
values are now the control arm.

## 2026-09-30 — compliance 02/03: dead expectations removed, precedence mapped

- Ticket 03 (axis/rim trace, closed as the map itself): at runtime the file
  axis wins everywhere — nothing computed overrides it; the one in-binary
  refinement seeds from canonical zero without corrupting file values.
  Rim-flag reads all gate in the "rim constrains" direction; strongest is
  BuildTree (rim segments never enter the tree). No contradictions; dead
  rim-only filter branch and debug double-insert recorded as accepted.
- Ticket 02 (base flags, verdict b): readers would act but no writer emits
  — un-flipping the define is a provable no-op, so the work is completion
  (clear `is_sane_base_` too — the old define left sane-base skips armed)
  plus removal of the commented-out gate and uncalled base-only branches.
  Enabling needs a true flag source from preprocessing, filed there.
- Preproc-01 removal (recorded here as the consumer side): both
  unconditional `Surface_F` loads now 2-arg (zero such files exist in
  either dataset); `IcpFine` — zero callers — gains a loud empty-frac
  refusal so it can never silently solve without the fracture term again.
- All three rebuild clean in-container (`build_sif`, exit 0, new strings
  in `Hierarchy-Clear`). Per-pair assembly reruns waived with reason: zero
  executed instructions change on current data (dead loads of nonexistent
  files, clears of always-false flags, edits inside uncalled functions).
  If any of these ever executes differently, the waiver is void — but on
  today's files there is nothing to execute.
