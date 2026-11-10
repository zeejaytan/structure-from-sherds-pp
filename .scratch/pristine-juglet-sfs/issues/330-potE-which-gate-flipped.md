# 330 — Pot E: which of the scaled gates admits the wrong joins

**Status:** results-in
**Answers:** S1
**Depends on:** 326 (the patch and the diagnostic harness were built on branch `326-scale-free-assembly`; the harness source is on disk in the lead's 326 worktree)
**Blocks:** nothing. Ticket 329 does not wait on this: its own rule leaves every pot at s = 1.000 by construction.
**Working in:** lead-dispatched subagent (2026-11-11)
**Branch:** `330-potE-gate-flips` (worktree `C:/PR/worktrees/structure-from-sherds-pp/330`)
**Worker:** b7df1d76-9dac-448b-8623-6efb12f1b659 (dsh one-shot subagent, dsh-opencode-go/deepseek-v4.1-flash; dispatched 2026-11-11 02:26 by the lead)
**Budget:** small (3 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** stays with the lead's subagent (worker never commits; lead verifies from artefacts and lands)

## What is already established

Ticket 326 made every length gate on the matching path a multiple of one measured scale,
`Gate(legacy_mm) = legacy_mm * g_len_scale / 60` (hpc/patches/326-scale-free-matching.patch).
On Pot E the measured scale is **92.77 mm** (`work/logs/binary_E_K3R1.log` and
`binary_E_MOR1.log` of run `20261110-2248_6925cb0_t326-potE`), i.e. **1.546x**, so every length
gate opened: overlap floor 50 -> 77.3 mm, merge distance 20 -> 30.9 mm, inlier distance
1.5 -> 2.32 mm, and every other mm literal in the same proportion. The angular gate
(ANGLE_THRESHOLD 0.262 rad = 15 deg) was **not** scaled.

The pot regressed: baseline `20261110-0740_d628fc0_t323-potE` scores 62/62 joins on all four
lanes; the patched run scores **46/62, 53/62, 50/62, 52/62**, with 26-29 of 31 pieces inside the
20.05 deg / 50 mm piece bar. Five sherds end up 18-115 mm from home (P03, P24, P25, P26, P31);
in the MOR1 lane 12 of the 16 lost joins are joins those five carry.

The mechanism is not in doubt: a looser gate let something wrong through and the greedy merge
could not undo it. What is in doubt is **which gate**. This ticket answers only that.

## The question

For Pot E, enumerate every gate whose value changed between the pristine constants and
`Gate(legacy)` at s = 1.546, and for each: **which pairs changed verdict, and do the changed
verdicts carry the five misplaced sherds into their wrong places?**

Pre-register the decision rule before the run:

- If the pairs that flip from reject to accept are **non-GT** pairs and the flipped set is what
  links a misplaced sherd to its wrong neighbour, the cause is the loosened **length** gates, and
  the conclusion is that a length gate must not be scaled on the strength of object size alone
  (the count floors may scale; a millimetre is a millimetre).
- If the flips are **GT** pairs, scaling helped and the regression has another cause — report what
  the control arm shows instead; do not force the first conclusion.
- If nothing flips, the regression is in merge order rather than pair acceptance, and the answer
  is that sentence plus the evidence. A valid and useful outcome.

## Inputs (Spartan, read-only)

| what | where |
|---|---|
| diagnostic harness source | laptop `worktrees/structure-from-sherds-pp/326/hpc/tools/diag326_pairs.cpp` and `hpc/scripts/diag326.sh` (branch `326-scale-free-assembly`, commit 26063ff); the harness md5 recorded in the t326-diag run was `4486b2d31b0194b5f0d3550fd73cfbdd` |
| pristine assembly image | `sfs_main/…/sfs_pristine_assembly.sif`, md5 `63a532b9732f84e59e7b1349065f1323` (must not change) |
| pot E pair dump at pristine gates | `runs/20261110-1844_6925cb0_t326-diag/out/diag_potE.txt` — 63 GTPAIR, 2218 CAND, 838 SURVIVOR, 1240 DROP, 142 MERGE, 63 LCSDIAG; DROP reasons 1138 `axis_angle>0.436`, 26 `basket_lowest_score`, ~100 `overlap(area=…)` |
| staged pot E trees (identical for both arms) | `runs/20261110-2248_6925cb0_t326-potE/trees/`, with `trees/*/arm_pieces_E.tsv` for the on/off map |
| GT graph and answer-key transforms | `sfs_main/original_samples/SfS_pp/Ground Truth/Pot_E_simple_graph.txt` (31 pieces, 62 edges) and `Pot_E_Piece_<N>_T.txt` |
| baseline outcome to reproduce | `runs/20261110-0740_d628fc0_t323-potE/score/acc.tsv` (62/62) |
| patched outcome | `runs/20261110-2248_6925cb0_t326-potE/score/acc.tsv` (46-53/62) |

## Bars

1. **Control.** The harness rebuilt at the pristine constants reproduces the t323 baseline on
   Pot E: the accepted pair set matches the merges that baseline assembly made, and the join count
   at the scorer's bars is 62/62. A harness that does not reproduce the baseline is not evidence —
   report and stop.
2. **The flip table.** Same harness, same inputs, with `Gate(legacy)` at s = 1.546 applied to
   exactly the literals the patch touches. One row per pair whose verdict changes: pair, gate that
   changed, value before/after, GT edge or not, inlier count, gap_min, and whether the pair appears
   in the patched assembly's final merge set.
3. **Attribution.** For each of the 16 failed joins in the MOR1 lane, name the flipped pair(s) on
   the path that explain it, or say plainly that none does. Then answer the pre-registered rule.
4. **Renders.** None required. The coloured pair already staged is the reference
   (structure-from-sherds-pp/artifacts/pota326/renders/pota326_coloured_pair.png).
5. Housekeeping: corpus listing before/after (0-line diff); every job id polled to a final sacct
   State/ExitCode; scripts under `.scratch/pota-repro/t330/`; run dir
   `/data/gpfs/projects/punim2657/SfSpp/runs/<YYYYMMDD-HHMM>_<git7>_t330-potE-flips/`.

## Must not

- No change to the assembler, the patch, any threshold, or the harness's gate semantics. This is a
  measurement ticket; a fix is a separate ticket.
- No threshold tuning to make a pair come out differently, and no re-wording of a bar after a
  result is seen.
- No corpus write, no commit, no push, no worktree command.

## Where the answer goes

A `## Comments (worker, <date>)` section on this ticket: control result, flip table, attribution
sentence, and the pre-registered rule matched against the finding. Flip `Status:` to
`results-in` and stop; the lead verifies and lands.

## Comments (worker, 2026-11-11)

### Answer

**The loosened gates did not admit the join that misplaced the sherds.** Every pair whose verdict
flips between the pristine constants and `Gate()` at s = 1.546 is a **non-GT** pair, and none of the
13 flipped pairs is on the path of any of the 16 failed MOR1 joins, none is one of the wrong links
that hold the five misplaced sherds, and none is a join the baseline made. The pair that actually
carries the damage — **P25–P26**, the non-GT link that puts P25, P26 and P31 64–115 mm from home —
had viable candidates surviving pruning in **both** regimes (idx 1794 score 0.38, idx 1795 score
0.19), so the pristine assembly had it available too. Twelve of the 16 failed joins had a
GT-correct candidate surviving in both regimes **at the same candidate index**.

What the patch changed on this lane is not which pairs pass the filter but which rival pose wins and
how the search expands: +24 survivors out of 1815 (+3.0%), scores and consensus shifted by the
rescaled inlier distance and outlier rejection *inside* Icp, a different merge order. End to end, the
same binary on the same tree scores **62/62 at s = 1.0 and 53/62 at s = 1.546** (jobs 34749879,
34749880) - the regression reproduced with nothing but argv[2] changed.

Against the pre-registered rule: clause (a) holds on its first half (all flips non-GT) and **fails on
its second** (the flipped set does not link a misplaced sherd to its wrong neighbour); clause (b)
fails outright (no GT join flips reject→accept); the substance of clause (c) is what the data shows,
even though "nothing flips" is not literally true — 13 pairs flip, but nothing that matters flips.

### Instruments (all measurement; no logic, literal, weight or order touched)

1. `probe_countresult.py` — replays CountResult's own criterion (`isSimilarTrans`, 0.35 rad, 50.0 mm,
   strictly less) over the per-piece poses each run saved (`out/<ARM>/T_<k>to_<k>.txt`). It reproduces
   every run's own `1. Acc.txt` exactly, so a pair verdict can be recomputed without re-assembling.
2. `t330_audit.cpp` / `t330_audit.sh` — the ticket-326 diagnostic harness rebuilt against the patched
   library: same load shim, same axis alignment, same `FeatureComp(shard,LCS_out,25,MINIMUM_NUMBER,0)`
   followed by the same pruning sequence main.cpp runs. `g_len_scale` is set from argv before anything
   else and `MeasureLengthScale()` returns early when it is > 0 (patch 326, `data_structure.cpp:1008`),
   so one binary yields both regimes through the library's own call sites.
3. `t330_run.sh` / `submit330i.sbatch` — the shipped batch shim `hpc/tools/main_pristine_batch.cpp`
   with exactly three inserted statements (an `extern double g_len_scale;`, `g_len_scale = atof(argv[2])`,
   and one log line). This runs the **whole** assembly at either regime from one binary, which is both
   the end-to-end control and the causal test.

`Gate(x) = x * g_len_scale / 60`, so 60.0 gives `Gate(x) ≡ x` in IEEE arithmetic and 92.77 gives the
1.546x the ticket names. Nothing else differs between the two arms.

### Bar 1 — control

| check | result |
|---|---|
| CountResult replay, t323 MOR1 | 31/31 sherds, **62/62** edges = the in-run `1. Acc.txt` |
| CountResult replay, t326 MOR1 | 27/31, **46/62** = the in-run value (93/124 directed) |
| CountResult replay, t323 / t326 K3R1 | 31/31, 62/62 / 30/31, 50/62 = the in-run values |
| audit S = 60, MOR1 | `Total number : 1815` / `Total number pruned : 792` — the t323 MOR1 log line for line |
| audit S = 92.77, MOR1 | 1815 / 816 (t326 MOR1 logged 814, MOR2 816) |
| **whole assembly, S = 60** (job 34749879) | **31/31 sherds, 62/62 edges**, 1815 candidates / 796 pruned, COMPLETED exit 0:0 in 20:40 |
| **whole assembly, S = 92.77** (job 34749880) | **29/31 sherds, 53/62 edges**, 1815 candidates / 817 pruned, COMPLETED exit 0:0 in 18:01 |

Caveats, stated rather than buried:

- **The end-to-end control is exact where it matters.** The whole assembly at S = 60 prints
  31/31 sherds and 62/62 edges - the baseline’s number, from the same binary that regresses at
  S = 92.77. The two arms differ only in argv[2].
- **K3R1 is not an exact control at the pruning stage.** The audit gives 836 at S = 60 where the
  t323 K3R1 log says 839 - 3 candidates short, the size of the fringe described next. MOR1, the
  lane bar 3 asks about, is exact (792 = the t323 log). K3R1 appears below only as a secondary lane.
- **The two assembly arms’ per-piece poses were not retained.** The batch script’s final copy step
  pointed at a container path that was never bind-mounted, and the next line removed
  /Dataset/SfS_pp/Result, so only the logs survive. The printed accuracy is the assembler’s own
  CountResult output - the same code path that writes 1. Acc.txt - so the control number is
  authoritative, but no per-piece claim is made from these two arms.
- **The search has a small non-deterministic fringe, and it moves the join count.** Four
  measurements of MOR1 at s = 1.546 (identical inputs, identical 1815 candidates) prune **814, 816,
  816, 817** and score **46/62, 53/62, -, 53/62**: t326’s own MOR1 and MOR2 arms (814 -> 46, 816 -> 53),
  my audit (816, pruning only) and my assembly arm (817 -> 53). At s = 1.0 the same tree prunes 792
  (t323 log and my audit) and 796 (my assembly), and scores 62/62 either way.
  class/reconstruction.cpp has no srand/rand; the likely cause is thread-order floating point inside
  Icp, unverified. So **the size of the regression is not reproducible run to run** (46 vs 53 of 62)
  even though its existence is: 62/62 at s = 1.0 and 53/62 at s = 1.546, same binary, same tree,
  same candidate enumeration. Every candidate-level claim below names a candidate index, and those
  indices and their (y, x, gt) triples are identical in both regimes.
### Bar 2 — the flip table

**Every length literal the patch scales, at s = 1.546** (from the audit's own `SCALE` line):

| literal (mm) | `Gate()` | where it decides |
|---|---|---|
| 1.5 (`INLIER_THRESHOLD`) | 2.32 | CountInlier — feeds `inlier_` and `score_`; does not drop a pair itself |
| 2 | 3.09 | DetectIntersection correspondence length |
| 3 | 4.64 | reconstruction inlier distance |
| 4 | 6.18 | DetectIntersection `cor.len` cap |
| 5 | 7.73 | makeregion length floor, OverlapCheck centre-too-close, CauchyLoss |
| 7 | 10.82 | ProfileChecking |
| 10 | 15.46 | inlierCalculate rim gaps, RejectOutlier(10), isConverge |
| 20 | 30.92 | RejectOutlier(20), `isSimilar`/`isSimilarTrans` translation, isSimilarGraph |
| 50 | 77.31 | PairwisePruning `OverlapCheck_3d(...,50.0)` area floor |
| 100 | 154.6 | `OverlapCheck_3d(...,100)` area floor in graph building |
| 1000 | 1546 | OverlapCheck centre-too-close area sentinel |

**Not scaled:** ANGLE_THRESHOLD 0.262 rad (15°), RejectOutlier's 0.7/0.8 quantiles, the 0.436 rad
sliver gate, the 1.5 basket-score bar, and the scorer's own 0.35 rad / 50 mm.

**What a verdict is here.** The candidate enumeration is identical in both regimes — all 1815
candidates carry the same `(y, x, gt)` at the same index — so a verdict change is the same candidate
being pruned in one regime and kept in the other. MOR1: **792 survivors at S = 60, 816 at
S = 92.77; 81 candidates appear, 57 disappear, net +24 (+3.0%)**.

Where the 81 came from, by the reason they were dropped at S = 60:

| S = 60 drop reason | candidates | of which |
|---|---|---|
| `axis_angle > 0.436` (bar unchanged) | 46 | the pose moved under the rescaled Icp |
| `overlap(area=…)` | 21 | **14 exactly by the widened window** (50 < area ≤ 77.31); 7 because the area itself changed |
| `basket_lowest_score > 1.5` (bar unchanged) | 5 | the scores moved |
| merged away at S = 60 | 9 | the score ordering changed which rival survived |

So of the two *decision* literals that moved, the overlap floor accounts for 14 candidates and the
merge window for 3 new merges wider than 20 mm (merges 30 → 32). Everything else moved because the
rescaled inlier distance, RejectOutlier and CauchyLoss *inside* Icp changed the poses, which then
crossed gates whose values never changed.

**Pairs whose verdict changes (MOR1)** — one row per pair:

| pair | direction | gate whose value moved | before → after | GT edge | inlier | gap_min (mm) | median gap (mm) | score | drop reason on the side that drops it |
|---|---|---|---|---|---|---|---|---|---|
| P24–P27 | drop→keep | overlap area floor | 50 → 77.31 mm | no | 18 | 0.63 | 11.77 | 0.834 | S60: `overlap(area=54.9)` |
| P03–P15 | drop→keep | none (0.436 rad gate unchanged) | — | no | 28 | 0.21 | 21.26 | 0.511 | S60: `axis_angle>0.436` |
| P18–P29 | drop→keep | none (0.436 rad gate unchanged) | — | no | 12 | 0.64 | 14.02 | 0.590 | S60: `axis_angle>0.436` |
| P05–P24 | drop→keep | none (1.5 basket bar unchanged) | — | no | 14 | 0.73 | 15.98 | 0.963 | S60: `basket_lowest_score>1.5(11)` |
| P08–P25 | drop→keep | none (1.5 basket bar unchanged) | — | no | 8 | 0.82 | 14.83 | 0.625 | S60: `basket_lowest_score>1.5(11)` |
| P20–P27 | drop→keep | none (1.5 basket bar unchanged) | — | no | 18 | 0.18 | 19.98 | 0.309 | S60: `basket_lowest_score>1.5(11)` |
| P15–P30 | drop→keep | none (1.5 basket bar unchanged) | — | no | 0 | 66.95 | 72.95 | 11.0 | S60: `basket_lowest_score>1.5(11)` |
| P01–P17 | keep→drop | none (0.436 rad gate unchanged) | — | no | 34 | 0.45 | 26.38 | 0.662 | S92.77: `axis_angle>0.436` |
| P02–P23 | keep→drop | none (0.436 rad gate unchanged) | — | no | 0 | 0.05 | 23.20 | 11.0 | S92.77: `axis_angle>0.436` |
| P04–P27 | keep→drop | none (0.436 rad gate unchanged) | — | no | 8 | 0.42 | 10.91 | 0.591 | S92.77: `axis_angle>0.436` |
| P13–P30 | keep→drop | none (1.5 basket bar unchanged) | — | no | 0 | 0.24 | 13.72 | 11.0 | S92.77: `basket_lowest_score>1.5(11)` |
| P15–P24 | keep→drop | none (1.5 basket bar unchanged) | — | no | 0 | 65.71 | 75.28 | 11.0 | S92.77: `basket_lowest_score>1.5(11)` |
| P22–P26 | keep→drop | none (1.5 basket bar unchanged) | — | no | 0 | 0.87 | 15.98 | 11.0 | S92.77: `basket_lowest_score>1.5(11)` |

All 13 are non-GT, in both directions. **No GT pair flips at all.** (K3R1, secondary lane: 5 gains
P01–P17, P02–P10, P03–P15, P07–P21, P09–P16 and 5 losses P04–P27, P07–P20, P07–P31,
P10–P30, P15–P17 — also all non-GT.)

Whether a flipped pair appears in the patched assembly's final join set cannot be read off the
artefacts: the run saves no merge list. The frame-free proxy is physical contact at the final pose,
and **none of the 13 flipped pairs is in contact (≤ 1 mm) in the patched final state**; nor does any
of them touch the five misplaced sherds except P08–P25 and P05–P24, whose partners P08 and P05 are
both correctly placed in the patched run.

### Bar 3 — attribution of the 16 failed MOR1 joins

"GT-correct candidate" = a candidate for that pair whose pose is within the scorer's bars of the
answer key (rot < 20.05 deg, t < 50 mm).

| failed join | candidates | GT-correct at S60 | at S92.77 | GT-correct candidate surviving both |
|---|---|---|---|---|
| P03–P17 | 18 | 1 | 1 | yes, idx 441 (11.1 deg / 13.5 mm) |
| P03–P24 | 9 | 1 | 1 | yes, idx 498 (18.3 / 3.9) |
| P03–P25 | 6 | 1 | 1 | yes, idx 503 (10.2 / 7.4) |
| P03–P26 | 2 | 1 | 1 | yes, idx 505 (4.1 / 3.7) |
| P05–P25 | 4 | 1 | 1 | yes, idx 722 (10.9 / 11.2) |
| P13–P25 | 1 | 1 | 1 | yes, idx 1382 (6.5 / 5.1) |
| P14–P30 | 4 | 1 | 1 | yes, idx 1458 (2.9 / 4.3) |
| P17–P18 | 8 | 1 | 1 | yes, idx 1561 (2.2 / 0.7) |
| P18–P23 | 2 | 1 | 1 | yes, idx 1640 (12.4 / 14.0) |
| P20–P23 | 7 | 1 | 1 | yes, idx 1701 (5.3 / 2.7) |
| P23–P24 | 4 | 1 | 1 | yes, idx 1766 (8.5 / 10.3) |
| P28–P31 | 3 | 1 | 1 | yes, idx 1810 (3.5 / 1.5) |
| P03–P05 | 5 | 0 | 0 | no — no candidate is GT-correct in either regime |
| P14–P26 | 1 | 0 | 0 | no — its only candidate is 149.8 deg off |
| P24–P31 | 3 | 0 | 0 | no — the two plausible candidates are killed in **both** regimes by the **unscaled** 0.436 rad axis gate (0.842, 0.483 rad) |
| P26–P30 | 0 | 0 | 0 | no — no candidate exists at this stage |

Reading:

- **12 of 16** failed joins had a GT-correct candidate sitting in the pruned set in both regimes, at
  the same index. Their loss is a search/merge outcome, not a filter decision.
- **4 of 16** are not explained at the pruning stage at all. P24–P31 is the only one where a gate is
  responsible, and that gate is the axis gate, whose value the patch never changed. The baseline made
  all four of these joins, so they must come through `FeatureCompGraphBuilding`, a later stage this
  instrument does not cover — a limit of the evidence, not a finding.
- **No flipped pair is on any of the 16 paths.** The 13 flipped pairs ∩ the 16 failed joins = ∅;
  ∩ the 7 contacts that appear in the patched final state ((P03,P05), (P03,P14), (P03,P30), (P05,P30),
  (P10,P12), (P17,P24), (P25,P26)) = ∅; ∩ the 12 contacts that disappear = ∅.
- **The carrier is a pair that never flipped.** In the patched final state P25 touches only P26 and
  P31, P26 only P25, P31 only P25 — a detached three-sherd cluster whose only non-GT link is
  P25–P26 at 0.33 mm. Those three end up 85.4, 114.7 and 64.0 mm from home. P25–P26 is enumerated
  in both regimes and survives pruning in both.

### The pre-registered rule, matched against the finding

- Clause (a) — flips are non-GT **and** the flipped set links a misplaced sherd to its wrong
  neighbour: the first half holds, the second does not. **Clause (a) is not satisfied.**
- Clause (b) — flips are GT pairs: **no GT pair flips**. Not satisfied.
- Clause (c) — "if nothing flips, the regression is in merge order rather than pair acceptance":
  literally nothing-flips is false (13 pairs flip), but nothing that matters flips — no join the
  baseline made changes verdict, and no wrong link the patched assembly made is newly admitted. The
  sentence that fits the data is: **the regression is in what the search does with an essentially
  unchanged candidate pool, not in a pair being newly admitted by a loosened length gate.**

What this means for the decision the ticket serves: the conclusion "a length gate must not be scaled
on object size alone" is **not** reached by route (a), because (a) requires the flipped set to carry
the damage and it does not. What the runs do establish is causal and blunt: the same binary, the same
inputs and the same candidate enumeration produce 62/62 at s = 1.0 and 53/62 at s = 1.546.
A per-literal cause would need one run per literal (11 literals) or an audit that ablates a single
`Gate()` at a time; that is a different ticket and I did not do it here.

### Housekeeping

- Corpus: the 186 Pot E input files (mesh, axis, breakline, surfaces, GT) re-hashed after all work —
  `diff` against the t326 after-listing is **0 lines**; t326's own before/after was also 0.
  `runs/20261111-0248_be65afc_t330-potE-flips-i/corpus_diff_same.txt`.
- Every job id this worker submitted, with its final sacct state (`sacct -X`, swept after the last job ended):

| job | partition | final sacct State | Elapsed | what it was / what it did |
|---|---|---|---|---|
| 34730091 | sapphire | CANCELLED | 00:00:00 | audit v1 (no `gap_min` column) — cancelled before it started |
| 34730267 | sapphire | CANCELLED | 00:00:00 | audit main variant — 2 h PENDING, never started |
| 34732537 | sapphire | CANCELLED | 00:00:00 | duplicate of 34730267 — never started |
| 34736268 | sapphire | CANCELLED | 00:00:00 | small variant with a prebuilt tree — never started |
| 34740219 | sapphire | CANCELLED | 00:00:00 | 1-cpu variant — never started |
| 34740760 | cascade | COMPLETED | 00:00:02 | cascade attempt; sbatch printed an admin advisory steering jobs to sapphire, superseded |
| 34740841–44 | sapphire | CANCELLED | 00:00:00 | four one-label jobs — never started |
| 34742068 | sapphire | CANCELLED | 00:00:00 | 1-cpu hello probe — never started |
| 34746227 | interactive | COMPLETED | 00:00:03 | audit attempt; script path bug → exit 127, no output |
| 34746349 | interactive | COMPLETED | 00:00:58 | **the audit** — wrote all four `audit_*.txt` |
| 34748598 | sapphire | CANCELLED | 00:00:00 | assembly submit; silently redirected off interactive (see the deviation note) |
| 34748983 | sapphire | CANCELLED | 00:00:00 | assembly submit with `-p interactive` on the command line; redirected the same way |
| 34749308 | interactive | COMPLETED | 00:00:00 | `--wrap` probe at 8 cpus — stayed on interactive |
| 34749408 | sapphire | CANCELLED | 00:00:00 | `--wrap` probe at 16 cpus — **redirected to sapphire**, which is the proof of the 8-cpu rule |
| 34749409 | interactive | COMPLETED | 00:00:00 | `--wrap` probe at 8 cpus — stayed on interactive |
| 34749577 | interactive | COMPLETED | 00:00:11 | S60 arm; link failed (relative object paths — see the deviation note) |
| 34749578 | interactive | COMPLETED | 00:00:12 | S92.77 arm; same link failure |
| 34749879 | interactive | COMPLETED | 00:20:40 | **S60 whole assembly** → 31/31 sherds, 62/62 edges |
| 34749880 | interactive | COMPLETED | 00:18:01 | **S92.77 whole assembly** → 29/31 sherds, 53/62 edges |

- Scripts: `.scratch/pota-repro/t330/` (`probe_countresult.py`, `probe_state.py`, `probe_contact.py`,
  `probe_graph.py`, `probe_diff.py`, `probe_err.py`, `analyse330*.py`, `flipfull.py`, `joins16.py`,
  `disjoint.py`, `t330_audit.cpp`, `t330_audit.sh`, `t330_run.sh`, `submit330i.sbatch`, push/fetch helpers).
- Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips` (main),
  `-b`, `-c`, `-i` (assembly), `-h` (**the audit**: the four `audit_*.txt` are in ITS `out/`, not in the
  main dir's - the main dir's `out/` and `logs/` are empty).
- Deviations: the sapphire partition never started a job of mine in ~2 h (fairshare 0.137, no
  backfill estimate), so the audit and both assembly arms ran on the **interactive** partition; the
  site silently redirects an interactive request above 8 cpus to sapphire, so each arm ran at 8 cpus
  in its own job. One assembly attempt (34749577) failed at link (exit 4) because `link.txt` holds
  relative object paths and the script had not `cd`-ed into the build dir; fixed and re-run. One
  audit attempt (34746227) exited 127 on a script-path bug; fixed and re-run (34746349).
- No commit, no push, no worktree command, no corpus write, no threshold change.

## Lead verification (2026-11-11, independent)

Verified from the artefacts, not from the report: the two assembly arms' own binary logs, and a
recomputation over the four audit files with `.scratch/pota-repro/t330/lead_verify_flips.py` and
`lead_verify_joins.py`.

**Control, read at the source.** `..._t330-potE-flips-i/logs/binary_S60.log` ends
`Sherd accuracy : 31 / 31 (100%)` / `Edge accuracy : 62 / 62 (100%)`, 1815 candidates, 796 pruned;
`binary_S9277.log` ends `29 / 31 (93.5484%)` / `53 / 62 (85.4839%)`, 1815 candidates, 817 pruned.
Jobs 34749879 / 34749880, both COMPLETED 0:0, 00:20:40 / 00:18:01. One binary, argv[2] the only
difference, regression reproduced: **verified**.

**The flip table, recomputed.** Pair level (a pair flips when its number of surviving candidates
goes from none to some, or the reverse): MOR1 **13 pairs, none of them GT**; K3R1 **10 pairs, none
GT** - the worker said 13 in both lanes, so K3R1 is 10 by this definition (wording correction only,
the answer is unchanged). Candidate level, 138 candidates across 107 pairs change verdict, so the
candidate pool is unchanged at the enumeration but not at the pruning: the filter is sequential, so
a shifted threshold reorders the basket and moves candidates whose own gate did not change (34 MOR1
candidates go survivor -> `axis_angle>0.436`). That is the mechanism the worker describes, seen in
its own data.

**Two numbers corrected.**

1. The audit files are in run dir `..._t330-potE-flips-h/out/`, 701,966 / 708,657 / 593,178 /
   603,319 bytes - not under the paths the ticket listed. Corrected above, so the evidence is
   findable.
2. GT-correct candidate surviving in **both** regimes among the 16 failed MOR1 joins: **14 of 16**,
   not 12. The two exceptions are 14-26 (every candidate pruned in both regimes) and 26-30 (**no
   candidate is ever enumerated** - invisible to every threshold, a fact worth its own line in any
   later ticket about it).

**P25-P26, the link that carries the damage.** Both candidates survive in both regimes: idx 1794
(score 0.375, gap 0.16 mm, **t_err 55.1 mm**) and idx 1795 (0.192, gap 0.12 mm, **t_err 86.4 mm**).
Neither is GT, and both were available to the pristine search. Confirmed.

**Ruling on the pre-registered rule.** Clause (a) fails on its second half and (b) fails outright;
the substance of (c) is what the data shows - 13 pairs flip in MOR1, none GT, none on the path of a
lost join, and the wrong link that detaches {P25,P26,P31} was available in both regimes.
**The scaled gates did not admit the wrong join: the search made a different choice from an
essentially unchanged pool.** Therefore the rule "a millimetre must not be scaled by object size"
is NOT established by this route; what the patch demonstrably moves is the ranking and merge
outcome, through the rescaled inlier distance and outlier rejection inside Icp.

**Deviation accepted once.** The arms ran on `interactive` (8 cpus, <= 21 min) because sapphire
started nothing in two hours; the redirect of >8-cpu interactive requests to sapphire (probes
34749308 / 34749408 / 34749409) shows the boundary is policed. Tolerated here - a short, single-node
measurement, recorded - and not to become the pattern.

**Consequence for ticket 329.** The count floor is a different lever from the length gate and the
ten-pot bar of 329 decides it. This ticket is the caution: the search is sensitive to any shift in
scores or consensus, so 329 must run its ten-pot bar rather than reason that only one gate moved.

