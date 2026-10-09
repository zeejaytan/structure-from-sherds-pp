# 02: Graduated scale schedule (large to small)

**What to build:** the one scale move Ceres affirmatively recommends —
start with large bowls (gradients alive on far starts), optimize, then
reduce for final precision. Same-machine run against the 01 tree shows
whether premature downweighting is the divergence mechanism.

**Answers:** S1

**Blocked by:** 01 (needs the measured residual distribution to set the
large/small endpoints — not guessed).

**Status:** ready-for-agent

- [ ] Hunk: `LossFunctionWrapper` graduated schedule (mutate-after-build,
      not a rebuild per phase), endpoints from 01's numbers. Weights,
      iterations, gates untouched.
- [ ] Same-node A/B vs 01 tree: Pot_A full assembly, pinned machine,
      in-job verification per ticket-12 protocol.
- [ ] Kill criterion stated before running: diverged-solve count falls
      AND the 1–4 pair still passes AND no previously-passing pair lost.
      If divergence persists through the large phase: premature
      downweighting excluded as sole mechanism — useful negative,
      route to 03 with the numbers. If sane changes path: revert,
      record which pairs moved.
- [ ] Verdict recorded in ticket 11 + S1; code decision cites the job
      pair. No weight/iteration/gate changes in this ticket.
