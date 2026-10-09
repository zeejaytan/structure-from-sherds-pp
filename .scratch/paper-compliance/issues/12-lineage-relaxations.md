# 12: Lineage matching relaxations — adopt or revert, measured

## ROW 3 IN FLIGHT 2026-10-10 (job 32613911, bm065): RejectOutlier → upstream (20, 0.7)

Three live-path sites reverted (MakeSingleCorres :523, MakeMergeCorres
:587, MakeMultiCorres env-default :654; dead MakeCorWithSur :733/:758
untouched — single caller dead IcpFine). Control arm is the row-2 tree
(2/8 + 1/15, best 246, job 32610367). Poll watching; verdict on landing.
**Working in:** row 3 — do not touch reconstruction.cpp or the row-3
sbatch until the verdict lands.

## ROW 2 OUTCOME 2026-10-09 (jobs 32284017 vs 32610367, same node bm065): ADOPT 1.5 — byte-identical tie

Same-node pair, one value apart (4.0 → 1.5): 2/8 + 1/15 both arms, best
246.000 both, 11 states both, profile 198/0 both, SURVIVED 29 both with
identical per-pair values, rejections 0/68/57 both, diverged 140 both,
final transforms md5-identical (all seven T files). The 1.5–4.0 score
band holds no basket on Pot_A — the relaxation earns nothing, so per the
ticket's rule (a revert that removes nothing without loss is adopted as
upstream's) the lineage 4.0 goes and upstream 1.5 stays. No-regression by
construction: nothing changed anywhere. Comment cites this job pair.
Caveat (same as row 6): Juglet impact unmeasured. Next row per ticket
order (row 3: RejectOutlier).

## ROW 1 OUTCOME 2026-10-09 (job 32284017, bm065): KEEP 0.610 — first fully-correct pair

2/8 + 1/15 (k=2 directed = pair **1-4 both directions**: 7.8°/20–26mm,
merged 5×, in the final 6-piece graph — the largest joined assembly yet).
16 states, profile 99/0, diverged 140 (floor holds), 68 axis rejections
(the tightened gate firing). Against row-6 arm (0/8, best 310, fragments):
tighter axis gate → fewer garbage candidates → beam spends budget on true
merges (observed effect; mechanism not proven). KEEP upstream 0.610.
Next row per ticket order.

## ROW 6 OUTCOME 2026-10-05 (jobs 32274521 vs 32275782, same node bm065)

MINIMUM=1 → 6, everything else identical: diverged solves 545 → 140
(3.9× fewer fantasy branches — the floor refuses underdetermined sets
loudly via score-11 instead of solving them). Matching intact (1-2 still
survives, value 10→6 — fewer, still present). Accuracy 0/8 both (no
recovery, none expected — floor bounds damage like the guard).
VERDICT: KEEP 6 (upstream value; kills garbage with no true-pair harm on
Pot_A). Caveat recorded: Juglet assembly impact unmeasured (no Juglet
assembly baseline exists anywhere — the probe covers breaklines, not
matching); small-piece handling (the lineage's original worry) must be
re-checked if Juglet assembly ever runs. Next row per ticket order.

**Answers:** S1

**Blocked by:** nothing — assembly A/B measurements; one variable each

**Status:** ready-for-agent

**Needs-eye:** none — gate counts and pair movement, no geometry claim.

## Why this ticket exists

The fork audit (ticket 11) surfaced cluster-lineage relaxations in the
LIVE matching path that ticket 05 never knew — not ours, but affecting
paper behavior, so per 05's rules they get tickets, not shrugs:

| # | Site | Upstream | Ours (lineage) | Paper |
|---|---|---|---|---|
| 1 | PairwisePruning axis gate | 0.610 (35°, per the replaced comment) | 1.571 (90°, "Evidence-based") | silent |
| 2 | PairwisePruning lowest_score wipe | 1.5 | 4.0 ("preserve Blue-Red") | silent |
| 3 | Correspondence RejectOutlier | (20, 0.7) | (2.0, 0.85) ("fork tightened") | silent |
| 4 | opposing_ratio veto | ABSENT upstream (verified) | 0.9 RELAXED | silent |
| 5 | CountInlier segment contribution | uniform (no adaptive code upstream, verified) | adaptive ("ROBUSTNESS FIX") | silent |
| 6 | LCS length gate | MINIMUM_NUMBER=6 | =1 ("Further reduced for NURBS") | silent |
| 7 | Priority-list scoring | none (upstream has no combined score) | 0.7 local + 0.3 global via `ConnectivityOptimizer` in live `MakeHierarchyPriorityList` | silent |

All verified live-path (default sbatch binary), all cluster-lineage
(`d2445d8`/`869e696` era comments), none measured against anything since.
The paper gives no values here, so this is NOT paper-config work — it is
adopt-or-revert: either each relaxation earns its keep with numbers or it
goes back to upstream's value.

## What to build (one variable each)

For each row: revert to upstream value → assembly A/B per pair (Pot_A +
Juglet honest + authors' sample) → keep whichever wins, with the
measurement cited in the comment replacing the current bare
"Evidence-based"/"RELAXED"/"FIXED" claims (several assert history or
evidence with no pointer — the ticket that measured replaces the
adjective). Order by blast radius: 6 (length gate affects every LCS),
3 (every correspondence), 1–2 (pruning), 4–5 (veto/weighting) last.

## Rules (binding)

- Reverting is the experiment, not the decision: a revert that loses
  pairs stays reverted-from (current value adopted WITH the numbers, and
  the comment gets the ticket cite). A revert that gains pairs or
  removes false ones without loss is adopted as upstream's.
- #3 interacts with ticket 08's normal-gate work (same correspondence
  path): sequence after 08's stage decision or state why not.
- #6 interacts with the 50-point shard floor (length-1 matches feed
  single-point "breaklines" — read ticket 18's fragment discussion
  before touching it).

## Acceptance criteria

- [ ] Per-row A/B with numbers; end value per row with measurement cite
      in the code comment (bare adjectives gone). Rows 4–5 are
      lineage-ADDED (no upstream value to revert to): their experiments
      are disable-vs-keep, not value-vs-value.
- [ ] No-regression rule per experiment
- [ ] Any row kept at lineage value carries the ticket number, not
      "Evidence-based"
