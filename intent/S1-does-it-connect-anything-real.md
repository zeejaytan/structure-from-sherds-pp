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
surface normals agreeing. The Juglet's wall is ~1.8 mm, so extraction
traces the *inner* face on one sherd and the *outer* face on its
neighbour — 1.7 mm apart with normals 67–129° opposed. Two different
surfaces are being compared as though they were the same. Pot_A's thin
wheel-thrown breaks put both traces on one face, 0.09–1.30 mm apart,
with normals genuinely agreeing.

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
