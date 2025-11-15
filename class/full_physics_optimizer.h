#ifndef FULL_PHYSICS_OPTIMIZER_H
#define FULL_PHYSICS_OPTIMIZER_H

#include "multi_hypothesis_optimizer.h"
#include "pottery_structure.h"
#include <vector>
#include <map>
#include <set>
#include <Eigen/Dense>

/**
 * FULL PHYSICS-BASED OPTIMIZATION SYSTEM
 * ======================================
 *
 * Building on proven discrete selection (100% recall), adds comprehensive
 * physics constraints for precision improvement (target: reduce 13 false positives).
 *
 * Architecture:
 *   Layer 1: Discrete selection per piece pair (PROVEN)
 *   Layer 2: Physics constraint validation
 *   Layer 3: Multi-constraint scoring and filtering
 *
 * Physics Constraints:
 *   - Collision detection (geometric overlap prevention)
 *   - Curvature continuity (surface smoothness)
 *   - SE(3) pose consistency (global transformation validation)
 *   - Breakline alignment (edge matching quality)
 *   - Spatial relationship validation
 */
class FullPhysicsOptimizer {
public:
    FullPhysicsOptimizer();

    // Main optimization interface
    std::vector<int> optimizeWithFullPhysics(const GlobalProblem& problem);

    // Physics constraint components
    struct PhysicsScore {
        double collision_score = 0.0;          // 0.0 = collision, 1.0 = safe
        double curvature_score = 0.0;          // 0.0 = discontinuous, 1.0 = smooth
        double pose_consistency_score = 0.0;   // 0.0 = inconsistent, 1.0 = consistent
        double breakline_alignment_score = 0.0; // 0.0 = poor, 1.0 = perfect
        double spatial_validity_score = 0.0;   // 0.0 = invalid, 1.0 = valid
        double pottery_constraint_score = 0.0; // 0.0 = violates vessel structure, 1.0 = valid

        double combined_score = 0.0;           // Weighted combination
        bool passes_hard_constraints = false;  // Must be true to be selected
    };

    // Layer 1: Discrete Selection (PROVEN FOUNDATION)
    std::vector<int> selectBestPerPair(const GlobalProblem& problem);
    std::map<std::pair<int,int>, std::vector<int>> groupHypothesesByPair(const GlobalProblem& problem);

    // Layer 2: Physics Constraint Validation
    PhysicsScore evaluatePhysicsConstraints(const GlobalProblem& problem,
                                          int hypothesis_idx,
                                          const std::vector<int>& current_selection);

    // Individual constraint evaluators
    double evaluateCollisionConstraints(const GlobalProblem& problem, int hyp_idx,
                                      const std::vector<int>& selection);
    double evaluateCurvatureContinuity(const GlobalProblem& problem, int hyp_idx);
    double evaluateSE3PoseConsistency(const GlobalProblem& problem, int hyp_idx,
                                     const std::vector<int>& selection);
    double evaluateBreaklineAlignment(const GlobalProblem& problem, int hyp_idx);
    double evaluateSpatialValidity(const GlobalProblem& problem, int hyp_idx);

    // POTTERY-SPECIFIC CONSTRAINT EVALUATION
    double evaluatePotteryStructureConstraints(const GlobalProblem& problem, int hyp_idx,
                                             const std::vector<int>& selection);

    // Layer 3: Multi-Constraint Integration
    std::vector<int> applyPhysicsFilters(const GlobalProblem& problem,
                                       const std::vector<int>& candidates);
    double computeWeightedPhysicsScore(const PhysicsScore& scores);

    // Geometric analysis utilities
    bool checkGeometricOverlap(const Eigen::Matrix4d& transform1,
                             const Eigen::Matrix4d& transform2,
                             int piece_a, int piece_b);

    double computeCurvatureAtInterface(const GlobalProblem& problem, int hyp_idx);

    Eigen::Matrix4d extractTransformationMatrix(const GlobalProblem& problem, int hyp_idx);

    bool validateGlobalTransformChain(const std::vector<Eigen::Matrix4d>& transforms,
                                    const std::vector<std::pair<int,int>>& connections);

    // Configuration parameters
    struct PhysicsConfig {
        // Constraint weights (sum to 1.0)
        double collision_weight = 0.25;
        double curvature_weight = 0.2;
        double pose_consistency_weight = 0.2;
        double breakline_weight = 0.15;
        double spatial_weight = 0.05;
        double pottery_constraint_weight = 0.15;  // POTTERY STRUCTURE CONSTRAINTS

        // Hard constraint thresholds (POTTERY-REALISTIC: Allow archaeological wear/damage)
        double min_collision_score = 0.3;      // Allow minor overlaps from reconstruction artifacts
        double min_curvature_score = 0.25;     // Pottery curvature can vary due to handcraft
        double min_pose_consistency = 0.4;     // RELAXED: Archaeological pieces have geometric wear
        double min_pottery_constraint = 0.4;   // Vessel structure with flexibility
        double min_combined_score = 0.55;      // RAISED: Maintain quality via combined score

        // Geometric tolerances
        double collision_tolerance = 0.001;    // 1mm overlap tolerance
        double curvature_tolerance = 0.1;      // Curvature change tolerance
        double transformation_tolerance = 0.01; // SE(3) consistency tolerance
    };

    PhysicsConfig config;

    // Debugging and analysis
    void printPhysicsAnalysis(const GlobalProblem& problem,
                            const std::vector<int>& selection,
                            const std::vector<PhysicsScore>& scores);

    void printConstraintDetails(int hypothesis_idx, const PhysicsScore& score);

private:
    // Internal state for global consistency tracking
    std::map<std::pair<int,int>, Eigen::Matrix4d> active_transformations;
    std::set<int> assembled_pieces;

    // Geometric analysis cache
    std::map<int, double> curvature_cache;
    std::map<std::pair<int,int>, bool> collision_cache;

    // POTTERY STRUCTURE DETECTION - replaces ground truth violations with domain knowledge
    PotteryStructureDetector pottery_detector_;
    bool pottery_analysis_complete_ = false;

    // Ground truth references removed - physics optimizer must be vessel-agnostic
    // All ground truth validation moved to external post-analysis tools

    void initializeGroundTruthReference();
    void initializePotteryStructureAnalysis(const GlobalProblem& problem);

    // Pottery constraint helper functions
    double evaluateStructuralCoherence(int piece_a, int piece_b);
    double evaluateVesselAxisAlignment(const GlobalProblem& problem, int hypothesis_idx);
    double evaluateVesselHeightConsistency(const GlobalProblem& problem, int hypothesis_idx);
    double evaluateConnectionDensity(int piece_a, int piece_b, const std::vector<int>& current_selection);

    // Pottery axis-based coordinate computation
    double computePieceAngleAroundAxis(int piece_id);
    double computePieceHeightOnAxis(int piece_id);

    // Pottery layer classification
    enum PotteryLayer { BASE_LAYER, BODY_LAYER, RIM_LAYER };
    PotteryLayer classifyPieceLayer(int piece_id);
};

// Helper functions for geometric computations
namespace PhysicsGeometry {
    double computeSurfaceCurvature(const std::vector<Eigen::Vector3d>& points,
                                 const std::vector<Eigen::Vector3d>& normals);

    bool detectCollision(const std::vector<Eigen::Vector3d>& mesh_a,
                        const std::vector<Eigen::Vector3d>& mesh_b,
                        const Eigen::Matrix4d& transform_a,
                        const Eigen::Matrix4d& transform_b,
                        double tolerance);

    double evaluateBreaklineQuality(const std::vector<Eigen::Vector3d>& edge_a,
                                  const std::vector<Eigen::Vector3d>& edge_b,
                                  const Eigen::Matrix4d& alignment_transform);

    Eigen::Matrix4d composeSE3Transforms(const std::vector<Eigen::Matrix4d>& transforms);

    double computeTransformationError(const Eigen::Matrix4d& expected,
                                    const Eigen::Matrix4d& actual);
}

#endif // FULL_PHYSICS_OPTIMIZER_H