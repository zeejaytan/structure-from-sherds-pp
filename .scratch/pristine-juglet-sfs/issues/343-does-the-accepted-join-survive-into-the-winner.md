# Ticket 343 — does the accepted 1-2 join survive into the winning state

**Status:** needs-info
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`; work only in the worktree it names
**Answers:** S1
**Depends on:** 342 (which join the winner carries), 340 (which candidate the search accepts), 338 (the final errors)
**Blocks:** any further change to the merge, the state score or the assembly — this decides which of the three is the lever
**Worker:** unassigned
**Budget:** small (4 failed acceptance runs; waiting on Slurm costs none)
**Hand-over:** DeepSeek: one dump of cluster membership per state; the hidden check is the table in Part B.
**Waits for:** ticket 341 to land — both touch `class/ranking_system.cpp`, so they run in order.

> **In one line.** Ticket 340 showed pair 1-2 accepted with its in-bar candidate (id 1, 2.14 deg /
> 2.20 mm) at step 2. Ticket 342 showed the winning state carries no edge on that pair. Between those
> two facts sits one question: did the join leave the state, or only the record of it?

## Why this is not settled

`RankingSubgraph::edge_` is appended only when an edge is used as a priority entry
(`class/ranking_system.cpp:2270`) or a bridge into a merge (`:2334`). `PrepareGraphBuildinginMerge`
copies the merged graph's nodes (`:2312-2320`) but not its `edge_`. So a state can hold pieces 1 and 2
in one cluster while printing no 1-2 edge — and 342's own r1 winner prints four edges over three
clusters with all nine shards matched, where a spanning forest needs six. The print cannot tell a join
that was dropped from a join whose record was dropped.

## What this ticket asks

For each state that holds both pieces, print the cluster each piece belongs to, so membership is read
directly instead of inferred from a partial record. Then say, per repeat, which of these holds:

1. **`joined-in-cluster`** — pieces 1 and 2 share a cluster in some state that survives to the winner,
   while the winner prints no 1-2 edge. The join is real and the record dropped it; the 84/94 then
   comes from how the assembly frame is composed across clusters, not from the search. Next ticket: the
   assembly frame.
2. **`never-joined`** — no state that survives to the winner holds them in one cluster. The search
   dropped the join; the record is innocent. Next ticket: the merge and the state score.
3. **`joined-then-lost`** — they share a cluster in an early state that does not survive, and not in
   the winner. The state-selection step is the lever; next ticket: the state score.

## The instrument

One switch, `SFS_CLUSTER_TRACE=1`, exact string, default off, banner `cluster trace: on|off`, GT-free in
`class/`, additions only. For every state visited after a merge and for the final `out_state_[0]`, print
one line per piece of that state:

```
[SFS-343] CLUSTER <step> <state_trace_id> <piece> <cluster_index> <n_pieces_in_cluster>
```

`cluster_index` is the index into the state's `graph_` vector, which is the cluster vector; `n_pieces`
is that cluster's node count. Print `[SFS-343] FINAL <state_trace_id>` once before the winner's block so
the winner can be identified. No sorting, no mutation, no reordering, no threshold, gate, `TOP_k`,
`BRANCH_b` or width change, and no re-print of another ticket's token.

## Part B — the table

Per repeat: the winner id, and for every state printed, the cluster of piece 1 and of piece 2, whether
they match, whether that state survives into `out_state_` (a state survives if its trace id reaches the
final list; say how you checked), and the score of the state. Beside each row, the 342 fact: no 1-2 edge
in the winner's printed list; and the 338 fact: the assembled pair reads 84.40 deg / 93.99 mm.

## What must not change

No behaviour: the four swallows and the accuracy are the control, not the target. Every flagged line is
guarded by the switch; with it off the run executes exactly what it executed before.

## Arms

1. signed 5/3 with `SFS_CLUSTER_TRACE=1` — repeat r1
2. signed 5/3 with `SFS_CLUSTER_TRACE=1` — repeat r2
3. signed 5/3, every switch unset (fidelity) — must read 2/9 sherds and 1/18 edges
4. Pot A, default — must read 8/8 sherds and 15/15 edges, else VOID

Same private 3x tree per arm with md5 before and after, same staging pattern as 342, `sbatch` without
asking, `sacct` State/ExitCode recorded.

## Acceptance

- [ ] `cluster trace: on`/`off` in the respective arms; zero `[SFS-343]` lines with the switch off.
- [ ] Additions only in `class/`; no threshold, gate, `TOP_k`, `BRANCH_b` or ordering change; no GT in `class/`.
- [ ] Part B filled for both trace-on repeats, with the survivor question answered and the slug named.
- [ ] Signed 5/3 reads 2/9 and 1/18 in the trace-on arms and in the fidelity arm; Pot A 8/8 and 15/15.
- [ ] Tree md5 identical before/after per signed arm; `sacct` State/ExitCode recorded.
- [ ] `## Comments (worker, <date>)` appended, ending with exactly one `**End state:**` line.

**Report, do not conclude.**
