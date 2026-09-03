# What we are trying to establish with Structure-from-Sherds++

**SfS++ is the classical, non-learned comparison partner** — it reassembles axially
symmetric pots by matching fracture surfaces and profiles, with no trained prior. That
makes it the most interesting baseline here and also the one most likely to be ruled out
on scope grounds.

Two questions, deliberately. This is a comparison partner, not a project we are
developing, and a thin project honestly gets two questions rather than a forced seven.

This folder is **state, not a log**. The research-phase writeups at the repo root
(`ROOT_CAUSE_DIAGNOSIS.md`, `NURBS_FIX_RESULTS.md`, and the rest) are the log.

Prefix **`S`**, permanent. Numbers are never reused. **S3 is next.**

| # | Question | Status | Blocked by |
|---|---|---|---|
| [S1](S1-does-it-connect-anything-real.md) | After the unit-conversion fixes, does it find any real joins? | open — the zero result predates the fixes | none |
| [S2](S2-axial-symmetry-and-our-material.md) | Is the axial-symmetry assumption compatible with Rabati material? | open | none |

## What is established

| Claim | Weight | Source |
|---|---|---|
| The legacy SfS++ system found **zero pairwise connections** between any Tray-000 pieces. | 1 tray | `ASSEMBLY_FAILURE_ROOT_CAUSE.md` |
| The raw GitHub PCD files are in **micrometres** while the system expects millimetres — a **1,000,000×** coordinate mismatch. | reproduced | `ROOT_CAUSE_DIAGNOSIS.md` (2025-11-04) |
| A second, independent unit bug: the **boundary-detection radius** was in the wrong unit. | reproduced | `BOUNDARY_RADIUS_FIX.md` (2025-11-05) |
| Preprocessing succeeded on all 8 pieces once both bugs were fixed. | 8 pieces | `PREPROCESSING_SUCCESS_SUMMARY.md` (2025-11-05) |
| The author's own sample dataset assembles at 100%; the "fixed" NURBS dataset gave 0%, with **missing `Surface_F.pcd` files** and surfaces 2–6× sparser. | 2 datasets | `NURBS_FIX_RESULTS.md`, `STEP_BY_STEP_COMPARISON.md` |

## The pattern worth noticing

**Every failure recorded here so far turned out to be a broken measurement, not a broken
method** — two unit conversions and a missing input file. That is failure mode 2 three
times running. Before concluding anything about SfS++'s capability on our material, assume
the ruler is wrong until it has been shown right.

## Naming warning

Despite the folder name, `sfspreproc-docker` on Spartan is the **assembly** code, not
preprocessing. See [`../AGENTS.md`](../AGENTS.md).

## Related

`U7` (what is actually being compared) in [`../../intent/`](../../intent/).
