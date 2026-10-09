# 10: Plausibility-gate diagnosis test battery (profile + overlap kill true merges)

**Answers:** S1

**Blocked by:** nothing — T0 instruments, T1–T6 run offline on its dumps

**Status:** resolved 2026-10-10 — battery executed, verdicts routed and
verified in the owning tickets (routing box checked line-by-line, not
assumed):

- 06 carries T1-order + garbage-guard: RESOLVED section (fix 1 measured,
  job 31843205) + DIRECTING NUMBERS block (20/20 flips, TLS skip,
  confounded subset, guard-required) — both present with numbers.
- 07 carries T6 + weights-mapping urgency: T6 rel-rot table + DIRECTING
  NUMBERS block (62–179° off, 150 iters did not prevent, unmapped weights
  prime suspect) — present with numbers.
- assembly-09 carries T5 shape verdict: DIRECTING NUMBERS block
  (35–297mm² true span, false inside, paper's 50 kills top-scored trues,
  SHAPE-not-value) — present with numbers.
- 18 keeps pollution: T4' recorded inconclusive-with-reason here AND the
  ticket stands on eye evidence independently (refuted-as-pollution
  2026-10-02; face traces are true 2-8 seams) — no orphan.

Nothing was fixed here (no matching-code change in this ticket — held
throughout). The battery's job (diagnose + design + direct) is done.

**Needs-eye:** none — gate verdicts recomputed offline, no geometry claim.
The eye re-enters only if rim content changes (ticket 18's rule).

## Diagnosis this battery starts from (measured 2026-10-01, both arms)

- Defaults arm (job 31835065): 8 MERGETABLE edges proposed → plausibility
  FAILs on **overlap** (35.7 / 39.4 / 95.5mm² vs gate 10 — all TRUE merges;
  note the 95.5: above even the paper's 50). Profile never fires (0
  PROFILE DEBUG lines). Death site: overlap gate.
- Paper-values arm (job 31835275): overlap-50 lets merges through → 23
  profile validations: **19 FAILED / 4 PASSED**. Of the 19 kills, **8 are
  all-true configurations** ([1,5]×4, [3,6], [1,3,5]×2, [1,4]) plus correct
  kills of non-true ones — and one false PASS ([7,8], non-mates).
  Death site: profile gate (7/7 max-rule, OLS fit, full-edge input).
- Both gates, at their respective values, reject true merges. The paper's
  COMBINATION (50 + stddev + orthogonal + inner-only) is untested as a
  combination. Beam's best (567) is the least-rejected wrong assembly.

Prime suspects, ranked by body count: (1) max-rule statistic (a single
point >7mm kills — 8 false kills smell like outliers, and ticket 18 put
face-crossing traces in the profile input); (2) OLS fit artifact;
(3) full-edge-vs-inner-only input; (4) overlap threshold placement
(true merges span 35–95mm² — check whether ANY single threshold separates
true from false before tuning it).

## T0 — profile dump mode (the only cluster work; everything else is offline)

Instrument `CheckGraphPlausibility`'s profile path (log-only, zero
behavior change): per validation, dump pieces, per-point xyz + piece +
surface-of-origin (in/out per vote) + breakline segment id, bin edges,
per-point dists under the live rule, verdict, and each piece's current T.
One rebuild, ONE rerun (paper-config arm — it reaches profile; defaults
die at overlap). All of T1–T6 read the dumps on the laptop.

## T1 — statistic (max-rule vs stddev)

Recompute every dumped validation under the paper's rule (std-dev of
orthogonal... of the live dists first, then with T2's refit): do the 8
false kills pass? Kill criterion: if max-rule-vs-stddev flips ≥6 of 8
with no new false passes, the STATISTIC is the cause and ticket 06's fix
order starts there.

## T2 — fit (OLS vs orthogonal)

Refit every bin with total least squares; compare verdicts bin by bin.
Kill criterion: verdicts move only where OLS leverage distorts (steep
segments) — else the fit is cosmetic and ticket 06 skips it.

## T3 — input (full-edge vs inner-only)

Subset dumps to vote-interior points (surface labels from T0); recompute
verdicts. Kill criterion: false kills that vanish on the subset implicate
input (fracture + exterior content), not fit or statistic.

## T4 — pollution (ticket-18 segments)

Drop face-crossing-class segments (segment ids from T0; ticket 18's
attribution decides the class definition) and recompute. Kill criterion:
kills that vanish implicate the append path — fix belongs to 18, and 06
must state post-18-fix bundles for every number.

## T5 — overlap separability (no new run; both logs + dump)

Distribution of plausibility overlap areas for true vs false merges
(baseline log has the true-side numbers 35–95mm²; extract the false side
the same way). Kill criterion: if the distributions overlap substantially,
NO single threshold separates and overlap is the wrong gate shape
entirely — ticket 09 owns that verdict, not a value.

## T6 — ICP poses (right pairs, wrong poses?)

From dumped Ts: per true pair, pairwise transform vs GT (rotation deg +
translation mm). Kill criterion: poses right but merges dead → pure
plausibility cause (current hypothesis); poses wrong → registration
shares the blame and ticket 07/08 scope expands to the ICP path.

## Rules (binding on all six)

- One variable each; T1–T4 share the SAME dumps (same arm, same run).
- Every recompute states bundle arm (pre/post-18-fix) per the ticket-18
  interaction rule.
- A test that flips false kills AND creates false passes reports both —
  trading false negatives for false positives is a finding, not a fix.
- Fix lands in the owning ticket (06 for fit/statistic/input, 18 for
  pollution, 09 for overlap shape); this ticket diagnoses and designs,
  it does not change matching code.

## RESULTS 2026-10-01 (battery executed on holder; 31 dumps)

Replica first: **30/30 recorded verdicts reproduced** (1 dump unprocessable
— see garbage finding below). The dump block is vindicated as a recorder;
offline variants below are trustworthy. Script:
`structure-from-sherds-pp/artifacts/juglet_run1/profile_battery.py`
(kept with the other gate scripts, same convention).

### T1 statistic: CONVICTED

Std-dev rule (paper) on identical bins: ALL 20 recorded FAILED flip to
PASSED, all 11 PASSED stay PASSED. Kill criterion demanded ≥6 of 8 with no
new false passes — delivered 20 of 20 with zero new false passes. The
max-absolute-deviation rule, not the 7mm value, kills true merges. Ticket
06's fix order starts here.

### T2 fit: COSMETIC

TLS (orthogonal) refit + max-rule: nearly everything still FAILED —
including recorded-PASSED rows (stricter, not fairer). The fit does not
decide verdicts. Ticket 06 skips the refit unless new evidence arrives.

### T4' pollution: INCONCLUSIVE (confounded)

Dropping non-~1.9mm-spacing segments flips most failures — but subsetting
trivially reduces max-rule violations (fewer points, fewer chances), so
this cannot convict pollution. T4 as designed is unmeasurable this way;
ticket 18 stands on the eye evidence, not on this test. (T3 folded in:
Breakline_0 IS the vote-interior wall, so "inner-only" == "wall-only" —
recorded, not hidden.)

### T5 overlap: CONVICTED (no separable threshold exists)

Adjacent-MERGETABLE attribution, both logs. Baseline (gate 10): true kills
[1-4]×2 (35.7/score97, 39.4/24), [1-5]×3 (95.5/42, 289/30, 297/6); correct
kills [5-6]×2 (60.5/4, 40.4/52). Paper arm (gate 50): true kills [1-5]×4
(53.03/58 ×2, 57.31/**144**, 53.65/**155**). TRUE merges span 35–297mm²;
FALSE [5-6] sits at 40–60mm², INSIDE the true range. Paper's own 50 kills
true merges carrying the run's highest scores. Mechanism note: overlap
area confounds pose error with pair validity (a true pair at a wrong pose
interpenetrates deeply — 289/297mm² — see T6), so the gate fires on the
registration symptom while appearing to judge pairs. Ticket assembly-09
owns threshold-vs-shape; the numbers say shape.

### T6 poses: CONVICTED (registration shares the blame)

Frame-free relative rotation, dumped T vs GT, all 2-piece dumps: 62–179°
errors on TRUE pairs ([3,6] 62°/90.5°, [4,6] 100°, [6,7] 153.9°,
[1,4] 179.4°...). Right pairs, catastrophically wrong poses. Whether the
error enters at pairwise ICP or graph composition is ticket 07/08
territory (unmapped weights + 150 outer iters that didn't help point at
ICP convergence) — routed there, not solved here. Ticket 09's item 4 is
ANSWERED: both (wrong poses AND killed pairs).

### Garbage-transform robustness bug (new, filed here, not fixed here)

Dump_0030 ([1,3,5], recorded PASSED): piece 1's dumped T carries
t_z = **-1.48e11**, all its points share z=-1.48e11, z-range 1.48e11 →
bin count 2.1e10 → C++ int overflow → validation loop skipped → **vacuous
PASS**. Corrupt configs sail through plausibility. Same family as the
5-8 abort (`length_error` at REGOUT, t_norm=28.4): garbage sizes from
garbage geometry. The T0 block is exonerated (allocates nothing; 30/30
replica); the no-env rerun is DOWNGRADED to optional for exactly that
reason. Any fix must (a) finite-check transforms entering beam states,
(b) make the bin-count overflow impossible — new small ticket or folded
into 06's input work; recorded here so it isn't lost.

## Acceptance criteria

- [x] T0 built (log-only diff reviewed), one rerun, dumps on disk with
      per-point labels verified present (segment ids, rim flags, Ts —
      surface-of-origin NOT captured: T3 folded, see above)
- [x] T1–T6 each report: hypothesis, numbers, kill verdict above
      (T4 inconclusive-with-reason; T5 convicted; rest as marked)
- [x] The single cause ranked: registration poses (T6) + plausibility
      gates firing on their symptom (T1 max-rule convicted, T5 no-threshold
      convicted); fit cosmetic (T2); pollution unmeasured-offline (T4)
- [ ] Fixes filed to owning tickets with the directing numbers
      (06 gets T1-order + garbage-guard; 07 gets T6 + weights-mapping
      urgency; assembly-09 gets T5 shape verdict; 18 keeps pollution) —
      NEXT
- [ ] Nothing fixed here (this ticket changes no matching code) — HELD

- [ ] T0 built (log-only diff reviewed), one rerun, dumps on disk with
      per-point labels verified present (not assumed)
- [ ] T1–T6 each report: hypothesis, numbers, kill verdict (cause /
      cosmetic / inconclusive with the stated reason)
- [ ] The single cause (or ranked causes) named with body counts, as above
- [ ] Fixes filed to owning tickets with the directing numbers; nothing
      fixed here
