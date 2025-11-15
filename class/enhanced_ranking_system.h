#ifndef ENHANCED_RANKING_SYSTEM_H
#define ENHANCED_RANKING_SYSTEM_H

#include "ranking_system.h"
#include "global_connectivity_engine.h"
#include "hub_guided_beam_search.h"
#include <memory>

/**
 * Enhanced StateManager with Global Graph Connectivity Enhancement (GGCE)
 *
 * This class extends the existing StateManager to include global connectivity
 * enhancement capabilities while maintaining 100% backward compatibility.
 *
 * Key Features:
 * - Inherits from StateManager (non-invasive enhancement)
 * - Feature flag control for gradual deployment
 * - Two-phase processing: Original algorithm + Global enhancement
 * - Comprehensive debug and monitoring capabilities
 * - Scalable configuration system
 *
 * Usage:
 * - Drop-in replacement for StateManager
 * - Enable via GGCE_ENABLED environment variable
 * - Configure via environment variables or programmatically
 * - Fallback to original algorithm if GGCE fails
 */
class EnhancedStateManager : public StateManager {
public:
    // BACKWARD COMPATIBLE: All original constructors preserved
    EnhancedStateManager() : StateManager(), ggce_enabled_(false) {
        initializeGGCE();
    }

    EnhancedStateManager(int N,
                        int s,
                        std::vector<Geom>& shard,
                        std::list<LCSIndex>& LCS_out,
                        int step_size,
                        const std::string& log_path)
        : StateManager(N, s, shard, LCS_out, step_size, log_path), ggce_enabled_(false) {
        initializeGGCE();
    }

    EnhancedStateManager(int step_size, std::string& log_path)
        : StateManager(step_size, log_path), ggce_enabled_(false) {
        initializeGGCE();
    }

    virtual ~EnhancedStateManager() = default;

    /**
     * Enhanced BuildStep with optional global connectivity enhancement
     * BACKWARD COMPATIBLE: Preserves original behavior when GGCE disabled
     */
    void BuildStep();

    /**
     * Get current GGCE configuration
     */
    const GGCEConfiguration& getGGCEConfiguration() const;

    /**
     * Update GGCE configuration (for testing and tuning)
     */
    void updateGGCEConfiguration(const GGCEConfiguration& config);

    /**
     * Check if GGCE is enabled and active
     */
    bool isGGCEEnabled() const { return ggce_enabled_; }

    /**
     * Force enable/disable GGCE (overrides environment configuration)
     */
    void setGGCEEnabled(bool enabled);

    /**
     * Enable hub-guided graph building with enhanced persistence
     * CRITICAL FIX #3: Prevents premature termination during graph merging
     */
    void setHubGuidanceEnabled(bool enabled);

    /**
     * Get GGCE statistics for current session
     */
    struct GGCEStatistics {
        int total_assemblies_processed;
        int assemblies_enhanced;
        int total_components_before_enhancement;
        int total_components_after_enhancement;
        double average_connectivity_improvement;
        double total_processing_time_ms;
        int successful_merges;
        int failed_merges;

        GGCEStatistics() : total_assemblies_processed(0), assemblies_enhanced(0),
                          total_components_before_enhancement(0), total_components_after_enhancement(0),
                          average_connectivity_improvement(0.0), total_processing_time_ms(0.0),
                          successful_merges(0), failed_merges(0) {}

        std::string toString() const;
    };

    const GGCEStatistics& getGGCEStatistics() const { return ggce_stats_; }

    /**
     * Reset GGCE statistics
     */
    void resetGGCEStatistics();

    /**
     * Initialize hub guidance system with LCS data
     */
    void initializeHubGuidance(const std::list<LCSIndex>& lcs_data);

    /**
     * Apply hub-guided scoring to connection
     */
    double applyHubGuidance(int piece_x, int piece_y, double base_score);

protected:
    /**
     * Apply global connectivity enhancement to final assembly state
     * This is called after the original algorithm completes
     */
    virtual bool applyGlobalConnectivityEnhancement();

    /**
     * Extract graph components from current state
     */
    std::vector<RankingSubgraph> extractGraphComponents() const;

    /**
     * Update internal state after GGCE processing
     */
    void updateStateAfterGGCE(const std::vector<RankingSubgraph>& enhanced_components);

    /**
     * Log GGCE debug information
     */
    void logGGCEDebug(const std::string& message) const;

    /**
     * Handle GGCE failures gracefully
     */
    void handleGGCEFailure(const std::string& error_message);

private:
    /**
     * Initialize GGCE system based on configuration
     */
    void initializeGGCE();

    /**
     * Validate GGCE prerequisites
     */
    bool validateGGCEPrerequisites() const;

    /**
     * Update statistics after GGCE processing
     */
    void updateGGCEStatistics(const GlobalGraphConnectivityEngine::EnhancementResult& result);

    // GGCE system components
    bool ggce_enabled_;
    std::unique_ptr<GlobalGraphConnectivityEngine> ggce_engine_;
    GGCEConfiguration ggce_config_;
    GGCEStatistics ggce_stats_;

    // Hub-guided beam search system
    std::unique_ptr<HubGuidedBeamSearch> hub_guidance_;
    bool hub_guidance_initialized_;

    // Enhanced graph building with hub guidance
    bool hub_guided_building_enabled_;

    // Fallback and recovery
    bool ggce_fallback_mode_;
    std::string last_ggce_error_;
};

/**
 * Factory function to create appropriate StateManager instance
 *
 * This function automatically detects if GGCE should be enabled and returns
 * the appropriate StateManager instance (enhanced or original).
 *
 * This allows for gradual migration:
 * - Code using this factory gets GGCE when enabled
 * - Code directly instantiating StateManager remains unchanged
 */
std::unique_ptr<StateManager> createStateManager(int N,
                                                int s,
                                                std::vector<Geom>& shard,
                                                std::list<LCSIndex>& LCS_out,
                                                int step_size,
                                                const std::string& log_path);

std::unique_ptr<StateManager> createStateManager(int step_size,
                                                std::string& log_path);

/**
 * GGCE Integration Utilities
 */
namespace GGCEUtils {
    /**
     * Convert State to vector of RankingSubgraph for GGCE processing
     */
    std::vector<RankingSubgraph> extractComponents(const State& state);

    /**
     * Update State with GGCE-enhanced components
     */
    void updateStateWithComponents(State& state, const std::vector<RankingSubgraph>& components);

    /**
     * Validate component consistency after GGCE processing
     */
    bool validateComponentConsistency(const std::vector<RankingSubgraph>& components);

    /**
     * Calculate connectivity metrics for a set of components
     */
    struct ConnectivitySummary {
        int total_components;
        int total_pieces;
        double connectivity_ratio;
        double fragmentation_score;

        std::string toString() const;
    };

    ConnectivitySummary calculateConnectivitySummary(const std::vector<RankingSubgraph>& components);

    /**
     * Generate human-readable assembly report
     */
    std::string generateAssemblyReport(const std::vector<RankingSubgraph>& components,
                                      const GlobalGraphConnectivityEngine::EnhancementResult& ggce_result);
}

/**
 * GGCE Test and Validation Framework
 *
 * This provides utilities for testing GGCE functionality without disrupting
 * production systems.
 */
class GGCETestFramework {
public:
    /**
     * Test GGCE on a specific assembly state without modifying it
     */
    struct TestResult {
        bool ggce_applicable;
        bool ggce_successful;
        int components_before;
        int components_after;
        double connectivity_improvement;
        double processing_time_ms;
        std::vector<std::string> debug_log;
        std::string error_message;

        TestResult() : ggce_applicable(false), ggce_successful(false),
                      components_before(0), components_after(0),
                      connectivity_improvement(0.0), processing_time_ms(0.0) {}

        std::string toString() const;
    };

    static TestResult testGGCEOnState(const State& state,
                                     const std::vector<Geom>& shard,
                                     const std::vector<LCSIndex>& lcs_reference,
                                     const GGCEConfiguration& config = GGCEConfiguration::loadFromEnvironment());

    /**
     * Benchmark GGCE performance on various assembly configurations
     */
    struct BenchmarkResult {
        std::vector<TestResult> test_results;
        double average_improvement;
        double average_processing_time;
        int success_rate_percent;

        std::string toString() const;
    };

    static BenchmarkResult benchmarkGGCEPerformance(const std::vector<State>& test_states,
                                                   const std::vector<Geom>& shard,
                                                   const std::vector<LCSIndex>& lcs_reference);

    /**
     * Validate GGCE correctness by comparing against expected results
     */
    static bool validateGGCECorrectness(const State& input_state,
                                       const State& expected_output,
                                       const std::vector<Geom>& shard,
                                       const std::vector<LCSIndex>& lcs_reference);
};

/**
 * GGCE Performance Monitor
 *
 * Tracks GGCE performance metrics and detects regressions
 */
class GGCEPerformanceMonitor {
public:
    struct PerformanceMetrics {
        double average_processing_time_ms;
        double connectivity_improvement_ratio;
        double success_rate;
        int total_assemblies_processed;
        std::chrono::system_clock::time_point last_updated;

        PerformanceMetrics() : average_processing_time_ms(0.0),
                             connectivity_improvement_ratio(0.0),
                             success_rate(0.0),
                             total_assemblies_processed(0),
                             last_updated(std::chrono::system_clock::now()) {}
    };

    /**
     * Update metrics with new GGCE result
     */
    void recordResult(const GlobalGraphConnectivityEngine::EnhancementResult& result);

    /**
     * Get current performance metrics
     */
    const PerformanceMetrics& getCurrentMetrics() const { return current_metrics_; }

    /**
     * Check if performance regression detected
     */
    bool detectRegression(const PerformanceMetrics& baseline) const;

    /**
     * Save metrics to file for persistent monitoring
     */
    void saveMetrics(const std::string& filepath) const;

    /**
     * Load baseline metrics from file
     */
    static PerformanceMetrics loadMetrics(const std::string& filepath);

private:
    PerformanceMetrics current_metrics_;
    std::vector<GlobalGraphConnectivityEngine::EnhancementResult> recent_results_;

    static constexpr int MAX_RECENT_RESULTS = 100;
    static constexpr double REGRESSION_THRESHOLD = 0.2; // 20% performance degradation
};

#endif // ENHANCED_RANKING_SYSTEM_H