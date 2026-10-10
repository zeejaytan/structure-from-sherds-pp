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
