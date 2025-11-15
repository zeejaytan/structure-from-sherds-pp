#include "hub_guided_beam_search.h"
#include <iostream>
#include <algorithm>

void HubGuidedBeamSearch::initializeHubGuidance(const std::vector<LCSIndex>& lcs_reference) {
    if (config_.debug_mode) {
        std::cout << "Hub Guidance: Initializing with " << lcs_reference.size()
                  << " LCS connections" << std::endl;
    }

    // Identify optimal hub piece
    optimal_hub_ = identifyOptimalHub(lcs_reference);

    if (optimal_hub_.piece_id == -1) {
        if (config_.debug_mode) {
            std::cout << "Hub Guidance: No valid hub found" << std::endl;
        }
        initialized_ = false;
        return;
    }

    // Compute hub-related scores for all pieces
    computePieceHubScores();

    if (config_.debug_mode) {
        std::cout << "Hub Guidance: Initialized with hub piece " << optimal_hub_.piece_id
                  << " (connects to " << optimal_hub_.connection_count
                  << " pieces, score=" << optimal_hub_.hub_score << ")" << std::endl;
    }

    initialized_ = true;
}

HubGuidedBeamSearch::HubCandidate HubGuidedBeamSearch::identifyOptimalHub(
    const std::vector<LCSIndex>& lcs_reference) {

    std::map<int, HubCandidate> piece_stats;

    // Analyze connection patterns
    for (const auto& lcs : lcs_reference) {
        int piece_x = lcs.shard_x_;
        int piece_y = lcs.shard_y_;
        double match_strength = static_cast<double>(lcs.size_);

        // Update stats for piece_x
        if (piece_stats.find(piece_x) == piece_stats.end()) {
            piece_stats[piece_x].piece_id = piece_x;
        }
        piece_stats[piece_x].connected_pieces.insert(piece_y);
        piece_stats[piece_x].total_match_strength += match_strength;

        // Update stats for piece_y
        if (piece_stats.find(piece_y) == piece_stats.end()) {
            piece_stats[piece_y].piece_id = piece_y;
        }
        piece_stats[piece_y].connected_pieces.insert(piece_x);
        piece_stats[piece_y].total_match_strength += match_strength;
    }

    // Calculate total number of pieces dynamically
    int total_pieces = piece_stats.size();

    // Calculate maximum possible connections and match strength for normalization
    double max_possible_connections = static_cast<double>(total_pieces - 1); // A piece can connect to all others
    double max_match_strength = 0.0;
    for (const auto& [pid, stats] : piece_stats) {
        if (stats.total_match_strength > max_match_strength) {
            max_match_strength = stats.total_match_strength;
        }
    }

    // Prevent division by zero
    if (max_match_strength == 0.0) max_match_strength = 1.0;

    // Calculate hub scores and find optimal
    HubCandidate best_hub;
    double max_hub_score = 0.0;

    for (auto& [piece_id, candidate] : piece_stats) {
        candidate.connection_count = static_cast<int>(candidate.connected_pieces.size());

        // Skip pieces with insufficient connections (dynamic threshold)
        int min_connections = std::max(1, static_cast<int>(total_pieces * config_.min_connectivity_ratio));
        if (candidate.connection_count < min_connections) {
            continue;
        }

        // Calculate normalized hub score using dynamic values
        double connectivity_score = static_cast<double>(candidate.connection_count) / max_possible_connections;
        double strength_score = candidate.total_match_strength / max_match_strength;

        candidate.hub_score = connectivity_score * 0.7 + strength_score * 0.3;

        if (config_.debug_mode) {
            std::cout << "Hub Analysis: Piece " << piece_id
                      << " connects to " << candidate.connection_count << " pieces"
                      << " (strength=" << candidate.total_match_strength
                      << ", hub_score=" << candidate.hub_score << ")" << std::endl;
        }

        if (candidate.hub_score > max_hub_score) {
            max_hub_score = candidate.hub_score;
            best_hub = candidate;
        }
    }

    return best_hub;
}

double HubGuidedBeamSearch::computeEnhancedScore(int piece_x, int piece_y, double base_score) {
    if (!initialized_ || optimal_hub_.piece_id == -1) {
        return base_score;
    }

    bool involves_hub = (piece_x == optimal_hub_.piece_id || piece_y == optimal_hub_.piece_id);

    if (involves_hub) {
        // Boost connections involving the hub
        double enhanced_score = base_score * config_.hub_boost_factor;

        if (config_.debug_mode) {
            std::cout << "Hub Guidance: BOOSTING connection " << piece_x << "-" << piece_y
                      << " (involves hub " << optimal_hub_.piece_id
                      << "): " << base_score << " → " << enhanced_score << std::endl;
        }

        return enhanced_score;
    }

    // Check for hub bypass (direct connections between hub-connected pieces)
    if (wouldBypassHub(piece_x, piece_y)) {
        double penalized_score = base_score * config_.bypass_penalty;

        if (config_.debug_mode) {
            std::cout << "Hub Guidance: PENALIZING bypass connection " << piece_x << "-" << piece_y
                      << " (bypasses hub " << optimal_hub_.piece_id
                      << "): " << base_score << " → " << penalized_score << std::endl;
        }

        return penalized_score;
    }

    return base_score;
}

bool HubGuidedBeamSearch::wouldBypassHub(int piece_x, int piece_y) const {
    if (optimal_hub_.piece_id == -1) {
        return false;
    }

    // Check if both pieces connect to hub and this would bypass it
    bool x_connects_to_hub = optimal_hub_.connected_pieces.count(piece_x) > 0;
    bool y_connects_to_hub = optimal_hub_.connected_pieces.count(piece_y) > 0;

    // This is a bypass if both pieces connect to hub but neither is the hub itself
    return (x_connects_to_hub && y_connects_to_hub &&
            piece_x != optimal_hub_.piece_id && piece_y != optimal_hub_.piece_id);
}

void HubGuidedBeamSearch::computePieceHubScores() {
    piece_hub_scores_.clear();

    for (int piece_id : optimal_hub_.connected_pieces) {
        piece_hub_scores_[piece_id] = config_.hub_boost_factor;
    }

    piece_hub_scores_[optimal_hub_.piece_id] = config_.hub_boost_factor;
}