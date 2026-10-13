# 02: Anchor A/B — solve the newcomer with the reconstruction fixed

**What to build:** the paper's Eq. 5 mechanism tested as a method change:
freeze one sherd's pose blocks, solve the other, unfreeze after. A
same-machine run against the 01 tree shows whether anchoring converges
pairs the cap alone cannot.

**Answers:** S1

**Blocked by:** 01 (runs only after the cap is measured — the cap result
decides whether anchoring adds anything, and the control tree is 01's
output).

**Status:** resolved

**Answer 2026-10-10 (A/B jobs 32663002 vs 32624499, bm065): FAIL —
reverted.** Anchor (freeze shard_y s/trans around Icp Solve, unfreeze
after) vs 01 tree: diverged 0 both; SURVIVED identical; accuracy
went 2/8 + 1/15 **backwards** to 0/8 + 0/15 — the same pair 1-4
did not survive, let alone grow, and 1-2 was never merge-attempted
in either arm. By the ticket's own kill criterion this is
"reshuffle without growth": anchoring changes the search without
converging it. Hunk reverted (00504f4 reverted; tree back to 5cd22a2
state for reconstruction.cpp); cap stays. The Eq. 5 mechanism as
implemented (freeze one block set, solve the rest) does not attack
the measured defect — the forensics (ticket 11: uniform
pre-conditions, median 2 pts at ~100mm either way) plus this A/B
say the missing ingredient is elsewhere. Cauchy scales remain the
unowned route (ticket 01's E-0 gate); Eq. 6 free-all global step
stays future work.

- [ ] Hunk: `SetParameterBlockConstant` on the fixed sherd's `s`/`trans`
      before Solve, `SetParameterBlockVariable` after, at the pairwise
      sites. Which sherd is fixed follows the code's own fixed/moving
      convention (shard_y fix / shard_x mov). No new machinery.
- [ ] Same-node A/B vs 01 tree: Pot_A full assembly, pinned machine,
      in-job verification per ticket-12 protocol.
- [ ] Kill criterion stated before running: 1-2 survives to a merge
      attempt AND/OR accuracy grows directionally (same pair + new
      ones). If accuracy reshuffles without growing: anchoring changes
      the search without converging it — record, revert, route the
      remainder (Cauchy scales) explicitly rather than drifting there.
- [ ] Verdict recorded in ticket 11 + S1; method-change justification
      cites the A/B, never the paper alone. Paper's Eq. 6 (free-all
      global step) is NOT this ticket — separate future work if 02 wins.

## Eq.6 design (DESIGN ONLY 2026-10-13 — DO NOT RUN until row-5→row-7→09 land)

Expensive run deliberately last, after the cheap measurement chain
row-5 → row-7 → 09. No code edited, no sbatch submitted in this ticket.

Mechanism (anchor vs free-all): ticket 02 tested the paper's Eq. 5 —
freeze one block set (`s`/`trans` of the fixed sherd, code convention
shard_y fix / shard_x mov) around the pairwise `Icp` Solve, solve the
rest, unfreeze after. Eq. 6 is the paper's global step (`paper:
sfspp-2502.13986v1.md:300-302`, Eq. 6 frees ALL of
Φ = Φ_old ∪ Φ_new, bundle-adjustment-like joint refinement), i.e. the
opposite DOF move: Eq. 5 removes gauge freedom to stabilise one
newcomer attach; Eq. 6 restores full gauge freedom so early-seated
pairs can move again under multi-pair constraints. Concretely: Eq. 5 =
`SetParameterBlockConstant` on old/fixed blocks at solve time; Eq. 6 =
all `s[i]`/`trans[i]` blocks VARIABLE at the global solve (no Constant
on any sherd block; `R_rim`/`H_rim` stay Constant as now — they are
median-derived scalars, not poses).

Why it might attack the forensics where anchoring didn't: ticket-11
forensics show UNIFORM pairwise pre-conditions (median 2 pts at
~100 mm either way) — the pairwise 12-DOF free-free solve is
underdetermined at entry, and 02 proved halving DOF at that same thin
correspondence only changes the search without converging it (2/8+1/15
→ 0/8+0/15, reverted 00504f4→5cd22a2). Eq. 6 does not re-attack that
entry defect; it attacks the downstream defect — accumulated drift
across seated pairs that no pairwise step (anchored or not) ever
revisits. If the blocker is pairwise starvation, Eq. 6 fails the same
way; if the blocker is cross-pair inconsistency after seating, only a
joint all-free step can relax it. Cauchy E-0 gate stays the competing
unowned route (tolerance, not geometry).

Planned hunk sketch (sites only, NO edit): the Eq. 6 venue is the
global solve in `class/reconstruction.cpp` `IcpIncGraphAxis`
(~:2186-2212) — specifically the pregraph-freeze loop (~:2186-2193,
`SetParameterBlockConstant(s[i])` / `(trans[i])` for
`pregraph.back().node_[i]`) is SKIPPED / replaced with explicit
`SetParameterBlockVariable` on all sherd blocks before
`ceres::Solve` (~:2212). Pairwise sites (`Icp` ~:1541, `Registration5`
~:1787, `Registration6` ~:2010, dead `IcpFine` ~:2412) UNTOUCHED —
cap (100/1000) + guard stay as-is at all five sites. One variable:
the freeze vs free-all at the global step only.

Kill criterion (stated BEFORE running): DIRECTIONAL growth — the same
pair that passes in the control (currently 1-4) still survives to a
merge attempt AND at least one new pair seats (accuracy grows, not
reshuffles). Reshuffle-without-growth (score moves, pairs change,
accuracy ≤ control) = record, revert, route explicitly (scorer
preference vs Cauchy E-0 gate) — never tune in place.

Deferred-run plan (NOT executed): same-node bm065 Pot_A full assembly,
Eq. 6 arm vs 01 tree (cap-only control, job 32624499 lineage); in-job
verification per ticket-12 protocol (marker + freshness + POT_A
checks); read diverged-solve count, per-pair accuracy/states/best
score, and whether the same pair passes, vs control. GUARD:
DO NOT RUN until row-5 → row-7 → 09 land.

## Eq.6 run (EXECUTED 2026-10-14 — notes only, Status/Answers untouched)

Guard lifted (rows 5, 7, 09 landed at HEAD 6a6fb8d). Same-node bm065
Pot_A full-assembly A/B, control vs free-all; one variable (freeze vs
free-all at the global step only). Arms: `hpc/sbatch/run_eq6_freeall_{control,eq6}.sbatch`
(new, uncommitted); `class/reconstruction.cpp` read + transient in-job
hunk only, restored post-run, no landed edits.

Control (job 32905462, COMPLETED 12:29, bm065): freeze as-is, no patch.
Eq.6 (job 32905987, COMPLETED 17:35, bm065): pregraph-freeze loop
(~:2186-2193) replaced with explicit `SetParameterBlockVariable` on all
sherd blocks present in the problem before `ceres::Solve` (~:2212);
R_rim/H_rim Constant untouched. First Eq.6 attempt (job 32905463)
aborted at the first global solve (Ceres CHECK "Parameter block not
found" → exit 134): unguarded `SetParameterBlockVariable` on sherd
blocks with no residuals in that iteration, which own no block in the
problem. Mechanical harness bug, not a method result; re-ran with a
`HasParameterBlock` presence guard (blocks absent from the problem are
already unconstrained). Exact transient hunk (9-line freeze block →
11-line free-all block) and both sbatch files retained uncommitted in
`hpc/sbatch/` for lead inspection.

Outcome (control vs Eq.6 — identical):

| read | control (32905462) | Eq.6 (32905987) |
|---|---|---|
| Sherd accuracy | 0/8 | 0/8 |
| Edge accuracy | 0/15 | 0/15 |
| Best assembly | State #0, 7/8 pcs, 260.000 | State #0, 7/8 pcs, 260.000 |
| diverged solves | 0 | 0 |
| SURVIVED (feature-match) | 29 | 29 |
| pair 1-4 to merge attempt | yes (6x MERGETABLE 1-4) | yes (6x, identical) |
| new pairs seated | 0 | 0 |

Merge-attempt pair histogram identical both arms (1-8:33, 5-7:26,
4-6:13, 3-6:13, 1-5:12, 5-8:6, 1-4:6, 5-6:5, 6-7:2, 7-8:1). Log diff
ignoring timing lines: only `Analysis time` values differ — the runs
are numerically identical. Free-all changes nothing measurable: with
no pair ever seating, the joint step has nothing seated to relax
(pairwise starvation, not cross-pair drift, binds — as the design note
predicted as the failure mode).

Kill-criterion reading (for lead review — no verdict edit made here):
pair 1-4 survives to merge in both arms BUT no new pair seats, so the
GROWTH bar (same-pair survival AND ≥1 new seated pair) is not met. This
is not even reshuffle-without-growth (scores/pairs do not move at
all) — it is a strict no-op. Provisional route: Eq.6 exhausted on
Pot_A the same way Eq.5 was (anchor regressed, free-all is inert);
remaining unowned route stays the Cauchy E-0 gate.

Lead verdict 2026-10-14: Eq.6 EXHAUSTED on Pot_A — strict no-op, not
even reshuffle (control reproduces the row-7 OFF baseline exactly:
0/8+0/15, 260.000, SURVIVED 29, diverged 0, so the measurement chain is
consistent). Solver-DOF moves are done on this pot (Eq.5 regressed,
Eq.6 inert at ~40% extra wall time: 17:35 vs 12:29 for identical
output). Route explicitly to the Cauchy E-0 gate; ticket stays
resolved.

Hygiene: both binaries separate (`build_sif/Hierarchy-Clear-eq6{ctl,free}`,
shared untouched); flock-serialised rebuilds; freshness + POT_A +
marker checks passed in-job; Spartan checkout left clean (transient
revert restored, HEAD 6a6fb8d); local `reconstruction.cpp` untouched.
Juglet impact unmeasured (Pot_A only).
