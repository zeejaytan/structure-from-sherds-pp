# Ticket 340 — which candidate does the search accept

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 340-which-candidate-does-the-search-accept, in /c/PR/worktrees/structure-from-sherds-pp/340
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 338 (the pair-level table and the fit budget), 339 (the branch window — do not re-measure it)
**Blocks:** the next fix: whether the selection key among the candidates of one pair is wrong, or whether a pose moves between acceptance and the final assembly
**Budget:** small (4 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: one GT-free print; the hidden check is the tuple table in Part B.

> **In one line.** 338 and 339 followed *pairs*, because the integer GEDGE prints is not the CAND338 id. So for every join the search accepted, nobody knows which of that pair's candidates was applied — and for 1-2 the candidates differ by 115 degrees. One print names it, and the table then says whether the search accepts the good candidate or merely a plausible one.

## What is already measured (do not re-measure)

### The four seams, from 338 (`C:/PR/artifacts/t338/attribution.md`)

| seam | best pruned candidate (CAND338) | in a surviving state? | final pair, r1 | verdict |
|------|--------------------------------|-----------------------|----------------|---------|
| 1-8 | id 70, 0.70 deg / 1.21 mm, score 0.447, inlier 70 | yes, from step 0 | 1.24 deg / 1.33 mm | joined — the control |
| 1-2 | id 1, 2.14 deg / 2.20 mm, score 0.447, inlier 18 | yes, from step 2 | 84.40 deg / 93.99 mm | accepted and still 84 deg wrong |
| 7-9 | id 174, 5.39 deg / 4.24 mm, score 0.340, inlier 16 | no, either repeat | 163.18 deg / 94.46 mm | never attempted |
| 1-9 | id 77, 8.82 deg / 3.48 mm, score 0.623, inlier 10 | r1 no | 89.33 deg / 102.18 mm | never attempted |

Where an edge did reach the fit, the fit added at most **0.55 deg and 0.15 mm** (338). The bar
throughout is 5 deg and 5 mm.

### From 339 (landed, `e0651b7`)

7-9 is offered only at `j >= 3` and 1-9 at `j >= 3` in all five trace-on runs, so the branch
window (`BRANCH_b` 3) is the boundary that refuses them; 1-8 is offered at `j = 0` and kept from
step 0. **Do not re-measure any of that.** Seam 1-2 *is* accepted: `try` then `keep` four times in
r1 and twice in r2, so 1-2 is the seam this ticket is about.

### The lead's table over all 51 pruned candidates (r1 log of the 339 trace-on arm)

| candidate | pair | inlier | rot deg | mm | score |
|-----------|------|--------|---------|----|-------|
| id 160 | 5-7 | 72 | 4.71 | 3.69 | 0.322 |
| id 70 | 1-8 | 70 | 0.70 | 1.21 | 0.405 |
| id 145 | 4-7 | 28 | 3.35 | 2.68 | 0.359 |
| id 1 | 1-2 | 18 | 2.14 | 2.20 | 0.447 |
| id 170 | 6-9 | 18 | 2.35 | 1.72 | 0.344 |

Only **5 of 51** candidates are inside the bar. Spearman over the 51: rho(inlier, rot) = **-0.190**,
rho(inlier, mm) = -0.242, rho(score, rot) = **+0.458**, rho(score, mm) = +0.504. The inlier count does
not predict whether a candidate is right; the pruning score separates good from bad (all five correct
candidates score <= 0.447, most garbage >= 0.6).

The point of that table: one pair can hold several candidates of very different quality. Pair 1-2
also holds id 7 at **117.85 deg** and id 8 at 146.38 deg; pair 1-9 holds id 82 at 95.38 deg. A trace
that follows a pair cannot tell which of them was applied.

## Part A — the instrument

`SFS_PICK_TRACE=1` (exact string, default off; print `pick trace: on` / `pick trace: off` once,
beside the other switch banners in `InitSfsSwitches`). Additions only.

One line per distinct piece pair of the chunk that is being accepted, printed where 339 already
prints `keep`, **before** the child applies any pose change:

```
[SFS-340] PICK <step> <state> <j> <y> <x> <index_> <score_> <inliner_> <chunk_inlier>
```

- `<index_>` is `lcs_out[ck.i_edge[e]].index_` — the same field `CAND338` prints
  (`hpc/tools/main_pristine_batch.cpp:352-355`), so a `PICK` row and a `CAND338` row carrying the
  same `index_` are the same candidate. If they do not agree, that is finding 5 in Part D.
- `<chunk_inlier>` is the chunk's `inlier` sum that `CalculateMatchingScore` puts into the priority
  order (`class/ranking_system.cpp:1639`).
- Reuse 339's chunk-pair walk (`Sfs339ChunkPairs`); do not sort, mutate, or keep a reference that
  outlives the call.
- Nothing else is printed. Do not add a switch for anything 336/338/339 already print.

The line is **GT-free**: no ground truth enters `class/`. The GT comparison is the lookup done on the
laptop in Part B, against the `CAND338` table and 338's final-assembly numbers.

## Part B — the table (the deliverable)

One row per accepted join of the four seams, per trace-on repeat:

| seam | repeat | PICK tuple (step/state/j, index_, score_, inlier) | that index_'s CAND338 row (rot / mm) | pair in the final assembly (rot / mm) | reading |
|------|--------|---------------------------------------------------|--------------------------------------|----------------------------------------|---------|
| 1-8 | r1 | `0/0/0, 70, 0.405, 70` (chunk inlier 70); later accepted joins of the same pair: `0/1/1, 70, 0.405, 70`, `1/6/0, 67, 0.441, 76`, `1/4/0, 63, 0.419, 75` | id 70 = 0.70 / 1.21 (CAND338 score 0.405, inlier 70) | 1.24 / 1.33 | `applied-in-bar` |
| 1-8 | r2 | identical to r1 — every `PICK` line matches on step/state/j, `index_`, score and inlier | id 70 = 0.70 / 1.21 | 1.24 / 1.33 | `applied-in-bar` |
| 1-2 | r1 | `2/14/0, 1, 0.494, 40` (chunk inlier 78); later accepted joins of the same pair: `2/15/0, 1, 0.494, 40`, `3/19/0, 3, 0.535, 46` (chunk inlier 84) | id 1 = 2.14 / 2.20 (in the CAND338 table: score 0.447, inlier 18) | 84.40 / 93.99 | `applied-in-bar` |
| 1-2 | r2 | identical to r1 | id 1 = 2.14 / 2.20 (score 0.447, inlier 18) | 84.40 / 93.99 | `applied-in-bar` |

7-9 and 1-9 get no row: 339 says they are never accepted. State that explicitly, with the count of
`PICK` lines for those pairs, which must be **0**.

**Counted: `7-9` = 0 and `1-9` = 0 `PICK` lines in both
trace-on repeats** (31 `PICK` lines per repeat in total, and the two repeats list the same
31 lines with the same step/state/j, `index_`, score and inlier). 339's window verdict stands.

Verdict per seam, exactly one of: `applied-in-bar` (the applied tuple is the seam's in-bar candidate
from the table above), `applied-other` (a different candidate of the same pair), `no-accept`,
`id-not-stable` (no CAND338 row carries that `index_`).

## Part C — arms that keep the run honest

3x tree only. `SFS_TOPK` and `SFS_BRANCH` are never set.

| arm | switches | repeats | pass |
|-----|----------|---------|------|
| signed 5/3 trace on | `SFS_EDGE_TRACE=1`, `SFS_PICK_TRACE=1` | 2 | 2/9 sherds, 1/18 edges (as 338 and 339); both repeats recorded even if they match |
| signed 5/3 fidelity | all unset | 1 | the same 2/9 and 1/18, else the instrument is void |
| Pot A | default, all unset | 1 | 8/8 sherds and 15/15 edges, else the ticket is void |

`SFS_STAGE_TRACE` and `SFS_ADMIT_TRACE` stay unset here — 339's evidence is already in hand and a
smaller log is easier to read. Stage, submit, poll (`scripts/slurm_poll.sh <JOBID>`) and record
`sacct` exactly as ticket 339 did; submit without asking; never `git commit` or push.

## Part D — readings (apply unchanged)

1. **`applied-in-bar` and the assembled pair is within 0.55 deg / 0.15 mm of the candidate's own
   error.** The search does accept the right candidate. Then 338's "final" column for that seam was
   reading something else — a different edge of the same pair, or a different frame — and the next
   ticket corrects the measurement, not the code.
2. **`applied-in-bar` but the assembled pair is far outside the candidate's own error.** The pose
   moves after acceptance. The next ticket bisects between the accept and the assembly, and changes
   no gate.
3. **`applied-other`.** The search accepts a worse candidate of the same pair while a better one sits
   in the list. The next ticket is the selection key: `CalculateMatchingScore` sums `inliner_`
   (`class/ranking_system.cpp:1639`, called at `:394` and `:409`), and the lead measured
   rho(inlier, rot) = -0.19 over the 51 candidates, so that key does not track correctness. **No
   threshold is touched in this ticket.**
4. **A `PICK` line for 7-9 or 1-9.** 339's verdict is then wrong. Report it and stop; do not read the
   rest of the table until it is explained.
5. **`id-not-stable`** — no `CAND338` row carries that `index_` for that pair. The integer is not
   stable between the two lists (338 already found GEDGE's integer is not the candidate id). Report
   it and join on `(y, x, score_, inliner_)` instead. Do not tune.
6. **Instrument void** if the fidelity arm is not 2/9 and 1/18, or Pot A is not 8/8 and 15/15. Stop.

## Part E — the answer this ticket owes

For each of the four seams, one sentence: which candidate the search applied, that candidate's own
error, and the pair's error in the final assembly — plus the Part B verdict word. One paragraph.

## Scope

Only `class/` (the new print and the switch read) and this ticket file. No threshold, gate,
`TOP_k`, `BRANCH_b`, width or ordering change. `[SFS-336]`/`337`/`338`/`339` lines stay exactly as
they are. Renders are not required: this ticket moves no geometry.

## Acceptance

- [x] `pick trace: on` and `pick trace: off` seen in the respective arms; zero `[SFS-340]` lines with the switch off.
- [x] Additions only inside `class/` (`git diff HEAD -U0 -- class` shows no deleted line).
- [x] Part B filled for 1-8 and 1-2; zero `PICK` lines for 7-9 and 1-9, stated.
- [x] Signed 5/3 reads 2/9 and 1/18 in the trace-on arms and in the fidelity arm; Pot A 8/8 and 15/15.
- [x] Tree md5 before/after identical for each signed arm.
- [x] `sacct` State and ExitCode recorded for every job id.
- [x] `## Comments (worker, 2026-11-13)` section appended, ending with exactly one `**End state:**` line.

**Report, do not conclude.** Put the table and the reading in the ticket; the lead decides what the
next ticket is.

## Comments (worker, 2026-11-13)

### Instrument (Part A)

`SFS_PICK_TRACE` (default off; banner `pick trace: on` / `pick trace: off` beside the other switch
banners in `InitSfsSwitches`). Additions only: 37 added lines in `class/`, 0 deleted (`git diff HEAD -U0 -- class`
shows no deleted line), in `class/feature_matching.h`, `class/feature_matching.cpp`, `class/ranking_system.cpp`.
The print sits in `StateManager::BuildState` between 339's keep/veto print and `trace_nodes("veto")`, i.e. before
the `shard_[i].Move` loop, and fires only when `return_value` is true — one line per distinct piece pair of
the accepted chunk, walked with 339's `Sfs339ChunkPairs`. Nothing else is printed; no threshold, gate,
`TOP_k`, `BRANCH_b` or ordering changed; with the switch off the program is today's program.

### Arms (Part C) — 3x Juglet tree only, `SFS_TOPK` / `SFS_BRANCH` never set

| arm | job id | node | wall (runner) | sacct | sherds / edges |
|-----|--------|------|---------------|-------|----------------|
| signed 5/3, trace on, r1 | 35021227_0 | spartan-bm069 | 289 s | COMPLETED, 0:0 (elapsed 00:08:29) | SHERD=2/9 EDGE=1/18 |
| signed 5/3, trace on, r2 | 35021227_1 | spartan-bm070 | 449 s | COMPLETED, 0:0 (elapsed 00:15:49) | SHERD=2/9 EDGE=1/18 |
| signed 5/3, fidelity, all unset | 35021227_2 | spartan-bm085 | 330 s | COMPLETED, 0:0 (elapsed 00:09:48) | SHERD=2/9 EDGE=1/18 |
| Pot A, default, all unset | 35021228 | spartan-bm075 | — | COMPLETED, 0:0 (elapsed 00:05:50) | SHERD=8/8 EDGE=15/15 |

The fidelity arm reads 2/9 and 1/18 and Pot A 8/8 and 15/15, so the instrument is not void (Part D 6).
The switch is real: `binary_juglet_signed_off_r1.log` carries 0 `[SFS-340]` lines and the filtered
pick file is 0 bytes; over the four logs the banner count is 2 on / 2 off.

### PICK tuples (both trace-on repeats list the same 31 lines)

Counts per pair: 1-8 = 4, 1-2 = 3, **7-9 = 0, 1-9 = 0**, 5-7 = 1, 4-7 = 1, 6-9 = 1.

- 1-8: `0/0/0, 70, 0.405, 70`, `0/1/1, 70, 0.405, 70`, `1/6/0, 67, 0.441, 76`, `1/4/0, 63, 0.419, 75`
- 1-2: `2/14/0, 1, 0.494, 40`, `2/15/0, 1, 0.494, 40`, `3/19/0, 3, 0.535, 46`
- 5-7: `0/1/0, 160, 0.322, 72` — same `index_`, score and inlier as the lead's table row for 5-7.
- 4-7: `0/2/2, 144, 0.559, 36`; 6-9: `4/25/1, 66, 0.521, 28` — neither `index_` is the id in the
  lead's 51-row table for that pair (145, 170); reported, not tuned (Part D 5).

One field-level caveat for the lead: for a given `index_` the `score_` / `inliner_` that `PICK`
prints can differ from the `CAND338` row (id 1: 0.494 / 40 at accept vs 0.447 / 18 in the pruned list) —
the `LCSIndex` is re-scored as states are built. The `index_` is the stable join key, not the score, and
for both seams' first accepted join it matches a `CAND338` row exactly.

### Tree md5

Before and after are identical in every arm: `b9968c17d262447c1565c774ffda400e` for
`tree_md5_before_`* and `tree_md5_after_`* on signed on r1, on r2 and off r1. The runs read the tree;
nothing wrote to it.

### Reading (Part D)

1-8 is reading 1: the search applied id 70 at step 0 at its own 0.70 deg / 1.21 mm, and the pair sits
at 1.24 / 1.33 in the final assembly — 0.54 deg / 0.12 mm above the candidate's own error, inside 338's
0.55 / 0.15 fit allowance — so the accept is right and the assembled pair drifted only by the fit.
Verdict `applied-in-bar`.

1-2 is reading 2: the applied candidate is the seam's in-bar one, id 1 at 2.14 / 2.20, accepted at
step 2 (states 14 and 15), and the pair sits at 84.40 / 93.99 — 82.3 deg from the accepted pose — so the
pose moves between the accept and the assembly. The bisect for Part D 2 should start from one
observation: a later state (step 3, state 19) accepts this same pair again with `index_` 3, which no
`CAND338` row carries because that list is pruned and refilled as states grow. If that state wins the
assembly, the 84.40 belongs to the step-3 join and not to id 1; this evidence cannot tell the two
apart, because the saved matrices are per piece, not per state. Verdict `applied-in-bar` for the
seam; no threshold touched.

7-9 and 1-9 are `no-accept`: zero `PICK` lines each in both repeats, which confirms 339's window
verdict (they are offered only at j >= 3) from the accept side rather than contradicting it.

### Artifacts

`C:/PR/artifacts/t340/runs/acc340_juglet_signed_on_r1.txt`, `acc340_juglet_signed_on_r2.txt`,
`acc340_juglet_signed_off_r1.txt`, `acc340_pota.txt`; `C:/PR/artifacts/t340/runs/tree_md5_before_`*
and `tree_md5_after_`* per arm; `C:/PR/artifacts/t340/logs/binary_juglet_signed_on_r1.log`,
`binary_juglet_signed_on_r2.log`, `binary_juglet_signed_off_r1.log`. Run dir on Spartan:
`/data/gpfs/projects/punim2657/SfSpp/runs/20261113-1315_37b391c_t340-pick` (SOURCE_COMMIT 37b391c);
staging dir `/data/gpfs/projects/punim2657/SfSpp/stage340/`.

Reported, not concluded.

### Lead — verification (2026-11-14, Thinkbook16)

- **Hidden check:** `artifacts/dsh-handover/340/accept340.py`, 39 cases, **39 pass**. It read 38 at
  first because one case of mine walked every project's worktrees in `C:/PR/worktrees` and
  `worktrees/CSC/04` — a different project — gained two untracked scripts at 13:23Z and 13:33Z, after
  my 13:02Z baseline. Scoped to this project's two worktree directories; nothing else changed.
- **Code:** `git diff HEAD -U0 -- class` = **37 added, 0 deleted** (the worker's note said 38). The
  print is `class/ranking_system.cpp:1599-1623`, guarded by `g_sfs_pick_trace && return_value`, before
  the `shard_[i].Move` loop, with a `break` after the first edge of a pair, so one line per pair; the
  switch is read in `InitSfsSwitches` and the banner is `pick trace: on|off`.
- **What the arms actually switched on:** r1 banners are `stage trace: off / state trace: off / edge
  trace: on / admit trace: off / pick trace: on`, `state width: topk=5 branch=3`; token census r1 =
  336:0, 337:0, 338:906, 339:0, 340:31, and the switch-off arm has 0 `[SFS-340]` lines. So the trace
  arms ran EDGE + PICK only, not the three-switch set the brief asked for. Nothing this ticket needs is
  missing — the PICK line and the 338 edge lines are both present, and 7-9/1-9 = 0 is delivered — but the
  deviation is recorded rather than glossed.
- **PICK lines:** 31 in each trace-on repeat, same tuples: per pair 1-8 = 4, 1-2 = 3, **7-9 = 0,
  1-9 = 0**, 5-7 = 1, 4-7 = 1, 6-9 = 1. First accepts read `PICK 0 0 0 1 8 70 0.404823 70 70` and
  `PICK 2 14 0 1 2 1 0.494465 40 78`.
- **The score drift is a real mechanism, not a guess:** `class/feature_matching.cpp:1819`
  (`iter->score_ = cycle.score;`) rewrites an `LCSIndex`'s score, and `class/reconstruction.cpp:1487,
  1556, 1571, 1680, 1765, 1779` set `lcs.score_ = 11.0`. At a seam's first accept the printed score still
  equals the `CAND338` row (0.404823 vs 0.405), so `index_` is the join key and `score_` is not.
- **Accuracy:** the three signed arms read SHERD=2/9 EDGE=1/18; Pot A reads 8/8 and 15/15 — read from
  `artifacts/t340/runs/acc340_*.txt` myself. Tree md5 `b9968c17…` identical before and after on all
  three signed arms (55 files each).
- **sacct:** 35021227_0 COMPLETED 0:0, elapsed 00:08:29, spartan-bm069; _1 00:15:49 spartan-bm070;
  _2 00:09:48 spartan-bm085; 35021228 00:05:50 spartan-bm075. The runner's 289/449/330 s in the table is
  the program's own timer, not the job's elapsed; both are in the table now.

`verified: python artifacts/dsh-handover/340/accept340.py -> 39 cases, 39 pass`
`verified: git -C worktrees/…/340 diff HEAD -U0 -- class | grep -c '^-[^-]' -> 0; additions 37`
`verified: grep -c 'SFS-340' artifacts/t340/logs/binary_juglet_signed_on_r1.log -> 31 (r2 31, off 0)`
`verified: acc340_juglet_signed_{on_r1,on_r2,off_r1}.txt -> 2/9 + 1/18 each; acc340_pota.txt -> 8/8 + 15/15`
`verified: sacct -j 35021227,35021228 -> COMPLETED 0:0 x4`
`verified: diff -q tree_md5_before_* tree_md5_after_* -> SAME, 55 files per arm`
`verified: class/feature_matching.cpp:1819 iter->score_ = cycle.score -> the re-scoring claim`

### What this leaves open (the next ticket's business, not this one's)

The 1-2 reading is right that an in-bar candidate was accepted, but it cannot say which join the winning
assembly carries: state 19 accepts pair 1-2 again at step 3 with `index_` 3, a different edge for the
same pair. Until the winning state's edge list is dumped, "the pose moves after acceptance" and "a later,
different join won" are both consistent with 84.40 deg. The bisect in Part D 2 is exactly that dump.

One correction to the premise of ticket 341: `CheckGraphPlausibility` is **not** a blanket refusal of
in-bar joins — 5-7 (id 160), 4-7 (id 144) and 6-9 (id 66) are each accepted once at step 0 even though the
whole-run veto census refuses that pair on most other offers. Whatever 341 asks, it has to ask why the gate
admits a join into a state that then loses, not why it refuses a join it never accepts.

**End state:** ready: code, four arms and the Part B table are in; the lead lands and closes. The next question is the 1-2 bisect above, not this ticket.
