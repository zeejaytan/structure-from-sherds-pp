# 01: Descriptor smoothing — Lanczos vs Savitzky-Golay, measured

**Answers:** S1

**Blocked by:** nothing — laptop measurement on code already built

**Status:** ready-for-agent

**Needs-eye:** none — numeric comparison of two smoothers, no geometry claim.

## Why this ticket exists

Paper (Fig. 7 caption): edge-line descriptors from finite differences,
smoothed with Savitzky-Golay, then Gaussian (kernel width 7, σ=2.0);
thickness unfiltered. Ours (`class/filter.cpp:161-260`): `LanczosDiffLow`
diffs, one `Gaussian(2, …)` pass, `GetThickness` from `sur_out_` with -1.0
for far/wrong-side points. No Savitzky-Golay anywhere repo-wide. Per-point
rows Dist/Height/Theta (+diffs) and Thickness exist in both — so the
question is narrowly whether the smoother swap changes descriptor values
enough to change matches, not whether descriptors exist.

## Research spike (do first — this ticket may END here)

1. Read `CalculateFeatureAxisless` (`:161-219`), `LanczosDiffLow`
   (`filter.h:35-37`), `Gaussian` (`filter.cpp:116-158`, kernel level 10),
   and the call at `:203-205` end to end. Write down the actual applied
   smoothing in plain terms (window, weights, passes).
2. Implement Savitzky-Golay with the paper's parameters as a laptop script
   over exported per-point Dist/Height/Theta series (or as a test-only code
   path — whichever is smaller, but NOT in the matching path yet).
3. Compare the two descriptor sets point by point on Pot_A breaklines:
   distribution of differences, and specifically whether any pair's match
   inliers would change. If the descriptors agree to within matching
   tolerance everywhere that matters, close with the table — the smoother
   is an implementation choice with no behavioral difference, and that is
   worth knowing rather than assuming in either direction.

## What to build (only if the spike shows a behavioral difference)

- Swap or add the paper's smoother behind the same interface, keeping
  parameter values visible and logged (the current call buries them).
- Thickness handling stays as-is unless the spike implicates it (the paper
  also leaves thickness unfiltered — that part already matches).

## Acceptance criteria

- [ ] Spike: both smoothers on the same series, difference distribution,
      and the verdict on whether any match changes
- [ ] Lane declared: gate (which pairs move) or close-with-table
- [ ] If implemented: Pot_A per-pair from the current baseline, Juglet
      honest-10 unchanged-or-better, no other-pair regressions
- [ ] Authors' sample rides along wherever "correct" is needed
