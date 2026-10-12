# 01: Log per-class residual norms and pull-weights at iteration 0

**What to build:** the measurement every sweep needs — per residual
class (distance, normal, axis, rim), the actual norm distribution and
pull-weight at iteration 0 on live Pot_A inputs, especially the ~100 mm
starts and 2-point sets. Zero behavior change.

**Answers:** S1

**Blocked by:** None (can start immediately).

**Status:** done (measured job 32696452, verdict below; E-0 gated
all sweeps as designed)

## Approved hunk spec (2026-10-11, user m00843 "i agree with recommendation, approve")

DECISIONS LOCKED (all three open questions resolved by approval):
1. Iteration 1 logging INCLUDED. Rationale: at Icp and both Registration
   overloads the constraint staging turns axis/rim/point-to-line on only
   for iter>0 (`reconstruction.cpp:1227-1243`, `:1479-1491`, `:1680-1699`),
   and parameters are re-zeroed every iteration (`:1244-1251`,
   `:1493-1502`, `:1701-1710`), so iteration 1 is a COLD START with all
   four classes present, not a warmed-up solve. Iter 0 = dist+norm only
   (absent-class case); iter 1 = all four (full-class case), same
   far-start geometry. This is the strongest measurement the code allows
   without changing staging. Ticket asks for axis/rim at iteration 0 —
   structurally absent there; log iters 0 AND 1 rather than pretend.
2. Add-order parity is an invariant of the four constraint-helper bodies,
   NOT of the solve site. The worker must CONFIRM it empirically in the
   first dump (even block index = distance, odd = normal within the
   problem's residual-block list) rather than trusting the derivation.
3. Verdict lands in this file (checkboxes below) + S1 Where-it-stands.
   Nothing tuned here — measurement only.

HUNK SHAPE
- One new file-local helper in `class/reconstruction.cpp`, called
  immediately before each `ceres::Solve`:
  `static void LogE0Residuals(const char* site, int iter, double a_dist,
  double a_norm, double a_axis, double a_rim, ceres::Problem& problem);`
  Iteration-agnostic (no `iter == 0` branch inside); the caller decides
  which iterations to log.
- MECHANISM (all Ceres 1.14.0, verified against pinned tarball
  `C:\PR\structure-from-sherds-pp\ceres-solver-1.14.0.tar.gz`, entries at
  archive root):
  * enumerate blocks `problem.GetResidualBlocks(&blocks)` — problem.h:376
  * per-block dimension `GetCostFunctionForResidualBlock(b)->num_residuals()` — cost_function.h:122
  * per-block loss `GetLossFunctionForResidualBlock(b)` — problem.h:389
  * raw residuals `problem.Evaluate({residual_blocks,
    apply_loss_function=false}, ...)` — problem.h:479, options :435-441.
    Note 1 :450-453: reads CURRENT parameter memory, so calling
    immediately before `ceres::Solve` measures that iteration's start.
    Note 3 :476-478: CANNOT be called during a solve — placement is
    strictly before `ceres::Solve`.
- PER BLOCK: `s = ||f||^2` over the post-weight residual vector, then
  `loss->Evaluate(s, out)` gives `out[1] = rho' = 1/(1+s/a^2)`
  (loss_function.cc:72-79) — the exact IRLS weight = fraction of gradient
  the block contributes. Log `||f||`, `s/a^2`, `rho'` per block plus the
  source constant `a` for that class.
- CLASS IDENTIFICATION: residual dimension separates most cases
  (axis 1-dim `BiaxialCaoErrorWithFixedAxis` reconstruction.h:322-343;
  rim 2-dim `CostFuncRim` :584-591), but iter 0 at Icp/Both-Reg takes the
  P2P path where dist and norm blocks are BOTH 3-dim
  (`CostFuncDist` :375-379 vs `CostFuncNorm` :520-524). Use add-order
  parity (even = dist, odd = norm) — CONFIRM EMPIRICALLY, see decision 2.
- RAW UNITS: functor weights multiply into the residual before the loss
  sees it, so `||f||` is post-weight. Log both: post-weight `||f||` and
  raw-unit conversion using the site's weights (distance mm = `||f||/w`,
  normal degrees = `||f||/w_n`, axis Cao-units = `||f||/w_a`, rim mm =
  `||f||`). Cao-error inlier unit is an OPEN QUESTION for the report —
  establish it from the data, do not assume.
- SITE CONSTANTS (source of truth for the `a` values):

  | Site (class/reconstruction.cpp) | a_dist | a_norm | a_axis | a_rim | insert before | log iters |
  |---|---|---|---|---|---|---|
  | Icp :1313-1316 | 4.0 | 1.8 | 1.8 | 1.8 | Solve :1361 | 0 and 1 |
  | Registration 5-arg :1555-1558 | 4.0 | 1.8 | 1.8 | 1.8 | Solve :1603 | 0 and 1 |
  | Registration 6-arg :1767-1770 | 4.0 | 1.8 | 1.8 | 1.8 | Solve :1822 | 0 and 1 |
  | IcpIncGraphAxis :1956-1959 | cauchy_dist (env, def 1.0) | cauchy_norm (env, def 0.5) | 1.0 | 1.0 | Solve :2017 | 0 (point_to_line=true at :1904; axis/rim passed as args :1888-1889) |

  Weights per site: Icp :1212 w_d=1.0 w_n=3.0 w_line=1.0 w_a=0.1
  w_r=w_h=1.0; Registration 5-arg :1466 and 6-arg :1667 identical; Graph
  :1900-1902 w_d=2.0 w_n=5.0 w_line=3.0 w_a=1.0 w_r=w_h=1.0.
  Graph scales are env-tunable (SFS_CAUCHY_DIST / SFS_CAUCHY_NORM, gate
  :1954-1955) — log the LIVE values so an env-override run is still a
  correct dump.
- GATE + DUMP FORMAT: copy the repo's zero-behavior-change dumping idiom
  verbatim from `class/ranking_system.cpp:1915-1944` (Ticket-10 T0):
  `std::getenv("SFS_E0_DUMP")` gate, `static bool` one-shot init,
  `std::snprintf` into `<dir>/dump_e0_%04d.tsv`,
  `std::ofstream(..., std::ios::out | std::ios::trunc)`.
  Columns: `site, iter, block_index, class, dim, a, ||f||, s/a^2, rho',
  raw_unit, raw_norm, cor_pair, cor_point`. Per-point labels come from
  the same add-order walk used for parity, so every row traces to a
  specific correspondence point (ticket requires labels VERIFIED PRESENT,
  not assumed).
- ZERO-BEHAVIOR-CHANGE PROOF: with the env var unset the only added code
  is the getenv check and a static bool test — the same idiom already
  live at :1309, :1903, :1954-1955. No scale, weight, iteration, or gate
  value is touched. `Evaluate` is read-only over parameter memory and is
  never called during a solve. Verification = default-path run
  byte-identical to control BEFORE any sweep.
- BUILD/CONCURRENCY: `class/reconstruction.cpp` is in MAINS
  (`CMakeLists.txt:100`, targets :239/:254) so this rebuilds the whole
  main binary. Solve sites are serial within one assembly (omp regions at
  :518/:582/:646/:729; `//#pragma omp parallel for` commented out at
  :1282), but the static counter still makes dump names unique per
  process.
- COST: one `Evaluate` (apply_loss_function=false) + one
  `LossFunction::Evaluate` per block, per logged iteration, gated off by
  default. `EvaluateOptions::num_threads` defaults to 1
  (problem.h:404-406) so the extra call does not contend with solve
  threading.
- OUT OF SCOPE for this hunk: any scale change (that is E-C/E-D/E-E),
  `LossFunctionWrapper` graduated schedule (that is E-B), solver-options
  block / trust-region cap / anchoring (owned by `solver-convergence/`,
  spec.md:68-72), and dead `IcpFine` (`reconstruction.cpp:2088`, zero
  callers — instrumented only "for uniformity" at :1359, which
  measurement-only does not need).

- [x] Hunk: log-only block per solve site — per residual class, `‖f‖`,
      `s/a²`, `ρ′` at iterations 0 and 1 (post-weight units + raw geometric
      conversion). No scale, weight, iteration, or gate change; default
      path provably identical (control run before any sweep).
      DONE — commits `cdbf4c6` (helper + 4 call sites + batch arm) and
      `5d76f5c` (const-correctness fix for `GetCostFunctionForResidualBlock`
      / `GetLossFunctionForResidualBlock`, both `const` in Ceres 1.14.0).
      Helper at `class/reconstruction.cpp:1201`, gated on
      `std::getenv("SFS_E0_DUMP")`, static dump counter, writes
      `<dir>/dump_e0_%04d.tsv`; call sites before the four `ceres::Solve`
      (`iter <= 1` at Icp / Registration5 / Registration6, `iter == 0` at
      IcpIncGraphAxis using the LIVE env-tunable `cauchy_dist`/`cauchy_norm`).
- [x] One run, same-node, Pot_A full assembly; dumps on disk with
      per-point labels verified present (not assumed).
      DONE — Slurm job `32696452` (sfs_e0_residuals), COMPLETED 0:0 in
      00:44:55 on `spartan-bm065`, two runs of the SAME binary (default +
      dump). 38,584 files / 1.1 GB / 11,911,342 data rows at
      `/data/gpfs/projects/punim2657/sfs_main/e0_dumps`. Per-point labels:
      ZERO rows with an empty `cor_pair` or `cor_point`; `cor_point`
      restarts at 0 per file and walks to the last point.
- [x] Report: knee-vs-reality per class (e.g. true-pair distance blocks
      at `s/a² ≈ 600` with `ρ′ ≈ 0.002`?), Cao-error inlier unit
      established (the unit the sources don't give), inlier σ per class
      for the 2–3σ scale anchors. State iteration coverage explicitly:
      iter 0 = dist+norm absent-class case, iter 1 = all four classes.
      DONE — see "Measurement" below.
- [x] Verdict recorded in ticket 11 (forensics owner) + S1
      Where-it-stands; nothing tuned here — measurement only.
      DONE — see "Verdict" below.

## Iteration coverage (as designed)

| Site | iter 0 | iter 1 |
|---|---|---|
| Icp | dist + norm only (4,628 + 4,628) | axis + dist_line4 + norm (303,914 + 11,717 + 11,717) |
| Registration5 | dist + norm only (134,961 + 134,961) | axis + dist_line2 + norm + rim (5,835,643 + 10,191 + 216,850 + 206,659) |
| Registration6 | dist + norm only (92,507 + 92,507) | axis + dist_line2 + norm + rim (4,375,192 + 7,554 + 183,794 + 176,240) |
| IcpIncGraphAxis | axis + dist_line4 + norm (91,739 + 7,970 + 7,970) — `point_to_line=true` hardcoded, so it already has P2L at iter 0 | not logged (ticket says iter 0 only) |

Two deviations from the ticket's table, both structural and both
expected from the source, neither a labelling error:
1. **Icp has no rim blocks at iter 1** (0 rim rows anywhere). The rim
   constraint is guarded by a per-sherd flag that is not set on the Icp
   path, so "iter 1 = all four classes" holds for Registration but not
   for Icp (Icp iter 1 = axis + P2L + norm).
2. **IcpIncGraphAxis has no rim rows** either, for the same reason; it
   has all three classes its path can actually build.

## Parity — CONFIRMED EMPIRICALLY (no STOP)

- `dim==3` even block index = `dist`, odd = `norm`: holds on every file
  inspected (e.g. `dump_e0_0000.tsv` block 0 `dist`, 1 `norm`, 2 `dist`;
  the `dist_line4`/`norm` pairs at `dump_e0_0001.tsv` share
  `cor_point` 0, 0, 1, 1, …). Icp iter 0 has exactly equal dist/norm
  counts (4,628 = 4,628), which is only possible if every `dist` has its
  `norm` twin.
- `dim==2` (the case the ticket's parity rule cannot express): the P2L
  constraint is added BEFORE the rim blocks, so the first `dim==2` block
  of a run is `dist_line2` and the rest are `rim`. Confirmed globally:
  17,745 files carry rim rows, and in ALL of them the first `dim==2`
  block is `dist_line2`; `dist_line2` appears exactly once per file
  (0 in 20,839 files, 1 in 17,745 files, never 2+); rim 0–106 blocks per
  file. Zero files have rim without `dist_line2` — the mislabel that would
  have fired the STOP never occurs.
- Lead's twin rule: every `dist` / `dist_line2` / `dist_line4` label has a
  `norm` row at the same `cor_point` (dist_line4 7,970 ↔ norm 7,970, 1:1).
  No orphans.

## Zero-behavior-change — PASS

Run A (SFS_E0_DUMP unset) vs run B (dump set) vs control (solver-01 job
`32624499`, same node bm065):

```
Sherd accuracy : 2 / 8 (25.000%)          Sherd accuracy : 2 / 8 (25.000%)          (control: same)
Edge accuracy  : 1 / 15 (6.667%)          Edge accuracy  : 1 / 15 (6.667%)          (control: same)
PROFILE 783                                PROFILE 783                                (control: 783)
DIVERGED-SOLVE 0                           DIVERGED-SOLVE 0                           (control: 0)
```

Whole-log md5 differs (`adb30f13` vs `6492ae3c`, 20,992 diff lines) and
every diff line is non-behavioral: 56 `Analysis time:` millisecond
values, ~2,608 `MAKEMULTI` lines re-ordered by OpenMP interleave
(3,188 vs 3,168; both logs 289,776 lines), and 1,535/1,073 blank-line
hunks from interleaved stdio flush. Control numbers were CITED from the
existing solver-01 job rather than spending a third run.

## Measurement (Pot_A full assembly, `s/a²`, `ρ′`, `‖f‖` in mm unless stated)

| Class | dim | a | rows | `‖f‖` p50 / p90 | `s/a²` p50 / p90 / p99 | past the knee (`s/a²>9`) | `ρ′` p50 |
|---|---|---|---|---|---|---|---|
| dist (P2P) | 3 | 4.0 | 232,096 (Icp 4,628 / R5 134,961 / R6 92,507; **iter 0 only**) | 111–119 / ~181 | **774–890 / 2,031–2,071 / 2,375–2,747** (max 4,318, fnorm cap 262.8) | **99.81%** | **0.00128** |
| norm | 3 | 1.8 (a=0.5 at Graph) | 652,427 | 1.24 / 5.08 | iter 0: 4.07–5.40 / 10.4 / 11.1 → 19.8% past 9; iter 1: 0.11 / 1.06 | 19.8% (iter 0) | 0.18 (iter 0) → 0.90 (iter 1) |
| dist_line4 (P2L) | 4 | 4.0 | 19,687 | 1.56 / 5.85 | 0.40 / 1.70 / 66.4 (max 267) | 15.9% | 0.712 |
| dist_line2 (Fixed P2L) | 2 | 4.0 | 17,745 (Reg iter 1 only) | 1.52 / 5.76 | 0.14 / 2.07 | 2.5% | 0.874 |
| axis (Cao) | 1 | 1.8 (a=1.0 at Graph) | 10,606,488 | 2.82 / 10.55 | iter 1: 2.45 / 34.1 / 99.7 (max 312) | 29.7% | 0.2895 |
| rim | 2 | 1.8 | 382,899 | 0.71 / 2.81 | 0.16 / 2.44 / 41.2 (max 123) | 3.9% | 0.9695 |

Iter-0 `s/a²` by site, for the sweep: Icp dist 889.5 / norm 4.07;
Registration5 dist 774.5 / norm 5.40; Registration6 dist 777.4 / norm
3.51.

**Knee-vs-reality, per class.**
- **dist is far past the knee, not near it.** The ticket's guess
  (`s/a² ≈ 600`, `ρ′ ≈ 0.002`) is real in kind but low in size: measured
  median `s/a² ≈ 780` (range 774–890 by site), 96.3% of dist rows above
  `s/a² > 100`, `ρ′` median 0.0013. In plain terms: at the moment the
  solver starts, the point-to-point term is weighted to roughly a
  thousandth of its nominal strength for essentially every correspondence
  point, and 99.98% of them sit above the `s/a² = 3` point where a
  Cauchy term has already shed two-thirds of its weight.
- **norm sits at the knee.** Median `s/a²` 4.1–5.4 at iter 0 (61.9% past
  `s/a²>3`, 19.8% past `>9`), `ρ′` 0.18. One iteration later normals are
  inlier-quality (`s/a²` 0.11, `ρ′` 0.90) — the normal term converges
  first because it is the only term already near its designed operating
  point.
- **dist_line4 / dist_line2 / rim are near-or-below the knee**
  (`ρ′` 0.71 / 0.87 / 0.97): the point-to-line and rim terms are mostly
  inliers at entry, so these are the terms that actually pull the first
  iteration.
- **axis straddles the knee** (`s/a²` median 2.45, 46.9% past 3, 29.7%
  past 9, `ρ′` 0.29): about half the surface points already disagree with
  the axis enough to be down-weighted, half do not.

**Consequence for the assembly.** The first iteration is driven almost
entirely by the normal, axis, point-to-line and rim terms, and almost
not at all by point position. That is the opposite of the intuitive
reading of "the point-to-point term dominates", and it is the empirical
basis the blocked scale sweeps (tickets 02–06) need: raising `a_dist`
from 4.0 would move ~232k point-to-point blocks from "effectively
ignored" back into the solve, and it is that term — not the normals —
that is currently switched off.

**Inlier σ per class** (raw geometric units, `‖f‖/w`; the 2–3σ anchors
the scale sweeps will use).
- dist mm: p50 111–119 mm, p90 ~181 mm, p99 195–210 mm, cap 262.8 mm.
- norm (degrees, `/3.0`): p50 0.74–1.84, p90 4.6–5.4, p99 ~5.997,
  hard tail 6.0 (20 rows at exactly 5.99998).
- axis (Cao soft-min units, `/w_a`): p5 1.89, p25 10.8, p50 28.2, p75
  63.0, p90 105, p99 180.
- rim mm: p50 0.71, p90 2.81, p99 11.6, max 19.96.

**Cao-error inlier unit — established from the data, not assumed.** The
`axis` residual is the Cao soft-min form `min(d1,d2) − (1/100)log(e^{−100(d1−c)} + e^{−100(d2−c)})` times `w_a`, so its inlier unit is the
Cao soft-min value itself in millimetres-at-s=100, and the data says a
clean axis agreement is ~28 mm at the median (p5 ~1.9 mm, p90 ~105 mm).
That unit is what `a_axis = 1.8` is being compared against — so the axis
term's knee is *inside its own inlier spread* (median inlier 28 vs knee
at 1.8), which is why 47% of surface points fall past it.

## Surprises and spec errors

1. **Icp and IcpIncGraphAxis never emit rim blocks** (0 rim rows in
   382,899 rim rows total, all from Registration). The rim constraint is
   guarded by a flag not set on those paths, so the ticket's "iter 1 =
   all four classes" is true only for the two Registration overloads.
2. **The ticket's `dim` parity rule cannot cover `dim==2`** at all (both
   rim and Fixed-P2L are 2-dim). The correct discriminator is add order —
   P2L before rim — and the shipped code uses it. Had the parity rule been
   extended naively (even=dist), every Fixed-P2L block after the first
   rim block would have been mislabelled.
3. **`s/a² ≈ 600` / `ρ′ ≈ 0.002` was an underestimate** for the
   point-to-point class: measured median 780 and 0.0013.
4. **A quantized tail in two distributions.** 20 `norm` rows share the
   identical residual `fnorm = 5.99998` and 6 `dist` rows share
   `fnorm = 262.838` (= 4·√4317.74). That exact-repeat extreme smells like
   a clamp inside the weight path rather than real geometry, but it touches
   26 of 11.9 M rows, so it is immaterial to the medians above. Note it for
   tickets 02–06 only so nobody reads the tail as signal.
5. **`dist` rows exist at iter 0 only**, as designed; at iter 1 the
   point-to-point term is replaced by point-to-line at those sites.

## Verdict

Measurement hunk delivered with zero behavior change and the parity
assumption confirmed empirically. Nothing tuned, no scale/weight/gate
touched. **S1 can now be answered:** at entry the point-to-point term is
effectively switched off (`s/a² ≈ 780` vs the `a=4` knee, `ρ′ ≈ 0.0013`)
while the normal, axis, point-to-line and rim terms are at or inside
their knees — that ordering, not "the point-to-point term dominates", is
what drives iteration 0. S1's "Where it stands" needs updating from
"unknown" to this.

**Out of scope confirmed:** dead `IcpFine` uninstrumented; no scale
change, no `LossFunctionWrapper`, no solver-option/tuning change.

## Reproducing this analysis

The measurement above was produced with
`.scratch/cauchy-scales/analyze_e0_v2.py` (streaming, 272 lines) run over
the dump directory; `analyze_e0.py` is the first draft and is kept only
as the trail (it stored all 12.9 M rows flat and never finished). The
v2 script checks coverage, per-point labels, the dist/norm twin rule,
dim==3 parity and the percentile tables in one pass. The parity,
twin and zero-file checks were additionally re-verified independently
with `grep`/`awk` on Spartan (17,745 rim-bearing files, all with the
first dim-2 block = dist_line2).
