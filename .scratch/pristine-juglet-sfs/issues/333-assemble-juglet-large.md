# Ticket 333 — assemble juglet_large at 3x: do the recovered dots become joins?

**Status:** landed
**Working in:** chat on Thinkbook16 (2026-11-12)
**Branch:** 333-assemble-juglet-large, in /c/PR/worktrees/structure-from-sherds-pp/333
**Answers:** S1
**Depends on:** 331 (the 3x input is built and measured), 332 (the dense 1x control on the cured tree)
**Blocks:** none
**Worker:** 7bf2cc0a-6107-49f9-82c8-a015e65ee402 (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-12 01:10Z by the lead)
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)

> **In one line.** Ticket 331 enlarged the Juglet 3x: every piece now carries 77-185 dots and 7 of 9 seams
> clear the matcher's 7-dot / 12.6 mm window on the same binary. This ticket hands that input to the
> assembler, unmodified, twice, plus one diagnostic arm with the agreement gate at its scale-equivalent
> 4.5 mm - and asks whether the recovered dots become joins.

## Why this is the right next test (facts, all measured)

- **The 1x baseline does not move with density.** Ticket 332 ran twelve arms on the cured 1x tree: sparse
  enumerates 6 of 9 pairs, dense enumerates 9 of 9, and the number of pairs that pass is **1 of 9 in every
  arm** (0 of 9 once), with 5-8 of the pairs fitting upside down (>90 deg). 1x score: **2/9 sherds, 1/18 edges**
  (`work/gt/trees/base18`, run `20261112-0245_30b80ae_t332-dense-window12`).
- **The 3x input clears the window.** Ticket 331 (run `20261112-0218_b1e1580_t331-large`) gives
  `185/96/125/102/150/141/174/126/77` Breakline_0 dots at 1.800 mm spacing, and **7 of 9** touching pairs
  reach >=7 dots and >=12.6 mm at 1.5 mm (8 of 9 at the scale-equivalent 4.5 mm; fail 1-9, 2-5 at 1.5 mm;
  only 2-5 fails at 4.5 mm). Non-touching pairs: 0 dots. The lead reproduced three pairs independently.
- **The assembler's gates are absolute millimetres**, so at 3x three of them are three times stricter
  relative to the vessel: agreement `INLIER_THRESHOLD 1.5` mm (`class/reconstruction.h`), overlap
  `OverlapCheck_3d(50, ...)` mm^2, merge `20` mm; the angular gate `0.262` rad is scale-free. That is
  exactly why this ticket needs the second arm.

## Part A — build the 3x tree (do not re-run the chain)

Use ticket 331's own outputs; do not re-run sampling/meshing/edgeline. Sources:

| tree slot | from |
|---|---|
| `Breaklines/Juglet_Piece_N_Breakline_0.pcd` | `.../20261112-0218_b1e1580_t331-large/outlarge_jug/geom/Juglet_Piece_N.Bl0` (only Bl0 - the assembler never reads Bl1) |
| `Surfaces/Juglet_Piece_N_Surface_{0,1}.xyz` | `.../outlarge_jug/geom/Juglet_Piece_N.S{0,1}` |
| `Axes/Juglet_Piece_N_Axis.xyz` | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Juglet_Piece_N_Axis.xyz` (GT, already 3x) |
| `Ground Truth/Juglet_Piece_N_T.txt` | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Juglet_Piece_N_T.txt` (already 3x) |
| graph | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Juglet_simple_graph.txt` |
| `Mesh/Juglet_Piece_N_Mesh.obj` | `/data/gpfs/projects/punim2657/Dataset/Juglet_large/Piece0N.obj` |

Pin md5s of every file you copy into `pins/tree333.md5`, and record the graph md5
(`ccae01dd9c897e92beb0d507baa45cea`, identical to the 1x copy). Follow the tree layout ticket 332's
`work/gt/trees/base18/juglet` uses - read it first and mirror it.

**Use the GT axes for both arms.** The question is whether scale turns dots into joins, not whether the
estimator is good; ticket 331's estimator axes exist (`axes_large/`) and may be recorded as a one-line note,
not used. Do not add a third axis arm.

## Part B — control, same binary, before anything is measured

Pot A through the very same binary and shim, with the new switch unset, must reproduce **8/8 sherds and
15/15 edges, twice**. If it does not, the run is **void** - say so and stop; do not adjust anything.

## Part C — the arms

- **Arm 1 (main): shipped gates, unmodified binary.** Two repeats on the 3x tree.
- **Arm 2 (diagnostic): `SFS_INLIER_MM=4.5`.** Two repeats on the same tree. The switch must be one code
  path: read once at start-up, default `INLIER_THRESHOLD` (1.5) when unset/empty/unparseable/outside a sane
  range, print once as a banner (`inlier gate: <x> mm`), and be used everywhere `INLIER_THRESHOLD` is
  consumed today. Nothing else changes - no overlap, merge or angle constant moves.

Everything else identical between arms: same tree, same binary, same shim, same count, same seeds/floors.
Arm 1 must show the banner value 1.5; arm 2 must show 4.5.

## Part D — measure and report

Score with CountResult (0.35 rad / 50 mm), as in 332. Per arm and repeat, report:

- sherds placed / 9, edges recovered / 18, which pairs, and which admitted joins are **not** GT;
- the upside-down count among admitted pairs, and the candidate count before the gates;
- the counts the arm's own log prints, plus per-pair degrees/mm from the `per_edge` JSON;
- each job id with its final `sacct` State/ExitCode (poll `sacct` directly, never `slurm_poll.sh`).

## Part E — pre-registered reading (before any job is submitted)

1. **Arm 1 >= 4/9 sherds and >= 8/18 edges, no non-GT join** -> the scale cure works at shipped gates; the
   window was the binder, the millimetre gates are not what 3x broke.
2. **Arm 1 ~ 1x but arm 2 markedly better** -> the window is fixed and the agreement gate (0.5 mm of the
   real vessel at 3x) is the remaining binder; the next ticket makes the mm gates scale-aware. **Do not
   edit any constant in this ticket.**
3. **Both arms ~ 1x and upside-down persists** -> scale does not fix assembly; the binder is the pose
   estimate/registration, and the scale line stops here.

Report the numbers first and the reading second. A shortfall is reported as a shortfall.

## Acceptance

- Pot A 8/8 + 15/15 twice, switch unset, same binary.
- Two repeats per arm; every job id with final sacct State/ExitCode.
- The per-arm table against the 1x reference (2/9 sherds, 1/18 edges; 332 `base18`), every number
  traceable to a named file.
- Consolidated assembly meshes / snapshots exported for the lead to eye (do not judge the pictures
  yourself; the lead renders and rules).

## Must not

- No change to any gate in arm 1; no scale-aware gate work here (that was ticket 329, wontfix, and would be
  its own ticket).
- No corpus write (`Dataset/` and `runs/` stay read-only apart from this run's own directory).
- No edit to ticket 331's run dir; no re-run of the chain; no change to the axis estimator.
- Do not quote ticket 331's own control column as the 1x baseline - piece 1 is anomalous there (lead block,
  ticket 331); the 1x reference is ticket 332's `base18` trees.
- No commit, no push, no branch or worktree command; the ticket is edited only by appending
  `## Comments (worker, <date>)` and flipping `Status:` to `results-in`.

## Where it lives

- Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t333-juglet-large/`, built from
  `hpc/scripts/new_run.sh` style layout (`run.json`, `submit.sbatch`, `logs/`, `out/`).
- Templates to read first: `.../20261112-0245_30b80ae_t332-dense-window12/` (tree layout, scoring, arms) and
  `.../20261110-0320_41494d0_t320b-juglet/submit.sbatch` (the juglet assembler job: SIF
  `sfs_main/sfs_pristine_assembly.sif`, `-DJUGLET_ORIG`, `juglet_orig_block.inc`, FLOOR=20, NP=9).

## Comments (worker, 2026-11-12)

### Pre-registration — written before any job was submitted

Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1238_55473e9_t333-juglet-large`
(git7 `55473e9`, branch `333-assemble-juglet-large`). Nothing has been submitted when this is written.

**Fixed for every run, so the arms differ in one thing only.** Same tree (`work/trees/px3/juglet`,
55 files pinned in `work/pins/tree333.md5`), same binary (built from a copy of `/opt/sfs-pristine` with
`src/gate333.patch` applied; every other object file md5-compared against an unpatched build, see
`logs/objects_build_orig.md5` vs `logs/objects_build.md5`), same shim (branch HEAD
`hpc/tools/main_pristine_batch.cpp` + `src/shim333.patch`, the two diagnostic hunks only), FLOOR=20,
NP=9, one `OMP_NUM_THREADS=4` run per repeat, one tree copy per repeat. Arm 1 leaves
`SFS_INLIER_MM` unset (gate 1.5 mm, the shipped value); arm 2 sets `SFS_INLIER_MM=4.5`
(gate 4.5 mm, the 1x-equivalent distance at 3x). No other constant, none of the overlap/merge/angle
gates, is touched.

**What will be counted, and from where.** Sherds placed /9 and edges recovered /18 from the binary's
own `Sherd accuracy : k / 9` and `Sherd accuracy : k / 18` lines (`score/1.Acc_<label>.txt`,
recomputed independently by `work/score58.py` into `score/<label>.json`), and that recomputation is
the number used. Admitted joins = the pairs printed by `[T333-PAIR]` after `Total number pruned`
(pairs that survived `PairwisePruning`; a non-GT admitted pair is an admitted pair not among the nine
GT pairs). Merged clusters = `[T333-CLUSTER]` lines. Candidate count before the gates = the
`Total number :` line. Upside-down = admitted pairs whose relative rotation is >90 deg in
`score/<label>.json` `tables.per_edge`. 1x reference = ticket 332 `base18` run `gt18_r1`:
2/9 sherds, 1/18 edges (`score/acc_gt18_r1.row`, `out/gt18_r1/1. Acc.txt` in run
`20261112-0245_30b80ae_t332-dense-window12`).

**The reading, registered now, applied unchanged afterwards.**

1. **Arm 1 at least 4 of 9 sherds and at least 8 of 18 edges, and no admitted join outside the GT
   graph** -> the scale cure works at the shipped gates, and the dots recovered in 331 are enough for
   the assembler as it stands.
2. **Arm 1 near 1x (2/9, 1/18) but arm 2 markedly better** -> "markedly better" means, on the two
   repeats taken together, arm 2 gains at least +2 sherds or +4 edges over arm 1 without losing
   ground on the other count. Then the dot window was the thing the denser 3x dots fixed and the
   absolute agreement gate (1.5 mm, i.e. 0.5 mm of real vessel at 3x) is the remaining binder: the
   next ticket makes the millimetre gates scale-aware. No constant is edited in this ticket.
3. **Both arms near 1x and the upside-down fits persist** -> scale does not fix assembly; the binder
   is the pose estimate / registration, and the scale line stops here.

A shortfall is reported as a shortfall. The pictures are exported for the lead and are not judged by
the worker.

### Results — job 34910751, one job, six arms

**Run and provenance.** Run dir `/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1238_55473e9_t333-juglet-large`,
worktree HEAD `55473e9`, branch `333-assemble-juglet-large`. `sacct -j 34910751 --format=JobID,State,ExitCode,Elapsed`:
`34910751 COMPLETED 0:0 01:48:17` (batch and extern step both `COMPLETED 0:0`). The tree is the one staged
from 331's own outputs, 55 files pinned in `work/pins/tree333.md5`, `md5sum -c` rc=0 with 0 FAILED inside the
job (`logs/tree_pins_check.txt`); Breakline_0 dot counts 185/96/125/102/150/141/174/126/77 reproduce ticket 331.
The binary is `/opt/sfs-pristine` + `src/gate333.patch` + `src/shim333.patch` (two hunks, diagnostics only);
a second build of the unpatched source was md5-compared object by object and **exactly one object differs**,
`CMakeFiles/Hierarchy-Clear.dir/class/reconstruction.cpp.o` (7a0a160d… shipped vs 221bb8d5… this build), logged in
`logs/objects_build_orig.md5` / `logs/objects_build.md5`. Patched file md5s: `psrc/class/reconstruction.cpp`
3167f050aba2b473e96841bda5081fa7, `psrc/class/reconstruction.h` 96d9af21bba2d92047a224dbad7ef1e3,
`asm/hpc/tools/main_pristine_batch.cpp` bef4886cdb4f978a4dc15e3d72d5b117. The switch prints as the first line
of every log; the banners read `inlier gate: 1.5 mm` (control, arm 1) and `inlier gate: 4.5 mm` (arm 2), one line
each in `logs/banner_<label>.txt`. No other gate constant was touched (overlap 50 mm^2, merge 20 mm, angle
0.262 rad, axis 0.436 rad all unmodified).

**Part B — control, run before any juglet number was measured.** Pot A through the same binary and shim with
`SFS_INLIER_MM` unset: `potA_r1 8/8 15/15` (wall 441 s), `potA_r2 8/8 15/15` (wall 1060 s); both banner
`inlier gate: 1.5 mm`; `score/acc_potA_r1.row`, `score/acc_potA_r2.row`, `out/potA_r*/1. Acc.txt`,
`logs/panel.tsv`. The control passed on the first submission that reached it, so the run is not void and the
juglet arms started.

**Part C/D — the four juglet arms** (`score/summary.json`, `logs/panel.tsv`, `logs/binary_<label>.log`):

| arm | gate | cand. before gates | admitted (log / unique / non-GT) | final clusters | sherds | edges | wall |
|---|---|---|---|---|---|---|---|
| 1x ref `gt18_r1` | 1.5 mm at 1x | 32 | 12 / – / – | – | **2/9** | **1/18** | 186 s |
| `arm1_r1` | 1.5 mm | 180 | 79 / 30 / 22 | {6,9} {2,4,5,7} {1,3,8} | **0/9** | **0/18** | 673 s |
| `arm1_r2` | 1.5 mm | 180 | 79 / 30 / 22 | {6,9} {2,4,5,7} {1,3,8} | **0/9** | **0/18** | 1382 s |
| `arm2_r1` | 4.5 mm | 180 | 79 / 30 / 22 | {3,4} {5,7} {1,8} | **2/9** | **1/18** | 2570 s |
| `arm2_r2` | 4.5 mm | 180 | 79 / 30 / 22 | {3,4} {5,7} {1,2,8} | **2/9** | **1/18** | 288 s |

Every juglet arm saw the same 180 candidates and admitted the same 30 distinct pairs (22 of them outside the
GT graph); the only GT pair never admitted is 6-7. The repeats agree exactly at the shipped gate — same
clusters, and `per_edge` angles/translations identical to 0.1 mm (`score/arm1_r1.json` vs `arm1_r2.json`) — so
the repeats demonstrate determinism, not seed robustness; arm 2 does differ between repeats in which pieces
are placed (6 vs 7) and in wall time (2570 s vs 288 s) while the score stays 2/9 and 1/18.

**The one GT pair that has ever passed, at all three settings** (`tables.per_edge`, directed pair 1-8):

| setting | 1-8 angle | 1-8 translation | passes 0.35 rad / 50 mm |
|---|---|---|---|
| 1x (`gt18_r1`) | 4.32 deg | 33.18 mm | yes |
| arm 1 (`arm1_r1`) | 2.00 deg | 53.5 mm | **no, 3.5 mm over** |
| arm 2 (`arm2_r1`) | 0.65 deg | 17.10 mm | yes |

All other GT pairs are hundreds to thousands of millimetres out in every arm; 5 of the GT pairs measured in
arm 1 sit upside down (>90 deg: 1-4, 2-5, 3-5, 6-7, 7-9), 1 (3-5) in `arm2_r1`, 3 in `arm2_r2` — the count drops
in arm 2 only because it places 6 and 7 pieces, so fewer GT pairs are measured at all.

**A calibration fact that voids one clause of the pre-registration.** The Pot A control scores 8/8 and 15/15
while its own admitted list holds 70 entries / 26 unique pairs, 21 of them outside its GT graph, and **four GT
pairs never admitted at all** (1-8, 1-9, 2-9, 7-9) — and it still recovers 15 of 15 edges. The admitted-pair list
is therefore neither necessary nor sufficient for a correct join: joins are made between accumulated
sub-graphs, not between two pieces. So clause 3 of reading 1 ("no admitted join outside the GT graph") cannot
be used as a success test, and the non-GT admitted counts in the table above are not evidence of failure by
themselves. Reading 1 is judged on its two counts.

### The reading, applied unchanged

**Reading 1 — fails.** 0 of 9 sherds, 0 of 18 edges, against the required >=4/9 and >=8/18.

**Reading 2 — the direction holds, the level does not.** Arm 2 does gain on arm 1 by +2 sherds and +1 edge
without losing ground, which is the letter of "markedly better"; but arm 1 was not "near 1x", it was below it,
and arm 2 lands exactly on the 1x score (2/9, 1/18) with the same single correct pair. Nothing exceeded 1x.
So the agreement gate does decide the marginal pair — at 4.5 mm the only correct pair comes back, at 1.5 mm at
3x it is 3.5 mm outside the bar — but recovering it buys exactly the 1x result, no more. The prediction that
a scale-aware millimetre gate would let the 331 dots become *additional* joins is not supported.

**Reading 3 — holds in substance.** The upside-down fits persist (5 GT pairs at >90 deg in arm 1), and scale
does not fix assembly: 5.6x the candidates (180 vs 32) and 30 of the 36 possible pairs admitted produced the
same one correct pair as 1x. The binder is the pose estimate/registration, not the number of dots. The scale
line stops here.

**A caveat on the ruler, not an edit to it.** The 3x juglet measures 124.5 x 110.3 x 195.0 mm in its GT frame
(1x equivalent 41.5 x 36.8 x 65.0 mm, computed from the nine `Piece0N.obj` + `Juglet_Piece_N_T.txt`). All
three gates in the scoring path (50 mm position, 10 mm strict, 4.6 mm seat) are absolute millimetres, so at
3x the 50 mm bar is 26% of the vessel height where at 1x it was 77%. Arm 1's only correct pair, 1-8, is
53.5 mm out — 27% of the 3x vessel height, against 51% for the 1x pair that passed. In vessel-relative terms
the 3x solve of that pair is the better one and it still fails, because the bar did not move. So "0/9" at the
shipped gate is the ruler plus the state selection, not a collapse of the geometry; it is reported as the
shipped-gate number and not restated as a pass.

**Deviations, stated.** (1) Both arms use the GT axes from `Dataset/Juglet_large`; 331's estimator axes in
`axes_large/` were not used, so no third arm exists. (2) The five submissions before 34910751 produced no
number: 34909395 FAILED 3:0, 34909738 FAILED 3:0, 34909890 FAILED 3:0, 34910106 FAILED 4:0, 34910189
CANCELLED — each aborted at stage build/stage/paths, none reached a measurement. (3) The assembler never
prints which pieces it merged with which; cluster membership comes from the new `[T333-CLUSTER]` dump, which
is a diagnostic hunk in the shim, not in the assembler proper.

**Exported for the lead's eye (not judged by the worker).** One consolidated OBJ per run, one group per placed
piece: `out/arm1_r1/consolidated.obj` (9 pieces, 60334 verts), `out/arm1_r2/consolidated.obj` (9, 60334),
`out/arm2_r1/consolidated.obj` (6, 50549), `out/arm2_r2/consolidated.obj` (7, 53114). Two-view snapshots:
`renders/arm1_r1.png`, `arm1_r2.png`, `arm2_r1.png`, `arm2_r2.png` in the run dir, fetched to
`C:/PR/artifacts/t333/renders/`. The arm-1 snapshot shows the three clusters standing apart as three separate
groups, which matches the cluster dump.


## Lead verification (2026-11-12, the coordinating session)

Read back independently before anything was recorded.

**Jobs.** `sacct -j 34910751,34915095,34909395,34909738,34909890,34910106,34910189 -X` -> 34910751 COMPLETED 0:0
01:48:17; 34915095 COMPLETED 0:0 00:00:15; 34909395/34909738/34909890 FAILED 3:0 (3 s, 3 s, 1:04); 34910106
FAILED 4:0 (1:12); 34910189 CANCELLED+ 0:0 (2:07). As reported.

**Numbers, re-read from `score/summary.json` (not from the report).** potA_r1 and potA_r2: sherds 8/8, edges
15/15, banner `inlier gate: 1.5 mm`, 124 candidates, 1 graph. arm1_r1 and arm1_r2: 0/9 and 0/18, banner 1.5 mm,
180 candidates, 79 admitted log entries / 30 unique pairs / 22 non-GT, clusters {6,9}{2,4,5,7}{1,3,8}, GT pair
6-7 the only one never admitted. arm2_r1: 2/9 and 1/18, banner 4.5 mm, same 180 candidates and the same 30
admitted pairs, clusters {3,4}{5,7}{1,8}. arm2_r2: 2/9 and 1/18, clusters {3,4}{5,7}{1,2,8}.

**Two corrections to the report.** (1) arm2_r2's third cluster is {1,2,8}, not {1,8}: the same score hides a
different mistake, an admitted 1-2 join that is not in the GT graph. (2) The per-edge numbers I can verify are
the 3x ones: arm1 1-8 = 1.99 deg / 53.5 mm (both repeats, 53.5 and 53.5); arm2_r1 1-8 = 0.65 deg / 17.1 mm;
arm2_r2 1-8 = 1.20 deg / 32.2 mm. The 1x figures quoted in the report (4.32 deg / 33.18 mm) come from the
worker's own diagnostic, not from a file I read; the 1x claim I verified is ticket 332's scored 2/9, 1/18
(`20261112-0245_30b80ae_t332-dense-window12/score/acc_gt18_r1.row`).

**The ruler arithmetic, checked.** 195.0 mm vessel height at 3x (65.0 mm at 1x), so the 50 mm bar is 25.6% of
height at 3x and 76.9% at 1x; arm 1's 53.5 mm is 27.4% of height. When the same pair is 53.5 mm out on a 195 mm
object, the difference between 0/9 and 2/9 is a 3.5 mm miss -- 1.8% of the object's height.

**The eye.** `artifacts/t333/renders/arm1_r1.png` and `arm2_r1.png` were read before this block. arm 1:
piece 1 stands alone 100 mm above a single huddled pile of the other eight; the pile does not read as a vessel.
arm 2: piece 1 again detached above, five pieces overlapping in a lump. Neither is a reassembly the conservator
would accept, whatever the count says. The 2/9 in arm 2 is piece 1 and one neighbour counted as "right" inside
a 50 mm ball.

**Ruling.** Reading 1 fails as pre-registered (0/9, 0/18 against >=4/9, >=8/18). Reading 2's direction holds
and its level does not: arm 2 lands exactly on the 1x score, nothing exceeds 1x, so replacing the agreement
gate at the 1x-equivalent distance buys back only the 1x result. Reading 3 holds in substance: 5.6x the
candidates and 30 of 36 pairs admitted still yield one correct pair -- the binder is the pose estimate /
registration, not the dot count. The scale line stops here; no gate was edited in this ticket.

**Accepted caveat, recorded as the worker put it.** The Pot A control scores 15/15 while never admitting four
of its GT pairs (1-8, 1-9, 2-9, 7-9), so admitted-pair lists are neither necessary nor sufficient for correct
joins. Clause 3 of the pre-registered reading ("no admitted join outside the GT graph") is therefore void as a
test and reading 1 was judged on its counts alone.

**What lands with the ticket.** `class/reconstruction.h` +11 and `class/reconstruction.cpp` +42/-2: one
`SFS_INLIER_MM` switch, read once at start-up, defaulting to `INLIER_THRESHOLD` (1.5) on unset/empty/
unparseable/out-of-[0.05,100], one banner line, used at both `inlierCalculate` call sites and nowhere else.
Default path is the shipped behaviour; the control run proves it (Pot A 8/8, 15/15, banner 1.5 mm).
