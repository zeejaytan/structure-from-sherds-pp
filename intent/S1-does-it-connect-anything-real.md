# S1 — After the unit fixes, does it find any real joins?

**Status:** open — the zero result predates the fixes · **Blocked by:** none

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

## Done when

- [ ] Tray-000 re-run end to end **after** both fixes, from the fixed preprocessing output
- [ ] Connection count reported, and if it is still zero, a check that the search radius is
      now in the same units as the point cloud — verified by printing both, not assumed
- [ ] At least one proposed join **rendered**: the two sherds, at a view that shows whether
      the break faces meet. A connection count is not evidence that a connection is real
- [ ] The 0% NURBS-dataset result revisited in the same light: `Surface_F.pcd` files were
      missing and surfaces were 2–6× sparser than the working sample, so that arm was also
      scoring a broken input

## Gate

If it still finds nothing with correct units and complete inputs, **that** is a capability
finding and can be reported as one. Until then it is an unmeasured system.

## Source

`ASSEMBLY_FAILURE_ROOT_CAUSE.md`, `ROOT_CAUSE_DIAGNOSIS.md` (2025-11-04),
`BOUNDARY_RADIUS_FIX.md`, `PREPROCESSING_SUCCESS_SUMMARY.md` (2025-11-05),
`NURBS_FIX_RESULTS.md`, `STEP_BY_STEP_COMPARISON.md`.
