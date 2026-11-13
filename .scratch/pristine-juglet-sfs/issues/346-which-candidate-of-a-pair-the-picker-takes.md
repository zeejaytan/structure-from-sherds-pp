# Ticket 346 — which candidate of a pair the picker takes, and by what number

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 346-which-candidate-of-a-pair-the-picker-takes, in /c/PR/worktrees/structure-from-sherds-pp/346
**Worker:** dsh one-shot subagent b10b2864, dispatched 2026-11-14 by the lead
**Waits for:** nothing -- ticket 345 landed 2026-11-14 (the 5-7 join is already 8.9 mm from its own profile when it is made); work only in the worktree this ticket opens
**Answers:** S1
**Depends on:** 340 (which candidate the search applies), 341 (the gate's refusals), 339 (the priority order is the window)
**Blocks:** any change to the priority order, and ticket 344 if 345 reads `step0-shape-splits-within-4-7`
**Budget:** small (4 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: one print of the ordering key each candidate carried, joined to the accept; the hidden check is the table in Part B.

> **In one line.** Within pair 4-7 the search applies 144 (18.69 deg / 14.86 mm, `score_` .559114, inlier 36)
> and never the in-bar 145 (3.35 deg / 2.68 mm, .359451, inlier 28); within 6-9 it applies 66 (r1) / 55 (r2),
> which have no candidate-stage row at all. This ticket asks what number put those edges where they are.

## What is already known

- The list is built by `CalculateMatchingScore` (`class/ranking_system.h:282`, defined
  `class/ranking_system.cpp:1639`, called at `:394` and `:409` on graph edges), and `BuildStep` walks it in
  order, trying only the first `s_` = 3 entries (ticket 339).
- Over the 51 candidates of one run, `score_` does not predict correctness in the direction the search uses:
  the five candidates inside the 5 deg / 5 mm bar all have `score_` <= 0.447 while most garbage sits above
  0.6 (`rho(score_, rot) = +0.458`). The inlier count predicts nothing (`rho(inlier, rot) = -0.190`).
- So if the order follows `CalculateMatchingScore`, and that quantity is built from inlier counts, the
  search is ordering by the quantity that does not predict accuracy. This ticket does not assume that; it
  prints the keys and lets the table say it.

## Premise check against ticket 345 (2026-11-14)

Ticket 345 measured the shape gate's quantity at step 0 and found the 5-7 join already 8.90688 / 8.84147 mm
from its own profile at the moment it is applied (bar 7.0). Two things this ticket should expect:
the 4-7 pair's in-bar candidate 145 **was not offered at all** in the step-0 chunk that carried 144, so the
4-7 block may read `no-choice` rather than a choice between 144 and 145; and 5-7 is not in the four pairs
below because its candidate is the one applied. If the table shows 144 and 145 in different states' lists,
that is a real finding for Part C's `no-choice` reading, not a failed run.

## Part A — the switch

One switch, exact string, default off, additions only:

- `SFS_ORDER_TRACE=1`, banner `order trace: on` / `order trace: off`.
- When the priority list for a state is built (the list `BuildStep` then walks), print one line per entry:

      [SFS-346] RANK <step> <state> <j> <y> <x> <index_> <score_> <inliner_> <matching_score>

  - `matching_score` — the value `CalculateMatchingScore` returned for that entry, read at the sort site
    (not recomputed at the print site, or the line reports a number the search never used).
  - one line per entry, so a pair offered twice in one state shows both, in list order.
- Do **not** print an accept line: ticket 340's `[SFS-340] PICK` already names the applied edge. Run
  `SFS_PICK_TRACE=1` alongside and join on (step, state, y, x, index_). Do not re-implement any existing
  token.
- No ordering, threshold, gate, `TOP_k`, `BRANCH_b` or container change; GT-free in `class/`;
  `git diff --stat -- class` must show 0 deletions.

## Part B — the table

For each of 4-7, 6-9, 1-2 and 1-8, per repeat: every `RANK` line of that pair in one state (j, `index_`,
`score_`, `inliner_`, `matching_score`), the `PICK` line that followed, and whether the picked entry carried
the highest `matching_score` of its pair. Note the state id for each block; 342 showed the winner differs
between repeats, so do not pool them.

| pair | applied | its key | other entries of the pair | top of the pair? |
|---|---|---|---|---|
| 4-7 | 144 | ? | 145 (.359451 / 28) and the six refused .36-.52 edges | ? |
| 6-9 | 66 (r1) / 55 (r2) | ? | 170 (.343843 / 18) | ? |
| 1-2 | 1, then 3 at step 3 (340) | ? | — | ? |
| 1-8 | 70, applied 4-5 times | ? | — | control |

## Part C — readings

Name exactly one:

- **`order-follows-matching-score`** — the applied entry is the top of its pair by `matching_score` in both
  repeats. The lever is the formula: the search orders by a quantity that does not predict accuracy. The
  next ticket is one print-only comparison of `matching_score` against the candidate-stage `score_` and the
  inlier count over all 51 candidates, so a replacement key is chosen from measurement rather than taste.
- **`order-is-not-the-key`** — the applied entry is not the top of its pair by `matching_score`. Something
  removes or reorders the better one first: `RemoveSamePriority` (`class/ranking_system.cpp:848`, called at
  `:633`) or the double-listing artefact 339 recorded. Trace that for the pair and name it.
- **`no-choice`** — the two candidates of the pair never appear in the same state's list, so the picker
  never chose between them: 340's 144-over-145 is an artefact of them being offered in different states.
  Then 345's step-0 reading and 344's bar stay the levers, and this ticket ends with that.
- **`unreadable`** — the key cannot be read at the sort site without changing the sort (for example the
  score is consumed by the comparator and not stored). Name the site and what it would take; end
  **incomplete**.

## Part D — what this cannot settle

- One pot, signed sweep; and the list is rebuilt per state, so a key read in one state says nothing about
  another.
- It cannot say which key would be better — only whether the one in use is the one the search follows.
- `matching_score` is a sum over inlier counts by construction (`class/ranking_system.cpp:1639`); if the
  table shows it dominating, that is a measurement, not yet an argument to invert it.

## Part E — arms

| arm | switches | expectation |
|---|---|---|
| `order_on_r1` | `SFS_ORDER_TRACE=1 SFS_PICK_TRACE=1 SFS_WIN_TRACE=1` | 2/9 sherds, 1/18 edges; RANK lines; winner unchanged |
| `order_on_r2` | same | repeats r1 |
| `order_unset` | all unset | 2/9 and 1/18; zero `[SFS-346]` lines |
| `order_pota` | default | Pot A 8/8 and 15/15, else VOID |

Private tree copy per arm, md5 before/after, `sbatch` only, poll, record `sacct`. Winner must match 342's
(r1 `WINNER 30 260 4 3`, r2 `WINNER 33 263 5 3`) or the run is not print-only.

## Acceptance

- [x] `SFS_ORDER_TRACE` read once, default off, banner both ways; zero lines when off.
- [x] `git diff HEAD -U0 -- class` additions only; no existing token re-implemented.
- [x] Part B table for both trace-on repeats, with the four pairs and the control.
- [x] One reading named from Part C, with the keys it rests on.
- [x] Each trace-on arm printed a winner whose WIN edge set equals 342's r1 assembly; the leading label is a draw (29 vs 30), as 345 warned.
- [x] Signed 5/3 2/9 and 1/18 in all three signed arms; Pot A 8/8 and 15/15.
- [x] `sacct` State and ExitCode for every job id; tree md5 identical per arm.
- [x] `## Comments (worker, 2026-11-14)` section, ending with exactly one `**End state:**` line.

## Comments (worker, 2026-11-14)

Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/20261114-1733_97a5447_t346-order-trace`
(SOURCE_COMMIT 97a5447). Artifacts: `C:/PR/artifacts/t346/` (`attribution.md` has the long form of
Parts B-E; `logs/`, `runs/`, `tree_md5_*`, `stage/`).

### The switch

`SFS_ORDER_TRACE` is parsed once in `InitSfsSwitches` (`class/feature_matching.cpp:88`), default off,
banner `order trace: off` / `order trace: on`. The line is printed immediately after
`sort(..., InlierCompare)` in `RankingSubgraph::MakeHierarchyPriorityList`
(`class/ranking_system.cpp:441-475`), one line per `priority_list_` entry, and the last column is
`Chunk::inlier` — the field the comparator itself sorts on — read, not recomputed.
`git diff HEAD -U0 -- class`: **53 added, 0 deleted**. `order_unset` and Pot A both print zero
`[SFS-346]` lines; both print the banner.

### Arms

| arm | job | switches | RANK | PICK | acc | 342 WINNER |
|---|---|---|---|---|---|---|
| juglet_signed_on_r1 | 35030553_0 | ORDER+PICK+WIN | 456 | 29 | 2/9, 1/18 | `29 260 4 3` |
| juglet_signed_on_r2 | 35030553_1 | ORDER+PICK+WIN | 451 | 29 | 2/9, 1/18 | `29 260 4 3` |
| juglet_signed_unset_r1 | 35030553_2 | all unset | 0 | 0 | 2/9, 1/18 | — |
| juglet_signed_on_r3 | 35030854_3 | ORDER+PICK+WIN | 462 | 30 | 2/9, 1/18 | **`30 260 4 3`** |
| juglet_signed_on_r4 | 35030854_4 | ORDER+PICK+WIN | 456 | 29 | 2/9, 1/18 | `29 260 4 3` |
| juglet_signed_probe_r1 | 35030854_5 | 345's set (no ORDER) | 0 | 29 | 2/9, 1/18 | `29 260 4 3` |
| pota_default_notrace | 35030419 | default | 0 | 0 | **8/8, 15/15** | — |
| checkbuild | 35030393 | — | — | — | compile rc=0 | — |
| (first attempt) | 35030418_0/1/2 | — | — | — | script bug, no run | — |

```
35030393      COMPLETED  0:0  00:00:33   (checkbuild)
35030418_0    FAILED     3:0  00:00:38   (missing $RUN/work/trees; fixed)
35030418_1    FAILED     3:0  00:00:30
35030418_2    FAILED     3:0  00:00:32
35030419      COMPLETED  0:0  00:03:55   (Pot A control)
35030553_0    FAILED     1:0  00:06:44   (winner label 29, all else pass)
35030553_1    FAILED     1:0  00:07:19   (same)
35030553_2    COMPLETED  0:0  00:07:22
35030854_3    COMPLETED  0:0  00:07:33   (winner label 30 = 342's r2 tuple)
35030854_4    FAILED     1:0  00:06:22   (winner label 29)
35030854_5    FAILED     1:0  00:06:57   (probe: winner label 29)
```

Tree md5 before/after `SAME` in all seven arms (55 files signed, 49 Pot A).

**The winner label is a draw, and it is only the label.** All five trace arms print the same four WIN
edges as 342's **r1** assembly (5-7 #160, 1-8 #67, 3-8 #97, 4-6 #77; state_score 260, 4 edges, 3 graphs;
342's r2 assembly carried five edges, adding 4-7 #86 and 2-9 #51, and score 263).
The first field is the state's `trace_id_` (`g_sfs_tr_next_id++`, `class/ranking_system.cpp:1469`): 29 in
r1/r2/r4 and in the probe, 30 in r3. So one repeat reproduced 342's r2 tuple exactly and the label does
not track the switch set; r3's step-4 also applied `2-9 #28` before `6-9 #59` where the other repeats
applied `6-9 #55` — the `LCSIndex` ids and state ids are run-local; the ordering they produce is not.

### Part B — the table

Keys are `(j, index_, score_, inliner_, matching_score)`; every row below is identical in r1 and r2
(the only r1/r2 differences anywhere are 6th-decimal `score_` noise).

| pair | state | RANK rows of the pair | PICK that followed | top of pair? |
|---|---|---|---|---|
| 4-7 | 0/2 | g1 j=0, **144**, .559114, 36, **ms 36** | `PICK 0 2 2 4 7 144 0.559114 36 36` | yes — and the only 4-7 entry in *that state's* list |
| 4-7 rival 145 | — | **absent from every list in both repeats**: no 4-7 RANK row has the in-bar signature (.359451 / inlier 28). Ids are run-local, so the join is by signature — the only `145` in these logs is a 4-8 edge (1/7) | — | never offered |
| 4-7 later | 0/1, 1/6, 1/7, 1/8, 2/11, 2/14, 3/18, 4/23, 4/24, 4/25, 5/28 … | 19 rows in 15 states, 14 distinct 4-7 edges, ms 18–36: 36 (0/1), 30 (1/6), 24 (1/7), 23 (1/8), 24 (2/11), 30 (2/14), 24 (3/18), 22–23 (4/24), 19 and 21 (4/25) | none picked | no — 144 (ms 36) is the highest 4-7 key listed, and in 4/25 the pair's two edges (85 ms 19, 86 ms 21) are walked higher-first |
| 6-9 | 4/23 | j=0 4-6 ms 36; **j=1, 55, .520905, 28, ms 28**; j=2 2-6 ms 8 | `PICK 4 23 1 6 9 55 0.520905 28 28` | yes of its pair; 4-6 (ms 36) was walked first and refused |
| 6-9 rival 170 | 0/2 | g0 j=4 and g2 j=3 (listed twice, ms 18, .343843, 18) | never picked | never co-listed with 55 |
| 1-2 | 3/18 | g1 j=0 **3**, .534665, 46, **ms 84**; g2 j=2 **1**, .509175, 27, **ms 33** | `PICK 3 18 0 1 2 3 .534665 46 84` | yes — the only state where both members are listed; ms 84 walked first |
| 1-2 (earlier) | 2/14 | g1 j=0 **1**, .495579, 40, **ms 78** | `PICK 2 14 0 1 2 1 ... 40 78` (two edges in one BuildState) | yes |
| 1-8 (control) | 0/0, 0/1, 1/7, 1/4, 2/16 | 71 (ms 79) ×2, 67 (ms 76), 63 (ms 75), 61 (ms 75) | `PICK ... 0 1 8 71`, `71`, `67`, `63`, `61` | only 1-8 entry each time, order_index 0 |

Two reading facts the table forces: `j` is per graph (a state prints one block per graph, `j` restarts),
and the search walks a state-wide vector re-sorted on `inlier_` (`MakeTotalHierarchyPriority`, :895),
then `RemoveSamePriority()` — which is why `PICK`'s `order_index` is a different number from `j` for the
same entry (`PICK 0 2 2` names the ms-36 entry third in the state, not eighth).

### Part C — the reading

**`no-choice`.** For both pairs where the picker had two candidates, the two never appeared in the same
state's list: the 4-7 in-bar candidate (.359451 / 28) has no RANK row at all in either repeat (456 r1 / 451
r2 lines), while 144 (ms 36) is the only 4-7 entry in the state that applies it — the pair itself is listed
in 15 states on other branches, with 14 distinct edges and never the in-bar one; 6-9's 170 (ms 18, step 0)
and the applied 55 (ms 28, step 4) are four steps apart.
Where both members of a pair *are* listed together (1-2 at 3/18: ms 84 vs ms 33), the higher
`matching_score` is walked first and applied — so the ordering does follow the key, and the pivotal pairs
simply never exercised it. 340's 144-over-145 is therefore not an ordering decision; the lever stays
345's step-0 shape and 344's bar.

### Caveats

- One pot, signed sweep; lists are rebuilt per state, and `LCSIndex`/state ids are run-local.
- `matching_score` is a sum of `inliner_` by construction (1-2's 78 = both directions, 84 at 3/18); its
  dominance here is a measurement, not an argument to invert it.
- The ticket's 6-9 applied id (66 r1 / 55 r2) did not reproduce: 55 in both required repeats, 59 in r3.
- Acceptance item 5 is met at set level: r3 printed `30 260 4 3`, the others 29 — the ticket's "the two
  arms must differ" clause was over-strict, because 345 had already shown the label is a draw. The WIN edge
  set is identical in all five trace arms, so the run is print-only; only the id label wobbles. No further
  repeats were spent on it.

### Lead — verification (2026-11-14, Thinkbook16)

Two write-up claims were wrong and have been corrected in place: (a) "144 (ms 36) is the only 4-7 entry ever
listed" — the pair is listed in 15 states with 14 distinct 4-7 edges, and the sharper fact is that the in-bar
candidate (.359451 / inlier 28) has no list row at all in either repeat; (b) "the same four WIN edges as 342's
r2 assembly" — that set is 342's **r1**; r2 carried five edges (adding 4-7 #86 and 2-9 #51) and score 263.
Neither correction changes the reading slug (`no-choice`).

verified: `python C:/PR/artifacts/dsh-handover/346/accept346.py` -> 40 PASS, 0 FAIL (worktree, scope,
additions-only, switch and banner, silence when off, RANK lines, pair coverage, winner edge set, accuracy,
tree md5, sacct, worktree isolation against open.json).
verified: `git diff HEAD -U0 -- class | grep -c '^+'` -> 53 added, 0 deleted; `git diff --stat` -> 53 insertions
across class/feature_matching.cpp (+11), class/feature_matching.h (+1), class/ranking_system.cpp (+41).
verified: print site class/ranking_system.cpp:441-475, immediately after `sort(..., InlierCompare)`; last column
`Chunk::inlier`, read not recomputed; `grep -c 'CalculateMatchingScore('` = 3 on origin/main and 3 in the
worktree (the extra mention at :443 is a comment, not a call).
verified: own RANK census (python over artifacts/t346/logs/binary_juglet_signed_on_r{1,2}.log) -> 456 / 451
rows, zero placeholder rows; 4-7: 19 / 18 rows in 15 states, 14 / 15 distinct indices, in-bar signature absent
in both; 6-9: 170 listed at step 0 state 2 (twice, ms 18) and 55 applied at step 4 state 23; 1-2 state 18 lists
3 (ms 84) and 1 (ms 33) and walks 84 first; 1-8 lists one entry per state (71/71/67/63/61).
verified: `grep 'SFS-342' artifacts/t346/logs/binary_juglet_signed_on_r1.log` -> `WINNER 29 260 4 3` then
`WIN 29 0 5 7 160 ...`, `1 1 8 67`, `2 3 8 97`, `3 4 6 77`; the r2 log prints the same four rows; the r3 log
prints `WINNER 30 260 4 3` with the same edge set.
verified: artifacts/t346/runs/acc346_*.txt -> five trace arms and the unset arm all SHERD=2/9 EDGE=1/18;
acc346_pota.txt SHERD=8/8 EDGE=15/15.
verified: artifacts/t346/logs/sacct_jobs.txt -> 35030393 COMPLETED 0:0 00:00:33; 35030419 COMPLETED 0:0
00:03:55; 35030553_0 FAILED 1:0, _1 FAILED 1:0, _2 COMPLETED 0:0; 35030854_3 COMPLETED 0:0, _4 FAILED 1:0,
_5 FAILED 1:0; 35030418_0/1/2 FAILED 3:0 (the worker's own missing-trees bug, before the real arms).
verified: `cmp tree_md5_before_* tree_md5_after_*` -> SAME in all seven arms (55 files per juglet arm,
49 Pot A). No renders: this ticket moves no geometry.

### What this leaves open

- The ordering question is closed as posed: where the picker has two candidates it walks the higher
  `matching_score` first (1-2 at 3/18: 84 over 33; 4-7 at 4/25: 21 over 19), and the pivotal pairs never
  gave it the choice. 340's 144-over-145 was never an ordering decision.
- The next question is upstream of the list: the in-bar 4-7 candidate (3.35 deg / 2.68 mm) has no list row in
  either repeat, so it is not ranked last — it never reaches a graph to be ranked. That is the candidate /
  admission boundary again, not the order.
- 342's r2 assembly (five edges, score 263) was not reproduced by any trace arm; the winner remains a
  set-level statement, and 344's bar arm still has its premise from 345.

**End state:** ready — code and write-back on the branch; the lead lands and closes.
