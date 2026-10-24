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

## Where it stands

SfS++ has not found a real join on the Juglet. That "0" holds for the dotted break-line
input it was given, so it is not yet a verdict on the method. Our copy's matching
settings are being tested one change at a time against the authors' own (ticket 12);
one change already kept leaves about 3.9 times fewer false joins; a second row
(lowest-score wipe back to 1.5) tied the control byte-identically, so the 4.0
relaxation earned nothing and upstream 1.5 is adopted; a third row
(correspondence filter back to 20/0.7) tied at 2/8+1/15 on the same pair,
so the (2.0,0.85) tightening earned nothing and upstream (20,0.7) is
adopted; a fourth row (opposing-normals veto enabled) lost the only
correct pair 1-4 and collapsed 2/8 to 0/8, so the veto stays disabled
with numbers; a fifth row (CountInlier adaptive gate) came back
byte-identical on every measure — dead code in overloads nothing calls —
so upstream's uniform rule is restored; the axis-gate row kept upstream
0.610 (first fully-correct pair 1-4); and the final row removed the
0.7/0.3 priority-list scorer, which moved scores (239 vs 260) without
moving a single pair — resolving the earlier read that the scorer
preferred the wrong assembly. All seven rows are now measured. Cleanup
then caught a leftover with teeth: the E-6 Huber experiment was still
live in the tree, starving pair 1-4 (post-row-7 trees read 0/8);
restoring Cauchy recovered 2/8+1/15 @ 262 exactly — the E-6 control
reproduced, so the zeros agree again. The solver walk is now bounded (trust-region cap:
diverged 140 to 0 with sane placements identical, same pair passing),
but convergence is untouched — poses elsewhere still far off. The
second solver candidate, anchoring one sherd's pose per pairwise
solve (ticket 02, Eq. 5), was A/B'd same-node and FAILED its kill
criterion: accuracy 2/8 + 1/15 went backwards to 0/8 + 0/15 (same
pair lost, no new pair, 1-2 never merge-attempted either way) —
hunk reverted, verdict in ticket 02. Convergence now routes to the
unowned Cauchy-scale work (E-0 gates all sweeps). **The E-0
measurement has now landed** (cauchy-scales ticket 01, job 32696452,
same node bm065, 38,584 dump files / 11.9 M rows): at iteration 0 the
point-to-point term sits a median ~780× past its own Cauchy knee
(`s/a² ≈ 780`, pull-weight `ρ′ ≈ 0.0013`) — effectively switched off
for essentially every correspondence point — while the normal, axis,
point-to-line and rim terms are at or inside their knees. So the
first iteration is driven by normals/axis/rim, not by point position;
the term that is off is the point-to-point one. Nothing was tuned.
The second test pot (Tray 000) is still unmeasured, and it is the
one that can show the method works at all.

Weight: one pot measured, at zero; the capability half rests on Tray 000.
Next: tickets 12 and 02-Eq.6 measured out, Cauchy restored (1-4 seats
again) — next Tray 000, the pot that can show the method works at all.
Viewer: `juglet_sfs29`

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

`docs/notes/ASSEMBLY_FAILURE_ROOT_CAUSE.md`, `docs/notes/ROOT_CAUSE_DIAGNOSIS.md` (2025-11-04),
`docs/notes/BOUNDARY_RADIUS_FIX.md`, `docs/notes/PREPROCESSING_SUCCESS_SUMMARY.md` (2025-11-05),
`docs/notes/NURBS_FIX_RESULTS.md`, `docs/notes/STEP_BY_STEP_COMPARISON.md`. Conservator, 2026-09-18: Rabati is
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

- Both `docs/notes/FORK_VS_UPSTREAM.md` docs reviewed claim-by-claim and committed.
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

## 2026-10-09 — row 1: first fully-correct pair (1-4 both ways), KEEP 0.610

2/8 + 1/15 (k=2 = pair 1-4 at 7.8°/20–26mm, merged 5×, final 6-piece
graph — largest joined yet). 16 states, profile 99/0, diverged 140,
68 axis rejections firing. Tighter gate → beam spends budget on true
merges (observed, mechanism unproven). Next row per ticket order.

## 2026-10-05 — row 6: floor kills garbage 3.9×, kept (upstream value)

Same-node pair (bm065): MINIMUM 1→6 drops diverged solves 545→140 with
matching intact (1-2 survives, value 10→6). No recovery (0/8 both —
floor bounds like the guard). Caveat: Juglet assembly impact unmeasured
(no baseline exists). Next row per ticket order.

## 2026-10-03 — ticket 04 resolved: SG arm removed, verified gone

Rebuild exit 0, binary newer than every source, zero SG strings.
Default path byte-identical by construction. One smoother, no switch.

## 2026-10-05 — determinism SOURCE found: node-dependent FP, same-node identical

Node-pinned pair on bm179 (jobs 32155220 + 32273451): identical on every
number (0/8, 0/15, 235.000 1+6+1, 13 states, 40/1, matching/pruning md5s).
Cross-node pairs always differ. Source: CPU-arch vector FP paths, not
threads (all off), not heap garbage (perturb didn't fix), no live code
races found. Standing rule: pin `--nodelist` for every assembly A/B;
cross-node verdicts keep failure direction, lose positive claims to
draws. Ticket 12 resolved on this basis (fix is operational, not code).
(Guard run in the same series: 550 loud rejects, zero aborts — guard
holds as designed; ticket 11.)

## 2026-10-02 — ticket 10 resolved: dead code gone, build green

Group A (9 unbuilt files) + Group B (uncalled twin, mode-1 branch,
disabled hub block, dead manager) deleted in separate commits;
in-container rebuild exit 0, binary verified current. Per-item
reachability re-checked at deletion time. Numbers waived by
construction (unreachable code + print-only). Standing: the tree has
no known-dead code in the live path. (Ticket 08 resolved earlier the same
day: grouping→25° stays, correspondence prune reverted as hurting, inlier
30° measured load-bearing — see its ticket.)

## 2026-10-02 — control: iterations load-bearing (0/8 at 50 vs 2/8 at 150)

Registration-50 (job 32087141) vs Registration-150 (job 31898200), all else
identical: 0/8 vs 2/8. Symmetric single-run isolation. 150 restored (it
earned it). Ticket 07 outer-count item conclusive-ish.
`POT_A_ORIG` restore postponed again — ticket 08's A/Bs need the POT_A
block next; restore lands after the last assembly run, and the ticket
says so.

## 2026-10-02 — 1-2 dies multi-stage, gate exonerated; poses own it

Witnessed pair: diverged attempts die at table-miss, sane-pose survivors
die at overlap (54–1065mm² interpenetration — correctly rejected).
`DetectIntersection` verified as genuine intrusion test. Overlap-gate
changes NOT indicated by 1-2 (assembly-09 owns shape independently).
Root cause stays poses. Aside: <5mm centroid auto-reject (area 1000)
matters for Juglet-scale pieces, not Pot_A.

Seven directed edges transform-similar; MERGETABLE ledger eliminates all
but {1→3, 3→1, 1→4} (pairs 1-2/2-4 never attempted). Pair 1-2 — eye-witnessed
genuine, probe 15/15 member, pieces transform-exact — survived matching
(value 10); death located 2026-10-02 above (table-miss for diverged
attempts, overlap gate for sane-pose survivors).

`CountResult` counts DIRECTED edges (total=30): "0/15 (3.333%)" is exactly
one passing directed edge (k=1, display truncates, percent exact). Context:
Registration-150 run (job 31898200) placed pieces 2, 3 exactly and piece 1
within 0.3°/2.0mm — first 2/8 in the series (ticket 07, single-run
evidence, control rerun owed). Sherd 2/8 (poses) is an independent
criterion from edges. Zero undirected pairs pass, so ticket 09's
pair-level "nothing fully recovered" stands. Staged GT graph
byte-identical to the probe's. Passing edge bounded to {1→3, 3→1, 1→4}
(transform-similar AND merge-attempted; pairs 1-2/2-4 never attempted).

## 2026-10-01 — fix 1 works as intended; 0/8 stands on poses+scores

Std-dev run (job 31843205): profile 87/2 (was 4/19), only kills a false
[1,5,8] config twice — every true merge passes. Beam chains 5–6-piece
graphs over 21 states (best 232) that are all wrong: 0/8 + 0/15.
Ticket 06 resolved (statistic fixed+measured, fit parked, guard done).
Profile layer cleared; what remains is poses (07/T6: 62–179° off) and
merge scoring/selection (08, assembly-09). 2026-10-13: ticket 09 resolved as SHAPE record (all five overlap stages KEPT at 50=paper, FM:1974 bare comment cited, no value moves); T6 re-measurement still open.

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

## 2026-10-12 — cauchy-scales series CLOSED: tolerance is not the knob (E-0…E-6)

What this means first: six same-node runs moved every tolerance the
solver has — distance online, half-online, normal tight, axis
tight, Huber kernel — and the seating never grew past the live 2/8.
The robust kernels are necessary (suppressing them starves the
solve, loosening them admits garbage that outscores true pairs) but
tuning them cannot seat the missing six. The remainder belongs to
the solver setup or the scorer, both already named as standing
routes. S1 itself stays open until something seats 8/8; the
tolerance contribution to it is done.

Per-arm entries below (E-2…E-6, newest first); E-0 measurement in
"Where it stands". Series record: cauchy-scales spec + tickets
01–06 (all done), forensics in juglet-sfs ticket 11.

## 2026-10-12 — E-6 Huber diagnostic: same failure under a different kernel, not kernel-driven (cauchy-scales 06)

What this means first: swapping the robustness rule from Cauchy to
Huber — at the original scales, everything else identical — produced
the same seating failure (0/8, pair 1-4 alive, no divergence). The
wrong-answer preference is not a quirk of the Cauchy curve; it
survives a kernel change. The tolerance line is now closed: all four
classes tested or cited, both kernel families tried, and the answer
never moved the seating. What owns the remainder is the solver setup
or the scorer — both already named as standing routes, neither
touched by this series.

Cauchy-scales ticket 06 Arm H, single Huber arm, same-node run on
spartan-bm065 (job 32756813, COMPLETED 0:0 in 00:14:55, commit
`34cf112`, 38,934 dump files). All four literals restored to
solver-01 root (dist 4.0, rest 1.8) with `CauchyLoss` → `HuberLoss`
at the three hardcoded pairwise solve sites; weights/caps/solver
untouched. Scaffold health passed — dump `a`-column shows the
restored values at all sites, graph env path untouched.

Outcome against solver-01 (same node): Sherd accuracy 2/8 →
**0/8**, Edge 1/15 → **0/15**, PROFILE 783 → 690, DIVERGED-SOLVE 0 →
0 (Huber showed no far-init divergence where Cauchy held 0 — and no
false-positive amplification either), best assembly 7/8 with score
262.000 → **239.000**. Score fell WITH accuracy — the starvation
pattern, not the predicted amplification. SURVIVED 14 /
FEATURE_MATCHING 1 unchanged, pair 1-4 still SURVIVED (value 21.0).
Nothing adopted per the ticket's rule. Tukey does NOT run — its gate
was Huber ranking surprising, and it wasn't. Evidence in
`.scratch/cauchy-scales/issues/06-loss-shape-diagnostic.md`;
forensics pointer in juglet-sfs ticket 11.

## 2026-10-12 — E-5 tight axis tolerance: suppressed axis, seating holds, permissive earned (cauchy-scales 05)

What this means first: the axis rule was tightened to full
suppression — and unlike the normal tightening, nothing got worse.
Seating held at 0/8 while the score rose 239 → 258: suppressing axis
reshuffled which wrong answer scores best without seating anything.
The ticket's HOLDS branch fired, so the permissive follow-up
(`a_axis = 2.8`, knee at the inlier median — the axis analogue of
E-3's p50 arm) is earned and runs next. It is the last tolerance arm;
after it comes only the shape diagnostic.

Cauchy-scales ticket 05 Arm T, single tight arm, same-node run on
spartan-bm065 (job 32753136, COMPLETED 0:0 in 00:15:36, commit
`e08e764`, 33,154 dump files). Tight tolerance 0.5 at the three
hardcoded pairwise solve sites; everything else frozen at the E-4
tree (dist 119.0, norm 0.5, rim 1.8, `w_a = 0.1`). The primary read
landed on the ticket's arithmetic prediction — `s/a²` median
27.4/47.6/17.9 with pull-weight `ρ′` median 0.035/0.021/0.053 at
iter 1 (was 2.45 / 0.29): axis fully suppressed at entry, as
designed. Rim rows at live 1.8 (123,114 + 140,834 rows) confirm the
rim-by-citation basis — already inlier-quality, no run spent.

Outcome against the E-4 control (same node): Sherd accuracy 0/8 →
**0/8**, Edge 0/15 → **0/15**, PROFILE 497 → 730, DIVERGED-SOLVE 0 →
0 (no new divergence), best assembly 7/8 with score 239.000 →
**258.000**. Score rose WITHOUT accuracy — the reshuffle signature
again, this time from removing a term rather than adding one.
SURVIVED 14 / FEATURE_MATCHING 1 unchanged, pair 1-4 still SURVIVED
(value 21.0). Not tuned in place per the ticket's rule. Evidence in
`.scratch/cauchy-scales/issues/05-axis-rim-sweeps.md`; forensics
pointer in juglet-sfs ticket 11.

## 2026-10-12 — E-4 tight normal tolerance: true pairs strangled, normals CLOSED (cauchy-scales 04)

What this means first: the "normals must agree" rule was tightened to
the strictest principled setting — the graph path's own default — and
both recovered joins were lost again, this time with the score FALLING
to 239 (below the control's 262, below E-2/E-3's 266). Tightening kills
the rotated false matches, but it kills the true pairs first: the
median true-pair disagreement at entry sits just above the tight knee,
so true pairs keep only partial weight while the solve proceeds on
axis/line/rim alone. The ticket's FALLS branch fired, so the permissive
follow-up does not run. Normals are now closed at both ends — live
keeps 2/8, tight falls to 0/8 — and the only tolerance classes left are
axis/rim (05) plus the shape diagnostic (06).

Cauchy-scales ticket 04 Arm T, single tight arm, same-node run on
spartan-bm065 (job 32743207, COMPLETED 0:0 in 00:11:33, commit
`cd26465`, 30,525 dump files). Tight tolerance 0.5 (graph path's own
normal default) at the three hardcoded pairwise solve sites; distance
kept at E-3's 119.0, axis/rim at 1.8, weights/caps/solver untouched.
The primary read landed on the ticket's arithmetic prediction —
`s/a²` median 53.0/70.3/68.7 with pull-weight `ρ′` median
0.019/0.014/0.014 at entry (was 4.1–5.4 / 0.18): normals ~fully
suppressed at entry, as designed.

Outcome against the cited control (solver-01, same node): Sherd
accuracy 2/8 → **0/8**, Edge 1/15 → **0/15**, PROFILE 783 → 497,
DIVERGED-SOLVE 0 → 0 (no new divergence), best assembly 7/8 with score
262.000 → **239.000**. Score fell WITH accuracy this time — the
solve starved rather than admitted garbage. SURVIVED 14 /
FEATURE_MATCHING 1 unchanged, pair 1-4 still SURVIVED. Not tuned in
place per the ticket's rule. Evidence in
`.scratch/cauchy-scales/issues/04-normal-sweep.md`; forensics pointer
in juglet-sfs ticket 11.

## 2026-10-12 — E-3 fixed distance at p50: half-online, joins lost, distance CLOSED (cauchy-scales 03)

What this means first: the "put the points onto each other" rule was set
to exactly half strength — the middle ground between fully off and fully
on — and both recovered joins were still lost while the score matched
E-2's false-match high. So there is no dial setting on this rule that
keeps the true joins: fully off (2/8), half on (0/8), fully on (0/8).
This closes the distance direction — it is a finding about the method,
not a broken measurement, not a wrong comparison.

Cauchy-scales ticket 03, single fixed arm, same-node run on
spartan-bm065 (job 32729140, COMPLETED 0:0 in 00:20:09, commit
`2098f55`, 40,121 dump files). Fixed tolerance 119.0 (the measured
point-to-point median) at the three hardcoded solve sites; helper
retained as record, weights/caps/solver untouched. The primary read
landed on the ticket's arithmetic prediction — `s/a²` median
1.005/0.859/0.856 with pull-weight `ρ′` median 0.50/0.54/0.54 at entry
(was 780 / 0.0013): the term is exactly half-online, as designed.

Outcome against the cited control (solver-01, same node): Sherd
accuracy 2/8 → **0/8**, Edge 1/15 → **0/15**, PROFILE 783 → 831,
DIVERGED-SOLVE 0 → 0 (no new divergence), best assembly 7/8 with score
262.000 → **266.000** — identical to E-2's score at the same 0/8. Score
level while accuracy goes to zero is the false-matches signature a
third time, now at half strength. SURVIVED 14 / FEATURE_MATCHING 1
unchanged, pair 1-4 still SURVIVED. Not tuned in place per the ticket's
rule. Full table in `.scratch/cauchy-scales/issues/03-distance-sweep.md`
verdict; forensics pointer in juglet-sfs ticket 11.

The step-0 scorer read (zero new runs, both logs on disk) answered YES:
the scorer prefers the wrong assembly — 266 points at 0/8 outscores 262
at 2/8 with identical pairwise survival, the split happening downstream
in merge/graph scoring. Tolerance and scoring are separate variables;
that finding routes to the scorer's owner alongside this verdict.
(Resolved 2026-10-14: ticket-12 row 7 removed the scorer with numbers —
identical pairs, scores-only movement.)
Why this matters for S1: the distance term cannot come online anywhere
without admitting false matches — the 2/8 must come from another class.
Next: tickets 04 (normals) / 05 (axis). Mechanism statement, not a join
count; the capability half still needs Tray 000.

## 2026-10-12 — E-2 graduated distance: term online, joins lost (cauchy-scales 02)

What this means first: when the solver starts, the rule that says "put
one fragment's points onto the other's matching points" was contributing
almost nothing (pull-weight ~0.001), and waking it up with a permissive
tolerance cost both recovered joins instead of growing them. So this is
a finding about the method — not a broken measurement, not a wrong
comparison.

Cauchy-scales ticket 02, behaviour-change hunk, same-node run on
spartan-bm065 (job 32722789, COMPLETED 0:0 in 00:20:16, commit
`fd3e42f`, 39,944 dump files). The schedule (distance tolerance
260/181/119/4.0 across iteration bands, three hardcoded solve sites;
in-run control `IcpIncGraphAxis` untouched on env values) worked
mechanically: the dump's tolerance column really reads 260 at entry,
and the primary read landed on the ticket's arithmetic prediction —
point-to-point `s/a²` median 0.18–0.21 (was ~780), pull-weight `ρ′`
median 0.83–0.85 (was 0.0013). The term is online.

Outcome against the cited control (solver-01, same node): Sherd
accuracy 2/8 → **0/8**, Edge 1/15 → **0/15**, PROFILE 783 → 855,
DIVERGED-SOLVE 0 → 0 (no new divergence), best assembly 7/8 with score
262.000 → **266.000**. Score up while accuracy goes to zero is the
signature of false matches admitted — the "loose admits false matches"
trade the research report warned of. SURVIVED 14 / FEATURE_MATCHING 1
unchanged, pair 1-4 still SURVIVED. Per the ticket's rule the schedule
was not tuned in place; its iteration boundaries stay free parameters.
Full table in `.scratch/cauchy-scales/issues/02-graduated-schedule.md`
verdict; forensics pointer in juglet-sfs ticket 11.

Why this matters for S1: the 2/8 cannot be grown by waking the distance
term alone — the next question (ticket 03) is whether it can come online
anywhere without admitting false matches, or the joins must come from a
different class. Mechanism statement, not a join count; the capability
half still needs Tray 000.

## 2026-10-11 — E-0 measured: the point-to-point term is off at entry (cauchy-scales 01)

What was measured, in the conservator's terms first: when the solver
starts, the rule that says "put one fragment's points onto the other
fragment's matching points" is contributing almost nothing — about a
thousandth of its intended pull on virtually every point pair — because
those point pairs are about 780 times further apart than the solver's
own tolerance for calling them outliers. The rules that are actually
steering the first attempt are the surface-normal agreement, the
axis-alignment, the profile-line distance and the rim fit, all of which
sit at or inside their tolerances.

Cauchy scales ticket 01, hunk log-only, same-node run on spartan-bm065
(job 32696452, COMPLETED 0:0 in 00:44:55), 38,584 dump files, 11.9 M
residual rows, Pot_A full assembly. Per class at iteration 0:
point-to-point `s/a²` median 774–890 (Icp 889.5, Registration-5 774.5,
Registration-6 777.4) with pull-weight `ρ′` median 0.0013 and 99.81%
past the `s/a² = 9` point; normals `s/a²` median 4.07–5.40 (`ρ′` 0.18);
after iteration 1 the normals become inlier-quality (`s/a²` 0.11,
`ρ′` 0.90). Axis (Cao soft-min) median inlier ≈ 28 mm at s=100 against
a knee at 1.8, so ~47% of surface points already fall past it.
Point-to-line 0.40 / 0.87 (ρ′), rim 0.16 (`ρ′` 0.97). Full table, the
per-site iter-0/iter-1 class census, the empirical add-order parity
confirmation (including the 2-dim case the ticket's parity rule could
not express) and the surprises are in
`.scratch/cauchy-scales/issues/01-log-residual-norms.md`.

Zero-behavior-change verified: with the dump env var unset the run is
outcome-identical to the control (2/8 sherds, 1/15 edges, PROFILE 783,
0 diverged solves). Nothing tuned, nothing scheduled.

Why this matters for S1: the "poses are 62–179° off" finding converges
on refinement robustness, and the robust weighing of the point-to-point
term is one of the levers whose current setting is now known rather
than assumed. It is a **mechanism statement, not a join count** — it
does not move the capability half, which still needs Tray 000.

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

## 2026-10-14 — pristine Pot A control: the pipeline seats 8/8 on suitable material

What this means first: the authors' own software, untouched, joined all 8
sherds of the ordinary round pot correctly, finding all 15 true seams. So the
instrument works on suitable material — the Juglet zero reads as material
scope (thick handmade sherds whose break traces the join test cannot see),
not a broken pipeline. One pot, one run: a control lead, not a universal claim.

- **Gate probe 15/15** reproduced on the authors' Pot A data (laptop-side run
  of `artifacts/juglet_run1/gate_probe_b0.py pota_orig`; all 17 bundle files
  md5-verified byte-identical to the cluster inputs).
- **E2E at the Table V gauge (k=5/b=3): Sherd accuracy 8/8 (100%), Edge
  accuracy 15/15 (100)** — exactly the paper's Table V Pot A bar (SfS++ SA
  100 / EA 100). Scoring run job 32983523 (image-build job 32982520), COMPLETED 0:0. (87.3% is the
  10-pot ALL mix, Table VII — never the bar.)
- **Purity:** upstream @ `9195c01`, `git status` clean; zero `class/`
  changes. Declared INFRA only: container recipe
  (`hpc/containers/container-pristine-assembly.def`), batch entry shim
  (`hpc/tools/main_pristine_batch.cpp`, upstream main minus viewer calls,
  after Xvfb proven infeasible — diag job 32976970 segfaulted 139/134/139),
  `/Dataset` bind, sbatch files. Missing `Surface_F.pcd` files tolerated by
  upstream's own graceful `ReadPCD` return.
- **Eye:** pair 2-5 staged (`visual-qa/viewer/pairs/pristine_pota_25.json`,
  manifest `visual-qa/preview/manifest_pristine_pota_25.json`); attempt vs GT
  surface coincidence median 0.54 mm, p90 1.16 mm. Whole-pot pair staged
  2026-10-15 (`pristine_pota_whole.json`, median 0.69 mm / p90 1.36 mm).
  Witnessed by the conservator 2026-10-15: "baseline verified, it rebuilt
  perfectly" — the capability-half of S1 is settled on this control.

Closeout 2026-10-15: pristine-pot-a-control spec + ticket 01 closed (all boxes
checked with worker-report evidence above); preprocessing follow-on result
recorded here and in E1 `Where it stands`; board rebuilt in the same commit.
## 2026-10-15 — follow-on: untouched preprocessing does NOT regenerate the authors' inputs

What this means first: the control above proves the assembler works *given
the authors' prepared inputs*. Running the **preprocessing** side untouched
on the authors' own Pot A scans does not reproduce those inputs — so the
chain from scans is unverified-to-failing, and the control's inputs remain
irreplaceable by the released code. Same verdict class as the Juglet arm:
**case 1, the method genuinely failing** — the gate and the answer key both
check out (ORIG arm 8/8 + 15/15 in the same job; GT hashes identical).

- **Fresh C++ outputs vs authors' inputs, all at published GT:** piece 1
  correct (0.52 mm median, 100% of the authors' tracing within 2 mm);
  pieces 2–8 shifted near-rigidly upward (+1.8–3.5 mm Z, 2.9–3.8 mm
  overall). Point counts match — positions don't. The shift is already in
  the fresh *surfaces* (piece 2: 3.65 mm), so edge tracing faithfully
  followed shifted input; the damage enters at the surface step.
- **Two blockers, ordered:** (1) *fatal file format* — fresh edge files
  carry no `#` segment headers the untouched reader requires
  (`data_structure.cpp:178`), so assembly loads 0 of 8 and places nothing
  (then crashes on the empty input — consequence, not a separate defect);
  only our fork writes headers. (2) *surface geometry* — even with headers
  fixed, fresh-vs-authors gate is 6/15 (only piece 1's seams pass).
- **MATLAB axes incomplete as released:** missing loader file,
  Windows-only paths, a dimension error on current MATLAB, and a 59-line
  refinement file existing only in our fork — axes leg closed as a
  documented gap per conservator call. Untouched source throughout
  (upstream DominicoRyu @ `d5ee6865`, fresh headless container
  `be24a382…`, virtual display for the viewer-globals abort).
- **Eye:** fresh (red) vs authors' (blue) at GT staged at
  `artifacts/visual-qa/task3_fresh_vs_orig/` (`whole_*`, `seam12_*` ~4 mm
  gap, `seam24_*` coherent shift) + `desc.md` — awaiting conservator
  witness; numbers above stand regardless.
- Evidence: `C:\PR\SfSpp_preprocessing\artifacts\untouched_pota_verify_20261015/`
  (`TASK3_EVIDENCE.md`, gate/eye-check scripts, sbatch, both binary logs);
  Spartan isolated tree + rundir `sfs_main/task3_pota_verify_33036311/`.

Weight: one pot, one run per arm — but full-population counts with a
reproduced 15/15 control, so chance explains nothing. Consequence: every
Pot_A number measured from the authors' data (15/15 gates, E-6 rows) was
measuring prepared inputs, never this pipeline's output from scans.

## 2026-10-20 — Pristine-scorer restore proven on the Pot A control (ticket 02)

What this means first: our copy had drifted from the authors' program
(false upstream comment moved one gate 0.436->0.610; a veto the authors
never had killed true pairs), which voided a month of control runs. The
restore (main reset to 11f4827, scoring sources back to upstream 9195c01
plus only the 3 kept run-glue hunks) was rebuilt clean on Spartan and
seated the authors' own Pot A fully: **8/8 fragments, 15/15 joins** (job
33502035), confirmed by eye on the render. The ruler is trusted again:
controls that fail from here fail on the material or the method, not on
our copy of the program.
