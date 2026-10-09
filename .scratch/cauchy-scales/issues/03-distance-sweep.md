# 03: Distance-scale sweep at fixed weights

**What to build:** the raw distance knee moved deliberately — arms with
`a/w` ≈ 1/2/4/8 mm (incl. a uniform ×2/÷2 arm), weights frozen,
measuring whether the knee brackets the operating point.

**Answers:** S1

**Blocked by:** 01 (endpoints from measured inlier σ, not guesses).

**Status:** ready-for-agent

- [ ] Hunks: one scale argument per arm at the live distance sites
      (fixed weights throughout — the direction E1–E3 never explored,
      isolated). The 8 mm arm tests the knee-below-init-error overlap
      directly.
- [ ] Same-node A/Bs vs 01 tree, one arm at a time: Pot_A full
      assembly, pinned machine, in-job verification per ticket-12
      protocol.
- [ ] Kill criterion stated before running: if small `a` raises
      divergence AND large `a` raises false-match convergence, the knee
      brackets the operating point and 02 is the principled answer; if
      only one side moves, follow it. Divergence rate and convergence
      quality read separately.
- [ ] Verdict recorded in ticket 11 + S1; surviving value carries the
      measurement cite, never "the paper's" (no such value exists).
