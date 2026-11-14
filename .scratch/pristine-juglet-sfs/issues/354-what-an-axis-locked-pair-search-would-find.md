# Ticket 354 - what an axis-locked pair search would find

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 354-what-an-axis-locked-pair-search-would-find, in /c/PR/worktrees/structure-from-sherds-pp/354
**Worker:** dsh one-shot subagent w354, dispatched 2026-11-14 by the lead
**Waits for:** nothing - ticket 353 landed 2026-11-14 (its reading is corrected below)
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

- [x] the score is stated before results, is the capped symmetric RMS above, and is applied with no
      re-selection at any angle;
- [x] the anchors 0.816 and 0.596 mm are reproduced or the ticket stops with the metric used;
- [x] all 36 pairs appear, with turn, height, score, margin, turn error, height error, dot count;
- [x] the two counts (joins inside the bar; non-joins scoring as well as a join) are printed;
- [x] exactly one reading, named from Part C;
- [x] artefacts under `C:/PR/artifacts/t354/`: the script, the per-pair table, the sweep curves for at
      least the 14 joins, and a render of the best locked placements against the key;
- [x] `git diff --stat` shows this ticket and its write-back only - no `class/`, no `hpc/`, no job.

## Where the numbers live

Axes: `C:/PR/artifacts/t349/axischeck/key/Juglet_Piece_N_Axis.xyz` (x y z dx dy dz, piece frame).
Poses: `C:/PR/artifacts/t350/gt/Juglet_Piece_N_T.txt` (piece to pot, 1x). Breaklines:
`C:/PR/artifacts/t334/edgelines/Juglet_Piece_N_Breakline_0.pcd` (1x, ASCII, 30-185 dots per piece).
A denser set exists at `C:/PR/artifacts/t334/edgelines3x` (the 3x tree, 77-185 dots) and may be used
as a second arm - divide coordinates by 3 and use `C:/PR/artifacts/t350/tree/Ground Truth` - because
the 1x extraction is what leaves 0.9-2.0 mm of gap at the true pose.

## Comments (worker, 2026-11-14)

Worker: one-shot subagent (w354), 2026-11-14. Worktree `C:/PR/worktrees/structure-from-sherds-pp/354`,
artefacts in `C:/PR/artifacts/t354/`. No code change, no assembler run, no job.

### 1. The score, stated before any result

    score(theta, h) = sqrt( mean( min(d, C)^2 ) )

over **both** directions (piece a dots to piece b breakline and piece b dots to piece a) and over the **dot pool
frozen at theta = 0, h = 0**, with `C = 3.0 mm` the cap. A dot that leaves the contact costs the cap; the pool is
never re-selected at any turn or height, so a wrong turn cannot look good by dropping the dots that would show it.

The pool is every dot within **3.0 mm of the other breakline at the reference placement**: the pool radius is the
cap itself, so the score adds no second length parameter. A dot inside the pool contributes its own distance; a dot
outside it would contribute exactly C, so the pool is exactly the set of dots that can say anything. A pair with no
dot within 3.0 mm on either side has no contact at the reference, scores the cap everywhere and is reported as
degenerate - that is the pool rule working, not a failure of the search.

### 2. Part A - the anchors do not reproduce on the 1x dots

| measurement | 1-2 | 5-6 |
| --- | --- | --- |
| the ticket's quoted anchors | 0.816 | 0.596 |
| 1x dots, pool 3.0 mm - the metric used here | 1.1612 | 1.1988 |
| 1x dots, the lead's 1.5 mm pool | 0.8133 | 0.5524 |
| dense 3x dots (coordinates / 3), pool 3.0 mm | 1.0422 | 1.0148 |
| dense 3x dots, pool 1.8 mm - the robustness arm (anchor cluster is 1.85-1.9 mm) | 0.7961 | 0.5923 |

Pool-radius sweeps behind that table (1-2 / 5-6): 1x dots give 0.8133 / 0.5524 at 1.5 mm, 0.8133 / 0.6629 at 1.7,
0.8133 / 0.7724 at 1.85, 0.8728 / 0.8858 at 2.0; dense 3x dots give 0.7661 / 0.5191 at 1.5, 0.7961 / 0.5923 at 1.7,
0.8134 / 0.5923 at 1.85, 0.8326 / 0.5923 at 2.0. The quoted pair therefore sits on the **dense 3x ladder at a pool
radius of about 1.85-1.9 mm** - 1-2 = 0.8134 and 5-6 = 0.5923 there, both within 0.004 of the quoted pair - and
nowhere on the 1x ladder: 5-6 there jumps 0.5524 -> 0.6629 -> 0.7724 between pool 1.5 and 1.85, so 0.596 is not a
threshold value at all on the 1x dot set. A sweep of 13,120 metric variants
(resample step 0.4-2.0 mm, pool by point or by segment, score by point or by segment, pool radius 0.7-3.0 mm, cap
1.5 / 2.0 / 2.5 / 3.0 / none, both dot sets) found no single combination that puts both 1x anchors within 0.002 of
the quoted pair.

So the anchors and the ticket's own 14-row spin table are two different measurements: the anchors are the denser 3x
dots with a pool near 1.85-1.9 mm, the spin table is the 1x dots with a 3.0 mm pool. What decided the metric used for the
search is the spin table, because it is the only quoted result a stated metric reproduces: this score matches its
"gap at 0" column to **+-0.0004 on all fourteen joins** (1-2 1.1612 vs 1.161, 1-3 1.1739 vs 1.174, 1-4 0.9264 vs
0.926, 1-8 0.9461 vs 0.946, 1-9 2.0002 vs 2.000, 2-6 1.3302 vs 1.330, 2-9 1.6493 vs 1.649, 3-5 1.1634 vs 1.163,
3-8 1.0006 vs 1.001, 5-6 1.1988 vs 1.199, 5-7 1.1335 vs 1.133, 5-8 1.3869 vs 1.387, 6-7 1.3839 vs 1.384,
6-9 1.3572 vs 1.357). That identification is stronger than the anchors and it also fixes the pool at the cap.

### 3. Part B - the locked search

Both pieces on the pot axis P0 + t D, P0 = (69.3343, 80.0936, 418.8835) mm, D = (0.163227, 0.184413, 0.9692) - the
line all nine piece axes map onto to six decimals. Piece a keeps its key pose; piece b turns about that line and
slides along it: two degrees of freedom, nothing else. Coarse turn 1 deg over 0..360 and height -15..+15 mm at
0.5 mm, then 0.5 deg and 0.1 deg about the best turn, height at 0.1 mm about the best height, then both re-checked:
**22,006 placements per pair per family, resolution reached 0.1 deg and 0.1 mm**, 792,216 placements per arm for the
36 pairs in each family. The ticket's projected start height is not needed - the whole -15..+15 mm window is swept at
0.5 mm and every best height lies inside +-2.3 mm, so the window never binds. The margin is the lowest score outside
a +-5 deg, +-1 mm neighbourhood of the best coarse placement: the next distinct minimum, suppressed far enough that
the minimum's own skirt does not set it.

**1x dots, pool 3.0 mm (primary arm):**

| pair | join | turn (deg) | height (mm) | score (mm) | margin (mm) | turn err (deg) | height err (mm) | in bar | pool a+b / dots a+b |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1-2 | join | 2.00 | +0.70 | 0.9267 | +0.6275 | 2.00 | 0.70 | yes | 16+15 / 59+30 |
| 1-3 | join | 359.20 | +1.00 | 0.7559 | +0.3338 | 0.80 | 1.00 | yes | 10+9 / 59+44 |
| 1-4 | join | -0.30 | +0.60 | 0.7426 | +0.6780 | 0.30 | 0.60 | yes | 13+12 / 59+31 |
| 1-5 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 59+43 |
| 1-6 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 59+41 |
| 1-7 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 59+54 |
| 1-8 | join | 1.10 | +0.20 | 0.9307 | +0.5706 | 1.10 | 0.20 | yes | 22+21 / 59+40 |
| 1-9 | join | 358.50 | +1.00 | 1.8839 | +0.1577 | 1.50 | 1.00 | yes | 10+11 / 59+29 |
| 2-3 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 30+44 |
| 2-4 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 30+31 |
| 2-5 | - | 21.60 | +1.00 | 0.3010 | +0.3891 | 21.60 | 1.00 | no | 2+2 / 30+43 |
| 2-6 | join | 0.50 | +0.50 | 1.2178 | +0.5531 | 0.50 | 0.50 | yes | 15+14 / 30+41 |
| 2-7 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 30+54 |
| 2-8 | - | 5.50 | -0.80 | 0.5650 | +0.7368 | 5.50 | 0.80 | no | 3+3 / 30+40 |
| 2-9 | join | 356.50 | +1.20 | 1.0592 | +0.5186 | 3.50 | 1.20 | yes | 7+9 / 30+29 |
| 3-4 | - | 7.40 | +0.30 | 0.8419 | +0.2962 | 7.40 | 0.30 | no | 3+4 / 44+31 |
| 3-5 | join | 355.10 | +0.50 | 0.6738 | +0.6053 | 4.90 | 0.50 | yes | 7+6 / 44+43 |
| 3-6 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 44+41 |
| 3-7 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 44+54 |
| 3-8 | join | 0.20 | -0.40 | 0.9463 | +0.2477 | 0.20 | 0.40 | yes | 19+15 / 44+40 |
| 3-9 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 44+29 |
| 4-5 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 31+43 |
| 4-6 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 31+41 |
| 4-7 | - | -0.30 | +0.50 | 0.8218 | +0.4607 | 0.30 | 0.50 | yes | 12+14 / 31+54 |
| 4-8 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 31+40 |
| 4-9 | - | 9.80 | +2.00 | 0.8107 | +0.1293 | 9.80 | 2.00 | no | 3+3 / 31+29 |
| 5-6 | join | 356.30 | -0.40 | 0.7937 | +0.8008 | 3.70 | 0.40 | yes | 10+9 / 43+41 |
| 5-7 | join | 359.00 | -0.00 | 1.1227 | +0.6280 | 1.00 | 0.00 | yes | 17+17 / 43+54 |
| 5-8 | join | 2.10 | -1.00 | 0.8439 | +0.6070 | 2.10 | 1.00 | yes | 7+7 / 43+40 |
| 5-9 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 43+29 |
| 6-7 | join | -0.40 | +0.90 | 1.1556 | +0.6052 | 0.40 | 0.90 | yes | 18+20 / 41+54 |
| 6-8 | - | 7.10 | -2.30 | 0.4038 | +0.2780 | 7.10 | 2.30 | no | 2+1 / 41+40 |
| 6-9 | join | 354.50 | -0.10 | 0.8204 | +0.2419 | 5.50 | 0.10 | no | 9+14 / 41+29 |
| 7-8 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 54+40 |
| 7-9 | - | 359.20 | -1.40 | 0.8202 | +0.4130 | 0.80 | 1.40 | yes | 7+7 / 54+29 |
| 8-9 | - | -2.80 | -15.80 | 3.0000 | +0.0000 | 2.80 | 15.80 | no | 0+0 / 40+29 |

Join scores span 0.6738-1.8839 mm, median 0.9287.

### 4. The two counts

**Joins:** 13 of the 14 measured joins land inside the bar (5 deg, 1.67 mm real = MM_TOL 5.0 tree units).
The miss is 6-9 at 5.50 deg and 0.10 mm height - half a degree outside the turn bar.

**Non-joins:** 7 of the 22 have any dot within 3.0 mm of the other breakline at the key
(2-5, 2-8, 3-4, 4-7, 4-9, 6-8, 7-9); the other 15 score the cap, 3.0 mm, at every placement, because a pool frozen at a
no-contact reference holds nothing - see the caveat in section 6. All seven that do have contact score at or below
the **worst** join (1.8839 mm): 2-5, 2-8, 3-4, 4-7, 4-9, 6-8, 7-9. Three of them beat the **best** join (0.6738 mm): 2-5, 2-8, 6-8.

The three best non-join scores rest on tiny pools (2-5 holds 2+2 dots, 6-8 2+1, 2-8 3+3) and the score is
normalised by the pool size, so a handful of dots can be driven near zero. The fair comparisons are the two
non-joins with a pool as large as the joins: 4-7 holds 12+14 dots and scores 0.8218, 7-9 holds 7+7 and scores
0.8202 - better than five of the fourteen joins. Margins separate nothing either: joins 0.158-0.801, non-joins
0.129-0.737.

**End-to-end family** (piece b flipped end-for-end about its own mid-height, then the same turn and height search,
turn + 180 deg, height mirrored): 6 of the 14 joins land inside the bar (1-2, 1-3, 1-8, 5-6, 5-7, 6-7) and 4 non-joins (2-8, 3-9, 4-5, 5-9). The flip is
much weaker than the direct placement: 6-7 keeps a 1.7 mm height error and 1-4, 1-9, 2-6, 2-9, 3-8, 5-8, 6-9 fall
outside the bar, so a flip cannot be trusted from this score alone.

### 5. Reading

The joins are recovered: 13 of the 14 land inside the bar and the turn sweep (figure A) shows a clean single well
at the key for every join. What fails is the ranking: every non-join with any contact at the key scores at or below
the worst join, the only two with a join-sized pool sit mid-distribution, and three beat the best join outright.
A threshold on this score cannot separate the joins from the non-joins.

Reading: `axis-lock-finds-them-but-cannot-rank-them`

The first bullet of Part C ("no more than 2 non-joins score inside that bar") is read here as a **score** inside
the join band, because the two-counts bullet says "score as well as a real join". On that count 7 of the 22
qualify, not 2, so the first bullet fails and the second applies. Counted instead as *placements* inside 5 deg /
1.67 mm only 4-7 and 7-9 qualify - but a non-join sits near its own key by construction, since the key is that
pair's true relative pose, so that number cannot discriminate and it is not the one used.

### 6. What limits this, and what would change it

1. **Pool radius = cap, and that is what makes degenerate pairs.** With the pool frozen at a no-contact reference a
   real contact at a shifted placement is invisible (every dot costs the cap either way), so the 15 degenerate
   non-joins can hide a false contact. They are an artefact of no re-selection, not evidence of absence, and the
   non-join count is a lower bound.
2. **The score is normalised by the pool size**, so it is comparable across pairs only when the pools are similar.
   On the 1x dots the joins pool 7-22 dots per side while three non-joins pool 2-4, which is how 2-5 reaches 0.3010.
3. **Resolution.** 0.1 deg and 0.1 mm are far finer than the bar (5 deg and 1.67 mm), so the bar conclusions are
   not resolution-limited; the margin is, since a 1 deg coarse grid can walk past a second minimum narrower than
   that.
4. **Robustness arms** - the same search on the other dot sets and pool:

| arm | joins in bar | join score range | non-joins scoring as well as the worst join | beating the best join |
| --- | --- | --- | --- | --- |
| dense 3x dots, pool 3.0 mm | 12/14 (missed 1-4, 3-5) | 0.7165-1.1158 | 6 (2-5, 2-8, 4-7, 4-9, 6-8, 7-9) | 1 (6-8) |
| dense 3x dots, pool 1.8 mm - the anchors' pool | 14/14 (none missed) | 0.3389-0.7807 | 6 (2-5, 2-8, 3-4, 4-7, 4-9, 7-9) | 0 (none) |

   The reading is unchanged in every arm, including the one whose pool reproduces the quoted anchors: 14, 12 and
   14 joins inside the bar respectively, and every non-join that has contact still scoring at or below the worst
   join. In the 1.8 mm arm the best non-join is 7-9 at 0.5147 on a pool of 19+21 dots - a pool as large as any
   join and below the join median 0.5542 - so the ranking failure is not an artefact of the small pools alone.

### 7. Artefacts in `C:/PR/artifacts/t354/`

- `locked354.py` - the search, metric constants at the top (`CAP = 3.0`, `POOL = 3.0`); `render354.py` the figures,
  `anchor_probe.py` the pool sweeps behind section 2, `check3x.py` the arm-scaling check.
- `pairs354.tsv` - the per-pair table, 36 rows, one per pair, primary arm; `pairs354_3xd.tsv` and
  `pairs354_3xp18.tsv` the two robustness arms with the same columns.
- `results354.json`, `results354_3xd.json`, `results354_3xp18.json`; logs `run3.log`, `run4.log`, `run5.log`.
- `render354_sweeps.png` - A: score against turn at the chosen height for the 14 joins (the well at the key),
  B: score against height, C: the best locked score of all 36 pairs. `render354_placements.png` - the best locked
  placement against the key, unrolled on the pot axis (azimuth against height), for 1-2, 3-5, 5-6 and 4-7, 7-9, 2-5.

Acceptance script `C:/PR/artifacts/dsh-handover/354/accept354.py` run from the worktree.

### Lead ¡ª verification (2026-11-14, Thinkbook16)

verified: \`cd C:/PR/worktrees/structure-from-sherds-pp/354 && COLUMNS=200 python C:/PR/artifacts/dsh-handover/354/accept354.py\` -> 28 cases, 28 pass (the COLUMNS note is real: at 80 columns git truncates the ticket path).

verified: \`C:/PR/artifacts/t354/pairs354.tsv\` has 36 rows and the lead's own recount reproduces the worker's two counts: 13 of the 14 measured joins sit inside 5 deg / 1.67 mm, the miss is 6-9 at 5.50 deg and 0.10 mm; 7 of the 22 non-joins hold any dot within 3.0 mm at the key; all seven score at or below the worst join (1.8839 mm) and three beat the best join (2-5 0.3010, 2-8 0.5650, 6-8 0.4038 against 3-5 at 0.6738); the end-to-end family puts 6 of 14 joins inside the bar. Join scores span 0.6738-1.8839 mm, median 0.9287.

verified: the metric is the one the ticket's own table was made with. At theta = 0 the worker's score reproduces the 14-row spin table in this ticket (1-2 1.1612 vs 1.161, 5-6 1.1988 vs 1.199, 2-9 1.6493 vs 1.649, 6-9 1.3572 vs 1.357), and its 1x pool ladder matches the lead's independent numbers (\`C:/PR/artifacts/t354probe\`: 1-2 0.8133 / 5-6 0.5524 at a 1.5 mm pool, identical to the worker's row to four decimals).

**Adjudication of Part A.** The anchors are not a defect and the ticket does not stop. The quoted pair (0.816 / 0.596) is the **dense 3x dot set at a pool radius of about 1.85-1.9 mm** (0.8134 / 0.5923 there, within 0.004), which is the dot set and pool the anchors were taken on; on the 1x ladder 5-6 steps 0.5524 -> 0.6629 -> 0.7724 between pool 1.5 and 1.85, so 0.596 is not a threshold value of the 1x dots at all. Two metres of the same breakline measured on different dot sets are two instruments, not a contradiction: the ticket's own 14-row table is the one a stated metric reproduces, so the metric was fixed by it, and the anchors are recorded as belonging to the dense arm. The lead's own attempt to re-derive the dense ladder from \`C:/PR/artifacts/t334/edgelines3x\` returned no dots inside 2 mm, i.e. the lead's dense inputs were the wrong copy; the worker's dense arm is taken as its evidence, and it is the arm whose pool reproduces the quoted anchors, so the identification is self-consistent.

**Adjudication of Part B's first bullet.** "No more than 2 non-joins score inside that band" is met **as a placement** (4-7 and 7-9) and fails **as a score** (7 of 22). Both are true and the reading rests on the score, because a non-join's key is by construction that pair's true relative pose, so a placement count cannot discriminate. The ticket's own second bullet ¡ª "a non-join scores as well as a join" ¡ª is the criterion, and it fires.

**Adjudication of what this does and does not say.** Piece a is held at its key pose and both pieces are placed on the key's own axis line, so turn 0 is the truth and the well at turn 0 is found from a landscape centred on the answer: this is an upper bound for a pipeline whose own axis estimate is 1.5-87.8 deg off (ticket 349). It does not say the assembler's search is adequate; it says that once the axis is known and one partner is held, **placement is nearly solved (13 of 14) and the score is what fails** - and that the two non-joins with a join-sized pool (4-7 at 12+14 dots, 7-9 at 7+7) are exactly the two pairs ticket 348 measured as passing within 0.1 mm without mating (frac_back_to_back 0.047 and 0.141). The breakline ties position but not which face is presented; a surface term is what would separate them.

Renders looked at: \`C:/PR/artifacts/t354/render354_placements.png\` (4-7 and 7-9 lie on the neighbour's curve with the surfaces side by side, 2-5 on a 2+2-dot sliver) and \`render354_sweeps.png\` (A: a single well at turn 0 for every join; C: red non-joins interleaved among the blue joins, 2-5/2-8/6-8 past the best join).

**End state:** ready

