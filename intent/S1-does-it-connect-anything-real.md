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

Weight: one pot measured on Juglet (2/9 sherds, 1/18 edges under signed 5/3; 4/14 measured joins have an in-bar candidate; `Pot_A` control solves 8/8 sherds, 15/15 edges).
Next: Ticket 354's locked 2-DoF grid search proved confounded by an oracle dot pool frozen at the ground-truth pose (`locked354.py:103-105`), failed to separate joins from non-joins even with that oracle, and is rejected in favour of diagnosing where the stock assembler's own `CalculateFeatureAxisless` -> `LCS` -> `Icp` pair stage loses Juglet joins compared to `Pot_A` (**Ticket 355**).
Viewer: `juglet_sfs29`

Tickets 336-338 (2026-11-13) moved the question from accuracy to *where* a join is
lost. 336's stage trace found that in 22 of the 32 seam-arm cells the two sherds
never share a graph at any step, so no stage ever sets their relative pose; 337
swept the search width and the loss survived it; 338 followed one named,
GT-good candidate per seam through every boundary. On the Juglet at signed 5/3
(2/9 sherds, 1/18 edges, all three arms identical) seam 1-8 is joined and tracks
its candidate to 1.24°/1.33 mm, while seam 7-9 — the one 338 opened on — has a
candidate at 5.39°/4.24 mm — just past the 5° test, but about thirty times closer
than the 163°/94 mm the assembly produced — that never enters a state graph in
either repeat, and 1-9's only usable candidate is already 8.82° before the search
starts; 1-2 is admitted from step 2 and still ends 84°/94 mm apart. So wherever an
edge is handed to the group fit it adds at most 0.55° and 0.15 mm: **the join is
lost before the fit — in candidate admission and state-graph construction — not in
the group pose fit.** The next fix belongs there, not at the pose stage.

Ticket 339 (2026-11-13) then asked *which* boundary, and the answer is a window, not a
gate. A trace of every candidate offered to a state, with its priority index and its fate,
shows 7-9 first offered at index 12 and 1-9 at index 30 — in all five repeats — while the
build only tries the first three (BRANCH_b 3, unchanged since 337 swept it). Control seam
1-8 is offered at index 0 and kept from step 0. So no threshold refuses either failing
join: 7-9 is never among the first three tries, and 1-9 is once, at index 0, where the plausibility
check refused it (inlier 164). 7-9 gets no graph edge anywhere,
which is why 338's empty column was a lower bound and not a verdict. The lever is therefore
the *order* of the priority list (CalculateMatchingScore / InlierCompare), not a wider
window. The instrument itself is print-only: nine signed runs, eight at 2/9 sherds and
1/18 edges, the single 3/9 the slowest run of the nine, with identical trace readings
across the repeats — the residual run-to-run variety is twelve-thread Ceres, not the switch.

Ticket 340 (2026-11-14) named the candidate the search actually applies, and the answer
splits the two failing seams from the third. Seam 1-8 accepts candidate 70 (0.70 deg / 1.21 mm)
at step 0 and the assembled pair reads 1.24 / 1.33. Seam 1-2 accepts its in-bar candidate, id 1
(2.14 deg / 2.20 mm), at step 2 — and still reads 84 deg / 94 mm in the end, because a later state
accepts the same pair again at step 3 with a different edge (`index_` 3) that the pruned table never
listed. Seams 7-9 and 1-9 accept nothing at all — zero accept lines in either repeat — which is
ticket 339's window verdict seen from the accept side rather than trusted from the offer side. The
instrument is print-only and inert: four arms, signed 2/9 sherds and 1/18 edges in both trace and
fidelity arms, Pot A 8/8 and 15/15, tree md5 unchanged, and 31 accept lines per repeat identical
across the two. So the falls are not "the right pair is never offered a good candidate": the good
join is admitted and then either refused elsewhere, overwritten by a second edge of the same pair,
or carried into a state that loses. Ticket 342 asks which of those the winner carries.
Ticket 342 (2026-11-14) asked which join the winning state carries, and the answer is neither of the
two edges on pair 1-2. The winner prints no 1-2 edge in either repeat (r1 state 30, score 260, four
edges over three clusters; r2 state 33, score 263, five edges over three clusters), and the last build
step's group that takes piece 2 is {2,4,5,6,7,9} - piece 1 is not in it. The 84 deg / 94 mm on that
pair is therefore not an assembly error on a joined pair: the state that wins never joins them, and
that pose is the honest distance between pieces the search left in different clusters. What the
reading can and cannot say is bounded by the instrument: `edge_` records only priority and bridge
entries (class/ranking_system.cpp:2270, :2334; the merge copies nodes at :2312-2320 but not `edge_`),
so "carries-none" means no such entry, and r1's four edges over three clusters with all nine shards
matched shows the printed list is a subset of the joins. The two repeats agree on the accuracy line
and on the 1-8 / 3-8 / 5-7 backbone with identical scores, but not on the state itself - the winning
state is not reproducible between repeats, only its score. Ticket 343 asks the question this leaves:
whether the accepted 1-2 join is dropped by the search or only by the record.

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

## 2026-11-14 — The gate that refuses joins inside the bar (ticket 341)

What this means first: the gate is not refusing the joins we measured. For the three
pairs whose best candidate sits inside the 5 deg / 5 mm bar, `CheckGraphPlausibility`
refuses on its own numbers — a profile distance or an edge-overlap area — and the edge
it judges carries a score no candidate-stage row has.

- 5-7: eight refusals, profile distance 8.856-8.860 mm against the 7.0 bar (27% over),
  every one on an edge of score .35974 / inlier 77; the pair's candidate is 160
  (.322249 / 72, 4.71 deg / 3.69 mm). It was applied once, at step 0. The 8.86 mm, and every
  other distance here, is a group-shape number: the profile is built from every piece of
  the chunk (`class/ranking_system.cpp:2535-2537`), not from the pair, and the printed value
  is the first point over the bar in the first failing bin, so later bins are never examined.
- 4-7: six refusals at 7.033-11.271 mm; the picker’s only 4-7 accept is 144, in both
  repeats —
  18.69 deg / 14.86 mm, outside the bar — not the in-bar 145 (3.35 deg / 2.68 mm).
- 6-9: three refusals on overlap 73.70 / 76.06 / 76.06 against the 50 bar; the applied
  edge is id 66 in r1 and 55 in r2, neither of which has a candidate row, not 170
  (2.35 deg / 1.72 mm).

The step-0 finding is the structural one: the whole profile-and-rim block sits inside
`if (step_counter > 0)` (class/ranking_system.cpp:2531), so a step-0 accept is never
shape-tested; the overlap test does run at step 0. 'Accepted once, refused later' is
therefore not one edge drifting, it is a join that was never measured at all.

Weight: two trace-on repeats with identical 45-line censuses (31 profile + 14 overlap),
fidelity arm silent at 2/9 sherds and 1/18 edges, Pot A 8/8 and 15/15, print-only (106
added lines, 0 deleted). One pot, signed sweep.

Open: whether the judged edge is the table's candidate or a re-scored/regenerated one —
scores are rewritten in place (`iter->score_ = cycle.score`, and `lcs.score_ = 11.0` at
six sites), so the tuple mismatch is not proof. What follows: the bounded bar arm for
5-7 is written but held (ticket 344) until a print-only arm runs the shape test at step 0
on the edges actually applied (ticket 345) — the gate never measured a step-0 join, so
8.86 mm cannot yet be read as a property of candidate 160. The picker’s choice within 4-7
is ticket 346.

## 2026-11-14 - The join inside the bar is already over the shape bar (ticket 345)

Ticket 341 showed the plausibility gate refusing 5-7 with a profile distance of 8.86 mm against a 7.0 mm
bar, but it could not say whether the join had been good when it was made and only the growing group had
drifted. Ticket 345 measured the same quantity at step 0, on the chunk that contains the pair, without
letting it act: 5-7's own distance **at the moment of the join** is 8.90688 mm (r1) and 8.84147 mm (r2) --
the same order as the later refusals. The 1-8 control, a join that sticks, measures 5.83 mm, so the dry run
measures the gate's quantity. The drift hypothesis is dead by measurement: for 5-7 the group is already
over the bar when the edge is applied. Two consequences: ticket 344 (one bounded bar arm) has a live
premise again, and the number the gate judges is a property of the whole chunk, not of the pair
(ticket 341's second caveat).

## 2026-11-14 - The order is not the problem; the candidate never reaches a list (ticket 346)

Ticket 346 printed the number each candidate carried into the priority order (`matching_score`, a sum of
inlier counts) and joined it to the accept. Wherever the picker has two candidates of one pair, the order is
followed: within pair 1-2 at step 3 state 18 the list carries matching_score 84 and 33 and the picker takes 84;
within 4-7 at step 4 state 25 it takes 21 over 19. The failing pairs are not misordered -- pair 4-7's in-bar
candidate (3.35 deg / 2.68 mm, score .359451, inlier 28) has no row in **any** priority list in either repeat,
and pair 6-9's in-bar candidate (2.35 deg / 1.72 mm) is listed only at step 0, four steps before the edge the
search applies. A correct candidate that never enters a list cannot be ordered into a state, so the lever is
upstream of the ranking key: which candidates reach a graph at all. (342's r1 assembly was reproduced at the
edge-set level by all five trace arms; 342's r2 assembly, five edges and score 263, was not reproduced by any
of them -- the winner remains a set-level statement.)
## 2026-11-14 - The join the gate refused is a join; the reference is not a contact list

The conservator asked, after the assembler's last result, whether the answer key's pose is right
but the edgeline we generate is wrong. The first half of that question has an answer already;
getting to it also stopped a ticket.

**Tickets 341/344/345 treated 5-7 as a join inside the bar that the gate refused, and this
section first answered that it is not a join - on the answer key's list alone. That answer was
wrong.** Ticket 347 measured the mating directly, at the true pose, with exact point-to-triangle
distances and each piece's outward normals calibrated by a parity test: **5-7 is back-to-back** -
459 vertices of one piece lie within 0.1 mm of the other's surface, median normal dot **-0.910**,
75.4% opposed, spread over 29.3 mm - the same signature as the two listed joins 6-7 (-0.940,
77.6%, 30.3 mm) and 1-8 (-0.887, 77.3%, 31.9 mm). **4-7 is not**: its normals run the same way
(median **+0.448**, 4.7% opposed) over 23.5 mm - two surfaces passing alongside each other, which
proximity alone would have called a join. Controls 1-5, 1-6 and 4-5 have no vertex within 0.5 mm.

**The key's list itself does not survive the test.** Of its 18 edges, **eight stand 3.3-18.8 mm apart at
the true pose** - 1-5 3.325, 1-6 3.636, 3-7 13.851, 4-5 4.864, 4-6 7.682, 5-9 17.987,
7-8 13.871, 8-9 18.846 mm - and 6-8 stands 0.612 mm off with no contact patch at all; none of the
nine can be joints. It also omits 5-7 (which mates) and 6-9
(0.000003 mm gap, 7/6 inliers under the 2 mm / 0.85-normal probe). So `Juglet_simple_graph.txt`
is a reference answer for something other than "these surfaces meet"; it ships with the key
(`Dataset/Juglet_GT/`, md5 `ccae01dd9c897e92beb0d507baa45cea`, copied by
`run_juglet_preprocessing.sbatch:249`), it is not our derivation, and the lane's "1 of 18 joins"
is measured against a list half of whose entries cannot be joined.

**What that changes and what it does not.** The gate's refusal of 5-7 was not a mistake about
geometry: its group profile reads 8.86 mm against the 7.0 mm bar (341) and 8.91 / 8.84 mm at the
moment of the join (345) - a real join whose assembled shape is that far off, which makes the
shape number, not the join, the thing to question. Ticket 344's premise returns for 5-7, but it
stays stopped until the join count is re-grounded on measured contacts rather than the key's
list: that re-scoring is the next ticket. The seated-sherd number (2 of 9) does not depend on the
list and is unchanged; the "1 of 18 edges" number does, and should not be quoted until it is
recomputed.

**On the edgelines themselves the question has been answered once, for gap rather than stretch.**
Ticket 326 measured, at the true pose on the 1x tree, the interior line of one piece against its
neighbour: nine of the ten touching pairs come within 0.30-2.91 mm (3-7 is 14.19 mm, the one
exception), every non-contacting pair sits 3.60-21.38 mm apart, and the strict probe (2.0 mm gap
plus normal dot >= 0.85) admits 1-8, 2-9, 3-5 and 6-9 with 4-18 inliers and finds zero on every
non-contact. A line that meets its true neighbour, and meets nothing else, is not a wrong line.

What is short is the **stretch**. Whole-curve medians at the true pose are 2.3-12.7 mm (1-8
2.33 mm with 19 dots within 2 mm; 1-2 3.71 / 5; 7-9 8.93 / 3; 1-9 and 2-5 zero), against the
matcher's demand of 7 dots over about 12.6 mm - so on most pairs the two lines coincide only where
the sherds actually touch, for a few millimetres. At 3x, where 7 of 9 seams do reach that length
(ticket 333), the assembly still seats 0/9 because the scoring gates are absolute millimetres.
Denser dots do not buy seats on their own.

**Where the ceiling actually is.** Parsed from the 51 candidates of one signed run: 8 of the 10
touching pairs get a candidate at all, and only **3** have one inside the 5 deg / 5 mm bar - 1-8
(0.70 deg / 1.21 mm), 1-2 (2.14 / 2.20) and 6-9 (2.35 / 1.72). 7-9 is just outside at 5.39 / 4.24;
1-9, 2-9, 2-5 (45.9 deg) and 3-5 (48.3) are far outside; 3-7 and 6-7 have no candidate at all. The
search cannot seat more than three touching joins whatever it does with the order, the window or
the gate. So the stage to repair is candidate generation, and what decides which repair route is
right is how far along the true contact the line runs - ticket 347.

Weight: the graph decoded by the lead from the file; the mesh-contact and stretch numbers are
ticket 326's run and `SfSpp_preprocessing/artifacts/67/out/A20/T67_gtpairs.tsv`; the candidate
table recomputed by the lead from `artifacts/t339/logs/binary_juglet_signed_on_r1.log`. One pot,
one signed sweep. Open at the edges of the reference itself: 6-9 touches but is not in the graph,
and 1-4 is in the graph but was not among ticket 326's measured ten - so ticket 347 re-measures the
mesh gap of all 18 graph edges plus 5-7 and 4-7 before using any of them.

## 2026-11-14 - The contact set, measured; the edge count retired (ticket 348)

Ticket 348 measured all 36 pairs the same way and applied the rule it was given (gap <= 0.05 mm,
back-to-back fraction >= 0.40 over a patch at least 8 mm across). **14 pairs are joins:**

`1-2  1-3  1-4  1-8  1-9  2-6  2-9  3-5  3-8  5-6  5-7  5-8  6-7  6-9`

with one short contact quoted beside it, 2-8 (4.4 mm patch, 95.5% opposed). Of the key's 18 edges
**7 are joins** (1-2, 1-4, 1-8, 1-9, 2-9, 3-5, 6-7) and **11 are not**: eight stand 3.3-18.8 mm
apart, 6-8 stands 0.612 mm off with no contact patch, and 2-5 (0.0017 mm) and 7-9 (0.0009 mm) touch
without facing each other (0.0% and 14.1% opposed). **7 measured joins are unlisted:** 1-3, 2-6,
3-8, 5-6, 5-7, 5-8, 6-9.

So the denominator is **14 joinable pairs**, not 10 and not 18. Of the 51 candidates, **5 pairs have
an in-bar candidate and 4 of those 5 are real joins** (4-7 meets the pose tolerance and is not a
join at all); the winning assemblies carry **3 of 4** (342 r1), **4 of 5** (342 r2) and **3 of 4**
(346's five arms); the seated-sherd number stays **2 of 9**, unchanged and list-independent.

Retired as scoring bases: "1 of 18 edges" and "3 of 10 touching pairs". The key's list is not a
contact list; `C:/PR/artifacts/t348/contacts348.tsv` replaces it.

Two things to carry forward. **2-9 is the rule's fragile verdict** (patch 8.050 mm, 0.6% above the
floor): a slightly stricter area floor would drop a real key join, so the 14 is quoted with that
stated. **4-6 is the other way round**: the key lists it, the meshes put the pieces 7.68 mm apart,
and both 342's r1 winner and 346's five arms carry it - the assembly joins a pair that does not
touch, so a key-only edge in a winner is evidence about the matcher as much as about the key.

Weight: one pot, one true pose, exact point-to-triangle distances, reproducing ticket 347's numbers
to within 0.0003 mm across the five pairs both measured; 14 of 36 pairs are joins.

## 2026-11-14 - The candidate stage caps the joins at four of fourteen, with the key's own axes (ticket 349)

Ticket 349 censused all 14 measured joins against the 51 candidate rows of one signed sweep
(`artifacts/t339/logs/binary_juglet_signed_on_r1.log`, job 35016164). **13 of the 14 joins reach the
candidate list; only 4 arrive inside the 5 deg / 5 mm bar** - 1-2 (2.14 / 2.20), 1-8 (0.70 / 1.21),
5-7 (4.71 / 3.69), 6-9 (2.35 / 1.72). **6-7 never appears at all**, on the strongest contact in the
set (66 dots over 39.0 mm, zone median 0.344 mm, 869 vertices within 0.1 mm). Near misses miss on the
spin alone: 5-8 7.00 / 4.97, 1-9 8.82 / 3.48, 3-8 11.00 / 4.53. The rest are wrong outright (1-4
18.6 / 11.8, 2-9 15.1 / 8.2, 5-6 13.0 / 15.6, 2-6 26.2 / 19.2, 1-3 46.2 / 39.4, 3-5 48.3 / 44.0).

**The axis explanation is dead, and this is the sharper form of the ceiling.** The ticket expected our
own axis estimate to be one of the wrong numbers. It is not even in play: the run compiles with
`-DJUGLET_ORIG`, whose `axis_path` is `JUGLET_ORIG_BASE "Axes/..."` with
`JUGLET_ORIG_BASE "/Dataset/SfS_pp/"` (`class/data_path.h:1719`, `:1746-1754`), and the mounted 3x
tree's `Axes/` was copied from `Dataset/Juglet_large`, documented in `stage333.py:9-10` as "GT axes,
already 3x". Measured independently by the lead: tree-vs-key **0.000000 deg on all nine pieces**,
while `Dataset/Axes/Juglet` (our 2026-09-29 PotSAC estimate) is **1.53, 87.83, 31.82, 10.72, 44.50,
13.26, 24.13, 12.81, 73.32** deg away from the key by piece. So the 4-of-14 ceiling is measured with
the key's own axes: it is an upper bound for this candidate stage, not a number our estimate
degraded. Ticket 349's Part B (substituting the key's axes) was void and was not run; the informative
inverse - writing our estimate into the pinned tree and re-running - has never been done in this lane.

Two controls keep the census honest. **4-7** carries an in-bar candidate (3.35 / 2.68) on a pair that
is not a join and whose faces point the same way, so an in-bar candidate is not evidence of a join.
**4-6** carries only 155.2 deg / 88.6 mm in the candidate table, yet the winning state carries it as
edge #77 (ticket 342): the winner's edges do not all come from this candidate stage.

Two gaps are stated, not papered over. Ticket 347 measured line runs for only 20 pairs, so 1-3, 2-6,
3-8, 5-6 and 5-8 have no run column. And the trace prints the 51 survivors of 180 candidates, so 6-7's
absence cannot be split into "never proposed" and "pruned" without a print at the proposal stage.

Weight: one pot, one signed sweep, 14 joins; the candidate rows re-read by the lead from the raw log
and the per-pair bests reproduced exactly. The reading is `two-causes`: 6-7 dies before the list, the
other nine reach it with a wrong pose, and the axes are not the reason.

## 2026-11-14 - The tilt is removable and buys two joins; the frame was overstated by three (ticket 350)

**The frame first, because it changes every millimetre this lane has written down.** The signed runs
bind the 3x bundle ticket 333 built, not the 1x corpus: one tree unit is 3 mm. Verified against the
key's own poses - rotation difference 0.0007-0.0027 deg and `|t|` ratio exactly 3.000000 on all nine
pieces. So the profile number the gate refuses with is 2.95 mm real (8.86 / 3) against a bar of
2.33 mm real (7.0 / 3), not 8.86 against 7.0; the step-0 join measured 2.97 / 2.95 mm real; the 1-8
control 1.94 mm; and the pose bar `MM_TOL 5.0` is 1.67 mm real, not 5 mm. **No verdict changes**:
every number divides by the same 3, so every ratio stands - but the absolute millimetres in tickets
338, 341, 345 and 349 were three times too big and this is the correction.

**What the tilt is worth.** Ticket 350 replayed all 180 first-stage candidates with the rotation
component about the shared piece axis removed (spin and translation kept). Of the sixteen pairs, five
were inside the bar before and seven after: only **1-9** (8.82 -> 1.89 deg, 1.16 -> 0.78 mm) and
**5-8** (7.00 -> 1.71 deg, 1.66 -> 1.36 mm) cross in, nothing crosses out. Nine measured joins stay
outside with 5.7-165.7 deg of *spin* left over (2-9 5.7, 3-8 6.6, 3-5 7.1, 1-4 8.5, 5-6 9.9, 1-3
11.3, 2-6 14.3, 6-7 165.7, and 4-6 3.4 deg but 11.0 mm real).

**So the axis is not the lever the frame invited.** The pipeline reads a piece's axis only after the
pair rotation is fitted - it uses it to measure `axis_raw_` and to gate at 25 deg, never to constrain
the fit - and this replay shows that constraining it would buy at most two joins. The rotation error
is mostly a spin about the shared axis, i.e. the fit knows where the axis is (the axes are exact here)
and is free along it. The next question is therefore how the spin hypothesis is chosen, and whether
the contact curve determines it at all at this resolution: ticket 353 asks it offline before any code
changes.

**Two limits stated.** The replay is a counterfactual that keeps each candidate's translation, so it
does not speak for a constrained search that would also re-fit the translation. And both repeats parse
one log, so they test the arithmetic, not the run.

## 2026-11-14 - The contact curve cannot see a spin (ticket 353)

The tilt removed, every pair's spin was swept over the full circle at its candidate's translation, and the
contact curve re-measured at each angle. It does not choose a spin. By the pre-registered rule 0 of the 14
measured joins are determinable; that is partly my rule's fault (it asked for a single local minimum from an
objective that is censored at a hard 3.0 mm cutoff and has 79-303 of them), and repaired to a single 0.20 mm
basin it gives **2 of 14** - **5-7** and **6-9**, the two the fit already places correctly, where the repair
asks for a 0.50 deg and a 0.25 deg move against errors of 1.55 and 0.24 deg.

**Where the fit is wrong the curve is flat.** Ten of the 14 joins move by less than 0.20 mm within +/-5 deg of
their own minimum, and the 0.20 mm basin is 2.2-28.0 deg wide. A 5 deg spin slides a breakline dot 1.07-1.54 mm
along the surface - five to eight times that floor - yet the in-zone RMS barely moves, because it measures the
*perpendicular* gap and the two arcs run nearly parallel over the short stretch where they meet (348: 0.2-0.7 mm
median over 11-39 mm). At the best spin the key's rotation error grows on 8 of the 13 measurable joins and
shrinks on 5, which is what a noise-level argmin looks like.

**So the residue is not a spin the fit failed to optimise.** The pair fit's 3-14 deg of residual spin is real,
but the contact curve cannot see it, and the 165.65 deg on 6-7 is a genuine back-to-front correspondence (the
run's own `SIGN-AXIS: accepted, axes are direction-consistent (0% of pairs > 90 deg)` rules out an axis-sign
artefact). The lever is therefore the data - denser or longer contact, i.e. the scale lane - or an objective
that registers along the arc rather than across it; not another spin estimator, and not the axis, which 350
showed buys two joins.

**Limits.** One log (r1), translation held throughout: this tests the spin at a fixed placement, not a joint
pose search, and not run-to-run variation. The two rows with no dots in the zone at any angle (3-5, 4-6) read
not-determinable for the translation's sake.
**Correction (2026-11-14).** The conclusion above overreaches, and the sentence "the lever is the data" is
withdrawn. Three choices in 353 each suppress the signal: the sweep ran at the candidate's own translation
(up to 24 mm off the axis, where the pieces are not in contact at all), it scored only dots already inside
3.0 mm so a wrong spin can look good by losing the dots that would show it, and it required the gap to rise
on **both** sides of the minimum. Re-run at the true pose with the dot set frozen at zero turn (the lead's
own reproduction, dots `C:/PR/artifacts/t334/edgelines`, poses `C:/PR/artifacts/t350/gt`, axes
`C:/PR/artifacts/t349/axischeck/key`), **12 of the 14 joins move by 0.20 mm or more when turned 5 deg in at
least one direction** - 5-6 +1.09, 6-9 +0.85, 2-9 +0.80, 1-3 +0.62, 3-5 +0.59 - against the 4 that the
both-sides test allowed. The pattern is the seam direction: on an unrolled map most seams run around the pot
and a turn slides them along themselves, while 5-6, the only near-vertical seam, is the sharpest.

So 353's repaired count of 2 of 14 is a statement about *that* metric, not about the information in the
curve: the breakline carries a weak but real spin signal, and how weak is not yet pinned. The two anchors the
new analysis quotes (1-2 0.816, 5-6 0.596 mm) reproduce in the lead's own reading only with a 1.5 mm pool
(0.813, 0.552), so the metric has to be fixed before any search is trusted with it. Ticket 354 does that and
then runs the search the pipeline does not do: both pieces on the known axis, only turn and height free, over
all 36 pairs, scored so that losing dots costs.

## 2026-11-14 - The placement is nearly solved under an oracle pool; the score cannot choose — and locked grid search is not the solution (ticket 354)

With the piece axis known and one partner held at its key pose, a two-degree-of-freedom search (turn about the pot axis, slide along it; 22,006 placements per pair at 0.1 deg and 0.1 mm) puts **13 of the 14 measured joins inside the 5 deg / 1.67 mm bar** - the miss is 6-9 at 5.50 deg - and every join shows a single clean well at the key. The end-for-end family is weaker: 6 of 14.

**Claim 2 (oracle dot-pool confound in `locked354.py`).** `locked354.py:103-105` and `:115-130` select the dot subset `self.ia, self.ib` (`d <= 3.0 mm`) **at the ground-truth pose (`theta = 0, h = 0`)** and freeze that ground-truth patch across the entire `360 deg x 30 mm` sweep. That makes the 13-of-14 placement rate an **oracle-contact upper bound**, not a blind search: moving away from `(0, 0)` pulls those ground-truth contact dots apart into the `3.0 mm` cap, and 15 non-joins with `0+0` dots at ground truth sit flat at `3.0000 mm` everywhere without ever testing whether they touch at another `(theta, h)`. Without a ground-truth contact mask (`t354probe/sweep_true.py` `capped(0)` vs `capped(5)` over all breakline dots), a 5 deg turn around the axis moves pair `1-2` by only `+0.008 mm` (`2.517 -> 2.525 mm`) and lowers the capped score on `1-9` (`2.794 -> 2.791`), `2-6` (`2.459 -> 2.458`), and `5-8` (`2.794 -> 2.784`).

**Claim 1 (ranking failure even with the oracle pool).** Seven of the 22 non-joins hold dots within 3.0 mm of the other breakline at the key; all seven score at or below the worst join (1.8839 mm on `1-9`) and three beat the best join (`2-5` 0.3010, `6-8` 0.4038, `2-8` 0.5650 mm against `3-5` at 0.6738 mm). Those three rest on 2-4 dots per side, so the score is normalised by a sliver; the worse news is the two non-joins with a join-sized pool — `4-7` (12+14 dots, 0.8218 mm) and `7-9` (7+7 dots, 0.8202 mm) sit mid-distribution, better than five joins (`0.5147 mm` vs `0.5542 mm` join median on dense 3x at 1.8 mm). Those are the two pairs ticket 348 measured as passing within 0.1 mm **without mating** (`frac_back_to_back` 0.047 and 0.141).

The conservator's anchor pair (0.816 / 0.596 mm) is the **dense 3x dot set at a pool radius of 1.85-1.9 mm** (0.8134 / 0.5923 there); on the 1x dots the same measurement at a 1.5 mm pool is 0.8133 / 0.5524, and 5-6 steps 0.5524 -> 0.6629 -> 0.7724 between pool 1.5 and 1.85, so the two anchor numbers are two instruments rather than a contradiction.

**Conservator decision (2026-11-14): do NOT build a locked grid search into the assembler.** As the conservator pointed out, the stock SfS++ assembler already solves much more complex multi-sherd sample pots (`Pot_A` at `8/8 sherds, 15/15 edges`) without a 2-DoF grid search and without knowing ground truth: `CalculateFeatureAxisless` (`class/filter.cpp:161-219`) extracts spin- and height-invariant cylindrical derivative features `(dr/ds, dz/ds, r*dtheta/ds, thickness)`, `FeatureComp / LCS` (`class/feature_matching.cpp:386-496`) matches contiguous seam segments `[start_..end_]`, and `Icp` (`class/reconstruction.cpp:1190-1397`) aligns that segment point-to-point at `iter = 0` (`UsePreCorres` + `P2PConstraint`) before switching to whole-breakline point-to-line (`MakeCor` + `P2LConstraint`) at `iter = 1..49`. Ticket 355 traces `iter = 0` vs `final` across both `Juglet` and `Pot_A` to pinpoint where in that stock two-phase pair stage the `Juglet` loses 10 of its 14 measured joins while `Pot_A` keeps 15 of 15.

## 2026-11-14 - The pair stage loses the rotation at the first solve (ticket 355)

Of the ten joins ticket 348 measured that never reach the bar, none is inside it at the first solve. Three lose the
seam itself in the LCS stage (1-3 overlap fraction 0.000 and 12.98 mm off the seam, 3-5 0.500 / 14.90, 6-7 0.500 /
27.13); the other seven have the seam right -- fraction 1.000, 0.46-3.92 mm -- and still leave the first solve at
6.64-33.29 deg of rotation error (`0 of 14` in the 5 deg / 5 mm bar at `iter = 0`). The remaining forty-nine iterations recover four of the fourteen joins (1-2 14.96 -> 1.93 deg,
1-8 8.07 -> 0.49, 6-9 14.77 -> 2.35, 5-7 6.64 -> 4.71) and leave 5-6 (7.63 -> 13.00) and 6-7 (153.33 -> 165.69)
slightly worse. Pot A, the control, starts at 6 of 15 joins inside the bar and ends at 10 of 15, seating 15 of 15;
Juglet ends at 4 of 14 and seats 2 of 9. The trace changes nothing: 2 of 9 sherds and 1 of 18 edges on every Juglet
arm, 8 of 8 and 15 of 15 on both Pot A arms, 180/180/0 and 124/0 trace rows against the candidate counts.

**Lead decomposition (2026-11-14) — the `iter = 0` loss is an off-axis hinge driven by opposing boundary-bevel normals, NOT a free spin along the pot axis:**

1. **The `UsePreCorres` LCS segment is NOT spin-free on the pot axis.** Unlike point-to-line `MakeCor` at `iter = 1..49`, `iter = 0` (`UsePreCorres` + `MakeSection`) uses a **1-to-1 point-to-point** pairing `[sy-1..ey-1]` <-> `[nx-sx..nx-ex]`. When those exact stock `LCS` dot pairs (`juglet_signed_on_r1.log`) are fit in **2-DoF (`spin` around the aligned `+z` pot axis passing through `(0, 0)` + `z` shift, zero `x/y` shift and zero axis tilt — no oracle, no grid search)**, **10 of the 14 measured Juglet joins land inside `5 deg / 5 mm` at `iter = 0`** (`1-2` `0.26 deg / 0.76 mm`, `1-9` `0.33 / 0.66`, `1-8` `0.38 / 0.73`, `5-7` `1.08 / 0.43`, `6-9` `2.04 / 0.94`, `3-8` `2.23 / 0.57`, `5-8` `2.36 / 0.64`, `1-4` `2.52 / 0.72`, `5-6` `2.95 / 3.31`, `2-9` `3.19 / 0.97`; median `2.13 deg / 0.72 mm`), spanning **all 9 of 9 sherds**. Allowing free `(dx, dy)` translation around the local patch (4-DoF) drops that to `6–7 of 14` because the radius `R` lever arm to the pot axis vanishes over a 7–14 dot patch.
2. **Stock `iter = 0` turns the axis off (`axis = false`, `class/reconstruction.cpp:1258`) and weak `w_a = 0.1` vs `w_n = 3.0` (`:1242`):** Solving an unconstrained 6-DoF rigid fit on a short 1D curve lets Ceres **hinge the sherds off the pot axis** (`tilt0 = 33.25 deg` while `spin0 = -1.69 deg` on `1-4`; `tilt0 = 29.84 deg` on `2-6`; `27.65 deg` on `1-9`; `15.55 deg` on `2-9`; `13.47 deg` on `1-2`; `11.59 deg` on `6-9`; `11.40 deg` on `5-8`; `9.16 deg` on `3-8`). Even at `iter = 49`, `1-9` (`spinF = -1.89 deg, tiltF = 8.62 deg`) and `5-8` (`spinF = +1.71 deg, tiltF = 6.79 deg`) miss the `5 deg` bar purely from residual axis tilt.
3. **Why `CostFuncNorm` (`w_n = 3.0`) hinges the Juglet sherds and why `CountInlier` rejects good poses — opposing `N_z` boundary-bevel bias in `Breakline_0.pcd`:** Evaluated at the **exact ground-truth pose (`0.00 deg, 0.00 mm`)**:
   - On `Pot_A`, 3D breakline normals across mutual nearest neighbours (`< 1.5 mm` real) agree to **`1.0–8.4 deg` (median `3.3 deg`)** across all 15 joins, and **15 of 15** pass `CountInlier` (`min(ang, 180-ang) < 15.0 deg`, `>= 6` inliers).
   - On the `Juglet`, the horizontal `(N_x, N_y)` components agree to **`2.1–12.9 deg` folded median `5.1 deg` (`14 of 14` pass `CountInlier`)**, **but the vertical component `N_z` tilts oppositely into the bevelled fracture edge on the upper vs lower sherd** (`1-4`: `N_z = -0.370` vs `+0.222 -> 34.9 deg`; `2-6`: `-0.271` vs `+0.221 -> 30.5 deg`; `1-9`: `-0.106` vs `+0.242 -> 26.9 deg`; `1-3`: `19.8 deg`; `1-2`: `19.2 deg`; `3-5`: `18.5 deg`; `3-8`: `17.9 deg`; `5-8`: `14.1 deg`; `14`-join folded 3D median `16.0 deg`, unfolded `18.2 deg`). Additionally, on `6-7`, Piece 7 shows opposite-sign radial normals (`+0.36` vs `-0.51`, `175.6 deg` unfolded / `4.4 deg` folded). Because of the `18–35 deg` opposing `N_z` bevel bias: (a) `CostFuncNorm` (`w_n = 3.0`) torques the unconstrained `iter = 0` solve into the `9–33 deg` off-axis hinge, and (b) `CountInlier` (`ANGLE_THRESHOLD = 0.262 rad = 15.0 deg`) fails `< 6` inliers (`score = 11.0`) on **5 of the 14 Juglet joins even at exact ground truth** (`1-4` `0/19`, `2-6` `1/26`, `1-9` `5/17`, `1-3` `5/22`, `3-5` `4/10`), while `14 of 14` pass using horizontal `(N_x, N_y)` normals.

Next: **ticket 357** (`357-what-causes-the-iter0-hinge-and-inlier-rejection.md`) tests the `2x2` factorial (`SFS_AXIS_LOCK_ICP` at `iter = 0..49` x `SFS_XY_BREAK_NORMALS` in `CostFuncNorm` / `RejectOutlier` / `CountInlier`) across all 180 Juglet candidates and the 124 `Pot_A` control candidates, measuring `in_bar_0`, `in_bar_F`, `CountInlier` survival, `pair_basket <= 1.5` survival, and end-to-end seated sherds and edges.

Caveats recorded with ticket 355: only 10 of 15 Pot A joins are inside the bar at their best candidate yet Pot A seats 15 of 15, so the per-candidate bar is not a seat predictor; `Pot_A`'s `1-5, 1-7, 2-5, 3-5, 4-7, 6-7` carry ~120-180 deg axis-sign flips in stored piece axes; and the printed iter-0 transform is in local frames while the breakline files are in tree coordinates, so the 0.06-0.25 mm seating number is the worker's measurement, not the lead's.

## 2026-11-15 - The hinge is the pair stage's fault; the assembly does not move (ticket 357)

Two default-off switches and a 2x2 factorial on Juglet and Pot A. With the candidate axis held through all fifty ICP
iterations (SFS_AXIS_LOCK_ICP=1) the first solve stops hinging: joins inside 5 deg / 5 mm go from 0 of 14 to 10 of 14,
the tilt at iter 0 falls from 1.4-33.2 deg to 0.001-0.012 deg, and 1-2, 1-4, 1-8, 1-9, 2-9, 3-8, 5-6, 5-7, 5-8 and
6-9 land on the seam at the first solve instead of after forty iterations. Pot A holds 8 of 8 sherds and 15 of 15
edges under the axis lock alone. The hinge was real, it was the axis, and locking it fixes the pose.

It does not fix the joins that matter. Of the ten in bar, three are still thrown away by the inlier gate on their own
normals (1-4 with 0 of 19, 1-9 with 4 of 17, 6-9 with 4 of 14 inliers); comparing breakline normals in the plane
orthogonal to the pot axis (SFS_XY_BREAK_NORMALS=1) lifts those to 32, 14 and 14 and turns the ground-truth audit's
five Juglet failures into passes, but it adds no bar entries by itself (1 of 14 at iter 0, 3 of 14 at final) and, in
combination with the axis lock, it breaks the Pot A control: 7 of 8 sherds and 9 of 15 edges, with piece 5 left alone
in a cluster.

Assembly does not move at all: 2 of 9 sherds and 1 of 18 edges on every Juglet arm, the same as stock. Ten of fourteen
joins now start in bar, eight survive the state graph, and the winner still seats two pieces. The pair stage is
therefore no longer what limits this object; the next lever is downstream - the pair_overlap and pair_basket gates
that still drop in-bar joins, and the winner that never carries them.

Caveats recorded with it: the reading axis-lock-alone-recovers-pair-joins-without-xynorm holds through its
parenthetical clause - the xy-normals switch adds nothing to the bar and hurts Pot A in combination - not through the
literal "at least 9 in bar and passing CountInlier" clause, because arm10 has 7 of its 10 in-bar joins passing that
gate (the xy switch raises it to 10 of 10 but costs Pot A). The two repeats agree exactly on each pair's best
candidate; the raw candidate blocks differ in 12 of 51 lines in the sixth digit, which is the known twelve-thread
Ceres drift. Five existing normal comparisons were replaced in place rather than added beside them, each guarded so
the off path is the legacy expression, and the stock arm's candidate block diffs to zero lines against ticket 355.

**Post-run verification (lead, 2026-11-15) — two implementation defects found in ticket 357, a real 3-sherd assembly masked by `CountResult`, and conservator redirection away from axis lock:**

1. **Bug in `SFS_AXIS_LOCK_ICP` (`class/reconstruction.cpp:1423-1523`) that halted `Icp` at `iter == 0`:** Applying the closed-form 2-DoF `(best_R, best_t)` to `L[shard_B - 1]` and rebuilding `COR[0]` before Ceres meant `COR[0]` already sat at its 2-DoF least-squares minimum on entry to `iter = 0`. Ceres took a zero step (`R_i = I, t_i = 0`), so `isConverge(T, 0.1, 2.0)` at `class/reconstruction.cpp:1521` fired immediately at `iter == 0` (`point_to_line == false`) and broke out of `Icp` before `iter = 1..49` (`MakeCor + P2LConstraint`) ever ran (`max|rotF - rot0| = 0.000000 deg` and `max|mmF - mm0| = 0.000000 mm` across all `180` Juglet and `124` `Pot_A` candidates in `arm10` and `arm11`). Combined with `Sfs357NormDot` (`class/reconstruction.h:56`) projecting flat-rim normals on `Pot_A` (`mean_nz = 0.95-0.99`, `||n_perp|| ~ 0.14 > 0.1` guard) into `(x, y)` and amplifying horizontal noise, `CAND338` survivors on `Pot_A` fell from `70` to `33` (`binary_pota_stock.log` vs `binary_pota_both.log`), stranding Piece 5 (`7/8 sherds, 9/15 edges`).
2. **`arm11_both` already seats `{1, 2, 8}` together in `FINAL`, masked by `CountResult`'s world-origin lever arm (`class/data_structure.cpp:715, 1009`):** Recomputing pairwise `PlaceErr` (`<= 5 deg, <= 5 mm` breakline RMS) on `[SFS-336] FINAL` shows that `arm11_both` (`CLUSTER 0 = {1,2,3,4,8}`) seats **both `1-2` (`2.55 deg / 0.70 mm`) and `1-8` (`1.22 deg / 0.45 mm`) together in the same cluster (a 3-sherd sub-assembly `{1, 2, 8}`)** alongside `CLUSTER 2 = {5,7}` (`4.70 deg / 1.18 mm`) — `3` accurate joins total across `5` sherds vs `2` separate pairs (`1-8` and `5-7`) in `stock`. `CountResult` still printed `2/9 sherds, 1/18 edges` because the Juglet sherds sit at median `1,604.9` tree units (`535 mm`) from `(0, 0, 0)` (vs `384.3` tree units on `Pot_A`), so `isSimilarTrans(..., 0.35, 50.0)` rejects any join with rotation error `> 1.79 deg` (`1-2` has world-origin `tnorm = 68.91 > 50.0`; `5-7` has `125.31 > 50.0`).
3. **Why unmodified 6-DoF (`SFS_AXIS_LOCK_ICP=0`) is the right direction, and what actually broke `arm01_xynorm_only` (`2/9 sherds, 1/18 edges`):** Per conservator pushback, stock 6-DoF `Icp` already seats `Pot_A` (`8/8 sherds, 15/15 edges`, even with Piece 7's axis `58.8 deg` off) and already keeps `9/14` true Juglet joins into `CAND338` in `arm01_xynorm_only` (jumping `1-2` `18 -> 69`, `1-8` `70 -> 97`, `2-6 -> 49`, `1-4 -> 53`, `1-9 -> 41`, `5-6` `30 -> 36`, `5-8 -> 34`, `2-9` `26 -> 30`, `6-9` `18 -> 24`, with `3-8` at `51` dropped in `PairwisePruning` to `pair_overlap`). Two 6-DoF bugs broke `arm01_xynorm_only` before `FINAL`:
   - `CostFuncNorm`/`CostFuncFixedNorm` (`class/reconstruction.h:569-580, 609-617`) projected out the vertical residual *after* rotation (`tempn - u*u.dot(tempn)`), removing 3D out-of-plane tilt stiffness, and `Sfs357NormDot`'s `0.1` guard failed to shield `Pot_A` flat rims. Leveling `(n_A, n_B)` orthogonal to axis **before** rotation when both have `||n_perp|| >= 0.5` and keeping the full 3D residual `(R_A * nA_use) - (R_B * nB_use)` restores pitch stiffness in 6-DoF `iter = 0` (`med_tilt0` drops `16.48 -> 12.14 -> 4.99 deg`, `3/14` in bar vs `0/14`).
   - `Icp()` cleared `g_sfs357_xy_axis` on return (`class/reconstruction.cpp:1560`), so `StateManager` (`IcpIncGraphAxis` + `Registration`) reverted to raw bevelled 3D normals at Step 0, slashing `{1, 2}` (`69 -> 24`) and `{2, 6}` (`49 -> 28`) so both were dropped at `CUT 0` (`CUT 0 KEPT 6:78,9:78,3:68,8:40,7:28 DROPPED 10:28,11:28,4:24`) in favour of false join `1-3` (`40`).

Next: **ticket 358** (rewritten 2026-11-15 in `.scratch/pristine-juglet-sfs/issues/358-carry-the-locked-pair-joins-into-assembly.md`) **drops all axis locking (`SFS_AXIS_LOCK_ICP=0`, no `SFS_STATE_LOCK_ICP`)**, audits full pair-to-final attrition on Ticket 357 `arm00_stock` and `arm01_xynorm_only` logs, fixes pre-rotation normal leveling (`0.5` flat-rim guard, full 3D post-rotation residual), extends `(x, y)` normals to `StateManager` (`IcpIncGraphAxis` + both `Registration` overloads behind default-off `SFS_XY_STATE_NORMALS=1`), and runs a 6-arm 6-DoF Slurm test across `Juglet` and `Pot_A` (`8/8 sherds, 15/15 edges` control).

## 2026-11-15 - The 6-DoF normals carry the pair joins into the assembly (ticket 358)

Two default-off switches and no axis lock anywhere. Leveling both breakline normals onto the plane across the pot axis
before rotation - which restores the vertical pitch stiffness the 357 form deleted by projecting the residual after
rotation - takes iter-0 in-bar joins from 0 to 4 of 14 on Juglet. Carrying the same plane into the state stage then
commits two clusters, {1,2,3,4,6,8,9} and {5,7}, seats 9 of the 14 measured joins inside 5 deg and the millimetre bar
(1-2 3.7 deg / 0.8 mm, 1-3 1.6 / 0.7, 1-8 1.2 / 0.5, 1-9 4.1 / 1.2, 2-6 3.3 / 0.6, 2-9 4.7 / 0.7, 3-8 2.5 / 0.6,
5-7 4.7 / 1.2, 6-9 4.1 / 0.5; 1-4 sits in the cluster but 11.8 deg off), and lifts the reported result from 2 of 9
sherds and 1 of 18 edges to 5 of 9 and 4 of 18 on both repeats, while Pot A holds 8 of 8 and 15 of 15 on both of its
arms. The 4.7 deg on 5-7 is the join the plausibility gate refused in tickets 341 and 345.

Caveats recorded with it. The reported 5/9 and 4/18 is CountResult, whose world-frame gate rejects these sherds for
their distance from the origin (about 1600 tree units) rather than for their fit - nine joins are inside the bar in
their own frame and that is the seating number. The join 1-3 is a measured join (ticket 348: 0.0004 mm gap, 320 vertex
pairs within 0.1 mm, median normal dot -0.984, 95 percent back-to-back) that the answer key omits, so the result is 9
of 14 measured joins against 8 of the 13 the key lists; the disagreement is with the reference, not the method. The
state stage's KEEP/CUT scores are not bit-reproducible - a pristine rebuild drifts the same way - so state scores are
not compared across arms; the committed membership and the PlaceErr table are, and both repeated exactly. The last gap
to a whole pot is merging {5,7}, whose bridges die at the pair stage (3-5 and 6-7 dropped as pair_overlap, 5-6 at
13.1 deg after Icp): a merge and overlap problem, not a normals one.

## 2026-11-15 - Correction to the 358 record: which joins the pair stage had in bar (review)

Two sentences in the 358 section over-claim, found in a read-only review of the landed run. (1) 3-5 does reach
CAND338: candidate 118 is in binary_juglet_xy_full_r1.log at 48.26 deg / 44.19 tree units (14.7 mm real) with 24
inliers, and the row the table stores is candidate 120, whose fit failed (0 inliers, score 11), because the picker
keeps the smaller rotation. Only 6-7 never reaches CAND338. (2) Of the nine seated joins only four were inside the
bar at the end of the pair fit - 1-2, 1-8, 5-7, 6-9; 1-3, 1-9, 2-6 and 2-9 came inside during assembly, so the group
stage improves relative poses as well as grouping them. The reported 5 of 9 sherds and 4 of 18 edges is unchanged, and
the world-frame explanation of that count stands.

## 2026-11-15 - The 1-4 mis-pose came in with the edge (ticket 360)

Ticket 360 traced sherd 1's 11.75 deg error to its three moments in both repeats of the winning arm. Piece 1 enters
the largest cluster {1,2,4,6,8,9} at step 5 (build ext 1 / order 2 / graph 2, root 4, 52 inliers) on edge index_ 4
(y 1, x 4, score 0.486727) - not pair-stage candidate 37, which the step discarded when it regenerated its own
candidate list and took the row with more chunk inliers (52 against 49). Its relative pose was already 8.0324 deg /
2.0854 mm out at the phase where the edge was applied, before any ICP; the join step's own IcpIncGraphAxis +
GraphAxisRefinement added 2.40 deg (10.4373 / 2.6290), and the step-6 1-3 merge's ICP added 1.32 (11.7539 /
2.8302), which the final answer keeps. The control 1-8 sits at 1.1250 -> 1.0998 -> 1.2145 deg at the same three
moments, so the instrument is not manufacturing the number. Reading: both-contribute - 69 % of the final error is
the edge's own answer.

The import is where the loss is made: the step had a 5.76 deg 1-4 candidate from the pair stage and took an 8.03 deg
one because its own regenerated list offered more inliers. The refinement contributes the smaller part and is a
separate question. All twelve rotation values were recomputed from the log's own [SFS-336] AXIS matrices and the GT
files (0 mismatches above 0.01 deg); the mm column is the breakline-RMS placement metric and does not carry any
verdict here, because 1-4 is out on rotation at every moment.


## 2026-11-15 - The four bridges: one gate, three fits, and the gate's second number (ticket 359)

Ticket 359 replayed the pair stage's three refusal measures offline - the overlap area against its 50.0 limit, the
axis angle against 0.436 rad, and the pair's own inlier count - at the true relative pose and at the pose the search
found, anchored first on six logged candidate rows (worst area error 0.040 percent, worst axis error 0.000003 rad).
At the true pose 6-7 is the only bridge the gate refuses, on its own measure: overlap area 191.17 against 50.0
(77.39 at the found pose). 3-5, 5-6 and 5-8 pass all three measures and are lost in the fit - 48.26 deg / 44.19 tree
units (14.7 mm), 13.08 deg / 15.67 units (5.22 mm), 8.32 deg / 5.78 units (1.93 mm). Controls behave: 1-8 and 5-7
pass at both poses, and 4-7 - a measured non-join - passes all three too, so "the true pose passes the gates" is a
necessary condition for a join, not a sufficient one. Reading: mixed.

The gate's refusal is real but is not the whole loss, and the second number is what says so: at the true pose 6-7
yields 6 inliers (the pairs the search accepts give 20-99) with its two breaklines a median 21.0 tree units (7.0 mm)
apart, and 3-5 gives 12 at 12.7 mm. Removing the 50.0 limit would therefore not carry 6-7 by itself, and 3-5's 48 deg
answer was made on thin correspondence; the measurement question for those two is how well the pair's curves agree,
not only the threshold. 5-6 (26 inliers) and 5-8 (30) are the two bridges where "the fit is the gap" is unambiguous.
Frame: one tree unit is 3 mm, so every length in units divides by three; the ticket's earlier 76.898 for 6-7 is the
stock arm's row for that candidate index and 77.3877 is the arm under test.

## 2026-11-17 - The breaklines close the 6-7 fit; the search still drops it (ticket 361)

Six-DoF assemble on the GMnr 3x breakline lanes (1.5 mm / 25 deg chord growth, 0.887 merge), with ticket 358's
switches (`SFS_AXIS_LOCK_ICP=0`, `SFS_XY_BREAK_NORMALS=1`, `SFS_XY_STATE_NORMALS=1`): **12 of the 14 measured joins
survive pruning** against 358's 9, and 10 of 14 are in bar at the final pose (5 deg / 5 mm). 6-7, refused by the gate
at 359's true pose, now fits at **3.98 deg / 1.17 mm with 16 inliers** and survives pruning, but the search drops it
(`FATE 7 64 -1 6 7 drop pc_overlap 16`); 5-6 (4.86 deg / 1.43 mm) and 5-8 (3.28 deg / 0.79 mm) commit. Clusters
C0{1,2,3,5,6,8,9} + C1{4,7}: 6 of 9 sherds and 5 of 18 edges, against 2 of 9 and 0 of 18 for the stock 6-DoF arm on
the same lanes. **Nothing seats - 0 of 9, largest cluster 7, Sherd 7 out - and the residue cluster is held by 4-7, a
measured non-join.**

The part that changes the question: on these lanes the *stock* arm already survives 11 and fits 6-7 at 3.12 deg /
1.08 mm, so **the GMnr breaklines fix 6-7's fit, and 359's 166.9 deg refusal was a property of the older lanes**, not
of the pair. What the 358 switches add is the commitment of 5-6 and 5-8 and the jump from 2 of 9 to 6 of 9 sherds.
The open loss is now matching: a join the fit places correctly is discarded at pruning while a false pair holds a
cluster. Pot A holds 8 of 8 and 15 of 15. Frame: the 3x seat bar is 13.788 mm (4.596 mm at 1x); the measured chamfers
are 172.6 mm (xy) and 537-646 mm (stock), so the renders are pictures, not seatings.

## 2026-11-17 - On the wall-paired edgeline the Juglet seats: 9 of 9 (ticket 362)

Ticket 80's default-on edgeline (ticket 79's wall-pair rule) run through the same 358 rig, on lanes built from
its own cells: **one cluster of nine, all 14 adjacent joins inside 5 deg / 5 mm at their committed poses, and
`juglet_def_xy` seats 9 of 9** at the 3x own-place bar (13.788 tree units = 4.596 mm), worst piece **11.093
tree units = 3.70 mm**. Sherd accuracy 9 of 9 and edge accuracy 13 of 18, against 8 of 9 / 8 of 18 for the stock
6-DoF arm on the same lanes (which seats 5 of 9, 17.748 tree units worst). Pot A holds 8 of 8 and 15 of 15, so
the arm is not void. This is the first seated Juglet in the project: ticket 67's best was 0 of 9 at 458-475 mm.

**6-7 is kept and committed** (`FATE 7 87 0 6 7 keep accept 852`, 32 inliers, 1.656 deg / 0.291 mm). Nothing in
the search was changed. The reason nothing had to be is measurable: at the GT pose 6-7's pair-stage overlap is
**14.35** on these lanes against **52.28** on ticket 361's GMnr bundle and **191.17** on ticket 355's - same
engine, same 50 bar, three segmentations, and only the older two refuse the true placement. The overlap test was
counting the spurious off-lip flap; the flap is gone, so the test passes the pair it should pass.

Two things to carry forward. `score58.py`'s 4.596 bar is a 1x bar and reports 0 of 9 on a 3x bundle; the bar has
to be derived from `SFS_TREE_UNITS_PER_MM`, not typed. And the non-adjacent pair 4-6 (62.9 deg / 14.3 mm at its
fit) still rides inside the single correct cluster: a wrong join inside a right assembly is invisible to
chamfer, so the join table is what certifies an assembly, not the picture.

**Amended by the lead, same day:** the 9 of 9 is a seating only when its bar is said out loud. The staged mesh is
the 3x file, so the own-place bar is 13.788 tree units, not score58's hard-coded 4.596; and the join table, not
the chamfer, is what certifies the assembly while a false 4-6 join can ride inside the correct cluster.

Reading: `fixed-edgeline-keeps-6-7`. What it means for S1: with the breakline fixed, an assembly the search can
certify exists - the open problem is no longer whether the pipeline can put the juglet back together, but which
joins it puts in besides the true ones.

Addendum, same day: the edgeline alone already seats five of the nine. With both 358 switches off, ticket 80's
lanes give one cluster of nine and 14 of 14 joins in bar and **5 of 9 seated** (worst 17.748 tree units = 5.92 mm);
the switches take that to 9 of 9 at 3.70 mm worst. So the line is what makes the vessel assemble and the rig is
what makes it sit - and the remaining false pair (4-6, 62.9 deg / 14.3 mm at its fit) is a matching question, not
a breakline one.



## 2026-11-18 - On our own shipped axes (axes323), def_3x seats 0 of 9 (ticket 363)

Ticket 80's clean `def_3x` `(S0, S1)` walls run through shipped `axes323` (`sfs_axes` tree `6a67364b` from `d272313`, Pot A control `8/8` byte-identical by `cmp`) produce `8/9` rank-0 directions within `10 deg` (`0.95–9.60 deg`, only Piece 2 at `15.73 deg`; `cand0 = best` for all 9) — **yet large axis-line pivot offsets (`d_perp = 22.9–134.6 mm` real / `68.7–403.9` tree units on Pieces 2–8, vs `1.8–7.2 mm` on Pot A P01–P03)**. Fed into ticket 362's unmodified rig (`runs/20261118-0247_4beb57f_t363-own-axes`, jobs `35293174`, `35294194_0..5` all `COMPLETED 0:0`), both `juglet_own_stock` and `juglet_own_xy` repeats collapse to **0/9 seated** (`574.7–653.9` tree = `191.6–218.0 mm` real on `own_xy`; `1649–1846` tree = `549.8–615.3 mm` on stock) and **1/18** true joins (`1-8`, `1.60 deg`), while `juglet_oracle_xy` seats `9/9` (`2.06–3.70 mm`) and `pota_def_xy` seats `8/8, 15/15` (`1.80 mm`). Piece 7 gets `0/14` upright (`< 20 deg`) pairwise hypotheses across all partners (`(6,7)` enters upside-down at `174.7 deg`), and even inside the `{1,2,3,5,6,8}` cluster only `1-8` is right while `3-5` sits at `52.4 deg` and `1-2` at `127.2 deg`. **Reading: `own-axes-seat-zero-on-def3x`.** What it means for S1: the blocker is our axis line (`d_perp` + tilt on small-arc/curved Juglet sherds), not the assembler or `def_3x` breaklines.

## 2026-11-18 - Direction is the assembly gate (`2/9 -> 8/9` cluster), pivot is needed to seat (`< 4.6 mm`), and `sfs_axes` loses both upstream (ticket 364)

Ablating `pos` (`d_perp`) vs `dir` (`deg`) on the `t363` `def_3x` rig (`runs/20261118-1039_da58830_t364-tilt-vs-pivot`, jobs `35310856`, `35310987_0..3,5`, `35311950`, `35312197_6` `COMPLETED 0:0`; `35310987_4` `8/9` at `4.76 mm`, repeat `9/9` at `3.70 mm`, Pot A `8/8, 15/15`) cleanly separates the two errors:
- **`juglet_own_dir_gt_pos`** (`d_perp = 0`, keeping `0.95°–15.73°` tilts): total collapse (`2/9` sherd, `1/36` directed joins, `0/7` upright Piece 7 candidates, `446 mm` chamfer) — removing the pivot offset alone fails completely.
- **`juglet_gt_dir_own_pos`** (`deg = 0`, keeping `6.9–134.6 mm` pivots): **8 of 9 pieces (`{1..8}`) assemble into a recognizable `2.09°–4.30°` Juglet** (`6/9` sherd accuracy, `7/36` directed joins, `6/13` breakline joins `<= 5° / 5 mm` incl. `6-7` `3.0°/0.6 mm`, `7.4–29.1 mm` origin slack inside `{1..8}`), while `Piece 9` stays detached (`9|1..8`) and pulls `score58`'s 9-piece Kabsch to `138–167 mm` (`0/9` seated at `4.596 mm`). **Reading: `both-pivot-and-tilt-required`.**

Tracing `sfs_axes` (`tree 6a67364b`, `basins364.json`, 10 seeds × 1000 draws) locates where both errors enter before `keep_lines`: PotSAC's `top60` raw basins already contain a `2.66°–9.09°` direction on all 9 Juglet sherds, **yet `1139` Juglet basins yield only `1` satisfying both `<= 5°` and `<= 15 mm` (`0` in `top60`, vs `5/5` in Pot A's `top60`)**. `refine_axis_normalized` collapses all 60 basins on `P2` (`4.03° -> >= 15.44°`) and `P9` (`7.53° -> >= 22.73°`), `refine_axis_profile` worsens `P4` (`2.73°/16.5 mm -> 7.40°/50.8 mm`), and `rank_by_profile` outranks much better refined candidates (`P5 0.69°`, `P6 1.84°`, `P3 4.40°`, `P2 9.51°`).

## 2026-11-18 - Preserving `k=0` (`a1_k0`/`a2_pool`) seats `7/8` non-base sherds and clean-wall pair pooling (`b2_pair`) seats `8/8`; `SFS_AXIS_SIGN=1` excluded Piece 7 (ticket 365)

Testing both Option (a) (`a1_k0` 2-DoF tilt polish + pre-polish `cost_in` order + `5 deg` dedup; `a2_pool` `tau=1.35, kmax=4`) and Option (b) (`b1_blind` 2-pass blind pooling; `b2_pair` clean `(S0, S1)` 2-piece pooling at `+3 deg / +1 mm`) in `runs/20261118-0242_308c771_t365-k0-preservation-postjoin`:
- **Clean-wall pair axes (`b2_pair`):** `8/9` exact pairs inside `5 deg`, `7/9` inside `(5 deg, 20 mm)` (`1-8 1.00°`, `1-2 1.12°`, `3-5 1.18°`, `1-4 1.93°`, `2-5 1.71°`, `1-9 0.75°`, `6-7 4.12°`, `7-9 3.02°`, `2-9 6.40°`); after `+3 deg / +1 mm` centroid perturbation, all `9/9` carried piece axes sit within `1.35°–4.71°` and `6/9` inside `20 mm` `d_perp`.
- **What `score58` missed vs cluster-only Kabsch on `[1, 2, 3, 4, 5, 6, 8, 9]`:** Every experimental arm left **only Piece 7** detached upside-down (`174.4°–178.8°`), skewing 9-piece `score58` (`613–947` tree = `204–315 mm`). Aligning `[1, 2, 3, 4, 5, 6, 8, 9]` without `[7]` proves: `juglet_a1_k0_r2` seats **7/8** (`2.17–4.56 mm` real; `P2 4.95 mm`), `juglet_a2_pool_r1/r2` seat **7/8** (`2.27–4.58 mm`), and `juglet_b2_pair_r1` seats **8/8** at **`1.93–4.28 mm` real (`5.79–12.85` tree — tighter than `juglet_oracle_xy`'s `13.61` tree max)**! `juglet_b1_blind` degraded `a1_k0` (`0/8` seated) by pooling unverified Pass-1 partners (`6<->5`, `2<->6`, `7<->5`). Positive controls held (`juglet_oracle_xy` `9/9` at `13.61` tree; `pota_a1_k0` `8/8, 15/15` at `2.01 mm`).
- **Why Piece 7 (`[7]`) never joined (lead-verified):** `Dataset/Juglet_large/Juglet_Piece_7_Axis.xyz` has `dir_z = -0.670` (scanned >90° relative to `1..6, 8, 9`), whereas blind axis extraction canonicalizes `dir_z >= 0` (`+0.652` in `b2_pair`, `1.73° / 11.75 mm` undirected, opposite vector sign). With `SFS_AXIS_SIGN=1` (`[SFS-335] signed=1`), `pair_axis` rejected **every** upright Piece 7 hypothesis (`(5,7)` `inl=81` `raw=176.6°/folded=3.4°`, `(6,7)` `raw=168.2°`, `(7,9)` `raw=179.1°`) and left `0` upright Piece 7 `CAND338` candidates. Amended finding for S1: single-sherd `a1_k0`/`a2_pool` already assembles `7/8` of `{1..6,8,9}` and clean pair pooling seats `8/8`, with `[7]` gated out only by `SFS_AXIS_SIGN=1` against `dir_z >= 0` canonicalization.

## 2026-11-19 - Blind base-sherd axis sign orientation (`orient_base_axis`) restores all 9/9 Juglet sherds into one closed assembly; `a366_4dof` widening rejected (ticket 366)

Following a paper-vs-code audit of base and rim handling (`papers/text/sfspp-2502.13986v1.md` §IV-B2/§IV-C1/Table III, `AxisExtraction/check_base_and_rim.m`, `sfs_axes/normalized.py`, `class/reconstruction.cpp`), Ticket 366 (`runs/20261118-1435_78581fb_t366-base-sign-4dof`) tested blind base-pole-to-wall axis orientation (`orient_base_axis`) and centroid-height offset measurement (`d_perp_sherd_mm`) alongside a 4-DoF + `K=3` candidate-retention variant (`a366_4dof`):
- **Blind base detection & sign fix (`orient_base_axis`, Claim 1 method gap fixed):** `normalized.is_base_fragment(C, vt)` (`r_min < 25 mm`, `>= 10%` of points with `|n·a| > 0.85`) fires **only** on `Juglet_Piece_7` (`r_min = 0.07 mm`, `28.8%` flat base normals, `z_wall - z_pole = -26.31` to `-27.73`) across all 9 Juglet sherds, and **only** on `Pot_A_01` (`deg_signed = 0.34°`, already upright) across Pot A. Flipping Piece 7's vector base-to-rim turns Piece 7 from `175.04°` to `4.96°` (`a1_base`), `175.56°` to `4.44°` (`a366_4dof`), and `178.98°` (`178.27°` rank-0) to `1.02°`/`1.73°` (`b2_base`), eliminating `SignAxisGuard`'s `4/18 (22.22%) <= 25%` blind spot (`0/18 = 0.00%` pairs `> 90°` across all three Juglet sets).
- **Lever-arm artifact in origin `d_perp` (Claim 2 broken measurement):** Juglet scans sit ~1,600 tree units (`533 mm`) from `(0, 0, 0)`, inflating origin-measured `d_perp_origin_mm` to `4–166 mm`. Measured at each sherd's centroid height (`d_perp_sherd_mm`, the pivot frame used by `AxisAlignment`), `a1_base` single-sherd axes already pass within `0.13–1.26 mm` on `8/9` pieces (`4.55 mm` on P9), and pair-pooled `b2_base` passes within `0.35–1.02 mm` on all 9 pieces.
- **Assembly outcome (`juglet_a1_base` adopts 9/9 upright closed Juglet & holds Pot A):**
  - `juglet_a1_base_r1` & `r2` (`a1_k0` + `orient_base_axis`): `CountResult` **`9/9` sherds (`11/18` undirected / `23/36` directed joins, including `4/4` GT edges touching Piece 7: `3-7, 6-7, 7-8, 7-9` at `1.74°–3.02°`, `27/100` upright `CAND336`, `8/17` upright `CAND338`)**. Under `score58`'s 9-piece origin-gauge Kabsch (`4.596 mm = 13.788` tree bar), **`8/9` pieces seat `<= 4.596 mm` (`2.326–4.144 mm`, Base Piece 7 at `2.450 mm` = `7.349` tree down from `186.189 mm` in `b2_pair_r1`), with Piece 2 at `4.653 mm` (`13.960` tree — `0.057 mm` above bar in `r2`; `4.875 mm` in `r1`)**; under vertex-centroid Kabsch (`cluster366.py`), **all `9/9` sherds seat in one closed cluster at `0.482 mm` max (`0.467 mm` in `r1`, matching `juglet_oracle_xy`'s `0.467 mm`)**. Its Pot A control (`pota_a1_k0rig`) passes **`8/8` sherds, `15/15` edges (`30/36` directed), `8/8` seated at `2.011 mm` max (`0.425 mm` vertex cluster)**.
  - `juglet_b2_base_r1` & `r2`: restores Base Piece 7 into the assembly (`4/4` P7 joins, `9/9` vertex cluster `0.461–0.463 mm`, `6/9` `score58` `<= 4.596 mm`, max `6.043–6.114 mm`), leaving P4 outside `CountResult` (`8/9` sherds, `8/18` edges).
  - `juglet_a366_4dof` & `pota_a366_4dof` (**rejected**): full 4-DoF polish improved Juglet rank-0 directions (`P5 6.00° -> 1.06°`, `P8 5.99° -> 3.80°`, `P4 10.32° -> 7.40°`, `P2 20.43° -> 15.73°`, `P7 4.96° -> 4.44°`), **but** keeping pre-polish candidates and widening `keep_lines` to `(tau=1.25, K=3)` lowered Juglet edge count (`11/18 -> 8/18`) and caused Pot A Piece 07 to pick an unpolished `68.38°` line (`deg_signed = 111.62°`), breaking Pot A (`7/8` sherds, `13/15` edges, Piece 8 detached, `75.14–100.96 mm` on both repeats).
- **Verdict for S1:** Blind base orientation (`orient_base_axis`) + single-sherd `a1_k0` axes (`a1_base`) puts all `9/9` Juglet sherds together upright (`11/18` edges, `8/9 <= 4.596 mm` and `9/9 <= 4.653 mm` on origin gauge, `9/9` at `0.482 mm` on vertex gauge) with Pot A healthy (`8/8, 15/15`). Next candidate improvement (keeping 4-DoF polish tilt gains without Pot A `K=3` regression) is deferred until clean promotion/ablation of `a1_base`.

## 2026-11-19 - Clean 4-DoF `c_in` seats 9/9 Juglet at 4.06 mm (0.47 mm vertex gauge) and `shipped_base` seats 9/9 at 0.47 mm; pre-polish `c_in` vs. post-polish `c_ref` trade-off isolated on Pot A Piece 08 (ticket 367)

Isolating three unconfounded `orient_base_axis` variants (none using `L_pre` injection or `tau=1.25`) in `runs/20261119-1023_273b8f9_t367-shipped-vs-4dof-cin` establishes both what `orient_base_axis` buys on shipped `sfs_axes` (`tree 6a67364b`) and why clean 4-DoF `c_in` succeeds on Juglet yet breaks Pot A Piece 08:
- **`shipped_base` (untouched `sfs_axes` `tree 6a67364b` + `orient_base_axis` only):** Flips **only Piece 7** on Juglet (`8/9` files byte-identical to `t363`, Piece 7 `4.69°` negated to base->rim, `SignAxisGuard` `0/18`) and changes **0/8 lines** on Pot A (`axes_pota_shipped_base` is **`8/8` byte-identical by `cmp` to `axes323`**). On Pot A, `pota_shipped_base_r1/r2` pass **`8/8` sherds, `15/15` edges (`30/36` directed), `8/8` seated at `2.02–2.06 mm`**. On Juglet, `juglet_shipped_base_r1/r2` bring all `9/9` sherds into one closed upright vessel (`0.466–0.476 mm` vertex-centroid Kabsch, `18/18` centroid-gauge joins `<= 1.73° / 0.86 mm` in `r1`, `4/4` Piece 7 joins), while leaving `6/9` (`5.56 mm` max) / `5/9` (`6.07 mm`) inside `4.596 mm` on `score58` origin gauge (`8/18` stock edges).
- **`a1_4dof_cin` & `a1_4dof_k1` (`c_in` pre-polish order + 4-DoF polish + `keep_lines(c_in, 1.2, 3)`, NO `L_pre`):** Achieves the **tightest Juglet assembly to date**: `juglet_a1_4dof_cin_r1` seats **`9/9 <= 4.596 mm` even on `score58`'s origin-translation gauge (`4.060 mm` max = `12.18` tree — tighter than `juglet_oracle_xy`'s `4.191 mm`), `9/9` at `0.468 mm` vertex cluster, `12/18` stock edges (`25/36` directed), and `18/18` centroid edges (`<= 2.13° / 0.91 mm`)** (`r2`: `8/9` origin gauge at `4.774 mm`, `0.483 mm` vertex cluster, `11/18` edges; `a1_4dof_k1` `K=1` slice hits `9/9` at `4.149 mm` / `0.479 mm` in `r1`, `8/9` at `4.773 mm` / `0.472 mm` in `r2`).
- **Why Pot A rejected `a1_4dof_cin` (`Reading: blocked: control-or-input-failed`):** As designed, `axes_pota_a1_4dof_cin` keeps `K = [1, 1, 1, 1, 1, 1, 1, 1]` (`0.34°–4.07°` on `P01..05`), **yet** `pota_a1_4dof_cin_r1/r2` drop `Pot_A_Piece_08` on both repeats (`7/8` sherds, `13/15` edges, `75.38–75.50 mm` origin gauge vs `8/8, 15/15` at `2.011 mm` for `pota_a1_k0rig_r1/r2` in the same rig). Mechanically, on Pot A `P08` (`18.98°`) and `P06` (`19.90°`), the polisher moved `c_in`'s rank-0 basin by only `0.299°` and `2.105°`; **pre-polish `c_in` order picked the wrong basin**, whereas shipped post-polish `c_ref` re-sort put `P08`'s `1.20°` (`0.831°` shift) and `P06`'s `11.35°` (`0.926°` shift) first. On Juglet `P3/P5/P6`, conversely, false `c_ref` winners were basins where 4-DoF polish walked `10.1°–40.8°` in direction. Next axis step: test a **direction-shift guard (`polish_shift_deg <= 6°`) or bounded-tilt 4-DoF** under post-polish `c_ref` ranking + `orient_base_axis` (`a1_k0 + orient_base_axis` remains the only candidate configuration passing both `8/8, 15/15` Pot A and `9/9` closed Juglet so far).
