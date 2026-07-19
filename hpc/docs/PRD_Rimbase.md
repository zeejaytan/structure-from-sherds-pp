PRD — Additional Rim/Base Validation Checks
Purpose and context

I’ve got height-based classification running and it does the job of separating likely rim, base and body pieces. That said, height alone lets through cases that are archaeologically odd: partial rims that should be treated as complete with one fragment; small base chips that trigger ring-building when the main base is already present; and rings that “close” numerically but don’t actually read as pottery. The next changes add independent geometric evidence for rims and bases, and they turn that evidence into explicit validators before I attempt ring closure. This follows published reassembly work that treats rim, profile and thickness as structural signals rather than just scores, and it lines up with standard archaeological categories of rim, body and base sherds. I will cite those precedents where relevant so the logic is defensible beyond this codebase (Structure-from-Sherds and SfS++ on axis-aware reassembly; thickness-profile methods; and practical rim/base recognition from archaeology). 
sites.brown.edu
+5
CVF Open Access
+5
arXiv
+5

Problem statement

The current implementation promotes pieces into rim/base buckets purely by percentile height. It doesn’t verify that a rim edge is circular and smooth relative to the vessel axis, it doesn’t detect foot-rings or planar bases as primitives, and it doesn’t recognise the “single-piece sufficiency” case where one large fragment already covers most of a rim or base. As a result I sometimes force ring fitting on fragments that should be accepted as complete, and I sometimes accept rings with large chord gaps because no second opinion challenges the height cue. The fix is to add independent detectors—circle/cone at the rim, plane/annulus at the base, curvature smoothness at the lip, and thickness trends—and require these to pass before I mark a ring candidate as valid. This mirrors how prior reassembly systems stabilise assembly with rim/profile constraints and thickness measurements, rather than relying on local ICP alone. 
CVF Open Access
+2
arXiv
+2

Goals and non-goals

My goal is to raise precision on rim/base decisions without slowing the pipeline noticeably. I will add geometry-only checks that are fast, robust to chips, and easy to inspect in logs: robust circle/cone fits for rims; robust plane and optional annulus (foot-ring) fits for bases; edge smoothness and tangency checks at the lip; and a thickness-trend check that prefers thinning lips and thicker base bands when present. I am not introducing texture or image classifiers here. If I later want weak priors from 2D tools such as ArchAIDE, I will blend them as optional hints, not requirements. 
ScienceDirect

Functional requirements

I will compute independent rim evidence per boundary edge before any ring attempt. For each candidate rim edge, I will project its points onto the plane orthogonal to the current axis near the edge’s mean height and fit a circle or short conical section with a robust estimator. I will record RMS point-to-circle distance, inlier ratio and angular coverage. I will also check that the edge is smooth in the sense of low curvature noise and that the local edge tangent is close to orthogonal to the axis (rim lips wrap around the axis). These fits and checks give me a per-edge “rim confidence” and clean residuals that I can log and threshold. They are standard primitive-fitting tasks and are directly supported by the PCL sample-consensus model for 3D circles and by pyRANSAC-3D for circles and planes, so there is no bespoke maths to maintain. 
PyPI
+4
Point Cloud Library
+4
GitLab
+4

I will compute independent base evidence per boundary edge as well. First, I will fit a plane to the base-candidate edge points; bases tend to be co-planar in wheel-thrown and many coil-built vessels. If there is a foot-ring, I will additionally fit an annulus by estimating inner and outer circles on that plane and checking that the edge lies between them with small residuals. This reflects ceramic form: a ring foot is a single circular band that supports the vessel, and many historic wares display measurable foot-ring geometry. The validator will accept either a good plane alone or a good plane plus annulus; in both cases I will record RMS residuals and angular coverage. 
gotheborg.com
+1

I will add a thickness-trend check as a third, independent line of evidence. Thickness profiles have been shown to support reassembly and they carry useful signals: lips often thin; bases and lower walls often thicken. I will sample thickness along fracture-adjacent paths and compute a simple statistic—local gradient sign and consistency—to boost or damp the rim/base label from height and primitives. This is cheap and geometry-only, and it matches published “thickness profile” approaches that succeed where surface wear confuses other cues. 
arXiv
+2
IMEKO
+2

I will introduce a “single-piece sufficiency” rule for both rim and base. If one fragment’s rim edge covers at least eighty per cent of a robust circle at its height with RMS under the configured threshold, I will mark the rim as complete and skip ring assembly for that vessel. Similarly, if a base fragment covers at least eighty per cent of the base plane’s perimeter circle (or an annulus if a foot-ring is present) with RMS under threshold, I will mark the base as complete. This prevents spurious ring attempts when a dominant fragment already defines the form.

I will make thresholds adaptive to data volume. If there are only one or two rim candidates, I will relax maximum-gap requirements while keeping RMS strict, and I will log that I accepted a partial rim on limited evidence. If there are at least three candidates with good coverage, I will require a <5 mm RMS and <10 mm maximum chord gap to declare a closed ring; these defaults reflect the kind of tight rim fits reported in axis-aware reassembly and are configurable. 
CVF Open Access

I will fold these validators into classification. Rather than a hard “top/bottom 20%” rule, I will compute a per-piece probability of being RIM or BASE by combining height with the three detectors above (primitives, smoothness/tangency, thickness trend). A piece that is in the top height band but fails both the primitive fit and the smoothness check will be treated as BODY for the purpose of ring phases, which avoids forcing bad ring attempts. This follows how SfS/SfS++ fuse axis-aware cues to stabilise assembly rather than trusting any single signal. 
CVF Open Access
+1

Non-functional requirements

All checks must add less than five per cent runtime on the current eight-piece case and less than ten per cent on medium components. Fits must be robust (Huber or RANSAC) so a few chipped points do not dominate. All thresholds and weights live in config and appear in logs. Results must be reproducible with a fixed random seed in RANSAC.

Design and interfaces

I will add a RimBaseEvidence module with small, testable functions. fitRimPrimitive() will return centre, radius or a simple conical R(h), RMS, inlier ratio and angular coverage for a boundary edge; fitBasePrimitive() will return plane parameters and an optional annulus with residuals; edgeSmoothnessAndTangency() will return curvature noise and the angle between edge tangents and the axis; thicknessTrend() will return a signed trend score and correlation. All four return confidences and residuals. These flow into a classifyWithEvidence() function that fuses height and evidence into per-piece probabilities and into a preValidateRing() stage that can mark a ring “complete with one piece” or “insufficient evidence”. I will expose a validateCircularClosure() that runs only after preValidateRing() succeeds, so ring phases never start on obviously bad inputs.

I will keep the primitive fits close to standard libraries. PCL’s SampleConsensusModelCircle3D and plane models cover the core, and pyRANSAC-3D gives a pure-Python fallback or a reference for tests. Both have public documentation and examples, which means these validators are easy to understand and reproduce. 
PyPI
+3
Point Cloud Library
+3
GitLab
+3

Data and configuration

I will store vessel-level rim and base models as first-class state: for the rim, a circle (or linear radius-with-height) plus covariance and accepted height; for the base, a plane plus optional annulus and covariance. Configurable knobs include rim RMS in millimetres (default 5), maximum chord gap (default 10), minimum angular coverage to declare completion (default 0.8 of 2π for single-piece sufficiency), plane RMS for base acceptance, annulus thickness bounds when a foot-ring is suspected, and thresholds for smoothness, tangency and thickness trend.

Telemetry

For each candidate edge I will log primitive residuals (RMS, inliers, coverage), smoothness/tangency scores, thickness trend statistics, and the combined confidence. For each vessel I will log whether single-piece sufficiency triggered and, if so, which fragment did it. For every accepted ring I will log the final fit metrics; for every rejected attempt I will log which validator failed first. This makes debugging straightforward and gives me a clean audit trail.

Acceptance criteria

On the established eight-piece dataset, I expect the rim or base to be marked complete with a single piece when coverage exceeds eighty per cent and RMS is below threshold, with no ring phase attempted. Where multiple rim/base pieces exist, I expect closed rings only when the <5 mm/<10 mm criteria are met; otherwise the ring remains open with a clear reason. On medium components, I expect fewer ring attempts started and a reduction in failed ring fits, with no increase in collisions after global refine. I will also run an ablation: disabling each detector in turn should worsen either precision (more false rings) or recall (missed valid rings) relative to the full validator.

Risks and mitigations

Bad axis estimates can degrade circle fits; I will allow one axis refresh from high-confidence rim points before final validation, which is standard practice in axis-aware reassembly. Foot-rings are not present on all wares; the annulus fit is optional and never blocks a good plane. Small fragments can pass smoothness but lack coverage; I will treat coverage as a first-class metric to avoid over-interpreting short arcs. If performance drifts, I will subsample boundary points for primitive fits and cap RANSAC iterations. 
CVF Open Access

Rollout and testing

I will guard the new validators behind a feature flag and add unit tests that feed synthetic circles, planes and annuli with controlled noise into the fitters. I will test on three corpora: the eight-piece clean pot, a chipped-rim vessel where single-piece sufficiency should trigger for the base but not the rim, and a foot-ring ware set where the annulus fit should improve base confidence. I will compare before/after on ring attempts started, ring accepts/rejects with metrics, and end-to-end assembly stability.