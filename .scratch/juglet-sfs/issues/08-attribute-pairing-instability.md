# 08: Attribute the run-to-run pairing instability

**What to build:** identify why the beam proposes a *different* candidate
pair on every run over byte-identical input, and fix it or bound it. Not
a capability question -- an instability that makes every other
measurement on this object harder to read.

**Answers:** S1 (feeds the S1 result: an unstable matcher cannot support
a trustworthy join count)

**Blocked by:** none -- observable now, independent of 06's gate finding

**Status:** in-progress — LCS stage attributed DETERMINISTIC (same-node A/B done 2026-10-13)
**Working in:** laptop chat — verdict in; next: probe downstream (ranking/beam) for pair-choice jitter

## Progress (2026-10-13, verdict)

Same-node A/B (job 32851381, `t08_lcs_ab`, COMPLETED 0:0) on JUGLET input:
- dump_A: 60 pairs, md5=558e70881928cb86867e9a981c87b76b
- dump_B: 60 pairs, md5=558e70881928cb86867e9a981c87b76b
- dump_C: 60 pairs, same md5
- `*** LCS-ACROSS *** IDENTICAL (cross-process byte-identical)`
- `*** LCS-TWICE *** IDENTICAL first=60 second=60` (twice, in-process rerun)

So FeatureComp + PairwisePruning output is fully deterministic on the
same node / same binary / byte-identical input — both across processes
and in-process. The seed/thread suspects at the LCS stage are OUT:
no unseeded-RNG or OpenMP reduction jitter survives the sorted
fixed-precision dump compare.

That moves the instability downstream: the 4-run pair flood in Evidence
(6-7 vs 4-9/7-9/3-8/… vs 3-7/7-8/3-4 vs 7-9/7-8/5-7) must come from
*after* the LCS dump — ranking/priority tie-breaks, beam-state copying
(`RankingSubgraph::Copy`), or assembly-stage choices — or from
cross-node / differing-input runs (ticket 12 rule), not from LCS
generation. Next cheap probe: dump the ranked/priority pair order per
run the same way and byte-compare that.

## Progress (2026-10-13)

Cheap log-only step landed in `main_headless_correct.cpp` (+107, uncommitted):
sorted post-pruning LCS dump with fixed precision 6 to stdout
(`*** LCS-DUMP ***` lines) and `sfs_lcs_dump.txt`, plus opt-in
`SFS_LCS_TWICE=1` in-process rerun printing `*** LCS-TWICE ***
IDENTICAL|DIFFER`. Run plan (same node, `--nodelist` pinned per
ticket 12 rule): `md5sum sfs_lcs_dump.txt` across two processes, plus
one `SFS_LCS_TWICE=1` run. IDENTICAL in-process but DIFFER across
processes => seed/thread/address-space cause; DIFFER in-process =>
order-sensitive/stateful, report as distribution.

**Needs-eye:** none yet. No geometry claim; this is about which pairs get
proposed. If the fix changes placements, stage correct-vs-attempt then.

## Evidence

Four runs, identical input, identical binary family, different proposed
pair each time:

| Run | Proposed pair | Notes |
|---|---|---|
| holder normalabs run | 6-7 | 8 attempts, all 6-7 |
| holder wide20b run | 4-9, 7-9, 3-8, 2-9, 2-7, 1-7, 7-8, 6-7 | broad flood |
| oracle arm juglet67 | 6-7 (injected), then 3-7, 7-8, 3-4 | 3-7 PASSED score=20 |
| oracle arm juglet29 | 7-9, 7-8, 5-7 | injected 2-9 never merged |

Ticket 04 already noted this shape ("membership jitters 7 vs 6
singletons -- nondeterminism noted") but did not attribute it. The
earlier "race/nondeterminism-as-cause" refutation was about *scores* in
single-threaded runs, which is a different claim: identical inputs gave
identical scores. **Pair choice** still varies, so something upstream of
scoring is not deterministic or is order-dependent.

## Suspects (unranked)

- Uninitialised memory or unseeded RNG in FeatureComp / LCS scoring
- Order-dependent tie-breaking in the priority list (`priority_list_`
  built from `sub_graph_`, a `std::list` -- stable order, but equal
  scores?)
- KD-tree build/query nondeterminism (OpenMP: `NUMBER_OF_THREAD 12`
  defined in data_structure.h; if FeatureComp parallelises, reduction
  order can change floating-point ties)
- Beam state copying: `RankingSubgraph::Copy` uses `assign` on `edge_`
  but `priority_list_`/`matched_index_` may be shared or stale between
  the 5 parallel states (`TOP_k` roots)

## Acceptance

1. Same input twice in one process -> identical proposed pair set
   (byte-compare the LCS dump). If yes: it's a seed/thread issue; fix.
2. If no: the matcher is genuinely input-order sensitive -- document
   which stage, and report every future result as a distribution over
   runs rather than a single number.
3. Either way, add a cheap print of the full proposed-pair set per run
   so instability is visible without a diff.
