# Ticket 350 — what the stage proposes before it prunes

**Status:** ready-for-agent
**Waits for:** nothing — ticket 349 landed 2026-11-14
**Branch:** to be created at start
**Worker:** unassigned
**Answers:** S1
**Depends on:** 349 (the census and its 51-of-180 gap), 348 (the measured contact set), 338 (the pruned candidate table)
**Blocks:** any change to the feature-matching thresholds or to `PairwisePruning`
**Budget:** small (4 failed acceptance runs)
**Hand-over:** DeepSeek: one print-only switch in `hpc/`, four arms on Spartan.

> **In one line.** Ticket 349 could not tell whether 6-7 was never offered or thrown away by the
> pruning step: print every candidate before pruning, pair it with the pruned list, and say which
> step loses the joins.

## Why this is the next question

Ticket 349 censused the 51 **survivors** of `PairwisePruning`. Its own trace shows the count before
that step: `Total number : 180` (`hpc/tools/main_pristine_batch.cpp:310`) against `Total number
pruned : 51` (`:316`). Of the 14 measured joins, 13 reach the pruned list and only 4 arrive inside
the 5 deg / 5 mm bar; **6-7 reaches it not at all**, on the strongest contact in the set (66 dots
over 39.0 mm, zone median 0.344 mm). Whether that pair is lost in the 129 rows the pruning step
removes, or was never formed by feature matching, decides which code is the lever — and the two
answers send the work to different files.

## Part A — print the pre-pruning list

Add one switch, `SFS_PRE_TRACE` (exact string, default off; print a banner `pre trace: off|on` once,
near the other banners), and under it, immediately after `cout << "Total number : " << LCS_out.size()`
and **before** `PairwisePruning(shard, LCS_out)` (`hpc/tools/main_pristine_batch.cpp:310-315`), print
one line per `LCSIndex` in `LCS_out`:

```
[SFS-350] PRE <index_> <y> <x> <score_> <inliner_> <axis_raw_deg> <rot_deg> <mat_mm>
```

`rot_deg` and `mat_mm` are the same GT-conjugated placement errors the CAND338 block already
computes (`:322-356`); reuse that arithmetic in a small helper or duplicate it inside the new block.
Print `[SFS-350] PRETOTAL <n>` after the loop. Nothing else changes: no threshold, no pruning rule,
no order, no `TOP_k` / `BRANCH_b`, no line in `class/` at all. The print lives in `hpc/tools/`, where
reading GT is already the established practice; `class/` stays GT-free.

## Part B — pair the two lists

For each of the 14 measured joins and the two controls (4-7, 4-6), report: number of PRE rows, number
of post-pruning CAND338 rows, the best PRE rotation error, the best post-pruning rotation error,
whether the best PRE row survived (match on `(y, x, score_, inliner_)`, never on `index_` — it is
run-local), and its errors. Then, specifically:

- **6-7** — how many PRE rows, and does any survive?
- **5-8 (7.00 deg / 4.97 mm), 1-9 (8.82 / 3.48), 3-8 (11.00 / 4.53)** — the near misses: did a
  better-rotation hypothesis exist before pruning and not survive? Quote the row.
- the whole-run totals: PRE count, post count, how many pairs appear in PRE at all, and how many PRE
  rows are inside the bar.

A per-pair table, numbers only, is the deliverable; put it in `C:/PR/artifacts/t350/prepost.tsv`
with the header `pair\tn_pre\tn_post\tbest_pre_rot\tbest_pre_mm\tbest_post_rot\tbest_post_mm\tbest_survived\tnote`.

## Part C — the reading, exactly one

- `pruning-throws-away-the-join` — for 6-7 or for a near miss, a PRE row inside the bar (or clearly
  better than the survivor) does not survive: the pruning criterion is the lever, quote it.
- `the-join-is-never-formed` — 6-7 has no PRE row at all: feature matching never forms the pair; the
  lever is upstream, and the ticket quotes the pair's contact against the matcher's demand.
- `pruning-is-not-the-cause` — PRE and post agree on the best row for every pair and all PRE rows for
  the failing pairs are equally wrong: pruning changes the count, not the answer.
- `pre-trace-inconclusive` — the two lists cannot be paired (duplicate or rewritten keys), or the
  print cannot see the pre-pruning list; say what would have to be printed instead.

## Part D — arms and acceptance

- [ ] arms: `pre_on_r1`, `pre_on_r2` (signed 5/3, everything else unset: no `SFS_EDGE_TRACE`, no
  `SFS_STAGE_TRACE`, no `SFS_PICK_TRACE`, no `SFS_WIN_TRACE`, no `SFS_ADMIT_TRACE`), `pre_unset` (all
  switches unset, must print no `[SFS-350]` line), and `pre_pota` (the Pot A control);
- [ ] `tree_md5` before and after on every arm, and the signed accuracy quoted per arm (sherds and
  edges) without being the decision;
- [ ] `sacct` State/ExitCode/Elapsed for every job id, recorded in the ticket;
- [ ] the PRE count on each trace-on arm equals the `Total number :` line of the same log, and
  `PRETOTAL` equals it;
- [ ] `git diff --stat` shows `hpc/tools/main_pristine_batch.cpp` and this ticket only — nothing in
  `class/`, no threshold, no gate, no order, no `SFS_TOPK` / `SFS_BRANCH` / `SFS_STATE_TRACE`;
- [ ] no `[SFS-336]` … `[SFS-349]` token is re-implemented or re-printed by the new switch;
- [ ] one reading from Part C, with the rows quoted;
- [ ] the write-back ends with exactly one `**End state:**` line.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading it.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- Stage from `/data/gpfs/projects/punim2657/SfSpp/stage345/` (the current templates), export
  `SOURCE_COMMIT=<this ticket's claim commit>`, private tree per arm with md5 before and after,
  `sbatch` only, poll with `bash C:/PR/scripts/slurm_poll.sh <JOBID>`, final `sacct` recorded.
- Report, do not conclude: the table, the totals, the one reading slug.
- If the same failure happens twice, stop and report `incomplete`, naming the blocker.