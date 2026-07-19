PRD — Base Detection for Hierarchical Archaeological Assembly
Purpose and scope

I want a reliable, geometry-only way to identify and validate vessel bases so the assembler can anchor reconstructions to a real datum instead of a height percentile. The detector must work directly on 3D scans of sherds, survive chipped edges, and produce clean residuals and confidences I can plug into the global solver and the phase controller. I will fuse three independent kinds of evidence: an axis-aware proximity cue around the pot bottom, a primitive fit on a base plane and (where present) foot-ring annulus, and a thickness-profile signal that often peaks near the base. This mirrors how recent reassembly systems use axis, profile and thickness as structural constraints rather than just scores, and it stands on published estimators I can cite and implement. Structure-from-Sherds and its follow-on explicitly operate with an axial model and report high accuracy on real assemblages; I will lean on that model and keep my detector compatible with their assumptions. 
CVF Open Access
+2
arXiv
+2

Problem this solves

Height bands alone mislabel small fragments and force ring attempts when a single large base fragment already defines the form. I also see “valid” rings with big chord gaps because nothing checks coplanarity or the presence of a foot-ring. The fix is to require independent geometric evidence, treat “one dominant base piece” as sufficient, and feed base parameters as hard-edged constraints (plane, circle/annulus, height) into the global step.

Objectives

I will deliver a base detector that outputs a vessel-level base model and per-piece base probabilities, with residuals I can log and thresholds I can tune. The model includes a base plane, an optional foot-ring annulus (inner/outer radii and centre), and uncertainty. It must integrate cleanly with the current rim→base→body controller and the pose graph. Accuracy is favoured over recall; when evidence is weak I’d rather leave the base unconstrained than over-fit.

Inputs and assumptions

The detector consumes each sherd’s point cloud or mesh, estimated normals, and boundary polylines. I assume an axis of symmetry can be estimated from one or more sherds; PotSAC gives me a robust axis even with noisy normals, and I’ll refresh the axis from confident rim/base points once if needed. Axially symmetric pottery is the main target; where symmetry is weak, plane fits and thickness remain useful, but I will down-weight axis-dependent cues. 
CVF Open Access

Functional design

I start by estimating or confirming the vessel axis and moving points to cylindrical coordinates, because base evidence strengthens near the axis root. Axis estimation follows PotSAC’s two-stage RANSAC and robust refinement; it’s a drop-in step with published details and code references. With an axis in hand, I score every boundary loop on every sherd for “base-likeness” using three tests that are independent but complementary.

The first test is planar structure near the minimum height. I run a robust plane fit on candidate base edges and compute RMS distance and inlier ratio. A good base fragment will offer a broad, low-noise plane close to the vessel’s minimum height band. This is a standard sample-consensus problem; I can use PCL’s plane and circle models or pyRANSAC-3D’s primitives without new maths, which keeps the code small and the behaviour explainable. 
GitHub
+1

The second test is foot-ring geometry. If the vessel stands on a ring foot, edges at the base will lie on (or between) two concentric circles in the base plane. I detect this by projecting candidate points to the fitted base plane and running a 3D circle fit; when both inner and outer arcs are present I fit an annulus. PCL’s SampleConsensusModelCircle3D and pyRANSAC-3D’s circle are directly applicable, returning centre, radius, normal and inliers. I care about RMS, inlier ratio and angular coverage; high coverage plus low RMS is strong confirmation. The ceramic literature treats ring feet as annular bases, so an explicit annulus detector is archaeologically sound. 
Point Cloud Library
+2
leomariga.github.io
+2

The third test is the internal thickness profile. Thickness tends to increase towards the base and thin towards the lip; I sample thickness along fracture-adjacent paths and ask for a stable local maximum in the lower height band. Thickness has been used successfully to drive reassembly and remains robust when surfaces are worn, so it’s a good cross-check when plane or ring signals are messy. I’ll compute a trend score (gradient sign and correlation) and fold it into the base probability. 
imeko.org
+2
arXiv
+2

I then fuse these cues into per-edge scores and per-piece probabilities and promote a vessel-level base model by robust consensus across the highest-scoring edges. If a single fragment alone explains most of an annulus or plane perimeter—say more than eighty per cent angular coverage with sub-threshold RMS—I mark the base as “complete by single piece” and skip ring assembly. If only one or two candidates exist, I keep RMS strict and relax coverage slightly, logging that the base is provisionally constrained on limited evidence. These decisions become soft or hard constraints in the global optimiser: coplanarity equalities for base-flagged points, inequality to keep other points above the plane, and, if detected, circle/annulus distance residuals anchoring the foot radius. The constraint form matches what axis-aware reassembly already uses for rim/profile, so it integrates naturally with my existing KKT or MIQP step. 
CVF Open Access

Success metrics and validation

I’ll accept a base detection when the fitted plane has RMS below five millimetres and at least sixty per cent inliers among candidate points, or when a foot-ring annulus has RMS below five millimetres with angular coverage above eighty per cent. On the eight-piece test set I expect the detector to either declare a complete base from the dominant base sherd or to guide base assembly without increasing collisions after global refine. On a larger set I expect fewer failed base rings and more stable global poses once the base constraint is active.

Interfaces and integration

I will add a BaseDetector with two public calls. evaluateEdgesForBase(const Shard& s, const Axis& axis) returns per-edge residuals and a base confidence for that sherd. promoteBaseModel(const std::vector<EdgeEvidence>&) runs robust consensus and returns a BaseModel struct containing plane parameters, optional annulus, covariance, and acceptance flags. The phase controller will call promoteBaseModel at the start of the BASE phase; if BaseModel::single_piece_complete is true, it will skip base ring construction and go straight to body filling with the plane/annulus residuals enabled in the global solve.

Configuration and thresholds

I will expose millimetre thresholds for plane RMS (default five), annulus RMS (default five), minimum angular coverage for single-piece completeness (default 0.8×2π), relaxed coverage when candidates are scarce (default 0.5×2π), and minimum inlier ratios for both plane and circles. Thickness trend weights and required correlation will be tunable. I’ll also expose a flag to refresh the axis once from high-confidence rim/base points, because that often tightens fits notably. Published axis estimators like PotSAC justify a refresh step when new evidence arrives. 
CVF Open Access

Telemetry and logging

For each candidate edge I will log plane RMS and inlier ratio, circle or annulus RMS, angular coverage, thickness trend statistics, and the fused confidence. For each vessel I will log whether single-piece sufficiency triggered and the parameters of the accepted base model. All thresholds used for decisions will be printed so test reports make sense.

Risks and mitigations

A poor axis estimate can degrade circle fits; I mitigate by allowing a single axis refresh from confident rim/base points and by down-weighting axis-dependent terms when the axis variance is high. Some wares have flat, poorly defined bases without ring feet; in those cases I rely on the plane and thickness cues and avoid forcing an annulus. Over-strict thresholds can starve assembly; I will treat them as soft during exploration and only harden at acceptance time. If performance dips, I’ll subsample boundary points and cap RANSAC iterations; PCL and pyRANSAC-3D both scale well at my fragment sizes. 
GitHub
+1

Rollout and tests

I will guard the detector behind a feature flag and ship a unit test set that feeds synthetic planes and annuli with controlled noise through the fitter to check RMS, coverage and failure modes. I’ll then run three datasets: the eight-piece pot, a chipped-base vessel where single-piece sufficiency should trigger, and a ring-foot ware where the annulus fit should carry the decision. I will compare before/after on base attempts started, accept/reject metrics, collisions after refine, and time-to-assembly.