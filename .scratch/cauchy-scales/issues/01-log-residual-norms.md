# 01: Log per-class residual norms and pull-weights at iteration 0

**What to build:** the measurement every sweep needs — per residual
class (distance, normal, axis, rim), the actual norm distribution and
pull-weight at iteration 0 on live Pot_A inputs, especially the ~100 mm
starts and 2-point sets. Zero behavior change.

**Answers:** S1

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent (hunk spec approved 2026-10-11, m00843; see "Approved hunk spec" below)

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

- [ ] Hunk: log-only block per solve site — per residual class, `‖f‖`,
      `s/a²`, `ρ′` at iterations 0 and 1 (post-weight units + raw geometric
      conversion). No scale, weight, iteration, or gate change; default
      path provably identical (control run before any sweep).
- [ ] One run, same-node, Pot_A full assembly; dumps on disk with
      per-point labels verified present (not assumed).
- [ ] Report: knee-vs-reality per class (e.g. true-pair distance blocks
      at `s/a² ≈ 600` with `ρ′ ≈ 0.002`?), Cao-error inlier unit
      established (the unit the sources don't give), inlier σ per class
      for the 2–3σ scale anchors. State iteration coverage explicitly:
      iter 0 = dist+norm absent-class case, iter 1 = all four classes.
- [ ] Verdict recorded in ticket 11 (forensics owner) + S1
      Where-it-stands; nothing tuned here — measurement only.
