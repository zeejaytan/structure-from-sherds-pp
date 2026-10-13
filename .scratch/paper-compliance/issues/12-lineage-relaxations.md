# 12: Lineage matching relaxations — adopt or revert, measured

## ROW 4 OUTCOME 2026-10-09 (jobs 32613911 vs 32619839, same node bm065): stays DISABLED, WITH numbers — enabling LOSES the only correct pair

Same-node pair, one variable (veto disabled → enabled): 2/8 + 1/15
(row 3) collapses to **0/8 + 0/15** (row 4). The veto fires exactly 3×,
all on pair 5-6 (a TRUE pair: GT-graph mates, 48–55 inliers, score
~0.5) — garbage removed nowhere. Downstream the only correct pair 1-4
merges 3× vs 8× and never reaches the beam; best state 266 vs 264
(scores move, joins don't — again). SURVIVED sets identical (29, same
values), rejections 0/68/57 both, diverged 140 both, profile 218/200
passed zero kills both — the veto's damage is purely downstream
starvation. Per the ticket's rule (a revert that loses pairs stays
reverted-from) the branch returns to disabled, now WITH the measurement
cite instead of the bare "too aggressive". Verdict: KEEP DISABLED.
Caveat (same as rows 6/2/3): Juglet impact unmeasured. Next row per
ticket order (row 5: CountInlier adaptive, disable-vs-keep).

## ROW 3 OUTCOME 2026-10-09 (jobs 32610367 vs 32613911, same node bm065): ADOPT (20, 0.7) — headline tie, same pair

Same-node pair, one variable (tightened (2.0, 0.85) → upstream (20,
0.7) at three live-path sites; dead MakeCorWithSur untouched): 2/8 +
1/15 both arms, best 264 vs 246 (scores move, joins don't — same pattern
as the smoother A/B), 11 states both, profile 200/198 passed with zero
kills both, SURVIVED 29 both with identical per-pair values, GT report
14 both, rejections 0/68/57 both, diverged 140 both. Same passing pair
both arms (1-4 both directions: 8.3°/21–27mm vs 7.8°/20–26mm). Final
transforms byte-differ but the scored outcome is identical — the
tightening earns nothing, so per the ticket's rule the lineage (2.0,
0.85) goes and upstream (20, 0.7) stays. Valid run: rebuild compiled
reconstruction.cpp, runtime gate confirmed 20.0/0.70, freshness clean.
Caveat (same as rows 6/2): Juglet impact unmeasured. Next row per ticket
order (row 4: opposing-ratio veto, disable-vs-keep).

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

## Row-5 work (2026-10-13, worker)

Finding pre-A/B: the `ROBUSTNESS FIX: Adaptive segment contribution` gates
live ONLY in CountInlier overloads 1 & 2 (feature_matching.cpp:1053-1072,
:1128-1149). The whole live assembly path — both inlierCalculate overloads
(reconstruction.cpp:791-868) and all four call sites (:1607/:1823/:2055/
:2250) — uses overload 3 (num_points vector<double>, :1157-1215), which
accumulates unconditionally (no gate). Repo-wide grep: zero callers of
overloads 1&2. Expectation: disable-vs-keep returns a byte-identical
no-op pair; verdict then DISABLE-as-dead-code-cleanup (adopt upstream
uniform `if (dummy_inlier > MINIMUM_NUMBER / 2)` per 869e696^, Row-2 rule),
acos clamp + INLIER DEBUG prints untouched (one variable: the gate only).
If the pair comes back NON-identical, the dead-code read is wrong and the
numbers decide (experiment is self-checking either way).

Arms (same-node bm065, Pot_A pota_e2e_fresh, `bin 8`; HEAD drifted
26db0e5 -> 6a67119 (ticket-09 comment-only at FM:1974, clear of the
CountInlier hunks) while queued — both arms pull the same HEAD, still one
variable; actual built HEAD is echoed in each job log):
- KEEP (adaptive as-is): job 32856411, RUNNING on bm065 since 18:12
  (pulled 6a67119, POT_A verified, in-container rebuild exit 0,
  freshness clean, binary build_sif/Hierarchy-Clear-row5keep), script
  hpc/sbatch/run_row5_countinlier_keep.sbatch, log e2e_pota_row5keep_<job>.log.
- OFF (transient in-job revert of both gates, restored post-run, tree left
  clean): job 32858323, script hpc/sbatch/run_row5_countinlier_off.sbatch
  (python patcher asserts each gate block found exactly once; verified on
  laptop against HEAD: 6+/34- across the two hunks only), binary
  build_sif/Hierarchy-Clear-row5off, log e2e_pota_row5off_<job>.log.
Both scripts: revert ticket-08 JUGLET data_path.h residue pre-pull,
ff-only pull, rows-1-4 marker verify, POT_A verify, flock-serialised
in-container rebuild (shared Hierarchy-Clear untouched), freshness check,
input census, outcome greps. Scripts shipped to sfs_main/ via scp
(uncommitted; for session review/commit). No commits by worker.

OUTCOME 2026-10-13 (jobs 32856411 KEEP vs 32858323 OFF, same node
bm065, both built HEAD 6a67119, sacct COMPLETED ExitCode 0:0 both,
15:24 vs 14:34): VERDICT DISABLE — adaptive gate removed, upstream
uniform adopted WITH numbers (Row-2 rule: a revert that removes nothing
without loss is adopted as upstream's).

A/B table (KEEP vs OFF — identical every line):

| measure | KEEP 32856411 | OFF 32858323 |
|---|---|---|
| directed joins | 0/8 + 0/15 | 0/8 + 0/15 |
| best state | #0, 7/8 pcs, 239.000 | identical |
| SURVIVED (whole-log) | 29 | 29 |
| GT breakdown | FM 1 / ICP_CORR 2 / SURVIVED 14, per-pair values identical (1-4: 21.0) | identical |
| AXIS / OVERLAP / SCORE rejections | 67 / 58 / 0 | 67 / 58 / 0 |
| veto (Opposing normals) | 0 | 0 |
| diverged | 0 | 0 |
| profile RESULT / PASSED | 88 / 88 | 88 / 88 |
| INLIER DEBUG lines | 0 | 0 |
| run outputs | results_2026_10_13_1826 | results_2026_10_13_1841, all 7 piece xyz md5-identical, assembly_data.txt identical modulo run-dir stamp |

Remaining log diff is timing/concurrency artifacts only (`Analysis
time:` ms values, stdout interleave splits/merges of MAKEMULTI/LEGACY
lines — same content, different flush boundaries). Pair 1-4 MAKEMULTI
traces show the same iter/raw/kept pattern modulo that interleave noise.

No-regression: outputs byte-identical — removing the gate changes
nothing anywhere. Baseline moved since rows 1-4 (then 2/8+1/15 best 264
diverged 140; now 0/8+0/15 best 239 diverged 0) from intervening landed
work (solver-01 trust cap diverged->0, ticket-09 overlap, ticket-08);
matching stage intact (SURVIVED per-pair values identical to row-3 era,
1-4 still 21.0). Pair 1-4 proposes but no longer seats at this HEAD —
cross-row read, not this ticket's variable.

Comment-cite LANDED in class/feature_matching.cpp (commit 23bd1f0 —
lead note: the worker-staged 14+/34- hunk was swept into the ticket-11
commit by a lead `git commit` that committed the whole index; message says
"no code touched" which is wrong for that hunk — see the commit question
below; content reviewed and accepted: upstream-uniform gate in both
overloads, acos clamps + INLIER DEBUG + contributing_segments counter
kept, ROBUSTNESS FIX adjectives replaced):
- overload 1: `// TICKET-12 ROW 5 2026-10-13: reverted to upstream uniform
  gate (was lineage "ROBUSTNESS FIX" adaptive, 869e696). Same-node A/B
  disable-vs-keep (jobs 32856411 vs 32858323, bm065): byte-identical on
  all measures ...` (full text in the staged hunk).
- overload 2: `// TICKET-12 ROW 5 2026-10-13: upstream uniform gate
  (adaptive removed; same-node A/B jobs 32856411 vs 32858323, bm065,
  byte-identical -- see first CountInlier overload for the full cite).`

Hygiene: OFF-arm trap ran from the run dir so its `git checkout --`
restoring feature_matching.cpp failed in-job (fatal: not a git
repository); restored manually post-run — Spartan checkout verified
clean (`git status --porcelain -- class/` empty, adaptive marker back).
data_path.h on Spartan now at committed POT_A (ticket-08's uncommitted
JUGLET residue reverted pre-pull by design; ticket-08 scripts re-apply
JUGLET in-job as needed). My run artifacts left on Spartan (untracked):
run_row5_*.sbatch, e2e_pota_row5*.log, slurm-*.out, build_sif/
Hierarchy-Clear-row5keep/-row5off, results_2026_10_13_1826/1841.
Caveat (same as rows 1-4/6): Juglet impact unmeasured. Next row per
ticket order (row 7: priority-list scoring).

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
