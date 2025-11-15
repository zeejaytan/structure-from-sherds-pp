#ifndef TWO_PHASE_ASSEMBLY_H
#define TWO_PHASE_ASSEMBLY_H

#include <vector>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <chrono>
#include "ranking_system.h"
#include "global_connectivity_engine.h"

/**
 * Two-Phase Assembly Architecture
 *
 * Solves the 3-graph fragmentation problem by separating local assembly
 * quality from global connectivity optimization.
 *
 * Phase 1: Local Component Building with Connection Preservation
 * - Enhanced local assembly (existing algorithm)
 * - Comprehensive connection preservation
 * - Quality-focused component generation
 *
 * Phase 2: Global Connectivity Resolution
 * - Cross-component bridge analysis using ALL preserved connections
 * - Enhanced GGCE as primary global optimization engine
 * - Component merging with comprehensive validation
 */

// Forward declarations
struct LCSIndex;
class State;
class RankingSubgraph;

//############################## Connection Preservation Layer ##############################//

/**
 * Comprehensive connection preservation system that maintains ALL valid
 * connections throughout the assembly process, preventing data loss during
 * component assignment.
 */
class ConnectionPreservationLayer {
public:
    struct PreservedConnection {
        LCSIndex original_connection;      // Original LCS connection data
        int piece_x, piece_y;             // Piece IDs (1-based)
        double connection_strength;        // Match quality score
        int component_x, component_y;     // Component assignments (-1 if unassigned)
        bool is_cross_component;          // True if spans different components
        std::string preservation_timestamp; // When connection was preserved

        // Enhanced metadata for global optimization
        double geometric_confidence;      // ICP/geometric validation score
        double spatial_proximity;        // Physical distance metric
        double normal_compatibility;     // Surface normal alignment
        bool passed_intersection_test;   // Intersection detection result

        PreservedConnection() : piece_x(-1), piece_y(-1), connection_strength(0.0),
                               component_x(-1), component_y(-1), is_cross_component(false),
                               geometric_confidence(0.0), spatial_proximity(0.0),
                               normal_compatibility(0.0), passed_intersection_test(false) {}
    };

    struct ComponentAssignment {
        int piece_id;                     // Piece ID (1-based)
        int component_id;                 // Component ID (0-based)
        std::string assignment_reason;    // Why this assignment was made
        std::chrono::system_clock::time_point timestamp;

        ComponentAssignment() : piece_id(-1), component_id(-1) {}
    };

private:
    // Core data structures
    std::vector<PreservedConnection> all_connections_;
    std::map<std::pair<int,int>, std::vector<PreservedConnection*>> piece_pair_connections_;
    std::map<int, ComponentAssignment> piece_assignments_;

    // Cross-component analysis
    std::vector<PreservedConnection*> cross_component_connections_;
    std::map<std::pair<int,int>, std::vector<PreservedConnection*>> component_bridges_;

    // Statistics and metadata
    struct PreservationStats {
        int total_connections_preserved;
        int cross_component_connections;
        int component_count;
        double preservation_efficiency;
        std::chrono::milliseconds processing_time;

        PreservationStats() : total_connections_preserved(0), cross_component_connections(0),
                             component_count(0), preservation_efficiency(0.0) {}
    } stats_;

public:
    ConnectionPreservationLayer();
    ~ConnectionPreservationLayer();

    /**
     * Core preservation functions
     */

    // Preserve connections AFTER pairwise pruning (the 88 survivors)
    void preserveConnectionsAfterPruning(const std::vector<LCSIndex>& pruned_connections);

    // Update component assignments during graph building
    void updateComponentAssignments(const std::vector<RankingSubgraph>& components);

    // Mark connections as cross-component when assignments change
    void identifyCrossComponentConnections();

    /**
     * Cross-component bridge analysis
     */

    // Get all cross-component connections for GGCE
    std::vector<PreservedConnection> getCrossComponentConnections() const;

    // Get connections between specific components
    std::vector<PreservedConnection> getConnectionsBetweenComponents(int comp1, int comp2) const;

    // Generate bridge candidates for GGCE optimization
    std::vector<BridgeCandidate> generateBridgeCandidatesForGGCE() const;

    /**
     * Analysis and diagnostics
     */

    // Get comprehensive preservation statistics
    PreservationStats getPreservationStatistics() const;

    // Validate connection integrity
    bool validateConnectionIntegrity() const;

    // Export preserved connections for analysis
    void exportPreservedConnections(const std::string& output_path) const;

    // Debug output
    void printPreservationSummary() const;
    void printCrossComponentAnalysis() const;

    /**
     * Component assignment tracking
     */

    // Get current component assignment for piece
    int getComponentAssignment(int piece_id) const;

    // Get all pieces in a component
    std::vector<int> getPiecesInComponent(int component_id) const;

    // Get component count
    int getComponentCount() const;

private:
    /**
     * Internal helper functions
     */

    // Calculate connection metadata
    void calculateConnectionMetadata(PreservedConnection& connection);

    // GLOBAL OPTIMIZATION: Compute optimal transformation for maximum assembly quality
    Matrix4d computeOptimalTransformation(const PreservedConnection* connection) const;

    // GLOBAL SCORING: Evaluate transformation quality for global assembly
    double evaluateTransformationQuality(const Matrix4d& transform, const PreservedConnection* connection) const;

    // Update cross-component flags
    void updateCrossComponentFlags();

    // Generate preservation timestamp
    std::string generateTimestamp() const;

    // Validate connection data
    bool validateConnection(const PreservedConnection& connection) const;
};

//############################## Enhanced Phase 1 Manager ##############################//

/**
 * Enhanced Phase 1: Local Component Building with Connection Preservation
 *
 * Wraps the existing graph building algorithm while preserving ALL connection
 * data for Phase 2 global optimization.
 */
class Phase1LocalAssemblyManager {
private:
    std::unique_ptr<ConnectionPreservationLayer> preservation_layer_;
    std::vector<RankingSubgraph> local_components_;

    struct Phase1Result {
        bool success;
        int components_generated;
        double local_assembly_quality;
        int connections_preserved;
        std::chrono::milliseconds processing_time;
        std::vector<std::string> debug_log;

        Phase1Result() : success(false), components_generated(0),
                        local_assembly_quality(0.0), connections_preserved(0) {}
    };

public:
    Phase1LocalAssemblyManager();
    ~Phase1LocalAssemblyManager();

    /**
     * Execute Phase 1: Local component building with preservation
     */
    Phase1Result executeLocalAssembly(
        const std::vector<LCSIndex>& pruned_connections,  // Connections AFTER pairwise pruning
        std::vector<Geom>& shard,
        StateManager& state_manager  // Use existing algorithm
    );

    /**
     * Get results for Phase 2
     */
    std::vector<RankingSubgraph> getLocalComponents() const;
    ConnectionPreservationLayer* getPreservationLayer() const;

    /**
     * Quality assessment
     */
    double assessLocalAssemblyQuality() const;
    void printPhase1Summary() const;
};

//############################## Enhanced Phase 2 Manager ##############################//

/**
 * Enhanced Phase 2: Global Connectivity Resolution
 *
 * Uses preserved connections to perform global optimization and component merging.
 * This is where the GGCE system becomes the primary optimization engine.
 */
class Phase2GlobalConnectivityManager {
private:
    std::unique_ptr<GlobalGraphConnectivityEngine> enhanced_ggce_;
    ConnectionPreservationLayer* preservation_layer_;  // Reference to Phase 1 data

    struct Phase2Result {
        bool success;
        int initial_components;
        int final_components;
        double connectivity_improvement;
        double global_assembly_quality;
        int bridges_attempted;
        int bridges_successful;
        std::chrono::milliseconds processing_time;
        std::vector<std::string> debug_log;
        std::vector<RankingSubgraph> final_component_data;

        Phase2Result() : success(false), initial_components(0), final_components(0),
                        connectivity_improvement(0.0), global_assembly_quality(0.0),
                        bridges_attempted(0), bridges_successful(0) {}
    };

public:
    explicit Phase2GlobalConnectivityManager(ConnectionPreservationLayer* preservation_layer);
    ~Phase2GlobalConnectivityManager();

    /**
     * Execute Phase 2: Global connectivity resolution
     */
    Phase2Result executeGlobalConnectivity(
        std::vector<RankingSubgraph>& components,
        std::vector<Geom>& shard
    );

    /**
     * Enhanced bridge candidate generation
     */
    std::vector<BridgeCandidate> generateEnhancedBridgeCandidates(
        const std::vector<RankingSubgraph>& components,
        const std::vector<Geom>& shard
    );

    /**
     * Multi-objective bridge optimization
     */
    std::vector<BridgeCandidate> optimizeBridgeSelection(
        std::vector<BridgeCandidate>& candidates
    );

    /**
     * Progressive component merging
     */
    bool executeProgressiveComponentMerging(
        std::vector<RankingSubgraph>& components,
        const std::vector<BridgeCandidate>& bridges,
        std::vector<Geom>& shard
    );

    /**
     * Execute single merge using GGCE implementation
     */
    bool executeSingleMergeFromGGCE(
        std::vector<RankingSubgraph>& components,
        const BridgeCandidate& candidate,
        std::vector<Geom>& shard
    );

    /**
     * Quality assessment and validation
     */
    double assessGlobalAssemblyQuality(const std::vector<RankingSubgraph>& components) const;
    bool validateFinalAssembly(const std::vector<RankingSubgraph>& components) const;
    void printPhase2Summary() const;
};

//############################## Two-Phase Assembly Coordinator ##############################//

/**
 * Main coordinator for the two-phase assembly system.
 *
 * This class orchestrates both phases and provides the main entry point
 * for the enhanced assembly algorithm.
 */
class TwoPhaseAssemblyCoordinator {
private:
    std::unique_ptr<Phase1LocalAssemblyManager> phase1_manager_;
    std::unique_ptr<Phase2GlobalConnectivityManager> phase2_manager_;

    struct TwoPhaseResult {
        bool success;

        // Phase 1 results
        int local_components_generated;
        double local_assembly_quality;
        int connections_preserved;

        // Phase 2 results
        int final_components;
        double connectivity_improvement;
        double global_assembly_quality;
        int bridges_successful;

        // Overall metrics
        std::chrono::milliseconds total_processing_time;
        double overall_improvement;
        std::vector<std::string> comprehensive_debug_log;

        // Actual component data for conversion back to original format
        std::vector<RankingSubgraph> final_component_data;

        TwoPhaseResult() : success(false), local_components_generated(0),
                          local_assembly_quality(0.0), connections_preserved(0),
                          final_components(0), connectivity_improvement(0.0),
                          global_assembly_quality(0.0), bridges_successful(0),
                          overall_improvement(0.0) {}
    };

public:
    TwoPhaseAssemblyCoordinator();
    ~TwoPhaseAssemblyCoordinator();

    /**
     * Main entry point: Execute complete two-phase assembly
     */
    TwoPhaseResult executeEnhancedAssembly(
        const std::vector<LCSIndex>& pruned_connections,  // Connections AFTER pairwise pruning
        std::vector<Geom>& shard,
        StateManager& state_manager
    );

    /**
     * Individual phase execution (for testing/debugging)
     */
    bool executePhase1Only(
        const std::vector<LCSIndex>& pruned_connections,  // Connections AFTER pairwise pruning
        std::vector<Geom>& shard,
        StateManager& state_manager
    );

    bool executePhase2Only(
        std::vector<RankingSubgraph>& components,
        std::vector<Geom>& shard
    );

    /**
     * Results and analysis
     */
    std::vector<RankingSubgraph> getFinalComponents() const;
    TwoPhaseResult getLastResult() const;

    /**
     * Configuration and diagnostics
     */
    void enableDebugMode(bool enabled);
    void exportComprehensiveResults(const std::string& output_path) const;
    void printComprehensiveSummary() const;

private:
    TwoPhaseResult last_result_;
    bool debug_mode_enabled_;

    // Helper functions
    void initializePhaseManagers();
    void logPhaseTransition(const std::string& phase_name) const;
    double calculateOverallImprovement(const TwoPhaseResult& result) const;
};

#endif // TWO_PHASE_ASSEMBLY_H