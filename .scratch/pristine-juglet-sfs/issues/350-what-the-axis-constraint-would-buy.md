# Ticket 350 — what the axis constraint would buy

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 350-what-the-axis-constraint-would-buy, in /c/PR/worktrees/structure-from-sherds-pp/350
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Waits for:** nothing — ticket 349 landed 2026-11-14
**Answers:** S1
**Depends on:** 349 (the four-of-fourteen ceiling), 348 (the measured contact set), 336 (the stage trace this reads), 341/345 (where the shape bar bites)
**Blocks:** any change to how a pair's rotation is formed
**Budget:** small (2 failed acceptance runs). **No assembler run, no code change.**
**Hand-over:** DeepSeek: offline arithmetic over a log already on the laptop.

> **In one line.** The pair stage tilts pieces by 4-19 deg even when the axes it is given are
> perfect; measure what would happen if every candidate were rotated onto the axis it already knows,
> before anyone changes the code.

## Why this replaced the earlier version of this ticket (lead, 2026-11-14)

The first version of 350 asked for a print of the candidates *before* `PairwisePruning`. That print
already exists: `SFS_336` writes `[SFS-336] CAND` for all 180 candidates with their matrices
(`class/feature_matching.cpp:1927-1938`), `DROP` with a reason for all 124 removed
(`:1974-2001`), `SURV` for the 51 survivors (`:2011-2020`), and `RCAND` for the 2752 candidates
made at later steps (`:2131-2142`). `C:/PR/artifacts/t339/logs/binary_juglet_signed_on_r1.log`
holds all of them. The question that print was meant to answer is already answered (see below), and
the question that matters now can be answered from the same log.

**What the existing trace already says** (lead, from that log): 180 candidates, 124 dropped
(116 `pair_axis`, 2 `pair_basket`, 6 `pair_overlap`), 51 survivors, 5 merged away. **6-7 was
formed** — one candidate, index 166, `axis_angle_` 0.1489 rad = 8.53 deg, score 0.548, 16 inliers —
and **died on the overlap rule** (`DROP pair_overlap 166 6 7 76.898` against the 50.0 limit at
`class/feature_matching.cpp:1912`). It could not have been inside the 5 deg bar even if kept.
For 13 of the 14 measured joins, the row with the smallest axis tilt is the one that **survives**
pruning, so pruning is not what loses the joins.

## The measurement this ticket makes

The candidate's rotation comes from the feature cycle and `Registration(...)`; the piece's own axis
(`axis_norm_`) is only **read afterwards**, to measure `axis_raw_` / `axis_angle_`
(`class/feature_matching.cpp:1902-1906`, `:2113-2117`) and to gate at 25 deg (`:1974`, `:2165`).
It never constrains the rotation. So the tilt that the gate measures is a by-product of a free
6-DoF fit, not a necessity: with the axes this run has (the key's own — verified in ticket 349,
identical to 0.000000 deg on all nine pieces), a correct pair is a pure spin about the shared axis.

**Compute, for every one of the 180 candidates, what the placement error would be if the tilt were
removed** — i.e. rotate the moved piece by the minimal rotation that brings the two axes into
coincidence, keep the spin, and re-score the candidate against the true relative placement exactly as
`[SFS-338] CAND338` does (`hpc/tools/main_pristine_batch.cpp:322-356`: `G_axis =
T_axis[y].GT[y]^-1.GT[x].T_axis[x]^-1`, rotation angle and breakline RMS of `M.G_axis^-1`).

Inputs, all already available:
- the 180 `CAND` lines and the 9 `AXIS` lines in `C:/PR/artifacts/t339/logs/binary_juglet_signed_on_r1.log`
  (and `_on_r2.log` for a repeat);
- the true poses `Dataset/Juglet_GT/Juglet_Piece_N_T.txt` (fetch once from Spartan, keep the md5);
- the measured contact set `C:/PR/artifacts/t348/contacts348.tsv`.

## Part A — frame and units, before any number is quoted

Say which tree the run used and what one unit is worth. The `mm` column of `CAND338` is the breakline
RMS **in the run's own frame**; if that frame is the 3x bundle built by ticket 333, divide by 3 to get
millimetres and say so. Quote the evidence (a breakline coordinate magnitude, or the tree path in the
run's submit script). Rotation angles are scale-free and unaffected; every millimetre printed must be
labelled with its frame.

## Part B — the table

One row per measured join (14) plus 4-7 and 4-6 as controls, per log:

`pair`, `best_index`, `tilt_deg`, `spin_deg` (total after removing the tilt), `total_deg_before`,
`total_deg_after`, `mm_before`, `mm_after`, `in_bar_before`, `in_bar_after`, `note`.

Take `best` = the candidate with the smallest **after** error, and also print the candidate with the
smallest **before** error so the two are comparable. Deliver
`C:/PR/artifacts/t350/replay350.tsv` (header exactly as above) and a short `replay350.md` that quotes
the rows the reading rests on.

## Part C — the reading, exactly one

- `axis-constraint-recovers-the-joins` — after removing the tilt, at least three joins that are
  outside the 5 deg bar come inside it: the pair alignment is the lever, and the next ticket builds
  the constrained candidate.
- `axis-constraint-buys-only-the-tilt` — the tilt disappears but the joins still miss the bar (the
  spin dominates): the lever is how the spin hypothesis is chosen, not the tilt.
- `projection-not-well-defined` — the axes cannot be used this way (flip ambiguity, `axis_norm_`
  sign, or the two axes are not parallel even at the true placement); say which, with the numbers.
- `replay-inconclusive` — a needed input is missing or the log cannot be paired to the GT poses.

## Part D — acceptance

- [ ] frame and unit stated with evidence before any millimetre figure;
- [ ] all 180 candidates replayed, not just the 51 survivors; the script is committed under `artifacts/t350/`;
- [ ] the replay reproduces the two `CAND338` numbers it must (1-8 index 70 at 0.70 deg, 1-2 index 1 at 2.14 deg) to within 0.05 deg — otherwise the replay is wrong, not the log;
- [ ] the table has 16 rows and the columns above;
- [ ] both repeats used, and the reading the same in both;
- [ ] one reading, quoted rows named;
- [ ] `git diff --stat` shows this ticket only — **no `class/`, no `hpc/`, no threshold**;
- [ ] the write-back ends with exactly one `**End state:**` line.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading it.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- The only Spartan work is fetching the nine GT pose files (small, read-only) and their md5; keep
  them under `artifacts/t350/gt/`.
- Report, do not conclude: the table, the numbers, the one reading slug.
- If the same failure happens twice, stop and report `incomplete`, naming the blocker.
