# 05: Axis sweep tight-first, rim excluded by citation (rewritten after E-4, 2026-10-12)

**What to build:** ONE tight axis scale first (`a_axis = 0.5` at the
three hardcoded pairwise sites), everything else frozen at the E-4
tree — plus a rim verdict by citation, not by a run. E-4 closed
normals both ends (live 1.8 keeps 2/8, tight 0.5 falls to 0/8 with
score starving to 239); axis is the last live tolerance direction,
and E-0 puts it in the same structural position distance was in
before E-3: knee inside its own inlier spread.

**Answers:** S1

**Blocked by:** 01 (measured) + 04 (E-4 verdict routes here — normals
CLOSED, axis next). 06 still runs last (diagnostic, never a fix).

**Status:** done (Arm T run `32753136`; verdict landed in ticket 11
+ S1 2026-10-12 — HOLDS, Arm P earned but deferred; rim by
citation)

## Why rewritten (E-4 verdict + E-0 axis numbers, 2026-10-12)

The previous version proposed sweeping axis and rim together with
"raw knees spanning the measured inlier σ" and rim `a/w` arms, with
no measured numbers attached. E-0 has since landed:

1. Axis is half-online at live, not marginally tight. E-0 pairwise
   axis (Cao units): `s/a²` p50 ≈ 2.45 at `a = 1.8`, `ρ′` p50 ≈
   0.29, weighted `‖f‖` p50 2.82 → raw p50 28.2 (weights
   `w_a = 0.1` at all three pairwise sites). Live raw knee
   `a/w = 18` sits just below the raw median 28.2 — the "half-online"
   regime E-3 tested for distance, and it lost. Tight-first is the
   cheaper falsifier, same logic as 04.
2. Rim is already online — no suppression to relieve. E-0 rim:
   `ρ′` p50 ≈ 0.97, `‖f‖` p50 0.71 / p90 2.81 at `a = 1.8`,
   `w_r = w_h = 1.0`. There is no headroom a run could open, so rim
   gets its verdict by citation in this ticket, not a worker round.
   The old ticket's mixing note stands and is recorded below:
   radius+height (mm each, different meaning) share one scale — if
   they ever split, record which moves.
3. Scale-unit discipline (lesson of 04's rewrite): arms are stated as
   the `a` literal with `s/a²` + `ρ′` predictions from E-0's measured
   `s`, plus the raw-knee translation for interpretation. No degree
   or mm labels on the arm itself.

## Approved hunk spec

- [x] Arm T (tight) hunk: replace the `1.8` axis literal with `0.5`
  at exactly the three hardcoded pairwise sites (`Icp`,
  `Registration` 5-arg, `Registration` 6-arg — axis is the third of
  the four literals, currently `.../reconstruction.cpp:1488` /
  `:1737` / `:1953`, verify at implementation time). Raw knee
  `0.5/0.1 = 5.0` vs raw median 28.2 (p5 1.9, p90 105) — axis fully
  suppressed at entry. Everything else frozen at the E-4 tree:
  dist 119.0, norm 0.5, rim 1.8, all weights (`w_a = 0.1` included).
  ONE variable vs the E-4 tree; the control comparison chains
  through E-4 to solver-01 rather than direct. `IcpIncGraphAxis`
  (env-tunable) is the in-run control, untouched.
  `multi_hypothesis_optimizer.cpp:888-890` untouched — dead by
  default (env-gated `ENABLE_MULTI_HYPOTHESIS`, no production sbatch
  sets it). Dead `IcpFine` (2.0) untouched, as in 02/03/04.
  DONE: commit `e08e764` (12 +-, sbatch 190 lines).
- [x] Dump calls pass the arm's live value (same live-value rule as
  02/03/04 — the dump's `a` column must show 0.5 for axis at iter 0,
  or it lies). Current dump lines `:1540`/`:1786`/`:2009` pass
  `(119.0, 0.5, 1.8, 1.8)` → Arm T passes `(119.0, 0.5, 0.5, 1.8)`.
  DONE: dumps pass (119.0, 0.5, 0.5, 1.8), `a = 0.5` verified at all
  3 pairwise sites; graph env `a = 1.0` untouched.
- [x] Weights, iteration counts, gates, solver options, caps, staging
  untouched. Six cap invariants still hold (100 ×5, 1000 ×5).
  DONE: E-4 tree frozen, verified in commit message.
- [ ] Batch arm `hpc/sbatch/run_e5_axis_tight.sbatch` modelled on
  `run_e4_norm_tight.sbatch`: marker grep (three sites
  `CauchyLoss(0.5)` for axis, dist still 119.0, norm still 0.5,
  dump-live-value checks, cap checks, POT_A check) + freshness +
  one Pot_A full assembly, pinned `--nodelist=spartan-bm065`, dump
  enabled. Single run citing E-4 (job 32743207, same node) as the
  direct control per 02's recorded deviation (dump
  outcome-neutrality already proven, job 32696452 A vs B;
  solver-01 job 32624499 is the root control).
  DONE: `run_e5_axis_tight.sbatch` (190 lines), job 32753136
  COMPLETED 0:0 in 00:15:36 on spartan-bm065, 33,154 dumps,
  log `sfs_main/e2e_pota_e5_32753136.log` (17M).
- [ ] **Kill criterion stated before running:** tighter strangles true
  pairs along with false ones (raw median 28.2 vs tight raw knee
  5.0 — true pairs keep ~3% weight, garbage gets ~none). Predicted
  at median weighted entry `s ≈ 7.94` (E-0 `s/a²` p50 2.45 at
  a=1.8): Arm T `s/a²` p50 ≈ 31.8, `ρ′` p50 ≈ 0.031 — axis fully
  suppressed at entry, solve proceeds on P2L/rim. Pair 1–4 survival
  is the tripwire. STOPs: (a) primary read must land on prediction
  or STOP; (b) DIVERGED-SOLVE stays 0; (c) accuracy branches —
  GROWS above 0/8 (E-4 tree baseline) → axis direction LIVE, arm the
  permissive follow-up; HOLDS 0/8 with 1–4 alive → partial signal,
  arm permissive to test the loose branch; FALLS with 1–4 lost or
  new divergence → axis carries load even half-online, route 06
  with the numbers; (d) score vs accuracy read SEPARATELY
  (scorer-prefers-wrong already answered YES in 03 step 0 — cite,
  do not re-run); (e) SURVIVED 14 / FEATURE_MATCHING 1 unchanged.
  No tuning whatever the outcome. Verdict NOT written here —
  worker reports evidence only; lead lands it in ticket 11 + S1.
  DONE (E-5 verdict 2026-10-12): primary read LANDED (axis `s/a²`
  p50 27.4/47.6/17.9, `ρ′` 0.035/0.021/0.053 at iter 1 vs predicted
  ~31.8/~0.031); DIVERGED 0; accuracy HOLDS 0/8 + 0/15 (E-4
  baseline), pair 1-4 alive (21.0) → HOLDS branch → Arm P EARNED;
  score 239 → 258 (reshuffle without seating); SURVIVED 14 /
  FEATURE 1 unchanged. No tuning. Verdict landed in ticket 11 + S1.
- [ ] Arm P (permissive, EARNED by Arm T HOLDS 2026-10-12):
  `a_axis = 2.8` (weighted units; raw knee `2.8/0.1 = 28` ≈ raw
  inlier median — the axis analogue of E-3's p50 arm) at the same
  three sites. Predicted median `s/a²` ≈ 1.0, `ρ′` ≈ 0.5. Same
  scaffold, same STOPs. Do NOT run alongside or before Arm T.
- [x] Rim verdict by citation (no run): E-0 rim `ρ′` p50 0.9695 at
  live 1.8 — already inlier-quality, no suppression to relieve, no
  headroom. Radius+height mixing recorded: both mm, different
  meaning, one scale; split only if a future ticket needs it.
  DONE: confirmed by E-5 frozen-tree rows (Reg5 123,114 + Reg6
  140,834 rows at `a = 1.8`).

## Out of scope (unchanged from spec.md)

Weights, iteration counts, gates, caps/anchoring, correspondence gate,
Juglet half, "the paper's" values. Ticket 06 (shape diagnostic) still
runs last.
