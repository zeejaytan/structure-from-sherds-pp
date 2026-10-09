# 05: Axis and rim sweeps after units measured

**What to build:** the two remaining classes swept only once E-0 gives
their units — axis raw knees spanning the measured inlier σ (live
18-unit knee is the permissive extreme); rim `a/w` ≈ live 1.8 / dead
4.0 / paper-deviation 7 mm.

**Answers:** S1

**Blocked by:** 01 (axis inlier unit unknown until measured — the
sources don't give it; rim arms need the baseline distribution too).

**Status:** ready-for-agent

- [ ] Hunks: one scale argument per arm, fixed weights throughout.
      Rim note: the block mixes radius+height (mm each, different
      meaning) under one scale — record which moves if they split.
- [ ] Same-node A/Bs vs 01 tree, one arm at a time: Pot_A full
      assembly, pinned machine, in-job verification per ticket-12
      protocol.
- [ ] Kill criterion stated before running: looser admits
      rim-consistent-but-wrong placements, tighter demands accurate
      radius/height init (`MakeRadiusHeight` medians). Divergence and
      convergence read separately.
- [ ] Verdict recorded in ticket 11 + S1; surviving values carry
      measurement cites, never "the paper's".
