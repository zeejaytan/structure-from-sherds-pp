PRD — Rim Assembly That Beats “High-Score but Wrong” ICP
Purpose and context

I need the rim to close with the correct neighbours even when local ICP scores look fantastic for the wrong pairs. The plan is to replace “pick the highest ICP” with a rim-aware pipeline that generates robust pairwise proposals, prunes them by global consistency, and then selects the ring with a solver that can switch off dodgy edges. This mirrors how recent pot-reassembly systems treat axis/rim cues as constraints rather than trusting local matches. 
CVF Open Access

Problem I’m solving

Right now the rim phase enforces degree-2 topology and closes a ring, but it still ranks edges by ICP “quality”. That lets false neighbours win by a mile, which explains the large RMS and the swapped adjacencies. The fix is to stop using raw ICP as the primary signal and make the decision depend on globally consistent rim evidence.

Objectives

My goal is to choose a single simple ring for the rim where each piece has degree two and the chosen edges are both cycle-consistent and rim-consistent. Locally good but globally incompatible joins must be rejected automatically. I will also require a quantitative ring-closure check (fit error and maximum gap) before I call the rim “closed”.

Scope

I will change only the rim phase. I will keep geometry-only inputs. I will add four behaviours: robust rim–rim proposals, cycle-consistent pruning, global selection with switchable edges under a hard degree-2 rule, and a strict ring-closure validator.

Design principles

First, I will estimate an axis and treat the rim as a circular (or gently conical) curve orthogonal to that axis, because this is the model used by the strongest assemblies of axially symmetric pots. It gives me a vessel-level signal that ICP does not see. 
CVF Open Access

Second, I will make pairwise proposals with a certifiable registration stage rather than raw ICP, because I need hypotheses that survive extreme outlier rates. TEASER++ is designed for exactly this kind of robust rigid registration and is well supported in C++/Python. 
arXiv
+2
arXiv
+2

Third, I will prune by short-cycle and ring-consistency before any ring growth, because cycle consistency is a standard way to eliminate locally “great” but globally impossible correspondences. 
yetianwei.github.io
+1

Fourth, I will choose the ring with a global solver that includes a switch per candidate edge; edges that fight the global fit will naturally switch off, which is how robust SLAM back-ends handle false loop closures. 
nikosuenderhauf.github.io
+1

Functional requirements

I will replace the “seed + chain by ICP quality” step with a two-stage proposal generator. For every rim–rim pair I will extract rim-edge points, run TEASER++ once to get up to K rigid hypotheses, and keep the best few by inlier count and residual. This produces a much cleaner shortlist than ICP alone, especially when the fracture geometry is repetitive. 
arXiv
+1

I will score each candidate not by its local ICP residual but by how much it improves the global rim model. Concretely, I will project rim points to the plane orthogonal to the current axis, refit a robust circle (or short cone) over all currently placed rim points, then compute the marginal drop in RMS if I tentatively add the candidate. I will also measure the angular gap the candidate closes on the circle. These two numbers form the primary score. Off-the-shelf circle fitters in PCL and pyRANSAC-3D give me the parameters, RMS and inlier counts I need. 
Point Cloud Library
+1

Before selection I will prune the shortlist by cycle consistency. For any triplet sharing two candidates I will require that the composed transforms are close to identity around the 3-cycle; inconsistent triples will be broken by removing the worst offender. This follows current multi-shape matching practice where cycle consistency is used to sanitise pairwise estimates. 
yetianwei.github.io

I will then solve a small global problem on the rim only: poses for rim sherds plus a switch variable per candidate edge, with hard degree-2 constraints at every node and a rim-circle term in the objective. I will use the “switchable constraints” formulation so wrong edges drop to near-zero weight during optimisation rather than poisoning the fit. This is the standard robustification used in pose-graph SLAM to survive false loop closures. 
nikosuenderhauf.github.io

I will end with a strict ring-closure validator. I will run a Huber-weighted circle fit on all rim points, require RMS below a configured millimetre threshold and maximum chord gap below a configured angular length, and only then mark the rim “closed”. This mimics the way axis/rim constraints are applied in Structure-from-Sherds rather than trusting local matches. 
CVF Open Access

Optionally, I will add a pottery cue that is independent of rim geometry: thickness continuity across the join. If the wall-thickness profile measured either side of a proposed join disagrees strongly, I will demote or reject that edge, because thickness profiles are known to be stable across wear and have been used on their own to reassemble pots. 
arXiv
+1

Non-functional requirements

I will keep runtime within ten per cent of the current rim phase on medium components by capping K per pair and pruning early with cycles. All steps must be deterministic under a fixed random seed. All thresholds and weights will live in config.

Interfaces and integration

I will add four small modules and call them only from the rim phase. The proposal stage will wrap TEASER++ and return a list of candidate 
(
𝑖
,
𝑗
,
𝑇
𝑖
𝑗
,
inliers
,
resid
)
(i,j,T
ij
	​

,inliers,resid). The cycle pruner will take that list and remove inconsistent triples. The ring solver will expose a solveRim(poses0, candidates) that returns the degree-2 ring and switched-off edges. The validator will expose fitAndCheckRim(points) and return RMS, max gap and coverage. I will not change downstream phases; they will simply receive a cleaner, validated rim.

Telemetry and audit

For every accepted rim edge I will log the marginal RMS drop, the angular gap closed, the switch weight, and any cycle-consistency residuals involved. For every rejected top candidate I will log which gate failed first. At ring acceptance I will log final RMS, max gap and circle parameters, with a one-line verdict. This makes “why 5-6 lost, why 3-6 won” obvious in numbers rather than feelings.

Risks and mitigations

If the axis is poor early on, rim circle fits can wobble; I will refresh the axis once using the highest-confidence rim arcs after a provisional ring fit. If TEASER++ returns no hypothesis for a real neighbour on sparse chips, I will allow a single fallback ICP pass but keep its score subordinate to rim-improvement and cycle checks. If the dataset is not truly axially symmetric, the circle validator will fail loudly; in that case I will fall back to a cone fit with a tolerance.

Rollout and tests

I will guard all changes behind a feature flag. I will add unit tests for the cycle pruner and the validator on synthetic rings with injected false edges. I will run two ablations on your dataset: “old ICP-ranked ring” versus “proposal+pruning+switchable solver”, and “no thickness cue” versus “with thickness cue”. I will accept the change when rim edges match ground truth adjacencies and the final fit error drops without a runtime blowout.

External evidence backing the design

Axis-aware pot reassembly that treats rim/profile as constraints, not just scores, is established in Structure-from-Sherds, which reports strong results on real sherds and motivates my circle-based rim checks. 
CVF Open Access

TEASER/TEASER++ provides fast, certifiable rigid registration resilient to extreme outlier rates, with public code in C++/Python; it is an appropriate replacement for ICP in the proposal stage. 
arXiv
+2
arXiv
+2

Cycle consistency is a principled way to prune locally good but globally inconsistent pairwise matches in multi-shape matching, which I use before ring selection. 
yetianwei.github.io

Switchable-constraint pose-graph solvers are a standard back-end to suppress false loop closures; I apply the same idea to rim edge selection so bad edges switch off. 
nikosuenderhauf.github.io

Rim validation by robust circle fitting is practical with existing libraries (PCL and pyRANSAC-3D) that return parameters, RMS and inlier ratios; these give me measurable acceptance tests. 
Point Cloud Library
+1

Thickness profile continuity is a recognised, wear-robust cue for ceramic reassembly and supports rejecting false neighbours that “fit” on the fracture but break the wall’s internal structure. 
arXiv
+1