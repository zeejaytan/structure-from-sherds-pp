# Ticket 349 — where the candidate stage loses the touching pairs

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 349-where-the-candidate-stage-loses-the-touching-pairs, in /c/PR/worktrees/structure-from-sherds-pp/349
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Waits for:** nothing — ticket 348 landed 2026-11-14
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

**Premise note (lead, 2026-11-14).** The reference run's tree already carries the key's axis
directions — 0.000° on all nine pieces (`class/data_path.h:1719` and `:1746-1754`, `stage333.py:110`) —
so the substitution this Part describes is a no-op on that tree and Part B was correctly not run.
Measuring our own estimate is the inverse direction (write `Dataset/Axes/Juglet` into the pinned
tree); that is the next ticket's question, not this one's.

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

- [x] all 14 measured joins and the two controls (4-7, 4-6) have a Part A row, with the source of
  each column named;
- [x] the candidate rows are quoted from the logs by pair (not by id) and matched on
  (y, x, score, inliers), since ids are run-local;
- [x] the axis comparison is in degrees per piece, from the two files, in the 1x frame;
- [x] Part B, if run, changes only the nine axis files on a private tree, and `tree_md5` proves it;
- [x] the signed-sweep accuracy of the Part B run is recorded with its sacct line;
- [x] one reading from Part C, with the rows quoted;
- [x] artefacts under `C:/PR/artifacts/t349/`;
- [x] `git diff --stat` shows only this ticket (plus any ticket 348 requires);
- [x] nothing in `class/` or `hpc/` is changed by this ticket.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading it.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- Heavy work on a Spartan compute node, submitted `sbatch` from a staged script, polled with
  `bash C:/PR/scripts/slurm_poll.sh <JOBID>`, final `sacct` State/ExitCode recorded.
- Report, do not conclude: numbers, the table, the one reading slug.
- If the same failure happens twice, stop and report incomplete, naming the blocker.

## Comments (worker, 2026-11-14)

Part A only. No assembler run, no new geometry, no threshold, gate, priority or file under `class/`
or `hpc/` touched. Deliverables: `C:/PR/artifacts/t349/census349.py`, `census349.tsv` (16 rows),
`census349.md`, and `logs/` (the 51 raw candidate rows, the 27 axis lines, the per-pair JSON, the
sacct line, the command lines).

**The axis premise does not hold for the reference run.** The candidate numbers this ticket quotes
come from job 35016164 (t339 arm `juglet_signed_on_r1`, run
`20261113-1023_c1aa93e_t339-admit`). That arm binds its private tree at `/Dataset/SfS_pp` and
compiles with `-DJUGLET_ORIG`, so `class/data_path.h` (line 6608-6647) reads
`/Dataset/SfS_pp/Axes/Juglet_Piece_N_Axis.xyz` — and that tree is the 3x bundle whose build script
(`.scratch/pristine-juglet-sfs/334/t333work/stage333.py`) copies
`Dataset/Juglet_large/...` with the note "GT axes, already 3x". The files are the key's directions
with the key point ×3 (`0.345475597391 0.356039490652 0.868264068532` on both sides, piece 1), and
the run's own `[SFS-336] AXIS` trace prints exactly those rows: tree-vs-key **0.000°** for all nine
pieces (0.000-0.035° from the printed matrices). The ticket's 8-of-9 figure is
`Dataset/Axes/Juglet` (1x, 2026-09-29), a directory this run never opens: 1.5°, 87.8°, 31.8°,
10.7°, 44.5°, 13.3°, 24.1°, 12.8°, 73.3° per piece. Both columns are in `census349.md` §1/§4.

**Part B was therefore not run.** Replacing the tree's nine axis files with the key's lines would
replace the key's directions with the same directions — a no-op that cannot move one candidate row,
so a `tree_md5 before/after` would prove nothing and the ticket's one permitted run would be spent
on it. The informative inverse (writing our 1x estimate into the pinned tree) is a different
experiment and was not taken.

**The census.** 13 of 14 joins have a candidate, 4 inside 5°/5 mm (1-2, 1-8, 5-7, 6-9); 6-7 has
none on the strongest contact in the set (66 dots, 39.0 mm, zone 0.344 mm, 869 vertices < 0.1 mm);
5-8 (7.00°/4.97 mm), 1-9 (8.82°/3.48) and 3-8 (11.00°/4.53) are near-misses with the translation
already inside the bar and the spin 2-6° outside; 1-4 (18.6°/11.8), 2-9 (15.1°/8.2), 5-6
(13.0°/15.6), 2-6 (26.2°/19.2), 1-3 (46.2°/39.4) and 3-5 (48.3°/44.0) are wrong outright. The two
controls behave as controls: 4-7 carries an in-bar candidate (3.35°/2.68 mm) on a pair that is not a
join and whose faces point the same way (`frac_btb 0.057`), and 4-6 — 7.68 mm of mesh gap, no line
run — carries only 155.2°/88.6 mm yet is carried as edge #77 in the winning state (339's finding).

**Reading: `two-causes`.** Two different loss points: 6-7 never reaches the candidate list at all
(stage boundary), while the other nine each reach it with a pose and fall outside the bar
(placement), and the axes are not the reason. Two limits are stated in the MD rather than smoothed
over: ticket 347 never measured a run for 1-3, 2-6, 3-8, 5-6 or 5-8 (their run columns are `na`,
their `cause` word is a judgement), and the trace prints the 51 rows that survived `PairwisePruning`
out of 180, so 6-7's absence cannot be separated into "never proposed" and "pruned" from these logs.
Closing either gap is cheap: run 347's reader on the five pairs, and print the per-attempt candidates
the trace's Part D describes.

### Lead — verification (2026-11-14, Thinkbook16)

verified: `COLUMNS=200 python artifacts/dsh-handover/349/accept349.py` -> 26 cases, 26 pass, 0 fail.

verified: `artifacts/t349/census349.tsv` -> 16 rows (14 joins + 4-7 + 4-6), header as specified. The
lead re-read the 51 `CAND338` rows of `artifacts/t339/logs/binary_juglet_signed_on_r1.log` with his
own parser and reproduced every row's candidate count and best candidate, the 4 in bar (1-2 2.138 /
2.203, 1-8 0.701 / 1.210, 5-7 4.708 / 3.690, 6-9 2.355 / 1.718) and the single join with none (6-7).

verified: the axis correction, independently. `class/data_path.h:1719` defines
`JUGLET_ORIG_BASE "/Dataset/SfS_pp/"` and `:1746-1754` builds every `axis_path` entry as
`JUGLET_ORIG_BASE "Axes/Juglet_Piece_N_Axis.xyz"`. The lead fetched the three axis sets from Spartan
and computed the angles himself (`artifacts/t349/axischeck/angles.py`): tree-vs-key **0.000000° on all
nine pieces** (piece 1 is `0.345475597391 0.356039490652 0.868264068532` on both sides, the point ×3),
while `Dataset/Axes/Juglet` vs key is **1.531  87.832  31.816  10.715  44.501  13.261  24.127  12.813
73.316** degrees by piece.

verified: `stage333.py:9-10` and `:110` -> the 3x bundle's `Axes/` is copied from `Dataset/Juglet_large`,
documented in that script as "GT axes, already 3x".

verified: the counts the write-up rests on -> `binary_juglet_signed_on_r1.log:187` `Total number : 180`
and `:600` `Total number pruned : 51`, so the 51 rows are survivors and the 6-7 gap is real but its
stage is not separable from these logs.

verified: `artifacts/t347/pairs347.tsv` -> 20 pairs only (the 18 graph edges plus 5-7 and 4-7), so the
`na` run columns for 1-3, 2-6, 3-8, 5-6 and 5-8 are honest gaps rather than omissions.

verified: `git status --porcelain` in the worktree -> the ticket only; nothing under `class/` or `hpc/`,
no commit by the worker.

Adjudications the lead recorded:

- **The premise is withdrawn.** The "eight of nine pieces carry a disagreeing axis" figure belongs to
  `Dataset/Axes/Juglet`, a directory this run never opens. With the tree's own axes the matcher already
  has the key's directions, so ticket 348's ceiling — 4 of 14 joins inside the bar — is an upper bound
  under perfect axes, not a number degraded by our estimate.
- **Part B is void as designed**, not skipped for convenience: substituting the key's axes for the key's
  axes cannot move a candidate row. The informative inverse is the next ticket's question.
- **`axis_diff_deg` is tree-vs-key and reads 0.000 on all 16 rows**; the 1x-vs-key column is kept in
  `census349.md` §4 for the record.
- The reading `two-causes` stands, with its two limits: 6-7 dies before the list, the other nine reach
  it with a wrong pose, and the axes are not the reason.
**End state:** ready — verified by the lead, landed and closed. The 16-row census, the axis
correction and the reading are on disk; Part B was skipped as a documented no-op; the two stated gaps
need a measurement and a trace this ticket did not licence.

