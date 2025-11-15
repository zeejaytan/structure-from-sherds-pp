#pragma once

#include <vector>
#include <Eigen/Dense>
#include <random>
#include <set>
#include "data_structure.h"

// POTTERY-AWARE GEOMETRIC VALIDATION (SIMPLIFIED FOR INTEGRATION)
#include "../pottery_geometric_validator_simple.h"

using namespace std;
using namespace Eigen;

// Configuration constants for robust ICP
#define COARSE_NORMAL_THRESHOLD     0.3    // Very permissive initial threshold
#define COARSE_DISTANCE_THRESHOLD   50.0   // Large initial distance tolerance
#define COARSE_MIN_CORRESPONDENCES  5      // Low minimum requirement for coarse stage

#define FINE_NORMAL_THRESHOLD       0.4    // Final target threshold (FIXED: was 0.65, too strict)
#define FINE_DISTANCE_THRESHOLD     35.0   // Final target distance (FIXED: was 20.0, too strict)
#define THRESHOLD_DECAY_RATE        0.85   // Rate of threshold tightening per iteration

#define MAX_ICP_ITERATIONS          15     // Maximum refinement iterations
#define CONVERGENCE_THRESHOLD       0.001  // Transformation convergence criteria
#define RANSAC_TRIALS              50     // Number of RANSAC validation trials
#define RANSAC_SAMPLE_SIZE         4      // Minimum points for transformation estimation

#define FINAL_MIN_INLIERS          6      // Final minimum inlier requirement (increased from 3)

// Result structure for robust ICP
struct RobustICPResult {
    Matrix4d transformation;
    double confidence;
    int inlier_count;
    bool success;
    string failure_reason;

    RobustICPResult() : transformation(Matrix4d::Identity()),
                       confidence(0.0), inlier_count(0), success(false),
                       failure_reason("Not executed") {}
};

// Main robust ICP function
RobustICPResult robustICP(const Corres& input_correspondences,
                         const BreakLine& piece_A,
                         const BreakLine& piece_B,
                         int piece_A_id = -1,
                         int piece_B_id = -1);

// Stage 1: Coarse Alignment
vector<CorPair> establishCoarseCorrespondences(const BreakLine& piece_A,
                                              const BreakLine& piece_B,
                                              double normal_threshold,
                                              double distance_threshold,
                                              int piece_A_id = -1,
                                              int piece_B_id = -1);

Matrix4d performCoarseAlignment(const vector<CorPair>& correspondences,
                               int max_iterations = 10);

// Stage 2: Iterative Refinement
Matrix4d iterativeRefinement(const BreakLine& piece_A,
                            BreakLine& piece_B,  // Non-const for transformation
                            Matrix4d initial_transform,
                            int piece_A_id = -1,
                            int piece_B_id = -1,
                            int max_iterations = MAX_ICP_ITERATIONS);

vector<CorPair> updateCorrespondences(const BreakLine& piece_A,
                                     const BreakLine& piece_B,
                                     double current_threshold,
                                     int piece_A_id = -1,
                                     int piece_B_id = -1);

bool hasConverged(const Matrix4d& delta_transform,
                 double threshold = CONVERGENCE_THRESHOLD);

// Stage 3: Robust Validation (RANSAC-style)
RobustICPResult robustValidation(const vector<CorPair>& all_correspondences,
                                const Matrix4d& candidate_transform,
                                int piece_A_id = -1,
                                int piece_B_id = -1,
                                const BreakLine* piece_A_ptr = nullptr,
                                const BreakLine* piece_B_ptr = nullptr);

Matrix4d computeTransformationFromSample(const vector<CorPair>& sample);

int countValidInliers(const vector<CorPair>& correspondences,
                     const Matrix4d& transform,
                     double distance_threshold = FINE_DISTANCE_THRESHOLD,
                     double normal_threshold = FINE_NORMAL_THRESHOLD,
                     int piece_A_id = -1,
                     int piece_B_id = -1,
                     const BreakLine* piece_A_ptr = nullptr,
                     const BreakLine* piece_B_ptr = nullptr);

// Utility functions
void applyTransformation(BreakLine& piece, const Matrix4d& transform);
double computePointToPlaneError(const Vector3d& point,
                               const Vector3d& plane_point,
                               const Vector3d& plane_normal);
vector<CorPair> randomSample(const vector<CorPair>& correspondences,
                            int sample_size);

// Integration with existing system
bool robustInlierCalculate(int& inlier,
                          const vector<bool>& true_node,
                          const vector<Corres>& cor,
                          const vector<BreakLine>& L,
                          bool volume_weight = false);

// POTTERY-AWARE VALIDATION FUNCTIONS
bool isPotteryValidationEnabled();
bool isPotteryValidConnection(int piece_a_id, int piece_b_id,
                            const Vector3d& point_a, const Vector3d& normal_a,
                            const Vector3d& point_b, const Vector3d& normal_b);