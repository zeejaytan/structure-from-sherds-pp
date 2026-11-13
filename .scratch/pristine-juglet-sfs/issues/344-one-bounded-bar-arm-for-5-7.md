# Ticket 344 — one bounded bar arm: does admitting 5-7 buy accuracy

**Status:** ready-for-agent
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`; work only in the worktree it names
**Answers:** S1
**Depends on:** 341 (the numbers behind the refusals), 340 (which candidate is applied), 337 (widening the branch window did not help)
**Blocks:** any change to the plausibility gate's bars
**Worker:** unassigned
**Budget:** small (4 failed acceptance runs)
**Hand-over:** DeepSeek: one env override and four arms; the hidden check is the arm table in Part B.

> **In one line.** The 5-7 join is refused eight times at a profile distance of 8.856-8.860 mm
> against a 7.0 mm bar — 1.9 mm, or 27%, over — so one arm with that single bar at 9.0 asks the
> only question left about it: does the pot get better when the gate stops saying no?

## Why this and nothing wider

Ticket 341 printed the number each refusal is about. Three pairs were refused while their best
candidate is inside the 5 deg / 5 mm bar, and only one of them is a bar question:

- **5-7** — eight refusals at 8.856-8.860 against 7.0. The smallest margin in the set, and the pair
  is already accepted once at step 0. This ticket.
- **4-7** — its only applied join is candidate 144, 18.69 deg / 14.86 mm, already outside the bar;
  the in-bar 145 is never applied. That is the picker's choice, not a bar.
- **6-9** — refused on edge overlap 73.70 / 76.06 against 50 (47-52% over) with an applied edge
  (id 66) that has no candidate row. Too far over to read as a tuning question, and its identity
  is unresolved.

Ticket 337 already showed that widening the branch window does not help. This is a different lever
and it must be tested on its own, with the default kept as the default.

## Part A — the switch

One environment override, read once, default unchanged:

- `SFS_PC_BAR` — the profile-distance bar passed to `ProfileChecking` at
  `class/ranking_system.cpp:2529` (today a literal `7.0`, twice). Read as a double, reject <= 0
  or > 20, print the resolved value beside the existing line, e.g. `pc bar: 7` or `pc bar: 9`.
- No other threshold, gate, TOP_k, BRANCH_b, ordering or container changes. The rim margin (10.0)
  and the overlap bar (50) stay literal. Additions only, GT-free in `class/`.

Print nothing else new. The 341 veto and edge lines already exist and stay behind `SFS_VETO_TRACE`.

## Part B — the arms

Four arms, all on the signed 5/3 set, the same tree per arm as ticket 341 (private copy, md5
before and after):

| arm | switches | expectation |
|---|---|---|
| `bar_default_on_r1` | `SFS_VETO_TRACE=1` only | 2/9 sherds, 1/18 edges; `pc bar: 7`; the eight 5-7 refusals still there |
| `bar9_on_r1` | `SFS_PC_BAR=9` `SFS_VETO_TRACE=1` | `pc bar: 9`; the 5-7 refusals at 8.856-8.860 disappear (each < 9.0), the 11.2 mm 4-7 ones stay |
| `bar9_on_r2` | same as above | repeats r1 |
| `bar9_pota` | `SFS_PC_BAR=9` | Pot A must still read 8/8 and 15/15 |

Plus one fidelity arm with every switch unset (`bar_unset`) at the default bar, which must read
2/9 and 1/18 and print no trace line. Record `sacct` State/ExitCode for every job.

## Part C — readings

- **`bar-admits-and-buys`** — signed accuracy beats 2/9 or 1/18 in both `bar9` repeats and Pot A
  stays 8/8 15/15. The bar is a live lever; the next ticket sweeps it (9.5, 10) and checks whether
  the newly admitted join appears in the winner's edge list.
- **`bar-admits-and-costs-nothing`** — the refusals vanish, signed accuracy does not move, Pot A
  intact. The gate is not what is costing accuracy; the lever is the picker (ticket 340's 144-over-145)
  or the step-0 skip.
- **`bar-admits-and-breaks-pota`** — Pot A drops below 8/8 or 15/15. The bar is load-bearing:
  record the exact drop and do not move it again without a per-pair argument.
- **`bar-does-not-reach`** — the 5-7 refusals survive at bar 9. Then the distance printed is not the
  value compared (report it, do not widen further).

State plainly which reading the numbers give; do not conclude which is the better pot.

## Part D — what this cannot settle

- The bar is only consulted for `step_counter > 0` (`class/ranking_system.cpp:2531`), so no step-0
  join changes. 5-7's step-0 accept is unaffected by any value of this bar.
- The judged edge's identity is still open (341): if the edge at 8.86 is a re-scored or regenerated
  5-7 edge, admitting it is still a real change to the graph — but it says nothing about candidate 160.
- One pot, signed sweep: a move from 2/9 to 3/9 is one sherd and could be chance. Say so.

## Acceptance

- [ ] `SFS_PC_BAR` read once, default 7.0, banner printed, invalid values rejected.
- [ ] `git diff HEAD -U0 -- class` additions only; no other literal changed.
- [ ] Four arms plus fidelity, `sacct` recorded, tree md5 unchanged per arm.
- [ ] The 5-7 refusals counted per arm at each bar; the 4-7 11.2 mm refusals counted (they must stay).
- [ ] Pot A at bar 9 reads 8/8 and 15/15, or the drop is recorded exactly.
- [ ] Part B table, one reading named, one `**End state:**` line.
