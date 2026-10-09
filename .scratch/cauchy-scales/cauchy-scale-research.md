# Ceres CauchyLoss scale semantics & robust-ICP tuning practice

**Question:** what does the Ceres `CauchyLoss` scale parameter mean mathematically,
how does it interact with the linear (functor) weights, and how should it be chosen
from residual units and inlier thresholds — for the SFS++ assembly solver's four
residual classes (distance/mm, normal-difference/unitless, axis Cao error, rim mm)?

**Method:** primary sources only — Ceres solver source (`loss_function.h`/`.cc`),
Ceres docs (modeling chapter incl. Theory, tutorial robust-fitting section, solver docs),
the general-robust-loss literature (Barron 2019, which includes Cauchy as a special case),
ICP robust-kernel practice (Open3D `RobustKernel` source), the SfS++ paper text, and the
local code under study. Every claim carries its source. Derivations from a cited formula
are marked `(derivation)`.
No code was changed for this report.

**Scope guard:** this report establishes what the sources say and what arithmetic follows
from them. It does **not** diagnose the observed divergence (§8).

---

## 1. Key result up front

For a residual block whose residual vector is `f` (already multiplied by the code's
linear weight `w`), Ceres calls the loss with the **squared norm** `s = ‖f‖²`, and the
block's contribution to the objective is `½·ρ(s)` with the **scaled** Cauchy robustifier

```
ρ(s; a) = a² · log(1 + s/a²)          (CauchyLoss(a), scale a > 0)
```

so that with the residual-vector norm `r = ‖f‖` (i.e. `s = r²`):

- **Effective per-block weight on the gradient:** `ρ′(s) = 1 / (1 + (r/a)²)` —
  equals 1 at `r = 0`, **0.5 at `r = a`**, 0.2 at `r = 2a`, 0.1 at `r = 3a`,
  → `(a/r)²` for `r ≫ a`.
- **Scale `a` is the residual-norm magnitude (in the residual's own units) at which
  downweighting reaches 50%.** It is the radius of the "quadratic bowl": inside
  `r < a` the block behaves ~like least squares; outside, its influence *decreases*
  toward zero (redescending — §3).
- **Linear weight `w` and scale `a` are not interchangeable.** `w` multiplies the
  residual *before* the loss, so it simultaneously (i) stiffens inliers (small-error
  cost scales as `w²`) **and** (ii) moves the outlier knee in *raw geometric* units to
  `a/w`. Scale `a` moves the knee **without** changing inlier stiffness (§5). Sweeping
  weights alone (E1–E3) therefore never explored the pure-robustification direction.

Numerically (§4): at a 100 mm initial distance error, `CauchyLoss(4.0)` assigns weight
≈ 0.0016 and influence ≈ 0.16 (vs. peak influence 2.0 at `r = a`) — i.e. the sources
predict far-init residuals contribute almost no gradient under the live-path scales
(applied arithmetic, not a diagnosis — §8).

---

## 2. Exact CauchyLoss definition in Ceres (primary source)

### 2.1 Formula and calling convention

Ceres solves `min ½·Σᵢ ρᵢ(‖fᵢ‖²)` subject to parameter bounds; `ρᵢ` is the `LossFunction`
and `fᵢ` the `CostFunction` residual vector ("residual block") —
[Modeling Non-linear Least Squares, Introduction](http://ceres-solver.org/nnls_modeling.html)
(eq. 1 and the `ρᵢ`/`fᵢ` definitions).

The loss interface contract — [same page, `LossFunction` section](http://ceres-solver.org/nnls_modeling.html):
given `s = ‖residuals‖²`, fill `out = [ρ(s), ρ′(s), ρ″(s)]`, and the term's cost is
`½·ρ(s)`. Sane robustifiers satisfy `ρ(0) = 0, ρ′(0) = 1`, `ρ′ < 1` in the outlier region,
`ρ″ < 0` in the outlier region — "so that they mimic the squared cost for small residuals."

Unscaled Cauchy — [same page, Instances](http://ceres-solver.org/nnls_modeling.html):
`ρ(s) = log(1 + s)`.

Header declaration with at-zero values — `CauchyLoss` in
https://raw.githubusercontent.com/ceres-solver/ceres-solver/master/include/ceres/loss_function.h
("Inspired by the Cauchy distribution", `ρ(s) = log(1 + s)`, "At s = 0: rho = [0, 1, −1/a²]").

Implementation — https://raw.githubusercontent.com/ceres-solver/ceres-solver/master/internal/ceres/loss_function.cc
(`CauchyLoss::Evaluate`: with `b = a², c = 1/b, sum = 1 + s·c`: `ρ = b·log(sum)`,
`ρ′ = 1/sum`, `ρ″ = −c/sum²`).

### 2.2 What the scale parameter does (length scaling, in residual units)

[Modeling page, "Scaling" subsection](http://ceres-solver.org/nnls_modeling.html) —
given a robustifier `ρ(s)`, scale `a > 0` produces `a²·ρ(s/a²)` with derivatives
`ρ′(s/a²)` and `(1/a²)·ρ″(s/a²)`, while "the behaviour near s = 0 is the same as the
original function." The squaring appears, the docs state explicitly, because
**"`a` is in the units of the residual vector norm whereas `s` is a squared norm.**
For applications it is more convenient to specify `a` than its square."

Consequences (derivations from the cited scaling rule + §2.1):

1. `a` has the **units of the residual vector** entering that loss — mm for a
   distance residual in mm, unitless for a normal-difference residual, etc. (§6 works
   out each class).
2. Near zero the loss is scale-free (`ρ ≈ s` regardless of `a`); `a` only sets **where
   robustification takes place**.
3. Ceres ships a `LossFunctionWrapper` precisely so a scale can be mutated after problem
   construction, documenting the tuning practice: "when performing estimation from data
   which has substantial outliers, convergence can be improved by **starting out with a
   large scale, optimizing the problem and then reducing the scale**. This can have better
   convergence behaviour than just using a loss function with a small scale" —
   [Modeling page, `LossFunctionWrapper`](http://ceres-solver.org/nnls_modeling.html).
   This is the graduated-non-convexity (coarse-to-fine) schedule, ranked as experiment E-B in §7.
4. Ceres distinguishes this **length scaling** ("affects the space in which s is measured")
   from **output scaling** (`ScaledLoss`: `s → c·ρ(s)`), whose documented purpose is exactly
   "you might want to **weight different error terms differently** (e.g., weight pixel
   reprojection errors differently from terrain errors)" —
   [Modeling page, `ScaledLoss`](http://ceres-solver.org/nnls_modeling.html).
   Our code's functor weights act inside the residual (equivalent to output scaling `×w²`
   for small residuals — §5), while `CauchyLoss(a)` is length scaling. They are
   mathematically distinct operations; the docs give them different classes.

### 2.3 Robustified gradient and IRLS weight (why "downweighting starts at a")

Ceres' Theory section derives the robustified gradient `g(x) = ρ′·Jᵀf` and Gauss–Newton
Hessian `H = Jᵀ(ρ′ + 2ρ″ffᵀ)J`, then re-weights residual/Jacobian so any Jacobian-based
least-squares step applies — [Modeling page, Theory](http://ceres-solver.org/nnls_modeling.html)
(`g(x)`, `H(x)`, rescaled `f̃/J̃`, Triggs reference; notes Ceres clamps the Triggs
correction at `ρ″ > 0`, see `corrector.cc`).

So `ρ′(s) = 1/(1 + s/a²)` is literally the multiplicative weight on that block's
gradient contribution. Rewriting with `r = ‖f‖`:

```
w_Cauchy(r; a) = 1 / (1 + (r/a)²)     (derivation; identical form confirmed
                                       independently in Open3D, below)
```

Independent ICP-practice confirmation — Open3D
https://raw.githubusercontent.com/isl-org/Open3D/master/cpp/open3d/pipelines/registration/RobustKernel.h
(`CauchyLoss`: `p(r) = (k²/2)·log(1 + (r/k)²)`, `Weight: w(r) = 1/(1 + (r/k)²)`,
framing: kernels turn the problem into iteratively-reweighted least squares and "the
only impact of the choice of kernel is through its first order derivative") and
https://raw.githubusercontent.com/isl-org/Open3D/master/cpp/open3d/pipelines/registration/RobustKernel.cpp
(same weight implemented). Same formula, same `k ↔ a` role, from the registration
literature side.

Influence function (gradient magnitude vs. residual norm), in Ceres cost units —
`ψ(r) = d/d r [½·a²·log(1 + r²/a²)] = r/(1 + (r/a)²)` (derivation). `ψ` rises linearly
(`ψ ≈ r`, least-squares-like) for `r < a`, peaks at **`r = a` with `ψ_max = a/2`**
(`dψ/dr = 0 ⟺ r = a`; derivation), then falls ∝ `a²/r` toward zero. A residual at
`10a` pushes ~25× less than one at `a`; at `25a`, ~100× less.

Redescending character (primary robust-loss literature): Barron 2019 §1
(https://arxiv.org/html/1701.03077v10) — Cauchy is the `α = 0` member of the general
loss; "the derivative's magnitude begins to decrease as |x| grows larger than c (in the
language of M-estimation, the derivative, aka 'influence', is 'redescending')"; `c`
"controls the size of the loss's quadratic bowl near x = 0" (Eqs. 1/5/8/9 and the
paragraph beginning "The shape of the derivative…"). Barron §1 also notes loss
monotonicity in `α` (Eq. 12), which underwrites graduated schedules (start convex,
increase robustness during optimization). Barron §3 (3.3 Fast Global Registration, 3.4
Robust Continuous Clustering) reports that exposing robustness as a tuned/annealed
hyperparameter improves registration/clustering versus a fixed robust loss (section
titles + abstract; body truncated in fetch, so no numeric claim is carried over here).

Ordering among Ceres' own losses: the header notes "in the region of interest
(i.e. s < 3): TrivialLoss ≥ HuberLoss ≥ SoftLOneLoss ≥ CauchyLoss" —
[loss_function.h](https://raw.githubusercontent.com/ceres-solver/ceres-solver/master/include/ceres/loss_function.h).
At equal *scaled* `s`, Cauchy downweights the most aggressively of the standard set.

---

## 3. Shape summary (what the curves say)

| Residual norm vs. scale | Weight `ρ′` | Influence `ψ(r)` (Ceres cost units) | Regime |
|---|---|---|---|
| `r ≪ a` | ≈ 1 | `≈ r` (linear, least-squares-like) | inlier bowl |
| `r = a` | **0.5** | **maximum, `a/2`** | knee: downweighting "starts" here by the 50% convention |
| `r = 2a` | 0.2 | `0.4a` (falling) | outlier slope |
| `r = 3a` | 0.1 | `0.3a` | strong suppression |
| `r ≫ a` | `≈ (a/r)²` | `≈ a²/r → 0` | effectively ignored; gradient contribution ~nil |

Contrast with Huber (same Ceres page): Huber's influence *saturates* to a constant for
large residuals (never decreases), Tukey's goes *exactly to zero* beyond `a`. Cauchy sits
between: influence decays smoothly to zero but never cuts off hard. Practical reading
supported jointly by the Ceres loss list, Barron §1 (`α` ordering), and the Open3D kernel
weights: Huber = safer gradients far out, weaker outlier rejection; Cauchy = strong
rejection but near-zero gradients for far outliers (underdetermined drift risk if *most*
residuals start far out — mechanism, §8); Tukey = most aggressive.

---

## 4. Numeric examples at our residual magnitudes

Conventions: `r` = norm of the residual *vector as passed to the loss* (i.e. **after**
the code's linear weight `w`; §5/§6 convert to raw geometric units). Weight
`w(r) = 1/(1 + (r/a)²)`; influence `ψ(r) = r·w(r)`; cost `C(r) = (a²/2)·ln(1 + (r/a)²)`
(all derivations from §2 formulas; arithmetic checked term by term).

### 4.1 Distance residuals (mm): r ∈ {1, 5, 10, 100}

| a (mm) | r=1: (w, ψ, C) | r=5 | r=10 | r=100 |
|---|---|---|---|---|
| **1.0** | (0.500, 0.500, 0.347) | (0.038, 0.192, 1.629) | (0.0099, 0.099, 2.308) | (0.00010, 0.010, 4.605) |
| **1.8** | (0.764, 0.764, 0.436) | (0.115, 0.574, 3.508) | (0.031, 0.314, 5.607) | (0.00032, 0.032, 13.02) |
| **4.0** (live dist) | (0.941, 0.941, 0.485) | (0.390, 1.951, 7.53) | (0.138, 1.379, 15.85) | (0.0016, 0.160, 51.5) |
| **5.0** (upstream dist) | (0.962, 0.962, 0.490) | (0.500, 2.500, 8.66) | (0.200, 2.000, 20.18) | (0.0025, 0.249, 74.9) |

Readings: quadratic cost would give `C = r²/2` (0.5 / 12.5 / 50 / 5000). At `a = 4`,
a 5 mm error costs 7.5 (mildly robustified, weight still 39%); a 10 mm error costs 15.9
vs. 50 with weight 14%; a 100 mm error costs 51.5 vs. 5000 with weight 0.16%.
At `a = 1.0`, even a 5 mm error is essentially discarded (weight 3.8%) — the bowl only
covers ~1 mm.

### 4.2 Normal-difference residuals (unitless): r ∈ {0.1, 0.5, 2.0}

| a | r=0.1: (w, ψ, C) | r=0.5 | r=2.0 |
|---|---|---|---|
| **0.5** (graph norm) | (0.962, 0.096, 0.0049) | (0.500, 0.250, 0.0866) | (0.0588, 0.118, 0.354) |
| **1.8** (live norm) | (0.997, 0.0997, 0.0050) | (0.928, 0.464, 0.120) | (0.448, 0.895, 1.303) |
| **2.0** (upstream norm) | (0.998, 0.0998, 0.0050) | (0.941, 0.471, 0.121) | (0.500, 1.000, 1.386) |

Angular calibration (derivation): for two unit normals `‖n₁ − n₂‖ = 2·sin(θ/2)`.
So `r = 0.1/0.5/2.0` ≈ 5.7° / 29° / 180° (opposite). The paper's geometric-verification
angle threshold is `θ = 30°` (paper Appendix C Table II, line 560–567 of
`papers/text/sfspp-2502.13986v1.md`), i.e. `r ≈ 0.52`. Under the live-path normal path
(`a = 1.8`, but residual pre-multiplied by `w_n = 3` — §6), the *raw* knee sits at
`1.8/3 = 0.6` ≈ 35° (derivation; §6.2).

### 4.3 What "100 mm from median-2-point inputs" means in these units (applied arithmetic)

Task context states solver inputs at ~100 mm with translations diverging to 1e11+ mm.
Applying §4.1: at `r = 100`, `a = 4` → weight 0.0016; `a = 1` → 0.0001; influence 0.16 /
0.01 against a peak influence of 2.0 / 0.5. The sources' mechanism (§8): blocks that far
out contribute ~nothing to gradient or (Gauss–Newton) Hessian while *also* still counting
log-compressed cost — an optimizer started with most residuals in this regime has almost
no data-driven gradient to follow. This is stated as the sources' predicted failure
*mode*, not as the diagnosis of our runs (§8).

---

## 5. Weight-vs-scale interaction (why E1–E3 could not substitute for scales)

### 5.1 The exact interaction

Our functors multiply the raw geometric error by a linear weight **inside** the residual
(code facts — `structure-from-sherds-pp/class/reconstruction.h`):
`CostFuncDist` writes `residual = w_d · temp` (lines 377–379);
`CostFuncNorm` writes `w_n · tempn` (lines 522–524);
`CostFuncLineDist` writes `w_line · projections` (lines 451–454);
`BiaxialCaoErrorWithFixedAxis` writes `w_ · (…)` (lines 342–343);
`CostFuncRim` writes `w_r·(R − r), w_h·(H − h)` (lines 590–591).

The loss then sees `s = ‖w·e‖² = w²·‖e‖²` where `e` is the raw geometric error vector.
Substituting into §2.1 (derivation):

```
C(e; w, a) = ½·a²·log(1 + w²‖e‖²/a²)
```

Two consequences:

1. **Effective raw-unit knee: `‖e‖* = a/w`** (vector-norm sense; per-component
   `≈ a/(w√n)` for an n-dimensional block with uniform per-component error — derivation).
   Doubling `w` at fixed `a` halves the geometric error at which downweighting starts.
2. **Inlier stiffness scales as `w²`, independent of `a`.** For small errors,
   `C ≈ ½·w²‖e‖²` (the `a` cancels — this is the scale-free near-zero behavior the Ceres
   Scaling section guarantees, §2.2). So `w` sets how hard inliers pull *relative to other
   classes*, while `a` sets where each class *stops* pulling.

Hence: moving `w` changes **both** the inter-class balance (inlier stiffness ∝ `w²`)
**and** the outlier knee (`a/w`) — coupled. Moving `a` changes **only** the knee.
Any hypothesis of the form "outlier rejection is mistuned" lives in the `a` direction;
E1–E3 moved `w` only, i.e. explored the coupled direction and rebalanced classes at the
same time. A null result there cannot reject a scale mistuning. (Logic from the cited
formulas; the E1–E3 history itself is task context.)

### 5.2 Dimensionality subtlety (per-block, not per-component)

Ceres applies one loss per residual *block* to `s = ‖f‖²` summed over the block's
components (§2.1). Our blocks differ in dimension (header + call sites):
P2P distance 3 (`CostFuncDist`, `reconstruction.h:354`; added at `reconstruction.cpp:1057`);
P2L 4 (`CostFuncLineDist`, `reconstruction.h:417`; added at `reconstruction.cpp:1014`);
normal 3 (`CostFuncNorm`, `reconstruction.h:499`); axis 1 (`BiaxialCaoError…`, `1, 3, 3`
at `reconstruction.cpp:879–887`); rim 2 (`CostFuncRim`, `2, 3, 3, 1, 1` at
`reconstruction.cpp:971–973`).
So the same `(w, a)` pair implies different per-component knees across classes
(`a/(w√n)`), and the rim block mixes radius and height errors (mm each, but different
physical meaning and noise) under one scale. Tuning guidance from the sources: choose
`a` per class from that class's inlier residual distribution, not one global number —
this is exactly what the robust-curve-fitting precedent does (§6.4).

---

## 6. Current scale/weight combinations → effective geometric knees

Live-path weights (three pairwise solve sites): `w_d = 1.0, w_n = 3.0, w_line = 1.0,
w_a = 0.1, w_r = w_h = 1.0` — `reconstruction.cpp:1211` (`Icp`), `:1463` (5-arg
`Registration`), `:1662` (6-arg `Registration`). Live scales `4.0/1.8/1.8/1.8` —
`reconstruction.cpp:1312–1315`, `:1552–1555`, `:1762–1765`.
Graph path (`IcpIncGraphAxis`, `reconstruction.cpp:1874`): weights `w_d = 2, w_n = 5,
w_line = 3, w_r = w_h = 1, w_a = 1` (env-tunable, `:1893–1896`); scales
`cauchy_dist` default 1.0 / `cauchy_norm` 0.5 / axis 1.0 / rim 1.0 (`:1947–1952`).
Dead `IcpFine` (`reconstruction.cpp:2079`): weights `1.5/1.5/1.0/1.0/0.5/0.5`
(`:2094–2096`); scales `5.0/2.0/2.0/2.0` (`:2161–2164`).

Effective knees `a/w` in raw geometric units (derivations from the cited lines):

| Path | Distance (mm) | Normal (raw ‖Δn‖) | Axis (Cao units) | Rim (mm) |
|---|---|---|---|---|
| Live (Icp + 2× Registration) | 4.0/1.0 = **4.0** | 1.8/3.0 = **0.60** (≈35°) | 1.8/0.1 = **18.0** | 1.8/1.0 = **1.8** |
| Graph defaults | P2P 1.0/2.0 = **0.50**; P2L 1.0/3.0 = **0.33** | 0.5/5.0 = **0.10** (≈5.7°) | 1.0/1.0 = **1.0** | 1.0 |
| Dead IcpFine | 5.0/1.5 = **3.33** | 2.0/1.5 = **1.33** (≈84°) | 2.0/1.0 = **2.0** | 2.0/0.5 = **4.0** |
| Upstream 5.0/2.0 (scales only; weights not established here) | 5.0/w_d | 2.0/w_n | 2.0/w_a | 2.0/w_r,h |

Three source-grounded observations (no diagnosis):

- The live distance knee (4 mm vector-norm; ~2–2.3 mm per-component uniform, §5.2) sits
  **below** the paper's verification distance threshold `d = 5 mm` (Appendix C Table II,
  paper lines 560–567) and below the 5–12 mm iteration-0 alignment errors the code comment
  says ICP must refine through (`reconstruction.cpp:1268–1273`: "iteration 0
  feature-match alignment has 5-12mm distances that need ICP refinement"). Arithmetic:
  at `r = 8 mm` (mid-range init error), `a = 4` → weight 0.2. Whether that helps or harms
  is an experiment (§7), but the knee-vs-init-error overlap is a fact about the numbers.
- The graph-path defaults are 8–12× tighter than live in distance (0.33–0.5 mm vs 4 mm)
  and 6× tighter in normal (0.10 ≈ 5.7° vs 0.60 ≈ 35°). The code comment at
  `reconstruction.cpp:1944–1948` itself notes handmade-ware init error (~10–15 mm) exceeds
  the fork-tightened capture range (1.0 mm dist) and records upstream 5.0/2.0.
- The live axis knee (18 Cao-error units, via `w_a = 0.1`) is so permissive that downweighting
  may rarely engage; the Cao error's inlier unit scale is not stated in the fetched sources,
  so this is flagged as *unverified — measure first* (experiment E-0/E-E, §7).

### How the sources say to choose a scale from units and thresholds

1. **Scale ≈ small multiple of the expected inlier residual magnitude, in the residual's
   own (post-weight) units.** Precedent — Ceres
   [Robust Curve Fitting tutorial section](http://ceres-solver.org/nnls_tutorial.html):
   `CauchyLoss(0.5)` on data with Gaussian noise σ = 0.2 plus outliers
   (https://ceres-solver.googlesource.com/ceres-solver/+/master/examples/robust_curve_fitting.cc —
   data-generation comment: `noise = randn(size(x)) * 0.2`, `outlier_noise` on ~5% of points;
   `new ceres::CauchyLoss(0.5)`). I.e. scale ≈ 2.5× the inlier σ. Transferred practice:
   estimate each class's inlier σ (post-weight residual norm on known-good fits) and start
   `a ≈ 2–3σ`.
2. **Keep the knee outside the init-error range you need to refine through, or schedule it
   (E-B).** The `LossFunctionWrapper` docs (§2.2) are the primary-source license for
   large→small schedules under outlier-heavy starts.
3. **Paper thresholds are verification thresholds, not optimizer scales.** Appendix C's
   `d = 5 mm`, profile bin/deviation 7 mm (paper lines 560–567, 550) gate *candidate matches
   after the fact*; the optimizer's `a` governs *gradient weighting during the fit*. Using
   5 mm as an `a/w` anchor is a reasonable experiment (E-C) but an interpretation, not a
   derivation — the paper does not state it.

---

## 7. Ranked scale experiments (with predicted effects)

Ordering principle from the sources: measure before changing (the knee is relative to the
actual residual distribution); prefer the one intervention Ceres explicitly recommends
(graduated scales); then sweep one class at a time at fixed weights so the `a` direction
E1–E3 missed is isolated. "Predicted effects" are the sources' signatures, stated
before seeing results.

- **E-0 — Log per-class residual norms and `ρ′` at iteration 0 (no scale change). Rank 1st.**
  Record, per block class, `‖f‖`, `s/a²`, `ρ′` on live inputs (esp. the ~100 mm starts and
  the 2-point sets). Predicted value: locates every knee relative to reality (e.g. whether
  true-pair distance blocks start at `s/a² ≈ 600` with `ρ′ ≈ 0.002`, §4.3) and establishes
  the Cao-error inlier unit the sources don't give. No behavior change; precondition for E-C–E-E.
- **E-B — Graduated scale schedule via `LossFunctionWrapper` (large → small). Rank 2nd.**
  The only scale intervention the Ceres docs affirmatively recommend (§2.2, item 3).
  Predicted: fewer far-init divergences than fixed-small scales (large-`a` phase keeps
  gradients alive: at `a = 25`, `r = 100` → weight 0.06 vs 0.0016 — derivation), with final
  precision of the small-`a` phase. If divergence persists through the large-`a` phase, that
  *excludes* premature-downweighting as the sole mechanism (useful negative result).
- **E-C — Distance-scale sweep at fixed weights (`a_dist` s.t. `a/w` ≈ 1/2/4/8 mm, incl. a
  uniform ×2/÷2 arm). Rank 3rd (joint with E-B's first arms).**
  Predicted signatures: if small `a` raises divergence *and* large `a` raises false-match
  convergence, the knee brackets the operating point and E-B is the principled answer; if
  only one side moves, follow it. The 8 mm arm (`a_dist = 8` at `w_d = 1`) tests the
  "knee below init error" overlap (§6) directly.
- **E-D — Normal-scale sweep in angular terms (raw knee ≈ 0.26/0.52/1.0 ↔ 15°/30°/60°,
  i.e. `a_norm = w_n × raw`). Rank 4th.** 30° is anchored to the paper's verification
  threshold (paper lines 560–567); 15°/60° bracket it. Predicted: tighter kills rotated
  false matches but risks true pairs under noisy normals (weight at 29° error: `a = 0.78`
  → 0.69; `a = 3.0` → 0.97 — derivations); looser does the reverse.
- **E-E — Axis/rim sweeps after E-0 establishes units. Rank 5th.**
  Axis: test raw knees spanning the measured inlier σ (the live 18-unit knee is the
  permissive extreme). Rim: test `a/w` ≈ 1.8 (live) / 4 (IcpFine) / 7 mm (paper profile
  deviation) — predicted: looser admits rim-consistent-but-wrong placements, tighter demands
  accurate radius/height init (`MakeRadiusHeight` medians, `reconstruction.cpp:1343`,
  `:1979–1982`).
- **E-F — Loss-shape swap at fixed scales (Huber vs Cauchy vs Tukey). Rank 6th (diagnostic).**
  Sources' signatures (§3): Huber should show *less* far-init divergence but *more*
  false-positive convergence than Cauchy at the same `a`; Tukey the reverse (hard zero
  beyond `a`). If the ranking flips, the divergence is not robust-kernel-driven.

In all sweeps, read **divergence rate and convergence quality separately**: the sources
predict scales trade one against the other (tight = rejects outliers + risks starving true
fits of gradient; loose = refines through init error + admits false matches). A scale that
improves one metric while harming the other is the expected outcome, not a contradiction.

---

## 8. What the paper's silence implies (faithful-implementation requirements)

The paper states (Appendix B, `papers/text/sfspp-2502.13986v1.md`):

- Pairwise cost with "robust Cauchy kernels `ρ_d, ρ_e`" and "`λ` … which we set to 0.4 in
  our experiments" (lines 489–491, Eq. 8); individual cost with "`ρ_f, ρ_g`" and
  "`μ` and `ν` … both of which we set to 0.4" (lines 517–519, Eq. 14).
- ICP/LM budget: "the LM algorithm runs for a maximum of 100 iterations, and … the
  overall maximum number of ICP iteration [is] 150" (line 240).
- Geometric-verification parameters `d = 5 mm`, `θ = 30°`, `S_overlap = 50 mm²`,
  bin `ω = 7 mm`, deviation `δ_d = 7 mm` (lines 560–567).

What it does **not** state anywhere in the fetched text: any Cauchy scale (no `a`/`k`/`c`
value for any of `ρ_d, ρ_e, ρ_f, ρ_g`), the residual parameterization the scales would
apply to (the paper's `d_ij`, `e_ij`, `f_i`, `g_i` are already *squared* quantities —
Eqs. 9–11, 13, 16–17 at lines 493–533 — whereas Ceres applies `ρ` to `‖f‖²` of a
*weighted* residual vector, §5.1), or the mapping from `λ/μ/ν = 0.4` to code weights
(the code's live weights `1.0/3.0/1.0/0.1/1.0/1.0` bear no stated derivation to 0.4;
`IcpIncGraphAxis` uses yet another scheme, §6).

Implication: **no choice of Cauchy scales can be called "the paper's values" — they are
underdetermined by the paper.** A faithful implementation must additionally choose, per
class: (i) the residual parameterization and linear weight (standing in for λ/μ/ν), and
(ii) a length scale `a` in post-weight residual units, i.e. an inlier-noise model the
paper never gives. The 4.0/1.8, 1.0/0.5/1.0/1.0, and 5.0/2.0 spreads are therefore all
*implementer choices in the paper's silent dimension*, not deviations from (or
compliances with) any stated value. The experiments in §7 are the sources' prescribed way
to fill that dimension: measure inlier σ per class (E-0), start from ~2–3σ (curve-fitting
precedent, §6), and schedule large→small under outlier-heavy starts (E-B).

## 9. What this report does not claim

- No claim about *why* our solver diverges: §4.3/§8 describe a failure *mode* the sources
  predict for redescending losses under far-init, underdetermined starts; asserting it is
  *the* cause would require the E-0 measurements and controlled sweeps (§7), which are
  future work.
- No claim that any particular scale value is correct: the sources give procedures and
  precedents (§6, items 1–3), not values for sherd data.
- Upstream `5.0/2.0` is reported as scales only; the weights they paired with were not
  established in this pass, so no effective-knee comparison with upstream is made.
- Wikipedia pages fetched during the sweep (M-estimator, Huber loss) are **not** cited:
  they are tertiary summaries, and every needed statement was available in the primary
  sources above.

## 10. Source index

Ceres (primary — docs, source, example):
- http://ceres-solver.org/nnls_modeling.html — Introduction (problem form, `ρᵢ`/`fᵢ`
  definitions); `LossFunction` (Evaluate contract, `½ρ(s)`, sane-robustifier conditions);
  Scaling (length-scale rule, `a` in residual-norm units, near-zero behavior); Instances
  (Cauchy `ρ(s) = log(1+s)`, aggressiveness ordering); `ScaledLoss` (weighting terms
  differently); `LossFunctionWrapper` (large→small schedule); Theory (`ρ′`-weighted
  gradient/Hessian, re-weighting, Triggs correction clamp).
- https://raw.githubusercontent.com/ceres-solver/ceres-solver/master/include/ceres/loss_function.h —
  `CauchyLoss` declaration, at-zero values, scaling commentary.
- https://raw.githubusercontent.com/ceres-solver/ceres-solver/master/internal/ceres/loss_function.cc —
  `CauchyLoss::Evaluate` implementation.
- http://ceres-solver.org/nnls_tutorial.html — Robust Curve Fitting (`CauchyLoss(0.5)`).
- https://ceres-solver.googlesource.com/ceres-solver/+/master/examples/robust_curve_fitting.cc —
  noise σ = 0.2, outlier fraction, scale choice precedent.
- http://ceres-solver.org/nnls_solving.html, http://ceres-solver.org/solving_faqs.html,
  http://ceres-solver.org/modeling_faqs.html — LM trust-region method, linear-solver
  selection, FullReport diagnostics (solver context; no scale claims drawn from them).

Robust-loss literature (primary):
- https://arxiv.org/html/1701.03077v10 (Barron, "A General and Adaptive Robust Loss
  Function") — §1 (Cauchy as `α = 0`: Eqs. 1/5/8/9, scale `c` = quadratic-bowl size,
  redescending influence, `α`-monotonicity/graduated non-convexity); abstract + §3
  (robustness as tuned/annealed hyperparameter for registration/clustering).

ICP robust-kernel practice (primary — source):
- https://raw.githubusercontent.com/isl-org/Open3D/master/cpp/open3d/pipelines/registration/RobustKernel.h —
  IRLS framing, Cauchy `p(r)`/`w(r)` formulas.
- https://raw.githubusercontent.com/isl-org/Open3D/master/cpp/open3d/pipelines/registration/RobustKernel.cpp —
  implemented weights incl. Cauchy.

Paper (primary — local text):
- `papers/text/sfspp-2502.13986v1.md` lines 226–240 (Eq. 2, Cauchy kernels, λ; LM/ICP
  budgets), 489–533 (Appendix B: Eqs. 8–17, λ = μ = ν = 0.4, squared-quantity
  parameterization), 539–567 (Appendix C verification, Table II thresholds).

Code under study (local — cited for parameterization only, not as authority):
- `structure-from-sherds-pp/class/reconstruction.h` lines 276–352 (`BiaxialCaoError…`),
  354–385 (`CostFuncDist`), 417–461 (`CostFuncLineDist`), 499–531 (`CostFuncNorm`),
  563–599 (`CostFuncRim`); lines 136–201 (constraint signatures), 238–273 (solve-site
  signatures).
- `structure-from-sherds-pp/class/reconstruction.cpp` lines 863–1086 (constraint bodies:
  weights inside residuals, block dimensions), 1211 / 1463 / 1662 (live weights),
  1268–1273 (5–12 mm init-error comment), 1308 (ICPCOR meandist log),
  1312–1315 / 1552–1555 / 1762–1765 (live 4.0/1.8 scales), 1893–1896 (graph weights),
  1944–1952 (graph scales + capture-range comment), 2094–2096 (IcpFine weights),
  2161–2164 (IcpFine scales).
