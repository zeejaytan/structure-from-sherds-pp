# 07: Huber diagnostic revert — restore Cauchy kernels (E-6 adopted nothing)

**Answers:** S1

**Blocked by:** 06 (E-6 verdict: ranking NOT flipped, NOT kernel-driven, nothing adopted)

**Status:** resolved 2026-10-14 (LAND — see lead verdict below)

**Needs-eye:** none — kernel swap-back + confirming run, no geometry claim.

## Why this ticket exists

E-6 (Arm H, job 32756813) swapped Cauchy→Huber at the three pairwise
solve sites to test whether divergence is kernel-driven. Verdict
(2026-10-12, landed in ticket 11 + S1): scaffold PASS, DIVERGED 0,
accuracy 2/8+1/15 → 0/8+0/15 (starvation, not amplification), ranking
NOT flipped → NOT kernel-driven, nothing adopted. But the HuberLoss
hunk is still live in the tree — every assembly since E-6 has run on
the experimental kernel, not the settled one. This ticket puts Cauchy
back.

## What to do

1. In `class/reconstruction.cpp`, swap `HuberLoss` → `CauchyLoss` at
   the three pairwise sites only (`Icp` ~:1486-1489, `Registration5`
   ~:1735-1738, `Registration6` ~:1951-1954; 4.0/1.8/1.8/1.8 scales
   stay — solver-01 root values). `LogE0Residuals` dump lines stay
   (env-gated logging, not the kernel). `IcpIncGraphAxis` weights and
   everything else untouched — one variable (the kernel only).
2. Same-node bm065 Pot_A confirming run on the reverted tree: must
   reproduce the settled baseline (post-row-7: 0/8+0/15, best ~260,
   diverged 0, SURVIVED 29). If it doesn't, STOP — the tree differs
   from the settled one somewhere else; report, don't tune.
3. Working notes in `## Revert work` below; verdict edit HELD for lead.
   No commits.

## Acceptance criteria

- [x] HuberLoss gone from the three sites (CauchyLoss at same scales)
- [x] Confirming run reproduces settled baseline on same node (see lead
      verdict: the ticket's stated baseline was mis-specified — the run
      reproduces the E-6 Cauchy control exactly, which IS the settled
      baseline for a Cauchy tree)
- [x] Spartan checkout left clean; scripts uncommitted for lead review

## Lead verdict (LAND 2026-10-14)

The STOP fired on a mis-specified baseline, not a bad tree. The ticket
stated post-row-7 numbers (0/8+0/15 @ ~260) — but that tree runs the
experimental Huber kernel, which E-6 itself proved starves pair 1-4
(2/8+1/15 → 0/8+0/15). The Cauchy-restored run (job 32948918) reproduces
the E-6 control (solver-01 job 32624499) EXACTLY: 2/8+1/15, best 262,
SURVIVED 29, diverged 0. That is the correct comparator, and it passes.
LAND the revert: the experimental leftover goes, the only true pair
seats again. Nuance for E-6's "not kernel-driven" (recorded in ticket
06): divergence is not kernel-driven (0 both either way), but the true
pair's survival IS kernel-gated — Cauchy seats 1-4, Huber starves it.

## Revert work

### Laptop edit (HELD uncommitted for lead review — DO NOT commit)
- `class/reconstruction.cpp`: HuberLoss → CauchyLoss at the three pairwise
  sites only (Icp :1486-1489, Registration5 :1735-1738, Registration6
  :1951-1954). Scales frozen at solver-01 roots (4.0/1.8/1.8/1.8).
  LogE0Residuals dump calls untouched; IcpIncGraphAxis + dead IcpFine
  untouched; weights/iterations/gates untouched. My hunk: 12 lines
  (kernel + comment on each). NOTE 15:50 AEDT: `git diff` now shows 12/45
  against HEAD eb6c389 — the extra 33-line deletion (@1332,
  `GraduatedDistScale` dead helper) is NOT mine; per eb6c389's message it
  is ticket-10's uncommitted dead-helper deletion sharing this worktree.
  My 3 site hunks verified intact after it appeared. CRLF preserved.
- Grep evidence (laptop tree): `HuberLoss` count 0;
  `new ceres::CauchyLoss(4.0)` ×3; `new ceres::CauchyLoss(1.8)` ×9.

### Confirming run — DOES NOT reproduce the stated baseline (STOP per spec)
- Script: `hpc/sbatch/run_huber_revert_confirm.sbatch` (new, uncommitted;
  pre-flight `bash -n` clean; in-job sed patch simulated against HEAD
  source → Huber 0, Cauchy 4.0×3/1.8×9, control sites intact, dumps intact).
  Transient-patch pattern copied from `run_row7_priority_off.sbatch`:
  pull ff-only → verify Huber-live preconditions → sed Huber→Cauchy →
  locked rebuild of separate binary `Hierarchy-Clear-huberrevert` →
  Pot_A run → trap restores `class/reconstruction.cpp`. Live copy scp'd to
  `sfs_main/run_huber_revert_confirm.sbatch` (untracked area); submitted
  `sbatch --nodelist=spartan-bm065`.
- Job 32948918, COMPLETED 0:0, node spartan-bm065. Build exit 0, binary
  exit 0, freshness check passed (binary newer than all sources). Input
  census: Breaklines 16 / Mesh 8 / Axes 8 / Surfaces 16. Run time ~15m.
- Outcome (log `sfs_main/e2e_pota_huberrevert_32948918.log`, single final
  report): **Sherd 2/8 (25%), Edge 1/15 (6.667%), best State #0 7/8 pieces
  score=262.000, DIVERGED-SOLVE 0, SURVIVED 29 total / 14 pair-line,
  FEATURE_MATCHING 1 pair-line, veto 0, overlap=true 56, PROFILE 104/104.**
- Vs stated baseline (0/8+0/15, best ~260, diverged 0, SURVIVED 29):
  SURVIVED and diverged match; **accuracy (2/8+1/15 vs 0/8+0/15) and best
  (262 vs ~260) do NOT match → no reproduce → STOP, no tuning attempted.**
- Note: this run reproduces the E-6 *control* (solver-01 job 32624499:
  2/8, 1/15, 262, SURVIVED 14 / FM 1, DIVERGED 0) exactly. The stated
  baseline (row-7 OFF) ran on the Huber-live tree; this is the first
  Cauchy-at-root-scales + local-only-scorer data point. What it means is
  for the lead (verdict HELD).

### Hygiene anomalies
- HEAD moved mid-ticket: Spartan pulled 6a6fb8d → **eb6c389** (lead's
  cleanup landed 15:07 AEDT while job was queued; laptop still adba2d4).
  Benign for this run: eb6c389 touches only stdout glyph strings
  (feature_matching.cpp, main_headless_correct.cpp), docs, and poll-junk
  deletion — commit message states `reconstruction.cpp` untouched; none of
  the outcome grep patterns touch the restored glyphs. Run is effectively
  adba2d4-code + Cauchy patch.
- Spartan checkout left clean (`git status --porcelain -- class/` empty
  after trap restore). Separate binary kept at
  `build_sif/Hierarchy-Clear-huberrevert` (untracked build dir).
- Other cluster activity: job 32754924 (`claude-r`) RUNNING on bm017
  throughout — different node, no build interaction with this run (own
  lock file `.huberrevert_build.lock`, own binary).
- Juglet impact unmeasured (Pot_A only) — caveat carried.