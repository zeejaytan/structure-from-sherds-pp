#pragma once

#include "data_structure.h"
#include "global_connectivity_engine.h"
#include "surface_overlap_detector.h"
#include "rim_base_evidence.h"
#include <vector>
#include <unordered_set>
#include <memory>
#include <map>
#include <queue>
#include <set>

/**
 * HYBRID PuzzleFusion++ Global Optimization System
 *
 * Elegant hybrid approach combining:
 * 1. Matrix-based exploration for beam search efficiency
 * 2. Real geometry validation for accurate assembly measurement
 * 3. Proven ICP transformations as the only source of movement
 * 4. Legacy geometry functions for reliable spatial validation
 *
 * Architecture:
 * - Exploration Phase: Fast matrix operations for hypothesis generation
 * - Validation Phase: Real geometry testing for top candidates
 * - Selection Phase: Choose based on actual assembly quality
 */

// Forward declarations
class GeometryValidator;
class AssemblyScorer;

/**
 * Proven connection from successful ICP
 * Immutable - represents validated edge alignment
 */
struct ProvenConnection {
    int piece_a, piece_b;                    // Connected pieces (1-based)
    Trans icp_transformation;               // Original ICP result
    double geometric_error;                 // ICP score (lower = better)
    int inlier_count;                      // Edge alignment quality (higher = better)
    double combined_quality;               // Computed quality metric

    // Breakline segment indices for edge competition detection
    int start_a, end_a;                     // Breakline segment on piece_a
    int start_b, end_b;                     // Breakline segment on piece_b

    ProvenConnection(const LCSIndex& lcs)
        : piece_a(lcs.shard_x_), piece_b(lcs.shard_y_)
        , icp_transformation(lcs.trans_), geometric_error(lcs.score_)
        , inlier_count(lcs.inliner_)
        , start_a(lcs.start_.x), end_a(lcs.end_.x)
        , start_b(lcs.start_.y), end_b(lcs.end_.y) {
        // Compute combined quality: 90% edge alignment, 10% geometric accuracy
        // PRIORITIZE EDGE ALIGNMENT: Pottery assembly should prioritize touching edges over geometric error
        double edge_quality = std::min(100.0, static_cast<double>(inlier_count) / 2.0);
        double geom_quality = 100.0 / (1.0 + geometric_error);
        combined_quality = 0.9 * edge_quality + 0.1 * geom_quality;
    }

    bool operator>(const ProvenConnection& other) const {
        return combined_quality > other.combined_quality;
    }
};

/**
 * Hybrid Assembly State
 * Maintains both matrix representations (for exploration) and real validation results
 */
struct HybridAssemblyState {
    std::vector<int> active_pieces;                           // Pieces in assembly (1-based)
    std::vector<ProvenConnection> selected_connections;       // Chosen ICP connections
    std::map<int, Matrix4d> piece_world_transforms;          // Matrix positions for exploration

    // Quality metrics
    double matrix_score;                                     // Fast matrix-based score
    double validated_score;                                  // Real geometry score (-1 if not validated)
    bool has_real_validation;                               // Whether real validation performed

    // Assembly metrics
    int total_pieces;
    double average_connection_quality;

    HybridAssemblyState() : matrix_score(0.0), validated_score(-1.0), has_real_validation(false) {}

    // Comparison for beam search (prefer validated scores)
    bool operator>(const HybridAssemblyState& other) const {
        if (has_real_validation && other.has_real_validation) {
            return validated_score > other.validated_score;
        }
        if (has_real_validation && !other.has_real_validation) {
            return true;  // Validated always beats unvalidated
        }
        if (!has_real_validation && other.has_real_validation) {
            return false;
        }
        return matrix_score > other.matrix_score;  // Both unvalidated, use matrix score
    }
};

/**
 * Configuration for hybrid optimization
 */
struct HybridOptimizationConfig {
    // Beam search parameters
    int beam_width = 8;                              // Number of states to maintain
    int max_iterations = 25;                         // Maximum optimization iterations

    // Validation parameters
    int top_candidates_for_validation = 3;           // How many states to validate per iteration
    double real_geometry_threshold = 50.0;          // Max distance for real assembly (mm)

    // Quality thresholds
    double minimum_connection_quality = 1.0;         // Minimum quality to consider connection
    double convergence_threshold = 1e-3;            // Score improvement threshold

    // Weights for scoring
    double spatial_weight = 0.8;                     // Edge alignment importance
    double connection_weight = 0.2;                  // Geometric error importance

    // Connection replacement (critical for escaping local optima)
    bool enable_connection_replacement = true;       // Allow replacing early connection choices

    bool enable_logging = true;                      // Debug output
};

/**
 * PHASE 3+ HIERARCHICAL ARCHAEOLOGICAL ASSEMBLY
 * Configuration for mandatory build order, closure validation, and progressive gap filling
 */
struct ArchaeologicalConfig {
    // Ring closure thresholds
    double rim_rms_threshold = 5.0;              // mm - RMS distance to circle for rim acceptance
    double rim_max_gap_threshold = 10.0;         // mm - Max chord gap between consecutive rim pieces
    double base_rms_threshold = 5.0;             // mm - RMS distance to circle for base acceptance
    double base_max_gap_threshold = 10.0;        // mm - Max chord gap between consecutive base pieces
    double min_angular_coverage = 0.90;          // 90% - Minimum angular coverage to accept ring

    // Gap filling parameters
    double gap_reduction_fraction = 0.20;        // 20% - Minimum gap reduction required for productive placement
    double min_gap_angle = 30.0;                 // degrees - Minimum angular gap to track
    int height_bands = 8;                        // Number of vertical bands for corridor gaps

    // PHASE 2: Gap-targeting scheduler
    double gap_targeting_frequency = 0.8;        // 80% - Fraction of iterations using gap-targeting (vs best-overall)

    // PHASE 3: Hub prevention
    int ring_degree = 2;                         // Fixed - Exact degree for ring pieces (rim/base)
    int max_degree_body = 3;                     // Soft limit - Maximum degree for body pieces
    double degree_penalty_weight = 0.3;          // 30% - Penalty strength for exceeding degree limit

    // PHASE 4: Circular topology bonuses
    double ring_closure_bonus = 1.0;             // 100% - One-time bonus when ring passes closure thresholds
    double arc_fill_bonus_scale = 0.5;           // 50% - Maximum bonus for filling large missing arcs
    double dense_section_penalty = 0.2;          // 20% - Penalty for connecting to already-dense sections
    double arc_density_angle = 60.0;             // degrees - Angle range to check for local density

    // PHASE 5: Pottery-aware validation
    double height_jump_mm = 50.0;                // mm - Maximum height difference across connection (increased from 8mm to allow valid body connections)
    double thickness_corr_min = 0.6;             // Minimum Pearson correlation for wall thickness
    double thickness_diff_max = 3.0;             // mm - Maximum absolute thickness difference
    double curvature_flip_deg = 25.0;            // degrees - Maximum normal angle change

    // RIM/BASE VALIDATION (PRD_Rimbase.md)
    // Primitive fitting thresholds
    double rim_primitive_rms_threshold = 5.0;    // mm - Max RMS for valid rim circle fit
    double base_primitive_rms_threshold = 5.0;   // mm - Max RMS for valid base plane fit
    double min_inlier_ratio = 0.7;               // [0,1] - Minimum fraction of inlier points

    // Single-piece sufficiency
    double min_coverage_for_completion = 0.8;    // [0,1] - 80% angular coverage = complete ring

    // Smoothness and tangency
    double max_curvature_noise = 0.5;            // Normalized curvature std dev threshold
    double max_tangency_angle = 15.0;            // degrees - Max deviation from perpendicular to axis

    // Thickness trends
    double thickness_gradient_threshold = 0.6;   // Pearson correlation for thickness trend

    // Foot-ring detection (base)
    double foot_ring_min_thickness = 5.0;        // mm - Minimum annulus radial thickness
    double foot_ring_max_thickness = 20.0;       // mm - Maximum annulus radial thickness

    // Adaptive thresholds
    int min_candidates_for_strict = 3;           // Relax gap thresholds if fewer rim/base pieces
    double relaxed_max_gap = 50.0;               // mm - Relaxed gap threshold for 1-2 pieces

    // Angular coverage computation (PRD_Rimbase.md fix for false sufficiency)
    double angular_gap_threshold = 30.0;         // degrees - Max gap to consider arc continuous

    // Edge extraction from surfaces (PRD_Rimbase.md Phase 6: Correct edge identification)
    double rim_edge_tolerance = 5.0;             // mm - Vertical tolerance for rim edge extraction
    double base_edge_tolerance = 5.0;            // mm - Vertical tolerance for base edge extraction
    double edge_radial_min_ratio = 0.3;          // Min radius fraction for valid edge points
    double edge_radial_max_ratio = 0.7;          // Max radius fraction for valid edge points
    bool enable_surface_edge_extraction = true;  // Use surface data instead of breaklines

    // Phase control
    int max_ring_attempts = 50;                  // Bailout threshold for ring completion loops
    bool allow_partial_rings = true;             // Proceed with body even if rings incomplete
    bool enable_size_prioritization = true;      // Prioritize larger pieces within phases

    // Logging
    bool enable_hierarchical_logging = true;     // Detailed phase transition logs
};

/**
 * Circle fit result with closure validation
 * Used to validate rim/base rings meet physical closure constraints
 */
struct CircleFitResult {
    Eigen::Vector3d center;                      // Circle center in 3D space
    Eigen::Vector3d normal;                      // Plane normal (axis direction)
    double radius;                               // Fitted circle radius (mm)
    double rms_error;                            // RMS distance from points to circle (mm)
    double max_gap;                              // Maximum chord gap between consecutive pieces (mm)
    double angular_coverage;                     // Fraction of 360° covered [0,1]
    int num_pieces;                              // Number of pieces in ring
    bool passes_closure;                         // true if meets all thresholds

    CircleFitResult()
        : center(Eigen::Vector3d::Zero()), normal(Eigen::Vector3d::UnitZ())
        , radius(0.0), rms_error(0.0), max_gap(0.0), angular_coverage(0.0)
        , num_pieces(0), passes_closure(false) {}

    // Validation against config thresholds
    bool validateClosure(const ArchaeologicalConfig& config, bool is_rim) const {
        double rms_threshold = is_rim ? config.rim_rms_threshold : config.base_rms_threshold;
        double gap_threshold = is_rim ? config.rim_max_gap_threshold : config.base_max_gap_threshold;

        return (rms_error < rms_threshold &&
                max_gap < gap_threshold &&
                angular_coverage >= config.min_angular_coverage);
    }
};

/**
 * Gap in assembly to be filled during progressive body placement
 * Tracks both arc gaps (rim/base rings) and vertical corridors (body)
 */
struct Gap {
    enum Type { RIM_ARC, BASE_ARC, VERTICAL_CORRIDOR };

    Type type;                                   // Gap classification
    int piece_a, piece_b;                        // Bounding pieces (for arc gaps)
    double angular_width;                        // Angular span in degrees (for arc gaps)
    double height_range[2];                      // Height bounds [min, max] in mm (for vertical gaps)
    double metric;                               // Gap size metric (angular width or vertical span)
    Eigen::Vector3d center_direction;            // Unit vector from circle center (for targeting)

    Gap() : type(RIM_ARC), piece_a(-1), piece_b(-1), angular_width(0.0), metric(0.0)
        , center_direction(Eigen::Vector3d::Zero()) {
        height_range[0] = 0.0;
        height_range[1] = 0.0;
    }

    Gap(Type t, int a, int b, double angle_or_height)
        : type(t), piece_a(a), piece_b(b), angular_width(0.0), metric(angle_or_height)
        , center_direction(Eigen::Vector3d::Zero()) {
        if (t == RIM_ARC || t == BASE_ARC) {
            angular_width = angle_or_height;
        } else {
            height_range[0] = 0.0;  // Will be set by extractGaps
            height_range[1] = angle_or_height;
        }
    }

    // Comparison for priority queue (largest gap first)
    bool operator<(const Gap& other) const {
        return metric < other.metric;
    }
};

/**
 * Gap List: Tracks and manages gaps during progressive assembly
 * Maintains live gap inventory for rim arcs, base arcs, and vertical corridors
 */
class GapList {
public:
    GapList() = default;

    // Extract initial gaps from ring (rim or base)
    void extractRingGaps(
        const std::vector<int>& ring_pieces,
        const HybridAssemblyState& state,
        const CircleFitResult& circle_fit,
        Gap::Type gap_type,
        const ArchaeologicalConfig& config);

    // Extract vertical corridor gaps between rim and base
    void extractVerticalGaps(
        const std::vector<int>& rim_pieces,
        const std::vector<int>& base_pieces,
        const HybridAssemblyState& state,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config);

    // Update gaps after placing a new piece
    void updateGapsAfterPlacement(
        int placed_piece,
        const HybridAssemblyState& new_state,
        const std::vector<Geom>& geometry);

    // Get largest gap for targeting
    Gap getLargestGap() const;

    // Check if specific gap is addressed by piece placement
    bool doesPieceFillGap(
        int piece_id,
        const Gap& gap,
        const HybridAssemblyState& state,
        const std::vector<Geom>& geometry,
        double& reduction_amount) const;

    // Query gap state
    int getGapCount() const { return gaps_.size(); }
    double getTotalGapMetric() const;
    const std::vector<Gap>& getAllGaps() const { return gaps_; }

    // Logging
    void logGapSnapshot(const std::string& phase_name) const;

private:
    std::vector<Gap> gaps_;

    // Helper: Calculate angular position of piece around circle
    double calculatePieceAngle(
        int piece_id,
        const Eigen::Vector3d& circle_center,
        const Eigen::Vector3d& circle_normal,
        const HybridAssemblyState& state,
        const std::vector<Geom>& geometry) const;

    // Helper: Sort pieces by angular order around circle
    std::vector<int> sortPiecesByAngle(
        const std::vector<int>& pieces,
        const Eigen::Vector3d& circle_center,
        const Eigen::Vector3d& circle_normal,
        const HybridAssemblyState& state,
        const std::vector<Geom>& geometry) const;
};

/**
 * Hierarchical Assembly State Machine (Phase 3+)
 * Enforces mandatory build order: RIM → BASE → BODY → COMPLETE
 */
enum class AssemblyPhase {
    INITIALIZATION,      // Initial setup, no constraints
    RIM_COMPLETION,      // Must complete rim ring (or reach bailout)
    BASE_COMPLETION,     // Must complete base ring (or reach bailout)
    BODY_FILLING,        // Progressive gap filling with body pieces
    COMPLETE             // Assembly finished
};

// Piece type classification for hierarchical assembly
enum class PieceType { RIM, BASE, BODY, UNCLASSIFIED };

struct HierarchicalAssemblyState {
    AssemblyPhase current_phase;

    // Rim ring state
    std::vector<int> rim_pieces;
    CircleFitResult rim_circle;
    bool rim_complete;
    int rim_attempts;

    // Base ring state
    std::vector<int> base_pieces;
    CircleFitResult base_circle;
    bool base_complete;
    int base_attempts;

    // Gap filling state
    GapList gap_list;
    std::vector<int> placed_pieces;  // All pieces placed so far

    // Piece classification
    std::map<int, PieceType> piece_types;  // piece_id -> RIM/BASE/BODY

    // PHASE 3: Hub prevention - degree tracking
    std::map<int, int> piece_degree_map;  // piece_id -> current degree (number of connections)

    HierarchicalAssemblyState()
        : current_phase(AssemblyPhase::INITIALIZATION)
        , rim_complete(false), rim_attempts(0)
        , base_complete(false), base_attempts(0) {}

    // Phase transition logic
    bool canTransitionToNextPhase(const ArchaeologicalConfig& config) const {
        switch (current_phase) {
            case AssemblyPhase::INITIALIZATION:
                return true;  // Always transition to rim completion

            case AssemblyPhase::RIM_COMPLETION:
                // Can transition if rim complete OR bailout reached
                return rim_complete ||
                       rim_attempts >= config.max_ring_attempts ||
                       (config.allow_partial_rings && rim_attempts > 0);

            case AssemblyPhase::BASE_COMPLETION:
                // Can transition if base complete OR bailout reached
                return base_complete ||
                       base_attempts >= config.max_ring_attempts ||
                       (config.allow_partial_rings && base_attempts > 0);

            case AssemblyPhase::BODY_FILLING:
                // Transition when all pieces placed or no more productive moves
                return gap_list.getGapCount() == 0;

            case AssemblyPhase::COMPLETE:
                return false;  // Terminal state
        }
        return false;
    }

    AssemblyPhase getNextPhase() const {
        switch (current_phase) {
            case AssemblyPhase::INITIALIZATION:
                return AssemblyPhase::RIM_COMPLETION;
            case AssemblyPhase::RIM_COMPLETION:
                return AssemblyPhase::BASE_COMPLETION;
            case AssemblyPhase::BASE_COMPLETION:
                return AssemblyPhase::BODY_FILLING;
            case AssemblyPhase::BODY_FILLING:
                return AssemblyPhase::COMPLETE;
            case AssemblyPhase::COMPLETE:
                return AssemblyPhase::COMPLETE;
        }
        return AssemblyPhase::COMPLETE;
    }
};

/**
 * Geometry Validator: Handles real point cloud operations
 * Encapsulates all legacy geometry functions for clean interface
 */
class GeometryValidator {
    friend class HybridPuzzleFusionOptimizer;  // Allow optimizer to access geometry for curvature validation
public:
    explicit GeometryValidator(const std::vector<Geom>& original_geometry);

    // Apply assembly state to geometry and measure real quality
    double validateAssemblyState(const HybridAssemblyState& state);

    // Measure real distance between two pieces after transformation
    double measureRealEdgeDistance(int piece_a, int piece_b, const HybridAssemblyState& state);

    // ENHANCED: Measure surface-based overlap between pieces (pottery-appropriate)
    bool measureRealSurfaceOverlap(int piece_a, int piece_b, const HybridAssemblyState& state, double& overlap_volume);

    // Check if assembly has geometric conflicts (overlaps, etc.)
    bool hasGeometricConflicts(const HybridAssemblyState& state);

    // Calculate expected vessel radius from piece curvature data
    double calculateVesselRadiusFromCurvature(const std::vector<int>& piece_ids);

private:
    std::vector<Geom> reference_geometry_;           // Original unchanged geometry
    std::vector<Geom> working_geometry_;             // Temporary for validation

    void applyStateToGeometry(const HybridAssemblyState& state);
    void restoreOriginalGeometry();
    double calculateRealAssemblyQuality(const HybridAssemblyState& state);
};

/**
 * Assembly Scorer: Handles quality calculations
 * Separates matrix-based and real-based scoring logic
 */
class AssemblyScorer {
public:
    explicit AssemblyScorer(const HybridOptimizationConfig& config, GeometryValidator* validator = nullptr);

    // Fast matrix-based scoring for exploration
    double calculateMatrixScore(const HybridAssemblyState& state) const;

    // Comprehensive scoring including real validation
    double calculateFinalScore(const HybridAssemblyState& state) const;

private:
    HybridOptimizationConfig config_;
    GeometryValidator* validator_;  // For geometry-based overlap detection

    double calculateConnectionQuality(const std::vector<ProvenConnection>& connections) const;
    double calculateCompletenessBonus(const HybridAssemblyState& state) const;
    double calculateVesselCoherence(const HybridAssemblyState& state) const;  // LEGACY vessel validation
    double calculateClusterCoherencePenalty(const HybridAssemblyState& state) const;  // Edge competition penalty
};

/**
 * Main Hybrid PuzzleFusion++ Optimizer
 * Elegant three-phase architecture: Explore -> Validate -> Select
 */
class HybridPuzzleFusionOptimizer {
public:
    explicit HybridPuzzleFusionOptimizer(const HybridOptimizationConfig& config = HybridOptimizationConfig());

    /**
     * Main optimization entry point
     * Returns best assembly using hybrid matrix-real approach
     */
    HybridAssemblyState optimizeGlobalAssembly(
        int total_pieces,
        const std::vector<LCSIndex>& all_connections,
        const std::vector<Geom>& geometry);

private:
    HybridOptimizationConfig config_;
    std::unique_ptr<GeometryValidator> validator_;
    std::unique_ptr<AssemblyScorer> scorer_;

    // Phase 1: Matrix-based exploration
    std::vector<HybridAssemblyState> generateInitialStates(
        const std::vector<ProvenConnection>& proven_connections,
        const std::vector<Geom>& geometry);

    void exploreAssemblyStates(
        std::vector<HybridAssemblyState>& states,
        const std::vector<ProvenConnection>& all_connections);

    void expandSingleState(
        const HybridAssemblyState& current_state,
        const std::vector<ProvenConnection>& all_connections,
        std::vector<HybridAssemblyState>& new_states);

    void generateReplacementStates(
        const std::vector<HybridAssemblyState>& current_states,
        const std::vector<ProvenConnection>& all_connections,
        std::vector<HybridAssemblyState>& new_states);

    // Phase 2: Real geometry validation
    void validateTopCandidates(std::vector<HybridAssemblyState>& states);

    // Phase 3: State management and selection
    void pruneBeamStates(std::vector<HybridAssemblyState>& states);
    bool hasConverged(const std::vector<HybridAssemblyState>& states) const;

    // Graph topology validation
    bool isValidTreeStructure(const HybridAssemblyState& state) const;
    int findRootPiece(const HybridAssemblyState& state) const;

    // Archaeological assembly - Phase 1: Anchor identification
    int identifyStructuralAnchor(const std::vector<ProvenConnection>& connections) const;

    // Archaeological assembly - Phase 2: Height and angle extraction
    double getHeight(int piece_id, const std::vector<Geom>& geometry) const;
    double getAngle(int piece_id, const std::vector<Geom>& geometry) const;

    // Archaeological assembly - Phase 3: Rim/base detection and hierarchical assembly
    struct PieceClassification {
        int piece_id;
        enum Type { RIM, BASE, BODY } type;
        double height_on_axis;
        double confidence;
    };
    std::vector<PieceClassification> classifyPiecesByStructure(const std::vector<Geom>& geometry) const;
    std::vector<int> identifyRimPieces(const std::vector<PieceClassification>& classifications) const;
    std::vector<int> identifyBasePieces(const std::vector<PieceClassification>& classifications) const;

    // PRD_Rimbase.md Phase 6: Edge extraction from surfaces (not breaklines)
    struct EdgeExtractionResult {
        std::vector<Eigen::Vector3d> edge_points;
        std::vector<Eigen::Vector3d> edge_normals;
        double edge_height;          // Z coordinate of extracted edge
        int num_points;
        bool is_valid;

        EdgeExtractionResult() : edge_height(0.0), num_points(0), is_valid(false) {}
    };

    EdgeExtractionResult extractRimEdge(
        const Geom& geom,
        const Eigen::Vector3d& axis_pos,
        double axis_height,
        const ArchaeologicalConfig& config) const;

    EdgeExtractionResult extractBaseEdge(
        const Geom& geom,
        const Eigen::Vector3d& axis_pos,
        double axis_height,
        const ArchaeologicalConfig& config) const;

    // PRD_Rimbase.md Phase 6: Geometry-based rim/base identification
    // Uses actual edge geometry (circularity, smoothness, tangency) to identify rim/base pieces
    // instead of relying solely on height classification
    struct GeometricRimBaseResult {
        std::vector<int> rim_pieces;        // Pieces with rim-like geometry
        std::vector<int> base_pieces;       // Pieces with base-like geometry
        std::map<int, double> confidence;   // piece_id -> confidence [0,1]
    };

    GeometricRimBaseResult identifyRimBasePiecesByGeometry(
        int total_pieces,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config) const;

    // PotSAC-based classification using pre-computed axis heights
    // Adaptive algorithm handles rim+base only, rim+body+base, and other configurations
    struct PotSACClassification {
        std::vector<int> rim_pieces;        // Pieces in top 33% of height range
        std::vector<int> base_pieces;       // Pieces in bottom 33% of height range
        std::vector<int> body_pieces;       // Pieces in middle 33% of height range
        double rim_threshold;               // Height threshold for rim (mm)
        double base_threshold;              // Height threshold for base (mm)
        double min_height, max_height;      // Height range (mm)
    };

    PotSACClassification classifyByPotSACHeights(
        int total_pieces,
        const std::vector<Geom>& geometry) const;

    // Helper functions for circular ring assembly (chain-building algorithm)
    std::map<int, int> getConnectionDegrees(
        const std::vector<int>& active_pieces,
        const std::vector<std::pair<int, int>>& connections) const;

    std::vector<int> getChainEnds(
        const std::map<int, int>& degrees) const;

    double computeAngularDistance(
        int piece_a, int piece_b,
        const std::vector<Geom>& geometry) const;

    // PRD_Rimbase.md: Evidence-based rim/base classification helpers (legacy)
    std::vector<Eigen::Vector3d> extractEdgePoints(const Geom& geom) const;
    std::vector<Eigen::Vector3d> extractEdgeNormals(const Geom& geom) const;
    std::vector<double> extractThicknessValues(const Geom& geom) const;
    Eigen::Vector3d extractAxisNormal(const Geom& geom) const;

    // Evidence fusion functions
    double fuseEvidenceForRim(
        double height_prior,
        const PrimitiveEvidence& prim,
        const SmoothnessEvidence& smooth,
        const ThicknessTrendEvidence& thick) const;

    double fuseEvidenceForBase(
        double height_prior,
        const PrimitiveEvidence& prim,
        const SmoothnessEvidence& smooth,
        const ThicknessTrendEvidence& thick) const;

    // Single-piece sufficiency detection
    struct SufficiencyResult {
        bool is_sufficient;
        int piece_id;
        double coverage;
        double rms_error;
        std::string reason;

        SufficiencyResult() : is_sufficient(false), piece_id(-1), coverage(0.0), rms_error(999.0) {}
    };

    SufficiencyResult checkRimSufficiency(
        const std::vector<int>& rim_pieces,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config) const;

    SufficiencyResult checkBaseSufficiency(
        const std::vector<int>& base_pieces,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config) const;

    // Adaptive threshold adjustment (PRD_Rimbase.md Phase 4)
    // Relaxes gap thresholds for data-scarce scenarios (1-2 pieces) while keeping RMS strict
    void adaptThresholdsToDataVolume(
        ArchaeologicalConfig& config,
        int num_rim_candidates,
        int num_base_candidates) const;

    // Circle topology validation
    bool validateCircularTopology(const std::vector<int>& pieces,
                                   const std::vector<ProvenConnection>& connections,
                                   double closure_tolerance = 5.0) const;

    // Bridge quality scoring for progressive gap filling
    double calculateBridgeQuality(int candidate_piece,
                                  int anchor_a, int anchor_b,
                                  const HybridAssemblyState& current_state,
                                  const std::vector<ProvenConnection>& connections,
                                  const std::vector<Geom>& geometry) const;

    // Enhanced bridge quality with gap targeting (Phase 3+)
    double calculateBridgeQualityWithGapTargeting(
        int candidate_piece,
        int anchor_a, int anchor_b,
        const HybridAssemblyState& current_state,
        const std::vector<ProvenConnection>& connections,
        const std::vector<Geom>& geometry,
        const GapList& gap_list,
        const ArchaeologicalConfig& config) const;

    // Archaeological assembly - Phase 3+: Circle fitting and closure validation
    CircleFitResult fitAndValidateCircle(
        const std::vector<int>& ring_pieces,
        const HybridAssemblyState& state,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config,
        bool is_rim) const;

    // Archaeological assembly - Phase 3+: Hierarchical assembly orchestration
    HybridAssemblyState performHierarchicalAssembly(
        int total_pieces,
        const std::vector<ProvenConnection>& all_connections,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config);

    // Helper functions for hierarchical assembly
    void classifyPiecesByHeight(
        HierarchicalAssemblyState& hier_state,
        int total_pieces,
        const std::vector<Geom>& geometry);

    bool attemptRimRingCompletion(
        HierarchicalAssemblyState& hier_state,
        HybridAssemblyState& assembly_state,
        const std::vector<ProvenConnection>& connections,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config);

    bool attemptBaseRingCompletion(
        HierarchicalAssemblyState& hier_state,
        HybridAssemblyState& assembly_state,
        const std::vector<ProvenConnection>& connections,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config);

    void performBodyGapFilling(
        HierarchicalAssemblyState& hier_state,
        HybridAssemblyState& assembly_state,
        const std::vector<ProvenConnection>& connections,
        const std::vector<Geom>& geometry,
        const ArchaeologicalConfig& config);

    std::vector<int> getSizeOrderedCandidates(
        const std::vector<int>& piece_ids,
        const std::vector<Geom>& geometry) const;

    // Utility functions
    std::vector<ProvenConnection> convertToProvenConnections(const std::vector<LCSIndex>& connections,
                                                             const std::vector<Geom>& geometry);
    Matrix4d calculateWorldTransform(int piece, const HybridAssemblyState& state,
                                   const ProvenConnection& new_connection);

    void logOptimizationProgress(int iteration, const std::vector<HybridAssemblyState>& states) const;

    // CLUSTER COHERENCE VALIDATION: Multi-piece geometric consistency
    // Adjusts connection quality based on whether connections form geometrically coherent triangles/clusters
    // Boosts connections that mutually support each other, penalizes isolated high-inlier connections
    std::vector<ProvenConnection> adjustConnectionQualityByClusterCoherence(
        const std::vector<ProvenConnection>& connections,
        int total_pieces);

    // Helper: Check if transformation cycle T_AB * T_BC ≈ T_AC for triangle {A,B,C}
    bool checkTransformationCycleConsistency(
        const ProvenConnection& conn_ab,
        const ProvenConnection& conn_bc,
        const ProvenConnection& conn_ac,
        double tolerance = 10.0) const;  // 10mm tolerance for pottery

    // EDGE COMPETITION CHECK: Detect if two connections compete for same physical edge
    // Returns true if triangle is geometrically impossible due to edge overlap
    bool checkEdgeCompetition(
        const ProvenConnection& conn_ab,
        const ProvenConnection& conn_bc,
        const ProvenConnection& conn_ac) const;

    // CURVATURE-AWARE TRIANGLE VALIDATION: Check if three pieces form coherent curvature flow
    // Uses breakline curvature data (feature_[6]) to validate pottery vessel structure
    bool validateTriangleCurvatureCoherence(
        int piece_a, int piece_b, int piece_c,
        const ProvenConnection& conn_ab,
        const ProvenConnection& conn_bc,
        const ProvenConnection& conn_ac,
        const std::vector<Geom>& geometry) const;

    // Helper: Find all triangles {A,B,C} where all three pairwise connections exist
    struct Triangle {
        int piece_a, piece_b, piece_c;
        int conn_ab_idx, conn_bc_idx, conn_ac_idx;  // Indices into connection list
        bool is_cycle_consistent;
        double cycle_error;  // Transformation composition error (mm)
    };
    std::vector<Triangle> findCoherentTriangles(
        const std::vector<ProvenConnection>& connections,
        int total_pieces);

    // Helper: Detect competing connections for the same piece
    // Two connections compete if they transform the same piece to DIFFERENT incompatible locations
    // Returns map: piece_id -> list of mutually exclusive connection groups
    struct ConnectionConflict {
        int piece_id;
        std::vector<int> conflicting_conn_indices;  // Connections that compete for this piece
        double spatial_conflict_distance;  // How far apart the competing transformations place the piece
    };
    std::vector<ConnectionConflict> detectCompetingConnections(
        const std::vector<ProvenConnection>& connections,
        int total_pieces);
};

/**
 * Build connected assembly from hybrid optimization result
 * Uses connection topology to ensure spatial connectivity like legacy system
 */
std::map<int, Matrix4d> buildConnectedAssembly(const HybridAssemblyState& result);

/**
 * Integration function for existing codebase
 * Maintains same interface as original while using hybrid approach
 */
RankingSubgraph performHybridPuzzleFusionOptimization(
    int total_pieces,
    std::vector<LCSIndex> all_connections,
    const std::vector<Geom>& shard);

// End of header (using #pragma once)