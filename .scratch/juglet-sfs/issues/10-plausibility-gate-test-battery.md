# 10: Plausibility-gate diagnosis test battery (profile + overlap kill true merges)

**Answers:** S1

**Blocked by:** nothing — T0 instruments, T1–T6 run offline on its dumps

**Status:** ready-for-agent

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

## Acceptance criteria

- [ ] T0 built (log-only diff reviewed), one rerun, dumps on disk with
      per-point labels verified present (not assumed)
- [ ] T1–T6 each report: hypothesis, numbers, kill verdict (cause /
      cosmetic / inconclusive with the stated reason)
- [ ] The single cause (or ranked causes) named with body counts, as above
- [ ] Fixes filed to owning tickets with the directing numbers; nothing
      fixed here
