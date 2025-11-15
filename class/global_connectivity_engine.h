#ifndef GLOBAL_CONNECTIVITY_ENGINE_H
#define GLOBAL_CONNECTIVITY_ENGINE_H

#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <queue>
#include <memory>
#include <string>
#include <chrono>
#include "ranking_system.h"
#include "connectivity_optimizer.h"
#include "intersection_detector.h"

// Forward declarations
class State;
class RankingSubgraph;
struct LCSIndex;

/**
 * Global Graph Connectivity Enhancement (GGCE) System
 *
 * This system addresses the fundamental 3-graph fragmentation problem by introducing
 * post-assembly analysis and inter-graph merging capabilities.
 *
 * Architecture:
 * 1. Two-phase processing: Enhanced local assembly + Global connectivity resolution
 * 2. Separation of concerns: Local optimization vs global connectivity
 * 3. Backward compatibility: Inheritance pattern preserves existing functionality
 * 4. Scalable design: O(n²) complexity with spatial indexing optimizations
 *
 * Key Components:
 * - GlobalConnectivityAnalyzer: Inter-component bridge detection
 * - InterGraphMergeEngine: Validated cross-component merging
 * - ConnectivityPriorityManager: Adaptive priority management
 * - GGCEConfiguration: Runtime configuration and feature flags
 */

//############################## Configuration System ##############################//

struct GGCEConfiguration {
    // Feature flags for gradual deployment
    bool enabled = false;
    bool debug_mode = false;
    bool verbose_logging = false;

    // Algorithm parameters
    double connectivity_weight = 0.6;
    double quality_weight = 0.3;
    double consistency_weight = 0.1;
    double quality_threshold = 0.7;

    // Performance tuning
    int max_merge_iterations = 10;
    int max_bridge_candidates = 50;
    double spatial_index_cell_size = 5.0; // mm

    // Intersection detection parameters
    double max_volume_overlap_ratio = 0.2;
    double critical_volume_overlap_ratio = 0.35;
    double point_inside_tolerance = 0.5; // mm
    double surface_proximity_threshold = 2.0; // mm

    // Algorithm variant selection
    std::string algorithm_variant = "conservative"; // "conservative", "aggressive", "balanced"

    /**
     * Load configuration from environment variables
     */
    static GGCEConfiguration loadFromEnvironment();

    /**
     * Validate configuration parameters
     */
    bool validate() const;

    /**
     * Get human-readable configuration summary
     */
    std::string toString() const;
};

//############################## Bridge Candidate System ##############################//

struct BridgeCandidate {
    int source_graph_index;      // Index of source graph component
    int target_graph_index;      // Index of target graph component
    int source_piece_id;         // Piece ID in source graph (1-based)
    int target_piece_id;         // Piece ID in target graph (1-based)

    // Scoring metrics
    double connectivity_score;   // Global connectivity benefit
    double geometric_confidence; // Geometric match quality
    double transformation_quality; // ICP transformation quality
    double consistency_score;    // Transformation chain consistency
    double combined_score;       // Final weighted score

    // Transformation data
    std::vector<LCSIndex> transformation_options;
    Matrix4d best_transformation; // Best ICP result

    // Validation results
    bool geometric_valid;
    bool intersection_valid;
    bool consistency_valid;
    std::string rejection_reason;

    BridgeCandidate() : source_graph_index(-1), target_graph_index(-1),
                       source_piece_id(-1), target_piece_id(-1),
                       connectivity_score(0.0), geometric_confidence(0.0),
                       transformation_quality(0.0), consistency_score(0.0),
                       combined_score(0.0), geometric_valid(false),
                       intersection_valid(false), consistency_valid(false) {}

    bool isValid() const {
        return geometric_valid && intersection_valid && consistency_valid;
    }

    std::string toString() const;
};

//############################## Spatial Indexing for Performance ##############################//

class SpatialIndex {
public:
    explicit SpatialIndex(double cell_size = 5.0);

    /**
     * Build spatial index from graph components
     */
    void buildIndex(const std::vector<RankingSubgraph>& components,
                   const std::vector<Geom>& shard);

    /**
     * Check if two graph components may have spatial connections
     * Early filtering to avoid expensive detailed analysis
     */
    bool mayHaveConnections(int graph_i, int graph_j) const;

    /**
     * Get candidate piece pairs for detailed analysis
     */
    std::vector<std::pair<int, int>> getCandidatePairs(int graph_i, int graph_j) const;

private:
    struct SpatialCell {
        std::set<int> graph_indices;
        std::set<int> piece_indices;
    };

    double cell_size_;
    std::map<std::tuple<int, int, int>, SpatialCell> spatial_grid_;
    std::vector<Vector3d> piece_centroids_;
    std::map<int, int> piece_to_graph_mapping_;

    std::tuple<int, int, int> getGridCell(const Vector3d& point) const;
};

//############################## Global Connectivity Analyzer ##############################//

class GlobalConnectivityAnalyzer {
public:
    explicit GlobalConnectivityAnalyzer(const GGCEConfiguration& config);

    /**
     * Detect potential bridges between graph components
     * O(n²) with spatial indexing optimization and early termination
     */
    std::vector<BridgeCandidate> detectPotentialBridges(
        const std::vector<RankingSubgraph>& components,
        const std::vector<Geom>& shard,
        const std::vector<LCSIndex>& lcs_reference);

    /**
     * Analyze cross-component connections between two specific graphs
     */
    std::vector<BridgeCandidate> analyzeCrossComponentConnections(
        const RankingSubgraph& graph_i,
        const RankingSubgraph& graph_j,
        int graph_i_index,
        int graph_j_index,
        const std::vector<Geom>& shard,
        const std::vector<LCSIndex>& lcs_reference);

    /**
     * Calculate connectivity metrics for current state
     */
    struct ConnectivityMetrics {
        int total_components;
        double average_component_size;
        double connectivity_ratio; // connected_pieces / total_pieces
        double geometric_quality_score;
        double transformation_consistency_score;
        double fragmentation_penalty;
    };

    ConnectivityMetrics calculateMetrics(const std::vector<RankingSubgraph>& components) const;

    /**
     * Hub Detection System - Identify optimal connectivity hub piece
     */
    struct HubCandidate {
        int piece_id;                  // Piece ID (1-based)
        int connection_count;          // Number of pieces it connects to
        double total_match_strength;   // Sum of match scores
        double centrality_score;       // Graph centrality metric
        double hub_score;             // Combined hub quality score
        std::vector<int> connected_pieces; // List of connected piece IDs

        HubCandidate() : piece_id(-1), connection_count(0), total_match_strength(0.0),
                        centrality_score(0.0), hub_score(0.0) {}
    };

    /**
     * Identify optimal hub piece for connectivity enhancement
     */
    HubCandidate identifyOptimalHub(const std::vector<LCSIndex>& lcs_reference);

    /**
     * Apply hub-centric prioritization to bridge candidates
     */
    std::vector<BridgeCandidate> applyHubCentricPrioritization(
        std::vector<BridgeCandidate>& candidates,
        const HubCandidate& hub);

private:
    /**
     * Prioritize bridge candidates using multi-objective optimization
     */
    std::vector<BridgeCandidate> prioritizeBridges(std::vector<BridgeCandidate>& candidates);

    /**
     * Evaluate geometric compatibility between pieces
     */
    double evaluateGeometricCompatibility(int piece_i, int piece_j,
                                        const std::vector<Geom>& shard,
                                        const std::vector<LCSIndex>& lcs_reference);

    /**
     * Calculate transformation quality using ICP analysis
     */
    double calculateTransformationQuality(const BridgeCandidate& candidate,
                                        const std::vector<Geom>& shard);

    const GGCEConfiguration& config_;
    std::unique_ptr<SpatialIndex> spatial_index_;
    std::unique_ptr<ConnectivityOptimizer> connectivity_optimizer_;
};

//############################## Merge Validation Framework ##############################//

struct ValidationResult {
    bool is_valid;
    double confidence_score;
    std::string rejection_reason;

    // Detailed analysis results
    struct GeometricAnalysis {
        bool passed;
        double overlap_score;
        double alignment_score;
        double surface_compatibility;
    } geometric_analysis;

    struct ConsistencyCheck {
        bool passed;
        double transformation_error;
        double chain_consistency;
        double global_alignment_error;
    } consistency_check;

    struct IntersectionAnalysis {
        bool passed;
        double volume_overlap_ratio;
        double point_containment_ratio;
        double surface_proximity_score;
        int opposing_normals_count;
    } intersection_analysis;

    struct ConnectivityImpact {
        double net_benefit;
        int components_reduction;
        double connectivity_improvement;
        double quality_impact;
    } connectivity_impact;

    ValidationResult() : is_valid(false), confidence_score(0.0) {}
    std::string toString() const;
};

class MergeValidationFramework {
public:
    explicit MergeValidationFramework(const GGCEConfiguration& config);

    /**
     * Comprehensive validation of a merge candidate
     */
    ValidationResult validateMerge(
        const BridgeCandidate& candidate,
        const std::vector<RankingSubgraph>& current_state,
        const std::vector<Geom>& shard);

private:
    /**
     * Geometric consistency validation
     */
    ValidationResult::GeometricAnalysis performGeometricAnalysis(
        const BridgeCandidate& candidate,
        const std::vector<Geom>& shard);

    /**
     * Transformation chain validation
     */
    ValidationResult::ConsistencyCheck validateTransformationConsistency(
        const BridgeCandidate& candidate,
        const std::vector<RankingSubgraph>& current_state);

    /**
     * Enhanced intersection detection
     */
    ValidationResult::IntersectionAnalysis performIntersectionAnalysis(
        const BridgeCandidate& candidate,
        const std::vector<Geom>& shard);

    /**
     * Global connectivity impact assessment
     */
    ValidationResult::ConnectivityImpact assessConnectivityImpact(
        const BridgeCandidate& candidate,
        const std::vector<RankingSubgraph>& current_state);

    const GGCEConfiguration& config_;
    std::unique_ptr<IntersectionDetector> intersection_detector_;
};

//############################## Inter-Graph Merge Engine ##############################//

class InterGraphMergeEngine {
public:
    explicit InterGraphMergeEngine(const GGCEConfiguration& config);

    /**
     * Execute validated merges on graph components
     */
    struct MergeResult {
        bool success;
        int merges_attempted;
        int merges_successful;
        int final_component_count;
        double connectivity_improvement;
        std::vector<std::string> merge_log;
        std::chrono::milliseconds execution_time;

        MergeResult() : success(false), merges_attempted(0), merges_successful(0),
                       final_component_count(0), connectivity_improvement(0.0) {}
    };

    MergeResult executeMerges(
        std::vector<RankingSubgraph>& components,
        const std::vector<BridgeCandidate>& candidates,
        std::vector<Geom>& shard);

private:
    /**
     * Execute a single merge operation
     */
    bool executeSingleMerge(
        std::vector<RankingSubgraph>& components,
        const BridgeCandidate& candidate,
        std::vector<Geom>& shard);

    /**
     * Plan optimal merge sequence with dependency resolution
     */
    struct MergeSequence {
        std::vector<BridgeCandidate> ordered_merges;
        double predicted_improvement;
    };

    MergeSequence planOptimalMergeSequence(const std::vector<BridgeCandidate>& candidates);

    /**
     * Update internal state after successful merge
     */
    void updateStateAfterMerge(
        std::vector<RankingSubgraph>& components,
        int merged_source_index,
        int merged_target_index);

    const GGCEConfiguration& config_;
    std::unique_ptr<MergeValidationFramework> validation_framework_;
};

//############################## Connectivity Priority Manager ##############################//

class ConnectivityPriorityManager {
public:
    explicit ConnectivityPriorityManager(const GGCEConfiguration& config);

    /**
     * Update priorities based on assembly state
     */
    void updatePriorities(State& state);

    /**
     * Calculate fragmentation penalty
     */
    double calculateFragmentationPenalty(const State& state);

    /**
     * Boost connectivity-promoting candidates
     */
    void boostConnectivityCandidates(State& state, double fragmentation_penalty);

    /**
     * Temporarily adjust quality thresholds for connectivity gains
     */
    void adjustQualityThresholds(State& state, double fragmentation_penalty);

private:
    const GGCEConfiguration& config_;
    static constexpr double FRAGMENTATION_THRESHOLD = 0.3;
    static constexpr double CONNECTIVITY_BOOST_FACTOR = 1.5;
};

//############################## Main GGCE System ##############################//

class GlobalGraphConnectivityEngine {
public:
    explicit GlobalGraphConnectivityEngine(const GGCEConfiguration& config = GGCEConfiguration::loadFromEnvironment());

    /**
     * Apply global connectivity enhancement to assembled components
     * This is the main entry point for the GGCE system
     */
    struct EnhancementResult {
        bool success;
        int initial_component_count;
        int final_component_count;
        double connectivity_improvement;
        double geometric_quality_change;
        std::chrono::milliseconds processing_time;
        std::vector<std::string> debug_log;

        EnhancementResult() : success(false), initial_component_count(0),
                             final_component_count(0), connectivity_improvement(0.0),
                             geometric_quality_change(0.0) {}

        std::string toString() const;
    };

    EnhancementResult applyGlobalConnectivityEnhancement(
        std::vector<RankingSubgraph>& components,
        std::vector<Geom>& shard,
        const std::vector<LCSIndex>& lcs_reference);

    /**
     * Check if global enhancement should be applied
     */
    bool shouldApplyGlobalEnhancement(const std::vector<RankingSubgraph>& components) const;

    /**
     * Get current configuration
     */
    const GGCEConfiguration& getConfiguration() const { return config_; }

    /**
     * Update configuration (for testing and tuning)
     */
    void updateConfiguration(const GGCEConfiguration& new_config);

private:
    GGCEConfiguration config_;
    std::unique_ptr<GlobalConnectivityAnalyzer> connectivity_analyzer_;
    std::unique_ptr<InterGraphMergeEngine> merge_engine_;
    std::unique_ptr<ConnectivityPriorityManager> priority_manager_;

    // Debug and monitoring
    void logDebugInfo(const std::string& message);
    void logPerformanceMetrics(const EnhancementResult& result);
};

#endif // GLOBAL_CONNECTIVITY_ENGINE_H