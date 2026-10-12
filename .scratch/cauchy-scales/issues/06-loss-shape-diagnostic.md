# 06: Loss-shape swap as diagnostic (rewritten after E-5, 2026-10-12)

**What to build:** ONE kernel swap first (Huber at solver-01 live
scales), everything else frozen — as a diagnostic of whether
divergence is kernel-driven at all. Never a fix in this ticket: no
shape is adopted here whatever wins.

**Answers:** S1

**Blocked by:** 01 (measured) + 05 (E-5 verdict — axis tight HOLDS,
score reshuffles without seating; tolerance arms nearly exhausted).
Runs last among the run tickets. Arm P (`a_axis = 2.8`) is EARNED
but DEFERRED per user decision 2026-10-12: 06 runs now, P after only
if 06 is inconclusive.

**Status:** done (Arm H run `32756813`; verdict landed in ticket 11
+ S1 2026-10-12 — not kernel-driven, Tukey gated off)

## Why rewritten (E-5 verdict + interview, 2026-10-12)

The previous version said "scales fixed at live values" and
"same-node A/B vs 01 tree" without stating the tree or the arm
order. Settled 2026-10-12:

1. Baseline is the solver-01 root tree (dist 4.0 / norm 1.8 / axis
   1.8 / rim 1.8 — the E-0 record), NOT the E-5 working tree (dist
   119, norm 0.5, axis 0.5). The working tree carries five
   experiments of suppression; a kernel swap there confounds shape
   with tight scales. Cleanest kernel-only reading: restore the
   four literals to root, swap one kernel, chain to solver-01
   (job 32624499) via the E-0 record. The restore is itself part
   of the hunk and is verified by marker grep.
2. Huber first, Tukey conditional. The failure mode across E-2–E-5
   is seating, not divergence (DIVERGED-SOLVE 0 in every arm; the
   guard holds). Huber's predicted signature (less far-init
   divergence, MORE false-positive convergence) attacks the live
   question — whether the scorer's wrong-answer preference is
   kernel-driven. Tukey (reverse signature) runs only if Huber's
   ranking surprises.
3. `ρ′`-comparability note: Huber and Tukey have no Cauchy `a`
   with the same `ρ′` curve — the `a` literal is kept numerically
   identical (4.0/1.8/1.8/1.8) so the swap is kernel-only, and the
   verdict compares rankings (DIVERGED count, accuracy, score-vs-
   accuracy split), never raw `ρ′` values across kernels. The
   LogE0 dump stays Cauchy-arithmetic; worker reports its numbers
   as scaffold health only, not as cross-kernel evidence.

## Approved hunk spec

- [x] Arm H (Huber) hunk: at exactly the three hardcoded pairwise
  sites (`Icp`, `Registration` 5-arg, `Registration` 6-arg —
  currently `.../reconstruction.cpp:1486-1489` / `:1735-1738` /
  `:1951-1954`, verify at implementation time), restore the four
  literals to solver-01 root (dist 4.0, norm 1.8, axis 1.8, rim
  1.8) AND swap all four `CauchyLoss` → `HuberLoss` at those sites
  only. Kernel-only + restore in one hunk; marker grep verifies
  both (three sites `HuberLoss(4.0)` + `HuberLoss(1.8)` ×3, zero
  `CauchyLoss(119.0)` / `CauchyLoss(0.5)` remaining at pairwise
  sites). `IcpIncGraphAxis` (env-tunable Cauchy) is the in-run
  control, untouched. `multi_hypothesis_optimizer.cpp:887-890`
  untouched — dead by default. Dead `IcpFine` untouched.
  DONE: commit `34cf112` (30 +-, sbatch 180 lines).
- [x] Dump calls pass the restored live values `(4.0, 1.8, 1.8,
  1.8)` (same live-value rule as 02–05 — the dump's `a` column
  must show the restored values, or it lies). Current dump lines
  `:1540`/`:1786`/`:2009`.
  DONE: dumps pass (4.0, 1.8, 1.8, 1.8), verified at all sites;
  graph env untouched.
- [x] Weights, iteration counts, gates, solver options, caps, staging
  untouched. Six cap invariants still hold (100 ×5, 1000 ×5).
  DONE: verified in commit message.
- [ ] Batch arm `hpc/sbatch/run_e6_huber.sbatch` modelled on
  `run_e5_axis_tight.sbatch`: marker grep (Huber at three sites,
  root scales, dump-live-value checks, cap checks, POT_A check) +
  freshness + one Pot_A full assembly, pinned
  `--nodelist=spartan-bm065`, dump enabled. Single run citing
  solver-01 (job 32624499, same node) as the direct control per
  02's recorded deviation (dump outcome-neutrality already proven,
  job 32696452 A vs B).
  DONE: `run_e6_huber.sbatch` (180 lines), job 32756813 COMPLETED
  0:0 in 00:14:55 on spartan-bm065, 38,934 dumps,
  log `sfs_main/e2e_pota_e6_32756813.log` (18M).
- [ ] **Kill criterion stated before running:** Huber should show
  less far-init divergence but MORE false-positive convergence
  than Cauchy at the same `a`; Tukey the reverse. Predicted: 0/8
  or worse with score at-or-above 262 (false-positive convergence
  — the scorer's wrong-answer preference amplified), DIVERGED 0.
  STOPs: (a) scaffold health — dump `a` column shows restored
  values or STOP; (b) DIVERGED-SOLVE read separately from accuracy
  (this ticket's only divergence question: does Huber diverge
  where Cauchy held 0?); (c) accuracy branches — ranking as
  predicted → divergence/seating not robust-kernel-driven,
  tolerance line CLOSED, route solver-setup with the numbers;
  ranking FLIPPED (Huber seats ≥1 where Cauchy seated 0, or
  diverges where Cauchy held) → kernel-driven after all, route a
  shape follow-up, adopt NOTHING here; (d) score vs accuracy read
  SEPARATELY (scorer-prefers-wrong cited from 03 step 0 — if Huber
  amplifies the split, that is the finding); (e) SURVIVED 14 /
  FEATURE_MATCHING 1 unchanged. No tuning whatever the outcome.
  Verdict NOT written here — worker reports evidence only; lead
  lands it in ticket 11 + S1.
  DONE (E-6 verdict 2026-10-12): scaffold health PASS; DIVERGED 0
  (no far-init divergence under Huber); accuracy 2/8 + 1/15 → 0/8
  + 0/15 with score 262 → 239 (starvation, NOT predicted
  amplification) → ranking NOT flipped → NOT kernel-driven;
  SURVIVED 14 / FEATURE 1 unchanged; pair 1-4 alive (21.0). No
  tuning, nothing adopted. Verdict landed in ticket 11 + S1.
- [ ] Arm K (Tukey, NOT RUN — gate was Huber ranking surprising,
  and it wasn't). Ticket CLOSED with Arm H as the diagnostic
  record.

## Out of scope (unchanged from spec.md)

Weights, iteration counts, gates, caps/anchoring, correspondence gate,
Juglet half, "the paper's" values. Adopting any shape. Arm P still
deferred, not skipped.
