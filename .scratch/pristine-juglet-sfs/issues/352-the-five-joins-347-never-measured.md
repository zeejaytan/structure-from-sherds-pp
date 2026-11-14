# Ticket 352 — the five joins ticket 347 never measured

**Status:** ready-for-agent
**Waits for:** nothing — ticket 349 landed 2026-11-14
**Branch:** to be created at start
**Worker:** unassigned
**Answers:** S1
**Depends on:** 347 (the contact-stretch measurement), 348 (the measured contact set)
**Blocks:** any statement about why 1-3, 2-6, 3-8, 5-6 or 5-8 produce a wrong candidate
**Budget:** small (2 failed checks)
**Hand-over:** DeepSeek: one offline measurement, the same reader as ticket 347, five extra pairs.

> **In one line.** Ticket 347 measured the line run for 20 pairs and ticket 348 found seven joins it
> had not measured; fill the five gaps so the candidate census has a line number for every join.

## Why this is needed

Ticket 349's census has `na` in the run columns for **1-3, 2-6, 3-8, 5-6 and 5-8** — five of the 14
measured joins. `C:/PR/artifacts/t347/pairs347.tsv` holds 20 pairs (the 18 graph edges plus 5-7 and
4-7); ticket 348's measured set added 1-3, 2-6, 3-8, 5-6, 5-8 and 6-9 (6-9 was already in the 18). So
for those five pairs the census can say the candidate is wrong but not whether the geometry offered
a run long enough to find it — and one of them, **5-8, misses the bar by 2.0 deg of spin with the
translation already inside** (7.00 deg / 4.97 mm).

## Part A — measure the five pairs

Run ticket 347's own reader (`C:/PR/artifacts/t347/measure347.py`, or its 1x table code) on the five
missing pairs at the true pose in the 1x frame, with the same definitions it used: Bl0 dots to the
other mesh's surface, zone = dots within 3.0 mm of the other mesh, point-to-SEGMENT distances, run =
the longest consecutive set of dots within 2.0 mm, reported in dots and millimetres, plus the zone
median and RMS and the whole-curve median. Do not change a definition to make a pair look better;
if a definition does not apply, say so.

Output: `C:/PR/artifacts/t352/pairs352.tsv`, same columns as `pairs347.tsv`, five rows, plus the
script and its `logs/`.

## Part B — say what it means for the census

For each of the five, put the run beside the candidate error from ticket 349's census:

| pair | run dots | run mm | zone median | candidate rot / mm | verdict |
|---|---|---|---|---|---|

with `verdict` one of `run-meets-the-demand`, `run-short-of-the-demand` (quote the matcher's demand:
7 dots over about 12.6 mm), `no-run`. Then one sentence per pair in the write-up.

## Part C — the reading, exactly one

- `gaps-are-short-runs` — the five pairs that produce wrong candidates also have runs below the
  matcher's demand: the line is the limit for them, and ticket 347's picture extends.
- `gaps-are-long-runs` — the runs meet the demand and the candidates are still wrong: the limit is
  the pair stage, not the line, and the near miss is a spin problem.
- `gaps-are-mixed` — name which pairs fall each way.
- `measurement-inconclusive` — the reader cannot produce the numbers for at least three of the five;
  name what is missing.

## Part D — acceptance

- [ ] all five pairs measured, in the 1x frame, with 347's definitions, one script;
- [ ] the table has the same columns as `pairs347.tsv`, and the five rows are quotable;
- [ ] the 1x frame is checked (the 3x tree's numbers must not be used as millimetres);
- [ ] Part B's table filled with numbers, no adjectives;
- [ ] one reading from Part C;
- [ ] artefacts under `C:/PR/artifacts/t352/`;
- [ ] `git diff --stat` shows this ticket only — no `class/`, no `hpc/`, no threshold.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading it.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- The meshes and poses are on Spartan; heavy work on a compute node, one `sbatch`, polled with
  `bash C:/PR/scripts/slurm_poll.sh <JOBID>`, final `sacct` recorded.
- Report, do not conclude: the table, the verdicts, the one reading slug.
- If the same failure happens twice, stop and report `incomplete`, naming the blocker.