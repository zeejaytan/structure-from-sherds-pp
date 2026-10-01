# 11: Fork-vs-upstream audit — assembly (validate every change, document it)

**Answers:** S1

**Blocked by:** nothing — read-only until the doc says otherwise

**Status:** ready-for-agent

**Needs-eye:** none — diff classification, no geometry claim.

## Why this ticket exists

Same debt as preprocessing ticket 19, assembly side: our
`structure-from-sherds-pp` fork layers cluster-lineage work (alternate
optimizers, env-gated branches, tuning comments, `main_headless_correct`
itself — which does NOT exist upstream) plus our tickets (dropped-shard
logging, SG smoother arm, base-flag completion, Surface_F removal,
IcpFine guard, paper-config values, T0 dump, profile stddev, dataset
flips). No document maps change → paper relation. Ticket 05 audited
paper-vs-code but never fork-vs-upstream (provenance: which hunks are
ours vs inherited). This ticket is that map.

## Baselines (pin these; if upstream moves, re-pin and say so)

- Upstream: `upstream/main` @ `9195c016` (fetch ref, 2026-10-01).
- Ours: `origin/HEAD` at time of audit (record the SHA in the doc).
- Note: `main_headless_correct.cpp` (±1197-line diff) has NO upstream
  counterpart — attribute it whole (cluster lineage, presumably) via
  `git log`, then classify OUR hunks within it. Same for
  `multi_hypothesis_optimizer*`, `puzzlefusion_*`, `two_phase_*`,
  `hub_guided_*`, physics files: lineage-or-ours first, paper relation
  second.

## Method (in this order — attribution before classification)

1. `git diff upstream/main HEAD -- class/ main*.cpp CMakeLists.txt`,
   grouped by file/hunk. Attribute every hunk: (a) cluster lineage,
   (b) our ticketed change (cite ticket), (c) unattributed (findings).
2. New-file inventory (SG arm already tracked; optimizer files; hpc
   scripts/sbatch; ggce tests): one line each.
3. Classify every OUR hunk: PAPER-MATCH / DEVIATION-measured /
   ROBUSTNESS-no-behavior / INFRA / UNATTRIBUTED (same scheme as
   preprocessing ticket 19 — the two docs must use identical labels).
4. Cross-check against ticket 05's verdict table + paper-silent inventory:
   any hunk with paper behavior that 05 doesn't know about is a finding;
   any "ours" behavior 05 listed but unattributed here gets its ticket.

## Deliverable (the "then document")

`FORK_VS_UPSTREAM.md` at repo root: pinned SHAs, per-file hunk table,
new-file inventory, unattributed list (empty or ticketed). Stands alone —
it must be readable without the tickets, with pointers in.

## Acceptance criteria

- [ ] Doc exists with pinned SHAs and the per-file table
- [ ] Every OUR hunk classified; every UNATTRIBUTED either ticketed or
      accepted-with-reason in the doc
- [ ] Ticket-05 cross-check done (no unknown-to-05 paper-behavior hunk)
- [ ] Label scheme identical to preprocessing ticket 19's doc
- [ ] Nothing changed in code in THIS ticket (audit only)
