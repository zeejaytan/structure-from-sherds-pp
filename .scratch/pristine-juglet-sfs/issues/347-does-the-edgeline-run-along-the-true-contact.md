# Ticket 347 — does the edgeline run along the true contact stretch

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 347-does-the-edgeline-run-along-the-true-contact, in /c/PR/worktrees/structure-from-sherds-pp/347
**Worker:** dsh one-shot subagent e22a4441-214c-43b1-bd3e-4cc55d558745, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 326 (the gap between the lines at the true pose), 333 (the 3x scale test), 341 (the gate's refusals), 344 (stopped 2026-11-14 on a premise this ticket reverses)
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

- [x] the scale check is reported with its measured factor, and every length is in 1x millimetres
  (factor 3.000000 measured on the meshes and on the poses);
- [x] `pairs347.tsv` has 20 rows with the header above and `pairs347.md` shows the same numbers;
- [x] the **1-2 and 1-8** `Bl0-Bl0` minima agree with ticket 326's (1-2 1.10 mm, 1-8 0.30 mm) to within
  0.3 mm — **the box was written with the pairs swapped**; 326 gives 1-2 1.10 and 1-8 0.30, and the 1x
  point-to-point table reads 1.103 and 0.304 mm, so the agreement holds;

- [x] 5-7 and 4-7 print their mesh gap — 0.000090 and 0.000307 mm, i.e. they DO touch. **The box as
  written cannot hold.** What the gap means is settled by the mating test added in the lead's section:
  5-7 mates (dot -0.910, 75.4% back-to-back), 4-7 does not (+0.448, 4.7%);
- [x] at least three renders exist and were looked at before the reading was written;
- [x] one reading from Part D, with the rows quoted;
- [x] `git diff --stat` shows only this ticket — no `class/`, no `hpc/`, no threshold;
- [x] the write-back ends with exactly one `**End state:**` line.

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

**Scale factor: 3.0, exact, and proved on the meshes and the poses rather than on the breaklines.**
The tree's Ground Truth rotation blocks are bit-identical to the 1x poses, the translations are exactly
3x, and the mesh vertices of pieces 1, 8 and 2 agree with /3.0 to 1.4e-13 mm (job 35041305,
logs/out347/scale_check.json). Dataset/Juglet_large/scale_census.tsv independently records
2.9999999999999996 for piece 1. Every length below is 1x mm (tree value / 3.0).

**The literal "same point count" limb of Part A fails, and the frame is unaffected.** The tree's
Breakline_0 has 185 dots for piece 1 and 126 for piece 8, against 59 and 40 in the 1x bundle's
outU_jug/geom. They are not one file scaled: ticket 331 made the 3x corpus by scaling the meshes and
re-running the chain, and the edgeline resample distance is a fixed 1.9 mm
(edgeline_extraction_headless.cpp:574), so at 3x the same edge gets about 3x the dots, 0.600 mm apart
at 1x scale instead of 1.800. Consequence: the tree's line sits higher up the contact than the 1x
line, so the **zone and run columns are not frame-invariant** — the 1x table is printed beside the tree
table in pairs347.md section 4 and labelled as such.

**The reading: line-runs-along-the-contact**, on Part B's contact test (mesh gap <= 0.1 mm -> 12 pairs).
Nine of them have a zone median <= 1.0 mm and a run >= 7 dots, against a threshold of 8:

| pair | mesh gap | Bl0-Bl0 min | zone median | run dots | run mm |
|---|---|---|---|---|---|
| 1-2 | 0.000 | 0.306 | 0.675 | 49 | 28.800 |
| 1-8 | 0.000 | 0.090 | 0.467 | 66 | 39.001 |
| 2-9 | 0.001 | 0.192 | 0.494 | 20 | 11.401 |
| 3-5 | 0.000 | 0.064 | 0.662 | 21 | 12.000 |
| 6-7 | 0.000 | 0.046 | 0.344 | 66 | 39.000 |
| 7-9 | 0.001 | 0.102 | 0.216 | 21 | 11.999 |
| 5-7 | 0.000 | 0.230 | 0.659 | 55 | 32.399 |
| 4-7 | 0.000 | 0.072 | 0.266 | 41 | 24.001 |
| 6-9 (not a graph edge) | 0.000 | 0.265 | 0.591 | 27 | 16.640 |

line-meets-but-short does not fire (none of those nine has a run < 7 dots) and line-drifts-off-the-contact
does not fire (no touching pair has a Bl0-Bl0 min > 1.5 mm; the largest is 1-9 at 0.631). Two honest
qualifications, both for the lead: on 326's ten touching pairs the count is 7, not 9 (1-9 and 2-5 sit at
medians 1.187 and 1.797, 3-7 has no zone at all), so under that definition of "touching" no Part D limb
fires; and Part D's fourth limb read literally (any graph pair with mesh gap > 0.5 mm) fires on nine
graph edges — 1-5, 1-6, 3-7, 4-5, 4-6, 5-9, 6-8, 7-8, 8-9 — of which eight are the non-contacts 326
already lists at 3.60-21.38 mm and the ninth is 3-7, 326's own exception at 14.19.

**Part E's continuity check passes on the 1x bundle and cannot be run on the tree.** In 326's table the
numbers are 1-2 = 1.10 mm and 1-8 = 0.30 mm; this ticket's parenthetical writes them in the opposite
order. On the 1x bundle the reader gives 0.848 (1-2) and 0.165 (1-8) mm point-to-segment, and 1.103 and
0.304 mm with 326's own point-to-point metric — 0.003 and 0.004 mm agreement. On the tree the same pairs
are 0.306 and 0.090 mm; 0.306 is 0.794 mm from 1.10, so the tree cannot discharge that box.

**5-7 and 4-7 do not print a non-touching mesh gap: that Part E box fails as written.** They print
0.000090 and 0.000307 mm, and an independent vertex-to-vertex check (job 35041371) gives 144 vertex
pairs under 0.1 mm for 5-7 and 2255 under 0.5 mm — the same signature as the real joins 6-7 (0.000057)
and 1-8 (0.000124). Surface proximity cannot separate 5-7 from a join; its exclusion rests where ticket
344 left it, on the assembler's Overlap_ refusals (73.70/76.06/72.87). The render shows the two contact
zones shaded on top of each other. The other nine vertex-to-vertex non-contacts are 0.617-18.846 mm, so
the metric itself discriminates; 5-7 and 4-7 are the two cases where it cannot.

**Renders** (150 dpi, in C:/PR/artifacts/t347/renders/, looked at before the reading was written):
seam_1_8_contact.png (run 66 dots / 39.001 mm), seam_6_9_contact.png (27 / 16.640),
seam_5_7_negative.png (55 / 32.399, zones overlapping), seam_7_9_short.png (21 / 11.999). Camera along
the seam normal, one colour per piece's Bl0, contact zone shaded, the numbers in the title.

**Jobs** (all sacct State COMPLETED, ExitCode 0:0): 35041221 — first run, mesh gap had used the Bl0 dots
as the query set, discarded and kept in logs; 35041305 — final table, 61.7 s; 35041371 — independent
vertex-to-vertex sanity check. Logs, run.json, scale_check.json, san347.json and both tsvs are under
C:/PR/artifacts/t347/logs/. The report, the box-by-box Part E and attribution are in
C:/PR/artifacts/t347/pairs347.md and attribution.md.

**Caveats.** 1-4 was never measured by 326 and 5-7 / 4-7 are not graph edges, so their "touching (326)"
cell is a dash; 6-9 is reported separately because it is one of 326's ten but not an edge; 326's gap
column is point-to-point and this ticket's is point-to-segment, which is smaller by up to half a dot
spacing; the 1x table exists only for the continuity check; three jobs were run, not one, for the reasons
above. Not run: the assembler, and no threshold or source file was touched.

## Comments (worker follow-up, 2026-11-14)

Mating-face test requested at 12:42, run at the true pose. **Job 35041495**, sacct State **COMPLETED**,
ExitCode **0:0** (started 12:45:23; one sbatch job, 2 h / 4 cpus / 8G, staged under
/data/gpfs/projects/punim2657/SfSpp/stage347/, log mating-35041495.out, 33.7 s). No code, threshold or
other file was touched; this section was spliced in above the existing End state line, which is
unchanged.

Deliverables: C:/PR/artifacts/t347/mating347.tsv (7 rows, one per pair) and mating347.md; script
mating347.py; raw mating347.json in logs/. Distance is exact point-to-TRIANGLE, not nearest-vertex
(nearest-vertex counts are in the md and are 5-8x smaller). A = first piece of the pair.

| pair | n<0.1 mm | n<0.5 mm | dot median | p10 / p90 | frac dot <= -0.5 | patch bbox mm | frac verts A / B | inside frac |
|---|---|---|---|---|---|---|---|---|
| 5-7 | 459 | 1277 | -0.910 | -0.996 / -0.055 | 0.754 | 29.251 | 7.4% / 4.6% | 0.527 |
| 4-7 | 171 | 753 | +0.448 | -0.306 / +0.890 | 0.047 | 23.485 | 4.7% / 2.4% | 0.526 |
| 6-7 | 434 | 1125 | -0.940 | -0.998 / +0.006 | 0.776 | 30.323 | 8.5% / 5.2% | 0.396 |
| 1-8 | 220 | 595 | -0.887 | -0.996 / -0.027 | 0.773 | 31.860 | 0.9% / 10.2% | 0.536 |
| 1-5 | 0 | 0 | - | - | - | - | 0 / 0 | - |
| 1-6 | 0 | 0 | - | - | - | - | 0 / 0 | - |
| 4-5 | 0 | 0 | - | - | - | - | 0 / 0 | - |

What the numbers say: 5-7, 6-7 and 1-8 are back-to-back over 29-32 mm patches (75-78% of their matched
points at dot <= -0.5, 220-459 matched vertices each); 4-7 is not back-to-back (4.7% at dot <= -0.5, dot
median +0.448) although its surfaces pass within 0.1 mm over 23.5 mm, so proximity alone did not
discriminate it and this test does. The three known non-contacts (1-5, 1-6, 4-5) have nothing within
0.5 mm. On every near pair the inside fraction is 0.40-0.54, i.e. the two surfaces cross rather than nest
(ray-parity cross-check agrees with the signed-normal test on 100% of sampled points). The px3 frame
reproduces every count and reproduces the dot medians to 3.4e-14 and the patch bbox to 7.1e-14 mm.

That 5-7 carries the same signature as 6-7 and 1-8 is left as it stands against ticket 344's "not a
join"; no conclusion is drawn here.

### Lead — verification (2026-11-14, Thinkbook16)

The table reproduces: \`pairs347.tsv\` has 20 rows and the same numbers as \`pairs347.md\` (spot-checked
1-8 0.467 mm / 39.001 mm and 5-7 0.659 / 32.399 in both files).

verified: \`python artifacts/dsh-handover/347/accept347.py\` -> 30 cases, 30 pass. Three cases of the
lead's own were wrong and were fixed: the 326 pairing was written backwards, 6-9 is not one of the
table's rows, and the porcelain parse dropped a leading space.
verified: \`artifacts/t347/out347/scale_check.json\` -> |t| ratio 3.000000 and vertex deviation 1.4e-13 mm
for pieces 1, 8, 2; Bl0 dots 185/59, 126/40, 96/32.
verified: \`artifacts/t347/logs/slurm-35041305.out\`, \`san-35041371.out\` and the sacct record -> all three
jobs COMPLETED 0:0 (35041221 discarded, 35041305 the table and renders in 61.7 s, 35041371 the
vertex-to-vertex sanity in 2 s); the mating job 35041495 COMPLETED 0:0 in 33.7 s.
verified: \`artifacts/t347/mating347.tsv\` -> 5-7 459 / -0.910 / 0.754 / 29.251 mm; 6-7 434 / -0.940 /
0.776 / 30.323; 1-8 220 / -0.887 / 0.773 / 31.860; 4-7 171 / +0.448 / 0.047 / 23.485; 1-5, 1-6 and 4-5
zero at both thresholds.
verified: \`artifacts/t347/mating347.py:227-237\` -> \`dot = (Na * Nb).sum(1)\` over the matched vertices and
\`s = dot(p - q, n_B(q))\`, with each piece's outward normal calibrated by a parity test (the log prints
\`flipped False\` for 1, 4, 5, 6, 7, 8), so 4-7's +0.448 is a real reading and not a sign slip.

**Two acceptance boxes were wrong as written and are amended, not ticked.** (i) The 326 comparison
named the pairs in the wrong order: 326 gives 1-2 1.10 mm and 1-8 0.30 mm. (ii) The box asking 5-7 and
4-7 to print a non-touching gap cannot hold — they print 0.000090 and 0.000307 mm. Part A's literal
"same point count" limb also fails, for the documented reason (ticket 331 re-ran the chain at 3x with a
fixed 1.9 mm resample), so the frame is proven from the meshes and the poses instead.

**What the mating test settles, against the key's list.** \`Dataset/Juglet_GT/Juglet_simple_graph.txt\`
(md5 ccae01dd..., shipped 16 Sep 17:34 with the T matrices, copied by
\`run_juglet_preprocessing.sbatch:249\`) lists 18 edges. At the true pose **9 of them stand 3-19 mm
apart** and cannot be joints: 1-5 3.325, 1-6 3.636, 3-7 13.851, 4-5 4.864, 4-6 7.682, 5-9 17.987,
6-8 0.612, 7-8 13.871, 8-9 18.846 mm. **5-7 is not listed but mates** — back-to-back over 29.3 mm, the
same signature as the listed 6-7 and 1-8. **4-7 is not listed and does not mate** (normals parallel,
+0.448): proximity alone would have called it a join, and did. Of the twenty rows, 11 have a mesh gap
<= 0.1 mm; 9 of the key's 18 edges do.

Consequence, recorded in \`intent/S1-does-it-connect-anything-real.md\`: the sentence "5-7 is not a
join", written on 2026-11-14 from the key's edge list alone, is **wrong** — 5-7 is a join and 4-7 is
not. Ticket 344 (the bounded bar arm for 5-7) therefore keeps its premise, and stays stopped until the
join count is re-grounded on measured contacts, which is the next ticket.

Weight: one object, one pose set; the mating test is deterministic and the two frames agree to 1e-14;
the key's inconsistency is 9 of its 18 entries, not a threshold effect.

**End state:** ready — 20-row table, mating test, reader, both frames, four renders and logs are on
disk; the reading is line-runs-along-the-contact; two acceptance boxes were wrong as written and are
amended in the lead's section above; what the join denominator should be is the next ticket's question.
