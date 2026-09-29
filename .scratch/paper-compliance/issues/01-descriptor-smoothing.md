# 01: Descriptor smoothing — Lanczos vs Savitzky-Golay, measured

**Answers:** S1

**Blocked by:** nothing — laptop measurement on code already built

**Status:** resolved 2026-09-29 — cosmetic for joins (scores move, pairs don't)

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

## CONTROL RUNS 2026-09-29 (authors' sample, Lanczos path)

Full assembly runs end to end: 163 feature matches → 124 surviving, 9 pairs
with scores (1-2: 8, 1-3: 10, 1-4: 3, 1-5: 6, 1-7: 5, 2-4: 6, 2-5: 12,
3-5: 19, 4-7: 12). Final: 6 pieces placed, accuracy 0/6, 0/9 edges — every
merge fails plausibility downstream. So pairwise matching discriminates on
this data (the A/B has signal to move), while final assembly recovers
nothing with defaults (a separate question about plausibility thresholds,
not this ticket).

Control rerun: IDENTICAL values on all 9 pairs, zero SFS-SG lines. The
matcher is deterministic — any movement in the SG arm is smoother-caused,
not noise.

## SG ARM RUN 2026-09-29 (authors' sample, SFS-SMOOTHER=sg, 13 marker lines)

Same 9 pairs survive. Values move ±1–2:

| pair | control | SG |
|---|---|---|
| 1-2 | 8 | 6 |
| 1-3 | 10 | 11 |
| 1-4 | 3 | 4 |
| 1-5 | 6 | 5 |
| 1-7 | 5 | 5 |
| 2-4 | 6 | 8 |
| 2-5 | 12 | 11 |
| 3-5 | 19 | 20 |
| 4-7 | 12 | 11 |

No pair gained or lost. The control rerun (above) is byte-identical in
values, so the ±1–2 is smoother-caused, not noise.

Final assembly, SG arm: 6 pieces placed, score 36.000 (control: 0.000),
accuracy still 0/6, 0/9 edges. So the smoother moves internal scores but
changes no join outcome on either arm.

VERDICT: the smoother is load-bearing for scores and inert for joins, on
default config where neither arm assembles correctly. Per the lane rules
this closes WITHOUT implementing: no pair moves, and the remaining
question (why defaults assemble 0/6 on the authors' sample) belongs to
plausibility thresholds, not smoothing. If a future tuned-config run shows
pairs on the margin, reopen with those numbers — do not re-argue the
2.6–20% descriptor gap, which is settled above.

Full assembly runs end to end: 163 feature matches → 124 surviving, 9 pairs
with scores (1-2: 8, 1-3: 10, 1-4: 3, 1-5: 6, 1-7: 5, 2-4: 6, 2-5: 12,
3-5: 19, 4-7: 12). Final: 6 pieces placed, accuracy 0/6, 0/9 edges — every
merge fails plausibility downstream. So pairwise matching discriminates on
this data (the A/B has signal to move), while final assembly recovers
nothing with defaults (a separate question about plausibility thresholds,
not this ticket).

## BLOCKING FIND FIXED FIRST: host-absolute dataset paths (d2445d8)

No arm of this comparison could run until this was fixed: d2445d8 changed
the dataset base from container-visible `/Dataset/` to a host-absolute
path, under which NO data file opens in-container. Every fresh build since
Nov 2025 silently loads nothing, finds zero connections, and segfaults at
`ranking_system.cpp:770` (`ext_state_[0]` unguarded). Reverted to
`/Dataset/` + bind-mounted dataset (one line, `class/data_path.h`); the
JUGLET and POT_A_ORIG blocks carry the same disease (absolute bases) and
need the same treatment before any run through them — recorded, not
bundled. The teardown segfault persists after full output (cosmetic for
results, real robustness bug, separate item).

Replicated the code's filters exactly (`LanczosDiffLow(7)`, Gaussian
sigma-2 k10 circular) and compared against the paper recipe (finite diffs
→ Savitzky-Golay(7, wrap) → Gaussian(7, σ=2.0)) on Pot_A piece-1's rim
Dist/Height/Theta series (`smoother_spike.py`):

| series | max abs diff | range | relative |
|---|---|---|---|
| Dist | 1.66 | 63.75 | **2.6%** |
| Height | 1.38 | 54.88 | **2.5%** |
| Theta | 1.26 | 6.25 | **20.2%** |

All three exceed the 1% bar, so the smoother changes descriptors and the
ticket proceeds to gate measurement per the acceptance criteria: implement
the paper's smoother behind the same interface, rebuild the assembly side,
and re-score matching (not the probe — the probe does not use descriptors;
the LCS/beam-search matcher does). Caveats on record: the paper's SG window
is unspecified (7 used to match); Theta's 20% needs care as an angle series
(the diffs are what match on, and the comparison is diffs-vs-diffs).

- [ ] Spike: both smoothers on the same series, difference distribution,
      and the verdict on whether any match changes
- [ ] Lane declared: gate (which pairs move) or close-with-table
- [ ] If implemented: Pot_A per-pair from the current baseline, Juglet
      honest-10 unchanged-or-better, no other-pair regressions
- [ ] Authors' sample rides along wherever "correct" is needed
