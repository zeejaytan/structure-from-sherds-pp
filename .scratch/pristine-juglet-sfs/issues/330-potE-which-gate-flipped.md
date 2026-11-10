# 330 — Pot E: which of the scaled gates admits the wrong joins

**Status:** ready-for-agent
**Answers:** S1
**Depends on:** 326 (the patch and the diagnostic harness were built on branch `326-scale-free-assembly`; the harness source is on disk in the lead's 326 worktree)
**Blocks:** nothing. Ticket 329 does not wait on this: its own rule leaves every pot at s = 1.000 by construction.
**Worker:** unassigned
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
