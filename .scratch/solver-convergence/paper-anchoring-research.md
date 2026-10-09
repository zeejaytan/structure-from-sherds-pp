# SfS++ solver convergence + anchoring — paper facts (no conclusions)

Research sidekick report. Facts out, no verdicts. All SfS++ quotes from primary source
`C:\PR\papers\text\sfspp-2502.13986v1.md` with section + line. No source code edited.
Row 4 (`feature_matching.cpp` veto) untouched.

## 1. What the SfS++ paper promises — quoted

### 1a. Pairwise ICP solver: P2P-then-P2L, Cauchy kernels, λ/μ/ν=0.4, LM-100-inner / ICP-150-outer

Sec. IV-C2 "Adjustment of matched sherds" (`sfspp-2502.13986v1.md:211-240`):

- `sfspp-2502.13986v1.md:213 (Sec. IV-C2)`: "Therefore, we adopt a two-step verification process. First, we run the Iterative Closest Point (ICP) on the initial pairs of edge lines. The ICP algorithm comprises two stages: correspondence assignment followed by correspondence optimization. Second, we actively exploit pottery's physical properties, such as consistency of profile curve and non-overlapping, to prune out remaining incorrect matching pairs after the ICP algorithm converges."
- `sfspp-2502.13986v1.md:215 (Sec. IV-C2, Forming correspondences)`: "In the initial iteration of ICP, we use correspondence pairs from descriptor matching. In subsequent iterations, correspondence pairs are formed by retrieving mutually closest points based on Euclidean distance, pruning pairs with normal vector differences exceeding 30^∘."
- `sfspp-2502.13986v1.md:226 (Sec. IV-C2, Eq. 2)`: "$$ J_{R}(A,B):=\\sum_{(i,j)\\in\\Omega^{AB}}\\rho_{d}(d_{ij}(\\mathtt{T}^{A},\\mathtt{T}^{B}))+\\lambda\\rho_{e}(e^{2}_{ij}(\\mathtt{T}^{A},\\mathtt{T}^{B})) $$"
- `sfspp-2502.13986v1.md:228 (Sec. IV-C2)`: "To handle outlier correspondences effectively, we adopt robust Cauchy kernels $\\rho_{d},\\rho_{e}$ and scale $e_{ij}$ by a factor $\\lambda$ to control its relative influence. In practice, the initial iteration of the ICP algorithm treats $d_{ij}$ as a point-to-point (P2P) distance, providing a high-quality starting assumption. From the next iteration onward, we switch to a point-to-line (P2L) distance, which better captures the geometric relationship between points and lines along the fracture edges, resulting in a more accurate alignment (see Fig. 9)."
- `sfspp-2502.13986v1.md:232 (Sec. IV-C2, Eq. 3)`: "$$ J_{I}(A):=\\mu\\sum_{i\\in\\Omega^{A}}\\rho_{f}(f_{i}^{2}(\\mathtt{T}^{A}))+\\nu\\sum_{i\\in\\Psi^{A}}\\rho_{g}(g_{i}^{2}(\\mathtt{T}^{A},r^{\\mathrm{rim}},h^{\\mathrm{rim}})) $$"
- `sfspp-2502.13986v1.md:234 (Sec. IV-C2)`: "We again use Cauchy kernels $\\rho_{f},\\rho_{g}$ to handle potential outliers, while $\\mu$ and $\\nu$ control the relative importance of the axis consistency and rim consistency terms."
- `sfspp-2502.13986v1.md:238 (Sec. IV-C2, Eq. 4)`: "$$ \\argmin_{\\mathtt{T}^{A},\\mathtt{T}^{B},r^{\\mathrm{rim}},h^{\\mathrm{rim}}}J_{R}(A,B)+\\sum_{E\\in\\{A,B\\}}J_{I}(E) $$" — both sherd poses free in the pairwise module.
- `sfspp-2502.13986v1.md:240 (Sec. IV-C2)`: "In the PM module, only two sherds are considered at a time. We employ the Levenberg-Marquardt algorithm for the robust nonlinear optimization described above, iteratively updating all variables. At each ICP iteration, the LM algorithm runs for a maximum of 100 iterations, and we set the overall maximum number of ICP iteration to 150. By balancing *pairwise* fracture matching in Eq. (8) with *individual* constraints in Eq. (14), we achieve stable reconstructions. This cost functions minimize distance and normal mismatches between shards and ensure the consistent alignment of axes and rims across multiple fragments. Detailed definitions of equations can be found in our supplementary materials [43]."

Appendix B "ICP refinement" (`sfspp-2502.13986v1.md:483-533`, Eqs. 8–17):

- `sfspp-2502.13986v1.md:485 (App. B)`: "After obtaining initial candidate pairs through descriptor matching, false matches may still exist. To eliminate these, we apply the Iterative Closest Point (ICP) algorithm to iteratively optimize point-to-point or point-to-line alignments, thereby refining the local alignment between sherds."
- `sfspp-2502.13986v1.md:489 (App. B, Eq. 8)`: pairwise cost restated with $\\rho_{d},\\rho_{e}$ and $\\lambda$.
- `sfspp-2502.13986v1.md:491 (App. B)`: "Here, $\\rho_{d}(\\cdot)$ and $\\rho_{e}(\\cdot)$ are Cauchy kernels, and $\\lambda$ is a positive scalar that balances the weight of the normal misalignment term, which we set to 0.4 in our experiments."
- `sfspp-2502.13986v1.md:493 (App. B, Eqs. 9–10)`: "At first ICP iteration, we use geometric error function as a point-to-point (P2P) alignment ($d_{ij}(\\mathtt{T}^{A},\\mathtt{T}^{B})=d_{ij}^{\\text{P2P}}(\\mathtt{T}^{A},\\mathtt{T}^{B})$" with $m_{ij}:=\\mathtt{R}^{A}\\mathbf{p}^{A}_{i}+\\mathbf{t}^{A}-\\mathtt{R}^{B}\\mathbf{p}^{B}_{j}-\\mathbf{t}^{B}$.
- `sfspp-2502.13986v1.md:501 (App. B)`: "Once the sherds have been roughly aligned after the first iteration of ICP, we switch the P2P alignment to point-to-line (P2L) alignment. Empirically, P2P model often adjusts precise geometric difference under good initial condition."
- `sfspp-2502.13986v1.md:503 (App. B, Eq. 11)`: P2L $d_{ij}^{\\mathrm{P2L}}$ as four squared projection terms along $\\hat{\\mathbf{l}}^{A}_{i},\\hat{\\mathbf{l}}^{B}_{j},\\hat{\\mathbf{n}}^{A}_{i},\\hat{\\mathbf{n}}^{B}_{j}$ dotted with $m_{ij}$.
- `sfspp-2502.13986v1.md:505-507 (App. B, Eq. 12)`: fracture-surface normal $\\hat{\\mathbf{l}}^{A}_{i}$ defined via cross product of surface normal and edge tangent; text notes it is "distinct from its the surface normal ($\\hat{\\mathbf{n}}_{i}$)".
- `sfspp-2502.13986v1.md:511-513 (App. B, Eq. 13)`: "$e_{ij}(T_{A},T_{B}):=\\|R_{A}\\hat{n}_{i}^{A}-R_{B}\\hat{n}_{j}^{B}\\|_{2}^{2}$" … "ensures the surface normals of the two sherds remain consistent when they are matched."
- `sfspp-2502.13986v1.md:517 (App. B, Eq. 14)`: individual cost restated with $\\mu,\\nu$.
- `sfspp-2502.13986v1.md:519 (App. B)`: "$\\mu$ and $\\nu$ is a positive scalar that balance the weight of the axis alignment term and the rim-consistency term, respectively, both of which we set to 0.4 in our experiments."
- `sfspp-2502.13986v1.md:521-527 (App. B, Eqs. 15–16)`: axis term via Cao-Mumford-style $\\epsilon^{s}_{i}$, curvature centre $c^{s}_{i}$, log-sum-exp softmin $f_{i}:=-\\tfrac{1}{t}\\ln(\\sum_{s}\\exp(-t\\|\\epsilon^{s}_{i}\\|^{2}_{2}))$ over inward/outward normals.
- `sfspp-2502.13986v1.md:531-533 (App. B, Eq. 17)`: rim term $g_{i}=\\|r^{\\mathrm{rim}}-r(R^{A}p^{A}_{i}+t^{A})\\|^{2}_{2}+\\|h^{\\mathrm{rim}}-h(R^{A}p^{A}_{i}+t^{A})\\|^{2}_{2}$, with "$r^{\\mathrm{rim}}$ and $h^{\\mathrm{rim}}$ … computed as the median value of all rim points."

### 1b. Exploration (fix old) vs state expansion (free all)

Sec. IV-D3 "Exploring candidates for expansion" (`sfspp-2502.13986v1.md:267-294`):

- `sfspp-2502.13986v1.md:269 (Sec. IV-D3)`: "Next, using this state, axis-based descriptors and pairwise feature matching are recomputed (line 7-8). This recomputation enhances both the geometric features and the quality of pairwise matching candidates because the previously selected top-$k$ states were globally adjusted with multiple sherds."
- `sfspp-2502.13986v1.md:271 (Sec. IV-D3)`: "Similar to the PM module, the pairwise matching results are adjusted using ICP and discarded if they do not satisfy the geometric criteria (line 9-11). However, unlike the PM module, the sherd registration optimizes only the newly added sherds while keeping the previously reconstructed ones fixed. Because many potential pairwise matching candidates are explored, the global adjustment—similar to bundle adjustment in SfM—is not carried out during the exploration stage, but only on the most probable candidates in state expansion stage to ensure computational efficiency."
- `sfspp-2502.13986v1.md:282-284 (Sec. IV-D3, Sherd registration)`: "Let $\\Phi^{\\mathrm{new}}$ represent a set of new sherds to be reassembled, and $\\Phi^{\\mathrm{old}}$ denote a previously reassembled set of sherds. The sherd registration attempts to attach the new sherds ($\\Phi^{\\mathrm{new}},n(\\Phi^{\\mathrm{new}})\\geq 1$) to the existing reassembled sherds ($\\Phi^{\\mathrm{old}},n(\\Phi^{\\mathrm{old}})\\geq 1$), analogous to the incremental addition of new images or views in SfM." … "The optimization problem, described in Eq. (5), is solved while keeping the sherds in $\\Phi^{\\mathrm{old}}$ fixed."
- `sfspp-2502.13986v1.md:286 (Sec. IV-D3, Eq. 5)`: "$$ \\argmin_{\\{\\mathtt{T}^{\\Phi^{\\mathrm{new}}}\\},r^{\\mathrm{rim}},h^{\\mathrm{rim}}}\\sum_{D\\in\\Phi^{\\mathrm{new}}}(\\sum_{E\\in\\Phi^{\\mathrm{old}}}J_{R}(D,E)+J_{I}(D)) $$"
- `sfspp-2502.13986v1.md:288 (Sec. IV-D3)`: "We use the ICP algorithm (updating correspondences at every iteration) as described in Sec. IV-C2. While the default approach reassembles one sherd at a time ($n(\\Phi^{\\mathrm{new}})=1$), in the cases involving graph merging, $\\Phi^{\\mathrm{new}}$ may include multiple sherds that were previously reassembled. In such cases, the relative poses within $\\Phi^{\\mathrm{new}}$ remain unchanged—that is the multiple sherds are considered as a single reassembled large chunk."
- `sfspp-2502.13986v1.md:294 (Sec. IV-D3)`: grouping threshold "25 ^∘ for rotation and 20 mm for translation" before averaging transforms as the seed for global adjustment; scoring "based on the number of inliers."

Sec. IV-D4 "State expansion" (`sfspp-2502.13986v1.md:296-304`):

- `sfspp-2502.13986v1.md:298 (Sec. IV-D4)`: "Based on the generated priority candidates (e.g. list of most probable expansion path from the current state), the algorithm expands the state with $b$ potential matching configurations (line 16-22). Each expanded state then undergoes a global optimization step—referred to as global sherds adjustment—followed by geometric verification (line 17)."
- `sfspp-2502.13986v1.md:300 (Sec. IV-D4, Global sherds adjustment)`: "Global sherds adjustment is conceptually similar to bundle adjustment in incremental SfM, its goal is to enhance global consistency among sherds while progressively eliminating erroneous configurations in noisy graphs. For each state, we select the top-$b$ ranked priority candidates for global configuration adjustment. At this stage, we solve the similar optimization described at Eq. (5); however, the set of relevant sherds now includes all sherds, defined as $\\Phi=\\Phi^{\\mathrm{old}}\\cup\\Phi^{\\mathrm{new}}$, where $\\Phi^{\\mathrm{old}}\\cap\\Phi^{\\mathrm{new}}=\\emptyset$. Unlike the sherd registration process, previously registered sherds in $\\Phi^{\\mathrm{old}}$ also participate in this adjustment."
- `sfspp-2502.13986v1.md:302 (Sec. IV-D4, Eq. 6)`: "$$ \\argmin_{\\{\\mathtt{T}^{\\Phi}\\},r^{\\mathrm{rim}},h^{\\mathrm{rim}}}\\sum_{A,B\\in\\Phi}\\frac{1}{2}J_{R}(A,B)+\\sum_{E\\in\\Phi}J_{I}(E) $$" with "$J_{R}$ and $J_{I}$ … defined in Eqs. (8)–(14)."
- `sfspp-2502.13986v1.md:304 (Sec. IV-D4)`: "As in Sec. IV-C2, we employ the LM algorithm for nonlinear optimization and utilize the ICP algorithm to update correspondences. Finally, the globally adjusted configurations are verified against the geometric criteria."

### 1c. Stability promise — as what mechanism

- `sfspp-2502.13986v1.md:240 (Sec. IV-C2)`: stability is claimed via cost balance — "By balancing *pairwise* fracture matching in Eq. (8) with *individual* constraints in Eq. (14), we achieve stable reconstructions."
- Fixing/anchoring is stated separately as an efficiency-scoped procedure, not as the stability mechanism: `sfspp-2502.13986v1.md:271` gives the reason as "to ensure computational efficiency" (global adjustment deferred to "only … the most probable candidates in state expansion stage"); `sfspp-2502.13986v1.md:284` states the fixed-$\\Phi^{\\mathrm{old}}$ solve without attaching a stability/convergence claim; `sfspp-2502.13986v1.md:300` states the all-free global step's goal as "enhance global consistency … while progressively eliminating erroneous configurations in noisy graphs."

## 2. What the paper does NOT specify (checked by grep over the paper text)

Searched `sfspp-2502.13986v1.md` for `Ceres|g2o|GTSAM|implementation|library|tolerance|epsilon|convergence|criterion|damping|initial.*mu|trust|step|prior|regular|gauge|anchor` plus a second pass for `Cauchy.*scale|trust.region|toleran|damping|step|prior|convergen|stable|anchor|fix`. Only hits are the quotes above plus one unrelated PotSAC line below. Absent items:

1. LM implementation / library — NOT named. Paper says only "the Levenberg-Marquardt algorithm" (`:240`, `:304`). No Ceres/g2o/GTSAM/custom-solver name, no version, no linear solver.
2. Cauchy kernel scales (the robust scale inside $\\rho_{d},\\rho_{e},\\rho_{f},\\rho_{g}$) — NOT stated. Only the outer multipliers $\\lambda=0.4$ (`:491`), $\\mu=\\nu=0.4$ (`:519`) are given.
3. Trust region / damping for the ICP LM — NOT specified. The sole "trust-region" hit is PotSAC axis estimation, not the ICP solver: `sfspp-2502.13986v1.md:173 (Sec. IV-B2)`: "PotSAC utilizes an extended Cao and Mumford error … in the subsequent trust-region-based nonlinear optimization, enabling high-precision estimation even under challenging conditions." No radius, update rule, or damping value for Eqs. 4–6.
4. Step cap / translation/rotation bound per LM step — NOT specified.
5. Prior / regulariser / gauge anchor for the pairwise free-free solve (Eq. 4 optimises both $\\mathtt{T}^{A},\\mathtt{T}^{B}$) — NOT specified. No fixed-sherd, fixed-axis-frame, or prior term is attached to Eq. 4 in `:236-240` or App. B.
6. Convergence tolerance / stopping epsilon (gradient, function, or parameter change) — NOT specified. Only iteration caps: "LM … maximum of 100 iterations" + "overall maximum number of ICP iteration [150]" (`:240`). No residual-change or correspondence-stable threshold.
7. ICP correspondence-convergence rule beyond caps — NOT numerically defined beyond the 30° normal-difference prune (`:215`) and post-ICP geometric verification (overlap + profile-curve checks, `:245-249`, App. C `:539-552` with $d{=}5$mm, $\\theta{=}30^{\\circ}$, $S_{overlap}{=}50$mm², $\\omega{=}7$mm, $\\delta_{d}{=}7$mm at `:560-567`).

## 3. SfM-inspiration claim + anchoring practice

### 3a. Which SfM practices the paper cites

- `sfspp-2502.13986v1.md:37 (Sec. I)`: "The core of SfM's success lies in its incremental approach, which iteratively registers new camera views, refines pairwise matches, and robustly discovers initially undetected correspondences while reducing false positives."
- `sfspp-2502.13986v1.md:85-96 (Table II)`: analogy rows — Extracted features SIFT → axis/edge-line/rim/thickness/base; Pairwise matching SIFT distance → weighted sum of sherd features; Geometric verification fundamental/essential-matrix or homography inliers → ICP inliers; Sanity check cheirality → overlap/thickness/profile-curve; Registered quantity camera views → sherds; Triangulated model 3D scene points → 3D pot model (axis + profile curve); Joint estimation bundle adjustment → global sherd and axis alignment via ICP.
- `sfspp-2502.13986v1.md:114 (Sec. III-B)`: "In incremental SfM, features are first extracted from each image, and pairwise matches are established. Then, the reconstruction is built up incrementally by adding one view at a time, refining previous matches and discovering new ones. At each step, camera positions and 3D points are updated incrementally as new views are added, while bundle adjustment iteratively refines both camera parameters and the 3D structure."
- `sfspp-2502.13986v1.md:135 (Sec. IV-A)`: pipeline modules map to "pairwise matches … refined through Iterative Closest Point (ICP) method and geometric validation" and "incremental exploration approach, expanding candidate paths at each step while leveraging the multi-graph beam search method."
- `sfspp-2502.13986v1.md:137 (Sec. IV-A)`: "These modules significantly enhance the algorithm's ability to robustly search possible matching configurations from noisy pairwise matches, efficiently converging toward the correct solution (see more details at Sec. V-E)."
- `sfspp-2502.13986v1.md:203 (Sec. IV-C)`: "Incremental SfM reconstructs 3D structures through descriptor matching, RANSAC-based geometric verification, and iterative refinement. Similarly, our three-step Pairwise Matching (PM) module uses axis-based edge line descriptors … via the Longest Common Subsequence (LCS) algorithm, achieving robust alignments akin to SfM's feature matching. Matches are further refined via ICP and physical constraints, in similar spirit to the SfM's geometric verification to ensure precision."

So the cited SfM practices are: descriptor matching, RANSAC-based geometric verification, iterative refinement, incremental registration one view at a time, and bundle adjustment (joint refinement).

### 3b. Anchoring (fix vs free) — COLMAP source status in this workspace

- COLMAP / Schönberger–Frahm paper is NOT present in `C:\PR\papers\text`. Directory listing observed 2026-10-09 shows only: `tora-*.md`, `garf-*.md`, `sfspp-2502.13986v1.md`, `pfpp-*.md`, `jigsaw-*.md`, `pmtr-*.md`, `cmnet-*.md`, `rpf-*.md`, `global-*.md`, `pqnet-*.md`, `dgl-*.md`, `se3equiv-*.md`, `diffassemble-*.md`, `rglnet-*.md`, `breakingbad-*.md`, `fantasticbreaks-*.md`, `uni3d-*.md`, `sare-*.md`, `crag-*.md`, `em3rf-*.md`, `milo-*.md`, `goris-*.md`. No `colmap*` file to quote; web search was unavailable in this session (no API key), so no fresh primary fetch was made.
- Per the brief's fallback: the well-known COLMAP incremental practice (NOT primary-verified in this workspace — needs the COLMAP paper/slides or codebase before use as a cite) is: register the next image alone by PnP against the existing 3D structure while the existing reconstruction is held fixed; run bundle adjustment periodically (after growth thresholds) and at the end, at which point existing cameras + points are freed and jointly refined. Do NOT cite this paragraph as primary — it records the question to check, not a checked source.
- What IS primary-verified here is only the SfS++ side: exploration fixes $\\Phi^{\\mathrm{old}}$ and solves $\\Phi^{\\mathrm{new}}$ only (Eq. 5, `:271`, `:284-286`); state expansion frees all $\\Phi=\\Phi^{\\mathrm{old}}\\cup\\Phi^{\\mathrm{new}}$ (Eq. 6, `:300-302`); global step runs only on top-$b$ candidates for efficiency (`:271`, `:298-300`).

## 4. Workspace prior mapping (so new work does not duplicate)

### Ticket 07 — `structure-from-sherds-pp/.scratch/paper-compliance/issues/07-icp-weights-iterations.md`

- Question: paper cost (Eqs. 8/14 + Cauchy, $\\lambda/\\mu/\\nu$, LM-100/ICP-150) vs live code cost (`Icp`, `reconstruction.cpp:1191+`, ceres-100-inner, `max_iteration=50`-outer, split linear weights × Cauchy scales, no 0.4 in live path).
- Mapping verdict (reproducible, not structurally different): P2P structure MATCH; P2L 4-term MATCH; normal term MATCH structure but VALUE BACKWARDS (paper 0.4× down vs code 3× up, 7.5× flip); axis MATCH structure (code sqrt noted as monotonic), value same direction (0.1 vs 0.4); rim MATCH with SPLIT weights (set both 0.4 to express paper); ceres-100 MATCH; outer-150 MATCH in paper-config; Cauchy scales UNSTATED in paper (code 4.0/1.8 by necessity).
- Experiments E1/E2/E3 (jobs 31854704/31881006/31890296): normal→0.4, rim→0.4, axis→0.4 — all no-recovery (0/8+0/15 each), all reverted; weights hypothesis recorded EXHAUSTED.
- Iterations: Registration-150 (job 31898200) first 2/8 vs Registration-50 control (job 32087141) 0/8 — outer count load-bearing; single-run evidence each way, control rerun owed; `IcpIncGraphAxis` LIVE (200/200), `IcpFine` DEAD.
- Status resolved 2026-10-01 with caveats; metric note on directed-edge counting kept in ticket.

### Ticket 11 — `structure-from-sherds-pp/.scratch/juglet-sfs/issues/11-solver-divergence.md`

- Mechanism: translations escape to 1e11–1e14 mm (pair 1-2 REGOUT t_norm 1.7e13/42.6/8.2e12/8.4e12; ICPITER acc_t_norm 128.9→3e14 between outer 0→1; Dump_0030 t_z=-1.48e11 vacuous-pass; length_error aborts).
- Forensics 2026-10-10 over 27 e2e logs: 26,312 attempts with iter-0 lines, 6,586 diverged (25%); diverged pre-conditions pts median 2, meandist median 106.3mm vs ALL-attempt median 2.0pts/99.3mm — same distribution; per-pair leaderboards agree — verdict UNIFORMITY, routed to solver setup (anchor/prior/cap), not input quality; attempt-floor NOT supported (would refuse sane alike).
- Guard outcome 2026-10-03 (job 32109955): 550 DIVERGED-SOLVE fires, zero aborts/length_errors, 0/8+0/15 — bounds damage at zero sane cost, convergence untouched; guard STAYS.
- Standing candidates in order: (c) unbounded LM steps (verify trust-region option in vendored Ceres headers), (b) unanchored 12-DOF gauge drift, (a) angle-axis singularities last; weights/iterations NOT here (exhausted/measured per 07); Cauchy-scale spread noted as unowned (pairwise 4.0/1.8, graph-ICP 1.0/0.5 env-tunable, axis/rim 1.8/1.0, 8× spread, no paper values).

## 5. No-duplication note vs 07/11

- Do NOT re-derive the Eq. 8/14 ↔ code functor mapping, the λ/μ/ν value table, the E1–E3 no-recovery runs, or the 50-vs-150 iteration A/B — ticket 07 already holds them with job IDs.
- Do NOT re-mine divergence onsets, re-argue uniformity vs pre-condition, or re-litigate the guard — ticket 11 already holds the 27-log table (26,312 attempts, 25% diverged, median-2-pts) and the guard verdict (550 fires, stays, convergence open).
- Open (not established by 07/11 or by the paper quotes above): LM library/options actually compiled in (trust-region flag existence in vendored Ceres headers), Cauchy-scale provenance per path, and any anchor/prior/step-cap design with A/B — the paper is silent on all three (Sec. 2), and 07/11 explicitly leave them open in that order.
