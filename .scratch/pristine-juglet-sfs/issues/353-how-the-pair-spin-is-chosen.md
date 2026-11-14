# Ticket 353 - how the pair's spin is chosen

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 353-how-the-pair-spin-is-chosen, in /c/PR/worktrees/structure-from-sherds-pp/353
**Worker:** dsh one-shot subagent w353, dispatched 2026-11-14 by the lead
**Waits for:** nothing - ticket 350 landed 2026-11-14 and closed the axis question
**Answers:** S1
**Depends on:** 350 (the tilt buys two joins and the residue is a spin), 349 (the candidate ceiling), 348 (the measured contact set), 347 (the contact-curve runs)
**Blocks:** any change to how a pair's rotation is formed
**Budget:** small (3 failed acceptance runs). Offline arithmetic; no assembler run, no code change.
**Hand-over:** DeepSeek: laptop arithmetic over geometry already fetched.

> **In one line.** After the tilt is removed the pair rotation is still wrong by 3-14 deg about the
> shared axis; measure whether the breakline determines that spin at all, and whether the best spin
> would put the pair inside the bar.

## Why this is not an axis ticket

Ticket 350 closed the axis question: with the key's own axes, rotating each candidate onto its axis
moves 5 of 16 pairs inside the bar to 7, i.e. two joins (1-9, 5-8). The residue on the other nine is
a spin, so the axis is not where the fault lies and no constrained-axis build is worth making. What
is left to decide is whether that spin is *measurable* from the contact curve: if it is, the fit's
spin estimate is the fault and can be fixed in code; if it is not, no code change can recover those
joins and the lever is the data (longer or denser contact evidence, or a coarser object).

## Part A - the spin sweep

For each of ticket 350's 16 pairs (the 14 measured joins plus 4-7 and 4-6 as controls), take the
candidate 350 chose (`best_index` in `replay350.tsv`), reproduce 350's tilt removal, and then vary
**one** parameter: the spin `theta` about the shared piece axis, `theta` in **[-180, +180] deg** in
0.25 deg steps, translation held at the candidate's.

For each `theta` compute the distance between the two pieces' `Breakline_0` polylines with ticket
347's definition (`dist_pt_seg`, point to SEGMENT, both directions) and record the RMS over the dots
within 3.0 mm of the other curve.

Inputs, all already on the laptop:
- `C:/PR/artifacts/t339/logs/binary_juglet_signed_on_r1.log` (the `CAND` and `AXIS` lines);
- `C:/PR/artifacts/t350/tree/Breaklines/Juglet_Piece_N_Breakline_0.pcd` (the tree the run binds);
- `C:/PR/artifacts/t350/gt/Juglet_Piece_N_T.txt` (the key's poses);
- `C:/PR/artifacts/t350/replay350.py` and `replay350_all180.tsv` - reuse 350's parser and its tilt
  removal rather than rewriting them; `theta = 0` must land on 350's numbers.

Nothing here needs Spartan. If a file listed above is missing, say so and stop - do not re-fetch a
tree to work around it.

## Part B - what the best spin would score

Re-score each pair at `theta_opt` against the key exactly as `CAND338` does (`G_axis =
T_axis[y].GT[y]^-1.GT[x].T_axis[x]^-1`, rotation angle and breakline RMS of `M.G_axis^-1`), and put it
beside the candidate's own score from 350.

Table `C:/PR/artifacts/t353/spin353.tsv`, one row per pair, columns:
`pair`, `in_contact_set`, `best_index`, `spin_cand_deg`, `rms_at_cand`, `spin_opt_deg`, `rms_at_opt`,
`rms_at_opt_plus5`, `rms_at_opt_minus5`, `determinable`, `back_to_front`, `rot_at_opt_deg`,
`mm_at_opt`, `rot_cand_deg`, `mm_cand_deg`, `in_bar_cand`, `in_bar_opt`, `note`.
Millimetres are real millimetres (the logged tree-unit RMS / 3, as in 350). Say which unit every
number is in.

Also write `C:/PR/artifacts/t353/spin353_curves.tsv` (`pair`, `theta_deg`, `rms_mm`) for all 16 pairs,
so the reading can be re-checked without re-running.

## Part C - the reading, exactly one

Pre-registered before any number is looked at. A pair's spin is **determinable** iff
(i) within the wrapped grid [-180, +180] the objective has a single local minimum at `theta_opt`,
and (ii) `min(rms_at_opt_plus5, rms_at_opt_minus5) >= rms_at_opt + 0.20 mm`.
0.20 mm is the resolution floor: the breakline dots are about 1.9 mm apart and the RMS values in this
lane are 0.2-0.7 mm. A pair with `|theta_opt| > 90` is flagged `back_to_front`: the best spin is a
half turn, i.e. the correspondence matched the wrong end of the arc, and it is counted separately. (Lead note, 2026-11-14: limb (i) as written cannot be satisfied by a censored objective; it was repaired to "the set within rms_opt + 0.20 mm is a single interval on the ring", and that repaired count is in the lead section below.)

- `spin-is-not-determinable` - at most 4 of the 14 joins are determinable: the contact curve does
  not fix the spin at this resolution, so the lever is the data, not the fit.
- `spin-is-determinable-fit-misses-it` - at least 5 joins are determinable, and for at least 2 of them
  `in_bar_opt` is 1 while `in_bar_cand` is 0: the fit's spin is the fault and the next ticket is the
  spin estimate inside the fit.
- `spin-is-determinable-and-still-out` - at least 5 joins are determinable, but no pair that was
  outside the bar comes inside it at `theta_opt`: the join needs more than the spin.
- `spin-replay-inconclusive` - `theta = 0` does not reproduce 350's numbers, or an input is missing;
  say which and stop.

## Part D - acceptance

- [x] `spin353.tsv` has 16 rows and the columns above; `spin353_curves.tsv` has the curves;
- [x] the script is kept under `C:/PR/artifacts/t353/` and reuses 350's parser and tilt removal;
- [x] at `theta = 0` every row reproduces ticket 350's `total_deg_after` to 0.01 deg - otherwise the
  frame or the tilt removal is wrong, not the data;
- [x] the determinable / not-determinable count and the back-to-front count are printed, and the
  reading is chosen by the rule above, not by the story;
- [x] the bar is applied as 5 deg and 5 real mm (state it), and the code's own 1.67 mm allowance is
  mentioned;
- [x] `git diff --stat` shows this ticket only - no `class/`, no `hpc/`, no threshold, no assembler
  run;
- [x] one reading, and one `**End state:**` line.

## Limits to state in the write-back

- Translation is held at the candidate's, so this is a counterfactual about the rotation only; a pair
  whose translation is wrong cannot be rescued by any spin and will read `not determinable` for the
  wrong reason. Say which pairs that could be.
- Both repeats of 350 parsed one log, so this tests the arithmetic over one run, not run-to-run
  variation.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there; never edit the main clone, another
  worktree, or `SfSpp_preprocessing/`.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- No Slurm job: everything needed is on the laptop. If you believe a job is needed, say why in the
  ticket and stop rather than submitting.
- Report, do not conclude: the tables, the counts, the one reading slug.
- If the same failure happens twice, stop and report `incomplete`, naming the blocker.

## Comments (worker, 2026-11-14)

### Result

The contact curve does not fix the spin. At the ticket's 0.20 mm resolution floor the breakline RMS is
too flat to pick a spin on 10 of the 14 joins, and on the 4 where it is sharp its minimum sits within
1.25 deg of the candidate's own spin -- so the fit already sits where the contact curve points, and the
3-14 deg residue is not a spin the fit failed to optimise. 0 of 14 joins are determinable by the rule;
2 of 14 read back_to_front.

Which claim this is: the objective, not the code and not the key. Nothing measured here says the fit is
buggy; it says that this objective, at this resolution, carries almost no information about the spin.
Weight: it does answer the ticket's question, but the answer is about the instrument as much as about the
geometry (see 'Why the count is 0'), so it is not evidence that any placement is good to 3-14 deg.

### Method, and the one interpretive choice

For each of the 16 pairs I took 350's best_index candidate from replay350.tsv, rebuilt 350's G_axis and
re-ran 350's tilt removal verbatim (replay350.py:141-153: minimal rotation Rc taking the candidate's axis
direction onto +z, pivoted at the breakline centroid ctr), giving Mp. The sweep then varies one parameter:

    M(theta) = [ Rz(theta) . Mp_R | Mp_t ]        theta = -180.00 ... +179.75 deg, step 0.25

The translation column is held, so the piece spins about its own axis line: in the aligned frame the piece
axis is +z through the origin (class/reconstruction.cpp:199-200 sets axis_norm_={0,0,1} and
axis_point_={0,0,0}), so under Mp it is the z line through Mp_t. That line coincides with the shared pot
axis only when the placement is right; the distance between the two lines is in the note column as
axis_off (0.08-24.30 mm). Rotating about the breakline centroid instead (350's tilt pivot) would carry the
piece off the cylinder by up to 2*r*sin(theta/2), tens of mm, and was rejected as non-physical. +theta is
right-handed about +z, so a perfect spin recovery reads spin_opt_deg = -spin_cand_deg; every defined row
satisfies rot_at_opt = |spin_opt + spin_cand| to 0.05 deg (checked in the script, flagged in the note if it
failed -- it never did).

Distances are 347's point-to-SEGMENT distance in both directions (dist_pt_seg / p2s, measure347.py:109-112
and 250-260), pooled RMS over dots within 3.0 of the other curve. Both polylines and Mp_t are divided by
3.0 before measuring, so the zone is 3.0 real mm and every rms column is real mm.

### Anchor: theta = 0 is 350

  quantity                          max abs deviation over the 16 rows
  rot (350 total_deg_after)         0.000464 deg
  mm  (350 mm_after)                0.000492 mm
  tilt (350 tilt_deg)               0.001479 deg
  signed spin (350 spin_deg)        0.000499 deg

Against the acceptance bar of 0.01 deg that is 20x inside it, on every row, so the frame, the tilt
removal and the sign convention are 350's. 350's parser read 180 candidates and 51 survivors from the r1
log, as before. The vectorised p2s is bit-identical to 347's own loop (max abs 0.0 mm over 16 pairs x 4
thetas x 2 directions).

### Counts

- determinable: 0 of 14 joins (0 of 16 rows).
- not determinable: 14 of 14.
- back_to_front: 2 of 14 joins -- 2-6 (theta_opt +111.00) and 6-7 (-130.50); counted separately, so they
  are not in the determinable count.
- in bar at the candidate: 6 of 14 joins (7 of 16 rows, the extra one being the 4-7 control).
- in bar at theta_opt: 7 of 14 joins. The only crossing is 2-9 (5.713/2.240 -> 4.463/1.985); the 4-7
  control crosses the other way (2.876/0.877 -> 5.126/1.312) and leaves the bar.

### Why the count is 0, and why the reading does not depend on how (i) is read

Test (ii) alone is failed by 10 of 14 joins: their RMS rises by less than 0.20 mm within +/-5 deg of their
own minimum. The four that pass are 1-8 (+0.282), 5-6 (+0.401), 5-7 (+0.220) and 6-9 (+0.223). Since
(ii) is necessary, at most 4 of 14 joins could be determinable whatever (i) returns -- that is the 'at most
4' arm of the rule on its own.

Test (i) -- a single local minimum -- is failed by every row, for an instrument reason: the objective is a
censored statistic. It is the RMS over dots inside a hard 3.0 mm cutoff, so it is bounded above by 3.0 mm,
the pooled dot set changes as dots cross that boundary, and the surface therefore steps by 0.01-0.05 mm
from grid point to grid point: 79-303 formal local minima per pair (78-289 even after rounding to 1
micron). Where the placement is far off, most of the curve sits on the 3.0 mm ceiling and carries no
gradient at all. So 'a single local minimum' is not a property this objective can have, and that half of
the rule fails for every pair regardless of the geometry.

The interpretable flatness number is the width of the 0.20 mm basin (how far you can spin before the
objective rises past its minimum + 0.20 mm, walking through thetas that have no dots in the zone):
2.2-28.0 deg to either side, 2.5-4.8 deg on six joins, 5.0-7.5 on four, 10.8-28.0 on two (1-4, 6-7).
By comparison a 5 deg spin slides a breakline dot 1.07-1.54 mm along the surface (dot radius 12.29-17.64
mm from the piece axis), which is 5-8x the 0.20 mm floor, yet the in-zone RMS moves by less than 0.20 mm
on 10 of 14 joins. Two near-parallel arcs slide along each other: the RMS measures their perpendicular
separation (axis offset, wall-thickness mismatch), not the tangential registration where the spin lives.

### The table

Units: spin_* and rot_* and basin are degrees; rms_*, mm_* are real mm (= tree units / 3); axis_off and
the closest-approach figures in the note are real mm; determinable, back_to_front, in_bar_* and
in_contact_set are 0/1. spin_opt_deg is the applied theta, so spin_opt + spin_cand is the residual spin.
Full 18 columns, exactly the file:

    pair	in_contact_set	best_index	spin_cand_deg	rms_at_cand	spin_opt_deg	rms_at_opt	rms_at_opt_plus5	rms_at_opt_minus5	determinable	back_to_front	rot_at_opt_deg	mm_at_opt	rot_cand_deg	mm_cand_deg	in_bar_cand	in_bar_opt	note
    1-2	1	3	0.626	0.8066	2.500	0.7861	0.9736	0.8639	0	0	3.126	0.928	0.627	0.612	1	1	fate=merged; axis_off=0.27mm; res_spin=+3.13; nmin=264; ndots=106; basin=7.0/4.2deg; rms_true=0.877
    1-3	1	17	-11.300	2.7906	74.500	1.6880	1.8463	1.9179	0	0	63.200	35.123	11.300	24.671	0	0	fate=pair_axis; axis_off=24.30mm; res_spin=+63.20; nmin=108; ndots=20; basin=4.0/2.2deg; rms_true=2.454
    1-4	1	37	-8.545	1.8390	44.250	1.6905	1.7830	1.7457	0	0	35.706	8.148	8.545	2.949	0	0	fate=merged; axis_off=1.54mm; res_spin=+35.71; nmin=208; ndots=35; basin=28.0/10.8deg; rms_true=2.064
    1-8	1	67	0.444	0.7452	0.500	0.7148	1.1335	0.9971	0	0	0.945	0.484	0.446	0.423	1	1	fate=merged; axis_off=0.08mm; res_spin=+0.94; nmin=303; ndots=139; basin=3.5/2.8deg; rms_true=0.744
    1-9	1	77	-1.888	1.1646	-1.000	1.1236	1.2961	1.3320	0	0	2.887	0.688	1.887	0.777	1	1	fate=surv; axis_off=0.94mm; res_spin=-2.89; nmin=144; ndots=61; basin=3.5/6.0deg; rms_true=1.199
    2-6	1	97	-14.322	2.8047	111.000	2.4022	2.4934	2.5844	0	1	96.678	20.188	14.323	5.448	0	0	fate=surv; axis_off=3.26mm; res_spin=+96.68; nmin=143; ndots=18; basin=5.5/7.5deg; rms_true=2.723
    2-9	1	108	-5.713	1.5442	1.250	1.4193	1.5490	1.6412	0	0	4.463	1.985	5.713	2.240	0	1	fate=surv; axis_off=0.37mm; res_spin=-4.46; nmin=106; ndots=46; basin=4.5/5.5deg; rms_true=1.553
    3-5	1	120	7.106	nan	nan	nan	nan	nan	0	0	nan	nan	7.106	20.666	0	0	fate=pair_axis; axis_off=7.60mm; NO_DOTS_IN_ZONE at any theta (closest approach 3.49mm); nmin=0; ndots=0; flat_nan
    3-8	1	134	-6.612	1.3657	-1.500	1.3218	1.5031	1.4411	0	0	8.112	1.650	6.612	1.440	0	0	fate=surv; axis_off=1.55mm; res_spin=-8.11; nmin=168; ndots=97; basin=7.0/5.0deg; rms_true=1.724
    5-6	1	158	9.905	1.2138	1.250	1.1088	1.5605	1.5100	0	0	11.155	5.121	9.905	5.032	0	0	fate=surv; axis_off=3.85mm; res_spin=+11.16; nmin=209; ndots=62; basin=3.0/2.5deg; rms_true=2.227
    5-7	1	160	1.549	0.8681	-0.500	0.8339	1.0536	1.1003	0	0	1.049	0.792	1.549	0.844	1	1	fate=surv; axis_off=0.57mm; res_spin=+1.05; nmin=288; ndots=116; basin=4.2/4.2deg; rms_true=0.902
    5-8	1	164	1.712	1.4720	-0.250	1.4309	1.5360	1.5282	0	0	1.460	1.360	1.710	1.360	1	1	fate=surv; axis_off=0.95mm; res_spin=+1.46; nmin=79; ndots=55; basin=9.2/18.5deg; rms_true=1.487
    6-7	1	166	-165.652	1.5488	-130.500	1.3879	1.4813	1.4782	0	1	63.848	14.269	165.652	26.559	0	0	fate=pair_overlap; axis_off=1.03mm; res_spin=+63.85; nmin=246; ndots=38; basin=12.0/22.5deg; rms_true=2.402
    6-9	1	170	0.243	1.0282	-0.250	1.0072	1.2881	1.2302	0	0	0.000	0.539	0.229	0.537	1	1	fate=surv; axis_off=0.28mm; res_spin=-0.01; nmin=130; ndots=67; basin=4.8/3.5deg; rms_true=1.007
    4-7	0	145	2.876	1.0629	2.250	0.9703	1.2567	1.1795	0	0	5.126	1.312	2.876	0.877	1	0	fate=surv; axis_off=0.22mm; res_spin=+5.13; nmin=181; ndots=89; basin=4.8/4.0deg; rms_true=1.123; control
    4-6	0	142	-3.430	nan	nan	nan	nan	nan	0	0	nan	nan	3.430	11.017	0	0	fate=pair_axis; axis_off=11.40mm; NO_DOTS_IN_ZONE at any theta (closest approach 4.69mm); nmin=0; ndots=0; flat_nan; control

### Part B: what the best spin would score against the key

At theta_opt the rotation error against the key grows on 8 of the 13 measurable joins and shrinks on 5
(2-9, 5-7, 5-8, 6-7, 6-9); the breakline mm grows on 8, shrinks on 4 (1-9, 2-9, 5-7, 6-7) and is unchanged
on 5-8. Chasing the contact minimum therefore makes the pose worse by the key's own measure on the
majority of joins, which is the expected shape if the contact RMS is dominated by the spin-invariant part
of the error (axis_off 0.08-24.30 mm) rather than by the spin.

The 6-7 row is the half-turn case: its best-by-contact spin (-130.50 deg) is a wrong-end correspondence
(|theta_opt| > 90, flagged back_to_front) and still leaves 63.85 deg / 14.27 mm -- a correspondence, not
a placement. 2-6 is the other flag (+111.00 deg, 96.68 deg / 20.19 mm left).

### The limits the ticket asks me to state

(a) Translation is held at the candidate's, so this is a counterfactual about the rotation only. The piece
spins about its own axis line, which is axis_off from the pot axis; the pairs whose translation is already
too wrong for any spin to rescue are the ones with a large axis_off: 1-3 (24.30 mm), 4-6 (11.40), 3-5
(7.60), 5-6 (3.85) and 2-6 (3.26) -- against a 5 mm bar. 3-5 and 4-6 cannot be assessed at all: their two
polylines never come within 3.0 mm of each other at any theta (closest approach 3.49 and 4.69 mm), so the
censored RMS is empty on all 1441 grid points and theta_opt, rot_at_opt and mm_at_opt are undefined
(written nan). Those two read not_determinable for the translation's sake, not the spin's. 5-6 is the
borderline one: it is the only sharp join outside the bar (0.401 mm rise, basin +3.0/-2.5 deg) and its
3.85 mm axis offset is 77% of the mm bar, so its spin estimate may be measuring the radial mismatch.

(b) Both of 350's repeats parsed binary_juglet_signed_on_r1.log, and so did this sweep: it tests the
arithmetic over one run, not run-to-run variation. A second run's log would be a separate ticket.

### The bar

Applied as 5.0 deg on rot_at_opt_deg and 5.0 real mm on mm_at_opt, both, for in_bar_opt (and 5.0/5.0 at
the candidate for in_bar_cand). The code's own pose allowance is MM_TOL 5.0 in tree units = 1.67 mm real
(intent/S1-does-it-connect-anything-real.md:1044), i.e. three times looser than the mm bar used here. It
would not change the reading: the determinable count is 0 either way, and the one sharp join that is
outside stays outside on rotation (5-6 at 11.16 deg).

### Reading

spin-is-not-determinable -- at most 4 of the 14 joins are determinable (0 by the rule as written; (ii)
alone caps it at 4), so by the ticket's pre-registered rule the contact curve does not fix the spin at
this resolution and the lever is the data, not the fit's spin estimate.

Two things the next ticket should not have to rediscover. First, the four joins where the objective is
sharp (1-8, 5-6, 5-7, 6-9) already sit at the contact minimum, theta_opt within 1.25 deg of 0: a
rotation-only move buys at most 0.5 deg on 5-7 and 6-9, costs 0.5 deg on 1-8 and 1.25 deg on 5-6, and
moves none of them across the bar. Second, the rule's second arm cannot fire on a flat curve: 2-9's
crossing is a 1.25 deg wander of an argmin whose rise is 0.13 mm, and the 4-7 control wanders the other
way, so a crossing here is not evidence of a determinable spin.

### Artifacts

- C:/PR/artifacts/t353/spin353.py -- the sweep. Reuses 350's parse_log, rot_between, rot_about, aligned,
  rms_disp, read_pcd, read_m4_file and 347's p2s by ast-extracting those function bodies (importing
  replay350.py would rewrite its own tables: it has no __main__ guard).
- C:/PR/artifacts/t353/spin353.tsv -- 16 rows, the 18 Part B columns.
- C:/PR/artifacts/t353/spin353_curves.tsv -- pair, theta_deg, rms_mm; 16 x 1441 rows. -180 and +180 are
  the same rotation and both rows are present with identical rms by construction.
- C:/PR/artifacts/t353/spin353_check.json -- anchor deviations, the p2s check, and per pair the minima
  positions, basin widths, rms at the key's spin, dot radius, axis offset and closest approach.

No code, threshold, switch or job: nothing under class/ or hpc/ was modified, no assembler run, no Slurm
submission, no commit. This ticket is the only file this worker changed.

### Lead --- verification (2026-11-14, Thinkbook16)

verified: `cd C:/PR/worktrees/structure-from-sherds-pp/353 && COLUMNS=200 python C:/PR/artifacts/dsh-handover/353/accept353.py` -> 21 cases, 21 pass (one case was mine to fix: `counts-printed` looked for `back[\s-]*to[\s-]*front` and did not match the underscore form `back_to_front`; relaxed to `back[\s_-]*to[\s_-]*front`).
verified: `C:/PR/artifacts/t353/spin353.tsv` -> 16 rows, 14 in the contact set; `determinable` is 0 on every row; `back_to_front` is 1 on 2-6 and 6-7 only; `in_bar_cand` 6 joins (7 rows with the 4-7 control); `in_bar_opt` 7 joins.
verified: the +/-5 deg rise recomputed by the lead from the table -> 1-8 0.2823, 5-6 0.4012, 5-7 0.2197, 6-9 0.2230 pass the 0.20 mm floor (the 4-7 control also passes, 0.2092); every other join is 0.0552-0.1725. So "at most 4 of 14" is right.
verified: `C:/PR/artifacts/t353/spin353_check.json` -> `p2s_vs_347_p2s_max_abs` 0.0; `theta0_anchor` max |deviation| 0.000464 deg (rot), 0.000492 mm, 0.001479 deg (tilt), 0.000499 deg (spin) - 20x inside the 0.01 deg bar.
verified: `git -C C:/PR/worktrees/structure-from-sherds-pp/353 diff --stat HEAD` -> the ticket only (170 insertions, 0 deletions); nothing under `class/` or `hpc/`, no job, no commit.

**Adjudication: test (i) as I wrote it was ill-posed, and the reading rests on (ii).** The rule asked for "a single local minimum on the wrapped grid". The objective is an RMS over dots inside a hard 3.0 mm cutoff, so it is bounded at 3.0 mm, its dot set changes as dots cross that boundary, and the surface steps by 0.01-0.05 mm between neighbouring grid points - 79-303 formal minima per pair, 78-289 even after rounding to a micron. A rule no row can satisfy carries no information, and that defect is mine, not the data's.

Repaired to the same 0.20 mm resolution as (ii), and re-run by the lead over `spin353_curves.tsv` (a basin is one contiguous interval of {theta : rms <= rms_opt + 0.20} on the ring, a nan counting as outside): exactly **2 of 14 joins** are determinable - **5-7** (single basin 8.75 deg wide, rise 0.2197, theta_opt -0.50) and **6-9** (8.50 deg, 0.2230, -0.25). 1-8 and 5-6 pass (ii) but have two lobes over the floor (6.75 and 6.00 deg total), so they do not; the 4-7 control has two lobes as well.

That repaired count does not change the answer and sharpens it: the only two joins whose spin the contact curve does fix are joins the fit already places correctly, and the repair asks for a spin move of 0.50 deg on 5-7 and 0.25 deg on 6-9 against errors of 1.55 and 0.24 deg. The curve confirms the fit where the fit is already right; where the fit is 3-14 deg out (2-9 5.71, 3-8 6.61, 3-5 7.11 unmeasurable, 1-4 8.55, 5-6 9.91, 1-3 11.30) it is flat to under 0.20 mm over +/-5 deg, and a 5 deg spin slides a breakline dot 1.07-1.54 mm along the surface - five to eight times that floor. So the poor placements are not a spin the fit failed to optimise; the perpendicular-distance objective cannot see the slide that a spin is.

Two readings of the ticket I am not taking: 2-9's crossing into the bar (5.713/2.240 -> 4.463/1.985) is a 1.25 deg wander of an argmin whose rise is 0.1297 mm, and the 4-7 control wanders out of the bar under the same sweep, so a crossing inside a flat basin is not evidence; and the 165.65 deg spin on 6-7 is a genuine back-to-front correspondence, not an axis-sign artefact - the run's own line `SIGN-AXIS: accepted, axes are direction-consistent (0% of pairs > 90 deg, n=18 GT pairs)` rules the artefact out.

Caveats kept in the record: one log (r1) and translation held throughout, so this tests the arithmetic of the spin at a fixed placement, not run-to-run variation nor a joint pose search; and 5-6 is the borderline row (0.401 mm rise, axis offset 3.85 mm = 77% of the mm bar), so its sharpness may be measuring the radial mismatch rather than the spin.

**End state:** ready - the lead lands and closes


