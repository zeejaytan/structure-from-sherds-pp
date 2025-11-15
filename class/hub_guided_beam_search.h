#ifndef HUB_GUIDED_BEAM_SEARCH_H
#define HUB_GUIDED_BEAM_SEARCH_H

#include "data_structure.h"
#include <map>
#include <set>
#include <vector>

/**
 * Hub-Guided Beam Search System
 *
 * Prevents 3-graph fragmentation by identifying optimal hub piece
 * and biasing beam search scoring toward hub-centric connectivity.
 */
class HubGuidedBeamSearch {
public:
    struct HubCandidate {
        int piece_id;
        int connection_count;
        double total_match_strength;
        double hub_score;
        std::set<int> connected_pieces;

        HubCandidate() : piece_id(-1), connection_count(0),
                        total_match_strength(0.0), hub_score(0.0) {}
    };

    struct Config {
        double hub_boost_factor = 1.5;        // 50% boost for hub connections
        double bypass_penalty = 0.7;          // 30% penalty for bypassing hub
        double min_connectivity_ratio = 0.3;  // Minimum 30% connectivity to be considered hub (dynamic)
        bool debug_mode = false;

        Config() {}
    };

private:
    Config config_;
    HubCandidate optimal_hub_;
    std::map<int, double> piece_hub_scores_;
    bool initialized_;

public:
    HubGuidedBeamSearch(const Config& config = Config())
        : config_(config), initialized_(false) {}

    /**
     * Initialize hub guidance system before beam search starts
     */
    void initializeHubGuidance(const std::vector<LCSIndex>& lcs_reference);

    /**
     * Compute enhanced connection score with hub bias
     */
    double computeEnhancedScore(int piece_x, int piece_y, double base_score);

    /**
     * Get optimal hub information
     */
    const HubCandidate& getOptimalHub() const { return optimal_hub_; }

    /**
     * Check if system is initialized
     */
    bool isInitialized() const { return initialized_; }

private:
    HubCandidate identifyOptimalHub(const std::vector<LCSIndex>& lcs_reference);
    bool wouldBypassHub(int piece_x, int piece_y) const;
    void computePieceHubScores();
};

#endif // HUB_GUIDED_BEAM_SEARCH_H