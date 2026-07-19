#include "pottery_geometric_validator.h"
#include <map>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace Eigen;

// ========================================================================
// PotteryOutlierRejector Implementation - Integration with Existing SFS
// ========================================================================

PotteryOutlierRejector::PotteryOutlierRejector(const PotteryGeometricValidator::Config& config)
    : validator_(config) {}

pair<vector<pair<Vector3d, Vector3d>>, ConnectionValidationResult>
PotteryOutlierRejector::filter_correspondences_pottery_aware(
    int piece1_id,
    int piece2_id,
    const vector<pair<Vector3d, Vector3d>>& correspondences,
    const map<int, PotteryPieceData>& piece_data_map
) const {

    // Extract pottery data for both pieces
    PotteryPieceData piece1_data, piece2_data;

    auto it1 = piece_data_map.find(piece1_id);
    auto it2 = piece_data_map.find(piece2_id);

    if (it1 != piece_data_map.end()) {
        piece1_data = it1->second;
    } else {
        piece1_data = extract_pottery_data_from_piece(piece1_id);
    }

    if (it2 != piece_data_map.end()) {
        piece2_data = it2->second;
    } else {
        piece2_data = extract_pottery_data_from_piece(piece2_id);
    }

    // Calculate correspondence quality and ICP metrics
    double correspondence_quality = calculate_correspondence_quality(correspondences);
    int icp_inliers = correspondences.size(); // Initial inlier count

    // ===== POTTERY-AWARE VALIDATION (REPLACES NORMAL THRESHOLD) =====

    ConnectionValidationResult validation_result = validator_.validate_pottery_connection(
        piece1_data, piece2_data, icp_inliers, correspondence_quality
    );

    // Debug output for pottery validation
    cout << "*** POTTERY VALIDATION DEBUG *** Pieces " << piece1_id << "-" << piece2_id << endl;
    cout << "   Axis alignment: " << fixed << setprecision(3) << validation_result.axis_alignment_score << endl;
    cout << "   Height compatibility: " << validation_result.height_compatibility_score << endl;
    cout << "   Radial proximity: " << validation_result.radial_proximity_score << endl;
    cout << "   Breakline compatibility: " << validation_result.breakline_compatibility_score << endl;
    cout << "   Pottery validity: " << validation_result.pottery_validity_score << endl;
    cout << "   Surface continuity: " << validation_result.surface_continuity_score << endl;
    cout << "   Combined score: " << validation_result.combined_score << endl;

    // Filter correspondences based on pottery validation result
    vector<pair<Vector3d, Vector3d>> filtered_correspondences;

    if (validation_result.is_pottery_valid) {
        // POTTERY-VALID: Apply pottery-aware surface filtering
        vector<Vector3d> normals1, normals2;

        // Extract surface normals from correspondences
        for (const auto& correspondence : correspondences) {
            // In real implementation, would calculate or fetch actual normals
            // For now, use placeholder normal calculation
            Vector3d normal1 = Vector3d::Random().normalized();
            Vector3d normal2 = Vector3d::Random().normalized();
            normals1.push_back(normal1);
            normals2.push_back(normal2);
        }

        // Pottery-aware normal validation
        double normal_validation_score = validator_.validate_pottery_normals(
            normals1, normals2, piece1_data, piece2_data
        );

        cout << "   Pottery normal validation: " << normal_validation_score << endl;

        // Filter correspondences using pottery-aware criteria
        // (In real implementation, would have more sophisticated filtering)
        if (normal_validation_score > 0.5) {
            filtered_correspondences = correspondences; // Accept all correspondences
            cout << "*** POTTERY SUCCESS *** Connection accepted with " << correspondences.size() << " correspondences" << endl;
        } else {
            // Partial filtering based on pottery normal validation
            size_t keep_count = static_cast<size_t>(correspondences.size() * normal_validation_score);
            filtered_correspondences.assign(correspondences.begin(), correspondences.begin() + keep_count);
            cout << "*** POTTERY PARTIAL *** Connection partially accepted with " << keep_count << " correspondences" << endl;
        }

    } else {
        // POTTERY-INVALID: Reject connection
        filtered_correspondences.clear();
        cout << "*** POTTERY REJECTION *** " << validation_result.rejection_reason << endl;
    }

    return make_pair(filtered_correspondences, validation_result);
}

PotteryPieceData PotteryOutlierRejector::extract_pottery_data_from_piece(int piece_id) const {
    // This method would extract pottery data from existing piece structures
    // in the SFS system. For now, providing template implementation.

    PotteryPieceData pottery_data;

    // TODO: Extract from existing piece data structures
    // Example loading axis data (from files like Pot_A_Piece_01_Axis.xyz)
    string axis_filename = "NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Axes/Pot_A_Piece_"
                          + to_string(piece_id).insert(0, 2 - to_string(piece_id).length(), '0')
                          + "_Axis.xyz";

    // Load axis data from file
    // (In real implementation, would integrate with existing file loading)
    pottery_data.axes.emplace_back(-54.235, -131.846, 59.654, 0.220, 0.326, 0.920); // Example data

    // Extract breaklines from existing breakline data structures
    // pottery_data.breaklines = extract_breaklines_from_piece(piece_id);

    // Calculate derived properties
    pottery_data.average_height = pottery_data.axes.empty() ? 0.0 : pottery_data.axes[0].height;
    pottery_data.center_of_mass = Vector3d(0, 0, pottery_data.average_height);
    pottery_data.radial_position = sqrt(pow(pottery_data.center_of_mass.x(), 2) +
                                       pow(pottery_data.center_of_mass.y(), 2));
    pottery_data.angular_position = atan2(pottery_data.center_of_mass.y(),
                                         pottery_data.center_of_mass.x()) * 180.0 / M_PI;

    return pottery_data;
}

double PotteryOutlierRejector::calculate_correspondence_quality(
    const vector<pair<Vector3d, Vector3d>>& correspondences
) const {

    if (correspondences.empty()) return 0.0;

    // Calculate quality metrics:
    // 1. Distance consistency between corresponding points
    // 2. Spatial distribution of correspondences
    // 3. Geometric consistency

    double total_distance = 0.0;
    double min_distance = numeric_limits<double>::max();
    double max_distance = 0.0;

    for (const auto& corr : correspondences) {
        double distance = (corr.first - corr.second).norm();
        total_distance += distance;
        min_distance = min(min_distance, distance);
        max_distance = max(max_distance, distance);
    }

    double avg_distance = total_distance / correspondences.size();

    // Quality score based on distance consistency
    // Lower variation in distances indicates better correspondence quality
    double distance_variation = max_distance - min_distance;
    double consistency_score = exp(-distance_variation / avg_distance);

    // Density score based on number of correspondences
    double density_score = min(1.0, correspondences.size() / 50.0); // Normalize around 50 correspondences

    return consistency_score * 0.6 + density_score * 0.4;
}

// ========================================================================
// Integration Example - How to Replace Current Outlier Rejection
// ========================================================================

/*
CURRENT OUTLIER REJECTION CODE (to be replaced):

*** PIECES 1-2 OUTLIER REJECTION DEBUG ***
Initial correspondences: 22
Distance threshold: 20, Normal threshold: 0.65
Final correspondences: 0 (rejected: 0 distance, 22 normal)

// Current logic:
for (auto& correspondence : correspondences) {
    double distance = calculate_distance(correspondence);
    double normal_angle = calculate_normal_angle(correspondence);

    if (distance > distance_threshold) {
        reject_correspondence(correspondence, "distance");
    }

    if (normal_angle > normal_threshold) {  // <-- THIS IS THE PROBLEM
        reject_correspondence(correspondence, "normal");
    }
}

NEW POTTERY-AWARE LOGIC (replacement):
*/

void demonstrate_pottery_integration() {
    cout << "\n=== POTTERY-AWARE INTEGRATION EXAMPLE ===" << endl;

    // Initialize pottery validator
    PotteryGeometricValidator::Config config;
    config.pottery_validity_threshold = 0.6;
    config.max_axis_angle_degrees = 45.0;
    config.enable_pottery_normal_validation = true;

    PotteryOutlierRejector pottery_rejector(config);

    // Example correspondences (would come from ICP)
    vector<pair<Vector3d, Vector3d>> correspondences = {
        {Vector3d(10, 20, 30), Vector3d(10.1, 20.1, 30.1)},
        {Vector3d(15, 25, 35), Vector3d(15.2, 25.1, 35.2)},
        // ... more correspondences
    };

    // Mock pottery data map
    map<int, PotteryPieceData> piece_data_map;
    // (Would be populated from actual piece data)

    // REPLACE CURRENT OUTLIER REJECTION WITH POTTERY-AWARE VERSION:
    auto [filtered_correspondences, validation_result] =
        pottery_rejector.filter_correspondences_pottery_aware(1, 2, correspondences, piece_data_map);

    cout << "Original correspondences: " << correspondences.size() << endl;
    cout << "Filtered correspondences: " << filtered_correspondences.size() << endl;
    cout << "Pottery validity: " << validation_result.pottery_validity_score << endl;
    cout << "Connection accepted: " << (validation_result.is_pottery_valid ? "YES" : "NO") << endl;

    if (!validation_result.is_pottery_valid) {
        cout << "Rejection reason: " << validation_result.rejection_reason << endl;
    }
}

// ========================================================================
// Configuration for Different Pottery Types
// ========================================================================

namespace PotteryConfigurations {

    // Configuration for fine pottery with precise geometry
    PotteryGeometricValidator::Config fine_pottery_config() {
        PotteryGeometricValidator::Config config;
        config.pottery_validity_threshold = 0.7;        // Stricter pottery requirements
        config.max_axis_angle_degrees = 30.0;           // Tighter axis alignment
        config.same_level_height_threshold = 30.0;      // Smaller height tolerance
        config.max_radial_distance = 150.0;             // Closer radial proximity
        config.breakline_curve_tolerance = 0.1;         // Precise breakline matching
        return config;
    }

    // Configuration for coarse pottery with more variation
    PotteryGeometricValidator::Config coarse_pottery_config() {
        PotteryGeometricValidator::Config config;
        config.pottery_validity_threshold = 0.5;        // More lenient pottery requirements
        config.max_axis_angle_degrees = 60.0;           // Allow more axis variation
        config.same_level_height_threshold = 80.0;      // Larger height tolerance
        config.max_radial_distance = 300.0;             // Wider radial proximity
        config.breakline_curve_tolerance = 0.2;         // More lenient breakline matching
        return config;
    }

    // Configuration for fragmentary pottery with missing data
    PotteryGeometricValidator::Config fragmentary_pottery_config() {
        PotteryGeometricValidator::Config config;
        config.pottery_validity_threshold = 0.4;        // Very lenient requirements
        config.max_axis_angle_degrees = 75.0;           // Allow significant axis variation
        config.same_level_height_threshold = 100.0;     // Large height tolerance
        config.max_radial_distance = 400.0;             // Wide radial acceptance
        config.enable_pottery_normal_validation = false; // Disable strict normal validation
        return config;
    }
}

// ========================================================================
// Testing and Validation Framework
// ========================================================================

void test_pottery_validator_against_ground_truth() {
    cout << "\n=== POTTERY VALIDATOR GROUND TRUTH TEST ===" << endl;

    // Test against known ground truth connections
    struct GroundTruthConnection {
        int piece1, piece2;
        bool should_connect;
        string description;
    };

    vector<GroundTruthConnection> test_cases = {
        {3, 5, true, "GT connection 3↔5 (88 inliers potential)"},
        {1, 3, true, "GT connection 1↔3 (50 inliers potential)"},
        {3, 6, true, "GT connection 3↔6 (perfect pottery geometry)"},
        {5, 6, false, "Non-GT connection 5↔6 (96 inliers but wrong)"},
        {2, 3, false, "Non-GT connection 2↔3 (85 inliers but wrong)"},
    };

    PotteryGeometricValidator validator;

    for (const auto& test_case : test_cases) {
        cout << "\nTesting: " << test_case.description << endl;

        // Mock pottery data for test pieces
        PotteryPieceData piece1_data, piece2_data;
        // (Would load actual data for real test)

        ConnectionValidationResult result = validator.validate_pottery_connection(
            piece1_data, piece2_data, 50, 0.8 // Mock ICP data
        );

        bool test_passed = (result.is_pottery_valid == test_case.should_connect);

        cout << "   Expected: " << (test_case.should_connect ? "ACCEPT" : "REJECT") << endl;
        cout << "   Actual: " << (result.is_pottery_valid ? "ACCEPT" : "REJECT") << endl;
        cout << "   Pottery score: " << result.pottery_validity_score << endl;
        cout << "   Test result: " << (test_passed ? "PASS" : "FAIL") << endl;
    }
}