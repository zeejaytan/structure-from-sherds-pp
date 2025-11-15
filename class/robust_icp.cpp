#include "robust_icp.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <cstdlib>

using namespace std;

// =============================================================================
// STAGE 1: COARSE ALIGNMENT WITH PERMISSIVE THRESHOLDS
// =============================================================================

vector<CorPair> establishCoarseCorrespondences(const BreakLine& piece_A,
                                              const BreakLine& piece_B,
                                              double normal_threshold,
                                              double distance_threshold,
                                              int piece_A_id,
                                              int piece_B_id) {
    vector<CorPair> correspondences;

    cout << "*** ROBUST ICP STAGE 1 *** Establishing coarse correspondences" << endl;
    cout << "Normal threshold: " << normal_threshold << ", Distance threshold: " << distance_threshold << endl;

    int points_A = piece_A.point_.cols();
    int points_B = piece_B.point_.cols();

    // Find nearest neighbors with permissive thresholds
    for (int i = 0; i < points_A; i++) {
        Vector3d point_A = piece_A.point_.col(i);
        Vector3d normal_A = piece_A.normal_.col(i);

        double best_distance = distance_threshold * 2.0; // Start above threshold
        int best_match = -1;

        for (int j = 0; j < points_B; j++) {
            Vector3d point_B = piece_B.point_.col(j);
            Vector3d normal_B = piece_B.normal_.col(j);

            // Compute distance
            double distance = (point_A - point_B).norm();
            if (distance >= distance_threshold) continue;

            // POTTERY-FIRST VALIDATION: Pottery validation replaces normal threshold
            bool valid_connection = false;

            if (isPotteryValidationEnabled() && piece_A_id > 0 && piece_B_id > 0) {
                // PRIMARY: Use pottery validation (bypasses normal threshold for pottery connections)
                valid_connection = SimplePotteryValidator::isPotteryValidContact(piece_A_id, piece_B_id, point_A, normal_A, point_B, normal_B);
            } else {
                // FALLBACK: Use normal threshold validation for non-pottery connections
                double normal_dot = abs(normal_A.dot(normal_B));
                valid_connection = (normal_dot >= normal_threshold);
            }

            if (!valid_connection) continue;

            // Track best match for this point
            if (distance < best_distance) {
                best_distance = distance;
                best_match = j;
            }
        }

        // Add correspondence if found
        if (best_match >= 0) {
            CorPair corr;
            corr.p_A = point_A;
            corr.p_B = piece_B.point_.col(best_match);
            corr.n_A = normal_A;
            corr.n_B = piece_B.normal_.col(best_match);
            correspondences.push_back(corr);
        }
    }

    cout << "*** COARSE CORRESPONDENCES *** Found " << correspondences.size()
         << " correspondences from " << points_A << " points" << endl;

    return correspondences;
}

Matrix4d performCoarseAlignment(const vector<CorPair>& correspondences,
                               int max_iterations) {
    cout << "*** ROBUST ICP STAGE 1 *** Performing coarse alignment with "
         << correspondences.size() << " correspondences" << endl;

    if (correspondences.size() < COARSE_MIN_CORRESPONDENCES) {
        cout << "*** COARSE ALIGNMENT FAILED *** Insufficient correspondences: "
             << correspondences.size() << " < " << COARSE_MIN_CORRESPONDENCES << endl;
        return Matrix4d::Identity();
    }

    Matrix4d transform = Matrix4d::Identity();

    for (int iter = 0; iter < max_iterations; iter++) {
        // Compute centroids
        Vector3d centroid_A = Vector3d::Zero();
        Vector3d centroid_B = Vector3d::Zero();

        for (const auto& corr : correspondences) {
            centroid_A += corr.p_A;
            centroid_B += corr.p_B;
        }
        centroid_A /= correspondences.size();
        centroid_B /= correspondences.size();

        // Compute cross-covariance matrix
        Matrix3d H = Matrix3d::Zero();
        for (const auto& corr : correspondences) {
            Vector3d p_A = corr.p_A - centroid_A;
            Vector3d p_B = corr.p_B - centroid_B;
            H += p_A * p_B.transpose();
        }

        // SVD for rotation
        JacobiSVD<Matrix3d> svd(H, ComputeFullU | ComputeFullV);
        Matrix3d R = svd.matrixV() * svd.matrixU().transpose();

        // Ensure proper rotation (det = 1)
        if (R.determinant() < 0) {
            Matrix3d V = svd.matrixV();
            V.col(2) *= -1;
            R = V * svd.matrixU().transpose();
        }

        // Compute translation
        Vector3d t = centroid_B - R * centroid_A;

        // Build transformation matrix
        Matrix4d iteration_transform = Matrix4d::Identity();
        iteration_transform.block<3,3>(0,0) = R;
        iteration_transform.block<3,1>(0,3) = t;

        // Check convergence
        if (hasConverged(iteration_transform)) {
            cout << "*** COARSE ALIGNMENT CONVERGED *** After " << iter + 1 << " iterations" << endl;
            break;
        }

        transform = iteration_transform * transform;
    }

    return transform;
}

// =============================================================================
// STAGE 2: ITERATIVE REFINEMENT WITH ADAPTIVE THRESHOLDS
// =============================================================================

Matrix4d iterativeRefinement(const BreakLine& piece_A,
                            BreakLine& piece_B,
                            Matrix4d initial_transform,
                            int piece_A_id,
                            int piece_B_id,
                            int max_iterations) {
    cout << "*** ROBUST ICP STAGE 2 *** Starting iterative refinement" << endl;

    Matrix4d current_transform = initial_transform;
    double current_normal_threshold = COARSE_NORMAL_THRESHOLD;
    double current_distance_threshold = COARSE_DISTANCE_THRESHOLD;

    // Apply initial transformation
    applyTransformation(piece_B, initial_transform);

    for (int iter = 0; iter < max_iterations; iter++) {
        cout << "*** REFINEMENT ITERATION " << iter + 1 << " *** Thresholds: normal="
             << current_normal_threshold << ", distance=" << current_distance_threshold << endl;

        // Re-establish correspondences with current alignment
        vector<CorPair> refined_correspondences = updateCorrespondences(
            piece_A, piece_B, current_normal_threshold, piece_A_id, piece_B_id);

        if (refined_correspondences.size() < COARSE_MIN_CORRESPONDENCES) {
            cout << "*** REFINEMENT WARNING *** Low correspondence count: "
                 << refined_correspondences.size() << endl;
        }

        // Compute transformation improvement
        Matrix4d delta_transform = performCoarseAlignment(refined_correspondences, 5);

        // Apply delta transformation
        applyTransformation(piece_B, delta_transform);
        current_transform = delta_transform * current_transform;

        // Gradually tighten thresholds
        current_normal_threshold = max(FINE_NORMAL_THRESHOLD,
                                     current_normal_threshold * THRESHOLD_DECAY_RATE);
        current_distance_threshold = max(FINE_DISTANCE_THRESHOLD,
                                       current_distance_threshold * THRESHOLD_DECAY_RATE);

        // Check convergence
        if (hasConverged(delta_transform)) {
            cout << "*** ITERATIVE REFINEMENT CONVERGED *** After " << iter + 1 << " iterations" << endl;
            break;
        }
    }

    return current_transform;
}

vector<CorPair> updateCorrespondences(const BreakLine& piece_A,
                                     const BreakLine& piece_B,
                                     double current_threshold,
                                     int piece_A_id,
                                     int piece_B_id) {
    return establishCoarseCorrespondences(piece_A, piece_B,
                                        current_threshold,
                                        FINE_DISTANCE_THRESHOLD,
                                        piece_A_id, piece_B_id); // NOW PROPAGATING PIECE IDs FOR POTTERY VALIDATION
}

bool hasConverged(const Matrix4d& delta_transform,
                 double threshold) {
    // Check translation magnitude
    Vector3d translation = delta_transform.block<3,1>(0,3);
    if (translation.norm() > threshold) return false;

    // Check rotation magnitude (Frobenius norm of R - I)
    Matrix3d rotation = delta_transform.block<3,3>(0,0);
    Matrix3d rotation_delta = rotation - Matrix3d::Identity();
    if (rotation_delta.norm() > threshold) return false;

    return true;
}

// =============================================================================
// STAGE 3: ROBUST VALIDATION WITH RANSAC-STYLE TESTING
// =============================================================================

RobustICPResult robustValidation(const vector<CorPair>& all_correspondences,
                                const Matrix4d& candidate_transform,
                                int piece_A_id,
                                int piece_B_id,
                                const BreakLine* piece_A_ptr,
                                const BreakLine* piece_B_ptr) {
    cout << "*** ROBUST ICP STAGE 3 *** Starting RANSAC validation with "
         << all_correspondences.size() << " correspondences" << endl;

    RobustICPResult result;

    if (all_correspondences.size() < FINAL_MIN_INLIERS) {
        result.failure_reason = "Insufficient correspondences for validation";
        cout << "*** VALIDATION FAILED *** " << result.failure_reason << endl;
        return result;
    }

    vector<Matrix4d> candidate_transforms;
    vector<int> inlier_counts;

    random_device rd;
    mt19937 gen(rd());

    // Test multiple correspondence subsets
    for (int trial = 0; trial < RANSAC_TRIALS; trial++) {
        // Sample small subset of correspondences
        vector<CorPair> sample = randomSample(all_correspondences, RANSAC_SAMPLE_SIZE);

        // Compute transformation from sample
        Matrix4d trial_transform = performCoarseAlignment(sample, 5);

        // Count inliers for this transformation - WITH POTTERY TYPOLOGY CHECKS
        int inliers = countValidInliers(all_correspondences, trial_transform,
                                       FINE_DISTANCE_THRESHOLD, FINE_NORMAL_THRESHOLD,
                                       piece_A_id, piece_B_id, piece_A_ptr, piece_B_ptr);

        candidate_transforms.push_back(trial_transform);
        inlier_counts.push_back(inliers);
    }

    // Also test the input candidate transform
    candidate_transforms.push_back(candidate_transform);
    int candidate_inliers = countValidInliers(all_correspondences, candidate_transform,
                                             FINE_DISTANCE_THRESHOLD, FINE_NORMAL_THRESHOLD,
                                             piece_A_id, piece_B_id, piece_A_ptr, piece_B_ptr);
    inlier_counts.push_back(candidate_inliers);

    // Choose best transformation
    auto max_it = max_element(inlier_counts.begin(), inlier_counts.end());
    int best_idx = distance(inlier_counts.begin(), max_it);

    result.transformation = candidate_transforms[best_idx];
    result.inlier_count = inlier_counts[best_idx];
    result.confidence = (double)result.inlier_count / all_correspondences.size();
    result.success = result.inlier_count >= FINAL_MIN_INLIERS;

    if (!result.success) {
        result.failure_reason = "Insufficient final inliers: " + to_string(result.inlier_count)
                              + " < " + to_string(FINAL_MIN_INLIERS);
    }

    cout << "*** RANSAC VALIDATION *** Best result: " << result.inlier_count
         << " inliers, confidence: " << result.confidence
         << ", success: " << (result.success ? "true" : "false") << endl;

    return result;
}

Matrix4d computeTransformationFromSample(const vector<CorPair>& sample) {
    return performCoarseAlignment(sample, 5);
}

int countValidInliers(const vector<CorPair>& correspondences,
                     const Matrix4d& transform,
                     double distance_threshold,
                     double normal_threshold,
                     int piece_A_id,
                     int piece_B_id,
                     const BreakLine* piece_A_ptr,
                     const BreakLine* piece_B_ptr) {
    int inliers = 0;

    Matrix3d R = transform.block<3,3>(0,0);
    Vector3d t = transform.block<3,1>(0,3);

    // POTTERY TYPOLOGY: Check axis alignment once per piece pair
    bool axis_check_passed = true;

    // DEBUG: Check if pottery validation and pointers are available
    if (isPotteryValidationEnabled()) {
        cout << "*** AXIS CHECK DEBUG *** Pieces " << piece_A_id << "-" << piece_B_id
             << " pottery_enabled=true"
             << " piece_A_ptr=" << (piece_A_ptr ? "valid" : "null")
             << " piece_B_ptr=" << (piece_B_ptr ? "valid" : "null");

        if (piece_A_ptr && piece_B_ptr) {
            cout << " axis_A_size=" << piece_A_ptr->axis_norm_.size()
                 << " axis_B_size=" << piece_B_ptr->axis_norm_.size() << endl;

            if (piece_A_ptr->axis_norm_.size() > 0 && piece_B_ptr->axis_norm_.size() > 0) {
                Vector3d axis_A = piece_A_ptr->axis_norm_[0].normalized();
                Vector3d axis_B = piece_B_ptr->axis_norm_[0].normalized();

                cout << "*** AXIS DATA LOADED *** Piece " << piece_A_id << " axis=["
                     << axis_A.transpose() << "], Piece " << piece_B_id << " axis=["
                     << axis_B.transpose() << "]" << endl;

                axis_check_passed = SimplePotteryValidator::checkAxisAlignment(axis_A, axis_B);

                if (!axis_check_passed) {
                    cout << "*** AXIS ALIGNMENT FAILED *** Pieces " << piece_A_id << "-" << piece_B_id
                         << " have misaligned axes - rejecting all correspondences" << endl;
                    return 0;  // Reject entire connection if axes don't align
                }
            } else {
                cout << "*** AXIS CHECK SKIPPED *** Pieces " << piece_A_id << "-" << piece_B_id
                     << " - axis_norm_ vectors are empty" << endl;
            }
        } else {
            cout << endl;
        }
    }

    for (const auto& corr : correspondences) {
        // Transform point B
        Vector3d transformed_B = R * corr.p_B + t;

        // Transform normal B
        Vector3d transformed_normal_B = R * corr.n_B;

        // POTTERY-FIRST INLIER CALCULATION: Archaeological validity overrides geometric perfection
        bool valid_inlier = false;

        if (isPotteryValidationEnabled() && piece_A_id > 0 && piece_B_id > 0) {
            // PRIMARY: Pottery-focused inlier validation - prioritize pottery structure compatibility
            double contact_distance = (corr.p_A - transformed_B).norm();

            // Pottery validation for archaeological appropriateness
            bool pottery_valid = SimplePotteryValidator::isPotteryValidContact(piece_A_id, piece_B_id, corr.p_A, corr.n_A, transformed_B, transformed_normal_B);

            if (pottery_valid) {
                // STRICT EDGE TOUCHING POLICY: pottery_valid means <5mm edge contact (from isPotteryValidContact)
                // Only correspondences with edges physically touching reach this point
                valid_inlier = true;

                // GRADUATED INLIER SCORING: Reward edge quality based on contact tightness
                // All distances here are <5mm since pottery_valid=true requires it
                if (contact_distance <= 2.0) {
                    inliers += 3;  // Perfect edge contact - extremely valuable
                    continue;
                } else if (contact_distance <= 4.0) {
                    inliers += 2;  // Excellent edge contact - within 4mm
                    continue;
                } else {
                    inliers += 1;  // Good edge contact - within 5mm (passed pottery_valid check)
                }
            } else {
                // POTTERY VALIDATION FAILED: This connection is not archaeologically appropriate
                // Do not count as inlier regardless of geometric metrics
                continue;
            }
        } else {
            // FALLBACK: Traditional geometric validation for non-pottery connections
            double distance = (corr.p_A - transformed_B).norm();
            if (distance >= distance_threshold) continue;

            double normal_dot = abs(corr.n_A.dot(transformed_normal_B));
            valid_inlier = (normal_dot >= normal_threshold);
        }

        if (valid_inlier) {
            inliers++;
        }
    }

    return inliers;
}

// =============================================================================
// MAIN ROBUST ICP FUNCTION
// =============================================================================

RobustICPResult robustICP(const Corres& input_correspondences,
                         const BreakLine& piece_A,
                         const BreakLine& piece_B,
                         int piece_A_id,
                         int piece_B_id) {
    cout << "*** ROBUST ICP START *** Processing " << input_correspondences.cor.size()
         << " input correspondences" << endl;

    RobustICPResult result;

    // Create mutable copy of piece_B for transformations
    BreakLine piece_B_copy = piece_B;

    // Stage 1: Coarse Alignment
    vector<CorPair> coarse_correspondences = establishCoarseCorrespondences(
        piece_A, piece_B_copy, COARSE_NORMAL_THRESHOLD, COARSE_DISTANCE_THRESHOLD,
        piece_A_id, piece_B_id);

    if (coarse_correspondences.size() < COARSE_MIN_CORRESPONDENCES) {
        result.failure_reason = "Stage 1 failed: insufficient coarse correspondences";
        cout << "*** ROBUST ICP FAILED *** " << result.failure_reason << endl;
        return result;
    }

    Matrix4d coarse_transform = performCoarseAlignment(coarse_correspondences);

    // Stage 2: Iterative Refinement
    Matrix4d refined_transform = iterativeRefinement(piece_A, piece_B_copy, coarse_transform, piece_A_id, piece_B_id);

    // Establish final correspondences for validation
    vector<CorPair> final_correspondences = updateCorrespondences(
        piece_A, piece_B_copy, FINE_NORMAL_THRESHOLD, piece_A_id, piece_B_id);

    // Stage 3: Robust Validation - WITH POTTERY TYPOLOGY CHECKS
    result = robustValidation(final_correspondences, refined_transform, piece_A_id, piece_B_id,
                             &piece_A, &piece_B);

    cout << "*** ROBUST ICP COMPLETE *** Success: " << (result.success ? "true" : "false")
         << ", Inliers: " << result.inlier_count << ", Confidence: " << result.confidence << endl;

    return result;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void applyTransformation(BreakLine& piece, const Matrix4d& transform) {
    Matrix3d R = transform.block<3,3>(0,0);
    Vector3d t = transform.block<3,1>(0,3);

    // Transform points
    for (int i = 0; i < piece.point_.cols(); i++) {
        Vector3d point = piece.point_.col(i);
        piece.point_.col(i) = R * point + t;
    }

    // Transform normals (rotation only)
    for (int i = 0; i < piece.normal_.cols(); i++) {
        Vector3d normal = piece.normal_.col(i);
        piece.normal_.col(i) = R * normal;
    }
}

double computePointToPlaneError(const Vector3d& point,
                               const Vector3d& plane_point,
                               const Vector3d& plane_normal) {
    Vector3d diff = point - plane_point;
    return abs(diff.dot(plane_normal));
}

vector<CorPair> randomSample(const vector<CorPair>& correspondences,
                            int sample_size) {
    vector<CorPair> sample;

    if (correspondences.size() <= sample_size) {
        return correspondences;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, correspondences.size() - 1);

    set<int> selected_indices;
    while (selected_indices.size() < sample_size) {
        selected_indices.insert(dis(gen));
    }

    for (int idx : selected_indices) {
        sample.push_back(correspondences[idx]);
    }

    return sample;
}

// =============================================================================
// INTEGRATION WITH EXISTING SYSTEM
// =============================================================================

bool robustInlierCalculate(int& inlier,
                          const vector<bool>& true_node,
                          const vector<Corres>& cor,
                          const vector<BreakLine>& L,
                          bool volume_weight) {
    cout << "*** ROBUST INLIER CALCULATION *** Processing " << cor.size() << " correspondences" << endl;

    inlier = 0;

    if (cor.empty()) {
        cout << "*** ROBUST ICP WARNING *** No correspondences provided" << endl;
        return false;
    }

    // Process each correspondence pair
    for (const auto& correspondence : cor) {
        if (correspondence.cor.empty()) continue;

        int piece_A_idx = correspondence.index_A - 1;
        int piece_B_idx = correspondence.index_B - 1;

        if (piece_A_idx >= L.size() || piece_B_idx >= L.size()) continue;

        // Run robust ICP on this correspondence pair
        RobustICPResult result = robustICP(correspondence, L[piece_A_idx], L[piece_B_idx],
                                         correspondence.index_A, correspondence.index_B);

        if (result.success) {
            inlier += result.inlier_count;
            cout << "*** ROBUST ICP SUCCESS *** Pieces " << correspondence.index_A
                 << "-" << correspondence.index_B << " contributed "
                 << result.inlier_count << " inliers" << endl;
        } else {
            cout << "*** ROBUST ICP FAILED *** Pieces " << correspondence.index_A
                 << "-" << correspondence.index_B << ": " << result.failure_reason << endl;
        }
    }

    cout << "*** ROBUST INLIER TOTAL *** " << inlier << " inliers from "
         << cor.size() << " correspondence pairs" << endl;

    return inlier >= FINAL_MIN_INLIERS;
}

// =============================================================================
// POTTERY-AWARE VALIDATION FUNCTIONS
// =============================================================================

bool isPotteryValidationEnabled() {
    // Check environment variable
    const char* env_pottery = getenv("ENABLE_POTTERY_VALIDATION");
    if (env_pottery && (string(env_pottery) == "1" || string(env_pottery) == "true")) {
        cout << "*** POTTERY VALIDATION *** ENABLED via environment variable" << endl;
        return true;
    }
    return false;
}

// OLD FUNCTION REMOVED - Now using SimplePotteryValidator::isPotteryValidContact() directly