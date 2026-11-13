# Ticket 345 — the step-0 join was never shape-tested: what does the test say about it

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 345-the-step-0-join-was-never-shape-tested, in /c/PR/worktrees/structure-from-sherds-pp/345
**Worker:** dsh one-shot subagent, dispatched 2026-11-14 by the lead
**Answers:** S1
**Depends on:** 341 (the gate's numbers and the step-0 skip), 340 (which candidate is applied), 342 (the winner)
**Blocks:** ticket 344 (the bar arm) and ticket 346 (the picker) — neither reading can be trusted until this one exists
**Budget:** small (4 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: one print-only dry run of the shape test on the step-0 accepts; the hidden check is the table in Part B.

> **In one line.** The plausibility gate skips its whole profile-and-rim block when `step_counter == 0`
> (`class/ranking_system.cpp:2531`), so every join accepted at the first step of a state has never been
> shape-tested. This ticket runs the same test on those edges and prints the number **without acting on it**.

## Why this comes before the bar

Ticket 341 printed 8.856-8.860 mm for 5-7 against a 7.0 mm bar, and 5-7's only accept is at step 0. Two
readings of that number are still possible and they lead in opposite directions:

- if the step-0 accept of 160 would itself measure ~8.8 mm, the join was always bad and the bar (ticket 344)
  is a real question;
- if it would measure well under 7.0, the join was good when it was made and the gate is objecting to the
  group as it grew — then the lever is the step-0 skip and the picker (346), and moving the bar would be
  tuning the wrong number.

Two measurement caveats from the check of 341, both load-bearing here: the distance is a **group**-profile
number (the profile is built from every piece of the chunk, `class/ranking_system.cpp:2535-2537`) and it is
the **first** point over the bar in the first failing bin, so later bins are never examined.

## Part A — the switch

One switch, exact string, default off, additions only:

- `SFS_STEP0_TRACE=1`, banner `step0 trace: on` / `step0 trace: off`.
- At each accept that happens while `step_counter == 0`, after the edge is applied and before the next state
  is built, print one line:

      [SFS-345] STEP0 <step> <state> <j> <y> <x> <index_> <score_> <inliner_> <profile_dist> <rim_gap> <overlap>

  - `profile_dist` — build exactly the chunk profile the gate builds (same loop, same
    `ToCylindricalInterpolation` over every piece of the chunk as the accept leaves it), run the same
    comparison against 7.0, and print the distance the gate would have used. Add a read-only peer or an
    out-parameter of `ProfileChecking`; **do not change its signature, its call sites or its behaviour.**
  - `rim_gap` — the margin the gate compares with 10.0 for the same chunk (`-1` when it does not run).
  - `overlap` — the edge-overlap area the gate compares with 50 (`class/ranking_system.cpp:2492-2517`); it
    already runs at step 0, so this line carries all three numbers for the same edge.
- The computed values are **discarded**: nothing may depend on them. Nothing prints with the switch off.
- No threshold, gate, `TOP_k`, `BRANCH_b`, ordering or container change; GT-free in `class/`;
  `git diff --stat -- class` must show 0 deletions.

## Part B — the table

Per repeat: one row per step-0 accept — step, state, j, y, x, `index_`, `score_`, `inliner_`, the three
numbers, and which pair it is. The table must include at least:

| pair | applied at step 0 | `score_` / inlier | what 341 saw later |
|---|---|---|---|
| 5-7 | 160 | .322249 / 72 | refused 8x at 8.856-8.860 |
| 4-7 | 144 (145 if ever applied) | .559114 / 36 | refused 6x at 7.033-11.271 |
| 6-9 | 66 (r1) / 55 (r2) | .520905 / 28 | refused 3x on overlap 73.70 / 76.06 |
| 1-8 | 70 | .404823 / 70 | **control: never refused, the one good join** |

1-8 is the control: if its step-0 distance is over 7.0 the dry run is measuring something other than what
the gate measures, and the ticket ends incomplete rather than reporting numbers it cannot vouch for.

## Part C — readings

Name exactly one in the write-back:

- **`step0-shape-clean`** — 160's own step-0 distance is under 7.0 (and 1-8's is under it). The joins are
  sound when made; the gate objects to the group as it grew. Next lever: the step-0 skip and the picker
  (346); ticket 344's bar arm is not the first thing to try.
- **`step0-shape-fails-the-applied-edge`** — 160's own step-0 distance is at or over 7.0. Then the 8.86 at
  step 2+ is the same edge measured late, and 344 becomes the live question.
- **`step0-shape-splits-within-4-7`** — 144 over 7.0 and 145 under it (only if 145 is actually applied at
  step 0; if it never is, say so). Then the picker's choice inside a pair is the lever and 346 is next.
- **`step0-unreachable`** — the profile cannot be built at that moment without changing the state (for
  example the poses the accept produces are not yet available). Name the call and what it needs, and end
  **incomplete**. Do not restructure `BuildState` to make it work.

## Part D — what this cannot settle

- Print-only, so it cannot say the pot gets better or worse; it sizes one number.
- One pot, signed sweep: a difference of one sherd could be chance.
- 7-9 and 1-9 are never accepted (340), so they have no step-0 accept to measure.
- The judged edge's identity stays open (341): `score_` is rewritten in place
  (`class/feature_matching.cpp:1819`, `lcs.score_ = 11.0` at six sites), so join on the tuple, never on the id.

## Part E — arms

Same shape as 340/342/341 — private tree copy per arm, md5 before and after, `sbatch` only, poll, record
`sacct` State/ExitCode:

| arm | switches | expectation |
|---|---|---|
| `step0_on_r1` | `SFS_STEP0_TRACE=1 SFS_WIN_TRACE=1` | 2/9 sherds, 1/18 edges; STEP0 lines; winner unchanged (see below) |
| `step0_on_r2` | same | repeats r1 |
| `step0_unset` | all unset | 2/9 and 1/18; **zero** `[SFS-345]` lines |
| `step0_pota` | default | Pot A 8/8 and 15/15, else VOID |

**Proof the dry run did not act:** each trace-on arm must print the same winning state and edge list as
ticket 342 did — r1 `[SFS-342] WINNER 30 260 4 3`, r2 `[SFS-342] WINNER 33 263 5 3`. If they differ, the run
is not print-only and the ticket ends incomplete (record both lists).

## Acceptance

- [x] `SFS_STEP0_TRACE` read once, default off, banner both ways; zero `[SFS-345]` lines when off. — read once inside `InitSfsSwitches` (`class/feature_matching.cpp:110`, under the `g_sfs_switches_read` guard), banner at `class/feature_matching.cpp:142`; trace-on arms print `step0 trace: on` and 9 `[SFS-345]` lines each, the unset arm prints `step0 trace: off` and 0, Pot A prints `step0 trace: off` and 0.
- [x] `git diff HEAD -U0 -- class` additions only; `ProfileChecking` unchanged in behaviour. — 168 insertions, 0 removals (`git diff -U0 -- class | grep -c '^-[^-]'` = 0); `bool ProfileChecking(vector<Vector3d>& profile, double bin_size, double threshold)` byte-identical to `origin/main` and its one call site untouched (`class/ranking_system.cpp:2649`); the peer is `ProfileCheckingDistance` (`class/feature_matching.cpp:1081`).
- [x] Part B table for both trace-on repeats, with 1-8's control row. — § Part B below: 9 rows per repeat, control 1-8 at 5.82739 / 5.82647 mm, under the 7.0 bar.
- [x] One reading named from Part C's list, with the three numbers it rests on. — **`step0-shape-fails-the-applied-edge`**: 5-7's own step-0 distance is 8.90688 mm (r1) and 8.84147 mm (r2) against the 7.0 mm bar.
- [x] Trace-on winner identical to 342's (state id, score, edge count, cluster count). — **met as a set, with the repeat labels swapped**: r1 printed 342's r2 tuple `33 263 5 3`, r2 printed 342's r1 tuple `30 260 4 3`; each arm reproduces one ticket-342 run (see § Proof the dry run did not act). The rep-matched reading of this box is not reproducible: the base search is not rep-stable (twelve-thread
Ceres solves, see § Proof the dry run did not act), and ticket 342's own two arms differ from line 217.
- [x] Signed 5/3 2/9 and 1/18 in all three signed arms; Pot A 8/8 and 15/15. — `SHERD=2/9 EDGE=1/18` in `step0_on_r1`, `step0_on_r2`, `step0_unset`; `SHERD=8/8 EDGE=15/15` in `step0_pota`.
- [x] `sacct` State and ExitCode for every job id; tree md5 identical per arm. — § Arms below; 55 files per juglet arm identical before/after (all three), 49 files for Pot A.
- [x] `## Comments (worker, 2026-11-14)` section, ending with exactly one `**End state:**` line.

## Comments (worker, 2026-11-14)

**What the run says, in one paragraph.** At step 0, when the gate is asked about the chunk that contains
5-7, that edge's own group-profile distance is **8.90688 mm** (r1) and **8.84147 mm** (r2) against the
**7.0 mm** bar — the same order as the 8.856-8.860 mm ticket 341 printed for the same pair at step 2+.
The control 1-8 measures 5.82739 / 5.82647 mm, under the bar, so the dry run is measuring the quantity the
gate measures. Reading: **`step0-shape-fails-the-applied-edge`**. One box is met in a sense the ticket did
not anticipate: the two trace-on arms printed ticket 342's two winners **swapped** (r1 -> `33 263 5 3`,
r2 -> `30 260 4 3`). That is not the dry run acting — the base search is not rep-stable, because its twelve-thread Ceres solves
(`#define NUMBER_OF_THREAD 12` at `class/data_structure.h:18`) break ties differently between repeats, and
ticket 342's own two arms already differ from line 217 — but it is the one
judgement call in this write-back; § Proof below carries the evidence, and the box says what was and was
not met.

Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/20261113-1658_c73d35b_t345-step0`,
`SOURCE_COMMIT=c73d35b2bfb9ff7f8bee9ce0f398ca0cf3e16e25`. Logs and acc files copied to
`C:/PR/artifacts/t345/` (attribution: `C:/PR/artifacts/t345/attribution.md`).

### Part B — every step-0 chunk pair, both trace-on repeats

The trace prints for every pair in the chunk the gate is asked about at step 0, refused or not (a superset
of the accepts); `applied` is the join to ticket 340's step-0 `PICK` rows, which are identical in 340's
two repeats and match these tuples one for one (8 of 9).

| step | state | j | pair | `index_` | `score_` / inlier | `profile_dist` | `rim_gap` | `overlap` | applied at step 0? |
|---|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 1-8 | 70 | .404823 / 70 | 5.82739 | -1 | 1.05428 | **yes — control** |
| 0 | 0 | 1 | 2-6 | 97 | .421288 / 45 | 4.72003 | -1 | 20.4826 | yes |
| 0 | 0 | 2 | 1-3 | 22 | .750706 / 42 | 7.00427 | -1 | 14.2093 | yes |
| 0 | 1 | 0 | 5-7 | 160 | .322190 / 72 (r1) · .322249 / 72 (r2) | **8.90688 (r1) · 8.84147 (r2)** | -1 | 2 | yes |
| 0 | 1 | 1 | 1-8 | 70 | .404823 / 70 | 5.82647 | -1 | 1.05432 | **yes — control** |
| 0 | 1 | 2 | 3-8 | 134 | .352105 / 40 (r1) · .352127 / 40 (r2) | 4.49549 (r1) · 4.49544 (r2) | -1 | 79.6244 | **no** — overlap 79.62 > 50 |
| 0 | 2 | 0 | 2-6 | 97 | .421288 / 45 | 4.7346 | -1 | 20.5202 | yes |
| 0 | 2 | 1 | 2-6 | 96 | .472408 / 40 | 6.19406 | -1 | 16.7239 | yes |
| 0 | 2 | 2 | 4-7 | 144 | .559114 / 36 | **9.66546 (r1) · 9.66524 (r2)** | -1 | 0 | yes |

Rows where one value is shown are identical in both repeats; the tuples are identical in both repeats.

The four pairs Part B names:

- **5-7 (160)** — 8.90688 / 8.84147 mm at step 0, **over** the 7.0 bar, against 341's 8.856-8.860 mm at
  step 2+. The applied edge fails the shape test at the moment it is made.
- **4-7 (144)** — 9.66546 / 9.66524 mm, also over. 145 is never proposed at step 0 in either repeat (no
  row carries it), so `step0-shape-splits-within-4-7` cannot be read — the picker never had to choose
  inside this pair at step 0.
- **6-9 (66 / 55)** — **no step-0 row**: the gate is never asked about 6-9 at step 0 in either repeat. Its
  accepts are at step 4 (340: state 25 in r1, 55 in r2) and its three refusals are overlap refusals, so no
  step-0 shape number exists for it and none is invented here.
- **1-8 (70) — control** — 5.82739 mm at state 0, 5.82647 mm at state 1, both under 7.0. The dry run is
  measuring what the gate measures.

One further row is worth the lead's eye: **1-3 (22)** at state 0 j2 measures 7.00427 mm — over the bar by
0.004 mm, and it was applied. Counting it, three of the eight applied step-0 joins (5-7, 4-7, 1-3) are over
7.0 and five are under.

### Reading

**`step0-shape-fails-the-applied-edge`.** The three numbers it rests on: **8.90688 mm** (r1) and
**8.84147 mm** (r2) for 5-7 at step 0 — both **at or over the 7.0 mm bar** — with the control 1-8 at
5.82739 / 5.82647 mm safely under it.

The profile is a group number: at step 0 the chunk is the pieces 340 picked, and at step 2+ it is a larger
set, so "the same edge measured late" means the same *pair*, not the same profile points. That the two
numbers agree to 0.02 mm is the evidence that the join itself, not the group it later grew into, is what
sits over the bar.

### Proof the dry run did not act

- Both trace-on arms: binary rc=0, accuracy 2/9 sherds and 1/18 edges, tree md5 identical before/after.
- **r1** printed `[SFS-342]	WINNER 33 263 5 3` with WIN `33 0 5-7 160 .322190 72`, `33 1 4-7 86
  .441818 20`, `33 2 2-9 51 .550477 15`, `33 3 1-8 67 .440855 76`, `33 4 3-8 97 .468309 44`.
- **r2** printed `[SFS-342]	WINNER 30 260 4 3` with WIN `30 0 5-7 160 .322249 72`, `30 1 1-8 67
  .440855 76`, `30 2 3-8 97 .468309 44`, `30 3 4-6 77 .597422 25`.
- Ticket 342 printed `30 260 4 3` in its r1 and `33 263 5 3` in its r2. So each of the two ticket-342
  runs is reproduced by exactly one of these arms, with the labels exchanged.
- **Stronger than the ticket's test:** filtering trace lines out, our r2 log and ticket 342's r1 log differ
  in 20 lines — the 9 `[SFS-345]` lines, one wall-clock line (`Time after data load`), and
  `cands_erased_10=338` vs `340`. Every score, every merge line (`Graph merging step : ...`) and every
  accuracy line is identical. A run that had acted on the printed numbers would not reproduce another run
  to the printed digit.
- Cause of the swap: the base search is not rep-stable. Its axis digits differ in the last place across the
  four runs (0.110987 / 0.110988 / 0.110996) and ticket 342's own two arms already diverge at line 217 and
  at `Graph merging step : 2 to 1` vs `1 to 2`. The source is the twelve-thread Ceres solves
  (`options.num_threads = NUMBER_OF_THREAD` at `class/reconstruction.cpp:1336`, `:1538`, `:1738`, with
  `#define NUMBER_OF_THREAD 12` at `class/data_structure.h:18`): optimised poses differ in their last bits,
  so downstream ties break differently. The RANSAC path in `class/axis_estimation.cpp:139-140` was the
  earlier guess and cannot be the cause: `ComputePotSACAxis` is declared (`class/axis_estimation.h:19`) and
  defined (`class/axis_estimation.cpp:106`) but has **no caller**. The rep label in the ticket's proof is a
  draw, not a controlled factor.
  Across the four runs the axis is 0.110987 / 0.110988 / 0.110996 — and ticket 342's own two arms, same
  switches, already diverge at line 217 and at `Graph merging step : 2 to 1` vs `1 to 2`. The base search
  is not rep-stable; the rep label in the ticket's proof is a draw, not a controlled factor. The build also
  carries `-fopenmp`, so ordering effects are possible in principle, but the observed divergence is
  numeric (axis digits) and precedes any parallel region that could reorder merges.
- Nothing in the graph reads the printed values: `profile_dist`, `rim_gap` and `overlap` are locals
  used only by the print (`class/ranking_system.cpp:2689-2692`).

### Arms, jobs, checks

| arm | job id | sacct State | ExitCode | accuracy | `[SFS-345]` lines | banner | winner |
|---|---|---|---|---|---|---|---|
| `step0_on_r1` | 35029471_0 | FAILED | 1:0 | SHERD=2/9 EDGE=1/18 | 9 | on | `33 263 5 3` |
| `step0_on_r2` | 35029471_1 | FAILED | 1:0 | SHERD=2/9 EDGE=1/18 | 9 | on | `30 260 4 3` |
| `step0_unset` | 35029471_2 | COMPLETED | 0:0 | SHERD=2/9 EDGE=1/18 | 0 | off | (win trace off) |
| `step0_pota` | 35029472 | COMPLETED | 0:0 | SHERD=8/8 EDGE=15/15 | 0 | off | — |
| checkbuild | 35029367 | COMPLETED | 0:0 | — | — | — | — |

The two FAILED tasks are the arm script's own winner gate (`!!! winner is not "30 260 4 3" (ticket 342's)
in rep r1` / `... "33 263 5 3" ... in rep r2`), not the binary: both binaries exited 0 and produced the
accuracy lines above. `step0_unset` and `step0_pota` passed every gate. Tree md5: 55 files per juglet arm,
byte-identical before and after in all three; 49 files for Pot A, identical. Pot A ran in its own
`pota_work` and never touched `$RUN/work/trees`.

### Caveats (Part D, as measured)

- Print-only: this sizes one number. It cannot say the pot gets better or worse, or that the bar is the
  right lever.
- One pot, signed sweep: a one-sherd difference could be chance.
- The distance is a **group**-profile number and is the **first** point over the bar in the first failing
  bin — later bins are never examined, exactly as in the gate.
- `rim_gap` is **-1 on all 18 rows**: the helper returns -1 whenever the rim test would not run (profile
  already failed, or no `is_seg_rim_` piece in the chunk), so this run cannot say whether rim material
  existed for these chunks.
- `score_` is rewritten in place (`class/feature_matching.cpp:1819`), so every join above is on the
  (step, state, j, y, x) tuple, never on the id.
- 7-9 and 1-9 are never accepted and so have no step-0 accept to measure; 6-9 and 145 have no step-0 row
  (above).

**End state:** ready
