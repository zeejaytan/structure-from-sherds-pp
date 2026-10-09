# 01: Log per-class residual norms and pull-weights at iteration 0

**What to build:** the measurement every sweep needs — per residual
class (distance, normal, axis, rim), the actual norm distribution and
pull-weight at iteration 0 on live Pot_A inputs, especially the ~100 mm
starts and 2-point sets. Zero behavior change.

**Answers:** S1

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

- [ ] Hunk: log-only block per solve site — per residual class, `‖f‖`,
      `s/a²`, `ρ′` at iteration 0 (post-weight units + raw geometric
      conversion). No scale, weight, iteration, or gate change; default
      path provably identical (control run before any sweep).
- [ ] One run, same-node, Pot_A full assembly; dumps on disk with
      per-point labels verified present (not assumed).
- [ ] Report: knee-vs-reality per class (e.g. true-pair distance blocks
      at `s/a² ≈ 600` with `ρ′ ≈ 0.002`?), Cao-error inlier unit
      established (the unit the sources don't give), inlier σ per class
      for the 2–3σ scale anchors.
- [ ] Verdict recorded in ticket 11 (forensics owner) + S1
      Where-it-stands; nothing tuned here — measurement only.
