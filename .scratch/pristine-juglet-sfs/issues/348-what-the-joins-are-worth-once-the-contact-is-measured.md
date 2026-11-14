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

- **nine of the key's 18 edges stand 3-19 mm apart** (1-5, 1-6, 3-7, 4-5, 4-6, 5-9, 6-8, 7-8, 8-9)
  and cannot be joints at any pose;
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

- [ ] all 36 pairs are measured, in the 1x frame, with one method and one script (quote its path);
- [ ] the rule is applied as written; 4-7, 1-5, 1-6 and 4-5 are excluded and 5-7, 6-9 included —
  or the failure is reported as the reading;
- [ ] the measured set is stated as an explicit sorted edge list;
- [ ] the 1x gaps for 1-2, 1-8, 6-7, 5-7 and 4-7 agree with `C:/PR/artifacts/t347/pairs347.tsv`
  to within 0.05 mm (quote both numbers);
- [ ] Part C covers all seven winner lists on record, with the split and the measured-join count;
- [ ] the denominator table is filled with numbers, not adjectives, and the seated-sherd row is
  shown to be independent of the list;
- [ ] one reading from Part D, with the rows quoted;
- [ ] artefacts under `C:/PR/artifacts/t348/` (`contacts348.tsv`, `rescore348.md`, the script,
  `logs/`), and the write-back quotes the table;
- [ ] `git diff --stat` shows only this ticket — no `class/`, no `hpc/`, no threshold.

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
