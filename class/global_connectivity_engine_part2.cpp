#include "global_connectivity_engine.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>

//############################## Merge Validation Framework Implementation ##############################//

MergeValidationFramework::MergeValidationFramework(const GGCEConfiguration& config)
    : config_(config) {

    // Initialize intersection detector with GGCE configuration
    intersection_detector_ = std::make_unique<IntersectionDetector>();
}

ValidationResult MergeValidationFramework::validateMerge(
    const BridgeCandidate& candidate,
    const std::vector<RankingSubgraph>& current_state,
    const std::vector<Geom>& shard) {

    ValidationResult result;

    try {
        // 1. Geometric consistency check
        result.geometric_analysis = performGeometricAnalysis(candidate, shard);

        // 2. Transformation chain validation
        result.consistency_check = validateTransformationConsistency(candidate, current_state);

        // 3. Enhanced intersection detection
        result.intersection_analysis = performIntersectionAnalysis(candidate, shard);

        // 4. Global connectivity impact assessment
        result.connectivity_impact = assessConnectivityImpact(candidate, current_state);

        // Composite validation decision
        result.is_valid = (
            result.geometric_analysis.passed &&
            result.consistency_check.passed &&
            result.intersection_analysis.passed &&
            result.connectivity_impact.net_benefit > 0.1  // Minimum benefit threshold
        );

        // Calculate confidence score
        if (result.is_valid) {
            result.confidence_score = (
                result.geometric_analysis.overlap_score * 0.3 +
                result.consistency_check.chain_consistency * 0.3 +
                result.intersection_analysis.surface_proximity_score * 0.2 +
                result.connectivity_impact.connectivity_improvement * 0.2
            );
        } else {
            result.confidence_score = 0.0;

            // Set rejection reason based on which validation failed
            if (!result.geometric_analysis.passed) {
                result.rejection_reason = "Geometric analysis failed";
            } else if (!result.consistency_check.passed) {
                result.rejection_reason = "Transformation consistency failed";
            } else if (!result.intersection_analysis.passed) {
                result.rejection_reason = "Intersection detected";
            } else {
                result.rejection_reason = "Insufficient connectivity benefit";
            }
        }

    } catch (const std::exception& e) {
        result.is_valid = false;
        result.confidence_score = 0.0;
        result.rejection_reason = std::string("Validation exception: ") + e.what();
    }

    return result;
}

ValidationResult::GeometricAnalysis MergeValidationFramework::performGeometricAnalysis(
    const BridgeCandidate& candidate,
    const std::vector<Geom>& shard) {

    ValidationResult::GeometricAnalysis analysis;
    analysis.passed = false;

    // Bounds checking
    int piece_i_idx = candidate.source_piece_id - 1; // Convert to 0-based
    int piece_j_idx = candidate.target_piece_id - 1;

    if (piece_i_idx < 0 || piece_i_idx >= shard.size() ||
        piece_j_idx < 0 || piece_j_idx >= shard.size()) {
        return analysis;
    }

    const auto& geom_i = shard[piece_i_idx];
    const auto& geom_j = shard[piece_j_idx];

    if (geom_i.edge_line_.point_.cols() == 0 || geom_j.edge_line_.point_.cols() == 0) {
        return analysis;
    }

    // Calculate overlap score based on spatial proximity
    double min_distance = std::numeric_limits<double>::max();
    double avg_distance = 0.0;
    int proximity_count = 0;

    // Sample points for performance (take every 10th point)
    int sample_step_i = std::max(1, static_cast<int>(geom_i.edge_line_.point_.cols()) / 100);
    int sample_step_j = std::max(1, static_cast<int>(geom_j.edge_line_.point_.cols()) / 100);

    for (int i = 0; i < geom_i.edge_line_.point_.cols(); i += sample_step_i) {
        for (int j = 0; j < geom_j.edge_line_.point_.cols(); j += sample_step_j) {
            double distance = (geom_i.edge_line_.point_.col(i) - geom_j.edge_line_.point_.col(j)).norm();
            min_distance = std::min(min_distance, distance);

            if (distance < config_.surface_proximity_threshold) {
                avg_distance += distance;
                proximity_count++;
            }
        }
    }

    // Calculate scores
    if (proximity_count > 0) {
        avg_distance /= proximity_count;
        analysis.overlap_score = std::exp(-avg_distance / config_.surface_proximity_threshold);
        analysis.alignment_score = candidate.geometric_confidence;
        analysis.surface_compatibility = 1.0 - (avg_distance / config_.surface_proximity_threshold);

        analysis.passed = (analysis.overlap_score > 0.5 &&
                          analysis.surface_compatibility > 0.3);
    }

    return analysis;
}

ValidationResult::ConsistencyCheck MergeValidationFramework::validateTransformationConsistency(
    const BridgeCandidate& candidate,
    const std::vector<RankingSubgraph>& current_state) {

    ValidationResult::ConsistencyCheck check;
    check.passed = false;

    try {
        // Bounds checking
        if (candidate.source_graph_index < 0 || candidate.source_graph_index >= current_state.size() ||
            candidate.target_graph_index < 0 || candidate.target_graph_index >= current_state.size()) {
            return check;
        }

        const auto& source_graph = current_state[candidate.source_graph_index];
        const auto& target_graph = current_state[candidate.target_graph_index];

        // Get transformation matrices for the pieces involved
        int source_piece_idx = candidate.source_piece_id - 1; // Convert to 0-based
        int target_piece_idx = candidate.target_piece_id - 1;

        if (source_piece_idx < 0 || source_piece_idx >= source_graph.T_.size() ||
            target_piece_idx < 0 || target_piece_idx >= target_graph.T_.size()) {
            return check;
        }

        // Calculate transformation error (simplified for now)
        Matrix3d R_source, R_target;
        Vector3d t_source, t_target;

        source_graph.T_[source_piece_idx].Output(R_source, t_source);
        target_graph.T_[target_piece_idx].Output(R_target, t_target);

        // Calculate relative transformation quality
        double rotation_error = (R_source - R_target).norm();
        double translation_error = (t_source - t_target).norm();

        check.transformation_error = rotation_error + translation_error / 100.0; // Normalize translation
        check.chain_consistency = std::exp(-check.transformation_error);
        check.global_alignment_error = check.transformation_error;

        // Pass if error is reasonable
        check.passed = (check.transformation_error < 2.0); // Threshold for acceptable error

    } catch (const std::exception& e) {
        // Handle any exceptions gracefully
        check.passed = false;
    }

    return check;
}

ValidationResult::IntersectionAnalysis MergeValidationFramework::performIntersectionAnalysis(
    const BridgeCandidate& candidate,
    const std::vector<Geom>& shard) {

    ValidationResult::IntersectionAnalysis analysis;
    analysis.passed = false;

    try {
        // Bounds checking
        int piece_i_idx = candidate.source_piece_id - 1;
        int piece_j_idx = candidate.target_piece_id - 1;

        if (piece_i_idx < 0 || piece_i_idx >= shard.size() ||
            piece_j_idx < 0 || piece_j_idx >= shard.size()) {
            return analysis;
        }

        // Use existing intersection detector
        // This is a simplified version - the actual implementation would use
        // the full IntersectionDetector functionality

        const auto& geom_i = shard[piece_i_idx];
        const auto& geom_j = shard[piece_j_idx];

        if (geom_i.edge_line_.point_.cols() == 0 || geom_j.edge_line_.point_.cols() == 0) {
            analysis.passed = true; // No geometry means no intersection
            return analysis;
        }

        // Simple proximity-based intersection check
        int close_points = 0;
        int opposing_normals = 0;
        double total_distance = 0.0;

        int sample_step = std::max(1, static_cast<int>(geom_i.edge_line_.point_.cols()) / 50);
        int sample_count = 0;

        for (int i = 0; i < geom_i.edge_line_.point_.cols(); i += sample_step) {
            for (int j = 0; j < geom_j.edge_line_.point_.cols(); j += sample_step) {
                double distance = (geom_i.edge_line_.point_.col(i) - geom_j.edge_line_.point_.col(j)).norm();

                if (distance < config_.surface_proximity_threshold) {
                    close_points++;
                    total_distance += distance;
                }
                sample_count++;
            }
        }

        // Calculate analysis results
        if (sample_count > 0) {
            analysis.volume_overlap_ratio = static_cast<double>(close_points) / sample_count;
            analysis.point_containment_ratio = analysis.volume_overlap_ratio; // Simplified
            analysis.surface_proximity_score = close_points > 0 ? (total_distance / close_points) : 0.0;
            analysis.opposing_normals_count = opposing_normals;

            // Pass if overlap is within acceptable limits
            analysis.passed = (analysis.volume_overlap_ratio < config_.max_volume_overlap_ratio &&
                              analysis.opposing_normals_count == 0);
        } else {
            analysis.passed = true; // No close points means no intersection
        }

    } catch (const std::exception& e) {
        analysis.passed = false;
    }

    return analysis;
}

ValidationResult::ConnectivityImpact MergeValidationFramework::assessConnectivityImpact(
    const BridgeCandidate& candidate,
    const std::vector<RankingSubgraph>& current_state) {

    ValidationResult::ConnectivityImpact impact;

    // Calculate current state metrics
    int current_components = static_cast<int>(current_state.size());

    // Merging these two graphs would reduce component count by 1
    impact.components_reduction = 1;
    impact.connectivity_improvement = 1.0 / current_components; // Higher benefit for more fragmented states

    // Quality impact assessment (simplified)
    impact.quality_impact = candidate.geometric_confidence - 0.5; // Relative to baseline

    // Net benefit calculation
    impact.net_benefit = impact.connectivity_improvement * config_.connectivity_weight +
                        impact.quality_impact * config_.quality_weight;

    return impact;
}

std::string ValidationResult::toString() const {
    std::ostringstream oss;
    oss << "ValidationResult{\n";
    oss << "  Valid: " << (is_valid ? "true" : "false") << "\n";
    oss << "  Confidence: " << confidence_score << "\n";
    if (!rejection_reason.empty()) {
        oss << "  Rejection: " << rejection_reason << "\n";
    }
    oss << "  Geometric: passed=" << (geometric_analysis.passed ? "Y" : "N")
        << ", overlap=" << geometric_analysis.overlap_score << "\n";
    oss << "  Consistency: passed=" << (consistency_check.passed ? "Y" : "N")
        << ", error=" << consistency_check.transformation_error << "\n";
    oss << "  Intersection: passed=" << (intersection_analysis.passed ? "Y" : "N")
        << ", overlap=" << intersection_analysis.volume_overlap_ratio << "\n";
    oss << "  Connectivity: benefit=" << connectivity_impact.net_benefit
        << ", reduction=" << connectivity_impact.components_reduction << "\n";
    oss << "}";
    return oss.str();
}

//############################## Inter-Graph Merge Engine Implementation ##############################//

InterGraphMergeEngine::InterGraphMergeEngine(const GGCEConfiguration& config)
    : config_(config) {

    validation_framework_ = std::make_unique<MergeValidationFramework>(config_);
}

InterGraphMergeEngine::MergeResult InterGraphMergeEngine::executeMerges(
    std::vector<RankingSubgraph>& components,
    const std::vector<BridgeCandidate>& candidates,
    std::vector<Geom>& shard) {

    auto start_time = std::chrono::high_resolution_clock::now();
    MergeResult result;
    result.merges_attempted = 0;
    result.merges_successful = 0;
    result.final_component_count = static_cast<int>(components.size());

    if (candidates.empty()) {
        result.success = true;
        result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_time);
        return result;
    }

    // Plan optimal merge sequence
    auto merge_sequence = planOptimalMergeSequence(candidates);

    if (config_.verbose_logging) {
        std::cout << "GGCE: Planned merge sequence with " << merge_sequence.ordered_merges.size()
                  << " merges, predicted improvement: " << merge_sequence.predicted_improvement << std::endl;
    }

    // Execute merges in planned order
    int iteration = 0;
    for (const auto& candidate : merge_sequence.ordered_merges) {
        if (iteration >= config_.max_merge_iterations) {
            result.merge_log.push_back("Maximum iterations reached");
            break;
        }

        result.merges_attempted++;

        // Validate merge before execution
        auto validation = validation_framework_->validateMerge(candidate, components, shard);

        if (!validation.is_valid) {
            result.merge_log.push_back("Merge rejected: " + validation.rejection_reason);
            if (config_.verbose_logging) {
                std::cout << "GGCE: Merge " << candidate.source_graph_index
                          << "->" << candidate.target_graph_index
                          << " rejected: " << validation.rejection_reason << std::endl;
            }
            continue;
        }

        // Execute the merge
        bool merge_success = executeSingleMerge(components, candidate, shard);

        if (merge_success) {
            result.merges_successful++;
            result.merge_log.push_back("Successfully merged graphs " +
                                     std::to_string(candidate.source_graph_index) + "->" +
                                     std::to_string(candidate.target_graph_index));

            if (config_.verbose_logging) {
                std::cout << "GGCE: Successfully merged graphs "
                          << candidate.source_graph_index << "->" << candidate.target_graph_index << std::endl;
            }
        } else {
            result.merge_log.push_back("Merge execution failed for graphs " +
                                     std::to_string(candidate.source_graph_index) + "->" +
                                     std::to_string(candidate.target_graph_index));
        }

        iteration++;
    }

    // Calculate final results
    result.final_component_count = static_cast<int>(components.size());
    result.connectivity_improvement = static_cast<double>(candidates.size() - result.final_component_count) /
                                     std::max(1.0, static_cast<double>(candidates.size()));
    result.success = (result.merges_successful > 0);

    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    return result;
}

bool InterGraphMergeEngine::executeSingleMerge(
    std::vector<RankingSubgraph>& components,
    const BridgeCandidate& candidate,
    std::vector<Geom>& shard) {

    try {
        // Bounds checking
        if (candidate.source_graph_index < 0 || candidate.source_graph_index >= components.size() ||
            candidate.target_graph_index < 0 || candidate.target_graph_index >= components.size() ||
            candidate.source_graph_index == candidate.target_graph_index) {
            return false;
        }

        auto& source_graph = components[candidate.source_graph_index];
        auto& target_graph = components[candidate.target_graph_index];

        // Merge target graph into source graph
        for (int i = 0; i < target_graph.node_.size() && i < source_graph.node_.size(); ++i) {
            if (target_graph.node_[i]) {
                source_graph.node_[i] = true;

                // Copy transformation if needed
                if (i < target_graph.T_.size() && i < source_graph.T_.size()) {
                    source_graph.T_[i] = target_graph.T_[i];
                }
            }
        }

        // Update graph score (simple combination)
        source_graph.graph_score_ += target_graph.graph_score_;
        source_graph.pc_score_ = std::max(source_graph.pc_score_, target_graph.pc_score_);

        // Remove the target graph (merge consumed it)
        components.erase(components.begin() + candidate.target_graph_index);

        // Update remaining candidate indices if needed
        updateStateAfterMerge(components, candidate.source_graph_index, candidate.target_graph_index);

        return true;

    } catch (const std::exception& e) {
        if (config_.debug_mode) {
            std::cout << "GGCE Error in executeSingleMerge: " << e.what() << std::endl;
        }
        return false;
    }
}

InterGraphMergeEngine::MergeSequence InterGraphMergeEngine::planOptimalMergeSequence(
    const std::vector<BridgeCandidate>& candidates) {

    MergeSequence sequence;

    // Simple greedy approach: sort by combined score
    sequence.ordered_merges = candidates;
    std::sort(sequence.ordered_merges.begin(), sequence.ordered_merges.end(),
              [](const BridgeCandidate& a, const BridgeCandidate& b) {
                  return a.combined_score > b.combined_score;
              });

    // Calculate predicted improvement
    sequence.predicted_improvement = 0.0;
    for (const auto& candidate : sequence.ordered_merges) {
        sequence.predicted_improvement += candidate.connectivity_score;
    }

    return sequence;
}

void InterGraphMergeEngine::updateStateAfterMerge(
    std::vector<RankingSubgraph>& components,
    int merged_source_index,
    int merged_target_index) {

    // After removing target graph, update any references to graphs with indices > merged_target_index
    // This is important for maintaining consistency in subsequent merge operations

    // For now, this is a placeholder - in a full implementation, we would need to update
    // any data structures that reference graph indices

    if (config_.debug_mode) {
        std::cout << "GGCE: Updated state after merging graph " << merged_target_index
                  << " into graph " << merged_source_index
                  << ", remaining components: " << components.size() << std::endl;
    }
}

//############################## Connectivity Priority Manager Implementation ##############################//

ConnectivityPriorityManager::ConnectivityPriorityManager(const GGCEConfiguration& config)
    : config_(config) {
}

void ConnectivityPriorityManager::updatePriorities(State& state) {
    double fragmentation_penalty = calculateFragmentationPenalty(state);

    if (fragmentation_penalty > FRAGMENTATION_THRESHOLD) {
        // Boost connectivity-promoting candidates
        boostConnectivityCandidates(state, fragmentation_penalty);

        // Temporarily relax quality thresholds for connectivity gains
        adjustQualityThresholds(state, fragmentation_penalty);
    }
}

double ConnectivityPriorityManager::calculateFragmentationPenalty(const State& state) {
    int component_count = static_cast<int>(state.graph_.size());

    if (component_count <= 1) {
        return 0.0; // No fragmentation
    }

    // Calculate total pieces
    int total_pieces = 0;
    for (const auto& graph : state.graph_) {
        for (bool has_piece : graph.node_) {
            if (has_piece) total_pieces++;
        }
    }

    if (total_pieces <= 1) {
        return 0.0;
    }

    // Penalty increases with number of components relative to total pieces
    return static_cast<double>(component_count - 1) / static_cast<double>(total_pieces);
}

void ConnectivityPriorityManager::boostConnectivityCandidates(State& state, double fragmentation_penalty) {
    // This would boost priority scores for connections that reduce fragmentation
    // For now, this is a placeholder that would integrate with the existing priority system

    if (config_.debug_mode) {
        std::cout << "GGCE: Boosting connectivity candidates with fragmentation penalty: "
                  << fragmentation_penalty << std::endl;
    }

    // In a full implementation, this would modify the priority scores in state.total_priority_
    // based on which connections would reduce the number of components
}

void ConnectivityPriorityManager::adjustQualityThresholds(State& state, double fragmentation_penalty) {
    // Temporarily lower quality requirements when fragmentation is high
    // This allows lower-quality connections if they significantly improve connectivity

    if (config_.debug_mode) {
        std::cout << "GGCE: Adjusting quality thresholds due to fragmentation: "
                  << fragmentation_penalty << std::endl;
    }

    // In a full implementation, this would modify quality thresholds used in validation
}

//############################## Main GGCE System Implementation (continued) ##############################//

GlobalGraphConnectivityEngine::EnhancementResult GlobalGraphConnectivityEngine::applyGlobalConnectivityEnhancement(
    std::vector<RankingSubgraph>& components,
    std::vector<Geom>& shard,
    const std::vector<LCSIndex>& lcs_reference) {

    auto start_time = std::chrono::high_resolution_clock::now();
    EnhancementResult result;

    result.initial_component_count = static_cast<int>(components.size());
    result.final_component_count = result.initial_component_count;

    try {
        logDebugInfo("Starting global connectivity enhancement");

        // Early exit if enhancement not needed
        if (!shouldApplyGlobalEnhancement(components)) {
            result.success = true;
            result.debug_log.push_back("Enhancement not needed (single component or below threshold)");
            result.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            return result;
        }

        // Phase 1: Analyze current connectivity state
        auto initial_metrics = connectivity_analyzer_->calculateMetrics(components);
        result.debug_log.push_back("Initial metrics: " + std::to_string(initial_metrics.total_components) +
                                  " components, fragmentation=" + std::to_string(initial_metrics.fragmentation_penalty));

        // Phase 2: Detect inter-component bridges
        logDebugInfo("Detecting potential bridges between components");
        auto bridge_candidates = connectivity_analyzer_->detectPotentialBridges(
            components, shard, lcs_reference);

        result.debug_log.push_back("Found " + std::to_string(bridge_candidates.size()) + " bridge candidates");

        if (bridge_candidates.empty()) {
            result.success = true;
            result.debug_log.push_back("No viable bridge candidates found");
            result.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            return result;
        }

        // Phase 3: Execute validated merges
        logDebugInfo("Executing merge operations");
        auto merge_result = merge_engine_->executeMerges(components, bridge_candidates, shard);

        // Phase 4: Calculate improvement metrics
        auto final_metrics = connectivity_analyzer_->calculateMetrics(components);

        result.final_component_count = final_metrics.total_components;
        result.connectivity_improvement = static_cast<double>(result.initial_component_count - result.final_component_count) /
                                         std::max(1.0, static_cast<double>(result.initial_component_count - 1));
        result.geometric_quality_change = final_metrics.geometric_quality_score - initial_metrics.geometric_quality_score;

        result.success = (merge_result.merges_successful > 0);

        // Combine debug logs
        result.debug_log.insert(result.debug_log.end(), merge_result.merge_log.begin(), merge_result.merge_log.end());

        // Log final results
        result.debug_log.push_back("Enhancement completed: " +
                                  std::to_string(result.initial_component_count) + " -> " +
                                  std::to_string(result.final_component_count) + " components");

        logPerformanceMetrics(result);

    } catch (const std::exception& e) {
        result.success = false;
        result.debug_log.push_back("Enhancement failed with exception: " + std::string(e.what()));

        if (config_.debug_mode) {
            std::cout << "GGCE Enhancement Error: " << e.what() << std::endl;
        }
    }

    result.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    return result;
}

void GlobalGraphConnectivityEngine::logPerformanceMetrics(const EnhancementResult& result) {
    if (config_.verbose_logging) {
        std::cout << result.toString() << std::endl;
    }

    // Log key metrics for monitoring
    if (config_.debug_mode) {
        std::cout << "GGCE Performance Summary:" << std::endl;
        std::cout << "  Processing Time: " << result.processing_time.count() << "ms" << std::endl;
        std::cout << "  Connectivity Improvement: " << result.connectivity_improvement << std::endl;
        std::cout << "  Component Reduction: " << (result.initial_component_count - result.final_component_count) << std::endl;
    }
}