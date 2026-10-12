# 11: Solver divergence — translations escape to 1e11–1e14 mm

## CAUCHY E-6 VERDICT 2026-10-12 (job 32756813, bm065): Huber matches Cauchy's failure — not kernel-driven

Huber kernel at solver-01 root scales (all four literals restored 4.0/1.8/1.8/1.8 AND `CauchyLoss` → `HuberLoss` at the three hardcoded pairwise sites, commit `34cf112`) vs solver-01 control (job 32624499): scaffold health passed (dump `a`-column shows restored values at all sites, 38,934 files; graph env untouched), but accuracy went 2/8 + 1/15 BACKWARDS to 0/8 + 0/15 while best score sat at 239.000 — below control 262. Pair 1-4 still SURVIVED (value 21.0). Divergence stayed 0 — Huber showed no far-init divergence where Cauchy held 0, and no false-positive-convergence amplification either (score fell with accuracy, the E-4 starvation pattern, not the predicted amplification). SURVIVED 14 / FEATURE_MATCHING 1 unchanged. Nothing adopted per the ticket's rule. Full numbers in cauchy-scales ticket 06 evidence. Route: ranking did NOT flip — the same seating failure under a different kernel, so divergence/seating is NOT robust-kernel-driven. Tolerance line CLOSED (all four classes CLOSED or cited; Arm P still deferred, not skipped). Standing routes: solver-setup (trust-region/init/correspondence gate — the anchor verdict's unowned route) and the scorer's wrong-answer preference (cited from 03 step 0, amplified nowhere). Tukey does NOT run — its gate was Huber ranking surprising, and it wasn't.

## CAUCHY E-5 VERDICT 2026-10-12 (job 32753136, bm065): tight axis holds — Arm P (permissive) EARNED

Tight axis tolerance (`a_axis = 0.5`, three hardcoded pairwise sites, commit `e08e764`, everything else frozen at the E-4 tree: dist 119.0, norm 0.5, rim 1.8, `w_a = 0.1`) vs E-4 control (job 32743207): primary read landed on the ticket's arithmetic prediction (axis `s/a²` p50 27.4/47.6/17.9 at Icp/Reg5/Reg6, pull-weight `ρ′` p50 0.035/0.021/0.053 at iter 1, `a = 0.5` confirmed in dump — was 2.45 / 0.29 at `a = 1.8`), axis fully suppressed at entry as designed, but accuracy HELD at 0/8 + 0/15 (E-4 baseline) while best score ROSE 239.000 → 258.000. Pair 1-4 still SURVIVED (value 21.0). Divergence stayed 0. SURVIVED 14 / FEATURE_MATCHING 1 unchanged. Rim rows at live 1.8 confirm the citation basis (123,114 + 140,834 rows). Not tuned in place per the ticket's rule. Full numbers in cauchy-scales ticket 05 evidence. Route: the HOLDS branch fired — suppressing axis changed the score but not the seating, so the permissive follow-up (Arm P, `a_axis = 2.8`, raw knee ≈ inlier median) is EARNED and runs next; it is the last tolerance arm before 06's diagnostic. Scorer-prefers-wrong cited from 03 step 0, not re-run.

## CAUCHY E-4 VERDICT 2026-10-12 (job 32743207, bm065): tight normal strangles true pairs — normals direction CLOSED both ends

Tight normal tolerance (`a_norm = 0.5`, the graph path's own normal default, three hardcoded pairwise sites, commit `cd26465`) vs solver-01 control: primary read landed on the ticket's arithmetic prediction (norm `s/a²` p50 53.0/70.3/68.7 at Icp/Reg5/Reg6, pull-weight `ρ′` p50 0.019/0.014/0.014 at iter 0, `a = 0.5` confirmed in dump — was 4.1–5.4 / 0.18 at `a = 1.8`), normals ~fully suppressed at entry as designed, but accuracy went 2/8 + 1/15 BACKWARDS to 0/8 + 0/15 while best score sat at 239.000 — below control 262 AND below E-2/E-3's 266. Pair 1-4 still SURVIVED. Divergence stayed 0. SURVIVED 14 / FEATURE_MATCHING 1 unchanged. Not tuned in place per the ticket's rule. Full numbers in cauchy-scales ticket 04 evidence. Route: the tight arm strangled true pairs along with false ones (median entry chord ~0.25–0.61 sits just above the tight raw knee 0.167 — true pairs kept only partial weight), so the FALLS branch fired and the permissive follow-up (Arm P) does NOT run — its gate was holding-or-growing. Normals direction CLOSED at both ends: live 1.8 keeps 2/8 (E-0 record), tight 0.5 falls to 0/8. Tickets 05 (axis/rim) / 06 (shape diagnostic) next; scorer-prefers-wrong already answered YES in 03 step 0, cited not re-run.

## CAUCHY E-3 VERDICT 2026-10-12 (job 32729140, bm065): half-online, joins lost — distance CLOSED

Fixed distance tolerance at measured p50 (`a_dist = 119.0`, three hardcoded sites, commit `2098f55`) vs solver-01 control: primary read landed on the ticket's arithmetic prediction (dist `s/a²` p50 1.005/0.859/0.856 at Icp/Reg5/Reg6, pull-weight `ρ′` p50 0.50/0.54/0.54 at iter 0, `a = 119` confirmed in dump — was 780 / 0.0013 at `a = 4.0`), term exactly half-online as designed, but accuracy went 2/8 + 1/15 BACKWARDS to 0/8 + 0/15 while best score sat at 266.000 — identical to E-2's 266, up from control 262. Third reading of the false-matches-outscore-true-pairs signature, now with the term half-online rather than fully online. Divergence stayed 0. SURVIVED 14 / FEATURE_MATCHING 1 unchanged, pair 1-4 still SURVIVED. Not tuned in place per the ticket's rule. Full numbers in cauchy-scales ticket 03 verdict. Route: distance direction CLOSED — tickets 04 (normals) / 05 (axis) next; scorer-prefers-wrong finding routes to scorer owner alongside (step-0 read: 266 at 0/8 outscores 262 at 2/8, pairwise identical, split downstream in merge/graph scoring).

## CAUCHY E-2 VERDICT 2026-10-12 (job 32722789, bm065): term online, joins lost

Graduated distance schedule (260/181/119/4.0 at iters 0/1-9/10-49/50+,
three hardcoded sites, commit `fd3e42f`) vs solver-01 control: primary
read landed on prediction (dist `s/a²` p50 0.18-0.21, `ρ′` p50 0.83-0.85
at iter 0, `a = 260` confirmed in the dump — was 780 / 0.0013 at
`a = 4.0`), but accuracy went 2/8 + 1/15 BACKWARDS to 0/8 + 0/15 while
best score ROSE 262 to 266: false matches admitted, the "loose" half of
the tight-vs-loose trade. Divergence stayed 0 (not instability).
SURVIVED 14 / FEATURE_MATCHING 1 unchanged, pair 1-4 still SURVIVED.
Schedule NOT tuned in place per the ticket's rule; iter boundaries stay
free parameters. Full numbers in cauchy-scales ticket 02 verdict. Route:
ticket 03 asks whether the distance term can come online anywhere
without admitting false matches, or the 2/8 must come from another
class. Convergence candidates exhausted in this series: cap (bounds, no
fix), anchor (changes search, no gain), graduated distance (online, joins
lost).

## MAKEMULTI ITER TAG 2026-10-10 (uncommitted, pre-01): graph path now labeled

`MakeMultiCorres` gained an `int iter` parameter (signature
`reconstruction.h:110-116`, 3 call sites threaded: `IcpIncGraphAxis`
main `:1947`, merge-table `:2043`, dead `IcpFine` `:2249`) and the
existing `*** MAKEMULTI ***` line now prints `iter=`
(`reconstruction.cpp:663`). Why: the graph path (live via
`ranking_system.cpp:1041` → `IcpIncGraphAxis`) had no iteration
label on its per-pair log — `Icp` has `ICPCOR iter=` at `:1308`,
graph had nothing. Without `iter`, the per-pair rows from up to 200
outer iterations are indistinguishable, so a graph-path onset table
(like the forensics below, but for the graph path) cannot be built.
Log-only, zero behavior change; the `meandist` extension proposed
alongside was dropped as scope creep (one variable: the label).
This unblocks the same forensics pattern for graph-path runs.

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
