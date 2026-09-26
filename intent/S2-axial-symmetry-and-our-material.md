# S2 — Is the axial-symmetry assumption compatible with our material?

**Status:** open · **Blocked by:** none · **Effort:** a measurement on existing scans

## Why it matters

SfS++ reassembles **axially symmetric** pots: it uses the fact that a wheel-thrown vessel
is a surface of revolution to constrain where a sherd can sit. That assumption is what
makes it work without a trained prior, and it is also its scope limit.

Rabati's Kura-Araxes material is **handmade**, not wheel-thrown. A handmade jar is roughly
symmetric in intent and measurably not symmetric in fact. If the departure is larger than
the tolerance the method allows, SfS++ is not applicable to that part of the assemblage —
and that is a scope statement, not a failure, and must not be reported as one.

The Early Kurgan material may be a different answer from the Kura-Araxes material. Do not
average them.

## Done when
- [ ] Departure from a surface of revolution measured on real scanned sherds, in
      **millimetres** of radial deviation at a given height — a physical number, not a
      fitted score
- [ ] Compared against the tolerance SfS++ actually assumes, read from the paper and the
      code at the pinned version rather than inferred
- [ ] Reported **separately** for wheel-thrown and handmade material
- [ ] A profile drawn — the fitted profile against the measured section — because a
      goodness-of-fit number cannot show *where* the departure is, and where matters

## Gate

If handmade Caucasus material sits outside the assumption, then SfS++ is a baseline for the
wheel-thrown comparison set only. Say that in the comparison table rather than showing it
failing on material it was never meant for — that would be a wrong claim about the method,
which is exactly the distinction this workspace exists to keep straight.

## Related finding (not this question's measure)

2026-09-21, from the Juglet assembly (ticket 03, S1): per-fragment PotSAC
axes, mapped into the assembled frame with ground truth, scatter 51 deg
mean pairwise across the 9 sherds -- and all downstream matching runs in
per-fragment axis frames (cylindrical profiles after AxisAlignment). This
is consistent with the scope concern above (handmade fragments defeat
vessel-axis estimation) but it is NOT the millimetre radial-deviation
measure this question asks for; none of the boxes below are ticked by it.

2026-09-21, ticket 04 counterfactual: all 9 sherds given ONE consistent
vessel-frame axis still yield 0 joins. So frames alone do not explain the
failure -- matcher features and merge also do not fire on this material.
Sharpens, not settles, the scope reading above.
2026-09-21, ticket 04 trace closed: the merge-stage gates are named --
RegistrationPruning drops edges over 0.436 rad post-registration axis
angle, RemoveEdgeUsingPCInlier (overlap/CountPCInlier 7.0,1.5), SortRoot
excludes zero-inlier sherds from roots. All three are axial-assumption
machinery (per-fragment axis frames, vessel-profile checks); each is a
concrete place where handmade ware exits the pipeline.
2026-09-24, ticket 05 verdict: the joint refinement itself diverges
(e5-scale placements, converging nowhere) -- thresholds exonerated,
since re-matching finds nothing to be strict about. The divergence is
the seat; ticket 06 takes the fix. S2's scope reading unchanged, mechanism
deepened by one level.
2026-09-24, ticket 06 characterization: divergence sits UPSTREAM of the
joint step, in per-edge Ceres Registration (initial merge placements
already e5, while legacy ICP on the same pair is sane). S2 unchanged.
2026-09-25, ticket 07 (real curves): matcher floods rather than starves
(all 18 mates with inliers, most false pairs too) yet joins still 0/18.
Handmade curves match anything against anything: no starvation, no
discrimination. Scope reading holds; failure moved downstream to merge.
2026-09-26, ticket 06 verdict -- **this narrows the scope boundary to a
measurable line, and the line is NOT axial symmetry.** The axial term is
exonerated three times over (w_a 1.0 / 0.1 / 0, all identical), and the
PotSAC axis scatter (51 deg) turns out not to be what stops the Juglet.
The binding constraint is the **breakline gate**: a point on one sherd's
breakline within 2 mm of a point on its neighbour's with agreeing surface
normals. Pot_A (thin, wheel-thrown) passes 15/15; the Juglet passes 0/18.
Proven dynamically too: ground-truth placement of a true pair is REJECTED
by the real binary (0 inliers), while the same harness accepts Pot_A's.

2026-09-26, preprocessing ticket 01 -- **and the cause is our own
extraction, measured.** The extractor traces ONE wall face per sherd,
arbitrarily (inner 1, 5, 9; outer 2, 3, 4, 6, 7, 8), putting 10/18 true
mates inner-vs-outer; and only 1/8 same-face mates is within 2 mm at
ground truth (others 5-30 mm off) because the segments do not span the
seam. **This narrows the scope statement further and in a useful
direction:** the failure is not "handmade material defeats axial
symmetry" but "our breakline extraction on thick-walled ware produces
fragments the gate cannot use" -- a fixable preprocessing defect, not an
inherent property of SfS++'s symmetry assumption. Note the distinction
this workspace exists to keep: a *method* claim and an *extraction*
claim are different, and this is the latter. (And the earlier "normals
67-129 deg opposed" reading is withdrawn as a headline: it came from an
opposite-face pair; on same-face pairs normals agree at 0.92-1.00.)

**So the honest scope statement is narrower and more useful than "not
axially symmetric":** SfS++'s join gate assumes mating breaklines
coincide within ~2 mm **on the same face of the wall**. That is
satisfied by thin wheel-thrown ware and violated by thick handmade ware,
independent of axial symmetry. Rabati material is handmade and
thick-walled, which puts it on the wrong side of that line -- but the
reason is breakline geometry, and it is fixable in preprocessing, not
inherent to the method's symmetry assumption. **None of the boxes in
"Done when" are ticked by this**: it is still not the millimetre
radial-deviation measure this question asks for, and it must not be
reported as one.

**Standing caveat (unchanged, now load-bearing):** one object, one
failure mode. A thick wall on an otherwise clean break is untested, and
this says nothing about the Early Kurgan material.

## Source
Upstream: Yoo and Liu et al., *Structure-From-Sherds++*; [`../README.md`](../README.md).
Workspace: `../../intent/U7-what-is-being-compared.md`.
