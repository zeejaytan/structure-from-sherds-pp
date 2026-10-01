# 05: Assembly-matching methodology audit — paper vs matcher

**Answers:** S1

**Blocked by:** nothing — audit only; each gap below names its own fix ticket
where one exists

**Status:** ready-for-agent

**Needs-eye:** none — text-vs-code comparison, no geometry claim.
Renders enter only if a traced path turns out to depend on geometry
nobody has looked at.

## Why this ticket exists

Ticket 16 audited preprocessing (§IV-B1), the axis, and assembly *inputs*
(descriptors, flags, files, precedence). The assembler's *matching
methodology* — what the paper says matching does vs what
`feature_matching` / `ranking_system` / the optimizers do — was never
compared. That is now the largest unaudited surface in the gap-closure
project. Paper source: `papers/text/sfspp-2502.13986v1.md` (matching +
validation passages, e.g. lines 249, 519–550 — cite by line, not memory).

## How to run it (ticket-16 rules apply)

- Every item verified against BOTH the paper text and the code; line
  numbers cited, not summaries. Anything touched later cites the lines.
- Anything the audit cannot decide from reading becomes a measurement
  proposal, not a guess. Do not let "already verified" override cheap
  local evidence checks.
- Nothing here re-argues settled measurements — cite ticket + line.

## Checklist (seeded from a 2026-10-01 spot-check, not exhaustive)

1. **Profile-curve validation.** Paper: rz-plane projection, 7mm bins,
   orthogonal regression, 7mm rejection threshold (lines ~249, ~550).
   Code: `ProfileChecking(profile, 6.5, 6.0)` at `ranking_system.cpp:1913`
   ("RELAXED", was 6.0/5.5, original 7.0/7.0), plus callers in
   `multi_hypothesis_optimizer.cpp:618,973` and
   `puzzlefusion_global_optimizer.cpp:434`. For each: (a) is
   `CalculateLeastSquare` the paper's orthogonal regression or ordinary
   least squares; (b) which callers run in the sbatch binary
   (`main_headless_correct` path) vs dead/legacy optimizers
   (`multi_hypothesis_optimizer_backup.cpp` exists — check before citing
   either); (c) where the relaxed numbers came from (measurement or drift).
2. **Overlap region detection.** Paper line ~249 (physical compatibility
   via overlapping areas). Code: `Enhanced3DOverlapDetector`
   (`surface_overlap_detector.h:26`, defaults 2.0mm/1000.0) read at
   `puzzlefusion_global_optimizer.cpp:321`. Same liveness question: does
   this execute in a run that matters, and do the thresholds match any
   paper value.
3. **Ranking/plausibility thresholds.** `CheckGraphPlausibility`, the
   rim-height bands in `CountPCInlier`, the 0.4/0.4 μ/ν weights (paper
   line ~519) — each compared to its paper value or recorded as ours.
4. **Match search itself.** What the paper specifies (LCS? beam search?
   pairwise pruning order) vs `FeatureComp` /
   `FeatureCompGraphBuilding` / `PairwisePruning` structure. Structural
   comparison only — no re-tuning inside this ticket.
5. **Ticket-18 interaction.** Face-crossing traces feed profile curves and
   descriptor series. Whatever this audit says about profile/descriptor
   sensitivity must cite ticket 18's pollution finding, not assume clean
   rims.

## Out of scope (recorded, not ducked)

- Re-tuning any threshold found different. New values get their own
  tickets with per-pair measurements, one variable each — bundling them
  here would make every result unattributable.
- The base-half removal (assembly ticket 02, closed): the paper HAS base
  machinery; we run rim-only by explicit decision. This audit records that
  as the standing deviation wherever rim/base appears, it does not
  re-litigate it.
- MLESAC-vs-RANSAC, LM-vs-trust-region, SG window: owned by ticket 16's
  notes and assembly ticket 04 respectively. Cross-cite, don't duplicate.

## Acceptance criteria

- [ ] Each checklist item resolved to match / deviation-with-ticket /
      accepted-with-reason, with paper-line + code-line cites
- [ ] Every deviation names its fix ticket or records why it is accepted
      (threshold drift needs a provenance answer, not a shrug)
- [ ] Live-vs-dead verdict for each optimizer call path in the sbatch
      binary (the backup-file precedent is why: duplicate files exist and
      only one can run)
- [ ] Nothing in this ticket re-argues settled measurements
