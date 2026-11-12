
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
