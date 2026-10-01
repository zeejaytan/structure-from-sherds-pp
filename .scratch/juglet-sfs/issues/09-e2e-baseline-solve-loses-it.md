# 09: E2E baseline — 15/15 matches in, 0/8 pieces out; the solve loses it

**Answers:** S1

**Blocked by:** nothing — log forensics first (job 31835065 log on Spartan),
then targeted A/B

**Status:** ready-for-agent

**Needs-eye:** a correct-vs-machine look enters if a proposed placement is
worth judging; the numbers below come first (the eye judges geometry, not
score tables).

## The handoff finding (ticket 07's clause, triggered 2026-10-01)

Preprocessing probe on the staged bundle: **15/15** (`e2e_fresh` arm).
Full assembly on the same bundle (job 31835065, `build_sif/Hierarchy-Clear`,
POT_A block, defaults): **0/8 sherds, 0/15 edges**. The fault has moved
downstream of extraction — recorded here, not in preprocessing. Log:
`sfs_main/e2e_pota_fresh_31835065.log` (+ `results_2026_10_01_1454/`).

## What the log already says (verified 2026-10-01, read the lines)

- Valid run: 8 shards load from `/Dataset/SfS_pp` (the fresh bind —
  Axes/Breaklines/Surfaces opens), binary carries the POT_A block
  (strings-verified pre-submit; ORIG absent).
- Matching is NOT the problem: all **15/15 true pairs SURVIVE** feature
  matching (values 7–148, "GROUND TRUTH DEBUG REPORT: SURVIVED: 15
  connections"). This mirrors the probe — the matcher agrees with the
  gate. (Side note: the report header says "Expected legitimate
  connections: 14" while 15 survive and the probe counts 15 mates from the
  same graph file — reconcile when convenient; it changes nothing here.)
- The solve: State 0 starts as 8 singletons → "Processed assembly state
  1/1 (score=0, 8 pieces)" → "*** COMPLETE ASSEMBLY FOUND! All 8 pieces
  successfully assembled! ***" with score **0.000** → accuracy 0/8, 0/15.
  A single 8-piece state with zero score wins by default as the only
  state. Pairwise pruning DID compute non-zero scores earlier (1-2: 11.0
  → 0.564 → 3.502 across iterations), so scores collapse somewhere
  between pruning and the final state — not absent from the start.
- Pottery validation rejects the bulk of correspondences per pair
  ("rejected: 6 distance, 225 pottery" typical) under "LEGACY ICP ...
  robust ICP removed" — read in context before citing as cause.

## What to find (log forensics, in this order)

1. Why exactly ONE state (1/1) with b=8/k=15 branching — where did
   expansion die? Beam-loop lines vs `PrepareNextStep` replenish.
2. Where the non-zero pruning scores go to 0.000 in the final state
   (`CalculateMatchingScore` → `graph_score_` ordering).
3. Whether plausibility (`CheckGraphPlausibility`, profile 6.5/6.0,
   overlap-10) rejected the TRUE merges, passed the FALSE ones, or never
   fired (score-0 win suggests the last — verify, don't assume).
4. ICP transforms per true pair: right pairs, wrong poses (registration
   failure) vs wrong pairs (matching failure — excluded already, but
   state it from the transforms, not from §above).

## PAPER-CONFIG ARM 2026-10-01 (job 31835275, COMPLETED 0:0)

Thirteen paper values, structure untouched (commit `3ff4f8d`). Same bundle,
same dataset, same binary path — only values differ from the 0/8 baseline.

- Result: **0/8 sherds, 0/15 edges** (log
  `sfs_main/e2e_pota_fresh_31835275.log`, `results_2026_10_01_1509/`).
- Behavior MOVED: 6 states (branching works — baseline had 1), best State
  #0 score **567.000** (baseline 0.000), real 2-piece merges in states.
  Scores discriminate now.
- Outcome UNMOVED: best state still 5 graphs (2+1+1+2+2 — "COMPLETE
  ASSEMBLY" fires on pieces-accounted, not pieces-joined), no piece
  correctly posed, no state joins all 8.

Verdict: values were A lever (search branches, scores discriminate) but
NOT the gap. The remainder is structural — weights mapping (07), OLS fit
+ max-rule + full-edge input (06), correspondence-stage gate (08), and/or
ICP poses. The bisect each owning ticket needs is now cheaper: paper
values are the control arm, current defaults the other; any single-value
revert that collapses 6-states-567 back to 1-state-0 names a load-bearing
value, while none of them reaching accuracy implicates structure.

## Out of scope (neighbor tickets own them)

- Preprocessing changes: forbidden. The input bundle is fixed
  (`pota_e2e_fresh`, probe 15/15 on record) — this ticket may not touch
  it, or nothing attributes.
- Profile numbers (06), weights (07), normal/grouping (08), overlap
  values (09): suspects with their own tickets and A/B rules. This ticket
  finds WHERE the solve loses it; those change WHAT it computes. Cite,
  don't duplicate.
- The restore of `POT_A_ORIG` in `data_path.h` (left flipped for this
  run — flip back when the next ORIG run needs it, not before).

## Acceptance criteria

- [ ] Forensics 1–4 answered with log line cites (which stage, what values)
- [ ] The single cause (or ranked causes) named: expansion death,
      score collapse, plausibility silence/misfire, or ICP poses —
      measured per pair where pairs are involved
- [ ] Fix lands in the owning ticket (06/07/08/09 or a new one), never here
- [ ] Re-run of THIS baseline after the fix, same bundle, same binary
      pattern — the number that must move is 0/8, and only a re-run moves it
