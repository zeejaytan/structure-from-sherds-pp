# 02: Anchor A/B — solve the newcomer with the reconstruction fixed

**What to build:** the paper's Eq. 5 mechanism tested as a method change:
freeze one sherd's pose blocks, solve the other, unfreeze after. A
same-machine run against the 01 tree shows whether anchoring converges
pairs the cap alone cannot.

**Answers:** S1

**Blocked by:** 01 (runs only after the cap is measured — the cap result
decides whether anchoring adds anything, and the control tree is 01's
output).

**Status:** in-progress
**Working in:** solver-02 anchor A/B Basesolve (2026-10-10)

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
