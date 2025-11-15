#ifndef AUTO_AGGLOMERATIVE_ASSEMBLER_H
#define AUTO_AGGLOMERATIVE_ASSEMBLER_H

#include <vector>
#include <set>
#include <string>
#include <memory>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cmath>

#include "ranking_system.h"
#include "feature_matching.h"
#include "data_structure.h"

/**
 * Auto-Agglomerative Assembler
 *
 * Inspired by PuzzleFusion++, this system replaces incremental beam search
 * with global optimization through agglomerative clustering.
 *
 * Core Innovation: Treats assembly as clustering problem rather than graph building,
 * eliminating 3-graph fragmentation through simultaneous multi-piece optimization.
 *
 * Architecture:
 * - Denoiser Phase: Iterative position refinement within clusters
 * - Verifier Phase: Quality assessment and merge candidate selection
 * - Agglomerative Phase: Progressive cluster merging toward unified assembly
 *
 * @author Senior System Engineer
 * @version 1.0 - Production Ready Implementation
 */
class AutoAgglomerativeAssembler {
public:
    //==================== Core Data Structures ====================//

    /**
     * Fragment Cluster - Represents a group of connected pottery sherds
     */
    struct FragmentCluster {
        std::vector<int> piece_ids;                    // Shard IDs in this cluster (1-based)
        std::vector<Matrix4d> transformations;         // Position transform for each shard
        double cluster_quality_score;                  // Overall cluster coherence score
        std::vector<double> individual_piece_scores;   // Per-piece contribution scores

        // Spatial properties
        Vector3d centroid;                             // Cluster geometric center
        double spatial_extent;                         // Maximum distance from centroid

        // Connectivity metadata
        std::vector<int> internal_connections;         // Connection indices within cluster
        std::unordered_set<int> external_piece_ids;    // Pieces connected to this cluster

        // Quality metrics
        double geometric_coherence;                    // How well pieces align geometrically
        double spatial_compactness;                    // How spatially compact the cluster is
        double connectivity_strength;                  // Total connection strength

        FragmentCluster() : cluster_quality_score(0.0), spatial_extent(0.0),
                           geometric_coherence(0.0), spatial_compactness(0.0),
                           connectivity_strength(0.0) {}

        int size() const { return static_cast<int>(piece_ids.size()); }
        bool empty() const { return piece_ids.empty(); }

        // Validation
        bool isValid() const {
            return piece_ids.size() == transformations.size() &&
                   piece_ids.size() == individual_piece_scores.size() &&
                   !piece_ids.empty();
        }
    };

    /**
     * Merge Candidate - Represents potential cluster merge with quality assessment
     */
    struct MergeCandidate {
        int cluster1_index;                           // Index of first cluster
        int cluster2_index;                           // Index of second cluster
        std::vector<int> connecting_edge_indices;     // Connection indices bridging clusters

        // Quality assessments
        double merge_quality_score;                   // Overall merge benefit score
        double geometric_compatibility;               // How well clusters align geometrically
        double spatial_proximity;                     // How close clusters are spatially
        double connectivity_strength;                 // Strength of inter-cluster connections

        // Predicted merge properties
        Vector3d predicted_centroid;                  // Expected centroid after merge
        double predicted_spatial_extent;              // Expected spatial extent after merge
        double predicted_quality_improvement;         // Expected quality improvement

        MergeCandidate() : cluster1_index(-1), cluster2_index(-1),
                          merge_quality_score(0.0), geometric_compatibility(0.0),
                          spatial_proximity(0.0), connectivity_strength(0.0),
                          predicted_spatial_extent(0.0), predicted_quality_improvement(0.0) {}

        bool isValid() const {
            return cluster1_index >= 0 && cluster2_index >= 0 &&
                   cluster1_index != cluster2_index &&
                   !connecting_edge_indices.empty();
        }
    };

    /**
     * Assembly Result - Complete result of auto-agglomerative assembly
     */
    struct AssemblyResult {
        bool success;                                 // Assembly completed successfully
        std::vector<FragmentCluster> final_clusters;  // Final cluster configuration
        double global_assembly_score;                // Overall assembly quality
        int total_pieces_assembled;                  // Number of pieces in final assembly

        // Performance metrics
        int iterations_completed;                    // Number of agglomerative iterations
        double computation_time_seconds;             // Total computation time
        std::string completion_status;               // Detailed completion status

        // Detailed statistics
        int initial_cluster_count;                   // Starting number of clusters
        int final_cluster_count;                     // Final number of clusters
        int total_merges_executed;                   // Number of successful merges
        std::vector<double> iteration_scores;        // Quality score progression

        AssemblyResult() : success(false), global_assembly_score(0.0),
                          total_pieces_assembled(0), iterations_completed(0),
                          computation_time_seconds(0.0), initial_cluster_count(0),
                          final_cluster_count(0), total_merges_executed(0) {}

        /**
         * Convert to State format for pipeline integration
         */
        State convertToState() const;

        /**
         * Generate comprehensive assembly report
         */
        std::string generateDetailedReport() const;
    };

    //==================== Configuration ====================//

    /**
     * Assembly Configuration - Comprehensive parameter control
     */
    struct Config {
        // Core algorithm parameters
        int max_agglomerative_iterations;             // Maximum clustering iterations
        int max_refinement_iterations;                // Maximum position refinement iterations
        double convergence_threshold;                 // Quality improvement threshold for convergence

        // Quality thresholds
        double min_merge_quality_threshold;           // Minimum quality for cluster merge
        double min_geometric_compatibility;           // Minimum geometric alignment requirement
        double max_spatial_extent_ratio;              // Maximum cluster spatial extent ratio

        // Optimization parameters
        double position_refinement_step_size;         // Step size for position optimization
        double quality_improvement_threshold;         // Minimum improvement to accept change

        // Performance tuning
        bool enable_parallel_processing;              // Enable multi-threading where possible
        bool enable_detailed_logging;                 // Enable comprehensive debug output
        bool enable_quality_caching;                  // Cache quality calculations for performance

        // Fallback settings
        bool enable_fallback_to_original;             // Fall back to original algorithm if needed
        double fallback_trigger_threshold;            // Quality threshold to trigger fallback

        Config() : max_agglomerative_iterations(15), max_refinement_iterations(5),
                  convergence_threshold(0.001), min_merge_quality_threshold(0.1),
                  min_geometric_compatibility(0.3), max_spatial_extent_ratio(3.0),
                  position_refinement_step_size(0.1), quality_improvement_threshold(0.001),
                  enable_parallel_processing(false), enable_detailed_logging(true),
                  enable_quality_caching(true), enable_fallback_to_original(true),
                  fallback_trigger_threshold(0.05) {}
    };

    //==================== Public Interface ====================//

    /**
     * Constructor with configuration
     */
    explicit AutoAgglomerativeAssembler(const Config& config = Config());

    /**
     * Destructor - cleanup resources
     */
    ~AutoAgglomerativeAssembler();

    /**
     * Main Assembly Function - Replaces incremental graph building entirely
     *
     * @param all_connections All connections surviving pairwise pruning
     * @param shard All piece geometries (modified in place with final transforms)
     * @param total_pieces Number of pieces to assemble
     * @return Complete assembly result with detailed metrics
     */
    AssemblyResult assembleGlobally(const std::vector<LCSIndex>& all_connections,
                                   std::vector<Geom>& shard,
                                   int total_pieces);

    /**
     * Get current configuration
     */
    const Config& getConfig() const { return config_; }

    /**
     * Update configuration (before assembly)
     */
    void updateConfig(const Config& new_config) { config_ = new_config; }

private:
    //==================== Core Algorithm Implementation ====================//

    /**
     * INITIALIZATION: Create initial fragment clusters with anchor strategy
     * PuzzleFusion++ Algorithm: Anchor fragment to resolve 3D transformation ambiguity
     */
    std::vector<FragmentCluster> initializeClusters(int total_pieces,
                                                   const std::vector<LCSIndex>& all_connections);

    /**
     * Select anchor fragment with highest connectivity (PuzzleFusion++ strategy)
     */
    int selectAnchorFragment(const std::vector<LCSIndex>& all_connections, int total_pieces);

    /**
     * Find initial transformation relative to anchor fragment
     */
    Matrix4d findInitialTransformRelativeToAnchor(int piece_id, int anchor_piece_id,
                                                 const std::vector<LCSIndex>& all_connections);

    /**
     * PHASE 1: DENOISER - Iterative position refinement within clusters
     */
    void refineClusterPositions(FragmentCluster& cluster,
                               const std::vector<LCSIndex>& all_connections,
                               const std::vector<Geom>& shard);

    /**
     * PHASE 2: VERIFIER - Find best cluster merge candidates
     */
    std::vector<MergeCandidate> findBestClusterMerges(
        const std::vector<FragmentCluster>& clusters,
        const std::vector<LCSIndex>& all_connections,
        const std::vector<Geom>& shard);

    /**
     * PHASE 3: AGGLOMERATIVE MERGE - Execute cluster merge
     */
    bool executeMerge(std::vector<FragmentCluster>& clusters,
                     const MergeCandidate& merge_candidate,
                     const std::vector<LCSIndex>& all_connections,
                     const std::vector<Geom>& shard);

    //==================== Quality Assessment ====================//

    /**
     * Evaluate overall cluster quality
     */
    double evaluateClusterQuality(const FragmentCluster& cluster,
                                 const std::vector<LCSIndex>& all_connections,
                                 const std::vector<Geom>& shard) const;

    /**
     * Evaluate potential merge quality
     */
    double evaluateMergeQuality(const FragmentCluster& cluster1,
                               const FragmentCluster& cluster2,
                               const std::vector<int>& connecting_edges,
                               const std::vector<Geom>& shard) const;

    /**
     * Assess geometric compatibility between clusters
     */
    double assessGeometricCompatibility(const FragmentCluster& cluster1,
                                       const FragmentCluster& cluster2,
                                       const std::vector<LCSIndex>& connections) const;

    //==================== Spatial Analysis ====================//

    /**
     * Compute optimal position for piece within cluster context
     */
    Matrix4d computeOptimalPositionInCluster(int piece_id,
                                            int piece_index_in_cluster,
                                            const FragmentCluster& cluster,
                                            const std::vector<LCSIndex>& all_connections,
                                            const std::vector<Geom>& shard) const;

    /**
     * Find connections between two clusters
     */
    std::vector<int> findConnectionsBetweenClusters(const FragmentCluster& cluster1,
                                                   const FragmentCluster& cluster2,
                                                   const std::vector<LCSIndex>& all_connections) const;

    /**
     * Update cluster spatial properties
     */
    void updateClusterSpatialProperties(FragmentCluster& cluster,
                                       const std::vector<Geom>& shard) const;

    //==================== Utility Functions ====================//


    /**
     * Validate cluster consistency
     */
    bool validateClusterConsistency(const std::vector<FragmentCluster>& clusters) const;

    /**
     * Generate comprehensive debug output
     */
    void logAssemblyProgress(const std::vector<FragmentCluster>& clusters,
                            int iteration,
                            const std::string& phase_name) const;

    //==================== Member Variables ====================//

    Config config_;                                   // Assembly configuration

    // Performance optimization
    mutable std::unordered_map<std::string, double> quality_cache_;  // Quality calculation cache

    // Statistics tracking
    std::chrono::high_resolution_clock::time_point start_time_;
    mutable int total_quality_evaluations_;
    mutable int cache_hits_;
    mutable int cache_misses_;

    // Debugging and validation
    mutable std::vector<std::string> debug_log_;

    //==================== Constants ====================//

    static constexpr double GEOMETRIC_TOLERANCE = 1e-6;
    static constexpr double SPATIAL_PROXIMITY_THRESHOLD = 10.0;  // mm
    static constexpr double MIN_CLUSTER_QUALITY = 0.01;
    static constexpr int MAX_PIECES_PER_CLUSTER = 50;
};

#endif // AUTO_AGGLOMERATIVE_ASSEMBLER_H