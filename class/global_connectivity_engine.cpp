#include "global_connectivity_engine.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <set>

//############################## Configuration System Implementation ##############################//

GGCEConfiguration GGCEConfiguration::loadFromEnvironment() {
    GGCEConfiguration config;

    // Feature flags
    if (const char* enabled = std::getenv("GGCE_ENABLED")) {
        config.enabled = (std::strcmp(enabled, "1") == 0 || std::strcmp(enabled, "true") == 0);
    }

    if (const char* debug = std::getenv("GGCE_DEBUG")) {
        config.debug_mode = (std::strcmp(debug, "1") == 0 || std::strcmp(debug, "true") == 0);
    }

    if (const char* verbose = std::getenv("GGCE_VERBOSE")) {
        config.verbose_logging = (std::strcmp(verbose, "1") == 0 || std::strcmp(verbose, "true") == 0);
    }

    // Algorithm parameters
    if (const char* conn_weight = std::getenv("GGCE_CONNECTIVITY_WEIGHT")) {
        config.connectivity_weight = std::atof(conn_weight);
    }

    if (const char* qual_weight = std::getenv("GGCE_QUALITY_WEIGHT")) {
        config.quality_weight = std::atof(qual_weight);
    }

    if (const char* cons_weight = std::getenv("GGCE_CONSISTENCY_WEIGHT")) {
        config.consistency_weight = std::atof(cons_weight);
    }

    if (const char* qual_thresh = std::getenv("GGCE_QUALITY_THRESHOLD")) {
        config.quality_threshold = std::atof(qual_thresh);
    }

    // Performance parameters
    if (const char* max_iter = std::getenv("GGCE_MAX_ITERATIONS")) {
        config.max_merge_iterations = std::atoi(max_iter);
    }

    if (const char* max_candidates = std::getenv("GGCE_MAX_CANDIDATES")) {
        config.max_bridge_candidates = std::atoi(max_candidates);
    }

    if (const char* cell_size = std::getenv("GGCE_SPATIAL_CELL_SIZE")) {
        config.spatial_index_cell_size = std::atof(cell_size);
    }

    // Intersection detection parameters
    if (const char* max_overlap = std::getenv("GGCE_MAX_VOLUME_OVERLAP")) {
        config.max_volume_overlap_ratio = std::atof(max_overlap);
    }

    if (const char* crit_overlap = std::getenv("GGCE_CRITICAL_OVERLAP")) {
        config.critical_volume_overlap_ratio = std::atof(crit_overlap);
    }

    if (const char* point_tol = std::getenv("GGCE_POINT_TOLERANCE")) {
        config.point_inside_tolerance = std::atof(point_tol);
    }

    if (const char* surf_thresh = std::getenv("GGCE_SURFACE_THRESHOLD")) {
        config.surface_proximity_threshold = std::atof(surf_thresh);
    }

    // Algorithm variant
    if (const char* variant = std::getenv("GGCE_VARIANT")) {
        config.algorithm_variant = std::string(variant);
    }

    return config;
}

bool GGCEConfiguration::validate() const {
    // Validate weight parameters sum to 1.0 (approximately)
    double weight_sum = connectivity_weight + quality_weight + consistency_weight;
    if (std::abs(weight_sum - 1.0) > 0.01) {
        std::cerr << "GGCE Configuration Error: Weights must sum to 1.0, got " << weight_sum << std::endl;
        return false;
    }

    // Validate ranges
    if (connectivity_weight < 0.0 || connectivity_weight > 1.0 ||
        quality_weight < 0.0 || quality_weight > 1.0 ||
        consistency_weight < 0.0 || consistency_weight > 1.0) {
        std::cerr << "GGCE Configuration Error: Weights must be in [0.0, 1.0] range" << std::endl;
        return false;
    }

    if (quality_threshold < 0.0 || quality_threshold > 1.0) {
        std::cerr << "GGCE Configuration Error: Quality threshold must be in [0.0, 1.0] range" << std::endl;
        return false;
    }

    if (max_merge_iterations < 1 || max_merge_iterations > 100) {
        std::cerr << "GGCE Configuration Error: Max iterations must be in [1, 100] range" << std::endl;
        return false;
    }

    if (max_bridge_candidates < 1 || max_bridge_candidates > 1000) {
        std::cerr << "GGCE Configuration Error: Max candidates must be in [1, 1000] range" << std::endl;
        return false;
    }

    if (spatial_index_cell_size <= 0.0 || spatial_index_cell_size > 100.0) {
        std::cerr << "GGCE Configuration Error: Spatial cell size must be in (0.0, 100.0] range" << std::endl;
        return false;
    }

    // Validate algorithm variant
    if (algorithm_variant != "conservative" &&
        algorithm_variant != "aggressive" &&
        algorithm_variant != "balanced") {
        std::cerr << "GGCE Configuration Error: Invalid algorithm variant: " << algorithm_variant << std::endl;
        return false;
    }

    return true;
}

std::string GGCEConfiguration::toString() const {
    std::ostringstream oss;
    oss << "GGCE Configuration:\n";
    oss << "  Enabled: " << (enabled ? "true" : "false") << "\n";
    oss << "  Debug Mode: " << (debug_mode ? "true" : "false") << "\n";
    oss << "  Verbose Logging: " << (verbose_logging ? "true" : "false") << "\n";
    oss << "  Connectivity Weight: " << connectivity_weight << "\n";
    oss << "  Quality Weight: " << quality_weight << "\n";
    oss << "  Consistency Weight: " << consistency_weight << "\n";
    oss << "  Quality Threshold: " << quality_threshold << "\n";
    oss << "  Max Iterations: " << max_merge_iterations << "\n";
    oss << "  Max Candidates: " << max_bridge_candidates << "\n";
    oss << "  Spatial Cell Size: " << spatial_index_cell_size << "mm\n";
    oss << "  Algorithm Variant: " << algorithm_variant << "\n";
    oss << "  Max Volume Overlap: " << max_volume_overlap_ratio << "\n";
    oss << "  Critical Overlap: " << critical_volume_overlap_ratio << "\n";
    oss << "  Point Tolerance: " << point_inside_tolerance << "mm\n";
    oss << "  Surface Threshold: " << surface_proximity_threshold << "mm";
    return oss.str();
}

//############################## Bridge Candidate Implementation ##############################//

std::string BridgeCandidate::toString() const {
    std::ostringstream oss;
    oss << "BridgeCandidate{\n";
    oss << "  Graphs: " << source_graph_index << " -> " << target_graph_index << "\n";
    oss << "  Pieces: " << source_piece_id << " -> " << target_piece_id << "\n";
    oss << "  Scores: conn=" << connectivity_score << ", geom=" << geometric_confidence;
    oss << ", trans=" << transformation_quality << ", cons=" << consistency_score << "\n";
    oss << "  Combined Score: " << combined_score << "\n";
    oss << "  Valid: geom=" << (geometric_valid ? "Y" : "N");
    oss << ", intersect=" << (intersection_valid ? "Y" : "N");
    oss << ", consist=" << (consistency_valid ? "Y" : "N") << "\n";
    if (!rejection_reason.empty()) {
        oss << "  Rejection: " << rejection_reason << "\n";
    }
    oss << "}";
    return oss.str();
}

//############################## Spatial Index Implementation ##############################//

SpatialIndex::SpatialIndex(double cell_size) : cell_size_(cell_size) {
    if (cell_size <= 0.0) {
        throw std::invalid_argument("SpatialIndex: Cell size must be positive");
    }
}

void SpatialIndex::buildIndex(const std::vector<RankingSubgraph>& components,
                             const std::vector<Geom>& shard) {
    spatial_grid_.clear();
    piece_centroids_.clear();
    piece_to_graph_mapping_.clear();

    // Calculate piece centroids and build graph mapping
    for (int graph_idx = 0; graph_idx < components.size(); ++graph_idx) {
        const auto& graph = components[graph_idx];

        for (int piece_idx = 0; piece_idx < graph.node_.size(); ++piece_idx) {
            if (!graph.node_[piece_idx]) continue;

            int piece_id = piece_idx + 1; // Convert to 1-based
            piece_to_graph_mapping_[piece_id] = graph_idx;

            // Calculate centroid of piece geometry
            if (piece_idx < shard.size() && shard[piece_idx].edge_line_.point_.cols() > 0) {
                Vector3d centroid = Vector3d::Zero();
                int point_count = shard[piece_idx].edge_line_.point_.cols();

                for (int i = 0; i < point_count; ++i) {
                    centroid += shard[piece_idx].edge_line_.point_.col(i);
                }
                centroid /= point_count;

                piece_centroids_.resize(std::max((int)piece_centroids_.size(), piece_id));
                piece_centroids_[piece_id - 1] = centroid; // Store using 0-based index

                // Add to spatial grid
                auto grid_cell = getGridCell(centroid);
                spatial_grid_[grid_cell].graph_indices.insert(graph_idx);
                spatial_grid_[grid_cell].piece_indices.insert(piece_id);
            }
        }
    }

    if (piece_centroids_.empty()) {
        std::cerr << "SpatialIndex Warning: No valid geometry found for spatial indexing" << std::endl;
    }
}

std::tuple<int, int, int> SpatialIndex::getGridCell(const Vector3d& point) const {
    return std::make_tuple(
        static_cast<int>(std::floor(point.x() / cell_size_)),
        static_cast<int>(std::floor(point.y() / cell_size_)),
        static_cast<int>(std::floor(point.z() / cell_size_))
    );
}

bool SpatialIndex::mayHaveConnections(int graph_i, int graph_j) const {
    if (graph_i == graph_j) return false;

    // Check if any spatial cells contain pieces from both graphs
    for (const auto& [cell_key, cell_data] : spatial_grid_) {
        if (cell_data.graph_indices.count(graph_i) &&
            cell_data.graph_indices.count(graph_j)) {
            return true;
        }
    }

    return false;
}

std::vector<std::pair<int, int>> SpatialIndex::getCandidatePairs(int graph_i, int graph_j) const {
    std::vector<std::pair<int, int>> pairs;

    // Find all piece pairs in nearby spatial cells
    for (const auto& [cell_key, cell_data] : spatial_grid_) {
        if (cell_data.graph_indices.count(graph_i) &&
            cell_data.graph_indices.count(graph_j)) {

            // Generate all pairs between pieces from the two graphs
            for (int piece_i : cell_data.piece_indices) {
                if (piece_to_graph_mapping_.count(piece_i) &&
                    piece_to_graph_mapping_.at(piece_i) == graph_i) {

                    for (int piece_j : cell_data.piece_indices) {
                        if (piece_to_graph_mapping_.count(piece_j) &&
                            piece_to_graph_mapping_.at(piece_j) == graph_j) {
                            pairs.emplace_back(piece_i, piece_j);
                        }
                    }
                }
            }
        }
    }

    // Remove duplicates and sort
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());

    return pairs;
}

//############################## Global Connectivity Analyzer Implementation ##############################//

GlobalConnectivityAnalyzer::GlobalConnectivityAnalyzer(const GGCEConfiguration& config)
    : config_(config) {

    // Initialize spatial index with configured cell size
    spatial_index_ = std::make_unique<SpatialIndex>(config_.spatial_index_cell_size);

    // Initialize connectivity optimizer
    connectivity_optimizer_ = std::make_unique<ConnectivityOptimizer>();
}

std::vector<BridgeCandidate> GlobalConnectivityAnalyzer::detectPotentialBridges(
    const std::vector<RankingSubgraph>& components,
    const std::vector<Geom>& shard,
    const std::vector<LCSIndex>& lcs_reference) {

    std::vector<BridgeCandidate> all_candidates;

    // DEBUG: Log component analysis
    if (config_.debug_mode) {
        std::cout << "GGCE DEBUG: Analyzing " << components.size() << " graph components for bridges" << std::endl;
        for (int i = 0; i < components.size(); ++i) {
            std::set<int> pieces;
            for (const auto& lcs : components[i].sub_graph_) {
                pieces.insert(lcs.shard_x_);
                pieces.insert(lcs.shard_y_);
            }
            std::cout << "  Graph " << i << ": {";
            for (int piece : pieces) {
                std::cout << piece << " ";
            }
            std::cout << "} (" << pieces.size() << " pieces)" << std::endl;
        }
    }

    // Early exit for single component
    if (components.size() <= 1) {
        return all_candidates;
    }

    // Build spatial index for performance
    spatial_index_->buildIndex(components, shard);

    // Analyze all component pairs
    for (int i = 0; i < components.size(); ++i) {
        for (int j = i + 1; j < components.size(); ++j) {

            // Early geometric filtering using spatial index
            // GGCE FIX: For multi-graph fragmentation, check LCS connections even without spatial overlap
            bool has_spatial_overlap = spatial_index_->mayHaveConnections(i, j);
            bool force_lcs_analysis = (components.size() > 1) && !has_spatial_overlap;

            if (!has_spatial_overlap && !force_lcs_analysis) {
                if (config_.verbose_logging) {
                    std::cout << "GGCE: Skipping graphs " << i << "-" << j
                              << " (no spatial overlap)" << std::endl;
                }
                continue;
            }

            if (force_lcs_analysis && config_.verbose_logging) {
                std::cout << "GGCE: Forcing LCS analysis for graphs " << i << "-" << j
                          << " (multi-graph fragmentation detected)" << std::endl;
            }

            // Detailed analysis for this component pair
            auto candidates = analyzeCrossComponentConnections(
                components[i], components[j], i, j, shard, lcs_reference);

            // Add to master list
            all_candidates.insert(all_candidates.end(), candidates.begin(), candidates.end());

            if (config_.verbose_logging && !candidates.empty()) {
                std::cout << "GGCE: Found " << candidates.size()
                          << " bridge candidates between graphs " << i << "-" << j << std::endl;
            }
        }
    }

    // DEBUG: Log bridge candidate search results
    if (config_.debug_mode) {
        std::cout << "GGCE DEBUG: Found " << all_candidates.size() << " total bridge candidates" << std::endl;

        // Check for cross-component LCS connections that might be missing
        std::cout << "GGCE DEBUG: Checking LCS connections for cross-component bridges..." << std::endl;
        for (const auto& lcs : lcs_reference) {
            // Find which graph each piece belongs to
            int graph_x = -1, graph_y = -1;
            for (int i = 0; i < components.size(); ++i) {
                std::set<int> pieces;
                for (const auto& comp_lcs : components[i].sub_graph_) {
                    pieces.insert(comp_lcs.shard_x_);
                    pieces.insert(comp_lcs.shard_y_);
                }
                if (pieces.count(lcs.shard_x_)) graph_x = i;
                if (pieces.count(lcs.shard_y_)) graph_y = i;
            }

            if (graph_x != -1 && graph_y != -1 && graph_x != graph_y) {
                std::cout << "  Cross-component LCS: Piece " << lcs.shard_x_
                          << " (graph " << graph_x << ") ↔ Piece " << lcs.shard_y_
                          << " (graph " << graph_y << "), size=" << lcs.size_ << std::endl;
            }
        }
    }

    // Phase 1: Identify optimal hub piece
    auto optimal_hub = identifyOptimalHub(lcs_reference);

    // Phase 2: Apply hub-centric prioritization
    return applyHubCentricPrioritization(all_candidates, optimal_hub);
}

std::vector<BridgeCandidate> GlobalConnectivityAnalyzer::analyzeCrossComponentConnections(
    const RankingSubgraph& graph_i,
    const RankingSubgraph& graph_j,
    int graph_i_index,
    int graph_j_index,
    const std::vector<Geom>& shard,
    const std::vector<LCSIndex>& lcs_reference) {

    std::vector<BridgeCandidate> candidates;

    // Get spatially filtered candidate pairs
    auto candidate_pairs = spatial_index_->getCandidatePairs(graph_i_index, graph_j_index);

    // GGCE FIX: If no spatial candidates but we have LCS data, generate all graph pairs
    if (candidate_pairs.empty()) {
        // Extract unique piece IDs from each graph's LCS connections
        std::set<int> pieces_i, pieces_j;
        for (const auto& lcs : graph_i.sub_graph_) {
            pieces_i.insert(lcs.shard_x_);
            pieces_i.insert(lcs.shard_y_);
        }
        for (const auto& lcs : graph_j.sub_graph_) {
            pieces_j.insert(lcs.shard_x_);
            pieces_j.insert(lcs.shard_y_);
        }

        // Generate all possible piece pairs between the two graphs
        for (int piece_i : pieces_i) {
            for (int piece_j : pieces_j) {
                candidate_pairs.emplace_back(piece_i, piece_j);
            }
        }
    }

    for (const auto& [piece_i, piece_j] : candidate_pairs) {

        // Create bridge candidate
        BridgeCandidate candidate;
        candidate.source_graph_index = graph_i_index;
        candidate.target_graph_index = graph_j_index;
        candidate.source_piece_id = piece_i;
        candidate.target_piece_id = piece_j;

        // Evaluate geometric compatibility
        candidate.geometric_confidence = evaluateGeometricCompatibility(
            piece_i - 1, piece_j - 1, shard, lcs_reference); // Convert to 0-based

        // Skip if geometric compatibility is too low
        if (candidate.geometric_confidence < config_.quality_threshold * 0.5) {
            candidate.rejection_reason = "Low geometric compatibility";
            continue;
        }

        // Calculate connectivity score using existing optimizer
        candidate.connectivity_score = connectivity_optimizer_->CalculateGlobalConnectivityScore(
            Chunk(), graph_i_index, {graph_i, graph_j}, lcs_reference);

        // Calculate transformation quality
        candidate.transformation_quality = calculateTransformationQuality(candidate, shard);

        // Calculate combined score
        candidate.combined_score =
            candidate.connectivity_score * config_.connectivity_weight +
            candidate.geometric_confidence * config_.quality_weight +
            candidate.transformation_quality * config_.consistency_weight;

        // Set validation flags (will be validated later in merge engine)
        candidate.geometric_valid = candidate.geometric_confidence > config_.quality_threshold;
        candidate.intersection_valid = true; // Will be validated by intersection detector
        candidate.consistency_valid = candidate.transformation_quality > config_.quality_threshold;

        candidates.push_back(candidate);
    }

    return candidates;
}

double GlobalConnectivityAnalyzer::evaluateGeometricCompatibility(
    int piece_i, int piece_j,
    const std::vector<Geom>& shard,
    const std::vector<LCSIndex>& lcs_reference) {

    // Bounds checking
    if (piece_i < 0 || piece_i >= shard.size() ||
        piece_j < 0 || piece_j >= shard.size()) {
        return 0.0;
    }

    // Check if pieces have valid geometry
    const auto& geom_i = shard[piece_i];
    const auto& geom_j = shard[piece_j];

    if (geom_i.edge_line_.point_.cols() == 0 || geom_j.edge_line_.point_.cols() == 0) {
        return 0.0;
    }

    // Look for existing matches in LCS reference
    double best_match_score = 0.0;
    int piece_i_id = piece_i + 1; // Convert to 1-based
    int piece_j_id = piece_j + 1;

    for (const auto& lcs : lcs_reference) {
        if ((lcs.shard_x_ == piece_i_id && lcs.shard_y_ == piece_j_id) ||
            (lcs.shard_x_ == piece_j_id && lcs.shard_y_ == piece_i_id)) {

            // Found a match! Calculate quality score based on inliers
            double match_quality = static_cast<double>(lcs.inliner_) /
                                 std::max(geom_i.edge_line_.point_.cols(), geom_j.edge_line_.point_.cols());

            best_match_score = std::max(best_match_score, match_quality);
        }
    }

    // If no existing matches found, calculate basic geometric compatibility
    if (best_match_score == 0.0) {
        // Simple distance-based compatibility
        if (geom_i.edge_line_.point_.cols() > 0 && geom_j.edge_line_.point_.cols() > 0) {
            Vector3d center_i = geom_i.edge_line_.point_.col(0);
            Vector3d center_j = geom_j.edge_line_.point_.col(0);

            // Simple proximity-based score (this is a placeholder - could be enhanced)
            double distance = (center_i - center_j).norm();
            best_match_score = std::exp(-distance / 100.0); // Normalize to [0,1]
        }
    }

    return std::min(1.0, best_match_score);
}

double GlobalConnectivityAnalyzer::calculateTransformationQuality(
    const BridgeCandidate& candidate,
    const std::vector<Geom>& shard) {

    // Placeholder implementation - would perform ICP analysis
    // For now, return a score based on geometric confidence
    return candidate.geometric_confidence * 0.8; // Conservative estimate
}

std::vector<BridgeCandidate> GlobalConnectivityAnalyzer::prioritizeBridges(
    std::vector<BridgeCandidate>& candidates) {

    // Sort by combined score (descending)
    std::sort(candidates.begin(), candidates.end(),
              [](const BridgeCandidate& a, const BridgeCandidate& b) {
                  return a.combined_score > b.combined_score;
              });

    // Limit to maximum candidates for performance
    if (candidates.size() > static_cast<size_t>(config_.max_bridge_candidates)) {
        candidates.resize(config_.max_bridge_candidates);
    }

    // Filter out candidates below quality threshold
    candidates.erase(
        std::remove_if(candidates.begin(), candidates.end(),
                      [this](const BridgeCandidate& candidate) {
                          return candidate.combined_score < config_.quality_threshold;
                      }),
        candidates.end());

    if (config_.verbose_logging) {
        std::cout << "GGCE: Prioritized " << candidates.size()
                  << " bridge candidates above quality threshold" << std::endl;
    }

    return candidates;
}

//############################## Hub Detection Implementation ##############################//

GlobalConnectivityAnalyzer::HubCandidate GlobalConnectivityAnalyzer::identifyOptimalHub(
    const std::vector<LCSIndex>& lcs_reference) {

    std::map<int, HubCandidate> piece_stats;

    // Analyze all LCS connections to find connectivity patterns
    for (const auto& lcs : lcs_reference) {
        int piece_x = lcs.shard_x_;
        int piece_y = lcs.shard_y_;
        double match_strength = static_cast<double>(lcs.size_);

        // Update stats for piece_x
        if (piece_stats.find(piece_x) == piece_stats.end()) {
            piece_stats[piece_x].piece_id = piece_x;
        }
        piece_stats[piece_x].connection_count++;
        piece_stats[piece_x].total_match_strength += match_strength;
        piece_stats[piece_x].connected_pieces.push_back(piece_y);

        // Update stats for piece_y
        if (piece_stats.find(piece_y) == piece_stats.end()) {
            piece_stats[piece_y].piece_id = piece_y;
        }
        piece_stats[piece_y].connection_count++;
        piece_stats[piece_y].total_match_strength += match_strength;
        piece_stats[piece_y].connected_pieces.push_back(piece_x);
    }

    // Calculate centrality and hub scores
    HubCandidate best_hub;
    double max_hub_score = 0.0;

    for (auto& [piece_id, candidate] : piece_stats) {
        // Remove duplicates from connected pieces
        std::sort(candidate.connected_pieces.begin(), candidate.connected_pieces.end());
        candidate.connected_pieces.erase(
            std::unique(candidate.connected_pieces.begin(), candidate.connected_pieces.end()),
            candidate.connected_pieces.end());

        // Update connection count to unique connections
        candidate.connection_count = static_cast<int>(candidate.connected_pieces.size());

        // Calculate centrality score (normalized by total pieces)
        candidate.centrality_score = static_cast<double>(candidate.connection_count) / 8.0; // 8 total pieces

        // Calculate hub score: prioritize high connectivity and match strength
        candidate.hub_score = (
            candidate.centrality_score * 0.6 +           // Connectivity weight
            (candidate.total_match_strength / 100.0) * 0.4  // Match strength weight (normalized)
        );

        if (config_.debug_mode) {
            std::cout << "GGCE Hub Analysis: Piece " << piece_id
                      << " connects to " << candidate.connection_count << " pieces"
                      << " (strength=" << candidate.total_match_strength
                      << ", hub_score=" << candidate.hub_score << ")" << std::endl;
        }

        // Track best hub candidate
        if (candidate.hub_score > max_hub_score) {
            max_hub_score = candidate.hub_score;
            best_hub = candidate;
        }
    }

    if (config_.debug_mode && best_hub.piece_id != -1) {
        std::cout << "GGCE: Optimal hub identified: Piece " << best_hub.piece_id
                  << " (connects to " << best_hub.connection_count << " pieces, score="
                  << best_hub.hub_score << ")" << std::endl;
    }

    return best_hub;
}

std::vector<BridgeCandidate> GlobalConnectivityAnalyzer::applyHubCentricPrioritization(
    std::vector<BridgeCandidate>& candidates,
    const HubCandidate& hub) {

    if (hub.piece_id == -1) {
        // No valid hub found, use original prioritization
        if (config_.debug_mode) {
            std::cout << "GGCE: No valid hub found, using standard prioritization" << std::endl;
        }
        return prioritizeBridges(candidates);
    }

    // Boost candidates that involve the hub piece
    for (auto& candidate : candidates) {
        bool involves_hub = (candidate.source_piece_id == hub.piece_id ||
                           candidate.target_piece_id == hub.piece_id);

        if (involves_hub) {
            // Apply hub connectivity boost
            double hub_boost = 1.5;  // 50% score boost for hub connections
            candidate.combined_score *= hub_boost;

            if (config_.debug_mode) {
                std::cout << "GGCE: Hub boost applied to connection "
                          << candidate.source_piece_id << "-" << candidate.target_piece_id
                          << " (involves hub piece " << hub.piece_id << ")" << std::endl;
            }
        }
    }

    // Apply standard prioritization with hub-boosted scores
    return prioritizeBridges(candidates);
}

GlobalConnectivityAnalyzer::ConnectivityMetrics GlobalConnectivityAnalyzer::calculateMetrics(
    const std::vector<RankingSubgraph>& components) const {

    ConnectivityMetrics metrics;
    metrics.total_components = static_cast<int>(components.size());

    if (components.empty()) {
        return metrics;
    }

    // Calculate component sizes
    std::vector<int> component_sizes;
    int total_pieces = 0;

    for (const auto& component : components) {
        int piece_count = 0;
        for (bool has_piece : component.node_) {
            if (has_piece) piece_count++;
        }
        component_sizes.push_back(piece_count);
        total_pieces += piece_count;
    }

    if (total_pieces == 0) {
        return metrics;
    }

    // Calculate metrics
    metrics.average_component_size = static_cast<double>(total_pieces) / components.size();
    metrics.connectivity_ratio = static_cast<double>(total_pieces - components.size()) /
                                std::max(1, total_pieces - 1); // (edges) / (max_possible_edges)

    // Calculate fragmentation penalty (higher for more components)
    metrics.fragmentation_penalty = static_cast<double>(components.size() - 1) /
                                   std::max(1.0, static_cast<double>(total_pieces));

    // Geometric quality and consistency scores would be calculated from actual geometry
    // For now, provide placeholder values
    metrics.geometric_quality_score = 0.8; // Would be calculated from actual matches
    metrics.transformation_consistency_score = 0.75; // Would be calculated from transformation chains

    return metrics;
}

//############################## Main GGCE System Implementation ##############################//

GlobalGraphConnectivityEngine::GlobalGraphConnectivityEngine(const GGCEConfiguration& config)
    : config_(config) {

    // Validate configuration
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid GGCE configuration");
    }

    // Initialize components
    connectivity_analyzer_ = std::make_unique<GlobalConnectivityAnalyzer>(config_);
    merge_engine_ = std::make_unique<InterGraphMergeEngine>(config_);
    priority_manager_ = std::make_unique<ConnectivityPriorityManager>(config_);

    if (config_.debug_mode) {
        std::cout << "GGCE: Initialized with configuration:\n" << config_.toString() << std::endl;
    }
}

bool GlobalGraphConnectivityEngine::shouldApplyGlobalEnhancement(
    const std::vector<RankingSubgraph>& components) const {

    // Only apply if multiple components detected
    if (components.size() <= 1) {
        return false;
    }

    // Apply fragmentation threshold based on algorithm variant
    double fragmentation_threshold;
    if (config_.algorithm_variant == "aggressive") {
        fragmentation_threshold = 1.0; // Always try to connect
    } else if (config_.algorithm_variant == "conservative") {
        fragmentation_threshold = 0.5; // Only for significant fragmentation
    } else { // balanced
        fragmentation_threshold = 0.3;
    }

    auto metrics = connectivity_analyzer_->calculateMetrics(components);
    return metrics.fragmentation_penalty > fragmentation_threshold;
}

std::string GlobalGraphConnectivityEngine::EnhancementResult::toString() const {
    std::ostringstream oss;
    oss << "GGCE Enhancement Result:\n";
    oss << "  Success: " << (success ? "true" : "false") << "\n";
    oss << "  Components: " << initial_component_count << " -> " << final_component_count << "\n";
    oss << "  Connectivity Improvement: " << connectivity_improvement << "\n";
    oss << "  Quality Change: " << geometric_quality_change << "\n";
    oss << "  Processing Time: " << processing_time.count() << "ms\n";

    if (!debug_log.empty()) {
        oss << "  Debug Log:\n";
        for (const auto& log_entry : debug_log) {
            oss << "    " << log_entry << "\n";
        }
    }

    return oss.str();
}

void GlobalGraphConnectivityEngine::logDebugInfo(const std::string& message) {
    if (config_.debug_mode) {
        std::cout << "GGCE Debug: " << message << std::endl;
    }
}

void GlobalGraphConnectivityEngine::updateConfiguration(const GGCEConfiguration& new_config) {
    if (!new_config.validate()) {
        throw std::invalid_argument("Invalid GGCE configuration update");
    }

    config_ = new_config;

    // Reinitialize components with new configuration
    connectivity_analyzer_ = std::make_unique<GlobalConnectivityAnalyzer>(config_);
    merge_engine_ = std::make_unique<InterGraphMergeEngine>(config_);
    priority_manager_ = std::make_unique<ConnectivityPriorityManager>(config_);

    logDebugInfo("Configuration updated");
}

// NOTE: The applyGlobalConnectivityEnhancement implementation will be in the next part
// along with the remaining classes to keep the file manageable.