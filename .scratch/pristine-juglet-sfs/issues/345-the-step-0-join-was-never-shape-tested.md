# Ticket 345 — the step-0 join was never shape-tested: what does the test say about it

**Status:** in-progress
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** 345-the-step-0-join-was-never-shape-tested, in /c/PR/worktrees/structure-from-sherds-pp/345
**Answers:** S1
**Depends on:** 341 (the gate's numbers and the step-0 skip), 340 (which candidate is applied), 342 (the winner)
**Blocks:** ticket 344 (the bar arm) and ticket 346 (the picker) — neither reading can be trusted until this one exists
**Worker:** dsh one-shot subagent d299a21d-a249-4b44-a188-1977506ac34e, dispatched 2026-11-14 by the lead
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

- [ ] `SFS_STEP0_TRACE` read once, default off, banner both ways; zero `[SFS-345]` lines when off.
- [ ] `git diff HEAD -U0 -- class` additions only; `ProfileChecking` unchanged in behaviour.
- [ ] Part B table for both trace-on repeats, with 1-8's control row.
- [ ] One reading named from Part C's list, with the three numbers it rests on.
- [ ] Trace-on winner identical to 342's (state id, score, edge count, cluster count).
- [ ] Signed 5/3 2/9 and 1/18 in all three signed arms; Pot A 8/8 and 15/15.
- [ ] `sacct` State and ExitCode for every job id; tree md5 identical per arm.
- [ ] `## Comments (worker, 2026-11-14)` section, ending with exactly one `**End state:**` line.
