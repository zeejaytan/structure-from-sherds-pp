# 01: Trust-region cap A/B — bound the solver walk

**What to build:** the solver stops leaving the map. One hunk sets the
trust-region bound at every solve site; a same-machine run against the
current tree shows diverged solves falling while sane placements stay
byte-identical.

**Answers:** S1

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

- [ ] Hunk: `initial_trust_region_radius = 100` +
      `max_trust_region_radius = 1000` at all five Solve sites
      (reconstruction.cpp ~:1351, :1591, :1808, :2001, :2199), same
      field+value everywhere incl. dead IcpFine. Scene-scale values,
      mechanism under test is the bound, not the number.
- [ ] Same-node A/B vs current tree (control): Pot_A full assembly,
      pinned machine (bm065 precedent), in-job marker + freshness +
      POT_A verification per ticket-12 protocol.
- [ ] Kill criterion stated before running: diverged-solve count falls
      (guard lines as counter) AND sane attempts byte-identical AND no
      previously-passing pair lost. If diverged stays while sane holds:
      cap insufficient, route to 02 with the numbers. If sane changes
      path: revert, record which pairs moved.
- [ ] Verdict recorded in ticket 11 (forensics owner) + S1 Where-it-stands;
      code comment cites the job pair. No Cauchy/weight/iteration/gate
      changes in this ticket.
