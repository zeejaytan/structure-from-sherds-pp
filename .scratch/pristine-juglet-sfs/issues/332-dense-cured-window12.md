# Ticket 332 — Density on the cured tree, with the matching window doubled: the control 69 left open

**Status:** in-progress
**Working in:** C:/PR/worktrees/structure-from-sherds-pp/332
**Branch:** 332-dense-cured-window12
**Answers:** S1
**Depends on:** 328 (merged to preprocessing main 2026-11-12, `3ec34bd`) — the wall side is cured, so a density result is no longer confounded by half the joins being filed on the wrong face
**Blocks:** none
**Worker:** e4e2b266-3eb7-4c07-bab3-907a6a5ab1cc (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-12 02:20 by the lead)
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Ticket 69 landed the doubled edgeline density as env switches, default off, because it
> regressed the axis method on the 141 GT files — and it left one control unrun: the density had never been
> tried on a tree whose wall side is correct. Run that now, and at the same time double the matcher's LCS
> window from 6 to 12 dots so the doubling of the sampling does not also halve the physical window.

## Why this is a clean control, in numbers

- The join test is a longest-common-subsequence run along the two edge lines, and the run must exceed
  `windowsize` (`class/feature_matching.cpp:294`), which `main.cpp:211` passes as
  `MINIMUM_NUMBER` = **6** (`class/reconstruction.h:24`). So a join needs a run of **7 dots**.
- At the shipped spacing that is 7 x 1.9 = **12.6 mm** of edge. With the density doubled (0.95 mm spacing)
  the same 12.6 mm is **13 dots** — so the window must go from 6 to **12** (runs must exceed 12) for the
  physical requirement to stay put at ~12.4 mm.
- On the cured, sparse tree six of the nine touching pairs die at exactly this floor or just after it
  (ticket 329 bar 2, job 34725192): four never form a candidate (matched runs of 1-4 dots) and four form
  only upside-down fits at 141-175 deg.
- The density double was measured before the cure (ticket 68, job 34268623): dot counts went from
  59/32/44/31/43/45/54/40/22 to **123/68/94/66/90/94/112/84/49**, true-join candidates from 12/18 to 18/18,
  and the vessel still seated 0 of 9. Ticket 69 then merged it default off (commit `b8fed69`;
  `SFS_SPHERE_RADIUS_MM`, `SFS_RESAMPLE_D_MM`, `edgeline_extraction_headless.cpp:485` and `:574`).

**So the question is narrow and pre-registerable:** with the wall side fixed, does doubling the *sampling*
(and keeping the physical window) seat the juglet — or was the 2x density result pre-cure a statement about
the walls, not about the sampling?

## Part A — the dense chain run on the cured tree

Run the standard preprocessing chain on the nine Juglet pieces from **preprocessing main** (the cured tree,
preprocessing main at `676def2` or later), with the density switches on and **no code change**:

```
SFS_SPHERE_RADIUS_MM=0.9 SFS_RESAMPLE_D_MM=0.95
```

Everything else stays at shipped defaults. Report:
- dot counts per piece, against 123/68/94/66/90/94/112/84/49 (the pre-cure measurement) — they need not be
  identical, the mesh stage draws 30 unseeded ray-cast points (`mesh_processing_headless.cpp:1526-1527`),
  so state the run-to-run spread rather than claiming byte identity;
- piece 9 must be flagged if it stays under 50 dots: the authors' own rule excludes pieces under 50
  points (`papers/text/sfspp-2502.13986v1.md:329`), so its result is outside the method's stated domain;
- the axis directions against `Dataset/Juglet_GT/Juglet_Piece_<i>_Axis.xyz` (the GT key; the last 6
  numbers of the file are point xyz then direction xyz), because ticket 69's Part A is where the density
  cost appeared on pots and the same must be checked here.

Use your own build/scratch directory under your run directory. Do **not** build in, write to, or leave
artefacts in the canonical checkout `/data/gpfs/projects/punim2657/sfs_preprocessing` — another worker
(ticket 331) is running the same chain with shipped defaults at the same time, and the two must not share a
binary, a build tree or a run directory.

## Part B — the window, switchable, default unchanged

Add one switch in the assembler, one code path:

- `main.cpp:211` currently reads `FeatureComp(shard, LCS_out, 25, MINIMUM_NUMBER, 0);`. Replace the
  `MINIMUM_NUMBER` argument with a value read once from the environment, `SFS_LCS_WINDOW`, defaulting to
  `MINIMUM_NUMBER` when unset or unparseable. Do not change `MINIMUM_NUMBER` itself: it also gates
  registration at `class/feature_matching.cpp:1015` and `:1045` (`dummy_inlier > MINIMUM_NUMBER / 2`),
  and that gate must not move.
- The binary must print the effective window once at start-up (`LCS window: <n>`) so every log states its
  own regime.
- With the variable unset, the binary must produce the same results as today on a pot lane — verify on
  **Pot A** (8/8 sherds, 15/15 edges) rather than assuming it.

## Part C — three arms, two repeats each, on the juglet lane

Arm A — sparse lines, window 6 (today's behaviour; the control).
Arm B — dense lines (Part A), window 6 (the pre-cure density arm, re-measured after the cure).
Arm C — dense lines (Part A), window 12 (the ticket's ask: sampling doubled, physical window preserved).

Submit each arm twice (the assembler is not deterministic: Pot E produced 30/31+57/62 and 31/31+62/62 from
byte-identical inputs). Score with the existing scorer against `Dataset/Juglet_GT/` and report, per arm and
per repeat: sherds seated within **10 mm and 30 deg**, and the number of true joins realised out of the nine
touching pairs. Every job gets its poll and final `sacct` State/ExitCode in the ticket.

## Pre-registered reading (write this into the ticket before the first sbatch)

- **C seats >= 7 of 9** (and beats B and A): the failure was sampling, not window length or the walls.
  Density is the cure, and 69's default-off merge should be revisited for the juglet lane only.
- **B > A but C ~ A**: the window length is what binds; more dots only help when the required run is
  shorter. That is the size hypothesis, and ticket 331 is the direct test of it.
- **C ~ B ~ A**: neither sampling nor window length is the gate on the cured tree. The remaining suspect is
  the registration itself — four pairs fit upside-down at 141-175 deg, which is a transform-estimate
  failure, not an acceptance-threshold one — and the next ticket is about the fit, not the gates.

## Must not

- Change `RESAMPLE_D_MM` or `sphereRadius` in the code, or any constant in `sfs_axes`. The density is
  switched by environment only.
- Scale any millimetre or area gate by object size. Ticket 326 was dropped for exactly that; do not revive
  it here.
- Write into `Dataset/`, `original_samples/` or any existing corpus directory.
- Run a pot lane with `SFS_LCS_WINDOW` set. The pots are the untouched control; Pot A is run only with the
  variable unset.
- Commit or push. The lead verifies from artefacts and lands.

## Deliverables

Dot-count and axis tables, the three-arm score table with job ids and sacct states, the nine-touching-pair
join table per arm, and the Pot A neutrality run — all in the ticket Comments with exact paths.
