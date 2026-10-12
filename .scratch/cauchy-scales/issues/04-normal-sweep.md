# 04: Normal-scale sweep — tight-first (rewritten after E-3, 2026-10-12)

**What to build:** ONE tight normal scale first (`a_norm = 0.5` at the
three hardcoded pairwise sites), weights frozen — not the 15°/30°/60°
sweep this ticket previously proposed. E-3 closed the distance direction
(half-online at p50 still 0/8+0/15 at score 266); normals are the term
that converges first (E-0: `ρ′` 0.18 at entry → 0.90 after one
iteration), so the tight arm attacks exactly the failure signature:
rotated false matches admitted at an elevated score.

**Answers:** S1

**Blocked by:** 01 (measured) + 03 (E-3 verdict routes here — distance
CLOSED, normals next). 05 reads 04's verdict before arming; 06 still
runs last (diagnostic, never a fix).

**Status:** ready-for-agent

## Why rewritten (E-3 verdict + unit reconciliation, 2026-10-12)

The previous version proposed arms at raw-knee ≈ 15°/30°/60° with
"predicted weights at 29° computed before running" (research §4.2).
Two problems:

1. The degree labels rest on ticket 01's "norm (degrees, /3.0)" line,
   whose numbers are actually weighted `fnorm`, not raw chords:
   `fnorm` p50 0.74–1.84 → chord 0.25–0.61 → θ ≈ 14°–36° median;
   p90 4.6–5.4 → θ ≈ 100°–129°; the 20-row tail at `fnorm` exactly
   5.99998 → chord exactly 2.0 → 180° opposite normals (the clamp
   noted in 01 surprise #4). Coherent — but it means the old
   degree arms were anchored to a mislabeled column. This rewrite
   states arms in scale units with `s/a²` + `ρ′` predictions computed
   straight from E-0's measured `s` distribution; no degree
   interpretation anywhere. (Open non-gating diagnostic: stored PCD
   normals are read without normalizing —
   `class/data_structure.cpp:301` keeps file values, only
   `line_normal_` is normalized at `:400` — so non-unit normals may
   feed the tail. Worker to report max `‖n‖` from dumps if cheap;
   does not gate the run.)
2. Sweep order. The failure mode is false admission (E-2/E-3: 0/8 at
   266 outscoring 2/8 at 262). The tight arm tests exactly that; the
   loose branch is only interesting if the tight arm keeps true pairs
   while the false-match signature persists. So: tight first, live is
   already measured (E-0 IS the live-arm record, no run needed),
   permissive only conditionally.

## Approved hunk spec

- [x] Arm T (tight) hunk: replace the `1.8` normal literal with `0.5`
  at exactly the three hardcoded pairwise sites (`Icp`,
  `Registration` 5-arg, `Registration` 6-arg — current lines
  `:1487`/`:1736`/`:1952`, verify at implementation time). Anchor is
  principled, not arbitrary: 0.5 is the graph path's own normal
  default (research §6 table: graph 0.5/5.0). Raw knee 0.5/3 ≈ 0.167
  in chord units. Distance / axis / rim scales keep their live
  literals (119.0 / 1.8 / 1.8). `IcpIncGraphAxis` (env-tunable
  `cauchy_norm`) is the in-run control, untouched.
  `multi_hypothesis_optimizer.cpp:888-890` untouched — dead by
  default (env-gated `ENABLE_MULTI_HYPOTHESIS`, no production sbatch
  sets it). Dead `IcpFine` (5.0) untouched, as in 02/03.
- [x] Dump calls pass the arm's live value (same live-value rule as
  02/03 — the dump's `a` column must show 0.5 for norm at iter 0, or
  it lies). Current dump lines `:1540`/`:1786`/`:2009`.
- [x] Weights, iteration counts, gates, solver options, caps, staging
  untouched. Six cap invariants still hold (100 ×5, 1000 ×5).
- [x] Batch arm `hpc/sbatch/run_e4_norm_tight.sbatch` modelled on
  `run_e3_p50.sbatch`: marker grep (three sites `CauchyLoss(0.5)` for
  norm, dist still 119.0, dump-live-value checks, cap checks, POT_A
  check) + freshness + one Pot_A full assembly, pinned
  `--nodelist=spartan-bm065`, dump enabled. Single run citing the
  existing solver-01 control (job 32624499, same node) per 02's
  recorded deviation (dump outcome-neutrality already proven, job
  32696452 A vs B).
- [x] **Kill criterion stated before running:** tighter kills rotated
  false matches but risks true pairs under noisy normals (median
  entry chord ~0.25–0.61 sits just above the tight raw knee 0.167 —
  true pairs keep partial weight, garbage gets ~none). Predicted at
  median entry `s ≈ 15.2` (E-0 iter-0 `s/a²` p50 4.07–5.40 at
  a=1.8): Arm T `s/a²` p50 ≈ 61, `ρ′` p50 ≈ 0.016 — normals ~fully
  suppressed at entry, solve proceeds on axis/P2L/rim. Pair 1–4
  survival is the tripwire. STOPs: (a) primary read must land on
  prediction or STOP; (b) DIVERGED-SOLVE stays 0; (c) accuracy
  branches — GROWS above 2/8 → normals direction LIVE, arm the
  permissive follow-up; HOLDS 2/8 with 1–4 alive → partial signal,
  arm permissive to test the loose branch; FALLS below 2/8 or loses
  1–4 → tight arm strangles true pairs, route 05/06 with the
  numbers; (d) score vs accuracy read SEPARATELY (scorer-prefers-wrong
  already answered YES in 03 step 0 — cite, do not re-run);
  (e) SURVIVED 14 / FEATURE_MATCHING 1 unchanged. No tuning whatever
  the outcome. Verdict NOT written here — worker reports evidence
  only; lead lands it in ticket 11 + S1.
- [ ] Arm P (permissive, CONDITIONAL on Arm T holding or growing):
  `a_norm = 3.6` (2× live) at the same three sites. Predicted median
  `s/a²` ≈ 1.17, `ρ′` ≈ 0.46. Same scaffold, same STOPs. Do NOT run
  alongside or before Arm T.

## Out of scope (unchanged from spec.md)

Weights, iteration counts, gates, caps/anchoring, correspondence gate,
Juglet half, "the paper's" values. Ticket 06 (shape diagnostic) still
runs last.
