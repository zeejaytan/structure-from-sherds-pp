# 01: Trust-region cap A/B — bound the solver walk

**What to build:** the solver stops leaving the map. One hunk sets the
trust-region bound at every solve site; a same-machine run against the
current tree shows diverged solves falling while sane placements stay
byte-identical.

**Answers:** S1

**Blocked by:** None (can start immediately).

**Status:** resolved 2026-10-10 — cap HOLDS. Same-node A/B vs row-3
tree (jobs 32613911 vs 32624499, bm065): diverged 140 → **0**, SURVIVED
29 identical with identical per-pair values, GT report 14 both, 2/8 +
1/15 both with the same passing pair (1-4 both directions: 7.8°/20–21mm
vs 8.3°/21–27mm). Best 262 vs 264, states 11 both, profile 208/200
passed zero kills both, rejections score 0/axis 68–69/overlap 56–57 —
fringe only. Kill criterion met on all three legs: garbage gone, sane
identical, no pair lost. The walk is bounded; convergence itself (poses
still 62–179° off elsewhere) is untouched — say so, don't oversell.
Route: ticket 02 (anchor) is now unblocked.

- [x] Hunk: `initial_trust_region_radius = 100` +
      `max_trust_region_radius = 1000` at all five Solve sites
      (reconstruction.cpp ~:1351, :1591, :1808, :2001, :2199), same
      field+value everywhere incl. dead IcpFine. Scene-scale values,
      mechanism under test is the bound, not the number.
- [x] Same-node A/B vs current tree (control): Pot_A full assembly,
      pinned machine (bm065 precedent), in-job marker + freshness +
      POT_A verification per ticket-12 protocol.
- [x] Kill criterion stated before running: diverged-solve count falls
      (guard lines as counter) AND sane attempts byte-identical AND no
      previously-passing pair lost. If diverged stays while sane holds:
      cap insufficient, route to 02 with the numbers. If sane changes
      path: revert, record which pairs moved.
- [x] Verdict recorded in ticket 11 (forensics owner) + S1 Where-it-stands;
      code comment cites the job pair. No Cauchy/weight/iteration/gate
      changes in this ticket.
