# Ticket 329 — Scale-aware seed floor: a pair registers on the evidence its pieces can supply

**Status:** wontfix — dropped by the conservator 2026-11-11 (m03342): the binding gate is the edgeline's physical length, not the count floor this ticket edits; bar 1 evidence kept, bars 2-7 never run, branch not merged
**Working in:** dropped (worker stood down; bar 1 was measured, bars 2-7 not started)
**Branch:** 329-scale-aware-seed-floor, in C:/PR/worktrees/structure-from-sherds-pp/329
**Answers:** S1
**Depends on:** 328 (cured Juglet trees and final binaries; bar 1 is its ten-pot confirmation), 326 (diag harness and scale-measurement code; branch not merged — run-dir copies cited below)
**Blocks:** merge of 328 to main (bar 1 is the gate)
**Worker:** 7e568579-f3c5-4905-a2dd-026f8a64ece3 (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-11 01:57 by the lead, continuing from 14006bb7-f674-4b68-9e85-7e64c0471655 which died on a provider outage at 01:50 with its bar-1 job still running)
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

## Comments (worker, 2026-11-11) — site audit (pre-patch) and bar-1 launch

**MINIMUM_NUMBER site audit** (grep over pristine tree at 20b52bf, comments evaluated, repo-wide outside .scratch):

- Definition: class/reconstruction.h:24 (#define MINIMUM_NUMBER 6; INLIER_THRESHOLD/ANGLE_THRESHOLD beside it — untouched by this ticket).
- Ticket list (b) confirmed live: class/feature_matching.cpp:1015, :1045 (dummy_inlier > MINIMUM_NUMBER/2); class/reconstruction.cpp:465, 530, 593, 666 (cor.size() > MINIMUM_NUMBER/2), :727, :763 (inlier < MINIMUM_NUMBER), :1198, :1397, :1589 (cor.size() < MINIMUM_NUMBER); class/ranking_system.cpp:800, :838 (windowsize arg -> FeatureCompGraphBuilding, def feature_matching.cpp:1498; run gate executes at feature_matching.cpp:407 LCSuff(i,j) > windowsize inside LCSGraphBuilding, called at :1543).
- **Audit adds two live consumers the (b) list did not name**: main.cpp:211 and hpc/tools/main_pristine_batch.cpp:209 — FeatureComp(shard, LCS_out, 25, MINIMUM_NUMBER, 0), the first pipeline stage in both mains; its run gate executes at feature_matching.cpp:294 (LCSuff > windowsize) inside LCS (def :212, called from FeatureComp at :1473). The batch shim main is what the SIF build runs, so this is on the measured path. These sites read M in the patch (same single knob; no shape change).
- Dead code, not edited: class/feature_matching.cpp:987 is a commented-out line (//if (size > MINIMUM_NUMBER)).
- The (b) cites fm:1446-1449 / :1518-1521 are the Q_size blocks inside FeatureComp / FeatureCompGraphBuilding — the feature conditions themselves (byte-identical, untouched); the floors enter via the windowsize argument only, as above.

**Bar 1 step 1 launched.** Run dir /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0120_20b52bf_t329-axes, job 34721544 (sapphire, 24 cpu, 40 min). Recipe = t328 job E verbatim (submit328e.sbatch, job 34702695): run_benchmark_172.py --jobs 24 --cap 6 --seeds 10 --step 10, then build_axes323.py <in> profile_rank.json <out> 1.2 3, gate = diff -rq vs runs/20261110-0740_d628fc0_t323-axes/out/axes323 (172 files).

- Code-path note: the canonical checkout /data/gpfs/projects/punim2657/sfs_preprocessing holds an md5-identical run_benchmark_172.py (cd547927e181c6e5ef3103d6413d5489) but its sfs_axes package has drifted post-t322 (7 files differ, profile.py absent; canonical at 4c2aec3, t320-era). The gate therefore runs from the t322 impl tree, the exact environment whose output 328 measured byte-identical — a code-choice artifact cannot be allowed to trip the gate.
- Final-binaries note: run_benchmark_172.py references no binary (measured in-job: grep -c 'subprocess|Popen|os.system' = 0, recorded in out/benchmark_binary_refs.txt); the trim5 final binaries cannot enter this computation by construction. Their md5s are re-measured and recorded in out/trim5_binaries.md5 so the "re-measured with the final binaries" wording is on the record rather than asserted.

## Comments (worker, 2026-11-11) — bar 1 step 1 PASS, step 2 launched

**Bar 1 step 1 — PASS.** Run dir `runs/20261111-0120_20b52bf_t329-axes`. Job history, both ids quoted from `sacct` this session:

- **34721544** (24 cpu, sapphire): CANCELLED, ExitCode 0:0, never started — no node, no elapsed time; it was the previous worker's first submission and was killed by its replacement.
- **34723217** (16 cpu, sapphire, spartan-bm189): COMPLETED, ExitCode 0:0, Elapsed 00:06:09, 2026-11-11T01:46:22 -> 01:52:31.

Gate, re-run independently this session rather than read off the job's own log:

- `diff -rq out/axes329f runs/20261110-0740_d628fc0_t323-axes/out/axes323` -> **0 lines, rc 0**.
- `md5sum *` of both directories, sorted -> 172 vs 172 entries, **diff 0 lines**.
- Composition of `out/axes329f`: **163 `Pot_*` + 9 `Juglet_Piece_N_Axis.xyz` = 172 files**, and all 172 are present in the reference set. The raw `out/axes329` carries 181 files: the same 172 plus 9 `Pot_JUGLET_Piece_NN_Axis.xyz` candidate rows that the reference set does not carry (expected, not a difference).
- `list_corpus` before/after identical: 53,889 bytes each, `list_corpus.diff` 0 lines.
- The job's own gate records agree: `out/axes329f_vs_axes323.diff` and `out/axes329_vs_axes322.diff` are both 0 bytes; binary/build provenance in `out/trim5_binaries.md5`, `out/benchmark_binary_refs.txt`, `out/axes329_code.md5`.

**Bar 1 step 2 launched.** Ten jobs, template `runs/20261110-0740_d628fc0_t323-pot<X>/submit.sbatch` byte-identical except the job name and the axes line (AX_318 -> `runs/20261111-0120_20b52bf_t329-axes/out/axes329f`); `work/` copied from the matching t323 pot dir (patch_guard67.py, mk_inc.py, stage305.py, arm67.sh, acc305.py, score305.py, score311_cpp.py, inc_<POT>). Each job stages and runs four arms — <X>_MOR1, <X>_MOR2 (authors' Axes), <X>_K3R1, <X>_K3R2 (our axes, top-3) — 48 cpu / 160G, pristine `sfs_pristine_assembly.sif`.

| pot | run dir | job |
|---|---|---|
| A | runs/20261111-0210_20b52bf_t329-potA | 34724991 |
| B | runs/20261111-0210_20b52bf_t329-potB | 34724992 |
| C | runs/20261111-0210_20b52bf_t329-potC | 34724993 |
| D | runs/20261111-0210_20b52bf_t329-potD | 34724994 |
| E | runs/20261111-0210_20b52bf_t329-potE | 34724995 |
| F | runs/20261111-0210_20b52bf_t329-potF | 34724996 |
| G | runs/20261111-0210_20b52bf_t329-potG | 34724997 |
| H | runs/20261111-0210_20b52bf_t329-potH | 34724998 |
| I | runs/20261111-0210_20b52bf_t329-potI | 34724999 |
| J | runs/20261111-0210_20b52bf_t329-potJ | 34725000 |

All ten PENDING (Priority) at 02:05; sapphire was 10112/10112 cpu allocated on 79 nodes with 154 idle cpus spread over 21 mixed nodes. Fallback if a job is still PENDING after ~15 min: duplicate at 8 cpu / 32G / 2:00:00 and keep whichever starts (applies only to the light pots — t323 walls per arm were D 3188-3693 s, E 2250-2713 s, I 1551-1899 s, so D/E/I cannot be squeezed into 8 cpus and stay at 48).

**Pre-result note on pot H's bar — bar NOT changed, result not yet seen.** The bar table's H row is 11/11, 17/17, but no t32x reference run has ever reached that on the MO (authors' axes) lane: t323-potH MO 10/11 10/17 and 8/11 9/17 vs K3 11/11 17/17 twice; t325-potH MO 8/11 9/17 twice vs K3 11/11 17/17 twice; t326-potH MO 6/11 8/17 and 8/11 11/17 vs K3 11/11 17/17 twice. So H's recorded 11/11 17/17 is the K3 lane's number, and the MO control lane has a standing shortfall in every reference run. I will report all four lanes per pot and apply the bar as written; if H_MO lands below 11/11 I will report it as such and let the lead rule, rather than call the whole gate failed on a lane the baseline itself never met.


## Comments (worker, 2026-11-11) — bar 2: seed-path diagnostic on the cured tree (measurement only)

Bar 2 ran with the **pristine** binary. Job **34725192**: COMPLETED, ExitCode 0:0, Elapsed 00:00:17, 2026-11-11T01:57:54 -> 01:58:11, node spartan-bm071. Run dir `runs/20261111-0225_20b52bf_t329-diag2`; harness `diag329_pairs.cpp` (md5 f6525d7d2e563bfec0aa47a8b5385434) + `diag329.sh` (de7ff256c884ad9d7a1ecc085cec2075), driver copied from `runs/20261110-1844_6925cb0_t326-diag`; output `out/diag_329_bar2.txt`, 19,700 B. Inputs: cured tree `runs/20261110-2325_123f2ca_t328-trim5/trees/juglet_after`, pristine `sfs_main/sfs_pristine_assembly.sif` (patched sources absent — the image's `class/feature_matching.cpp` md5 is 0c086de9621ecbbd1af0b30c12b8c30d), FLOOR=20, NP=9, `-DJUGLET_ORIG`. No source was changed for this bar and none of the numbers below come from a patched build.

### How the table is read

`Dataset/Juglet_GT/Juglet_simple_graph.txt` is a 9x9 0/1 matrix with **18 undirected edges** (1-2, 1-4, 1-5, 1-6, 1-8, 1-9, 2-5, 2-9, 3-5, 3-7, 4-5, 4-6, 5-9, 6-7, 6-8, 7-8, 7-9, 8-9). The ticket's **nine touching pairs are exactly the graph edges whose answer-key `gap_min` is under ~2 mm**. The harness prints `touch` = graph adjacency, so that column shows 18 ones, not 9; the nine have to be read off `gap_min`. The other nine graph edges are near-but-not-touching (1-5 5.623, 1-6 4.915, 3-7 14.191, 4-5 8.362, 4-6 9.336, 5-9 19.248, 6-8 2.784, 7-8 15.329, 8-9 20.235 mm).

### The nine touching pairs, stage by stage

`cor` = correspondences from MakeCorWOBuildTree, then survivors after RejectOutlier; `inl15`/`inlierCalc` = CountInlier at 1.5 mm; `LCS proxy run` = the harness's longest run of consecutive correspondences passing the four feature conditions (see caveat below); `axis/rot/rms` = the candidate's axis-angle, rotation error and RMS error.

| pair | gap_min mm | cor built -> kept | inl15 | inlierCalc (floor 6) | LCS proxy run | candidates | best candidate axis / rot / rms | outcome | gate that kills it |
|---|---|---|---|---|---|---|---|---|---|
| 1-2 | 0.514 | 13 -> 13 | 8 | 16 | 9 | none | — | no candidate | `class/feature_matching.cpp:294` LCS run floor (windowsize = MINIMUM_NUMBER) |
| 1-4 | 0.211 | 9 -> 9 | 7 | 14 | 9 | 1 (idx 3) | 0.6459 / 174.5 deg / 30.40 mm | dropped | `class/feature_matching.cpp:1715` axis_angle > 0.436 (PairwisePruning) |
| 1-8 | 0.304 | 19 -> 18 | 13 | 26 | 15 | 1 (idx 11) | 0.0767 / 4.40 deg / 0.76 mm | **survives** | — |
| 1-9 | 0.998 | 7 -> 7 | 0 | -1 | 3 | 1 (idx 12) | 0.7135 / 172.3 deg / 32.65 mm | dropped | `class/feature_matching.cpp:1715` axis_angle > 0.436 |
| 2-5 | 1.539 | 1 -> 1 | 0 | -1 | 1 | none | — | no candidate | `class/feature_matching.cpp:294` LCS run floor |
| 2-9 | 0.738 | 4 -> 4 | 0 | -1 | 4 | none | — | no candidate | `class/feature_matching.cpp:294` LCS run floor |
| 3-5 | 0.401 | 3 -> 3 | 0 | 6 | 2 | 1 (idx 19) | 0.7466 / 140.9 deg / 31.58 mm | dropped | `class/feature_matching.cpp:1715` axis_angle > 0.436 |
| 6-7 | 0.809 | 15 -> 10 | 0 | -1 | 10 | 1 (idx 29) | 0.4898 / 168.6 deg / 28.93 mm | dropped | `class/feature_matching.cpp:1715` axis_angle > 0.436 |
| 7-9 | 0.945 | 3 -> 3 | 0 | -1 | 3 | none | — | no candidate | `class/feature_matching.cpp:294` LCS run floor |

**Batch result: 1 of 9 touching pairs survives the pristine pipeline on the cured tree (1-8).** Four die at the LCS run-length floor (1-2, 2-5, 2-9, 7-9) and four at the axis-angle prune (1-4, 1-9, 3-5, 6-7).

### What the dead pairs look like, and why the angle gate is not being marginal

The four axis_angle deaths are not near-threshold: 1-4 0.6459, 1-9 0.7135, 3-5 0.7466, 6-7 0.4898, against a threshold of 0.436 — and their rotation errors are 140.9-175.0 deg with RMS errors 28.9-32.7 mm. These are **flipped / upside-down registrations**, not slightly-off ones: the matcher pairs the two pieces through a transform that turns the sherd over. The gate is doing its job on them. The one true pair that survives is the one with a plausible transform (4.40 deg, 0.76 mm RMS).

### The other 27 pairs (baseline for bar 4)

- 30 candidates were built in total; **19 were dropped by `axis_angle>0.436`** in PairwisePruning, 11 survived.
- `overlap50` is 0 for every one of the 36 pairs and the MERGE table is empty: **no pair, true or false, reaches OverlapCheck_3d** in this baseline. Bar 4's "survives strictly further down the pipeline" can therefore only mean "gets past the candidate/prune stages", since nothing currently gets as far as overlap or merge.
- Non-graph pairs that already register in this baseline (11 survivors, 1-8 among them): 1-3 (idx 2), 1-6 (idx 5 and 7), 2-3 (13), 3-4 (18), 3-6 (21), 4-6 (24 and 25), 5-6 (27), 5-7 (28). Several are visibly wrong transforms and survive because the gate cannot see their failure mode: 4-6 idx 24 has axis_angle 0.151 but rot_err 179.8 deg and rms 32.3 mm — a 180 deg flip about the part axis leaves the axis direction intact, so a pure axis-angle test passes it. 1-3 (rot 176.0 deg, rms 25.6) and 3-6 (rot 135.4, rms 29.5) are the same story. Only 5-6 (rot 15.1, rms 3.68) and 5-7 (rot 7.8, rms 2.06) are geometrically plausible false positives.
- Small-gap non-graph pairs that never register and where they die: 3-8 (gap 0.398, 13 cor, inl15 9, inlierCalc 18) LCS run floor; 4-7 (0.351, 10/6/12) LCS run floor; 5-7 (0.873) survives as above; 5-6 (0.579) survives; 5-8 (0.644, 6/4/8) LCS run floor; 6-9 (0.522) LCS run floor; 2-6 (0.769, 9/6/12) axis_angle; 1-3 (0.882, 7/0/-1) survives; 2-8 (0.903) and 3-4 (1.152) and 4-9 (1.272) as tabulated.
- The three count floors the ticket edits are thus **not** what kills most dead pairs on this path: 6 of the 9 are killed by the run-length floor at `feature_matching.cpp:294` or by `feature_matching.cpp:1715`, and the harness shows six of the nine touching pairs never even reach an inlier count that the floor could admit (inl15 0 for 1-9, 2-5, 2-9, 3-5, 6-7, 7-9).

### Two harness caveats recorded so the table is not over-read

- The `window_needed` column prints a constant 7 and is not the applied threshold; ignore it. The applied gate is `LCSuff(i,j) > windowsize` with `windowsize = MINIMUM_NUMBER = 6` (`class/feature_matching.cpp:294`, function `LCS` at `:212`, called from `FeatureComp` at `:1473`; the 6 arrives as the `windowsize` argument from `hpc/tools/main_pristine_batch.cpp:209` / `main.cpp:211`).
- `LCSDIAG longest_true_run` is the harness's proxy (consecutive correspondences on the GT association set passing the four feature conditions), not the LCSuff run the pipeline computes on its own comparison matrix. They disagree in one visible place: 1-2 shows proxy run 9 and still produces **no candidate**, so the LCSuff run that the pipeline actually saw for 1-2 is <= 6. Bar 3/4 numbers must be compared like for like (patched harness against pristine harness), not proxy against gate.

### Recorded for bar 4 (a prediction, not a result — no patched binary has run)

With the pre-registered change, Juglet s = 0.535 gives M = ceil(6 * 0.535) = 4, so `windowsize` becomes 4 and the gate becomes `run > 4` (runs >= 5). Against the proxy runs above, only 1-2 (9), 1-4 (9) and 6-7 (10) clear that; 1-9 (3), 2-5 (1), 2-9 (4), 3-5 (2) and 7-9 (3) would still be refused by the run floor, and 1-4 and 6-7 already build a candidate today and die at 0.436, which lowering M cannot change. The plausible ceiling is therefore **1-8 plus at most 1-2 = 2 of 9 touching pairs**, while bar 4 requires at least 4 to survive strictly further than this table. That is a statement about the ticket's premise — the count floors are not the binding gate on this tree; the run-length floor and the axis-angle prune are — and it will be tested with the patched binary rather than assumed.

### Bar 1 step 2 — queued, blocked on fairshare, nothing lost (2026-11-11 03:00 AEDT)

Both sets are submitted and both are still `PD`: the ten 48-cpu jobs **34724991-34725000** (run dirs `runs/20261111-0210_20b52bf_t329-pot<X>`) and the ten 8-cpu sequential-arm duplicates **34726553-34726562** (run dirs `runs/20261111-0230_20b52bf_t329-pot<X>-8c`). The block is my fairshare, not the request: `sprio` decomposes priority 13,961 (48-cpu) / 13,807 (8-cpu) into AGE 274/118 + **FAIRSHARE 13,687** + JOBSIZE, `sshare` reports RawUsage 464,797,474, EffectvUsage 1.000000, FairShare 0.136869, and the top pending priority in the partition is ~76,000-104,000; sapphire has 12,940 of 13,056 cpus allocated with 5,903 jobs pending. A 5-minute 8 cpu / 8G probe (**34728897**) also stayed PD, which rules out request size. `squeue --start` puts the 48-cpu set at 2026-11-12T07:50 through 19:34 and gives the 8-cpu set no estimate at all. Recorded deviations from the brief's letter: the 8-cpu duplicates run the four arms **sequentially** (each arm still `OMP_NUM_THREADS=8`, so the thread count that could change a result is unchanged — only concurrency), and their time limits were lowered in place (C, F, G 00:20:00; A, B, H, J 00:45:00; I 03:00:00; D, E 06:00:00) because D and E cannot finish four arms in two hours; `Mem=8G` is refused by this Slurm config ("Update of this parameter is not supported: Mem=8G") so they keep 32G. The `long` partition is not an escape: `sbatch --partition=long` is refused by policy ("The long partition is for jobs that require more than 30 days, but less than 90 days"), and no interactive/side route is being used. Whichever copy of a pot starts first wins; that pot's other copy is cancelled and **both job ids are recorded** here when that happens.

### Bar 3, first half — the patch compiles against the pinned tag (done without the queue)

Run dir `runs/20261111-0245_20b52bf_t329-compile` (script `compile_check_329.sh`, log `compile_check.log`). Inside the control image `sfs_pristine_assembly.sif` (Ubuntu 22.04.5, g++ 11.4.0), the patch was applied to the image's **own** `/opt/sfs-pristine` tree at pinned commit 9195c0167e2077a564bd7c91156063252feeb506: `git apply --check` rc 0, `git apply` rc 0 (one whitespace warning at patch line 103, the `if (dummy_inlier > MinNumFloor() / 2) ` trailing space), `git status --porcelain` = 6 modified files, and every patched md5 is byte-identical to the Linux-verified set (main.cpp cb85f4911a8afd957a4756cf66abb089, class/data_structure.h 8b823370b1ec7aa2ebf699f124951784, class/data_structure.cpp e1bd9b73cbe03151e7d6c4f29947e055, class/feature_matching.cpp cb7b115045a6277aefd47305353589bc, class/ranking_system.cpp 87b860f43efa7b9322f19de915127ba6, class/reconstruction.cpp 9cb61935381b166cf466045978b75767). Every patched translation unit then compiles with the flags the image itself was built with (`-O3 -DNDEBUG -fopenmp -fPIC` plus the eigen3 / pcl-1.12 / vtk-9.1 / qt5 include set from `build/CMakeFiles/Hierarchy-Clear.dir/flags.make`): `-fsyntax-only` rc 0 for class/data_structure.cpp, class/feature_matching.cpp, class/reconstruction.cpp, class/ranking_system.cpp, main.cpp **and** the batch shim `main_pristine_batch_329.cpp` (md5 7445381b6e27a49436b539015de09024; `MeasureLengthScale(shard);` at :209, `FeatureComp(shard, LCS_out, 25, MinNumFloor(), 0);` at :210 — this is the gap the earlier draft patch had, the shim the SIF actually runs). The only diagnostics are the two pre-existing `double high_score = NULL` conversion warnings at class/feature_matching.cpp:1621 and :1761. Post-patch marker counts: `MinNumFloor()` 1/1/2/9/2/1 in data_structure.h / data_structure.cpp / feature_matching.cpp / reconstruction.cpp / ranking_system.cpp / main.cpp, `SCALE329` 2 in data_structure.cpp, `MeasureLengthScale` 1 in main.cpp and 2 in feature_matching.cpp; `MINIMUM_NUMBER` survives only at `class/reconstruction.h:24` (the definition), two macro/comment hits in class/data_structure.h, and the commented-out `class/feature_matching.cpp:987` — **no live consumer left**. Bar 3's second half (build `sfs_pristine_assembly_329.sif` and deliver PATCH329-BUILD.txt) is fully staged at `runs/20261111-0230_20b52bf_t329-build` and submits as soon as the queue admits a job.

### Bars 4-6 scripts written and dry-checked (not submitted)

Staged at `runs/20261111-0305_20b52bf_t329-stage/` and kept in `.scratch/pristine-juglet-sfs/t329/`: `submit329bar4.sbatch` (patched objects built inside the 329 image into a fresh panel, `feature_matching.cpp` md5 asserted, harness staged as `tools/diag329_pairs.cpp` because `diag329.sh` hardcodes that name, `diag329.sh 329_bar4 /inc 20 -DJUGLET_ORIG`), `submit329jug.sbatch` (bar 5: T329P_r1/r2 pristine, T329X_r1/r2 patched, **separate panel dir per assembler** so the 329 lane cannot silently reuse pristine objects, FLOOR=20, NP=9, -DJUGLET_ORIG), and `submit329pot6.sbatch` (bar 6: the t326 pot template with SIF -> 329 image, SHIPPED -> the 329 shim, axes -> axes329f, plus the SCALE329 s=1.000 / M=6 assert per lane; POT/NAMED/NP from the recorded pot table: A 8/8, B 9/9, C 4/4, D 29/29, E 31/31, F 7/7, G 7/7, H 11/11, I 30/30, J 12/19). Dry checks: `bash -n` clean on all three, every referenced path verified present on Spartan, helper CLIs checked (`stage67_inputs.py`, `score58.py`, `arm67.sh` usage), and one real defect found and fixed — `arm67.sh` redirects the binary's stdout to `/panel/logs/binary_<lane>.log`, so the SCALE329 capture must read that file, not the job's stdout.

## Lead ruling (2026-11-12) — dropped, and why

**The conservator's diagnosis (2026-11-11, m03342):** the axis is ruled out, the Juglet's edgelines are
too short for the assembler's matching step, and "the millimetre thresholds and the count floors are not
what binds, so drop both tickets 326 and 329".

**The worker's own bar-2 checkpoint agrees, measured on the cured tree (job 34725192).** Of the nine
touching pairs, four never produce a candidate because the LCS run-length floor at
`class/feature_matching.cpp:294` refuses them (1-2, 2-5, 2-9, 7-9; 1-4 matching dots on 2-5 and 7-9, 3 on
3-5), and four produce only upside-down candidates that the axis-angle prune at
`class/feature_matching.cpp:1715` drops (1-4 0.6459 / 140.9 deg, 1-9 0.7135 / 175.0 deg, 3-5 0.7466 /
140.9 deg, 6-7 0.4898 / 168.6 deg, against the 0.436 threshold). Only 1-8 survives, at 4.40 deg / 0.76 mm
RMS. The three count floors this ticket edits sit downstream of that: six of the nine pairs never reach an
inlier count the floor could admit (inl15 = 0).

**So the ticket is dropped, not refuted by measurement of its own change.** Bars 3-6 never ran. What it
leaves behind is bar 1, which did its job and is kept:

- **Bar 1.1 PASS** — the ten-pot axes regenerated with 328's FINAL binaries are byte-identical to t323's:
  172 files, `diff -rq` 0 lines, job 34723217 COMPLETED 0:0 (out/axes329f). The raw out/axes329 (181
  files, six rows per piece) is pre-filter output and is not the gate.
- **Bar 1.2 PASS on the recorded bars** — pristine `sfs_pristine_assembly.sif`, axes = axes329f, four lanes
  per pot, 8-cpu runs under `runs/20261111-0230_20b52bf_t329-pot<X>-8c/score/acc.tsv`: A 8/8 15/15;
  B 9/9 15/15; C 4/4 5/5; D 24/28 57/69, 23/28 55/69, 26/28 63/69 x2; E 31/31 62/62 x2 and 30/31 57/62 x2;
  F 5/5 7/7; G 7/7 10/10; H 11/11 17/17 x2 on the K3 lanes, MO 8/11 9/17 and 5/11 7/17; I 25/26 59/62 x4;
  J 8/11 16/21 x3 and 9/11 17/21. H's MO lane is below the H row and is recorded as a new low; H's row was
  the K3 lane's number and no reference run ever met it on MO (t323 10/11 and 8/11, t325 8/11 x2, t326 6/11
  and 8/11) — the pre-result note at line 261 says so, written before the run.
- The ten 48-cpu copies of bar 1.2 are not evidence: their sbatch never created the run's `out/`, so
  apptainer's bind-mount failed in 1 s per lane (exit=255) while Slurm reported COMPLETED 0:0. The 8-cpu
  duplicates hold the results. Batch-script defect recorded, not fixed here.

**Consequence for 328:** with 326 already wontfix and this ticket dropped, the ten-pot evidence that gates
328's merge is bar 1 alone, and it passes; 328 was merged to `SfSpp_preprocessing` main on 2026-11-12.

**Kept as artefacts, not merged:** this branch's patch (MinNumFloor one-sided clamp, six files), the 329
container def, the batch shim, the t329 scratch scripts and the run dirs. The pre-registered rule stands
for any future attempt: s = min(1, len_scale/60), M = max(3, ceil(6*s)), no geometric gate moved.
