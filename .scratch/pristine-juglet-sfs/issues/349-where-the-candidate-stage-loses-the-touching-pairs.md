# Ticket 349 — where the candidate stage loses the touching pairs

**Status:** ready-for-agent
**Waits for:** nothing — ticket 348 landed 2026-11-14
**Branch:** to be created at start
**Worker:** unassigned
**Answers:** S1
**Depends on:** 347 (the lines run along the contact), 348 (the measured set), 338 (the candidate table), 326 (the probe at the true pose)
**Blocks:** any change to the axis pipeline, the inlier gate, or the breakline extraction
**Budget:** medium (one conditional run of the candidate stage, 4 failed checks)
**Hand-over:** DeepSeek: offline census first; the run in Part B only if Part A leaves a pair with evidence and no candidate.

> **In one line.** The generated line does lie along the true contact, and most touching pairs
> still never present a candidate inside the bar — find, pair by pair, which numbers of the
> candidate stage are wrong, and whether our own axis estimate is one of them.

## Why this is the next question

The lane has now ruled out the things that are *not* the ceiling:

- the **line** — 347: on the pairs whose meshes meet, the interior line runs along the contact
  (zone median 0.216-0.675 mm, runs 11.4-39.0 mm on nine of them);
- the **order** — 346: the priority list follows its key correctly on every pair it can compare;
- the **window** — 339: both failing joins sit past `BRANCH_b`, and widening it already failed once (337);
- the **gate** — 341: the gate refuses with a number that is over its bar, not by accident;
- the **winner** — 342: the winning state does not carry the failing pairs at all.

What is left is the stage that produces the candidates. Read against ticket 348's measured set (14
joins), one signed run's 51 candidates give: **13 of the 14 joins have at least one candidate and
only 4 are inside the 5 deg / 5 mm bar** — 1-8 (0.70 / 1.21), 5-7 (4.71 / 3.69), 1-2 (2.14 / 2.20),
6-9 (2.35 / 1.72). **6-7 has no candidate at all** although its lines coincide over 39 mm at
0.344 mm. Just outside the bar: 5-8 (7.00 / 4.97), 1-9 (8.82 / 3.48), 3-8 (11.00 / 4.53), 5-6
(13.00 / 15.64), 2-9 (15.12 / 8.15), 1-4 (18.57 / 11.79), 2-6 (26.21 / 19.15), 1-3 (46.21 / 39.39),
3-5 (48.31 / 44.00). So even a perfect search seats at most **4 of the 14** joins from this stage.

## Premise check against ticket 348 (2026-11-14)

Ticket 348 landed the measured contact set, so the population this ticket censuses is fixed: 14
joins — `1-2 1-3 1-4 1-8 1-9 2-6 2-9 3-5 3-8 5-6 5-7 5-8 6-7 6-9`. Three consequences for the
census below:

- the earlier framing against ticket 326's ten touching pairs is retired — 2-5 and 7-9 are not
  joins (they touch without facing), 3-7 stands 13.85 mm away, so their candidate numbers say
  nothing about the ceiling;
- **6-7** is the one measured join with no candidate at all, and it is now the sharpest case in the
  census: the lines coincide over 39 mm at 0.344 mm and the stage produces nothing;
- two control rows belong in the table: **4-7** (in-bar candidate 3.35 / 2.68 and not a join) and
  **4-6** (a key edge the meshes put 7.68 mm apart, whose only candidate is 155.17 deg / 88.64 mm,
  yet which the winning state carries as edge #77 — so the winner's edge did not come from this
  candidate stage at all).

## Part A — the census, pair by pair

For each of the 14 measured joins from ticket 348, plus the two controls 4-7 and 4-6, assemble one
row from data that already exists:

- **what the geometry offers** at the true pose: the run of dots within 2 mm and its length in mm,
  the median gap, and the normal agreement along it (347's `pairs347.tsv`, `mating347.tsv`);
- **what the candidate stage produced**: every `[SFS-338] CAND338` row for that pair in the five
  trace-on logs (id, score, inlier count, rotation error, translation error), and whether it was
  inside the 5 deg / 5 mm bar;
- **what the axis files say**: the per-piece axis the tree uses (`Dataset/Axes/Juglet`) against the
  key's `Dataset/Juglet_GT/Juglet_Piece_N_Axis.xyz` — angular difference in degrees per piece;
- **what the inlier gate saw**: for a pair with a candidate, the inlier count against the run
  length Part A measured; for a pair with none, whether the stage rejected every pose it tried.

`SfSpp_preprocessing/AGENTS.md:137-146` warns that `Dataset/Axes/Juglet` is **our own MATLAB
PotSAC output of 2026-09-16, not a reference**, and that the key is the `_Axis.xyz` file — so an
axis error is a candidate explanation, and the numbers above are how we find out whether it is
the one.

## Part B — the axis arm, only if Part A points at it

If Part A shows a pair with enough evidence (run >= 7 dots, median gap <= 1.0 mm) whose candidate
is far outside the bar, run the candidate stage once on a private input tree in which the nine
`Dataset/Axes/Juglet` files are replaced by the key's `_Axis.xyz` lines, all trace switches
**off**. Change **nothing else** — no threshold, no gate, no `TOP_k`, no `BRANCH_b` — and record:

- the same 51-row candidate table, with the same fields;
- the per-pair error against the bar, side by side with the stock run;
- the signed-sweep accuracy (sherds and edges), which must be quoted but is not this ticket's
  decision;
- `tree_md5` before and after on the private tree, so the axes are the only difference.

If Part A does not point at the axis, say so and skip Part B — `axis-not-implicated` is a result.

## Part C — the reading, exactly one

- `axis-error-drives-the-candidates` — with the key's axes the far pairs come inside the bar
  (quote the before/after per pair): the lever is the axis pipeline, not the matcher.
- `evidence-below-the-matcher-demand` — the pairs that fail have runs shorter than the matcher
  needs (quote dots and mm): the lever is the demand per unit of contact, and **not** the line.
- `candidate-stage-never-tries` — a pair with a long run and good normals produces no candidate
  and no refusal that names it: the lever is where the stage gives up, and the trace for that is
  the next ticket.
- `two-causes` — different pairs fail for different reasons; name each pair's cause in the table
  and split the follow-up accordingly.
- `census-inconclusive` — the existing logs cannot separate the causes for at least half the
  pairs; name what would have to be printed and stop.

## Part D — acceptance

- [ ] all 14 measured joins and the two controls (4-7, 4-6) have a Part A row, with the source of
  each column named;
- [ ] the candidate rows are quoted from the logs by pair (not by id) and matched on
  (y, x, score, inliers), since ids are run-local;
- [ ] the axis comparison is in degrees per piece, from the two files, in the 1x frame;
- [ ] Part B, if run, changes only the nine axis files on a private tree, and `tree_md5` proves it;
- [ ] the signed-sweep accuracy of the Part B run is recorded with its sacct line;
- [ ] one reading from Part C, with the rows quoted;
- [ ] artefacts under `C:/PR/artifacts/t349/`;
- [ ] `git diff --stat` shows only this ticket (plus any ticket 348 requires);
- [ ] nothing in `class/` or `hpc/` is changed by this ticket.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading it.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- Heavy work on a Spartan compute node, submitted `sbatch` from a staged script, polled with
  `bash C:/PR/scripts/slurm_poll.sh <JOBID>`, final `sacct` State/ExitCode recorded.
- Report, do not conclude: numbers, the table, the one reading slug.
- If the same failure happens twice, stop and report incomplete, naming the blocker.
