# 04: Normal-scale sweep in angular terms

**What to build:** the raw normal knee moved in degrees — arms at ≈
15°/30°/60° (i.e. `a_norm = w_n × raw`), weights frozen. 30° anchored
to the paper's verification angle (interpretation, stated as such).

**Answers:** S1

**Blocked by:** 01 (baseline residual distribution; 03 informs but does
not gate — different class, may run in either order after 01).

**Status:** ready-for-agent

- [ ] Hunks: one scale argument per arm at the live normal sites,
      fixed weights throughout.
- [ ] Same-node A/Bs vs 01 tree, one arm at a time: Pot_A full
      assembly, pinned machine, in-job verification per ticket-12
      protocol.
- [ ] Kill criterion stated before running: tighter kills rotated false
      matches but risks true pairs under noisy normals; looser does the
      reverse. Predicted weights at 29° error computed before running
      (research §4.2); divergence rate and convergence quality read
      separately.
- [ ] Verdict recorded in ticket 11 + S1; surviving value carries the
      measurement cite, never "the paper's".
