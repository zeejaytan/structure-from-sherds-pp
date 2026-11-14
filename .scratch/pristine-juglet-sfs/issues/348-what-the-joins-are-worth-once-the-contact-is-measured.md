# Ticket 348 — what the joins are worth once the contact is measured

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 348-what-the-joins-are-worth-once-the-contact-is-measured, in /c/PR/worktrees/structure-from-sherds-pp/348
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 347 (the mesh-gap and mating measurement), 326 (the gap at the true pose), 342 and 346 (the winners' edge lists), 338 (the candidate table)
**Waits for:** nothing — 347 landed 2026-11-14
**Blocks:** the candidate-generation ticket, and every future statement about join accuracy
**Budget:** small (2 failed checks)
**Hand-over:** DeepSeek: offline analysis of existing logs and meshes. No assembler change, no new run of the search.

> **In one line.** Every join number this lane has quoted is measured against an answer key whose
> edge list is not a list of contacts; replace the denominator with contacts measured from the
> meshes, say what the assemblies actually carried, and retire the old number.

## Why this is needed

Ticket 347 measured, at the true pose and in the 1x frame, the mesh gap of all 18 edges in
`Dataset/Juglet_GT/Juglet_simple_graph.txt` plus 5-7 and 4-7, and then tested whether the
surfaces actually mate (normals back-to-back) rather than merely pass close:

- **eight of the key's 18 edges stand 3.3-18.8 mm apart** (1-5, 1-6, 3-7, 4-5, 4-6, 5-9, 7-8, 8-9)
  and 6-8 stands 0.612 mm off with no contact patch at all — none of the nine can be joints at any pose;
- **5-7 is not listed but mates** (459 vertices within 0.1 mm, median dot -0.910, 75.4% opposed,
  29.3 mm of patch) — the same signature as the listed 6-7 and 1-8;
- **4-7 is not listed and does not mate** (+0.448, 4.7% opposed) although it passes within 0.1 mm;
- **6-9 touches** (0.000003 mm) and is not listed either.

So "1 of 18 edges" and "3 of 10 touching pairs" are both measured against a list that half-fails
the test it is being used for. The seated-sherd number (2 of 9) is a pose metric and does not
depend on the list — this ticket shows that too, rather than asserting it.

## Part A — build the measured contact set

For **all 36 unordered pairs** of the nine sherds, at the true pose in the 1x frame, measure with
exact point-to-triangle distances (the method of `C:/PR/artifacts/t347/mating347.py`, which 347
verified):

- `gap` — closest vertex-to-surface distance, mm;
- `n_lt_0.1` and `n_lt_0.5` — vertices of either piece within that distance of the other's surface;
- `dot_med`, `frac_back_to_back` — median and fraction at or below -0.5 of the normal dot on the
  pairs within 0.1 mm;
- `bbox_mm` — diagonal of the patch those pairs occupy;
- `inside` — the signed inside/outside split, as a sanity column.

**A pair is a measured join iff** `gap <= 0.05 mm` **and** `frac_back_to_back >= 0.40` **and**
`bbox_mm >= 8 mm` (sherd 9's whole diagonal is 23.2 mm, and 347's full seams span 11.4–39.0 mm
while grazing 4-7 fails on `frac_back_to_back` at 0.047). Any back-to-back contact with
`gap <= 0.05 mm`, `frac_back_to_back >= 0.40` and `3 mm <= bbox_mm < 8 mm` (such as 2-5, whose
347 edgeline run is 3.6 mm) is tagged `short-contact` in the table and quoted alongside the
`>= 8 mm` set. Apply the rule mechanically; do not adjust it to make a pair fit, and if it
fails a full seam 347 called a join (or passes 4-7), say so and stop — that is a finding, not a
number to tune.

## Part B — the table

One row per pair that is either a key edge, a measured join, or within 0.5 mm (about 20 rows),
columns:

```
pair  key_edge  gap  n_lt_0.1  n_lt_0.5  dot_med  frac_btb  bbox_mm  measured_join  t326_touching
```

`t326_touching` is ticket 326's ten-pair set (1-2, 1-8, 1-9, 2-5, 2-9, 3-5, 3-7, 6-7, 6-9, 7-9),
so the two definitions can be compared row by row. State the measured set as an explicit edge
list, sorted, in the write-up.

## Part C — re-score what the assemblies carried

For every winner on record — 342's r1 and r2 traces and 346's five trace arms (the
`[SFS-342] WIN` lines in `C:/PR/artifacts/t342/logs/` and `C:/PR/artifacts/t346/logs/`) —
classify every edge in the winner's list, by the pair it names, as:

- `measured-join` — in Part A's set;
- `key-only` — a key edge that is not a measured join (impossible to have seated);
- `neither` — not in the key, not measured.

Report per winner: the edge count, the split, and the number of **measured joins** carried.
Then report the two denominators side by side:

| quantity | against the key's list | against the measured set |
|---|---|---|
| touching / joinable pairs | 10 (326) or 18 (graph) | ? |
| joins a perfect search could seat from the 338 candidates | 3 of 10 | ? |
| joins the winning assembly carries | ? | ? |
| sherds seated (pose |rot| <= 5 deg, |mm| <= 5) | 2 of 9 | 2 of 9 (unchanged) |

The last row must be shown to be list-independent (it is a per-sherd pose test), or corrected
with the reason.

## Part D — the reading, exactly one

- `key-list-unusable` — 5 or more of the key's 18 edges are not measured joins **and** at least one
  measured join is unlisted: the list is retired as a scoring basis, Part A's set replaces it, and
  every earlier "x of 18" statement is marked superseded.
- `key-list-usable-with-caveats` — fewer than 5 key edges fail, but at least one does: quote which,
  and score both ways from now on.
- `mating-rule-too-strict` — the rule excludes a pair 347 measured as back-to-back (4-7 passing is
  the other direction and is `mating-rule-too-loose`); stop rather than widen the rule.
- `contact-set-unreliable` — a control (1-5, 1-6, 4-5) passes the rule, or the 1x frame disagrees
  with 347's numbers by more than 0.3 mm; stop.

## Part E — acceptance

- [x] all 36 pairs are measured, in the 1x frame, with one method and one script (quote its path);
- [x] the rule is applied as written; 4-7, 1-5, 1-6 and 4-5 are excluded and 5-7, 6-9 included —
  or the failure is reported as the reading;
- [x] the measured set is stated as an explicit sorted edge list;
- [x] the 1x gaps for 1-2, 1-8, 6-7, 5-7 and 4-7 agree with `C:/PR/artifacts/t347/pairs347.tsv`
  to within 0.05 mm (quote both numbers);
- [x] Part C covers all seven winner lists on record, with the split and the measured-join count;
- [x] the denominator table is filled with numbers, not adjectives, and the seated-sherd row is
  shown to be independent of the list;
- [x] one reading from Part D, with the rows quoted;
- [x] artefacts under `C:/PR/artifacts/t348/` (`contacts348.tsv`, `rescore348.md`, the script,
  `logs/`), and the write-back quotes the table;
- [x] `git diff --stat` shows only this ticket — no `class/`, no `hpc/`, no threshold.

## Rules for the worker

- `cd` into the worktree this ticket opens and work only there. Never edit the main clone, another
  worktree, or anything under `SfSpp_preprocessing/` except reading its artefacts.
- Never `git commit`, `git push`, `git worktree`, `git reset`. The lead commits.
- The meshes and poses are on Spartan under `/data/gpfs/projects/punim2657/`; heavy work runs on a
  compute node (one `sbatch`, 2 h, 4 cpus, `--mem=8G`), polled with
  `bash C:/PR/scripts/slurm_poll.sh <JOBID>`, final `sacct` recorded. Reading logs and computing
  on the laptop is allowed if it needs no more than the existing TSVs.
- Report, do not conclude: tables, the split, the one reading slug.
- If the same failure happens twice, stop and report incomplete, naming the blocker.

## Comments (worker, 2026-11-14)

One script, `C:/PR/artifacts/t348/measure348.py` (staged and run as
`/data/gpfs/projects/punim2657/SfSpp/stage348/measure348.py`), all 36 unordered pairs, exact
point-to-triangle distances in the 1x frame, true pose from `Dataset/Juglet_GT`.
Job **35043043**, `sacct`: `COMPLETED 0:0 00:17:49` (`artifacts/t348/logs/sacct.txt`), stderr
empty. Every piece's outward orientation calibrated independently by ray parity; **none of the
nine needed a flip**. The independent ray-parity cross-check agreed with the signed distance on
100% of sampled points in every pair that has a patch. The A-side patch statistics reproduce
347's exactly (5-7: 459 / -0.910 / 0.754; 6-7: 434 / -0.940 / 0.776; 1-8: 220 / -0.887 / 0.773;
4-7: 171 / +0.448 / 0.047), so the frame and method are the same ones 347 verified.

**Measured contact set (14, sorted):**

```
1-2  1-3  1-4  1-8  1-9  2-6  2-9  3-5  3-8  5-6  5-7  5-8  6-7  6-9
```

**Short contact (not in the set):** `2-8` — gap 0.000339, frac 0.955, bbox 4.364 mm.

Rule applied as written: `4-7` (0.000307 mm, +0.457, 5.7% opposed), `1-5` (3.325), `1-6` (3.636)
and `4-5` (4.864) are excluded; `5-7` (0.000090, 0.755, 29.422) and `6-9` (0.000003, 0.891,
15.191) are included — exactly the pairs the ticket named. No pair 347 measured back-to-back is
excluded, and 4-7 does not pass, so neither `mating-rule-too-strict` nor `mating-rule-too-loose`
applies. Two boundary results are stated in `rescore348.md`: `2-9` passes with bbox 8.050 mm
(0.6% above the 8 mm floor), and the ticket's short-contact example `2-5` does **not** qualify
(bbox 2.592, 0.0% opposed — 347's 3.6 mm was an edgeline run, not this patch).

1x agreement with `C:/PR/artifacts/t347/pairs347.tsv`: 1-2 0.000085 vs 0.000; 1-8 0.000124 vs
0.000; 6-7 0.000057 vs 0.000; 5-7 0.000090 vs 0.000; 4-7 0.000307 vs 0.000 — largest difference
0.0003 mm against a 0.05 mm tolerance.

| pair | key | gap (mm) | n<0.1 | frac_btb | bbox (mm) | verdict |
|---|---|---|---|---|---|---|
| 1-2 | yes | 0.000085 | 596 | 0.913 | 25.189 | measured join |
| 1-3 | no | 0.000444 | 320 | 0.953 | 14.766 | measured join (unlisted) |
| 1-4 | yes | 0.000093 | 401 | 0.825 | 18.500 | measured join |
| 1-8 | yes | 0.000124 | 676 | 0.709 | 31.987 | measured join |
| 1-9 | yes | 0.000002 | 343 | 0.895 | 15.005 | measured join |
| 2-5 | yes | 0.001712 | 37 | 0.000 | 2.592 | touching, not opposed |
| 2-6 | no | 0.000068 | 485 | 0.734 | 21.320 | measured join (unlisted) |
| 2-8 | no | 0.000339 | 110 | 0.955 | 4.364 | short contact |
| 2-9 | yes | 0.000679 | 300 | 0.897 | 8.050 | measured join (barely) |
| 3-5 | yes | 0.000177 | 271 | 0.742 | 12.520 | measured join |
| 3-8 | no | 0.000222 | 611 | 0.741 | 23.548 | measured join (unlisted) |
| 4-7 | no | 0.000307 | 369 | 0.057 | 23.720 | touching, faces away |
| 5-6 | no | 0.000231 | 413 | 0.683 | 15.948 | measured join (unlisted) |
| 5-7 | no | 0.000090 | 845 | 0.755 | 29.422 | measured join (unlisted) |
| 5-8 | no | 0.000054 | 305 | 0.685 | 12.478 | measured join (unlisted) |
| 6-7 | yes | 0.000057 | 869 | 0.776 | 30.436 | measured join |
| 6-9 | no | 0.000003 | 530 | 0.891 | 15.191 | measured join (unlisted) |
| 7-9 | yes | 0.000859 | 135 | 0.141 | 9.566 | touching, not opposed |

Part C — seven winner lists, three distinct rows (346's five arms carry the same four edges):

| winner | edges | measured join (in key) | measured join (unlisted) | key-only | neither | measured joins carried |
|---|---|---|---|---|---|---|
| 342 r1 | 4 | 1 (1-8) | 2 (5-7, 3-8) | 1 (4-6) | 0 | 3 of 4 |
| 342 r2 | 5 | 2 (1-8, 2-9) | 2 (5-7, 3-8) | 0 | 1 (4-7) | 4 of 5 |
| 346 r1-r4, probe | 4 | 1 (1-8) | 2 (5-7, 3-8) | 1 (4-6) | 0 | 3 of 4 |

| quantity | against the key's list | against the measured set |
|---|---|---|
| touching / joinable pairs | 18 edges; 10 pairs (326) | 14 (of 36, 7 key edges) |
| joins a perfect search could seat from the 338 candidates | 3 of 10 | 5 of 29 distinct candidate pairs meet the seated tolerance (1-2, 1-8, 4-7, 5-7, 6-9); 4 of the 5 are measured joins |
| joins the winning assembly carries | 1 of 4, 2 of 5, 1 of 4 | 3 of 4, 4 of 5, 3 of 4 |
| sherds seated | 2 of 9 | 2 of 9 (unchanged) |

The seated-sherd row is list-independent: it is a per-piece pose test (each piece's estimated
transform against ground truth), not a count over either edge list — so the edge list's coverage
does not predict it, and the 338-candidate table shows the independence directly (4-7 meets the
pose tolerance while not being a joint at all).

Two numbers that move: key edges the measurement supports are **7 of 18**, not 1 of 18; ticket
326's touching pairs that are real joins are **7 of 10**, not 3 of 10. Eleven key edges fail
(1-5, 1-6, 2-5, 3-7, 4-5, 4-6, 5-9, 6-8, 7-8, 7-9, 8-9) and seven measured joins are unlisted
(1-3, 2-6, 3-8, 5-6, 5-7, 5-8, 6-9).

A look at the patches (`renders/renders348.png`, job 35043290, `COMPLETED`) confirms the
shape reading: all 14 measured joins are long thin curve-following bands with the two pieces'
points interleaved (8.0-32.0 mm), `2-8` is a 4.4 mm scatter, `7-9` is a single rim line, and
`4-7` is the deceptive one — a 23.7 mm band of coincident surfaces facing the same way.

Two side observations, numbers only, left for the lead: (i) ticket 326 records 7-9 at 1.79 mm
while exact point-to-triangle distance is 0.000859 mm, and 7-9 is the one key edge absent from
`pairs347.tsv`; (ii) the key's nine "standing apart" edges are eight: 6-8 is 0.612 mm apart
(347 and this run agree) and fails on contact area, not distance. Piece 5's measured partners
are 3, 6, 7 and 8, while the key pairs it with 1, 2, 4 and 9.

Artefacts: `C:/PR/artifacts/t348/` — `measure348.py`, `contacts348.tsv`, `rescore348.md`,
`logs/` (Slurm out/err, `sacct.txt`, `contacts348.json`), `render348.py` + `run348r.sbatch`
and their `logs/`. No file under `class/` or `hpc/` was touched; no threshold or switch changed;
the assembler was not re-run.

Self-check: `accept348.py` reports 23 cases, 23 pass, 0 fail (`artifacts/t348/logs/accept348.txt`).
One case (`only-ticket-changed-in-repo`) needs `COLUMNS=200` to pass: at the default 80 columns
git truncates the ticket's own path to `...joins-are-worth-once-the-contact-is-measured.md`, so
the check cannot see `issues/348-` in the line it is testing — the diff itself is the ticket only
(`git diff --name-only HEAD` lists that one file).

Reading: key-list-unusable

### Lead — verification (2026-11-14, Thinkbook16)

The lead re-ran the check and re-derived the contact set from the table itself, rather than reading
the worker's arithmetic, before recording anything.

verified: `cd worktrees/structure-from-sherds-pp/348 && COLUMNS=200 python artifacts/dsh-handover/348/accept348.py` -> 23 cases, 23 pass, 0 fail. The `COLUMNS` note holds; `git diff --name-only HEAD` names the ticket alone.
verified: `artifacts/t348/contacts348.tsv` -> 36 rows, one per unordered pair, header as specified.
verified: `python artifacts/t348/lead_check348.py` (the lead's own re-application of the rule to the table, written after the return) -> 14 joins (1-2 1-3 1-4 1-8 1-9 2-6 2-9 3-5 3-8 5-6 5-7 5-8 6-7 6-9), short contact 2-8, key-and-join 7 (1-2 1-4 1-8 1-9 2-9 3-5 6-7), key-only 11 (1-5 1-6 2-5 3-7 4-5 4-6 5-9 6-8 7-8 7-9 8-9), measured-only 7 (1-3 2-6 3-8 5-6 5-7 5-8 6-9). The worker's list and counts are exact.
verified: `artifacts/t348/measure348.py:48-50` -> `GAP_MAX = 0.05`, `FRAC_MIN = 0.40`, `BBOX_MIN = 8.0`, with `SHORT_MIN = 3.0` for the short-contact tag: the rule the ticket wrote, not a widened one.
verified: winner classification against the logged edge lists (342 r1 5-7 #160, 1-8 #67, 3-8 #97, 4-6 #77; 342 r2 adds 4-7 #86 and 2-9 #51; 346's five arms 5-7, 1-8, 3-8, 4-6) -> 3 of 4, 4 of 5, 3 of 4 measured joins carried; 4-7 is neither; 4-6 is key-only.
verified: `artifacts/t348/logs/sacct.txt` -> 35043043 `COMPLETED 0:0 00:17:49`, 35043290 `COMPLETED 0:0 00:01:58`.
verified: `artifacts/t348/renders/renders348.png` looked at -> the 14 joins are interleaved bands 8-32 mm long along one curve; 4-7 is two offset chains along the same curve; 2-5, 2-8, 3-4 and 4-9 are loose scatters with no band. The picture matches the table.
verified: `git diff --stat HEAD` in the worktree -> the ticket only, nothing under `class/` or `hpc/`.

Adjudications the lead recorded:

- The body's own "nine ... stand 3-19 mm apart" was wrong for 6-8 (0.612 mm, no contact patch); it now reads
  eight at 3.3-18.8 mm with 6-8 separated out. `intent/S1` carries the same correction.
- **7-9**: gap 0.000859 mm but 14.1% opposed and median dot -0.187 — the pieces graze along one rim
  line, they do not mate. Ticket 326's 1.79 mm is a coarser number for the same pair, not a contradiction.
  7-9 is absent from `pairs347.tsv`, so 347 never scored it.
- **2-9** passes at bbox 8.050 mm, 0.6% above the floor: the fragile verdict of the set, and the one
  to quote whenever the 14 is used.
- **4-6** is carried by 342's r1 winner and by 346's arms while the meshes stand 7.68 mm apart. Recorded
  as a lead for the next ticket, not as a defect of this one: the assembly joins a pair the key lists
  and the meshes say is not a join.
**End state:** ready — verified by the lead, landed and closed.
