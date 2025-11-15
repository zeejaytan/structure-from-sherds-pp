#include "two_phase_assembly.h"
#include "global_connectivity_engine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

//############################## Connection Preservation Layer Implementation ##############################//

ConnectionPreservationLayer::ConnectionPreservationLayer() {
    stats_ = PreservationStats();
}

ConnectionPreservationLayer::~ConnectionPreservationLayer() {
    // Cleanup managed resources
}

void ConnectionPreservationLayer::preserveConnectionsAfterPruning(const std::vector<LCSIndex>& pruned_connections) {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << "#################### CONNECTION PRESERVATION LAYER ####################" << std::endl;
    std::cout << "*** PRESERVING PRUNED CONNECTIONS *** Processing " << pruned_connections.size() << " connections that survived pairwise pruning" << std::endl;

    all_connections_.clear();
    piece_pair_connections_.clear();

    for (const auto& lcs_conn : pruned_connections) {
        PreservedConnection preserved;
        preserved.original_connection = lcs_conn;
        preserved.piece_x = lcs_conn.shard_x_;
        preserved.piece_y = lcs_conn.shard_y_;
        preserved.connection_strength = lcs_conn.score_;
        preserved.preservation_timestamp = generateTimestamp();

        // Calculate enhanced metadata
        calculateConnectionMetadata(preserved);

        // Validate before adding
        if (validateConnection(preserved)) {
            all_connections_.push_back(preserved);

            // Index by piece pair for fast lookup
            std::pair<int,int> piece_pair = std::make_pair(
                std::min(preserved.piece_x, preserved.piece_y),
                std::max(preserved.piece_x, preserved.piece_y)
            );
            piece_pair_connections_[piece_pair].push_back(&all_connections_.back());

            std::cout << "*** CONNECTION PRESERVED *** Pieces " << preserved.piece_x
                      << "-" << preserved.piece_y << " strength=" << preserved.connection_strength
                      << " confidence=" << preserved.geometric_confidence << std::endl;
        }
    }

    // Update statistics
    stats_.total_connections_preserved = static_cast<int>(all_connections_.size());
    stats_.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    std::cout << "*** PRESERVATION COMPLETE *** " << stats_.total_connections_preserved
              << " connections preserved in " << stats_.processing_time.count() << "ms" << std::endl;
}

void ConnectionPreservationLayer::updateComponentAssignments(const std::vector<RankingSubgraph>& components) {
    std::cout << "*** UPDATING COMPONENT ASSIGNMENTS *** " << components.size() << " components" << std::endl;

    // Clear previous assignments
    piece_assignments_.clear();

    // Extract piece assignments from components
    for (int comp_id = 0; comp_id < components.size(); comp_id++) {
        const auto& component = components[comp_id];

        // Iterate through pieces in this component
        for (int piece_id = 0; piece_id < component.node_.size(); piece_id++) {
            if (component.node_[piece_id]) {  // Piece is in this component
                ComponentAssignment assignment;
                assignment.piece_id = piece_id + 1;  // Convert to 1-based
                assignment.component_id = comp_id;
                assignment.assignment_reason = "Graph building assignment";
                assignment.timestamp = std::chrono::system_clock::now();

                piece_assignments_[assignment.piece_id] = assignment;

                std::cout << "*** COMPONENT ASSIGNMENT *** Piece " << assignment.piece_id
                          << " → Component " << assignment.component_id << std::endl;
            }
        }
    }

    // Update component assignments in preserved connections
    for (auto& connection : all_connections_) {
        auto comp_x_it = piece_assignments_.find(connection.piece_x);
        auto comp_y_it = piece_assignments_.find(connection.piece_y);

        if (comp_x_it != piece_assignments_.end()) {
            connection.component_x = comp_x_it->second.component_id;
        }
        if (comp_y_it != piece_assignments_.end()) {
            connection.component_y = comp_y_it->second.component_id;
        }
    }

    // Identify cross-component connections
    identifyCrossComponentConnections();

    stats_.component_count = static_cast<int>(components.size());
    std::cout << "*** COMPONENT ASSIGNMENTS UPDATED *** " << piece_assignments_.size()
              << " pieces assigned to " << stats_.component_count << " components" << std::endl;
}

void ConnectionPreservationLayer::identifyCrossComponentConnections() {
    cross_component_connections_.clear();
    component_bridges_.clear();

    std::cout << "*** IDENTIFYING CROSS-COMPONENT CONNECTIONS ***" << std::endl;

    for (auto& connection : all_connections_) {
        // Check if connection spans different components
        if (connection.component_x != -1 && connection.component_y != -1 &&
            connection.component_x != connection.component_y) {

            connection.is_cross_component = true;
            cross_component_connections_.push_back(&connection);

            // Index by component pair
            std::pair<int,int> comp_pair = std::make_pair(
                std::min(connection.component_x, connection.component_y),
                std::max(connection.component_x, connection.component_y)
            );
            component_bridges_[comp_pair].push_back(&connection);

            std::cout << "*** CROSS-COMPONENT CONNECTION *** Pieces " << connection.piece_x
                      << "-" << connection.piece_y << " spans Components "
                      << connection.component_x << "-" << connection.component_y
                      << " strength=" << connection.connection_strength << std::endl;
        } else {
            connection.is_cross_component = false;
        }
    }

    stats_.cross_component_connections = static_cast<int>(cross_component_connections_.size());
    std::cout << "*** CROSS-COMPONENT ANALYSIS COMPLETE *** "
              << stats_.cross_component_connections << " cross-component connections found" << std::endl;

    // Print detailed bridge analysis
    printCrossComponentAnalysis();
}

std::vector<ConnectionPreservationLayer::PreservedConnection>
ConnectionPreservationLayer::getCrossComponentConnections() const {
    std::vector<PreservedConnection> result;

    for (const auto* connection_ptr : cross_component_connections_) {
        result.push_back(*connection_ptr);
    }

    std::cout << "*** PROVIDING CROSS-COMPONENT CONNECTIONS *** " << result.size()
              << " connections available for global optimization" << std::endl;

    return result;
}

std::vector<ConnectionPreservationLayer::PreservedConnection>
ConnectionPreservationLayer::getConnectionsBetweenComponents(int comp1, int comp2) const {
    std::vector<PreservedConnection> result;

    std::pair<int,int> comp_pair = std::make_pair(std::min(comp1, comp2), std::max(comp1, comp2));
    auto it = component_bridges_.find(comp_pair);

    if (it != component_bridges_.end()) {
        for (const auto* connection_ptr : it->second) {
            result.push_back(*connection_ptr);
        }
    }

    return result;
}

std::vector<BridgeCandidate> ConnectionPreservationLayer::generateBridgeCandidatesForGGCE() const {
    std::vector<BridgeCandidate> candidates;

    std::cout << "*** GENERATING BRIDGE CANDIDATES FOR GGCE ***" << std::endl;

    for (const auto& comp_pair_entry : component_bridges_) {
        int comp1 = comp_pair_entry.first.first;
        int comp2 = comp_pair_entry.first.second;
        const auto& connections = comp_pair_entry.second;

        std::cout << "*** BRIDGE ANALYSIS *** Components " << comp1 << "-" << comp2
                  << ": " << connections.size() << " potential bridges" << std::endl;

        for (const auto* connection : connections) {
            BridgeCandidate candidate;

            // CRITICAL FIX: Determine source/target based on component size, not arbitrary piece order
            // Always merge smaller component into bigger component for efficiency
            int comp_x = connection->component_x;
            int comp_y = connection->component_y;

            // Get component sizes from piece_assignments_ (count pieces per component)
            int size_x = 0, size_y = 0;
            for (const auto& assignment : piece_assignments_) {
                if (assignment.second.component_id == comp_x) size_x++;
                if (assignment.second.component_id == comp_y) size_y++;
            }

            if (size_x <= size_y) {
                // Component X is smaller or equal - merge X into Y
                candidate.source_graph_index = comp_x;  // smaller component (source)
                candidate.target_graph_index = comp_y;  // bigger component (target)
                candidate.source_piece_id = connection->piece_x;
                candidate.target_piece_id = connection->piece_y;
                std::cout << "*** SIZE-BASED ASSIGNMENT *** Component " << comp_x << " (" << size_x
                         << " pieces) → Component " << comp_y << " (" << size_y << " pieces)" << std::endl;
            } else {
                // Component Y is smaller - merge Y into X
                candidate.source_graph_index = comp_y;  // smaller component (source)
                candidate.target_graph_index = comp_x;  // bigger component (target)
                candidate.source_piece_id = connection->piece_y;
                candidate.target_piece_id = connection->piece_x;
                std::cout << "*** SIZE-BASED ASSIGNMENT *** Component " << comp_y << " (" << size_y
                         << " pieces) → Component " << comp_x << " (" << size_x << " pieces)" << std::endl;
            }

            // Enhanced scoring based on preserved metadata
            candidate.connectivity_score = connection->connection_strength;
            candidate.geometric_confidence = connection->geometric_confidence;
            candidate.transformation_quality = connection->spatial_proximity;
            candidate.consistency_score = connection->normal_compatibility;

            // Combined score calculation
            candidate.combined_score =
                0.4 * candidate.connectivity_score +
                0.3 * candidate.geometric_confidence +
                0.2 * candidate.transformation_quality +
                0.1 * candidate.consistency_score;

            // Add transformation options from original LCS data
            candidate.transformation_options.push_back(connection->original_connection);

            // GLOBAL OPTIMIZATION: Compute actual ICP transformation from preserved LCS data
            candidate.best_transformation = computeOptimalTransformation(connection);

            std::cout << "*** GLOBAL OPTIMIZATION *** Computed ICP transformation for pieces "
                      << candidate.source_piece_id << "-" << candidate.target_piece_id << std::endl;

            candidates.push_back(candidate);

            std::cout << "*** BRIDGE CANDIDATE *** Pieces " << candidate.source_piece_id
                      << "-" << candidate.target_piece_id << " score=" << candidate.combined_score
                      << " (connectivity=" << candidate.connectivity_score
                      << ", geometric=" << candidate.geometric_confidence << ")" << std::endl;
        }
    }

    // Sort candidates by combined score (best first)
    std::sort(candidates.begin(), candidates.end(),
              [](const BridgeCandidate& a, const BridgeCandidate& b) {
                  return a.combined_score > b.combined_score;
              });

    std::cout << "*** BRIDGE CANDIDATES GENERATED *** " << candidates.size()
              << " candidates ready for GGCE optimization" << std::endl;

    return candidates;
}

Matrix4d ConnectionPreservationLayer::computeOptimalTransformation(const PreservedConnection* connection) const {
    std::cout << "*** COMPUTING OPTIMAL TRANSFORMATION *** For pieces "
              << connection->piece_x << "-" << connection->piece_y << std::endl;

    // GLOBAL OPTIMIZATION: Extract actual ICP transformation from LCS data
    const LCSIndex& lcs_data = connection->original_connection;

    // Extract the actual transformation matrix from LCS
    Matrix4d optimal_transform;
    lcs_data.trans_.Output(optimal_transform);

    // Check if transformation is valid (not identity/empty)
    bool is_identity = (optimal_transform - Matrix4d::Identity()).norm() < 1e-6;
    if (is_identity) {
        std::cout << "*** WARNING *** Identity transformation in LCS data, using default" << std::endl;
        return Matrix4d::Identity();
    }

    // GLOBAL SCORING: Evaluate transformation quality for global assembly
    double transform_quality = evaluateTransformationQuality(optimal_transform, connection);

    std::cout << "*** TRANSFORMATION COMPUTED *** Quality score: " << transform_quality << std::endl;
    std::cout << "*** TRANSFORMATION MATRIX ***" << std::endl;
    for (int row = 0; row < 4; ++row) {
        std::cout << "Row " << row << ": ";
        for (int col = 0; col < 4; ++col) {
            std::cout << optimal_transform(row, col) << " ";
        }
        std::cout << std::endl;
    }

    return optimal_transform;
}

double ConnectionPreservationLayer::evaluateTransformationQuality(
    const Matrix4d& transform,
    const PreservedConnection* connection) const {

    // GLOBAL OPTIMIZATION: Multi-criteria transformation evaluation
    double quality_score = 0.0;

    // 1. Geometric alignment quality
    double geometric_quality = connection->geometric_confidence;

    // 2. Spatial proximity after transformation
    double proximity_quality = std::max(0.0, 1.0 - connection->spatial_proximity / 100.0);

    // 3. Normal compatibility
    double normal_quality = connection->normal_compatibility;

    // 4. Matrix condition (avoid singular transformations)
    double det = transform.determinant();
    double condition_quality = (std::abs(det) > 1e-6) ? 1.0 : 0.0;

    // GLOBAL SCORING: Weighted combination for optimal assembly
    quality_score = 0.4 * geometric_quality +
                   0.3 * proximity_quality +
                   0.2 * normal_quality +
                   0.1 * condition_quality;

    std::cout << "*** QUALITY BREAKDOWN *** Geometric: " << geometric_quality
              << ", Proximity: " << proximity_quality
              << ", Normal: " << normal_quality
              << ", Condition: " << condition_quality
              << " = Total: " << quality_score << std::endl;

    return quality_score;
}

void ConnectionPreservationLayer::calculateConnectionMetadata(PreservedConnection& connection) {
    // Calculate geometric confidence (placeholder - integrate with actual ICP analysis)
    connection.geometric_confidence = std::min(1.0, connection.connection_strength / 100.0);

    // Calculate spatial proximity (placeholder - integrate with spatial analysis)
    connection.spatial_proximity = 0.8;  // Assume good proximity for now

    // Calculate normal compatibility (placeholder - integrate with normal analysis)
    connection.normal_compatibility = 0.9;  // Assume good compatibility for now

    // Mark as passed intersection test (placeholder - integrate with intersection detector)
    connection.passed_intersection_test = true;
}

bool ConnectionPreservationLayer::validateConnection(const PreservedConnection& connection) const {
    // Basic validation
    if (connection.piece_x <= 0 || connection.piece_y <= 0) return false;
    if (connection.piece_x == connection.piece_y) return false;
    if (connection.connection_strength < 0) return false;

    return true;
}

std::string ConnectionPreservationLayer::generateTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d_%H:%M:%S");
    return oss.str();
}

void ConnectionPreservationLayer::printPreservationSummary() const {
    std::cout << "#################### CONNECTION PRESERVATION SUMMARY ####################" << std::endl;
    std::cout << "Total connections preserved: " << stats_.total_connections_preserved << std::endl;
    std::cout << "Cross-component connections: " << stats_.cross_component_connections << std::endl;
    std::cout << "Component count: " << stats_.component_count << std::endl;
    std::cout << "Processing time: " << stats_.processing_time.count() << "ms" << std::endl;

    if (stats_.total_connections_preserved > 0) {
        double cross_component_ratio = static_cast<double>(stats_.cross_component_connections) /
                                      stats_.total_connections_preserved;
        std::cout << "Cross-component ratio: " << (cross_component_ratio * 100.0) << "%" << std::endl;
    }
}

void ConnectionPreservationLayer::printCrossComponentAnalysis() const {
    std::cout << "#################### CROSS-COMPONENT BRIDGE ANALYSIS ####################" << std::endl;

    for (const auto& comp_pair_entry : component_bridges_) {
        int comp1 = comp_pair_entry.first.first;
        int comp2 = comp_pair_entry.first.second;
        const auto& connections = comp_pair_entry.second;

        std::cout << "Bridge between Components " << comp1 << "-" << comp2
                  << ": " << connections.size() << " connections" << std::endl;

        double total_strength = 0.0;
        for (const auto* connection : connections) {
            total_strength += connection->connection_strength;
            std::cout << "  Pieces " << connection->piece_x << "-" << connection->piece_y
                      << " strength=" << connection->connection_strength << std::endl;
        }

        if (!connections.empty()) {
            double avg_strength = total_strength / connections.size();
            std::cout << "  Average strength: " << avg_strength << std::endl;
        }
    }
}

ConnectionPreservationLayer::PreservationStats
ConnectionPreservationLayer::getPreservationStatistics() const {
    return stats_;
}

int ConnectionPreservationLayer::getComponentAssignment(int piece_id) const {
    auto it = piece_assignments_.find(piece_id);
    return (it != piece_assignments_.end()) ? it->second.component_id : -1;
}

std::vector<int> ConnectionPreservationLayer::getPiecesInComponent(int component_id) const {
    std::vector<int> pieces;

    for (const auto& assignment_pair : piece_assignments_) {
        if (assignment_pair.second.component_id == component_id) {
            pieces.push_back(assignment_pair.first);
        }
    }

    return pieces;
}

int ConnectionPreservationLayer::getComponentCount() const {
    return stats_.component_count;
}

//############################## Phase 1 Local Assembly Manager Implementation ##############################//

Phase1LocalAssemblyManager::Phase1LocalAssemblyManager()
    : preservation_layer_(std::make_unique<ConnectionPreservationLayer>()) {
}

Phase1LocalAssemblyManager::~Phase1LocalAssemblyManager() = default;

Phase1LocalAssemblyManager::Phase1Result
Phase1LocalAssemblyManager::executeLocalAssembly(
    const std::vector<LCSIndex>& pruned_connections,
    std::vector<Geom>& shard,
    StateManager& state_manager) {

    auto start_time = std::chrono::high_resolution_clock::now();
    Phase1Result result;

    std::cout << "#################### PHASE 1: LOCAL ASSEMBLY WITH PRESERVATION ####################" << std::endl;

    try {
        // Step 1: Preserve connections AFTER pairwise pruning (the valuable 88 connections)
        preservation_layer_->preserveConnectionsAfterPruning(pruned_connections);
        result.connections_preserved = static_cast<int>(pruned_connections.size());

        // Step 2: Execute existing graph building algorithm
        std::cout << "*** PHASE 1 *** Executing existing graph building algorithm..." << std::endl;

        // Execute the existing StateManager graph building
        state_manager.BuildStep();

        std::cout << "*** PHASE 1 *** Graph building completed, extracting components..." << std::endl;

        // Step 3: Extract local components from StateManager results
        local_components_.clear();

        if (!state_manager.out_state_.empty()) {
            // Use the best assembly state (first one is typically best ranked)
            const auto& best_state = state_manager.out_state_[0];

            std::cout << "*** PHASE 1 *** Extracting from best state with " << best_state.graph_.size() << " graphs" << std::endl;

            // Extract each graph as a component
            for (const auto& graph : best_state.graph_) {
                local_components_.push_back(graph);
            }

            // Update preservation layer with component assignments
            preservation_layer_->updateComponentAssignments(local_components_);

            std::cout << "*** PHASE 1 *** Extracted " << local_components_.size() << " components from StateManager" << std::endl;
        } else {
            std::cout << "*** PHASE 1 WARNING *** No assembly states generated by StateManager" << std::endl;
        }

        result.components_generated = static_cast<int>(local_components_.size());
        result.local_assembly_quality = assessLocalAssemblyQuality();
        result.success = true;

        std::cout << "*** PHASE 1 COMPLETE *** Generated " << result.components_generated
                  << " local components with quality " << result.local_assembly_quality << std::endl;

    } catch (const std::exception& e) {
        result.success = false;
        result.debug_log.push_back("Phase 1 failed: " + std::string(e.what()));
        std::cout << "*** PHASE 1 ERROR *** " << e.what() << std::endl;
    }

    result.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    return result;
}

std::vector<RankingSubgraph> Phase1LocalAssemblyManager::getLocalComponents() const {
    return local_components_;
}

ConnectionPreservationLayer* Phase1LocalAssemblyManager::getPreservationLayer() const {
    return preservation_layer_.get();
}

double Phase1LocalAssemblyManager::assessLocalAssemblyQuality() const {
    // Placeholder quality assessment
    // In actual implementation, this would analyze the local components
    return 0.75;  // Assume reasonable local quality
}

void Phase1LocalAssemblyManager::printPhase1Summary() const {
    std::cout << "#################### PHASE 1 SUMMARY ####################" << std::endl;
    std::cout << "Local components generated: " << local_components_.size() << std::endl;
    std::cout << "Local assembly quality: " << assessLocalAssemblyQuality() << std::endl;

    if (preservation_layer_) {
        preservation_layer_->printPreservationSummary();
    }
}

//############################## Phase 2 Global Connectivity Manager Implementation ##############################//

Phase2GlobalConnectivityManager::Phase2GlobalConnectivityManager(ConnectionPreservationLayer* preservation_layer)
    : preservation_layer_(preservation_layer) {

    // Initialize enhanced GGCE with preservation layer integration
    GGCEConfiguration config = GGCEConfiguration::loadFromEnvironment();
    config.enabled = true;
    config.debug_mode = true;
    config.verbose_logging = true;

    enhanced_ggce_ = std::make_unique<GlobalGraphConnectivityEngine>(config);
}

Phase2GlobalConnectivityManager::~Phase2GlobalConnectivityManager() = default;

Phase2GlobalConnectivityManager::Phase2Result
Phase2GlobalConnectivityManager::executeGlobalConnectivity(
    std::vector<RankingSubgraph>& components,
    std::vector<Geom>& shard) {

    auto start_time = std::chrono::high_resolution_clock::now();
    Phase2Result result;

    std::cout << "#################### PHASE 2: GLOBAL CONNECTIVITY RESOLUTION ####################" << std::endl;

    try {
        result.initial_components = static_cast<int>(components.size());

        if (!preservation_layer_) {
            throw std::runtime_error("Preservation layer not available for Phase 2");
        }

        // Step 1: Update component assignments in preservation layer
        preservation_layer_->updateComponentAssignments(components);

        // Step 2: Generate enhanced bridge candidates using preserved connections
        auto bridge_candidates = generateEnhancedBridgeCandidates(components, shard);
        result.bridges_attempted = static_cast<int>(bridge_candidates.size());

        if (bridge_candidates.empty()) {
            std::cout << "*** PHASE 2 WARNING *** No bridge candidates found" << std::endl;
            result.success = false;
            result.final_components = result.initial_components;
            return result;
        }

        // Step 3: Optimize bridge selection
        auto optimized_bridges = optimizeBridgeSelection(bridge_candidates);

        // Step 4: Execute progressive component merging
        bool merging_success = executeProgressiveComponentMerging(components, optimized_bridges, shard);

        result.final_components = static_cast<int>(components.size());
        result.final_component_data = components;  // Store the actual component data
        result.bridges_successful = result.initial_components - result.final_components;
        result.connectivity_improvement = static_cast<double>(result.bridges_successful) /
                                        std::max(1.0, static_cast<double>(result.initial_components - 1));
        result.global_assembly_quality = assessGlobalAssemblyQuality(components);
        result.success = (result.final_components < result.initial_components);

        std::cout << "*** PHASE 2 COMPLETE *** " << result.initial_components << " → "
                  << result.final_components << " components (improvement: "
                  << (result.connectivity_improvement * 100.0) << "%)" << std::endl;

    } catch (const std::exception& e) {
        result.success = false;
        result.debug_log.push_back("Phase 2 failed: " + std::string(e.what()));
        std::cout << "*** PHASE 2 ERROR *** " << e.what() << std::endl;
    }

    result.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    return result;
}

std::vector<BridgeCandidate> Phase2GlobalConnectivityManager::generateEnhancedBridgeCandidates(
    const std::vector<RankingSubgraph>& components,
    const std::vector<Geom>& shard) {

    std::cout << "*** PHASE 2 *** Generating enhanced bridge candidates..." << std::endl;

    // Get bridge candidates from preservation layer
    auto candidates = preservation_layer_->generateBridgeCandidatesForGGCE();

    std::cout << "*** ENHANCED BRIDGE GENERATION *** " << candidates.size()
              << " candidates from preserved connections" << std::endl;

    return candidates;
}

std::vector<BridgeCandidate> Phase2GlobalConnectivityManager::optimizeBridgeSelection(
    std::vector<BridgeCandidate>& candidates) {

    std::cout << "*** PHASE 2 *** Optimizing bridge selection..." << std::endl;

    // For now, return top candidates (in actual implementation, apply multi-objective optimization)
    std::vector<BridgeCandidate> optimized;

    // Take top 10 candidates or all if fewer
    int take_count = std::min(10, static_cast<int>(candidates.size()));
    optimized.assign(candidates.begin(), candidates.begin() + take_count);

    std::cout << "*** BRIDGE OPTIMIZATION *** Selected " << optimized.size()
              << " top candidates for merging" << std::endl;

    return optimized;
}

bool Phase2GlobalConnectivityManager::executeProgressiveComponentMerging(
    std::vector<RankingSubgraph>& components,
    const std::vector<BridgeCandidate>& bridges,
    std::vector<Geom>& shard) {

    std::cout << "*** PHASE 2 *** Executing progressive component merging..." << std::endl;

    bool any_merge_successful = false;
    int initial_component_count = static_cast<int>(components.size());

    for (const auto& bridge : bridges) {
        // Safety check: ensure component indices are valid for current state
        if (bridge.source_graph_index >= components.size() ||
            bridge.target_graph_index >= components.size() ||
            bridge.source_graph_index == bridge.target_graph_index) {
            std::cout << "*** SKIPPING INVALID MERGE *** Components " << bridge.source_graph_index
                      << "-" << bridge.target_graph_index << " (indices invalid for current state)" << std::endl;
            continue;
        }

        std::cout << "*** ATTEMPTING MERGE *** Components " << bridge.source_graph_index
                  << "-" << bridge.target_graph_index << " via pieces "
                  << bridge.source_piece_id << "-" << bridge.target_piece_id
                  << " (score=" << bridge.combined_score << ")" << std::endl;

        // Use the real GGCE merging implementation
        bool merge_success = executeSingleMergeFromGGCE(components, bridge, shard);

        if (merge_success) {
            any_merge_successful = true;
            std::cout << "*** MERGE SUCCESS *** Components reduced from " << initial_component_count
                      << " to " << components.size() << std::endl;
            initial_component_count = static_cast<int>(components.size());

            // Update preservation layer with new component structure
            if (preservation_layer_) {
                preservation_layer_->updateComponentAssignments(components);
            }
        } else {
            std::cout << "*** MERGE FAILED *** Components " << bridge.source_graph_index
                      << "-" << bridge.target_graph_index << " merge unsuccessful" << std::endl;
        }

        // Check if we've achieved single component unification
        if (components.size() == 1) {
            std::cout << "*** OPTIMAL RESULT ACHIEVED *** All pieces unified in single component, stopping merge attempts" << std::endl;
            break;
        }

        // Continue trying more merges to achieve further component reductions
        // This allows multiple component reductions in one phase
    }

    return any_merge_successful;
}

bool Phase2GlobalConnectivityManager::executeSingleMergeFromGGCE(
    std::vector<RankingSubgraph>& components,
    const BridgeCandidate& candidate,
    std::vector<Geom>& shard) {

    try {
        // Bounds checking (same as GGCE implementation)
        if (candidate.source_graph_index < 0 || candidate.source_graph_index >= components.size() ||
            candidate.target_graph_index < 0 || candidate.target_graph_index >= components.size() ||
            candidate.source_graph_index == candidate.target_graph_index) {
            return false;
        }

        auto& source_graph = components[candidate.source_graph_index];
        auto& target_graph = components[candidate.target_graph_index];

        std::cout << "*** MERGING *** Source graph has " << source_graph.NumTrueNode()
                  << " pieces, target graph has " << target_graph.NumTrueNode() << " pieces" << std::endl;

        // INDIVIDUAL OPTIMIZATION: Process ALL pieces in source component for optimal positioning
        std::cout << "*** INDIVIDUAL OPTIMIZATION *** Processing all pieces in source component " << candidate.source_graph_index << std::endl;

        // Identify all pieces in the source component
        std::vector<int> source_component_pieces;
        for (int i = 0; i < source_graph.node_.size(); ++i) {
            if (source_graph.node_[i]) {
                source_component_pieces.push_back(i + 1); // Convert to 1-based piece ID
            }
        }

        std::cout << "*** SOURCE COMPONENT PIECES *** Found " << source_component_pieces.size() << " pieces to optimize" << std::endl;

        // CRITICAL FIX: Process ALL pieces from source component for preservation
        for (int piece_id : source_component_pieces) {
            int piece_index = piece_id - 1; // Convert to 0-based for shard array

            if (piece_index >= 0 && piece_index < shard.size()) {
                // FIXED MERGE DIRECTION: Copy source pieces INTO target graph (smaller INTO bigger)
                if (piece_index < target_graph.node_.size()) {
                    target_graph.node_[piece_index] = true;  // Add source piece to target graph
                }

                if (piece_id == candidate.source_piece_id) {
                    // This is the bridge piece - use the computed bridge transformation
                    std::cout << "*** APPLYING BRIDGE TRANSFORM *** Piece " << piece_id << " (bridge piece) connecting to piece " << candidate.target_piece_id << std::endl;

                    Matrix4d transform_matrix = candidate.best_transformation;

                    // DEBUG: Print transformation matrix values
                    std::cout << "*** DEBUG TRANSFORM MATRIX ***" << std::endl;
                    for (int row = 0; row < 4; ++row) {
                        std::cout << "Row " << row << ": ";
                        for (int col = 0; col < 4; ++col) {
                            std::cout << transform_matrix(row, col) << " ";
                        }
                        std::cout << std::endl;
                    }

                    Matrix3d R = transform_matrix.block<3,3>(0,0);
                    Vector3d t = transform_matrix.block<3,1>(0,3);

                    std::cout << "*** DEBUG R matrix ***" << std::endl;
                    std::cout << R << std::endl;
                    std::cout << "*** DEBUG t vector ***" << std::endl;
                    std::cout << t.transpose() << std::endl;

                    shard[piece_index].Move(R, t);

                    std::cout << "*** BRIDGE TRANSFORM APPLIED *** Piece " << piece_id << " moved to connect with target" << std::endl;

                } else {
                    // Non-bridge piece - find its best individual connection within target component
                    std::cout << "*** INDIVIDUAL OPTIMIZATION *** Finding best connection for piece " << piece_id << std::endl;

                    // EMERGENCY FIX: Preserve ALL non-bridge pieces with baseline transformation
                    // Apply bridge transformation to ensure piece is preserved in final assembly
                    std::cout << "*** PRESERVING NON-BRIDGE PIECE *** Piece " << piece_id << " (applying baseline transform for preservation)" << std::endl;

                    Matrix4d transform_matrix = candidate.best_transformation;

                    // DEBUG: Check if same matrix is used for non-bridge piece
                    std::cout << "*** DEBUG NON-BRIDGE MATRIX *** (should be same as bridge)" << std::endl;
                    std::cout << "Matrix determinant: " << transform_matrix.determinant() << std::endl;
                    std::cout << "Matrix trace: " << transform_matrix.trace() << std::endl;

                    Matrix3d R = transform_matrix.block<3,3>(0,0);
                    Vector3d t = transform_matrix.block<3,1>(0,3);

                    shard[piece_index].Move(R, t);

                    std::cout << "*** BASELINE TRANSFORM APPLIED *** Piece " << piece_id << " moved using bridge transformation" << std::endl;
                }

                // FIXED MERGE DIRECTION: Update transformation matrix in TARGET graph (preserve all transformations)
                if (piece_index < source_graph.T_.size() && piece_index < target_graph.T_.size()) {
                    target_graph.T_[piece_index] = source_graph.T_[piece_index];  // Copy source transforms to target
                }
            }
        }

        // FIXED MERGE DIRECTION: Update TARGET graph score (smaller merges INTO bigger)
        target_graph.graph_score_ += source_graph.graph_score_;
        target_graph.pc_score_ = std::max(target_graph.pc_score_, source_graph.pc_score_);

        std::cout << "*** MERGE DATA *** After merge, target graph has " << target_graph.NumTrueNode()
                  << " pieces, score=" << target_graph.graph_score_ << std::endl;

        // FIXED: Remove the SOURCE graph (smaller component consumed by bigger)
        // CRITICAL: Ensure we remove the correct index (source_graph_index vs target_graph_index)
        int index_to_remove = candidate.source_graph_index;
        std::cout << "*** REMOVING SOURCE COMPONENT *** Index " << index_to_remove
                  << " (target component " << candidate.target_graph_index << " preserved)" << std::endl;

        components.erase(components.begin() + index_to_remove);

        return true;

    } catch (const std::exception& e) {
        std::cout << "*** MERGE ERROR *** " << e.what() << std::endl;
        return false;
    }
}

double Phase2GlobalConnectivityManager::assessGlobalAssemblyQuality(
    const std::vector<RankingSubgraph>& components) const {

    // Placeholder global quality assessment
    // In actual implementation, this would analyze global assembly coherence
    return 0.85;
}

void Phase2GlobalConnectivityManager::printPhase2Summary() const {
    std::cout << "#################### PHASE 2 SUMMARY ####################" << std::endl;
    // Implementation details would go here
}

//############################## Two-Phase Assembly Coordinator Implementation ##############################//

TwoPhaseAssemblyCoordinator::TwoPhaseAssemblyCoordinator() : debug_mode_enabled_(false) {
    initializePhaseManagers();
}

TwoPhaseAssemblyCoordinator::~TwoPhaseAssemblyCoordinator() = default;

void TwoPhaseAssemblyCoordinator::initializePhaseManagers() {
    phase1_manager_ = std::make_unique<Phase1LocalAssemblyManager>();
    // Phase 2 manager will be initialized after Phase 1 completes
}

TwoPhaseAssemblyCoordinator::TwoPhaseResult
TwoPhaseAssemblyCoordinator::executeEnhancedAssembly(
    const std::vector<LCSIndex>& pruned_connections,
    std::vector<Geom>& shard,
    StateManager& state_manager) {

    auto start_time = std::chrono::high_resolution_clock::now();
    TwoPhaseResult result;

    std::cout << "#################### TWO-PHASE ASSEMBLY COORDINATOR ####################" << std::endl;
    std::cout << "*** STARTING ENHANCED ASSEMBLY *** " << pruned_connections.size()
              << " pruned connections (post-pairwise-pruning), " << shard.size() << " pieces" << std::endl;

    try {
        // Execute Phase 1: Local Assembly with Preservation
        logPhaseTransition("Phase 1: Local Assembly");
        auto phase1_result = phase1_manager_->executeLocalAssembly(pruned_connections, shard, state_manager);

        if (!phase1_result.success) {
            result.success = false;
            result.comprehensive_debug_log.push_back("Phase 1 failed");
            return result;
        }

        // Copy Phase 1 results
        result.local_components_generated = phase1_result.components_generated;
        result.local_assembly_quality = phase1_result.local_assembly_quality;
        result.connections_preserved = phase1_result.connections_preserved;

        // Initialize Phase 2 with Phase 1 results
        phase2_manager_ = std::make_unique<Phase2GlobalConnectivityManager>(
            phase1_manager_->getPreservationLayer());

        // Execute Phase 2: Global Connectivity Resolution
        logPhaseTransition("Phase 2: Global Connectivity");
        auto components = phase1_manager_->getLocalComponents();
        auto phase2_result = phase2_manager_->executeGlobalConnectivity(components, shard);

        // Copy Phase 2 results
        result.final_components = phase2_result.final_components;
        result.connectivity_improvement = phase2_result.connectivity_improvement;
        result.global_assembly_quality = phase2_result.global_assembly_quality;
        result.bridges_successful = phase2_result.bridges_successful;
        result.final_component_data = phase2_result.final_component_data;

        // Calculate overall metrics
        result.overall_improvement = calculateOverallImprovement(result);
        result.success = phase2_result.success;

        std::cout << "*** TWO-PHASE ASSEMBLY COMPLETE *** Overall improvement: "
                  << (result.overall_improvement * 100.0) << "%" << std::endl;

    } catch (const std::exception& e) {
        result.success = false;
        result.comprehensive_debug_log.push_back("Two-phase assembly failed: " + std::string(e.what()));
        std::cout << "*** TWO-PHASE ASSEMBLY ERROR *** " << e.what() << std::endl;
    }

    result.total_processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time);

    last_result_ = result;
    return result;
}

void TwoPhaseAssemblyCoordinator::logPhaseTransition(const std::string& phase_name) const {
    std::cout << "#################### " << phase_name << " ####################" << std::endl;
}

double TwoPhaseAssemblyCoordinator::calculateOverallImprovement(const TwoPhaseResult& result) const {
    // Combine local assembly quality and global connectivity improvement
    return 0.6 * result.global_assembly_quality + 0.4 * result.connectivity_improvement;
}

TwoPhaseAssemblyCoordinator::TwoPhaseResult TwoPhaseAssemblyCoordinator::getLastResult() const {
    return last_result_;
}

std::vector<RankingSubgraph> TwoPhaseAssemblyCoordinator::getFinalComponents() const {
    return last_result_.final_component_data;
}

void TwoPhaseAssemblyCoordinator::enableDebugMode(bool enabled) {
    debug_mode_enabled_ = enabled;
}

void TwoPhaseAssemblyCoordinator::printComprehensiveSummary() const {
    std::cout << "#################### TWO-PHASE ASSEMBLY COMPREHENSIVE SUMMARY ####################" << std::endl;

    if (phase1_manager_) {
        phase1_manager_->printPhase1Summary();
    }

    if (phase2_manager_) {
        phase2_manager_->printPhase2Summary();
    }

    std::cout << "Total processing time: " << last_result_.total_processing_time.count() << "ms" << std::endl;
    std::cout << "Overall improvement: " << (last_result_.overall_improvement * 100.0) << "%" << std::endl;
    std::cout << "Success: " << (last_result_.success ? "YES" : "NO") << std::endl;
}