# Ticket 346 — which candidate of a pair the picker takes, and by what number

**Status:** needs-info
**Waits for:** ticket 345 (the step-0 dry run, same file — tickets sharing files run in order)
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`; work only in the worktree it names
**Answers:** S1
**Depends on:** 340 (which candidate the search applies), 341 (the gate's refusals), 339 (the priority order is the window)
**Blocks:** any change to the priority order, and ticket 344 if 345 reads `step0-shape-splits-within-4-7`
**Worker:** unassigned
**Budget:** small (4 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: one print of the ordering key each candidate carried, joined to the accept; the hidden check is the table in Part B.

> **In one line.** Within pair 4-7 the search applies 144 (18.69 deg / 14.86 mm, `score_` .559114, inlier 36)
> and never the in-bar 145 (3.35 deg / 2.68 mm, .359451, inlier 28); within 6-9 it applies 66 (r1) / 55 (r2),
> which have no candidate-stage row at all. This ticket asks what number put those edges where they are.

## What is already known

- The list is built by `CalculateMatchingScore` (`class/ranking_system.h:282`, defined
  `class/ranking_system.cpp:1639`, called at `:394` and `:409` on graph edges), and `BuildStep` walks it in
  order, trying only the first `s_` = 3 entries (ticket 339).
- Over the 51 candidates of one run, `score_` does not predict correctness in the direction the search uses:
  the five candidates inside the 5 deg / 5 mm bar all have `score_` <= 0.447 while most garbage sits above
  0.6 (`rho(score_, rot) = +0.458`). The inlier count predicts nothing (`rho(inlier, rot) = -0.190`).
- So if the order follows `CalculateMatchingScore`, and that quantity is built from inlier counts, the
  search is ordering by the quantity that does not predict accuracy. This ticket does not assume that; it
  prints the keys and lets the table say it.

## Part A — the switch

One switch, exact string, default off, additions only:

- `SFS_ORDER_TRACE=1`, banner `order trace: on` / `order trace: off`.
- When the priority list for a state is built (the list `BuildStep` then walks), print one line per entry:

      [SFS-346] RANK <step> <state> <j> <y> <x> <index_> <score_> <inliner_> <matching_score>

  - `matching_score` — the value `CalculateMatchingScore` returned for that entry, read at the sort site
    (not recomputed at the print site, or the line reports a number the search never used).
  - one line per entry, so a pair offered twice in one state shows both, in list order.
- Do **not** print an accept line: ticket 340's `[SFS-340] PICK` already names the applied edge. Run
  `SFS_PICK_TRACE=1` alongside and join on (step, state, y, x, index_). Do not re-implement any existing
  token.
- No ordering, threshold, gate, `TOP_k`, `BRANCH_b` or container change; GT-free in `class/`;
  `git diff --stat -- class` must show 0 deletions.

## Part B — the table

For each of 4-7, 6-9, 1-2 and 1-8, per repeat: every `RANK` line of that pair in one state (j, `index_`,
`score_`, `inliner_`, `matching_score`), the `PICK` line that followed, and whether the picked entry carried
the highest `matching_score` of its pair. Note the state id for each block; 342 showed the winner differs
between repeats, so do not pool them.

| pair | applied | its key | other entries of the pair | top of the pair? |
|---|---|---|---|---|
| 4-7 | 144 | ? | 145 (.359451 / 28) and the six refused .36-.52 edges | ? |
| 6-9 | 66 (r1) / 55 (r2) | ? | 170 (.343843 / 18) | ? |
| 1-2 | 1, then 3 at step 3 (340) | ? | — | ? |
| 1-8 | 70, applied 4-5 times | ? | — | control |

## Part C — readings

Name exactly one:

- **`order-follows-matching-score`** — the applied entry is the top of its pair by `matching_score` in both
  repeats. The lever is the formula: the search orders by a quantity that does not predict accuracy. The
  next ticket is one print-only comparison of `matching_score` against the candidate-stage `score_` and the
  inlier count over all 51 candidates, so a replacement key is chosen from measurement rather than taste.
- **`order-is-not-the-key`** — the applied entry is not the top of its pair by `matching_score`. Something
  removes or reorders the better one first: `RemoveSamePriority` (`class/ranking_system.cpp:848`, called at
  `:633`) or the double-listing artefact 339 recorded. Trace that for the pair and name it.
- **`no-choice`** — the two candidates of the pair never appear in the same state's list, so the picker
  never chose between them: 340's 144-over-145 is an artefact of them being offered in different states.
  Then 345's step-0 reading and 344's bar stay the levers, and this ticket ends with that.
- **`unreadable`** — the key cannot be read at the sort site without changing the sort (for example the
  score is consumed by the comparator and not stored). Name the site and what it would take; end
  **incomplete**.

## Part D — what this cannot settle

- One pot, signed sweep; and the list is rebuilt per state, so a key read in one state says nothing about
  another.
- It cannot say which key would be better — only whether the one in use is the one the search follows.
- `matching_score` is a sum over inlier counts by construction (`class/ranking_system.cpp:1639`); if the
  table shows it dominating, that is a measurement, not yet an argument to invert it.

## Part E — arms

| arm | switches | expectation |
|---|---|---|
| `order_on_r1` | `SFS_ORDER_TRACE=1 SFS_PICK_TRACE=1 SFS_WIN_TRACE=1` | 2/9 sherds, 1/18 edges; RANK lines; winner unchanged |
| `order_on_r2` | same | repeats r1 |
| `order_unset` | all unset | 2/9 and 1/18; zero `[SFS-346]` lines |
| `order_pota` | default | Pot A 8/8 and 15/15, else VOID |

Private tree copy per arm, md5 before/after, `sbatch` only, poll, record `sacct`. Winner must match 342's
(r1 `WINNER 30 260 4 3`, r2 `WINNER 33 263 5 3`) or the run is not print-only.

## Acceptance

- [ ] `SFS_ORDER_TRACE` read once, default off, banner both ways; zero lines when off.
- [ ] `git diff HEAD -U0 -- class` additions only; no existing token re-implemented.
- [ ] Part B table for both trace-on repeats, with the four pairs and the control.
- [ ] One reading named from Part C, with the keys it rests on.
- [ ] Winner identical to 342's in both trace-on arms.
- [ ] Signed 5/3 2/9 and 1/18 in all three signed arms; Pot A 8/8 and 15/15.
- [ ] `sacct` State and ExitCode for every job id; tree md5 identical per arm.
- [ ] `## Comments (worker, 2026-11-14)` section, ending with exactly one `**End state:**` line.
