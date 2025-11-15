#ifndef GROUND_TRUTH_DEBUG_H
#define GROUND_TRUTH_DEBUG_H

#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>

class GroundTruthDebugger {
public:
    // Ground truth connectivity matrix for Pot A (1-indexed)
    static bool isGroundTruthConnection(int piece1, int piece2) {
        // Convert to 0-indexed for matrix lookup
        int p1 = piece1 - 1;
        int p2 = piece2 - 1;

        if (p1 < 0 || p1 >= 8 || p2 < 0 || p2 >= 8) return false;

        // Pot A ground truth matrix (symmetric)
        static bool gt_matrix[8][8] = {
            {0, 1, 1, 1, 1, 1, 1, 0},  // Piece 1
            {1, 0, 0, 1, 1, 0, 0, 1},  // Piece 2
            {1, 0, 0, 0, 1, 1, 0, 0},  // Piece 3
            {1, 1, 0, 0, 0, 1, 1, 1},  // Piece 4
            {1, 1, 1, 0, 0, 0, 0, 0},  // Piece 5
            {1, 0, 1, 1, 0, 0, 1, 0},  // Piece 6
            {1, 0, 0, 1, 0, 1, 0, 0},  // Piece 7
            {0, 1, 0, 1, 0, 0, 0, 0}   // Piece 8
        };

        return gt_matrix[p1][p2];
    }

    // Track rejection reasons
    enum RejectionStage {
        FEATURE_MATCHING_STAGE,
        ICP_CORRESPONDENCE_STAGE,
        ICP_CONVERGENCE_STAGE,
        INLIER_CALCULATION_STAGE,
        PAIRWISE_PRUNING_STAGE,
        SURVIVED
    };

    struct RejectionInfo {
        int piece1, piece2;
        RejectionStage stage;
        std::string reason;
        double value;  // Score, distance, angle, etc.
        double threshold;
    };

    static std::vector<RejectionInfo> rejections;

    // Track all legitimate connections through pipeline
    static void trackConnection(int piece1, int piece2, RejectionStage stage,
                               const std::string& reason, double value = 0.0, double threshold = 0.0) {
        if (isGroundTruthConnection(piece1, piece2)) {
            RejectionInfo info;
            info.piece1 = piece1;
            info.piece2 = piece2;
            info.stage = stage;
            info.reason = reason;
            info.value = value;
            info.threshold = threshold;
            rejections.push_back(info);

            std::cout << "*** GROUND TRUTH DEBUG *** Pieces " << piece1 << "-" << piece2
                      << " (" << getStageString(stage) << "): " << reason;
            if (value != 0.0) {
                std::cout << " (value=" << value;
                if (threshold != 0.0) {
                    std::cout << ", threshold=" << threshold;
                }
                std::cout << ")";
            }
            std::cout << std::endl;
        }
    }

    static std::string getStageString(RejectionStage stage) {
        switch(stage) {
            case FEATURE_MATCHING_STAGE: return "FEATURE_MATCHING";
            case ICP_CORRESPONDENCE_STAGE: return "ICP_CORRESPONDENCE";
            case ICP_CONVERGENCE_STAGE: return "ICP_CONVERGENCE";
            case INLIER_CALCULATION_STAGE: return "INLIER_CALCULATION";
            case PAIRWISE_PRUNING_STAGE: return "PAIRWISE_PRUNING";
            case SURVIVED: return "SURVIVED";
            default: return "UNKNOWN";
        }
    }

    // Generate final report
    static void generateReport() {
        std::cout << "\n#################### GROUND TRUTH DEBUG REPORT ####################" << std::endl;
        std::cout << "Expected legitimate connections: 14" << std::endl;

        // Count by stage
        std::map<RejectionStage, int> stage_counts;
        std::map<std::pair<int,int>, RejectionStage> final_stage;

        for (const auto& rejection : rejections) {
            std::pair<int,int> pair = {rejection.piece1, rejection.piece2};
            stage_counts[rejection.stage]++;
            final_stage[pair] = rejection.stage;
        }

        std::cout << "\nRejection breakdown:" << std::endl;
        for (const auto& count : stage_counts) {
            std::cout << "  " << getStageString(count.first) << ": " << count.second << " connections" << std::endl;
        }

        std::cout << "\nDetailed rejections:" << std::endl;
        for (const auto& rejection : rejections) {
            std::cout << "  Pieces " << rejection.piece1 << "-" << rejection.piece2
                      << " @ " << getStageString(rejection.stage) << ": " << rejection.reason;
            if (rejection.value != 0.0) {
                std::cout << " (value=" << rejection.value;
                if (rejection.threshold != 0.0) {
                    std::cout << " vs threshold=" << rejection.threshold;
                }
                std::cout << ")";
            }
            std::cout << std::endl;
        }

        std::cout << "#################### END GROUND TRUTH DEBUG REPORT ####################\n" << std::endl;
    }

    // Clear for new run
    static void reset() {
        rejections.clear();
    }
};

// Static member declaration (definition moved to main file to avoid multiple definition)

#endif // GROUND_TRUTH_DEBUG_H