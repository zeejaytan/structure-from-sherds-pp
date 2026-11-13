# Ticket 340 — which candidate does the search accept

**Status:** ready-for-agent
**Working in:** chat on Thinkbook16 (2026-11-13)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`; work only in the worktree it names
**Answers:** S1
**Depends on:** 338 (the pair-level table and the fit budget), 339 (the branch window — do not re-measure it)
**Blocks:** the next fix: whether the selection key among the candidates of one pair is wrong, or whether a pose moves between acceptance and the final assembly
**Worker:** unassigned
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
| 1-8 | r1 | | | | |
| 1-8 | r2 | | | | |
| 1-2 | r1 | | | | |
| 1-2 | r2 | | | | |

7-9 and 1-9 get no row: 339 says they are never accepted. State that explicitly, with the count of
`PICK` lines for those pairs, which must be **0**.

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

- [ ] `pick trace: on` and `pick trace: off` seen in the respective arms; zero `[SFS-340]` lines with the switch off.
- [ ] Additions only inside `class/` (`git diff HEAD -U0 -- class` shows no deleted line).
- [ ] Part B filled for 1-8 and 1-2; zero `PICK` lines for 7-9 and 1-9, stated.
- [ ] Signed 5/3 reads 2/9 and 1/18 in the trace-on arms and in the fidelity arm; Pot A 8/8 and 15/15.
- [ ] Tree md5 before/after identical for each signed arm.
- [ ] `sacct` State and ExitCode recorded for every job id.
- [ ] `## Comments (worker, 2026-11-13)` section appended, ending with exactly one `**End state:**` line.

**Report, do not conclude.** Put the table and the reading in the ticket; the lead decides what the
next ticket is.
