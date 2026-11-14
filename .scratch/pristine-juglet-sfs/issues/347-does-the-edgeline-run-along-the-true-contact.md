# Ticket 347 — does the edgeline run along the true contact stretch

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 347-does-the-edgeline-run-along-the-true-contact, in /c/PR/worktrees/structure-from-sherds-pp/347
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 326 (the gap between the lines at the true pose), 333 (the 3x scale test), 341 (the gate's refusals), 344 (stopped: 5-7 is not a join)
**Waits for:** nothing — the inputs are on disk and the reference check is done
**Blocks:** the candidate-generation ticket, and any change to the breakline extraction
**Budget:** small (2 failed checks)
**Hand-over:** DeepSeek: offline geometry, no assembler run; the hidden check is the table in Part B.

> **In one line.** At the true pose the two interior edgelines of a touching pair come within a
> millimetre of each other, but only over a short run — measure how short, on every pair, and say
> whether the line is right and the matcher asks too much of it, or the line wanders off the contact.

## What is already known

- The assembler reads exactly one breakline per piece, `Breakline_0`, the boundary of the interior
  face (`class/data_path.h` names only `_0`). So the line this ticket measures is the line the
  assembly seats by.
- Ticket 326 measured, at the true pose on the 1x tree, the **closest approach** of the two interior
  lines: nine of the ten touching pairs 0.30-2.91 mm (3-7 is 14.19, the exception) and every
  non-contacting pair 3.60-21.38 mm. Its strict probe (2.0 mm gap, normal dot >= 0.85) admits 1-8,
  2-9, 3-5 and 6-9 with 4-18 inliers and finds zero on every non-contact. The lines meet where the
  sherds truly touch and nowhere else.
- What that did **not** measure is the **stretch**: `SfSpp_preprocessing/artifacts/67/out/A20/T67_gtpairs.tsv`
  gives whole-curve medians of 2.33 mm (1-8, 19 dots within 2 mm), 3.71 (1-2, 5 dots), 8.93 (7-9,
  3), 12.65 (3-5, 5), and no dots at all within 2 mm for 1-9 and 2-5. The matcher wants 7 dots over
  about 12.6 mm. So the suspicion is that the line is right and short, not right and absent.
- Ticket 333 scaled the corpus 3x: 7 of 9 seams then reach 7 dots and 12.6 mm, but the assembly seats
  0/9 because the scoring gates are absolute millimetres (50 mm position, 10 mm strict, 4.6 mm seat).
  Denser dots alone did not buy seats.
- The reference's own edges are in dispute at the margins: the graph
  (`SfSpp_preprocessing/artifacts/67/juglet_gt/Juglet_simple_graph.txt`, 18 edges, decoded by the
  lead) does not list 6-9 although the meshes meet at 0.58 mm, and it does list 1-4 although ticket
  326 did not measure it among the ten touching pairs. Part B re-measures all of them.

## Part A — inputs, frame and the scale check

No switch and no assembler change: this ticket edits no line of `class/` or `hpc/`.

Inputs, all read-only:

- the tree the signed runs read —
  `/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1238_55473e9_t333-juglet-large/work/trees/px3/juglet`
  (`Breaklines/Juglet_Piece_N_Breakline_0.pcd`, `Ground Truth/Juglet_Piece_N_T.txt`, and the meshes;
  list the directories with `ls`, never `find` over `/data/gpfs`);
- the 1x bundle — `/data/gpfs/projects/punim2657/SfSpp/runs/20261103-1332_fab76f5_t66-admitsign-default/outU_jug/geom`;
- the contact graph — `C:/PR/SfSpp_preprocessing/artifacts/67/juglet_gt/Juglet_simple_graph.txt`.

**The frame check comes first.** The tree is a 3x copy of the corpus. For pieces 1 and 8, prove that
the tree's `Breakline_0` has the same point count as the 1x bundle's and coordinates equal to the 1x
file times a single factor within 1e-3 mm; report the factor you measure (expect 3.0) and divide every
length by it. If the factor is not constant across the two pieces, stop and report
`scale-frame-mismatch` — every number after that point would be in two frames at once.

`X_world = R*X + t` for the `T` files (`Dataset/Juglet_GT/README-axes.md`). Use the tree's own `T`
files; do not substitute the axes.

## Part B — the pair table

One row per pair: all 18 graph edges, plus 5-7 and 4-7 as negative controls — **20 rows**. Report every
length in 1x millimetres, three decimals.

| pair | in graph | touching (326) | mesh gap | Bl0-Bl0 min | zone median | zone RMS | <=1.0 | <=1.5 | <=2.0 | run dots | run mm | run |dot| | whole median |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

Definitions, so the table means one thing:

- **mesh gap** — symmetric minimum vertex-to-mesh distance between the two meshes at the true pose.
  This is the pair's contact test: `<= 0.1` touching, `> 0.5` the reference and the meshes disagree.
- **Bl0-Bl0 min** — closest approach of the two interior polylines (point-to-segment, both directions).
  This reproduces ticket 326's number and is the continuity check on this ticket's own code.
- **zone** — the contact zone is the set of each piece's `Bl0` dots within **3.0 mm** of the other
  piece's mesh. `Bl0` sits 1-2 mm inside the true edge, so a 0.1 mm zone would be empty; say so in the
  write-back and keep the 0.1 mm test for the meshes, not for the lines.
- **zone median / RMS / <=x** — point-to-**segment** distances from those dots to the other polyline
  (the dots are about 1.8 mm apart at 1x, so point-to-point would overstate every distance by up to
  half a dot spacing). Both directions, pooled; report the count so the fraction has a denominator.
- **run dots / run mm** — the longest run of consecutive `Bl0` dots of one piece that stay within
  2.0 mm of the other polyline, allowing at most two single-dot breaks; the longer run of the two
  directions, its length in dots and in millimetres of arc.
- **run |dot|** — median absolute normal agreement along that run (the `.pcd` normals, both files).
- **whole median** — all dots of both lines against the other polyline, for continuity with
  `T67_gtpairs.tsv`.

Deliver `pairs347.tsv` with that header and one row per pair, plus a `pairs347.md` rendering the same
table for reading.

## Part C — renders, at least three

Every operation that reads geometry is rendered before it is reported. One view per figure, camera
along the seam normal so the gap is seen edge-on, `Bl0` of each piece a different colour, the mesh
contact zone shaded, the numbers in the title:

1. seam 1-8 at the true pose (the joins that stick: expected zone median 1-2 mm, run >= 7 dots);
2. seam 6-9 at the true pose (the contact the graph omits);
3. seam 5-7 at the true pose, which is **not** a contact — the negative control that shows what a
   false positive looks like against the same measurement;
4. if the reading is `line-meets-but-short`, a fourth figure of seam 7-9 with the 2.0 mm band drawn.

PNG, 150 dpi, in `artifacts/t347/renders/`. Name them `seam_1_8_contact.png`, `seam_6_9_contact.png`,
`seam_5_7_negative.png`, ... so the hidden check can find them.

## Part D — the reading, exactly one

Pick one, quote the rows that force it, and do not conclude beyond it:

- `line-runs-along-the-contact` — at least 8 of the touching pairs have a contact-zone median
  `<= 1.0 mm` **and** a run of `>= 7` dots. The line follows the true contact; the ceiling is what
  the candidate stage can generate from it.
- `line-meets-but-short` — at least 6 touching pairs have a zone median `<= 1.0 mm` but 3 or more have
  a run `< 7` dots. The line is right where the sherds meet and too short to satisfy the matcher's
  absolute `7 dots / 12.6 mm`; the lever is the matcher's demand per unit of contact, not the line.
- `line-drifts-off-the-contact` — 3 or more touching pairs with mesh gap `<= 0.1 mm` and `Bl0-Bl0`
  min `> 1.5 mm`. The extraction is the lever; do not touch the search.
- `reference-disagrees-with-the-mesh` — any pair the graph lists with mesh gap `> 0.5 mm`. Stop and
  report; that edge is not a join and its scores are not evidence.

`scale-frame-mismatch` is not a reading: stop at Part A and report.

## Part E — acceptance

- [ ] the scale check is reported with its measured factor, and every length is in 1x millimetres;
- [ ] `pairs347.tsv` has 20 rows with the header above and `pairs347.md` shows the same numbers;
- [ ] the 1-8 and 1-2 `Bl0-Bl0` minima agree with ticket 326's (1.10 and 0.30 mm) to within 0.3 mm —
  if they do not, the frame or the reader is wrong and the table is void;
- [ ] 5-7 and 4-7 print a mesh gap that is not touching (quote it);
- [ ] at least three renders exist and were looked at before the reading was written;
- [ ] one reading from Part D, with the rows quoted;
- [ ] `git diff --stat` shows only this ticket — no `class/`, no `hpc/`, no threshold;
- [ ] the write-back ends with exactly one `**End state:**` line.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading its artefacts.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- Everything heavy runs on a Spartan compute node, never the login node and never the laptop: one
  `sbatch` job (2 h, 4 cpus, `--mem=8G`), submitted from a staged script, polled with
  `bash C:/PR/scripts/slurm_poll.sh <JOBID>`, with the final `sacct` State/ExitCode recorded.
- Write only to the worktree and to `C:/PR/artifacts/t347/` (`measure347.py`, `pairs347.tsv`,
  `pairs347.md`, `renders/`, `attribution.md`, `logs/`).
- `python3` with numpy; matplotlib is at `/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3`.
- If the same failure happens twice, stop and report incomplete, naming the blocker.
- Report, do not conclude: numbers, table, renders, and the one reading slug Part D allows.

## Comments (worker, 2026-11-14)

**End state:** not yet run.
