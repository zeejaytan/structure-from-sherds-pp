# 06: Loss-shape swap as diagnostic (Huber vs Cauchy vs Tukey)

**What to build:** the shape question answered once, at fixed scales —
swap the kernel family and check the sources' predicted ranking, as a
diagnostic of whether divergence is kernel-driven at all. Never a fix
in this ticket.

**Answers:** S1

**Blocked by:** 01 (needs the baseline; runs last — diagnostic, not
treatment).

**Status:** ready-for-agent

- [ ] Hunks: kernel swap only, scales fixed at live values, weights
      frozen throughout.
- [ ] Same-node A/B vs 01 tree: Pot_A full assembly, pinned machine,
      in-job verification per ticket-12 protocol.
- [ ] Kill criterion stated before running: Huber should show less
      far-init divergence but more false-positive convergence than
      Cauchy at the same `a`; Tukey the reverse. If the ranking flips,
      divergence is not robust-kernel-driven — record, route back to
      solver-setup with the numbers. Diagnostic only: no shape is
      adopted here whatever wins.
- [ ] Verdict recorded in ticket 11 + S1.
