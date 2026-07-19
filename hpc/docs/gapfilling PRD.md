PRD: Phase 3+ Hierarchical Archaeological Assembly — Next Steps
Overview and intent

I have Phase 3 working: rim and base detection by height, circular topology checks for those rings, and structural bonuses that feed straight into connection ranking and the beam search. The next increment turns those detectors and bonuses into hard-edged behaviour: I will drive assembly in a defined order, fill gaps methodically, and reject ring solutions that do not properly close. This document sets out what I am building next, how it will behave, and how I will know it works.

Problem the next phase solves

Right now the system rewards archaeologically plausible joins, but it still treats the overall pot structure as a side effect of local choices. That means a near-miss rim that doesn’t quite close can sneak through, and body placement can meander rather than bridge real gaps. By adding progressive gap filling, explicit size ordering, strict circular closure tests, and a full hierarchical build plan (rim, then base, then body), I turn archaeological cues into global constraints that shape the whole assembly, not just edge scores.

Goals and non-goals

My goal is to produce complete, connected vessels that honour two simple physical facts: rims and bases are circular structures with small closure error, and large body sherds generally anchor the mid-section before slivers. I am not trying to solve mixed-vessel partitioning in this step; the work here assumes I am operating within a single pot candidate produced by the existing clustering and beam initialisation.

Scope of changes

I will add four behaviours. First, progressive gap filling that uses the bridge quality score to place body pieces specifically where they close measured gaps between established rim and base neighbours. Second, size-based prioritisation so that, within any placement queue, larger fragments are tried earlier without overriding the connection score. Third, explicit circular closure validation that rejects or defers a rim or base ring if the measured closure error is five millimetres or more. Fourth, a strict hierarchical assembly schedule where the solver completes the rim circle, then the base circle, and only then proceeds with progressive filling through the body.

Current state summary

Phase 3 already classifies fragments into rim, base and body by vessel height quantiles. It validates ring-like topology by requiring two neighbours per rim or base piece and accepts partial circles when more than half the circumference is supported. It applies bonuses for rim-to-rim, base-to-base, and vertical links. The beam search consumes these scores indirectly via better edge rankings. What is missing is a mandatory build order, hard validation on ring closure, and a targeted approach to filling the mid-section.

Functional requirements

I want ring construction to be deliberate. When the candidate component contains three or more high-confidence rim pieces, the solver must enter a “rim completion” phase. In that phase it may place only edges that connect rim pieces or close explicit rim gaps, and it must hold back body placements. It leaves this phase only when the rim passes closure validation or when no admissible rim edge remains. The same behaviour applies to the base once the rim is decided: complete the base ring or explicitly record that the base cannot be closed with the current evidence.

Progressive gap filling must operate on a live gap list. After the rim and base phases, the system enumerates gaps as arcs between neighbouring placed pieces on each ring and as vertical corridors between the rim and base. It must score candidate body pieces against specific gaps using the bridge quality function I have already implemented, and it must place the highest scoring bridge that strictly reduces the measured gap length or area. Each placement must trigger a global pose update and a refresh of the gap list.

Size-based prioritisation is advisory rather than absolute. Within any eligible candidate set, larger pieces in terms of surface area or convex hull volume should be tested first, but they must still clear the same geometric thresholds and ring or gap constraints. I will not allow a large but poor fit to displace a smaller, accurate bridge.

Circular closure validation is mandatory. For any proposed rim or base ring, I will fit a circle in the plane orthogonal to the current axis estimate, compute point-to-circle distances for all ring samples, and report both the root-mean-square error and the maximum chord gap between consecutive segments along the circumference. The ring is accepted only if the RMS distance is below five millimetres and the maximum gap is below ten millimetres, with these numbers configurable. If the test fails, the solver must either continue searching ring edges or mark the ring as uncloseable and proceed with a recorded warning.

The hierarchical assembly schedule ties it all together. The system must follow an explicit state machine: detect and attempt rim closure; if successful, fix the rim and attempt base closure; after both rings are either closed or declared uncloseable, perform progressive body filling until no gap can be reduced further. At each transition I will perform a global pose refinement so new constraints propagate to all placed pieces.

Measurement and acceptance

I will consider rim closure successful when the accepted ring meets the five millimetre RMS and ten millimetre maximum gap thresholds, when each ring piece has exactly two ring neighbours, and when the total angular coverage exceeds ninety per cent. I will consider progressive gap filling effective when, run on the same dataset as Phase 3, it reduces the number of unbridged gaps by at least thirty per cent and improves overall connected surface area without increasing the collision count. Size ordering is acceptable when, holding all other settings constant, the median number of ICP attempts to reach a full assembly is reduced, or the wall-clock time to the same completeness drops by at least ten per cent.

Algorithmic design notes

Gap extraction works on the existing graph. For the rim and base I will project ring edges into the orthogonal plane, sort endpoints by angle around the circle centre, and compute angular gaps from one neighbour to the next. I will maintain a small structure that records, for each gap, its angular width and the identities of its bounding pieces. For the vertical mid-section I will bin the vessel height into a small number of bands and measure empty columns between placed geometry by radial coverage; a “corridor” is any band-pair where the covered radial arcs do not overlap for a significant angle.

Bridge scoring already combines average connection quality, triangle consistency, piece size and vertical coherence. I will reuse that scalar and add an explicit penalty if a candidate does not lie predominantly within the angular span of the target gap. I will also require that the candidate’s placement reduces the gap metric by a fixed fraction; otherwise I mark it as non-productive and move to the next.

Ring closure validation relies on robust circle fitting. I will use a Huber-weighted least squares fit so a small number of chipped lip points do not dominate. The axis estimate may move slightly as the rim stabilises; I will allow one axis refresh after a provisional rim fit and re-validate the circle once before accepting it as closed.

Data and configuration

I will add three small bits of state. First, a vessel-level ring model for rim and base that stores the circle centre, radius, height and covariance of the fit. Second, a maintained list of gaps as described above. Third, a simple queue object that orders candidates by size within any phase. I will expose configuration for the ring RMS and max-gap thresholds, the minimum angular coverage to accept a ring, the fraction by which a body placement must reduce a gap, and the weights already used in the bridge scorer.

Interfaces and integration

I will add a phase controller that wraps the existing optimiser. It will present a simple entry point that takes a candidate component and returns an assembly with rim and base flags, a list of accepted bridges, and any uncloseable ring diagnostics. Internally it will call out to four helpers: ring completion, base completion, gap enumeration, and progressive filling. I will not change the public signature of the global optimiser; I will pass in ring models and gap targets through the existing constraint and weighting hooks.

Telemetry and logging

I will log three classes of events. Ring attempts and outcomes with fit errors and thresholds. Gap list snapshots before and after each placement with the measured reduction. Candidate evaluation summaries that record the top ten bridges considered at each step and their scores and reasons for rejection. These logs will let me explain any assembly and spot cases where the rules are too strict or too loose.

Performance and resource targets

Ring fitting and gap extraction are lightweight compared with ICP and global refinement. My budget is five per cent overhead on current runs for medium components of up to a few dozen pieces. I will keep circle and plane fits to a single robust pass and cap the number of gap candidates evaluated per iteration. If I see slowdowns, I will sample ring points and reduce gap bins.

Risks and mitigations

The main risk is over-constraining awkward vessels with damaged rims or incomplete bases. To reduce false rejections, I will treat thresholds as soft during exploration and only harden them at acceptance time. Another risk is starving the body when a ring never quite closes; to avoid dead ends I will allow the controller to bail out of a ring phase after a fixed number of non-improving attempts and proceed with body filling while keeping the partial ring constraints as soft guides.

Rollout and testing

I will start with a feature flag that forces the hierarchical schedule only on a test suite. I will include three datasets: a clean eight-piece pot like the one already reported, a chipped-rim vessel with known partial closure, and a coarse-ware base with no foot ring. I will record pre- and post-metrics for ring closure error, number of gaps remaining, connected surface area, collision count, and time to completion. I will accept the change when the clean pot passes the ring thresholds, the chipped pot defers ring closure but still fills the body usefully, and no dataset shows an increase in collisions.