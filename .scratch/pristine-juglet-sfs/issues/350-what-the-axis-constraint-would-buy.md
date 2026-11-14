# Ticket 350 — what the axis constraint would buy

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 350-what-the-axis-constraint-would-buy, in /c/PR/worktrees/structure-from-sherds-pp/350
**Worker:** dsh one-shot subagent w350, dispatched 2026-11-14 by the lead
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

**Which millimetres, and which bar.** The logged `CAND338` mm is a breakline RMS in the
run's 3x tree units; every `mm_before` / `mm_after` in the table is that number divided by 3, i.e.
real millimetres. The code's own bar is `MM_TOL 5.0` on the *tree-unit* number, which is 1.67 real
mm (and the profile bar `7.0` tree units = 2.33 real mm), so `in_bar_*` below is the *looser* reading
of the bar; the lead re-checked every row at 1.67 mm and no verdict changes (see the lead's
verification note).

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

- [x] frame and unit stated with evidence before any millimetre figure;
- [x] all 180 candidates replayed, not just the 51 survivors; the script is committed under `artifacts/t350/`;
- [x] the replay reproduces the two `CAND338` numbers it must (1-8 index 70 at 0.70 deg, 1-2 index 1 at 2.14 deg) to within 0.05 deg — otherwise the replay is wrong, not the log;
- [x] the table has 16 rows and the columns above;
- [x] both repeats used, and the reading the same in both;
- [x] one reading, quoted rows named;
- [x] `git diff --stat` shows this ticket only — **no `class/`, no `hpc/`, no threshold**;
- [x] the write-back ends with exactly one `**End state:**` line.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading it.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- The only Spartan work is fetching the nine GT pose files (small, read-only) and their md5; keep
  them under `artifacts/t350/gt/`.
- Report, do not conclude: the table, the numbers, the one reading slug.
- If the same failure happens twice, stop and report `incomplete`, naming the blocker.

## Comments (worker, 2026-11-14)

Offline replay, Parts A-C. No assembler run, no new geometry, nothing under `class/` or `hpc/`
touched, no threshold, gate, priority or switch changed. Deliverables under `C:/PR/artifacts/t350/`:
`replay350.py`, `replay350.tsv` (16 rows, repeat r1), `replay350_r2.tsv` (repeat r2),
`replay350_all180.tsv` (all 180 candidates), `replay350.md`, `summary350.json`, `gt/` (the nine
key poses with their md5) and `tree/` (the tree the run actually binds: Ground Truth, Breaklines,
Axes).

**Frame before any number.** The run binds `20261112-1238_55473e9_t333-juglet-large`,
`work/trees/px3/juglet` at `/Dataset/SfS_pp` (`submit_juglet339_array.sbatch:83`, `:91`), not
`Dataset/Juglet`. Measured against the key poses `gt/` fetched for this ticket: rotation difference
0.000000 deg on all nine pieces, translation ratio exactly 3.000000 — so **one tree unit is 3 mm**.
Angles are unit-free (`axis_raw_`/`rot` need no conversion); the `CAND338` `mm` is a breakline RMS
in tree units and every millimetre in the deliverables is that number /3 (1-8's logged 1.210 is
0.403 mm). The 5 mm bar is applied to the divided column. md5 of the nine files:
`0d936379b768cdfb19a9906fa3e36a02`, `f0f6e47f8bce271f8390d357e3464772`,
`818c23a55c65fbdcdb276abc81207663`, `03df795afd83c7868b3061e6d5b9569c`,
`752521135dcd9465eeb5d49ed4e81a97`, `61d6ddd0d30687b2803fcfff2b16c3c3`,
`373a22af34d3434764c1c63d55044010`, `d0f4ec81a244d1540bfa0f67ad3f55c4`,
`d816f3f9d2e28495d8da298584a06768`.

**Replay checked against the log.** All 180 `CAND` rows replayed; the 51 survivors reproduce
`CAND338` to max 0.001445 deg and 0.002132 mm, and the tilt reproduces the logged `axis_raw_` to
7.1e-05 deg. Both anchors hold: 1-2 **index 1** 2.1391 deg vs 2.13831 logged, 1-8 **index 70**
0.7012 vs 0.70. After the tilt is removed the residual rotation's axis is the piece axis to
0.0076 deg on every row, so the delivered `spin_deg` and `total_deg_after` agree to 0.0146 deg by
construction of the frame, not by assumption. Both repeats agree to <= 0.001 deg and give the same
reading.

**Result.** 5 of the 16 rows are inside the bar before the correction (the four joins 1-2 1.930 /
0.636, 1-8 0.487 / 0.426, 5-7 4.708 / 1.230, 6-9 2.353 / 0.573, and the control 4-7 3.353 /
0.894), 7 after: **1-9** 8.816 / 1.159 -> 1.887 / 0.777 and **5-8** 6.998 / 1.656 -> 1.710 /
1.360 cross in; nothing crosses out. Nine joins stay outside, and their residual is the spin:
2-9 15.124 -> 5.713, 3-8 11.004 -> 6.612, 1-4 15.775 -> 8.545, 1-3 179.197 -> 11.300, 3-5
140.263 -> 7.106, 2-6 26.206 -> 14.323, 5-6 13.001 -> 9.905, 6-7 165.692 -> 165.652 (spins
-165.65) and 4-6 153.455 -> 3.430 deg with 23.092 -> 11.017 mm. Two joins recovered, not the three
`axis-constraint-recovers-the-joins` requires.

**Reading: `axis-constraint-buys-only-the-tilt`.** The tilt is removable and the bar is 5 deg, yet
removing it brings in only the two joins whose spin was already small (1-9, 5-8). Every other join
keeps a spin of 5.7-165.7 deg, so the lever is how the spin hypothesis is chosen, not the tilt.
Three of the 16 rows are candidates `MergeSimPair` folded away (1-2 #3, 1-4 #37, 1-8 #67); the 51
survivors alone give every pair the same verdict, so the reading does not depend on them.

**Limits.** The replay is a counterfactual about the pose (tilt removed exactly, spin kept), so it
does not say what a constrained candidate search would register — that is the next ticket's
question. The spin is a property of the candidate's existing rotation, not an independent estimate;
both repeats parse one log, so they test the arithmetic, not the run. The full 16-row table and the
per-pair arguments are in `replay350.md` (quoted there verbatim from `replay350.tsv`).

### Lead — verification (2026-11-14, Thinkbook16)

verified: `cd C:/PR/worktrees/structure-from-sherds-pp/350 && COLUMNS=200 python
C:/PR/artifacts/dsh-handover/350/accept350.py` -> 29 cases, 29 pass. Two of those cases were mine and
wrong, fixed in the check and not in the worker's files: `nothing-but-the-ticket-in-status` sliced
porcelain lines at `l[3:]` and so dropped the leading dot of `.scratch/...`; the anchor case demanded
the literal strings `0.70` and `2.14` while the write-up quotes the more precise `0.7012` and
`2.1391`.

verified: the frame, independently of the worker — `gt/Juglet_Piece_N_T.txt` against
`tree/Ground Truth/Juglet_Piece_N_T.txt`: rotation difference 0.0007-0.0027 deg on all nine (exact
equality up to float noise; the arccos is `nan` on four pieces because the trace rounds past 3),
`|t|` ratio exactly 3.000000 on all nine (piece 1 -129.18 / 264.67 / 436.67 -> -387.55 / 794.01 /
1310.00). **One tree unit is 3 mm.**

verified: the anchors, from `replay350_all180.tsv` — index 1 (1-2) `total_deg_before` 2.139 vs
`rot_deg_c338` 2.138 and `mm_before` 0.734 vs `mm_c338_mm` 0.734; index 70 (1-8) 0.701 vs 0.701,
0.402 vs 0.403.

verified: four table rows against the log — 6-9 2.353 (logged 2.353), 5-7 4.708 (4.70843), 4-7 3.353
(3.353), 1-8 0.487 (0.487) — and every mm column equals the logged value / 3.

verified: `git diff --stat HEAD` = this ticket only (61 insertions, 8 deletions); nothing under
`class/` or `hpc/`; nothing committed by the worker (`HEAD` is the dispatch claim).

**Adjudication — which bar the in-bar columns use.** The table's mm columns are real millimetres
(logged / 3) and `in_bar` applies 5 deg and 5 real mm. The code's own bar is `MM_TOL 5.0` against the
*tree-unit* RMS, i.e. 1.67 real mm. I re-checked all 16 rows at 1.67 mm: no verdict changes — the two
rows that cross in are 1-9 (0.777 mm = 2.3 units) and 5-8 (1.360 mm = 4.1 units), both inside 5
units, and every row that stays out is out on the rotation anyway (4-6's 11.017 mm = 33 units is the
one that is out on both). **The reading therefore holds under either reading of the bar**, and the
1.67 mm figure is recorded in S1 as the bar the pipeline actually applies.

**Adjudication — what the reading does not say.** (a) The correction removes only the component of
the rotation about the shared piece axis; a candidate wrong by a spin cannot be rescued by it, so the
5-to-7 movement is the whole of what the tilt can buy and the `recovers-the-joins` limb (three joins)
is genuinely not met. (b) Translation is kept, so this says nothing about a constrained search that
would also re-fit the translation. (c) 6-7's -165.65 deg 'spin' shows the projection is a
decomposition, not a repair: it split a 165.69 deg error into 8.53 deg of tilt and 165.65 deg of
spin, which is a statement about that candidate's rotation and nothing about the join.

**End state:** ready
