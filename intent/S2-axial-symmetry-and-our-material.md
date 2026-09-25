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

## Source
Upstream: Yoo and Liu et al., *Structure-From-Sherds++*; [`../README.md`](../README.md).
Workspace: `../../intent/U7-what-is-being-compared.md`.
