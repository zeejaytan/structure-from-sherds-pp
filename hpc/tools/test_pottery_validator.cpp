#include "pottery_geometric_validator.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;
using namespace Eigen;

/**
 * Concrete test implementation using real Pot A data
 * Validates pottery-aware geometric validation against known ground truth
 */

// Real axis data from Pot A pieces (from NURBS dataset)
map<int, vector<tuple<double, double, double, double, double, double>>> real_axis_data = {
    {1, {{-54.234979, -131.846461, 59.653882, 0.219556, 0.325761, 0.919606}}},
    {2, {{-117.992094, -68.468343, 88.040807, 0.472213, 0.266610, 0.840199}}},
    {3, {{30.488074, -12.585845, 345.929348, -0.969619, -0.232183, 0.077009},
         {93.590422, 1.371402, 340.613397, -0.963998, -0.022357, 0.264968}}},
    {4, {{101.129082, -63.293830, 87.507797, -0.509524, 0.300586, 0.806247}}},
    {5, {{-207.775726, -29.377107, 278.187343, 0.762038, 0.253348, 0.595914}}},
    {6, {{-65.855052, -35.921239, 317.730421, 0.978496, 0.021035, 0.205188}}},
    {7, {{8.035286, 2.464983, 356.651128, -0.082129, -0.996583, 0.008738}}},
    {8, {{-132.232869, -34.400806, 195.432163, -0.808977, -0.137409, -0.571555},
         {-106.856238, -106.085960, 127.673040, 0.436533, 0.477772, 0.762347}}}
};

// Ground truth adjacency matrix (from Pot_A_simple_graph.txt)
vector<vector<int>> ground_truth_adjacency = {
    {0, 1, 1, 1, 1, 1, 1, 0},  // Piece 1 connects to: 2,3,4,5,6,7
    {1, 0, 0, 1, 1, 0, 0, 1},  // Piece 2 connects to: 1,4,5,8
    {1, 0, 0, 0, 1, 1, 0, 0},  // Piece 3 connects to: 1,5,6
    {1, 1, 0, 0, 0, 1, 1, 1},  // Piece 4 connects to: 1,2,6,7,8
    {1, 1, 1, 0, 0, 0, 0, 0},  // Piece 5 connects to: 1,2,3
    {1, 0, 1, 1, 0, 0, 1, 0},  // Piece 6 connects to: 1,3,4,7
    {1, 0, 0, 1, 0, 1, 0, 0},  // Piece 7 connects to: 1,4,6
    {0, 1, 0, 1, 0, 0, 0, 0},  // Piece 8 connects to: 2,4
};

// Maximum ICP inlier scores from actual SFS run
map<pair<int, int>, int> actual_icp_scores = {
    {{1,2}, 12}, {{1,3}, 50}, {{1,4}, 76}, {{1,5}, 83}, {{1,6}, 38}, {{1,7}, 41},
    {{2,4}, 46}, {{2,5}, 38}, {{2,8}, 41}, {{3,5}, 88}, {{3,6}, 50}, {{4,6}, 50},
    {{4,7}, 32}, {{4,8}, 43}, {{6,7}, 32},
    // Non-ground truth connections that scored high:
    {{5,6}, 96}, {{2,3}, 85}, {{4,5}, 62}, {{1,8}, 52}, {{3,4}, 30}, {{5,8}, 43}
};

PotteryPieceData create_pottery_data(int piece_id) {
    PotteryPieceData pottery_data;

    // Load real axis data
    if (real_axis_data.count(piece_id)) {
        for (const auto& axis_tuple : real_axis_data[piece_id]) {
            pottery_data.axes.emplace_back(
                get<0>(axis_tuple), get<1>(axis_tuple), get<2>(axis_tuple),
                get<3>(axis_tuple), get<4>(axis_tuple), get<5>(axis_tuple)
            );
        }
    }

    // Calculate derived properties
    if (!pottery_data.axes.empty()) {
        pottery_data.average_height = 0.0;
        pottery_data.center_of_mass = Vector3d::Zero();

        for (const auto& axis : pottery_data.axes) {
            pottery_data.average_height += axis.height;
            pottery_data.center_of_mass += axis.position;
        }

        pottery_data.average_height /= pottery_data.axes.size();
        pottery_data.center_of_mass /= pottery_data.axes.size();

        pottery_data.radial_position = sqrt(
            pottery_data.center_of_mass.x() * pottery_data.center_of_mass.x() +
            pottery_data.center_of_mass.y() * pottery_data.center_of_mass.y()
        );

        pottery_data.angular_position = atan2(
            pottery_data.center_of_mass.y(),
            pottery_data.center_of_mass.x()
        ) * 180.0 / M_PI;
    }

    return pottery_data;
}

void test_pottery_validator_comprehensive() {
    cout << "\n=== COMPREHENSIVE POTTERY VALIDATOR TEST ===" << endl;
    cout << "Testing against real Pot A data and ground truth" << endl;
    cout << string(70, '=') << endl;

    // Initialize pottery validator
    PotteryGeometricValidator::Config config;
    config.pottery_validity_threshold = 0.6;
    config.max_axis_angle_degrees = 45.0;
    config.same_level_height_threshold = 50.0;
    config.max_radial_distance = 200.0;
    config.enable_pottery_normal_validation = true;

    PotteryGeometricValidator validator(config);

    // Create pottery data for all pieces
    map<int, PotteryPieceData> piece_data;
    for (int i = 1; i <= 8; ++i) {
        piece_data[i] = create_pottery_data(i);
    }

    // Test results
    int ground_truth_connections = 0;
    int ground_truth_accepted = 0;
    int non_ground_truth_tested = 0;
    int non_ground_truth_rejected = 0;

    cout << "\n1. GROUND TRUTH CONNECTION VALIDATION:" << endl;
    cout << string(50, '-') << endl;

    // Test all ground truth connections
    for (int i = 0; i < 8; ++i) {
        for (int j = i + 1; j < 8; ++j) {
            if (ground_truth_adjacency[i][j] == 1) {
                ground_truth_connections++;

                int piece1 = i + 1;
                int piece2 = j + 1;
                pair<int, int> connection_key = {min(piece1, piece2), max(piece1, piece2)};

                // Get actual ICP score if available
                int icp_inliers = 25; // Default
                if (actual_icp_scores.count(connection_key)) {
                    icp_inliers = actual_icp_scores[connection_key];
                }

                ConnectionValidationResult result = validator.validate_pottery_connection(
                    piece_data[piece1], piece_data[piece2],
                    icp_inliers, 0.8 // Mock correspondence quality
                );

                cout << "GT " << piece1 << "↔" << piece2
                     << " (ICP: " << setw(2) << icp_inliers << " inliers): ";

                if (result.is_pottery_valid) {
                    cout << "✅ ACCEPTED";
                    ground_truth_accepted++;
                } else {
                    cout << "❌ REJECTED";
                }

                cout << " | Pottery: " << fixed << setprecision(3) << result.pottery_validity_score
                     << " | Combined: " << result.combined_score << endl;

                // Detailed breakdown for interesting cases
                if ((piece1 == 3 && piece2 == 5) || (piece1 == 3 && piece2 == 6)) {
                    cout << "     ↳ Axis: " << result.axis_alignment_score
                         << " | Height: " << result.height_compatibility_score
                         << " | Radial: " << result.radial_proximity_score
                         << " | Break: " << result.breakline_compatibility_score << endl;
                }
            }
        }
    }

    cout << "\n2. NON-GROUND TRUTH CONNECTION VALIDATION:" << endl;
    cout << string(50, '-') << endl;

    // Test some high-scoring non-ground truth connections
    vector<pair<int, int>> non_gt_high_scores = {{5,6}, {2,3}, {3,4}, {5,8}};

    for (const auto& connection : non_gt_high_scores) {
        non_ground_truth_tested++;

        int piece1 = connection.first;
        int piece2 = connection.second;
        pair<int, int> connection_key = {min(piece1, piece2), max(piece1, piece2)};

        int icp_inliers = 50; // Default
        if (actual_icp_scores.count(connection_key)) {
            icp_inliers = actual_icp_scores[connection_key];
        }

        ConnectionValidationResult result = validator.validate_pottery_connection(
            piece_data[piece1], piece_data[piece2],
            icp_inliers, 0.8
        );

        cout << "Non-GT " << piece1 << "↔" << piece2
             << " (ICP: " << setw(2) << icp_inliers << " inliers): ";

        if (result.is_pottery_valid) {
            cout << "⚠️  ACCEPTED";
        } else {
            cout << "✅ REJECTED";
            non_ground_truth_rejected++;
        }

        cout << " | Pottery: " << fixed << setprecision(3) << result.pottery_validity_score
             << " | Combined: " << result.combined_score << endl;
    }

    cout << "\n3. PERFORMANCE SUMMARY:" << endl;
    cout << string(50, '-') << endl;

    double gt_recall = static_cast<double>(ground_truth_accepted) / ground_truth_connections;
    double non_gt_rejection_rate = static_cast<double>(non_ground_truth_rejected) / non_ground_truth_tested;

    cout << "Ground Truth Connections: " << ground_truth_connections << endl;
    cout << "Ground Truth Accepted: " << ground_truth_accepted << endl;
    cout << "Ground Truth Recall: " << fixed << setprecision(1) << (gt_recall * 100) << "%" << endl;
    cout << endl;
    cout << "Non-GT High Scores Tested: " << non_ground_truth_tested << endl;
    cout << "Non-GT Correctly Rejected: " << non_ground_truth_rejected << endl;
    cout << "Non-GT Rejection Rate: " << fixed << setprecision(1) << (non_gt_rejection_rate * 100) << "%" << endl;
    cout << endl;

    // Compare with current system results
    cout << "4. COMPARISON WITH CURRENT SYSTEM:" << endl;
    cout << string(50, '-') << endl;
    cout << "Current System GT Recall: 33.3% (5/15 connections found)" << endl;
    cout << "Pottery Validator GT Recall: " << fixed << setprecision(1) << (gt_recall * 100) << "%" << endl;

    double improvement = (gt_recall - 0.333) / 0.333 * 100;
    cout << "Improvement: " << fixed << setprecision(1);
    if (improvement > 0) {
        cout << "+" << improvement << "% better";
    } else {
        cout << improvement << "% worse";
    }
    cout << endl;

    cout << "\n5. KEY INSIGHTS:" << endl;
    cout << string(50, '-') << endl;

    if (gt_recall > 0.6) {
        cout << "✅ SUCCESS: Pottery validator significantly improves GT discovery" << endl;
    } else if (gt_recall > 0.4) {
        cout << "⚠️  PARTIAL: Some improvement, may need parameter tuning" << endl;
    } else {
        cout << "❌ NEEDS WORK: Pottery validator needs refinement" << endl;
    }

    cout << "   → Focus on connections 3↔5 (88 inliers) and 3↔6 (perfect pottery geometry)" << endl;
    cout << "   → These should be accepted by pottery validator" << endl;
    cout << "   → Current normal threshold (0.65) incorrectly rejects them" << endl;
}

void test_specific_problem_cases() {
    cout << "\n=== SPECIFIC PROBLEM CASE ANALYSIS ===" << endl;

    PotteryGeometricValidator validator;

    // Test the specific cases we identified as problems
    cout << "\nConnection 3↔5 (GT, 88 inliers, currently rejected):" << endl;
    PotteryPieceData piece3_data = create_pottery_data(3);
    PotteryPieceData piece5_data = create_pottery_data(5);

    ConnectionValidationResult result_3_5 = validator.validate_pottery_connection(
        piece3_data, piece5_data, 88, 0.9
    );

    cout << "   Pottery validity: " << result_3_5.pottery_validity_score << endl;
    cout << "   Should be accepted: " << (result_3_5.is_pottery_valid ? "YES" : "NO") << endl;

    cout << "\nConnection 3↔6 (GT, perfect pottery geometry, currently rejected):" << endl;
    PotteryPieceData piece6_data = create_pottery_data(6);

    ConnectionValidationResult result_3_6 = validator.validate_pottery_connection(
        piece3_data, piece6_data, 50, 0.8
    );

    cout << "   Pottery validity: " << result_3_6.pottery_validity_score << endl;
    cout << "   Should be accepted: " << (result_3_6.is_pottery_valid ? "YES" : "NO") << endl;

    cout << "\nConnection 5↔6 (Non-GT, 96 inliers, currently accepted):" << endl;
    ConnectionValidationResult result_5_6 = validator.validate_pottery_connection(
        piece5_data, piece6_data, 96, 0.95
    );

    cout << "   Pottery validity: " << result_5_6.pottery_validity_score << endl;
    cout << "   Competition with GT: " << (result_5_6.combined_score < result_3_6.combined_score ?
                                         "GT should win" : "Non-GT might win") << endl;
}

int main() {
    cout << "Pottery-Aware Geometric Validation Test Suite" << endl;
    cout << "Testing against real Pot A data and known ground truth" << endl;

    test_pottery_validator_comprehensive();
    test_specific_problem_cases();

    cout << "\n=== TEST COMPLETE ===" << endl;
    return 0;
}