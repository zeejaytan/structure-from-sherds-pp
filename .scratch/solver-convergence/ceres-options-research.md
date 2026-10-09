# Ceres LM trust-region controls: primary-source fact sheet

Scope: underdetermined free-free pairwise solve (brief framing: 12 unknowns = 2x SE(3),
median 2–6 point correspondences at ~100 mm mean distance, vessel ~300 mm).
Facts only — no recommendation. No source code was edited.

## 1. Which Ceres is pinned where (two different answers in this workspace)

| Location | Pin | Evidence |
|---|---|---|
| Laptop/Docker build | Ceres **1.14.0** (vendored tarball) | `structure-from-sherds-pp/Dockerfile:1` (`FROM ubuntu:20.04`), `Dockerfile:6` (`ENV CERES_VERSION="1.14.0"`); tarball at `structure-from-sherds-pp/ceres-solver-1.14.0.tar.gz` (repo-root listing). Header cites below are to tarball member `include/ceres/solver.h`, `types.h`, `problem.h` (extracted to `C:\tmp\ceres114\` for reading only). |
| HPC containers | Ubuntu 22.04 (jammy) `apt libceres-dev` = Ceres **2.0.0** (`2.0.0+dfsg1-5`) | `structure-from-sherds-pp/hpc/containers/container.def:2` (`From: ubuntu:22.04`) + `:37` (`libceres-dev` in the apt list at `:31–38`); `structure-from-sherds-pp/hpc/containers/container-base.def:2` (`From: ubuntu:22.04`) + `:41–45` (apt list, `libceres-dev` at `:45`). Version number from [packages.ubuntu.com/jammy/libceres-dev](https://packages.ubuntu.com/jammy/libceres-dev) ("Package: libceres-dev (2.0.0+dfsg1-5)"). |

**Does `max_trust_region_radius` exist in the jammy-pinned version? Yes.**
It already exists in the older vendored 1.14.0 header
(`include/ceres/solver.h:86` default + `:352–353` field docs, see §2),
and the current official docs (which track post-2.0) document it at
[nnls_solving.html § `max_trust_region_radius`](http://ceres-solver.org/nnls_solving.html).
So it exists in both 1.14.0 and 2.0.0.

API break to know about: vendored 1.14.0 has `Problem::SetParameterization`
(`problem.h:329–330`) / `LocalParameterization` (`problem.h:54,285,330`) and **no**
`SetManifold` (absent across all 496 lines of `problem.h`; grep for
`SetManifold` returns nothing). `Problem::SetManifold` / `ceres::Manifold` are the
Ceres 2.0+ API, documented at
[nnls_modeling.html § `SetManifold()`](http://ceres-solver.org/nnls_modeling.html).
An experiment written against 1.14 headers must use `SetParameterization`;
one written for the jammy container (2.0.0) may use `SetManifold`.

## 2. Candidate controls: exact name, default, one line, source

All defaults below are from the vendored 1.14.0 `Options()` constructor
(`include/ceres/solver.h:62–141`); field-doc lines follow it.

### Trust-region walk bounds (all in `ceres::Solver::Options`)

| Field | Default (1.14.0) | One line | Primary source |
|---|---|---|---|
| `trust_region_strategy_type` | `LEVENBERG_MARQUARDT` (`solver.h:78`) | Selects the LM trust-region step computation (vs `DOGLEG`). | tarball `include/ceres/solver.h:78` (default), `:308` (field); [nnls_solving.html § Levenberg-Marquardt](http://ceres-solver.org/nnls_solving.html) |
| `minimizer_type` | `TRUST_REGION` (`solver.h:63`) | Selects trust-region vs line-search minimizer family. | `solver.h:63` (default), `:173` (field); [nnls_solving.html § Introduction](http://ceres-solver.org/nnls_solving.html) |
| `initial_trust_region_radius` | `1e4` (`solver.h:85`) | Starting trust-region radius for the first LM step. | `solver.h:85` (default), `:352` (field); [nnls_solving.html § `initial_trust_region_radius`](http://ceres-solver.org/nnls_solving.html) |
| `max_trust_region_radius` | `1e16` (`solver.h:86`) | Hard ceiling the radius is never expanded past. | `solver.h:86` (default), `:352–353` (field); [nnls_solving.html § `max_trust_region_radius`](http://ceres-solver.org/nnls_solving.html) |
| `min_trust_region_radius` | `1e-32` (`solver.h:87`) | Minimizer terminates when the radius shrinks below this. | `solver.h:87` (default), `:355–357` (field + "Minimizer terminates when …"); [nnls_solving.html § `min_trust_region_radius`](http://ceres-solver.org/nnls_solving.html) |
| `min_relative_decrease` | `1e-3` (`solver.h:88`) | Lower bound on relative decrease before a step is accepted. | `solver.h:88` (default), `:359–361` (field); [nnls_solving.html § `min_relative_decrease`](http://ceres-solver.org/nnls_solving.html) |
| `min_lm_diagonal` / `max_lm_diagonal` | `1e-6` / `1e32` (`solver.h:89–90`) | Clamp `diag(J'J)` used for the LM trust-region scaling. | `solver.h:89–90` (defaults), `:363–371` (field + comment); [nnls_solving.html § `min_lm_diagonal` / `max_lm_diagonal`](http://ceres-solver.org/nnls_solving.html) |
| `max_num_consecutive_invalid_steps` | `5` (`solver.h:91`) | Invalid steps tolerated (shrink radius and retry) before `NUMERICAL_FAILURE`. | `solver.h:91` (default), `:373–378` (field); [nnls_solving.html § `max_num_consecutive_invalid_steps`](http://ceres-solver.org/nnls_solving.html) |
| `use_nonmonotonic_steps` | `false` (`solver.h:80`) | Allow steps that locally increase cost ("jump over boulders"); best-ever point is still returned. | `solver.h:80` (default), `:313–339` (field); [nnls_solving.html § Non-monotonic Steps](http://ceres-solver.org/nnls_solving.html) |
| `max_consecutive_nonmonotonic_steps` | `5` (`solver.h:81`) | Window size for accepting non-monotonic steps. | `solver.h:81` (default), `:339` (field); [nnls_solving.html § Non-monotonic Steps](http://ceres-solver.org/nnls_solving.html) |
| `minimizer_type` = `LINE_SEARCH` family (`line_search_type`, `line_search_direction_type`, …) | `WOLFE` / `LBFGS` (`solver.h:64–65`) | Different step-size control (direction first, then distance); not the current mode. | `solver.h:64–65`; [nnls_solving.html § Line Search Methods](http://ceres-solver.org/nnls_solving.html) |

### Stop criteria (`ceres::Solver::Options`)

| Field | Default (1.14.0) | One line | Primary source |
|---|---|---|---|
| `max_num_iterations` | `50` (`solver.h:82`) | Max minimizer iterations per `Solve()` call. | `solver.h:82` (default), `:341–342`; [nnls_solving.html § `max_num_iterations`](http://ceres-solver.org/nnls_solving.html) |
| `max_solver_time_in_seconds` | `1e9` (`solver.h:83`) | Wall-clock cap per solve. | `solver.h:83`; [nnls_solving.html § `max_solver_time_in_seconds`](http://ceres-solver.org/nnls_solving.html) |
| `function_tolerance` | `1e-6` (`solver.h:92`) | Stop when `(new_cost − old_cost) < ftol × old_cost`. | `solver.h:92` (default), `:380–384`; [nnls_solving.html § `function_tolerance`](http://ceres-solver.org/nnls_solving.html) |
| `gradient_tolerance` | `1e-10` (`solver.h:93`) | Stop when max projected-gradient norm is below this (≈ `1e-4 × function_tolerance` per header). | `solver.h:93` (default), `:386–391`; [nnls_solving.html § `gradient_tolerance`](http://ceres-solver.org/nnls_solving.html) |
| `parameter_tolerance` | `1e-8` (`solver.h:94`) | Stop when `\|step\|₂ ≤ ptol × (\|x\|₂ + ptol)`. | `solver.h:94` (default), `:393–397`; [nnls_solving.html § `parameter_tolerance`](http://ceres-solver.org/nnls_solving.html) |

### Linear solver choice (`ceres::Solver::Options::linear_solver_type`)

| Field/value | Default (1.14.0) | One line | Primary source |
|---|---|---|---|
| `linear_solver_type` | `SPARSE_NORMAL_CHOLESKY` if SuiteSparse/CXSparse linked else `DENSE_QR` (`solver.h:96–100`) | Which factorization solves the normal equations each LM step. | `solver.h:96–100` (default), `:401` (field); [nnls_solving.html § Linear Solvers](http://ceres-solver.org/nnls_solving.html) |
| `DENSE_QR` (enum `types.h:72`) | — | Method of choice for small dense problems ("a couple of hundred parameters and a few thousand residuals"). | `include/ceres/types.h:61–72`; [nnls_solving.html § DENSE_QR](http://ceres-solver.org/nnls_solving.html) |
| `DENSE_NORMAL_CHOLESKY` (enum `types.h:68`) | — | Cheaper than QR when rows ≫ columns; squares conditioning. | `types.h:68`; [nnls_solving.html § DENSE_NORMAL_CHOLESKY](http://ceres-solver.org/nnls_solving.html) |
| `DENSE_SCHUR` / `SPARSE_SCHUR` (enum `types.h:83–87`) | — | Eliminate one bipartite block set first; designed for bundle adjustment (camera/point structure, `S = B − E C⁻¹Eᵀ`). | `types.h:78–87`; [nnls_solving.html § DENSE_SCHUR & SPARSE_SCHUR](http://ceres-solver.org/nnls_solving.html) |
| `SPARSE_NORMAL_CHOLESKY` (enum `types.h:76`) | — | Sparse Cholesky on `H` (needs SuiteSparse/CXSparse/Eigen-sparse). | `types.h:74–76`; [nnls_solving.html § SPARSE_NORMAL_CHOLESKY](http://ceres-solver.org/nnls_solving.html) |
| `ITERATIVE_SCHUR` / `CGNR` (enum `types.h:92–95`) | — | Iterative (inexact-step LM) solvers for large scale; `CGNR` only with `LEVENBERG_MARQUARDT`. | `types.h:89–95`; [nnls_solving.html § ITERATIVE_SCHUR / CGNR](http://ceres-solver.org/nnls_solving.html) |

Fact relevant to the tiny-pairwise question: the official docs present
`DENSE_QR` as the method of choice for small dense problems and the Schur
solvers as exploitation of camera/point bipartite sparsity
(same section links as above). The code's 12-unknown pairwise problem has no
camera/point bipartition (pose-only blocks `s[i]`/`trans[i]`, see §3).

### Anchoring / freezing parameter subsets (`ceres::Problem`)

| Method | Default | One line | Primary source |
|---|---|---|---|
| `Problem::SetParameterBlockConstant(double* values)` | Blocks are variable unless set constant | Holds the indicated block fixed during optimization. | tarball `include/ceres/problem.h:314–315` ("Hold the indicated parameter block constant during optimization."); [nnls_modeling.html § `SetParameterBlockConstant()`](http://ceres-solver.org/nnls_modeling.html) |
| `Problem::SetParameterBlockVariable(double* values)` | — | Re-allows a held-constant block to vary. | `problem.h:317–318`; [nnls_modeling.html § `SetParameterBlockVariable()`](http://ceres-solver.org/nnls_modeling.html) |
| `Problem::IsParameterBlockConstant(double* values)` | — | Query whether a block is currently held constant. | `problem.h:320–321`; [nnls_modeling.html § `IsParameterBlockConstant()`](http://ceres-solver.org/nnls_modeling.html) |
| `Problem::SetParameterization(values, LocalParameterization*)` (1.14) / `SetManifold` (2.0+) | No parameterization unless set | Restricts how a block may move (e.g. subset/fixed-axis); 1.14 name is `SetParameterization`, 2.0 name is `SetManifold`. | `problem.h:323–330` (1.14); [nnls_modeling.html § `SetManifold()` / `SubsetManifold`](http://ceres-solver.org/nnls_modeling.html) (2.x docs) |
| `Problem::SetParameterLowerBound/UpperBound(values, index, bound)` | ±∞ (unbounded) | Per-coordinate box bounds; trust-region path handles them (LM/Dogleg "augmented with a line search if bounds constraints are present"). | `problem.h:337–339`; `solver.h:150–173` + `:293–303` (bounded trust-region problem statement); [nnls_solving.html § Trust Region Methods](http://ceres-solver.org/nnls_solving.html) |

## 3. What our code currently sets vs leaves default

File: `structure-from-sherds-pp/class/reconstruction.cpp` (2259 lines).
Five `ceres::Solve` sites. Parameter blocks are per-shard angle-axis `s[i]`
(`double[3]`) + translation `trans[i]` (`double[3]`).

| # | Enclosing function (line) | Solve block (lines) | Sets (non-default or re-stated) | Anchors? | Trust-region / tolerance fields left at default |
|---|---|---|---|---|---|
| 1 | `Icp` (pairwise/cycle, `:1194`; blocks allocated `:1220–1223`; `ceres_iteration = 100` at `:1213`) | `:1351–1358` | `max_num_iterations = ceres_iteration` (100); `minimizer_progress_to_stdout = false`; `linear_solver_type = SPARSE_SCHUR`; `function_tolerance = 1e-6` (equals 1.14 default at `solver.h:92`); `num_threads = 1` | **None** — no `SetParameterBlockConstant` between problem build (`:1310–1349`) and `Solve` (`:1358`). Both pose blocks free. | `initial/max/min_trust_region_radius`, `min/max_lm_diagonal`, `use_nonmonotonic_steps`, `minimizer_type`, `trust_region_strategy_type`, `parameter_tolerance`, `gradient_tolerance` untouched |
| 2 | `Registration` 4-arg (single moving piece, `:1450`; `s`/`trans` single blocks `:1461–1462`; `ceres_iteration = 100` at `:1465`) | `:1591–1598` | Same five fields as #1 (100 iters, `SPARSE_SCHUR`, `1e-6`, 1 thread, no stdout) | Rim scalars only: `SetParameterBlockConstant(&R_rim)` + `(&H_rim)` at `:1587–1588` (inside `if (rim)`). Pose blocks (`s`, `trans`) stay free. | Same as #1 |
| 3 | `Registration` 6-arg (`merge_node` variant, `:1648`; blocks `:1660–1661`; `ceres_iteration = 100` at `:1664`) | `:1808–1815` | Same five fields as #1 (100 iters, `SPARSE_SCHUR`, `1e-6`, 1 thread) | Rim scalars only (`:1804–1805`), same as #2 | Same as #1 |
| 4 | `IcpIncGraphAxis` (multi-shard, `:1874`; blocks `:1891–1892`; `ceres_iteration = 200` at `:1898`) | `:2001–2008` | Same five fields (200 iters, `SPARSE_SCHUR`, `1e-6`, 1 thread) | Prior-graph shards frozen: `SetParameterBlockConstant(s[i])` + `(trans[i])` for `pregraph.back().node_[i]` at `:1993–1998` | Same as #1 |
| 5 | `IcpFine` (multi-shard refine, `:2079`; blocks `:2092–2093`; `ceres_iteration = 100` at `:2098`; comment at `:1356` etc. notes it is currently dead/never called) | `:2199–2206` | Same five fields (100 iters, `SPARSE_SCHUR`, `1e-6`, 1 thread) | **None** at this site (no `SetParameterBlockConstant` near `:2198–2206`) | Same as #1 |

Additional facts:

- Nowhere in `reconstruction.cpp` is any of `initial/max/min_trust_region_radius`,
  `use_nonmonotonic_steps`, `minimizer_type`, `trust_region_strategy_type`,
  `parameter_tolerance`, `gradient_tolerance`, `SetManifold`/`SetParameterization`,
  or `DENSE_*` set (grep for `trust_region|min_trust|max_trust|initial_trust|`
  `nonmonotonic|minimizer_type|line_search|parameter_tolerance|gradient_tolerance|`
  `SetManifold|SetParameterization|LocalParameterization|DENSE_|ITERATIVE_` returns
  only the five `SPARSE_SCHUR` lines `:1354, :1594, :1811, :2004, :2202`).
- The `20000 mm` sane-translation check is a **post-solve rejection**, not a solver
  bound: `:1365–1379` (Icp), `:1603–1610` (Registration 4-arg), `:1829–1836`
  (Registration 6-arg). Comment at `:1365–1372` notes observed walks to `1e11+` mm
  (`1.7e13` on pair 1-2) on underdetermined sets; sane placements live at hundreds
  of mm (GT max 427).
- Loss functions at the pairwise sites: `CauchyLoss(4.0)` distance + `CauchyLoss(1.8)`
  normal/axis/rim (`:1312–1315`, `:1552–1555`). Robust loss downweights outlier
  residuals; it does not bound parameter travel.
- Outer loops (`max_iteration = 150` at `:1213, :1465, :1664`; `200` at `:1898`;
  `100` at `:2098`) re-linearize around fresh correspondences each outer iteration
  and call `Solve` again — per-solve caps apply per inner `Solve`, not across the
  outer loop.

## 4. Candidate one-variable experiments (exact field to set; no picking)

Each row changes exactly one thing against the current code at the target site
(site #1, `Icp :1351–1358`, is the free-free pairwise solve). Values shown are
placeholders in the correct units/scale context (scene: correspondences ~100 mm,
vessel ~300 mm, sane translations < hundreds of mm per `:1365–1372`); the fact
is the field, not the number.

| Label | Exact change (one variable) | What it factually does (from §2) |
|---|---|---|
| cap-only | `options.max_trust_region_radius = <X>;` (e.g. order of scene scale) before `ceres::Solve` | Caps how far the trust-region radius may ever expand (`solver.h:86, :352–353`) |
| init-only | `options.initial_trust_region_radius = <Y>;` | Lowers the first-step radius from the `1e4` default (`solver.h:85, :352`) |
| floor-only | `options.min_trust_region_radius = <Z>;` | Raises the radius below which the minimizer terminates (`solver.h:87, :355–357`) |
| lm-clamp-only | `options.max_lm_diagonal = <D>;` and/or `options.min_lm_diagonal = <d>;` | Clamps `diag(J'J)` used in LM scaling (`solver.h:89–90, :363–371`) |
| nonmonotonic-only | `options.use_nonmonotonic_steps = true;` (+ optionally `options.max_consecutive_nonmonotonic_steps = <W>;`) | Permits locally cost-increasing steps within window `W` (`solver.h:80–81, :313–339`) |
| anchor-only (fix piece A) | `problem.SetParameterBlockConstant(s[fix]); problem.SetParameterBlockConstant(trans[fix]);` before `Solve` | Holds one shard's rotation + translation fixed; solves only the other (`problem.h:314–315`) |
| anchor-only (freeze translation, keep rotation) | `problem.SetParameterBlockConstant(trans[i]);` for the anchored shard(s) | Freezes translation blocks only; rotations still solve (`problem.h:314–315`) |
| anchor-only (freeze rotation, keep translation) | `problem.SetParameterBlockConstant(s[i]);` for the anchored shard(s) | Freezes rotation blocks only; translations still solve (`problem.h:314–315`) |
| bounds-only | `problem.SetParameterLowerBound(trans[i], k, -B); problem.SetParameterUpperBound(trans[i], k, +B);` per axis `k = 0,1,2` | Box-bounds each translation coordinate (`problem.h:337–339`); honored through the bounded trust-region path (`solver.h:293–303`) |
| ptol-only | `options.parameter_tolerance = <P>;` (default `1e-8`) | Stops when the step norm is small relative to `\|x\|` (`solver.h:94, :393–397`) |
| gtol-only | `options.gradient_tolerance = <G>;` (default `1e-10`) | Stops on small projected gradient (`solver.h:93, :386–391`) |
| ftol-only | `options.function_tolerance = <F>;` (current code sets `1e-6`) | Stops on small relative cost change (`solver.h:92, :380–384`) |
| iters-only | `options.max_num_iterations = <N>;` (current: 100 at sites #1–3, #5; 200 at #4) | Caps LM iterations per `Solve` (`solver.h:82, :341–342`) |
| linsolver-only | `options.linear_solver_type = ceres::DENSE_QR;` (replacing `SPARSE_SCHUR`) | Factorizes the small dense Jacobian directly instead of Schur elimination (`types.h:72` vs `:83–87`; docs §§ DENSE_QR, DENSE_SCHUR & SPARSE_SCHUR) |

Notes for whoever runs these: (a) radius-cap rows and anchor rows are orthogonal —
cap rows touch only `Solver::Options`, anchor rows only `Problem`; (b) radius caps
apply per inner `Solve`, while the outer `max_iteration` loop (`:1230`, `:1674`,
…) re-solves from the new state, so a per-solve cap does not cap the outer walk;
(c) on 1.14 headers use `SetParameterization`/`LocalParameterization`
(`problem.h:329–330`), on the jammy container (2.0.0) `SetManifold` is available
per the 2.x modeling docs.
