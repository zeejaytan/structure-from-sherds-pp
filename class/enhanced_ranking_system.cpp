#include "enhanced_ranking_system.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <numeric>

//############################## Enhanced State Manager Implementation ##############################//

void EnhancedStateManager::initializeGGCE() {
    try {
        // Load configuration from environment
        ggce_config_ = GGCEConfiguration::loadFromEnvironment();
        ggce_enabled_ = ggce_config_.enabled;
        ggce_fallback_mode_ = false;
        hub_guided_building_enabled_ = false;  // Initialize new member variable

        if (ggce_enabled_) {
            // Validate configuration
            if (!ggce_config_.validate()) {
                logGGCEDebug("Invalid GGCE configuration, disabling");
                ggce_enabled_ = false;
                return;
            }

            // Initialize GGCE engine
            ggce_engine_ = std::make_unique<GlobalGraphConnectivityEngine>(ggce_config_);

            logGGCEDebug("GGCE initialized successfully");
            if (ggce_config_.debug_mode) {
                std::cout << "GGCE Configuration:\n" << ggce_config_.toString() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        handleGGCEFailure(std::string("GGCE initialization failed: ") + e.what());
    }
}

void EnhancedStateManager::BuildStep() {
    // Phase 1: Execute original algorithm (BACKWARD COMPATIBLE)
    try {
        // If hub guidance is enabled, enhance the graph building process
        if (hub_guided_building_enabled_) {
            logGGCEDebug("Executing hub-guided enhanced BuildStep");
            StateManager::BuildStep();  // Original algorithm with enhanced data
        } else {
            StateManager::BuildStep();  // Standard algorithm
        }
    } catch (const std::exception& e) {
        // If original algorithm fails, rethrow - no GGCE can help here
        throw;
    }

    // Phase 2: Optional global connectivity enhancement
    if (ggce_enabled_ && !ggce_fallback_mode_) {
        if (validateGGCEPrerequisites()) {
            bool enhancement_success = applyGlobalConnectivityEnhancement();

            if (!enhancement_success && ggce_config_.debug_mode) {
                std::cout << "GGCE: Enhancement failed, results from original algorithm preserved" << std::endl;
            }
        } else {
            logGGCEDebug("GGCE prerequisites not met, skipping enhancement");
        }
    }
}

bool EnhancedStateManager::validateGGCEPrerequisites() const {
    // Check if we have valid output states
    if (out_state_.empty()) {
        return false;
    }

    // Check if we have multiple components (fragmentation)
    bool has_fragmentation = false;
    for (const auto& state : out_state_) {
        if (state.graph_.size() > 1) {
            has_fragmentation = true;
            break;
        }
    }

    return has_fragmentation;
}

bool EnhancedStateManager::applyGlobalConnectivityEnhancement() {
    if (!ggce_engine_) {
        return false;
    }

    bool any_enhancement_successful = false;

    try {
        // Process each output state
        for (auto& state : out_state_) {
            // Skip states with single components
            if (state.graph_.size() <= 1) {
                continue;
            }

            logGGCEDebug("Applying GGCE to state with " + std::to_string(state.graph_.size()) + " components");

            // Extract components
            auto components = extractGraphComponents();
            int initial_component_count = static_cast<int>(components.size());

            // Build LCS reference from initial graph
            std::vector<LCSIndex> lcs_reference(initial_graph_.begin(), initial_graph_.end());

            // Apply GGCE enhancement
            auto enhancement_result = ggce_engine_->applyGlobalConnectivityEnhancement(
                components, shard_, lcs_reference);

            // Update statistics
            updateGGCEStatistics(enhancement_result);

            // CRITICAL FIX #4: Validate actual connectivity improvement
            bool actual_improvement = (enhancement_result.success &&
                                     enhancement_result.final_component_count < initial_component_count);

            if (actual_improvement) {
                // Update state with enhanced components
                updateStateAfterGGCE(components);
                any_enhancement_successful = true;

                logGGCEDebug("GGCE enhancement GENUINELY successful: " +
                           std::to_string(initial_component_count) + " -> " +
                           std::to_string(enhancement_result.final_component_count) + " components");

                if (ggce_config_.verbose_logging) {
                    std::cout << enhancement_result.toString() << std::endl;
                }
            } else {
                // Enhanced error reporting for false positives
                if (enhancement_result.success && enhancement_result.final_component_count >= initial_component_count) {
                    logGGCEDebug("GGCE false positive detected: claimed success but no component reduction " +
                               std::to_string(initial_component_count) + " -> " +
                               std::to_string(enhancement_result.final_component_count));
                } else {
                    logGGCEDebug("GGCE enhancement failed: " +
                               (enhancement_result.debug_log.empty() ? "No debug information" : enhancement_result.debug_log.back()));
                }
            }
        }

    } catch (const std::exception& e) {
        handleGGCEFailure(std::string("GGCE enhancement error: ") + e.what());
        return false;
    }

    return any_enhancement_successful;
}

std::vector<RankingSubgraph> EnhancedStateManager::extractGraphComponents() const {
    std::vector<RankingSubgraph> components;

    if (!out_state_.empty()) {
        // Use the first (best) output state
        const auto& best_state = out_state_[0];
        components = best_state.graph_;
    }

    return components;
}

void EnhancedStateManager::updateStateAfterGGCE(const std::vector<RankingSubgraph>& enhanced_components) {
    if (out_state_.empty()) {
        return;
    }

    // Update the first (best) output state with enhanced components
    auto& best_state = out_state_[0];
    best_state.graph_ = enhanced_components;

    // Recalculate state score
    best_state.UpdateStateScore();

    // Update true_node_ vector to reflect new connectivity
    std::fill(best_state.true_node_.begin(), best_state.true_node_.end(), false);
    for (const auto& component : enhanced_components) {
        for (int i = 0; i < component.node_.size(); ++i) {
            if (component.node_[i] && i < best_state.true_node_.size()) {
                best_state.true_node_[i] = true;
            }
        }
    }
}

void EnhancedStateManager::logGGCEDebug(const std::string& message) const {
    if (ggce_config_.debug_mode || ggce_config_.verbose_logging) {
        std::cout << "EnhancedStateManager: " << message << std::endl;
    }
}

void EnhancedStateManager::handleGGCEFailure(const std::string& error_message) {
    last_ggce_error_ = error_message;
    ggce_fallback_mode_ = true;

    if (ggce_config_.debug_mode) {
        std::cout << "GGCE Error: " << error_message << " (fallback mode enabled)" << std::endl;
    }
}

void EnhancedStateManager::updateGGCEStatistics(const GlobalGraphConnectivityEngine::EnhancementResult& result) {
    ggce_stats_.total_assemblies_processed++;

    if (result.success) {
        ggce_stats_.assemblies_enhanced++;
        ggce_stats_.successful_merges += (result.initial_component_count - result.final_component_count);
    } else {
        ggce_stats_.failed_merges++;
    }

    ggce_stats_.total_components_before_enhancement += result.initial_component_count;
    ggce_stats_.total_components_after_enhancement += result.final_component_count;
    ggce_stats_.total_processing_time_ms += result.processing_time.count();

    // Update average connectivity improvement
    if (ggce_stats_.assemblies_enhanced > 0) {
        double total_improvement = (ggce_stats_.total_components_before_enhancement -
                                   ggce_stats_.total_components_after_enhancement);
        ggce_stats_.average_connectivity_improvement = total_improvement / ggce_stats_.assemblies_enhanced;
    }
}

const GGCEConfiguration& EnhancedStateManager::getGGCEConfiguration() const {
    return ggce_config_;
}

void EnhancedStateManager::updateGGCEConfiguration(const GGCEConfiguration& config) {
    if (!config.validate()) {
        throw std::invalid_argument("Invalid GGCE configuration");
    }

    ggce_config_ = config;

    if (ggce_engine_) {
        ggce_engine_->updateConfiguration(config);
    }

    logGGCEDebug("GGCE configuration updated");
}

void EnhancedStateManager::setGGCEEnabled(bool enabled) {
    ggce_enabled_ = enabled;
    ggce_config_.enabled = enabled;

    if (enabled && !ggce_engine_) {
        initializeGGCE();
    }

    logGGCEDebug(enabled ? "GGCE enabled" : "GGCE disabled");
}

void EnhancedStateManager::resetGGCEStatistics() {
    ggce_stats_ = GGCEStatistics();
    logGGCEDebug("GGCE statistics reset");
}

std::string EnhancedStateManager::GGCEStatistics::toString() const {
    std::ostringstream oss;
    oss << "GGCE Statistics:\n";
    oss << "  Assemblies Processed: " << total_assemblies_processed << "\n";
    oss << "  Assemblies Enhanced: " << assemblies_enhanced << "\n";
    oss << "  Success Rate: " << (total_assemblies_processed > 0 ?
                                  (100.0 * assemblies_enhanced / total_assemblies_processed) : 0.0) << "%\n";
    oss << "  Components Before: " << total_components_before_enhancement << "\n";
    oss << "  Components After: " << total_components_after_enhancement << "\n";
    oss << "  Average Improvement: " << average_connectivity_improvement << "\n";
    oss << "  Successful Merges: " << successful_merges << "\n";
    oss << "  Failed Merges: " << failed_merges << "\n";
    oss << "  Total Processing Time: " << total_processing_time_ms << "ms\n";
    oss << "  Average Processing Time: " << (total_assemblies_processed > 0 ?
                                            (total_processing_time_ms / total_assemblies_processed) : 0.0) << "ms";
    return oss.str();
}

//############################## Factory Functions ##############################//

std::unique_ptr<StateManager> createStateManager(int N,
                                                int s,
                                                std::vector<Geom>& shard,
                                                std::list<LCSIndex>& LCS_out,
                                                int step_size,
                                                const std::string& log_path) {
    // Check if GGCE should be enabled
    GGCEConfiguration config = GGCEConfiguration::loadFromEnvironment();

    if (config.enabled) {
        return std::make_unique<EnhancedStateManager>(N, s, shard, LCS_out, step_size, log_path);
    } else {
        return std::make_unique<StateManager>(N, s, shard, LCS_out, step_size, log_path);
    }
}

std::unique_ptr<StateManager> createStateManager(int step_size, std::string& log_path) {
    // Check if GGCE should be enabled
    GGCEConfiguration config = GGCEConfiguration::loadFromEnvironment();

    if (config.enabled) {
        return std::make_unique<EnhancedStateManager>(step_size, log_path);
    } else {
        return std::make_unique<StateManager>(step_size, log_path);
    }
}

//############################## GGCE Utilities Implementation ##############################//

namespace GGCEUtils {

std::vector<RankingSubgraph> extractComponents(const State& state) {
    return state.graph_;
}

void updateStateWithComponents(State& state, const std::vector<RankingSubgraph>& components) {
    // This is a const_cast workaround for the State interface
    State& mutable_state = const_cast<State&>(state);
    mutable_state.graph_ = components;
    mutable_state.UpdateStateScore();
}

bool validateComponentConsistency(const std::vector<RankingSubgraph>& components) {
    // Check that components have valid structure
    for (const auto& component : components) {
        if (component.node_.empty()) {
            return false;
        }

        // Check that at least one piece is in the component
        bool has_pieces = std::any_of(component.node_.begin(), component.node_.end(),
                                     [](bool has_piece) { return has_piece; });
        if (!has_pieces) {
            return false;
        }

        // Check transformation matrix consistency
        if (component.T_.size() != component.node_.size()) {
            return false;
        }
    }

    return true;
}

ConnectivitySummary calculateConnectivitySummary(const std::vector<RankingSubgraph>& components) {
    ConnectivitySummary summary;
    summary.total_components = static_cast<int>(components.size());
    summary.total_pieces = 0;

    for (const auto& component : components) {
        for (bool has_piece : component.node_) {
            if (has_piece) {
                summary.total_pieces++;
            }
        }
    }

    if (summary.total_pieces > 1) {
        // Connectivity ratio: (total_pieces - components) / (total_pieces - 1)
        // This measures how well connected the assembly is (1.0 = fully connected)
        summary.connectivity_ratio = static_cast<double>(summary.total_pieces - summary.total_components) /
                                    static_cast<double>(summary.total_pieces - 1);

        // Fragmentation score: components / total_pieces (0.0 = no fragmentation)
        summary.fragmentation_score = static_cast<double>(summary.total_components) /
                                     static_cast<double>(summary.total_pieces);
    } else {
        summary.connectivity_ratio = 1.0;
        summary.fragmentation_score = 0.0;
    }

    return summary;
}

std::string ConnectivitySummary::toString() const {
    std::ostringstream oss;
    oss << "Connectivity Summary:\n";
    oss << "  Components: " << total_components << "\n";
    oss << "  Total Pieces: " << total_pieces << "\n";
    oss << "  Connectivity Ratio: " << connectivity_ratio << "\n";
    oss << "  Fragmentation Score: " << fragmentation_score;
    return oss.str();
}

std::string generateAssemblyReport(const std::vector<RankingSubgraph>& components,
                                  const GlobalGraphConnectivityEngine::EnhancementResult& ggce_result) {
    std::ostringstream oss;

    auto summary = calculateConnectivitySummary(components);

    oss << "=== Assembly Report ===\n";
    oss << summary.toString() << "\n\n";
    oss << ggce_result.toString() << "\n\n";

    oss << "Component Details:\n";
    for (int i = 0; i < components.size(); ++i) {
        oss << "  Component " << i << ": ";

        std::vector<int> pieces_in_component;
        for (int j = 0; j < components[i].node_.size(); ++j) {
            if (components[i].node_[j]) {
                pieces_in_component.push_back(j + 1); // Convert to 1-based
            }
        }

        oss << pieces_in_component.size() << " pieces [";
        for (int j = 0; j < pieces_in_component.size(); ++j) {
            if (j > 0) oss << ", ";
            oss << pieces_in_component[j];
        }
        oss << "]\n";
    }

    return oss.str();
}

} // namespace GGCEUtils

//############################## GGCE Test Framework Implementation ##############################//

GGCETestFramework::TestResult GGCETestFramework::testGGCEOnState(
    const State& state,
    const std::vector<Geom>& shard,
    const std::vector<LCSIndex>& lcs_reference,
    const GGCEConfiguration& config) {

    TestResult result;

    try {
        // Create a copy of components for testing
        std::vector<RankingSubgraph> test_components = state.graph_;
        std::vector<Geom> test_shard = shard;

        result.components_before = static_cast<int>(test_components.size());
        result.ggce_applicable = (test_components.size() > 1);

        if (!result.ggce_applicable) {
            result.error_message = "Single component - GGCE not applicable";
            return result;
        }

        // Create GGCE engine with test configuration
        GlobalGraphConnectivityEngine ggce_engine(config);

        auto start_time = std::chrono::high_resolution_clock::now();

        // Apply GGCE
        auto enhancement_result = ggce_engine.applyGlobalConnectivityEnhancement(
            test_components, test_shard, lcs_reference);

        auto end_time = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        // Record results
        result.ggce_successful = enhancement_result.success;
        result.components_after = enhancement_result.final_component_count;
        result.connectivity_improvement = enhancement_result.connectivity_improvement;
        result.debug_log = enhancement_result.debug_log;

        if (!enhancement_result.success && !enhancement_result.debug_log.empty()) {
            result.error_message = enhancement_result.debug_log.back();
        }

    } catch (const std::exception& e) {
        result.ggce_successful = false;
        result.error_message = std::string("Test exception: ") + e.what();
    }

    return result;
}

std::string GGCETestFramework::TestResult::toString() const {
    std::ostringstream oss;
    oss << "GGCE Test Result:\n";
    oss << "  Applicable: " << (ggce_applicable ? "true" : "false") << "\n";
    oss << "  Successful: " << (ggce_successful ? "true" : "false") << "\n";
    oss << "  Components: " << components_before << " -> " << components_after << "\n";
    oss << "  Improvement: " << connectivity_improvement << "\n";
    oss << "  Processing Time: " << processing_time_ms << "ms\n";

    if (!error_message.empty()) {
        oss << "  Error: " << error_message << "\n";
    }

    if (!debug_log.empty()) {
        oss << "  Debug Log:\n";
        for (const auto& log_entry : debug_log) {
            oss << "    " << log_entry << "\n";
        }
    }

    return oss.str();
}

//############################## GGCE Performance Monitor Implementation ##############################//

void GGCEPerformanceMonitor::recordResult(const GlobalGraphConnectivityEngine::EnhancementResult& result) {
    recent_results_.push_back(result);

    // Keep only recent results for performance
    if (recent_results_.size() > MAX_RECENT_RESULTS) {
        recent_results_.erase(recent_results_.begin());
    }

    // Update running metrics
    current_metrics_.total_assemblies_processed++;
    current_metrics_.last_updated = std::chrono::system_clock::now();

    // Calculate averages
    double total_time = 0.0;
    double total_improvement = 0.0;
    int successful_count = 0;

    for (const auto& res : recent_results_) {
        total_time += res.processing_time.count();
        if (res.success) {
            total_improvement += res.connectivity_improvement;
            successful_count++;
        }
    }

    current_metrics_.average_processing_time_ms = total_time / recent_results_.size();
    current_metrics_.success_rate = static_cast<double>(successful_count) / recent_results_.size();

    if (successful_count > 0) {
        current_metrics_.connectivity_improvement_ratio = total_improvement / successful_count;
    }
}

bool GGCEPerformanceMonitor::detectRegression(const PerformanceMetrics& baseline) const {
    // Check for performance regression
    if (current_metrics_.average_processing_time_ms > baseline.average_processing_time_ms * (1.0 + REGRESSION_THRESHOLD)) {
        return true;
    }

    // Check for quality regression
    if (current_metrics_.connectivity_improvement_ratio < baseline.connectivity_improvement_ratio * (1.0 - REGRESSION_THRESHOLD)) {
        return true;
    }

    // Check for success rate regression
    if (current_metrics_.success_rate < baseline.success_rate * (1.0 - REGRESSION_THRESHOLD)) {
        return true;
    }

    return false;
}

void GGCEPerformanceMonitor::saveMetrics(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << "# GGCE Performance Metrics\n";
        file << "average_processing_time_ms=" << current_metrics_.average_processing_time_ms << "\n";
        file << "connectivity_improvement_ratio=" << current_metrics_.connectivity_improvement_ratio << "\n";
        file << "success_rate=" << current_metrics_.success_rate << "\n";
        file << "total_assemblies_processed=" << current_metrics_.total_assemblies_processed << "\n";

        auto time_t = std::chrono::system_clock::to_time_t(current_metrics_.last_updated);
        file << "last_updated=" << time_t << "\n";

        file.close();
    }
}

GGCEPerformanceMonitor::PerformanceMetrics GGCEPerformanceMonitor::loadMetrics(const std::string& filepath) {
    PerformanceMetrics metrics;

    std::ifstream file(filepath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                if (key == "average_processing_time_ms") {
                    metrics.average_processing_time_ms = std::stod(value);
                } else if (key == "connectivity_improvement_ratio") {
                    metrics.connectivity_improvement_ratio = std::stod(value);
                } else if (key == "success_rate") {
                    metrics.success_rate = std::stod(value);
                } else if (key == "total_assemblies_processed") {
                    metrics.total_assemblies_processed = std::stoi(value);
                } else if (key == "last_updated") {
                    std::time_t time_t = std::stoll(value);
                    metrics.last_updated = std::chrono::system_clock::from_time_t(time_t);
                }
            }
        }
        file.close();
    }

    return metrics;
}

// CRITICAL FIX #3: Hub-guided graph building implementation
void EnhancedStateManager::setHubGuidanceEnabled(bool enabled) {
    hub_guided_building_enabled_ = enabled;
    if (enabled && ggce_config_.debug_mode) {
        std::cout << "*** HUB-GUIDED BUILDING ENABLED *** Enhanced graph merging with persistence" << std::endl;
    }
}