# 02: Graduated scale schedule (large → small)

**What to build:** a build-time graduated scale for the distance term at the
three hardcoded solve sites (`Icp`, `Registration` 5-arg, `Registration` 6-arg),
so the first outer iterations run with a permissive distance bowl and the scale
steps down to the live value as the geometry settles. This is the one scale
intervention Ceres affirmatively recommends (`cauchy-scale-research.md` §2.2
item 3, `LossFunctionWrapper` docs), tested here against the mechanism E-0
actually measured rather than the one the research report predicted.

**Answers:** S1

**Blocked by:** 01 — measured. Done: the endpoints in this ticket come from the
38,584 dump files of job 32696452, not from guesses.

**Status:** ready-for-agent (hunk spec revised 2026-10-11 after E-0 landed;
previous version's `LossFunctionWrapper` premise corrected — see "Spec
corrections")

## What E-0 measured (the only premise this ticket rests on)

From `class/reconstruction.cpp` `LogE0Residuals` dumps, job 32696452,
spartan-bm065, commit cdbf4c6. All numbers are per-block `s/a²` where `a` is the
site's live distance scale 4.0 and `s = ‖f‖²` post-weight.

- **dist** (P2P distance blocks, iteration 0, 232,096 rows): `fnorm` p50
  111–119 mm, p90 ~181 mm, max 262.8 mm. `s/a²` p50 774–890 (Icp 889.5,
  Reg5 774.5, Reg6 777.4), p90 ~2,062, max 4,318. `rho'` p50 **0.00128**.
  99.98% of blocks past `s/a² > 3`.
- **norm** at the same iteration: `s/a²` p50 4.07–5.40, `rho'` p50 0.18.
- **axis** (Cao, iter 1): `s/a²` p50 2.45, `rho'` 0.2895.
- **rim** (Reg iters only): `s/a²` 0.16, `rho'` 0.9695.
- **dist_line4** (P2L): 0.40; **dist_line2** (Fixed P2L): 0.14.

Reading: iteration 0 is steered by normals, axis, point-to-line and rim. The
distance term is off by roughly three orders of magnitude — not marginally
tight, **effectively suppressed**. That is the failure mode this ticket tests a
remedy for.

## Spec corrections (why this ticket changed after E-0)

The research report (`cauchy-scale-research.md` §2.2 item 3, §7 E-B) predicted
E-B as a remedy for *premature downweighting* under outlier-heavy starts, and
this ticket was written on that premise. E-0 measured something stronger, so the
premise is restated rather than assumed:

1. **No `LossFunctionWrapper` exists in this tree.** All 34 loss references in
   `class/reconstruction.cpp` are raw `new ceres::CauchyLoss(a)` built inside
   each solve site immediately before `AddResidualBlock` — there is nothing to
   mutate after construction. The schedule must be a **build-time** choice (a
   phase-derived scale read when the loss is constructed), not a mutate-in-place
   wrapper. Ceres' `LossFunctionWrapper` remains the source of the *practice*
   recommendation; it is not the implementation shape available here.
2. **The direction is right but the purpose is different.** Ceres' advice is
   "start large, refine to small for precision." Here the distance term at the
   *final* value (4.0) is 780× past the knee on iteration 0, so a fixed 4.0 never
   lets the term participate at all until iteration 1 has already moved the
   fragments on normals+axis+rim. The question is not "does premature tightening
   cause divergence" — it is **"does a permissive early phase bring the distance
   term online, and does the schedule end at the same place."**
3. **`IcpIncGraphAxis` is excluded from the schedule.** It reads its scales from
   the environment (`SFS_CAUCHY_DIST`, `SFS_CAUCHY_NORM`, `:2109-2114`), so a
   code-side schedule would silently redefine what those variables mean. Leave it
   on env values and treat it as an **in-run control**: same binary, same node,
   same inputs, unaffected by this hunk. Note this amends spec.md:57 ("same value
   everywhere the class appears on the live path") for this one site; the
   amendment is deliberate and recorded here.
4. **Dead `IcpFine`** (`:2250-2390`, scales 5.0/2.0/2.0/2.0 at `:2332-2335`) has
   **zero callers** — confirmed by grep across the repo (only the declaration at
   `reconstruction.h:268` and this definition). Do not touch it. A schedule
   applied to a dead path is a silently empty experiment.

## Approved hunk spec

- [ ] Add one small file-local helper near the `LogE0Residuals` block
      (`:1201-1333`) that maps (site, outer iteration) → distance scale, e.g.
      `static double GraduatedDistScale(const char* site, int iter)`.
      Same shape and gate as the E-0 helper: file-local, no header change, no
      new dependency. **It is not env-gated** — the schedule is the behaviour
      under test, so the default path IS the experimental path. (State the
      contrast with ticket 01 explicitly in the commit message.)
- [ ] Schedule, anchored to the measured distribution (values are implementer
      choices from the measurement, not derivations — `cauchy-scale-research.md`
      §6 item 1 says a ≈ 2–3σ; the interval endpoints below are the measured
      p50/p90/percentile anchors):

      | outer iter | `a_dist` | anchor |
      |---|---|---|
      | 0 | 260.0 | max observed `fnorm` 262.8 mm — nothing is past the knee at entry |
      | 1–9 | 181.0 | p90 of the measured distribution |
      | 10–49 | 119.0 | p50 of the measured distribution |
      | 50+ | 4.0 | the live value; unchanged from iter 50 onward |

      Rationale for the iter boundaries: `ceres_iteration = 100` inner iterations
      per outer iteration (`:1354`, `:1615`, `:1820`), `max_iteration = 150`. The
      first outer iteration already re-zeros all parameters and rebuilds
      correspondences, so iteration 1 is a cold start with axis/rim/P2L staged
      on — the 1–9 band is where those classes come online with the distance term
      already admitting the residuals E-0 measured. The step to p50 at iter 10
      and the final step at iter 50 are both before the solve can have converged
      on the p50-centered geometry. **These boundaries are the schedule's
      free parameters**; record them as choices.
- [ ] Apply `GraduatedDistScale` at exactly the three hardcoded distance sites:
      `:1453` (Icp), `:1702` (Registration 5-arg), `:1918` (Registration 6-arg) —
      replacing the literal `4.0` in `new ceres::CauchyLoss(4.0)` only. `loss_norm`,
      `loss_axis`, `loss_rim` at those sites keep their literals 1.8/1.8/1.8.
- [ ] Do **not** change `:2111` (`IcpIncGraphAxis`, env-tunable) or `:2332`
      (dead `IcpFine`).
- [ ] Weights, iteration counts, gates, solver options, caps, staging, the
      `LogE0Residuals` call sites and the E-0 instrumentation are all untouched.
      The six control invariants the batch arm greps must still hold
      (`initial_trust_region_radius = 100` ×5, `max_trust_region_radius = 1000` ×5).
- [ ] Keep the `LogE0Residuals` calls passing the site's **live** constant at
      iteration 0/1 so the dump still reports the schedule's actual early scale
      (i.e. the constant passed to `LogE0Residuals` must be the graduated value,
      not the literal 4.0, or the dump lies about iteration 0). Confirm the dump
      column `a` shows 260 at Icp iter 0.

- [ ] Same-node A/B vs the 01 tree: Pot_A full assembly, pinned machine
      (bm065), in-job verification per ticket-12 protocol — marker grep +
      freshness + POT_A check + outcome-line comparison, exactly the shape of
      `hpc/sbatch/run_e0_residual_norms.sbatch`. New arm
      `hpc/sbatch/run_e2_graduated.sbatch` modelled on it.
- [ ] Two runs in the arm (per ticket-12 practice): the schedule run, plus one
      control run of the **same binary** to confirm the arm's outcome lines match
      the cited control — do not cite a third tree.
- [ ] **Kill criterion stated before running:**
      1. Primary read — does the distance term come online? Compare the dump's
         `s/a²` and `rho'` for the `dist` class at iteration 0 between this run
         and ticket 01's dump. With `a = 260` the median `s/a²` should fall from
         ~780 to roughly `(115/260)² ≈ 0.20` and `rho'` from 0.0013 to ~0.84
         (arithmetic prediction, not a commitment).
      2. Divergence: `DIVERGED-SOLVE` count. Ticket 01 and its control are both 0,
         so the criterion is **"stays 0"** — this hunk is not expected to fix
         divergence, because the cap work already brought it to 0. If divergence
         appears, that is new and must be reported as a STOP.
      3. Join quality: sherd/edge accuracy vs the cited control (2/8 sherds,
         1/15 edges, PROFILE 783). **Movement in either direction is the
         finding**, not just improvement — a scale that raises distance influence
         is expected to move false matches too (`cauchy-scale-research.md` §7:
         tight trades starvation against rejection, loose the reverse).
      4. No previously-passing pair may be lost: the 1–4 pair still passes and the
         pair count does not drop below the control's.
      - If the dump shows the term still suppressed through the large phase
        (primary read fails): the suppressed-at-entry mechanism is NOT fixed by a
        permissive schedule alone — useful negative, route to ticket 03 with the
        numbers and say so.
      - If sane changes path (a pair's state changes, accuracy moves, or
        divergence appears): do not tune the schedule in place. Revert, record
        which pairs moved, and report.
- [ ] Report: the primary read with the before/after `s/a²` and `rho'` numbers,
      the outcome lines against the control, the diverged count, and the
      schedule-as-chosen table. Record the verdict in ticket 11 + S1
      Where-it-stands, and note explicitly that the schedule's iter boundaries
      are free parameters that this run does not tune.

## Verdict — E-2 FAILED its kill criterion (term online, joins lost), 2026-10-12

Job `32722789` (sfs_e2_graduated), COMPLETED 0:0 in 00:20:16 on
`spartan-bm065`, commit `fd3e42f`. Same node, same inputs as the E-0
control. Worker died after pushing correct code; lead audited the hunk
and read the dumps directly.

**Implementation verified correct before trusting the numbers:**
`GraduatedDistScale` at `class/reconstruction.cpp:1335-1363`, schedule
table exactly per spec (iter<1 → 260.0, <10 → 181.0, <50 → 119.0, else
4.0), build-time, not env-gated. Applied at the three hardcoded sites
only (`:1486` Icp, `:1735` Registration5, `:1951` Registration6);
`IcpIncGraphAxis` still reads `SFS_CAUCHY_DIST` at `:2142-2144`, dead
`IcpFine` still `CauchyLoss(5.0)` at `:2365`. Dump calls pass the
graduated value, not 4.0 (`:1540`, `:1786`, `:2009`) — the dump's `a`
column does not lie. Six cap invariants intact.

**Primary read — the term came online, on prediction:**
dist class at iteration 0, `a = 260` confirmed in the dump column at all
three sites (Icp 260, Reg5 260, Reg6 260; in-run control
`IcpIncGraphAxis` still `a = 1`):

| site | n | `s/a²` p50 / p90 / max | `ρ′` p50 / p90 |
|---|---|---|---|
| Icp | 4,628 | 0.2107 / 0.4846 / 0.5768 | 0.8261 / 0.9665 |
| Registration5 | 141,014 | 0.1817 / 0.5155 / 0.9483 | 0.8462 / 0.9688 |
| Registration6 | 99,806 | 0.1782 / 0.5088 / 0.8133 | 0.8487 / 0.9571 |

Ticket predicted `s/a² ≈ 0.20`, `ρ′ ≈ 0.84` from `(115/260)²`. Landed
on it. E-0 at `a = 4.0` was `s/a²` p50 774–890, `ρ′` p50 0.00128.
39,944 dump files at `/data/gpfs/projects/punim2657/sfs_main/e2_dumps`.

**Outcome lines vs the cited control (solver-01 job 32624499, same node):**

| | control | E-2 schedule |
|---|---|---|
| Sherd accuracy | 2/8 (25.000%) | **0/8 (0.000%)** |
| Edge accuracy | 1/15 (6.667%) | **0/15 (0.000%)** |
| PROFILE | 783 | 855 |
| DIVERGED-SOLVE | 0 | 0 (stays 0 — no new divergence) |
| Best assembly | 7/8, score 262.000 | 7/8, score 266.000 |
| SURVIVED / FEATURE_MATCHING | 14 / 1 | 14 / 1 (unchanged) |
| Pair 1-4 | SURVIVED | still SURVIVED |

**Reading:** a permissive distance bowl at entry brings the term online
*and* costs both recovered joins. Score went UP (266 vs 262) while
accuracy went to zero — the signature of false matches admitted, the
"loose admits false matches" trade `cauchy-scale-research.md` §7 warns
of. Divergence stayed 0, so this is not instability; the term pulls
harder on wrong things. Per the ticket's own rule: do not tune the
schedule in place. The iter boundaries (1/10/50) remain free parameters
this run does not tune. Route to ticket 03 with these numbers: whether
the distance term can be brought online anywhere without admitting false
matches, or whether the 2/8 must be recovered by a different class.

## Out of scope (unchanged from spec.md)

Weights, iteration counts, gates, caps/anchoring (`solver-convergence/` tickets
01/02), correspondence gate at formation, the Juglet half. No value in this
ticket is "the paper's" — the paper states no scale, so every number here is an
implementer choice anchored to the E-0 measurement.
