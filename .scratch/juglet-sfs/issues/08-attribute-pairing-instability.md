# 08: Attribute the run-to-run pairing instability

**What to build:** identify why the beam proposes a *different* candidate
pair on every run over byte-identical input, and fix it or bound it. Not
a capability question -- an instability that makes every other
measurement on this object harder to read.

**Answers:** S1 (feeds the S1 result: an unstable matcher cannot support
a trustworthy join count)

**Blocked by:** none -- observable now, independent of 06's gate finding

**Status:** ready-for-agent

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
