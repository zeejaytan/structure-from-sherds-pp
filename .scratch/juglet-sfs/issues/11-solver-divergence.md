# 11: Solver divergence — translations escape to 1e11–1e14 mm

## ANCHOR VERDICT 2026-10-10 (solver-02, jobs 32663002 vs 32624499, bm065): FAILED, reverted

Anchor (freeze shard_y s/trans around Icp Solve only,
unfreeze after; code fix/mov convention) vs 01 tree:
diverged 0 both arms; SURVIVED list identical; accuracy
2/8 + 1/15 BACKWARDS to 0/8 + 0/15 — the same pair 1-4
lost, no new pair seated, 1-2 never merge-attempted in
either arm. Kill criterion's "reshuffle without growth"
branch: anchoring changes the search without converging
it. Hunk reverted; cap stays. Convergence candidates
exhausted in this series: cap (bounds, no fix) and anchor
(changes search, no gain). Standing unowned route: Cauchy
scales (ticket 01's E-0 gate) — the 8x scale spread with
no paper values anywhere is the next measured suspect, and
the forensics below (uniform pre-conditions) leave solver
setup or input quality as the only venues; anchor was the
last cheap solver-setup candidate.

## CAP VERDICT 2026-10-10 (solver-01, jobs 32613911 vs 32624499, bm065): BOUNDED, convergence open

Trust-region cap (initial=100, max=1000, all 5 Solve sites) vs row-3
tree: diverged 140 → **0**; SURVIVED 29 identical, GT 14 both, 2/8 +
1/15 both, same pair 1-4 both ways. The unbounded-step candidate (c) is
CONFIRMED as the divergence mechanism and CLOSED as a fix: the walk is
bounded at zero sane cost. What remains is convergence itself — poses on
non-passing pairs still 62–179° off, 1-2 still never merge-attempted.
The guard stays (defense in depth; fires 0× now, costs nothing). Next:
ticket 02 (anchor per Eq. 5) is the live convergence candidate; Cauchy
scales remain unowned. Cap hunk stays in the tree with the job-pair cite.

## FORENSICS TABLE 2026-10-10 (27 e2e logs, all on disk — uniformity verdict)

Mined every `e2e_pota_fresh_*.log` + `e2e_pota_row2_*.log` for divergence
onsets (script pattern kept in the ticket body: ICPCOR iter=0 per attempt
as pre-condition, DIVERGED-SOLVE vs REGOUT as outcome):

- 26,312 attempts with iter-0 lines; 6,586 diverged solves (25% — every
  fourth solve leaves the map; the guard's 550-per-run is the same rate).
- Diverged pre-conditions: pts min 1 / max 48 / **median 2**; meandist
  9.7–195.1mm / **median 106.3mm**.
- ALL-attempt control: pts min 1 / max 52 / **median 2.0**; meandist
  2.3–199.4mm / **median 99.3mm**. Same distribution to the digit.
- Per-pair: diverged and sane leaderboards agree (2-3, 2-5, 3-5, 2-4 top
  both) — divergence strikes in proportion to attempts, not to pairs.

**Verdict: UNIFORMITY.** Diverged and sane attempts enter with
indistinguishable inputs (median 2 pts at ~100mm either way) — no
pre-condition table routes this to input quality. Per the ticket's own
rule, the fix venue is **solver setup (anchor/prior/cap)**, not 18 or
Cauchy-scales. The attempt-floor candidate is NOT supported by this data
(a floor at N would refuse sane and diverged attempts alike — both enter
at median 2 pts). Standing candidates in order: (c) unbounded LM steps —
verify the trust-region option exists in the vendored Ceres headers;
(b) unanchored 12-DOF gauge drift; (a) angle-axis singularities last.
Guard stays (bounds damage at zero sane cost); convergence untouched.

**Answers:** S1

**Blocked by:** nothing — log forensics first (all evidence on disk),
then one fix

**Status:** ready-for-agent

**Needs-eye:** none — solver traces and gate counts, no geometry claim.

## Mechanism (measured, four crime scenes, one class)

- Pair 1-2 REGOUT (job 31898200): t_norm 1.7e13 / 42.6 / 8.2e12 / 8.4e12
  across attempts — same pair, same data, solver lands sane once.
- ICPITER trace (job 31898200, pair 2-5): `acc_t_norm` 128.879 → 
  308346353332796.2 (3e14) between outer iters 0 and 1. One LM run
  detonates; everything after works in fantasy land.
- Dump_0030: piece T with t_z=-1.48e11 enters beam state, plausibility
  vacuous-PASSES on int overflow (separate guard, ticket 06 done).
- 5-8 abort run: `length_error` at REGOUT t_norm=28.4 (thousand-mm scale
  on a 300mm pot) — garbage sizes from garbage geometry.

Candidate root causes (NOT decided — forensics decides): (a) unanchored
free-free 12-DOF solve with null-space drift (both shards' R,t free, no
prior); (b) bad correspondence input yanking LM out of basin (pollution,
outliers — Cauchy should blunt this, evidently doesn't always);
(c) unbounded LM steps (no trust-region cap visible in options);
(d) angle-axis singularities. T6's 62–179° pose errors are the OUTPUT of
this mechanism; weights E1–E3 didn't touch it (emphasis changes nothing
when the solver leaves the map).

## FORENSICS 2026-10-02 (existing logs, no new run)

- Scale: 28–890 insane translations (>2000mm on a ~300mm vessel) PER RUN,
  all 12 e2e logs. 3–5% of REGOUT validations. The solver routinely leaves
  the map — not an edge case.
- Pre-condition (pair 1-2): iter-0 sets of **2–6 points at 54–127mm**
  meandist entering a 12-DOF free-free solve. Underdetermined input →
  unbounded output. Mechanism needs no mystery: 12 unknowns fit to 2
  noisy points 100mm apart can put the answer anywhere, and LM obliges.
- Pipeline link: `miss_cor` fires only below MINIMUM_NUMBER=1 (lineage,
  ticket 12 row 6 — upstream had 6). A 2-point set PASSES the entry gate
  into the 12-parameter solve. The floor meant to prevent exactly this
  was lowered to meaninglessness by the same lineage that loosened
  everything else.

Mine all e2e logs (`sfs_main/e2e_pota_fresh_*.log`) for divergence onsets:
per diverging attempt, the iter-0 meandist/inlier count/corPts
(ICPCOR lines) and the iter where acc_t_norm first exceeds vessel scale
(ICPITER lines). Question: do divergences share a pre-condition (few
inliers? large initial meandist? specific pairs?) or strike uniformly?
If uniform → solver setup (anchor/prior/cap). If preconditioned →
input quality (route to 18/Cauchy-scales with the pre-condition table).

## GUARD OUTCOME 2026-10-03 (job 32109955): bounds damage, no recovery

0/8 + 0/15, 19 states, best 335, profile 73/0, **550 DIVERGED-SOLVE
fires**, zero aborts/length_errors. Verdict as predicted: garbage
rejected loudly 550× (mechanism scale confirmed — every ~sixth solve
leaves the map), no fantasy merges from those branches, no abort. No
recovery (convergence untouched — 1-2 still never merges). Guard STAYS
(it prevents the abort + vacuous-pass family at zero sane cost).
Convergence itself (anchor/prior/input/Cauchy) remains open work, in
that order per the forensics.

GUARD FIRST 2026-10-02 (chosen — see forensics): same 8-line block in
`Icp` + both `Registration` overloads (one variable, three call sites):
reject translations >20000mm loudly via existing failure channels
(score 11 / inlier 0). Sane placements live ≤427mm (GT max); garbage at
1e11+. Only insane solves change path; sane solves byte-identical
behavior. This bounds damage (no fantasy merges, no overflow passes, no
length_error aborts) but does NOT fix convergence — say so in the
verdict, don't oversell.
RUN LOGISTICS 2026-10-02 (accidental A/B, kept): job 32107435 runs the
PRE-guard 2/8-config tree (stale-binary submit: the guard push hadn't
propagated when the build pulled — push-then-pull race, same class as
before; verified by mtime discipline AFTER submit, too late). Kept
running deliberately: it reproduces the 2/8 configuration on a fresh
rebuild (2/8 stability data point, free). The guard run follows on the
guard binary; compare the two for guard effect (garbage-T counts,
abort absence) separately from accuracy.

ATTEMPT FLOOR second (if guard's loud lines show a clean pre-condition
worth refusing earlier).

ATTEMPT FLOOR (added 2026-10-02 from forensics — first candidate by
mechanism fit): refuse ICP attempts with <N correspondences or initial
meandist > X (values to set from the onset table, not guessed; upstream's
6 is the null hypothesis for N). Directly attacks the measured
pre-condition (2–6 pts at 54–127mm). Note the interaction: MINIMUM_NUMBER
currently gates BOTH LCS length (ticket 12 row 6) and ICP entry
(`miss_cor`) — the experiment sets the ICP-entry floor, LCS length stays
a separate question for 12.

1. **Guard (cheapest):** after each solve, reject candidate Ts with
   ||t|| beyond vessel scale (loud, at pair level) — converts garbage
   poses into missing candidates instead of fantasy merges. Does not fix
   convergence; bounds damage.
2. **Anchor:** fix shard A, solve B only (halves DOF, kills gauge drift).
   Changes the method (paper solves both, Eq 4) — needs the forensics +
   A/B to justify, never by appeal.
3. **Prior/step cap:** translation prior or trust-region radius cap.
   Verify the Ceres option exists in the vendored headers before
   promising it (no `max_trust_region_radius` assumption without reading
   `ceres-solver` headers in the sif).
4. NOT here: weights (exhausted, ticket 07), iterations (measured,
   ticket 07), correspondences input (18), Cauchy scales (unowned —
   EVIDENCE 2026-10-02: scales vary by path — pairwise 4.0/1.8, graph-ICP
   1.0/0.5 env-tunable, axis/rim 1.8/1.0. An 8x spread with no paper
   values anywhere. Whoever takes Cauchy work starts here).

## Acceptance criteria

- [ ] Forensics table: divergence onsets with pre-conditions (or
      uniformity verdict with the numbers)
- [ ] ONE fix implemented per the forensics; A/B per pair (Pot_A +
      Juglet honest + authors' sample); no-regression rule
- [ ] Post-fix logs show zero garbage-T candidates (finite, vessel-scale)
      OR the fix reverted with the numbers that killed it
- [ ] 1-2 converges or fails loudly at a NAMED stage (no more silent
      table-miss after fantasy solves)
