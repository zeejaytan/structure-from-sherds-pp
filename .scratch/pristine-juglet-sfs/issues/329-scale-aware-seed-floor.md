# Ticket 329 — Scale-aware seed floor: a pair registers on the evidence its pieces can supply

**Status:** in-progress
**Working in:** lead-dispatched subagent (2026-11-11)
**Branch:** 329-scale-aware-seed-floor, in C:/PR/worktrees/structure-from-sherds-pp/329
**Answers:** S1
**Depends on:** 328 (cured Juglet trees and final binaries; bar 1 is its ten-pot confirmation), 326 (diag harness and scale-measurement code; branch not merged — run-dir copies cited below)
**Blocks:** merge of 328 to main (bar 1 is the gate)
**Worker:** unassigned
**Budget:** large (8 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** stays with the lead's subagent (worker never commits; lead verifies from artefacts and lands)

> **In one line.** After 328's cure every Juglet seam agrees in normal sign, but the assembler still registers
> only 3 of 9 touching pairs, because its seed floors are sample counts fixed for 60 mm pots: a pair must
> produce more than 3 inliers inside 1.5 mm and 15 deg, survive a second floor of 6, and walk an LCS run longer
> than 6 samples (~12.6 mm at the 1.8 mm line spacing) — on a 33 mm vessel whose seams supply 1-19
> correspondences. Make the count floors read the object's own measured scale, clamped so every pot gets the
> identical integers it gets today, prove the pots do not move, and measure what the Juglet gains. No geometric
> gate changes at all.

## What is already known (lead-verified 2026-11-10/11; reproduce it, do not re-derive it)

**(a) The cure fixed the normals; the floors are what is left.** On 328's cured tree
(run `20261110-2325_123f2ca_t328-trim5`, job 34706334) all nine touching pairs have median normal dot
`dot_med_d3` between +0.786 and +0.997, and `n_d3_dotneg` is 0 on all nine. The strict seed gate
(`CountInlier` at `INLIER_THRESHOLD` 1.5 mm, `ANGLE_THRESHOLD` 0.262 rad folded, pair registers only if
`dummy_inlier > MINIMUM_NUMBER/2` = 3) still passes only three. Per pair (correspondences / inside 1.5 mm /
best |dot| inside 1.5 mm / inliers / what caps it):

| pair | corr | <1.5mm | best \|dot\| | inl | capped by |
|---|---|---|---|---|---|
| 1-2 | 13 | 13 | 0.9962 | 8 | — registers |
| 1-4 | 9 | 9 | 0.9832 | 7 | — registers |
| 1-8 | 19 | 19 | 0.9992 | 14 | — registers |
| 1-9 | 7 | 2 | 0.8596 | 0 | alignment (30.7 deg) then distance; count is NOT the cap |
| 2-5 | 1 | 0 | — | 0 | 1 correspondence in the whole pair |
| 2-9 | 4 | 4 | 0.9499 | 0 | alignment (18.2 deg) |
| 3-5 | 3 | 3 | 0.9997 | 0 | count: 3 correspondences cannot exceed a floor of 3 |
| 6-7 | 15 | 9 | 0.8771 | 0 | alignment (28.7 deg) |
| 7-9 | 3 | 3 | 0.9196 | 0 | count AND alignment (23.1 deg) |

Touching set per the graph the scorer reads (`Juglet_simple_graph.txt`): 1-2, 1-4, 1-8, 1-9, 2-5, 2-9, 3-5,
6-7, 7-9. 3-7 is in the graph at 14.19 mm and does not touch; 6-9 is not in the graph.

**(b) Where the floors live** (pristine main 89fcd9d; audit the list yourself, comments excluded):
`class/reconstruction.h:24-26` defines `MINIMUM_NUMBER 6`, `INLIER_THRESHOLD 1.5`, `ANGLE_THRESHOLD 0.262`.
Consumers: `class/feature_matching.cpp:1015` and `:1045` (`dummy_inlier > MINIMUM_NUMBER / 2`);
`class/reconstruction.cpp:465, 530, 593, 666` (`cor_line.cor.size() > MINIMUM_NUMBER / 2`), `:727` and `:763`
(`inlier < MINIMUM_NUMBER` after `CountInlier`), `:1198, :1397, :1589` (`COR[i].cor.size() < MINIMUM_NUMBER`);
`class/ranking_system.cpp:800, :838`; and the LCS run requirement in `feature_matching.cpp:1446-1449` and
`:1518-1521` (a run of MORE than `MINIMUM_NUMBER` samples = 7 or more, ~12.6 mm at the 1.8 mm spacing; the
feature conditions themselves are the `Q_size`/`Compare` tolerances at `:273-279`, `:385-391`).

**(c) The scale measurements exist.** 326's shim measured `len_scale` (median breakline bbox diagonal over the
admitted pieces, reference 60 mm): pots A 124, B 97, C 150, D 107, E 92.8, F 60.6, G 71.9, H 96, I 106, J 105 mm
(every ratio >= 1.0096), Juglet 32.07-32.42 mm (0.535-0.540). Mechanism in
`hpc/patches/326-scale-free-matching.patch` (branch `326-scale-free-assembly`, commit 8f006fd), SCALE326 marker
pattern in `work/logs/binary_<lane>.log` of the t326 runs.

**(d) 326's lesson, binding here.** Scaling the geometric gates up broke pot E (62/62 -> 46-52/62) and scaling
them down seated nothing (0/9, tighter than control). This ticket scales NO geometric gate. Only the integer
count floors move, and only downward (hard clamp at the 60 mm reference).

**(e) The control to beat.** 328's E2E (job 34710135, run `20261110-2352_123f2ca_t328-e2e-dup8`): pristine
assembler on the cured trees seats **2/9 pieces, 0/18 joins** on both repeats and commits to pair 1-8 — the one
pair whose evidence was always strong. `score/acc.tsv`: T328C_r1 2/9 1/18, T328C_r2 2/9 1/18, T66O_r1 2/9 0/18.

## The change, pre-registered before any run

- `s_raw = len_scale_mm / 60` (326's measurement, code reused verbatim); `s = min(1.0, s_raw)` — hard clamp.
- `M = max(3, (int)ceil(6.0 * s))`. Pots: s = 1 -> M = 6, the identical integer at every site, so every pot
  decision path is bit-for-bit the same computation. Juglet: s ~ 0.535 -> ceil(3.21) = 4.
- Every consumer in (b) reads `M` in place of `MINIMUM_NUMBER` (and `M / 2` for the half floors). The macro
  stays defined; introduce the derived `M` beside it so the diff is one auditable block plus site edits.
- `INLIER_THRESHOLD` (1.5), `ANGLE_THRESHOLD` (0.262), every `Q_size`/`Compare` LCS feature condition, the
  overlap/merge/rim distances, `Clustering(…, 20)`, `RejectOutlier(…, 0.7/0.8)` fractions: **byte-identical,
  untouched.**
- Print one `SCALE329 len_scale=<mm> s=<val> M=<int>` marker per lane, like 326's SCALE326.

## Bars

### Bar 1 — 328's ten-pot confirmation (blocking gate for 328's merge and for this ticket)

1. Regenerate the axes with 328's FINAL binaries — `bin-MeshPreprocessingHeadless` /
   `bin-EdgeLineExtractionHeadless` in run `20261110-2325_123f2ca_t328-trim5` (`binaries.md5`, `src.md5`,
   `patch3.tar` record the build) — via `scripts/run_benchmark_172.py --cap 6 --seeds 10 --step 10` (copy in
   the canonical checkout `/data/gpfs/projects/punim2657/sfs_preprocessing`; the exact invocation 328 used is in
   the `20261110-2252_123f2ca_t328-planenorm` logs of job 34702695) -> `out/axes329`, then t323's
   `build_axes323.py` -> `axes329f`. `cmp` every file against
   `runs/20261110-0740_d628fc0_t323-axes/out/axes323` (172 files). **ANY differing file stops the ticket**:
   report the file, never nudge anything. (328 measured this identity on the 3 mm-plane build; the landed code is
   the 5 mm+trim build — this re-measures it on the final binaries.)
2. Ten-pot E2E with the **pristine** `sfs_pristine_assembly.sif`, template
   `runs/20261110-0740_d628fc0_t323-pot<X>/submit.sbatch`, axes dir = your `axes329f`, both lanes (MO = authors'
   released axes, K3 = ours), two repeats, ten jobs:

| pot | recorded bar (sherd, edge) |
|---|---|
| A | 8/8, 15/15 |
| B | 9/9, 15/15 |
| C | 4/4, 5/5 |
| D | >=23/28, >=55/69 |
| E | >=29/31, >=57/62 |
| F | 5/5, 7/7 |
| G | 7/7, 10/10 |
| H | 11/11, 17/17 |
| I | >=25/26, >=59/62 |
| J | >=8/11, >=16/21 |

   **Any pot below its bar in any lane: STOP after reporting.** 328 does not merge and bars 3-6 do not run. All
   lanes land or the ticket stops; no lane is dropped from the table.

### Bar 2 — seed-path diagnosis on the cured tree, pristine binary (measurement only, no code change)

For each of the nine touching pairs and all 27 non-contact pairs, trace the real assembler path stage by stage:
correspondences (`MakeCorWOBuildTree`) -> `RejectOutlier` survivors -> `CountInlier` + registration floor ->
LCS run length and which of the four feature conditions fail -> ranking candidate number ->
`OverlapCheck_3d` -> merge. Reuse and extend `diag326_pairs.cpp` + `diag326.sh` (run-dir copies in
`20261110-2325_123f2ca_t328-trim5`; laptop originals on branch `326-scale-free-assembly` at 8f006fd,
`hpc/tools/` + `hpc/scripts/`). Deliverable: one table, pair x stage -> surviving count, with the exact gate
that kills each dead pair named (file:line). This table is the baseline bar 4 is judged against.

### Bar 3 — implement and prove the build

Patch exactly as pre-registered above. Build the SIF (pattern: `hpc/sbatch/build_pristine_assembly_326.sbatch`,
own def file, own image name, e.g. `sfs_pristine_assembly_329.sif`). Deliver `PATCH329-BUILD.txt`: md5 of the
pristine image before/after (must stay `63a532b9732f84e59e7b1349065f1323`, mtime 2026-10-14), md5 of every
patched source file, and the SCALE329 marker visible in one lane's `work/logs/binary_<lane>.log`.

### Bar 4 — Juglet seed re-measure (the progress bar)

Re-run the bar-2 diagnostic with the patched binary on the SAME cured trees. Pass requires ALL of:

- at least 4 of the 9 touching pairs survive strictly further down the pipeline than in bar 2 (stage-by-stage
  comparison; registration alone does not count if the pair dies at the very next stage);
- the union of pieces in surviving true pairs covers >= 6 of 9;
- **no non-contact pair newly registers** (floors only drop, so the only newly possible registrations are pairs
  with exactly 3 inliers — name any non-contact with 3 and show where it still dies).

Table: bar 2 vs bar 4, per pair, per stage.

### Bar 5 — Juglet E2E, two repeats

Template `runs/20261110-2352_123f2ca_t328-e2e-dup8/submit328f.sbatch` (SIF
`sfs_main/sfs_pristine_assembly.sif` for controls, your 329 image for the patched lanes; FLOOR=20, NP=9,
`-DJUGLET_ORIG`, `juglet_orig_block.inc`). Lanes: cured trees x patched binary r1, r2; cured trees x pristine
binary r1, r2 (the 328 control, 2/9 0/18). Bar: **neither patched repeat below the control** (2/9 pieces, 0/18
joins); report seated/joins for all four lanes, best-of-2 named as best-of-2. Renders (both patched repeats +
one control, seating view) into `artifacts/pota329/renders/`, linked workspace-relative.

### Bar 6 — ten-pot E2E with the patched assembler

Same recorded bars as bar 1, template the t326 pot variant (`runs/20261110-2248_6925cb0_t326-pot<X>/submit.sbatch`
shows the patched-SIF + marker pattern; keep BOTH lanes MO and K3 — bar 1 used pristine, this bar uses the 329
image, two repeats). Additional requirement: **every pot lane's SCALE329 prints s=1.000, M=6**. Any pot below its
bar: the clamp failed in practice — STOP, report.

### Bar 7 — corpus, checks, renders

- `list_corpus` before/after over `Dataset/` and `sfs_main/original_samples`: diff empty (pattern in every
  t32x run dir).
- Pristine SIF md5 unchanged; corpus manifests identical.
- `python ../scripts/check_intent_links.py` from the repo root: no NEW errors or warnings (pre-existing: 2
  errors / 20 warnings, `intent/README.md:45`, `intent/S1-does-it-connect-anything-real.md:259`).
- Renders linked as workspace-relative Markdown paths; heavy files stay on Spartan.

## Must not

- **No change to any geometric gate**: `INLIER_THRESHOLD`, `ANGLE_THRESHOLD`, LCS feature conditions
  (`Q_size`, `Compare`), overlap/merge/rim distances, `Clustering` counts of points, `RejectOutlier`
  fractions. The integer `MINIMUM_NUMBER` floors are the ONLY behavioural edit. The angle gate is explicitly
  out of scope — 328's lead ruling stands; if bar 2 names it, report it, do not fix it (separate ticket).
- No per-piece, per-pot or object-type branch: one code path, driven by the measured scale, clamped at the
  reference. No "small-object" flag.
- No assembler edit beyond the consumer sites in (b), the reused scale measurement, and the SCALE329 print.
- No preprocessing edit of any kind — 328's code is frozen at 3ec34bd; its binaries are inputs.
- **No bar is re-specified after its results are seen** (328 bar-3 precedent). A failed bar is reported with its
  numbers; it is not re-worded, and thresholds are never nudged to make a number pass.
- No commits, no pushes, no `git worktree` commands: the lead reviews, verifies and lands. Append findings as
  `## Comments (worker, <date>)` and flip `Status:` to `results-in` only.
- Poll Slurm with `sacct` directly (job id + State/ExitCode quoted live); never invent a job id; every sbatch
  gets its final sacct recorded in the ticket.

## Inputs (Spartan paths unless said)

- 328 cured Juglet trees: `runs/20261110-2325_123f2ca_t328-trim5/trees` and the E2E staging copies
  `runs/20261110-2352_123f2ca_t328-e2e-dup8/trees/juglet_cured_r1|r2`; final binaries + build record in the
  trim5 run dir.
- Axes identity reference: `runs/20261110-0740_d628fc0_t323-axes/out/axes323`; the planenorm-build
  `axes328`/`axes328f` (`runs/20261110-2252_123f2ca_t328-planenorm/out/`, measured identical) for cross-check.
- Ten-pot E2E templates: `runs/20261110-0740_d628fc0_t323-pot<X>/submit.sbatch` (pristine, bar 1) and
  `runs/20261110-2248_6925cb0_t326-pot<X>/submit.sbatch` (patched-image + marker pattern, bar 6).
- Juglet E2E template: `runs/20261110-2352_123f2ca_t328-e2e-dup8/submit328f.sbatch`; scorer `score58.py`
  (`t328` scratch, run dirs carry copies).
- Corpus: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp` (caveats: read
  `SfSpp_preprocessing/AGENTS.md` "Corpus caveats" before scoring anything); Juglet answer key:
  `/data/gpfs/projects/punim2657/Dataset/Juglet_GT/`.
- Scale-measurement code: `hpc/patches/326-scale-free-matching.patch` (branch `326-scale-free-assembly`,
  8f006fd, laptop; the t326 runs carry the built behaviour).
- Laptop: assembler main 89fcd9d; worktree will be `C:/PR/worktrees/structure-from-sherds-pp/329` on branch
  `329-scale-aware-seed-floor`. Spartan is fetch-only; runs under
  `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t329-<tag>/` with `run.json`, `logs/`,
  `out/`; scratch scripts into `.scratch/pristine-juglet-sfs/t329/` on the branch (lead commits them at
  landing).

## Constraints

- Spartan compute only; the laptop edits code in the worktree and reads logs.
- Two repeats everywhere the assembler runs (it is non-deterministic; pot E precedent).
- Budget large = 8 failed acceptance runs; waiting costs none; stop on the first bar miss and report.
- Checkpoint each finished bar into the ticket as a `## Comments (worker, <date>)` append; end every report
  with exactly one `**End state:** ready|waiting: <job ids>|blocked: <reason>|incomplete: <what>`.
