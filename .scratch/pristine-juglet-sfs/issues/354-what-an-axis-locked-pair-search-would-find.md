# Ticket 354 - what an axis-locked pair search would find

**Status:** ready-for-agent
**Waits for:** nothing - ticket 353 landed 2026-11-14 (its reading is corrected below)
**Branch:** to be created at start
**Worker:** unassigned
**Answers:** S1
**Depends on:** 350 (the residue is a spin), 353 (the spin sweep), 348 (the measured contact set), 349 (the candidate ceiling)
**Blocks:** any change to how a pair rotation is formed, and any claim that the breakline carries no spin signal
**Budget:** small (4 failed acceptance runs). Offline arithmetic on the laptop; no assembler run, no code change.
**Hand-over:** DeepSeek: laptop arithmetic over geometry already fetched.

> **In one line.** Lock each piece on its axis, so a pair has only two free moves - turn and height -
> and find out whether the breakline then picks the right pair inside the bar, with a score that
> charges for dots that leave the contact rather than dropping them.

## Why this is the next question

Ticket 353 swept the spin at each candidate's own (wrong) position and scored only dots already
inside 3.0 mm of the other curve, then required the gap to rise on **both** sides of the minimum. It
read flat on 10 of 14 joins and concluded the curve cannot see a spin, so the lever must be the data.
That conclusion overreaches in three ways, and the lead's own reproduction of the sweep says so:

| what 353 did | why it understates the signal |
| --- | --- |
| swept at the candidate's translation (up to 24 mm off the axis) | a piece that is not on the axis is not in contact, so the curve is being asked about a placement that cannot engage |
| scored only dots already within 3.0 mm | dots that leave the zone are dropped, so a wrong spin can look good by losing the dots that would show it |
| required a rise on both sides (+/-5 deg) | a real, one-sided rise is invisible to that test |

Lead's reproduction (dots from `C:/PR/artifacts/t334/edgelines`, poses from `C:/PR/artifacts/t350/gt`,
axes from `C:/PR/artifacts/t349/axischeck/key`, spin about the pot axis at the true pose, the dot set
frozen at theta = 0, symmetric point-to-segment RMS, 3.0 mm pool):

| pair | gap at 0 | gap at +5 | gap at -5 | best turn in +/-9 |
| --- | --- | --- | --- | --- |
| 1-2 | 1.161 | 1.212 | 1.403 | +2.0 |
| 1-3 | 1.174 | 1.793 | 1.007 | -4.0 |
| 1-4 | 0.926 | 1.264 | 1.017 | -1.0 |
| 1-8 | 0.946 | 1.166 | 1.340 | +0.5 |
| 1-9 | 2.000 | 2.027 | 2.261 | +2.0 |
| 2-6 | 1.330 | 1.382 | 1.461 | +1.0 |
| 2-9 | 1.649 | 2.444 | 1.352 | -5.0 |
| 3-5 | 1.163 | 1.752 | 0.798 | -6.5 |
| 3-8 | 1.001 | 1.408 | 1.114 | -1.5 |
| 5-6 | 1.199 | 2.289 | 0.893 | -3.5 |
| 5-7 | 1.133 | 1.424 | 1.271 | -1.0 |
| 5-8 | 1.387 | 1.270 | 1.682 | +5.5 |
| 6-7 | 1.384 | 1.558 | 1.515 | 0.0 |
| 6-9 | 1.357 | 2.208 | 0.836 | -5.5 |

Twelve of the 14 joins move by 0.20 mm or more when turned 5 deg in at least one direction - not the
four 353's both-sides test allowed. So the curve is **weak but not blind**, and whether it is strong
enough to place a pair is exactly what this ticket measures. The seam geometry explains the pattern:
on an unrolled map most seams run around the pot, and a turn slides such a seam along itself, while
5-6, the only near-vertical seam, is the sharpest responder (+1.09 mm).

Two things are still unknown and they decide the ticket's reading: the **absolute** strength of the
signal (the 1x breaklines already sit 0.9-2.0 mm from each other at the true pose, so 0.2 mm is near
the floor) and whether a scoring rule that charges for lost dots turns that into a usable minimum.

## Part A - fix the metric before using it

1. State the scoring once, in the write-up, before any result: the score of a placement is
   `sqrt(mean(min(d, C)^2))` over **both** directions and **all** dots, with `C` = the cap (3.0 mm),
   so a dot that leaves the contact costs `C` instead of disappearing. No re-selection at any angle.
2. Reproduce the ticket's two anchors with that score at zero turn and zero height: **1-2 = 0.816** and
   **5-6 = 0.596** mm (the analysis that raised this ticket quotes them). Record the values obtained.
   The lead's own attempt landed at 0.813 and 0.552 only with a 1.5 mm pool and a frozen dot set, so
   the metric must be pinned exactly; if the anchors cannot be reproduced, stop and say which metric
   was used - do not proceed on a different measure.
3. Report the dot counts on both sides for every pair (dots that are in the pool at theta = 0), so a
   rank that comes from a handful of dots is visible.

## Part B - the locked search

For all **36 pairs** (the 14 measured joins of ticket 348 and the 22 others), with both pieces on the
known pot axis:

- turn: 0 to 360 deg, 0.5 deg step;
- height: -15 to +15 mm real, 0.5 mm step (one tree unit = 3 mm - ticket 350);
- plus the end-to-end placement (turn + 180 deg, height mirrored about the piece's own mid-height),



> **Search budget.** 36 pairs x 720 turns x 61 heights is 1.6 million placements and will not run in

> Python on a laptop. Search coarse to fine: turn at 1 deg, then 0.5 deg around the best, then 0.1 deg;

> take the height from the two dot sets projected on the axis, then sweep +/-15 mm at 0.5 mm around it;

> re-check the turn at the chosen height. Report the resolution actually reached (0.5 deg and 0.5 mm at

> minimum) and the number of placements evaluated per pair.
  reported separately.

Per pair report: best (turn, height), its score, the score of the next-best **distinct** minimum (the
margin), the turn error and height error against the key in degrees and real mm, the dot count, and
whether the best placement is inside the bar (5 deg and 1.67 mm real - the code's own `MM_TOL` 5.0 in
tree units, ticket 350).

Two counts, and they are the point of the ticket:
- how many of the **14 measured joins** land inside the bar;
- how many of the **22 non-joins** score as well as a real join does (4-7 already showed a non-join
  can look like one, ticket 348).

## Part C - the reading, exactly one

- `axis-lock-recovers-the-joins` - at least 8 of the 14 joins land inside 5 deg / 1.67 mm, and no more
  than 2 non-joins score inside that bar: the locked 2-DoF search is the fix, and the next ticket
  builds it into the code.
- `axis-lock-finds-them-but-cannot-rank-them` - at least 8 joins inside the bar, but the non-joins score
  as well as the joins (margin under 20% on at least a third of them): the placement works and the
  scoring is the lever, not the search.
- `axis-lock-cannot-separate` - fewer than 8 joins inside the bar even when locked: the breakline alone
  is too weak, and the next question is the mating surface (ticket 348's contact patches) or the
  end point of this arm for this vessel.
- `locked-replay-inconclusive` - the anchors are not reproduced, or an input is missing; say which.

## Part D - what else this settles

- Fold in **ticket 352** (the five joins 347 never measured: 1-3, 2-6, 3-8, 5-6, 5-8). Part B covers
  all 36 pairs with the same reader, so the run columns those pairs lack come out of this ticket;
  352 is marked wontfix for that reason.
- **6-7**: ticket 349 left open whether the candidate was never formed or pruned. Ticket 350 answered
  it - formed, turned back-to-front, and removed by the overlap rule at 76.9 against a limit of 50.
  Part B's end-to-end placement is where that pair can be seen as a flip rather than a failure.

## Part E - acceptance

- [ ] the score is stated before results, is the capped symmetric RMS above, and is applied with no
      re-selection at any angle;
- [ ] the anchors 0.816 and 0.596 mm are reproduced or the ticket stops with the metric used;
- [ ] all 36 pairs appear, with turn, height, score, margin, turn error, height error, dot count;
- [ ] the two counts (joins inside the bar; non-joins scoring as well as a join) are printed;
- [ ] exactly one reading, named from Part C;
- [ ] artefacts under `C:/PR/artifacts/t354/`: the script, the per-pair table, the sweep curves for at
      least the 14 joins, and a render of the best locked placements against the key;
- [ ] `git diff --stat` shows this ticket and its write-back only - no `class/`, no `hpc/`, no job.

## Where the numbers live

Axes: `C:/PR/artifacts/t349/axischeck/key/Juglet_Piece_N_Axis.xyz` (x y z dx dy dz, piece frame).
Poses: `C:/PR/artifacts/t350/gt/Juglet_Piece_N_T.txt` (piece to pot, 1x). Breaklines:
`C:/PR/artifacts/t334/edgelines/Juglet_Piece_N_Breakline_0.pcd` (1x, ASCII, 30-185 dots per piece).
A denser set exists at `C:/PR/artifacts/t334/edgelines3x` (the 3x tree, 77-185 dots) and may be used
as a second arm - divide coordinates by 3 and use `C:/PR/artifacts/t350/tree/Ground Truth` - because
the 1x extraction is what leaves 0.9-2.0 mm of gap at the true pose.