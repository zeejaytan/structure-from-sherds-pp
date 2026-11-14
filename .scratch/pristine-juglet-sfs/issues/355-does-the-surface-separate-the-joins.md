# Ticket 355 - does the surface separate the joins the breakline cannot

**Status:** ready-for-agent
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** to be created at dispatch
**Worker:** unassigned
**Waits for:** nothing - ticket 354 landed 2026-11-14
**Answers:** S1
**Depends on:** 354 (the locked placements and their scores), 348 (the measured contact set and its rule), 347 (the mating-face method), 349 (the candidate ceiling)
**Blocks:** any claim that a surface term can rescue the pair stage, and any build of the axis-locked pairing into the assembler
**Budget:** small (3 failed acceptance runs). Laptop arithmetic over geometry already on Spartan; one small job at most.
**Hand-over:** DeepSeek: laptop arithmetic, one script, one table.

> **In one line.** Rebuild all 36 axis-locked placements from ticket 354, measure the mating surface
> at each of them, and find out whether a surface term keeps the 13 real joins while rejecting the
> two non-joins (4-7, 7-9) that the breakline score ranks mid-pack.

## Why this is the next question

Ticket 354 showed the placement is nearly solved: with the piece axis known, a turn-and-height search
puts **13 of the 14 measured joins inside 5 deg / 1.67 mm**, and every join has one clean well at the
key. What fails is the choice. Seven of the 22 non-joins hold edgeline dots within 3.0 mm of their
partner at the key; all seven score at or below the worst join (1.8839 mm) and three beat the best
join (2-5 0.3010, 2-8 0.5650, 6-8 0.4038 against 3-5 at 0.6738). Those three rest on 2-4 dots per
side, so an overlap floor dismisses them. The two with a join-sized pool are the problem:
**4-7 (0.8218 mm, 12+14 dots) and 7-9 (0.8202 mm, 7+7)** sit mid-distribution, better than five real
joins.

Those are exactly the two pairs ticket 348 measured as passing within 0.1 mm **without mating**:
4-7 has 5.7% of its facing points back-to-back (median dot +0.457) and 7-9 14.1% (- 0.187), against
68-95% for the 14 measured joins. A breakline fixes where a piece sits but not which face it presents;
the surface is the only thing on record that tells the two apart. Nobody has yet measured it at the
placement the search actually chose - only at the key - and that is what decides whether the term is
usable.

## Part A - rebuild the placements before measuring them

Reconstruct each pair exactly as ticket 354 scored it: piece **a** at its key pose from
`/data/gpfs/projects/punim2657/Dataset/Juglet_GT`, piece **b** turned about the pot axis line by the
table's `turn_deg` and slid along it by `height_mm`, both from
`C:/PR/artifacts/t354/pairs354.tsv` (36 rows, primary arm, 1x dots, pool 3.0 mm).

- Reproduce each row's `score_mm` with the metric ticket 354 states: capped symmetric RMS
  `sqrt(mean(min(d, 3.0)^2))` over the dot pool frozen at turn 0 / height 0, both directions.
  **Acceptance: every row within 0.02 mm**; quote the largest difference and its pair.
- Anchor the frame on the key: at turn 0 / height 0 for **1-8, 5-7, 6-7, 4-7** reproduce ticket 348's
  1x numbers - n<0.1 = 676 / 845 / 869 / 369 and frac_btb = 0.709 / 0.755 / 0.776 / 0.057.
  **Acceptance: every count and fraction within 1% and 0.01.**
- The axis line is the one ticket 354 used: P0 = (69.3343, 80.0936, 418.8835) mm, D = (0.163227,
  0.184413, 0.9692). State the sign convention you used for `turn_deg` and prove it with the 1-2 row
  (turn 2.00, height +0.70, score 0.9267).

If either anchor fails, **stop**: the reading is `placements-do-not-rebuild`, and say what did not
rebuild. Do not proceed to measure a placement you cannot reproduce.

## Part B - the surface at the chosen placement

At each of the 36 locked placements, the mating measure of ticket 347/348 (exact point-to-triangle
distances, outward normals calibrated by ray parity, signed distance at the closest surface point):

1. vertex pairs within **0.1 mm**: count, median dot, fraction with dot <= -0.5 (back-to-back);
2. the contact patch: bounding-box diagonal, and each piece's fraction of vertices inside it;
3. the same at turn 0 / height 0, so the shift from the key is visible.

Write `C:/PR/artifacts/t355/surface355.tsv`, 36 rows, columns:
`pair  measured_join  turn_deg  height_mm  bl_score  pool_a  pool_b  n_lt_0.1  dot_med  frac_btb  bbox_mm  verts_frac_a  verts_frac_b  n_lt_0.1_key  frac_btb_key  surf_ok`
where `surf_ok` is ticket 348's rule applied at the **locked** placement (gap <= 0.05 mm AND
frac_btb >= 0.40 AND bbox >= 8 mm) and `_key` columns are the same measure at turn 0.

Also write `C:/PR/artifacts/t355/render355.png`: for 1-2, 3-5, 4-7, 7-9 and 2-5, the facing patch at
the locked placement (the two pieces' points within 0.1 mm, coloured by dot sign), so the conservator
can see whether the surfaces meet or stand side by side. One figure, five panels, axes labelled.

## Part C - three rules, scored on the same 36 pairs

Joins are ticket 348's 14 (2-9 is the fragile one, bbox 8.050 mm - say so where it matters); the
other 22 are non-joins, seven of which have any contact at the key.

| rule | admits a pair when |
| --- | --- |
| **R1** | `bl_score <= 1.8839` (the worst join's score; the breakline alone) |
| **R2** | R1 and `pool_a >= 6` and `pool_b >= 6` |
| **R3** | R2 and `surf_ok` at the locked placement |

For each rule report: joins kept of 14 (list the misses), non-joins admitted of 22 (list them, and
mark which of the seven with contact), and the smallest join margin (`1.8839 - bl_score` for R1;
state how you define it for R2/R3). Add **R3a** = `surf_ok` alone, without the breakline score, to
show what the surface buys by itself.

## Part D - the reading, exactly one

- `surface-term-separates` - R3 keeps at least 12 of the 14 joins and admits **no** non-join among
  the seven with contact, and beats R2 on at least one of them (4-7 or 7-9);
- `surface-term-helps-not-enough` - R3 keeps at least 12 joins but still admits 1-2 of the seven;
- `surface-term-costs-joins` - R3 keeps fewer than 10 joins (say which and why: no patch, wrong face,
  or a patch below the bbox floor);
- `placements-do-not-rebuild` - Part A did not reproduce (a stop, not a result).

Quote the rows that decide it. Do not add a fifth reading; if none fits, report incomplete.

## Part E - acceptance

- [ ] Part A's two anchors are quoted with both numbers (36 scores within 0.02 mm; the four 348 rows);
- [ ] the sign convention for `turn_deg` is stated and proved on the 1-2 row;
- [ ] `surface355.tsv` has 36 rows and every column above;
- [ ] R1, R2, R3 and R3a are each scored with joins kept, non-joins admitted, and the lists;
- [ ] 4-7 and 7-9 are named individually in every rule's outcome;
- [ ] `render355.png` exists with five labelled panels and is described in one sentence;
- [ ] exactly one reading from Part D, with the deciding rows quoted;
- [ ] artefacts under `C:/PR/artifacts/t355/` (`surface355.tsv`, `render355.png`, the script, `logs/`);
- [ ] `git diff --stat` shows this ticket and its write-back only - no `class/`, no `hpc/`, no job
      submission change.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading its artefacts.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- Meshes and poses are on Spartan under `/data/gpfs/projects/punim2657/Dataset/`; heavy work runs on a
  compute node (one `sbatch`, 2 h, 4 cpus, `--mem=8G`), polled with
  `bash C:/PR/scripts/slurm_poll.sh <JOBID>`, final `sacct` recorded. Reading logs and arithmetic on the
  laptop are allowed.
- Report, do not conclude: tables, the four rule outcomes, the one reading slug.
- If the same failure happens twice, stop and report incomplete, naming the blocker.

## Where the numbers live

- `C:/PR/artifacts/t354/pairs354.tsv` - the 36 locked placements and their scores (primary arm);
  `pairs354_3xd.tsv`, `pairs354_3xp18.tsv` are the robustness arms - not needed here.
- `C:/PR/artifacts/t348/contacts348.tsv` - the 14 measured joins and the rule that made them;
  `C:/PR/artifacts/t347/mating347.py` - the mating measure to reuse (do not re-derive it).
- `C:/PR/artifacts/t354/render354_placements.png` - the picture this ticket answers.

**End state:** _(the worker writes exactly one line here: ready / incomplete / blocked / stopped, with the one thing that decides it)_
