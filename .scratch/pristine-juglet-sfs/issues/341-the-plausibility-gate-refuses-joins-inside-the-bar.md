# Ticket 341 — the plausibility gate refuses joins that are inside the bar

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 341-the-plausibility-gate-refuses-joins-inside-the-bar, in /c/PR/worktrees/structure-from-sherds-pp/341
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 340 (which candidate is applied), 339 (the veto reasons are already printed), 338 (the candidate table)
**Blocks:** any change to the plausibility gate or its bars
**Budget:** small (4 failed acceptance runs)
**Hand-over:** DeepSeek: numbers behind an existing `fail_reason`; the hidden check is the veto table in Part B.

> **In one line.** Three pairs whose best candidate is inside the 5 deg / 5 mm bar are offered at
> `j = 0`, tried, and refused eight, seven and five times by `CheckGraphPlausibility` — the gate says
> no to joins the answer key says are nearly right. This ticket records the numbers the gate is using
> when it says no, and nothing else.

## What is already measured (do not re-measure)

`SFS_ADMIT_TRACE=1` (ticket 339) already prints `[SFS-339] FATE ... veto <reason> <value>` for every
refusal, with the `fail_reason` string. Read out of the five trace-on logs of the 339 run
(`C:/PR/artifacts/t339/logs/binary_juglet_signed_on_r{1,2,3,6,7}.log`), 45-48 vetoes per run:

| pair | best candidate (CAND338) | in bar? | offers | minimum j | refusals | reasons |
|------|--------------------------|---------|--------|-----------|----------|---------|
| 5-7 | id 160, 4.71 deg / 3.69 mm, inlier 72 | yes (4.71 < 5) | 10, 8, 10, 10, 10 | 0 | 8, 6, 8, 8, 8 | `PC_` every time |
| 4-7 | id 145, 3.35 deg / 2.68 mm, inlier 28 | yes | 22, 20, 20, 20, 20 | 0 | 7, 7, 6, 6, 6 | `PC_` every time |
| 6-9 | id 170, 2.35 deg / 1.72 mm, inlier 18 | yes | 10, 14, 9, 9, 9 | 0 | 3, 5, 3, 3, 3 | `Overlap_73.70`, `Overlap_76.06`, `Overlap_72.87` |

Across a whole run the reason census is `PC_` 30-34 of 45-48 refusals, the rest `Overlap_<n>_` with
one `Score_` in r2. So the gate is not refusing a rare case: it is refusing most attempts, and three
of the refusals it makes are on pairs whose best candidate is inside the bar.

Seam 1-2 is the other side of the same coin: it *is* kept (4 keeps in r1, 2 in r2) and its final pair
still reads 84.40 deg / 93.99 mm (338). Ticket 340 is what says whether the kept edge is the in-bar
candidate.

### Premise check against ticket 340 (2026-11-14)

340 landed and sharpens this ticket in one important way: `CheckGraphPlausibility` is **not** a blanket
refusal. Each of the three pairs here is *accepted once* at step 0 — `PICK 0 1 0 5 7 160 0.322249 72 72`,
`PICK 0 2 2 4 7 144 0.559114 36 36`, `PICK 4 25 1 6 9 66 0.520905 28 28` — and refused on most of its
later offers, in states where other pieces have already moved. So the question this ticket must answer is
no longer "why does the gate say no to a good join" but **"what do the numbers look like when the same
join is refused later and accepted at step 0"**: a PC count that collapses as pieces move is pose drift,
not a bar that is too tight, and reading 1 below must not be taken until Part B shows the numbers at both
moments. Add to Part B one column: the same pair's step-0 accept (from 340's table) beside the refusal.

## Part A — the instrument

Extend the existing veto print; do not add a new switch and do not re-implement the reason string.
Inside `CheckGraphPlausibility` (`class/ranking_system.cpp:2211-2292`), at the point the reason is
set, add the numbers that produced the verdict:

```
[SFS-341] VETO <step> <state> <j> <y> <x> <reason> <value_a> <value_b> <value_c>
```

- `PC_` — the point-cloud inlier count that failed, the distance bar it was compared against, and
  the rim-gap value (report `-1` for a value the code does not compute at that site; do not invent
  one).
- `Overlap_<n>_` — `<n>` as printed, the bar, and the number of edges in the chunk.
- `Score_` — `graph_score_`, the bar, and the chunk's `inlier` sum.

Gated by `SFS_VETO_TRACE=1` (exact string, default off, banner `veto trace: on|off`). Additions only;
no threshold, gate, `TOP_k`, `BRANCH_b` or ordering change; GT stays out of `class/`.

## Part B — the table (the deliverable)

One row per distinct refusal of the three pairs above, per trace-on repeat:

| pair | repeat | step/state/j | reason | value_a | value_b | value_c | the candidate's own error (CAND338) | reading |
|------|--------|--------------|--------|---------|---------|---------|------------------------------------|---------|

Reading per pair, exactly one of: `refused-while-in-bar` (the numbers show the gate rejecting a pair
whose candidate is inside the bar), `refused-on-its-own-data` (the PC inlier count is near zero or
the rim gap is empty, so the edge's own point data is weak and no bar change would help),
`refused-on-pose` (the edge's applied pose is not the candidate's — hand this to 340's finding).

## Part C — arms

As 339 and 340: 3x tree, `SFS_TOPK`/`SFS_BRANCH` unset.

| arm | switches | repeats | pass |
|-----|----------|---------|------|
| signed 5/3 trace on | `SFS_VETO_TRACE=1` (+ `SFS_EDGE_TRACE=1` if 340's table needs it) | 2 | 2/9 sherds, 1/18 edges |
| signed 5/3 fidelity | all unset | 1 | the same 2/9 and 1/18, else the instrument is void |
| Pot A | unset | 1 | 8/8 and 15/15, else the ticket is void |

## Part D — readings (apply unchanged)

1. **`refused-while-in-bar`.** The gate's bar is refusing a join the answer key calls right, six to
   eight times, on three different pairs. The next ticket is a **bar experiment on the PC test**, and
   it must be bounded: one arm with that bar relaxed, signed 5/3 accuracy must rise above 2/9 sherds
   and 1/18 edges, and Pot A must stay 8/8 and 15/15. No other threshold moves in the same ticket.
2. **`refused-on-its-own-data`.** The pair's own point-cloud evidence is weak at that moment. No gate
   change is justified; the next ticket is where that evidence is computed, not the bar.
3. **`refused-on-pose`.** The edge the gate sees is not the candidate the table lists. Say so and
   stop: 340 must explain the gap first.
4. Instrument void if the fidelity arm differs from 2/9 and 1/18, or Pot A is not 8/8 and 15/15.

## Acceptance

- [ ] `veto trace: on`/`off` in the respective arms; zero `[SFS-341]` lines with the switch off.
- [ ] Additions only in `class/`; no threshold, gate, `TOP_k`, `BRANCH_b` or ordering change.
- [ ] Part B filled for 5-7, 4-7 and 6-9 in both trace-on repeats, with the three numbers per refusal.
- [ ] Signed 5/3 2/9 and 1/18 in the trace-on arms and the fidelity arm; Pot A 8/8 and 15/15.
- [ ] `sacct` State and ExitCode recorded for every job id.
- [ ] `## Comments (worker, 2026-11-14)` section appended, ending with exactly one `**End state:**` line.

**Report, do not conclude.** No gate is changed in this ticket.
## Comments (worker, 2026-11-14)

**Second pass — the lead's rework is in this section; the first pass' numbers are gone.** Part A now prints the
number the bar is actually compared against (the failing profile distance, captured inside `ProfileChecking`),
and a companion `EDGE` line names the edge the refusal is about. All four arms were re-run on the new build.
Here is what that changed.

### What changed in the code

One switch, `SFS_VETO_TRACE=1` (default off, banner `veto trace: on`/`veto trace: off`), extends the existing
veto print at the point the reason string is set:

```
[SFS-341] VETO <step> <state> <j> <y> <x> <reason> <value_a> <value_b> <value_c>
[SFS-341] EDGE <step> <state> <j> <y> <x> <index_> <score_> <inliner_>
```

`y` prints before `x`. `step` is the gate's `step_counter`; `state` is `state.trace_id_`; `j` is the parameter
named `graph_index` in the gate's signature, which `BuildState` fills with `order_index`. For `PC_`: value_a =
the largest profile-point deviation in the bin that failed, value_b = the bar that refused (7.0 profile
distance, or 10.0 when the rim-gap margin refused), value_c = the failing rim gap, -1 when no rim test ran. For
`Overlap_<n>_`: value_a = n, value_b = 50, value_c = number of edges in the chunk. For `Score_`: graph_score_,
0, the chunk's inlier sum. The `EDGE` line resolves the chunk's edge exactly as `[SFS-340] PICK` does.

Sites: `class/feature_matching.h:29-30`; `class/feature_matching.cpp:66,:70,:103,:131,:1005,:1050`;
`class/ranking_system.cpp:654-712` (helper), `:1655` (state capture), `:2482` (Score_), `:2506` (Overlap_),
`:2529-2530,:2553,:2558,:2567` (rim gap), `:2576-2584` (PC_). **106 insertions, 0 deletions**, four files
(three source + this ticket); no threshold, gate, TOP_k, BRANCH_b, ordering or container touched. Source
snapshot `src341.tar.gz` md5 `c134ce6c630d36134080158e8ee4052d`, diff md5
`0ea1ffc54aa09ef867b57d7a9f927428` (234 lines), both in `/data/gpfs/projects/punim2657/SfSpp/stage341`.

### Runs (sacct)

Run dir `/data/gpfs/projects/punim2657/SfSpp/runs/20261114-0253_f111c74_t341-veto` (the 0226 dir holds the
superseded first build).

| job | what | node | State | ExitCode | wall | sherd/edge | VETO/EDGE | banner |
|---|---|---|---|---|---|---|---|---|
| 35026928 | compile pre-check | spartan-bm134 | COMPLETED | 0:0 | 00:00:37 | — | — | — |
| 35026977_0 | signed 5/3 trace on r1 | spartan-bm071 | COMPLETED | 0:0 | 00:04:32 | 2/9, 1/18 | 45/45 | on |
| 35026977_1 | signed 5/3 trace on r2 | spartan-bm084 | COMPLETED | 0:0 | 00:10:56 | 2/9, 1/18 | 45/45 | on |
| 35026977_2 | signed 5/3 all switches unset | spartan-bm084 | COMPLETED | 0:0 | 00:10:03 | 2/9, 1/18 | 0/0 | off |
| 35026980 | Pot A default, unset | spartan-bm085 | COMPLETED | 0:0 | 00:02:59 | 8/8, 15/15 | 0/0 | off |

Fidelity holds (2/9, 1/18 with every switch unset), Pot A reads 8/8 and 15/15 and owned its own
`$RUN/pota_work`, tree md5 before == after in all four arms (55/55/55/49 files), no
[SFS-336/338/339/340/342] token in any arm. `acc_rc=1` on the signed arms is `checkacc333.py`'s own exit for an
imperfect assembly, not a job failure.

### Part B — the three pairs

Candidate evidence (step-0 `CAND338` table of the 339 r1 log): 5-7 = id 160, .322249, inlier 72, 4.71 deg /
3.69 mm; 4-7 = id 144, .559114, 36, 18.69/14.86 (out of bar) and id 145, .359451, 28, 3.35/2.68 (in); 6-9 = id
170, .343843, 18, 2.35/1.72 (in). Step-0 accepts quoted from ticket 340: `PICK 0 1 0 5 7 160 0.322249 72 72`,
`PICK 0 2 2 4 7 144 0.559114 36 36`, `PICK 4 25 1 6 9 66 0.520905 28 28`.

| pair | step/state/j | reason | a | b | c | applied edge (index_, score, inlier) | judged = candidate? | slug |
|---|---|---|---|---|---|---|---|---|
| 5-7 | 1/7/0 | PC_ | 8.8562/8.8592 | 7 | -1 | 126, .35974, 77 | no | refused-on-pose |
| 5-7 | 2/13/0 | PC_ | 8.8572/8.8589 | 7 | -1 | 108, .35974, 77 | no | refused-on-pose |
| 5-7 | 2/14/1 | PC_ | 8.8578/8.8565 | 7 | -1 | 100, .35974, 77 | no | refused-on-pose |
| 5-7 | 2/15/1 | PC_ | 8.8581/8.8597 | 7 | -1 | 100, .35974, 77 | no | refused-on-pose |
| 5-7 | 3/20/0 | PC_ | 8.8585/8.8575 | 7 | -1 | 93, .35974, 77 | no | refused-on-pose |
| 5-7 | 3/21/0 | PC_ | 8.8592/8.8584 | 7 | -1 | 90, .35974, 77 | no | refused-on-pose |
| 5-7 | 3/22/0 | PC_ | 8.8588/8.8582 | 7 | -1 | 58, .35974, 77 | no | refused-on-pose |
| 5-7 | 3/23/0 | PC_ | 8.8582/8.8568 | 7 | -1 | 90, .35974, 77 | no | refused-on-pose |
| 4-7 | 3/21/2 | PC_ | 11.2219/11.2715 | 7 | -1 | 78, .361115, 30 | no | refused-on-pose |
| 4-7 | 3/22/2 | PC_ | 11.2430/11.2714 | 7 | -1 | 46, .361115, 30 | no | refused-on-pose |
| 4-7 | 4/26/2 | PC_ | 9.4624/9.4613 | 7 | -1 | 80, .508306/.508312, 23 | no | refused-on-pose |
| 4-7 | 4/27/2 | PC_ | 7.0333/7.0357 | 7 | -1 | 86, .517556/.517547, 21 | no | refused-on-pose |
| 4-7 | 5/28/0 | PC_ | 7.9562/7.9565 | 7 | -1 | 38, .373922, 24 | no | refused-on-pose |
| 4-7 | 5/29/2 | PC_ | 7.5915/7.5917 | 7 | -1 | 78, .429171, 18 | no | refused-on-pose |
| 6-9 | 1/8/0 | Overlap_73.702413_ | 73.7024 | 50 | 2 | 156, .758858, 36 | no | refused-on-pose |
| 6-9 | 2/16/2 | Overlap_76.063163_ | 76.0632 | 50 | 1 | 123, .660523, 44 | no | refused-on-pose |
| 6-9 | 3/20/1 | Overlap_76.063163_ | 76.0632 | 50 | 1 | 101, .660523, 44 | no | refused-on-pose |

The `a` column is r1/r2 where the two repeats differ; reasons, ids and inliers are identical across repeats.
Full table, instrument and checks: `C:/PR/artifacts/t341/attribution.md`.

### Slugs and readings

**5-7 — refused-on-pose.** The gate refuses an edge with score .35974 and inlier 77 at distances 8.856-8.860
against the 7.0 bar, every time; the measured candidate is 160 (.322249/72), which is a different edge. 160 is
applied once, at step 0, where the gate skips the profile test entirely (`if (step_counter > 0)`,
`class/ranking_system.cpp:2531`) — so the accept certifies the picker, not the shape. **This reverses the first
pass' reading of 5-7** (which said refused-while-in-bar): the old reading had only the candidate table, the
`EDGE` line now shows the judged edge is a regenerated 5-7 edge, not 160.

**4-7 — refused-on-pose.** Six refusals on edges .361115/30, .508306/23, .517547/21, .373922/24, .429171/18;
neither 144 (.559114/36) nor 145 (.359451/28) is among them. The only 4-7 join ever applied is 144, whose own
pose (18.69 deg / 14.86 mm) is outside the bar; 145, the in-bar candidate, is never applied. Distances
7.033-11.271 vs 7.0 — the closest call in the table is 4/27 at 0.5% over; everything else is 9-61% over. Hands
to 340: why prefer 144 (score .559114, 18.69 deg) over 145 (score .359451, 3.35 deg)?

**6-9 — refused-on-pose.** Three `Overlap_` refusals, 73.70/76.06/76.06 against the 50 bar, on chunks with 2,
1 and 1 edges; the judged edge is 156 (.758858/36) then 123/101 (.660523/44), not candidate 170 (.343843/18),
and the step-4 accept uses 66 (.520905/28), which has no `CAND338` row. Hands to 340: why 156/123/101 rather
than 170, and what is 66?

No pair reads **refused-while-in-bar** and none reads **refused-on-its-own-data**. Consequence: no bounded bar
experiment is justified by this evidence — the in-bar candidate is never the edge the gate judges. The premise
'inside the bar, still refused' is true of the candidate table and not of the refusal. 340 must explain the
edge substitution (and the step-0 skip) before any bar moves.

### Instrument checks

Banners on/on/off/off; 31 `PC_` + 14 `Overlap_` + 0 `Score_` per trace-on repeat; 0 lines with the switch off
and in Pot A. The `EDGE` triple is the edge `PICK` would print, checked against the 339 r1 log at the same
step: step 1 5-7 `RCAND ... 126 5 7 0.35974` with `PRIO ... 77 ... 126 5 7 0.35974`, and step 1 6-9
`RCAND ... 156 6 9 0.758858`. Id equality with `CAND338` is not expected and not used: `index_` is renumbered
per build (`iter->index_ = LCS_out.size()`, `class/feature_matching.cpp:1645/:1715`; `RegistrationPruning`
renumbers per graph — the 339 log prints id 80 twice at step 4 with different scores), and at step 1 the 5-7
candidates are 125-128, so 160 is gone. Identity rests on (score, inlier). `Overlap_` value_a equals the number
in the reason string; repeat scatter is solver nondeterminism (value_a <= 0.05, scores <= 9e-6).

### Caveats

- No log measures the judged edges' pose error, so 'judged = candidate?' is answered by identity, not by a
  measured out-of-bar pose.
- `CAND338` exists only for the initial build (51 rows, printed once at log lines 601-651); later candidates
  have no ground-truth deg/mm.
- The step-0 accept is not a shape test, so 'accepted once, refused later' is not pose drift of one edge.
- The `Overlap_` number is a max over the refused chunk's pairs and the chunk can hold a second pair (the 339
  log prints a 2-9 row beside one 6-9 refusal), so 73.70/76.06 is not attributable to the 6-9 edge alone.
- One VETO line per distinct pair in the chunk: 45 lines exceed the number of gate refusals.
- `Score_` never fired here; that site is instrumented but unexercised.
- The 70-insertion first build (run dir 0226) printed `graph_score_` as `PC_` value_a; superseded.

### Lead — verification (2026-11-14, Thinkbook16)

Read the diff, the code around the gate, both trace-on logs and all four acceptance files myself. The numbers are
right and the conclusion the worker drew from them is half right: the numbers are settled, the identity sentence
is not.

The instrument is print-only: `git diff HEAD -U0 -- class` = 106 added lines, 0 deleted, in `class/feature_matching.cpp`
(+18), `class/feature_matching.h` (+2) and `class/ranking_system.cpp` (+86); every new statement sits under
`g_sfs_veto_trace` except the one static write that records the failing distance (cleared at
`class/feature_matching.cpp:1005`, captured at `:1050` inside `if (dist > threshold)`).

The step-0 finding is real and it is the strongest thing in the ticket: the whole profile-and-rim block is inside
`if (step_counter > 0)` at `class/ranking_system.cpp:2531`, so a step-0 accept is never shape-tested; the overlap
test sits outside that guard (`:2495-2517`) and does run at step 0. 'Accepted at step 0, refused later' is
therefore not one edge drifting out of tolerance, it is a join that was never measured.

Refusal census, identical in both trace-on repeats: 45 lines = 31 `PC_` + 14 `Overlap_`. 5-7: eight `PC_` at
8.8565-8.8597 mm against the 7.0 bar (27% over), all on an edge of score .35974 / inlier 77. 4-7: six `PC_` at
7.0333-11.2715 (the closest, 4/27, half a percent over). 6-9: three `Overlap_` at 73.7024, 76.0632, 76.0632
against the 50 bar (47-52% over). The remaining `Overlap_` lines (79.6244, 50.0942 x2) belong to other pairs in
the same chunks.

Where I part company: `verified:` the `CAND338` table carries exactly four rows for these three pairs - 160 for
5-7, 144 and 145 for 4-7, 170 for 6-9 - so the judged tuple .35974/77 has no candidate-stage row. That does not
settle 'a different edge'. Two explanations fit and this run cannot separate them: the score is refreshed on an
edge that survives (`class/feature_matching.cpp:1819` `iter->score_ = cycle.score;`, and `lcs.score_ = 11.0`
assigned at `class/reconstruction.cpp:1487,1556,1571,1680,1765,1779`), or a 5-7 edge is regenerated from a
merged boundary. Both mean the gate is judging a number that is not the candidate-stage measurement - which is
the useful finding - and neither lets the ticket say 'no pair is refused while carrying its in-bar candidate' as
a fact. Written down as: the three refusals are on profile distance and overlap of the pair; whether the edge
judged is the table's candidate is unresolved, and the tuple mismatch is not proof.

Consequence, corrected: one bounded bar arm is still justified, for 5-7 only - its margin is 1.9 mm over a 7.0 mm
bar and it is already accepted once at step 0. 6-9's overlap is 47-52% over and 4-7's applied join is already
outside the bar, so neither is a bar question: 4-7's is the picker's choice (144 over 145), 6-9's is where id 66
came from. Ticket 344 carries the 5-7 arm.

verified: `git diff HEAD -U0 -- class` -> 106 inserted, 0 deleted, three files.
verified: `sed -n '2495,2545p' class/ranking_system.cpp` -> overlap test outside the step guard, profile and rim
inside `if (step_counter > 0)` at `:2531`.
verified: reason census on `artifacts/t341/logs/binary_juglet_signed_on_r{1,2}.log` -> 31 `PC_` + 14 `Overlap_`
each; 5-7 distances 8.8565-8.8597; 4-7 7.0333-11.2715; 6-9 73.7024/76.0632/76.0632.
verified: five `[SFS-341] EDGE` lines for 5-7 -> index 58-126, score .35974, inlier 77 every time.
verified: `awk` on the 339 r1 log -> four `CAND338` rows for these pairs (160, 144, 145, 170), none at .35974.
verified: `artifacts/t341/runs/acc341_*.txt` -> signed on r1/r2 and off r1 all SHERD=2/9 EDGE=1/18; Pot A 8/8
and 15/15; sacct 35026928 COMPLETED 0:0, 35026977_0/_1/_2 COMPLETED 0:0, 35026980 COMPLETED 0:0.
verified: tree md5 before == after in four arms (55/55/55/49 files); switch-off log carries no `[SFS-341]` line.
verified: `fig1_refusal_timeline.png` and `fig2_values_vs_bars.png` viewed - distance points sit above the 7.0
line for 5-7 and 4-7, and the overlap points above 50 for 6-9.

Attribution: `C:/PR/artifacts/t341/attribution.md` (201 lines) matches the logs on every row I re-read.
**End state:** ready: numbers recorded, the lead lands and closes
