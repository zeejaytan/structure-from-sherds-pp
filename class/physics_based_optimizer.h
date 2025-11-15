#ifndef PHYSICS_BASED_OPTIMIZER_H
#define PHYSICS_BASED_OPTIMIZER_H

#include <vector>
#include <map>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include "multi_hypothesis_optimizer.h"

/**
 * PHYSICS-BASED POTTERY ASSEMBLY OPTIMIZER
 * ========================================
 *
 * Replaces broken local scoring with real pottery physics constraints:
 * 1. SE(3) Pose Graph Optimization - Global pose consistency
 * 2. Breakline Geometry Matching - Real pottery edge alignment
 * 3. 3D Collision Detection - No piece intersections
 * 4. Curvature Continuity - Surface smoothness across connections
 * 5. Assembly Topology - Connected vessel structure
 */

using Matrix4d = Eigen::Matrix<double, 4, 4>;
using Vector3d = Eigen::Vector3d;
using Quaterniond = Eigen::Quaterniond;

struct SE3Pose {
    Vector3d translation;
    Quaterniond rotation;

    SE3Pose() : translation(Vector3d::Zero()), rotation(Quaterniond::Identity()) {}

    Matrix4d toMatrix() const {
        Matrix4d T = Matrix4d::Identity();
        T.block<3,3>(0,0) = rotation.toRotationMatrix();
        T.block<3,1>(0,3) = translation;
        return T;
    }

    SE3Pose compose(const SE3Pose& other) const {
        SE3Pose result;
        result.rotation = rotation * other.rotation;
        result.translation = translation + rotation * other.translation;
        return result;
    }

    SE3Pose inverse() const {
        SE3Pose result;
        result.rotation = rotation.inverse();
        result.translation = -(result.rotation * translation);
        return result;
    }
};

struct BreaklineGeometry {
    pcl::PointCloud<pcl::PointNormal>::Ptr points;
    std::vector<Vector3d> normals;
    std::vector<double> curvatures;
    std::vector<Vector3d> principal_directions;

    BreaklineGeometry() : points(new pcl::PointCloud<pcl::PointNormal>) {}
};

struct PhysicsHypothesis {
    int piece1, piece2;
    SE3Pose relative_transform;
    double alignment_score;
    double curvature_continuity;
    double geometric_error;
    bool is_collision_free;

    // Breakline matching data
    std::vector<std::pair<int,int>> breakline_correspondences;
    double breakline_alignment_error;
    double normal_alignment_error;
};

class PhysicsBasedOptimizer {
public:
    struct Config {
        // Optimization weights
        double pose_consistency_weight = 1.0;
        double breakline_alignment_weight = 0.8;
        double collision_penalty_weight = 100.0;  // High penalty for intersections
        double curvature_continuity_weight = 0.5;
        double topology_weight = 0.3;

        // Thresholds
        double pose_consistency_threshold = 0.01;  // meters
        double normal_alignment_threshold = 0.1;   // radians
        double curvature_match_threshold = 0.05;   // 1/meter
        double collision_margin = 0.002;           // 2mm minimum separation

        // Optimization parameters
        int max_iterations = 100;
        double convergence_threshold = 1e-6;
        bool enable_collision_checking = true;
        bool enable_curvature_matching = true;
    };

private:
    Config config_;

    // Piece data
    std::vector<SE3Pose> global_poses_;
    std::vector<BreaklineGeometry> breakline_geometries_;
    std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> piece_meshes_;

    // Optimization state
    std::vector<PhysicsHypothesis> physics_hypotheses_;
    std::vector<std::vector<bool>> selection_matrix_;  // [pair][hypothesis]

public:
    PhysicsBasedOptimizer(const Config& config = Config()) : config_(config) {}

    // ============================================================================
    // MAIN OPTIMIZATION INTERFACE
    // ============================================================================

    /**
     * Replace the broken computeObjective() with physics-based optimization
     */
    RankingSubgraph optimizeWithPhysics(
        int pieces,
        const std::vector<LCSIndex>& connections,
        std::vector<Geom>& shard_geometry);

    // ============================================================================
    // 1. SE(3) POSE GRAPH OPTIMIZATION
    // ============================================================================

    /**
     * Enforce global pose consistency across all connections
     * Constraint: ||Log(T_k^(-1) * X_i^(-1) * X_j)||_2 ≤ ε
     */
    double computePoseConsistencyError(const std::vector<int>& selected_hypotheses);
    void optimizePoseGraph(std::vector<int>& selected_hypotheses);
    Matrix4d logSE3(const Matrix4d& T) const;

    // ============================================================================
    // 2. BREAKLINE GEOMETRY MATCHING
    // ============================================================================

    /**
     * Match actual pottery breakline geometry, not arbitrary surface points
     */
    void extractBreaklineGeometry(int piece_id, const Geom& geometry);
    double computeBreaklineAlignmentError(const PhysicsHypothesis& hyp);
    double computeNormalAlignmentError(const PhysicsHypothesis& hyp);

    // ============================================================================
    // 3. 3D COLLISION DETECTION
    // ============================================================================

    /**
     * Ensure no piece-piece intersections in final assembly
     */
    bool checkCollisionFree(const std::vector<int>& selected_hypotheses);
    bool checkMeshMeshIntersection(int piece1, int piece2,
                                  const SE3Pose& relative_pose);
    double computeMinimumSeparation(int piece1, int piece2,
                                   const SE3Pose& relative_pose);

    // ============================================================================
    // 4. CURVATURE CONTINUITY
    // ============================================================================

    /**
     * Ensure surface curvature is continuous across connections
     */
    double computeCurvatureContinuity(const PhysicsHypothesis& hyp);
    void computeSurfaceCurvature(int piece_id, const Geom& geometry);

    // ============================================================================
    // 5. ASSEMBLY TOPOLOGY CONSTRAINTS
    // ============================================================================

    /**
     * Ensure selected connections form proper vessel topology
     */
    bool validateTopology(const std::vector<int>& selected_hypotheses);
    bool formsConnectedComponent(const std::vector<int>& selected_hypotheses);
    bool hasValidVesselGenus(const std::vector<int>& selected_hypotheses);

    // ============================================================================
    // PHYSICS OBJECTIVE FUNCTION
    // ============================================================================

    /**
     * Complete physics-based objective function
     * Replaces broken sum(inlier_counts) with real pottery constraints
     */
    double computePhysicsObjective(const std::vector<int>& selected_hypotheses);

    // ============================================================================
    // MIXED-INTEGER OPTIMIZATION
    // ============================================================================

    /**
     * Solve the discrete selection problem with physics constraints
     * Uses branch-and-bound with physics-guided pruning
     */
    std::vector<int> solveMixedInteger();
    std::vector<int> branchAndBound(std::vector<int> partial_solution,
                                   double current_best_score);

    // ============================================================================
    // INTEGRATION WITH EXISTING SYSTEM
    // ============================================================================

    /**
     * Convert physics hypotheses back to existing RankingSubgraph format
     */
    RankingSubgraph convertToRankingResult(const std::vector<int>& selected_hypotheses);
    void initializeFromExistingHypotheses(const GlobalProblem& problem);

    // ============================================================================
    // DEBUG AND ANALYSIS
    // ============================================================================

    void debugPhysicsConstraints(const std::vector<int>& selected_hypotheses);
    void visualizePhysicsResults(const std::vector<int>& selected_hypotheses);
    void printConstraintViolations(const std::vector<int>& selected_hypotheses);
};

#endif // PHYSICS_BASED_OPTIMIZER_H