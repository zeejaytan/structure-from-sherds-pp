#pragma once

#include "data_structure.h"
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <Eigen/Sparse>
#include <Eigen/SparseLU>

// SE(3) optimization types
typedef Eigen::Matrix<double, 6, 1> Vector6d;  // 6DoF vector [tx, ty, tz, rx, ry, rz]
typedef Eigen::VectorXd VectorXd;
typedef Eigen::SparseMatrix<double> SparseMatrixXd;
typedef Eigen::Triplet<double> TripletXd;

// Forward declarations
class RankingSubgraph;

/**
 * MULTI-HYPOTHESIS GLOBAL OPTIMIZATION SYSTEM
 *
 * Implements the global optimization approach from the pseudocode:
 * 1. Multi-hypothesis generation (K=5 per edge pair instead of single best)
 * 2. Global compatibility pruning (cycle consistency, collision checks)
 * 3. Binary switch optimization with alternating pose-switch updates
 * 4. SE(3) factor graph optimization with robust losses
 *
 * Integrates with existing proven infrastructure:
 * - Uses existing ICP transformations and quality scoring
 * - Leverages intersection detection and geometric validation
 * - Builds on PuzzleFusion++ optimization framework
 */

/**
 * Hypothesis for a single edge pair connection
 * Represents one possible way two pieces can connect
 */
struct ConnectionHypothesis {
    int piece_a, piece_b;                    // Connected pieces (1-based)
    Trans icp_transformation;               // ICP transformation
    double geometric_error;                 // ICP score (lower = better)
    int inlier_count;                      // Edge alignment points (higher = better)
    double local_score;                    // Combined local quality
    double switch_weight;                  // Binary switch y_k ∈ [0,1]
    int hypothesis_id;                     // Unique identifier

    // Axis information for debugging
    int axis_x, axis_y;

    ConnectionHypothesis(const LCSIndex& lcs, int hyp_id)
        : piece_a(lcs.shard_x_), piece_b(lcs.shard_y_)
        , icp_transformation(lcs.trans_), geometric_error(lcs.score_)
        , inlier_count(lcs.inliner_), switch_weight(0.0)
        , hypothesis_id(hyp_id), axis_x(lcs.axis_index_x_), axis_y(lcs.axis_index_y_) {

        // Compute local score: emphasize edge alignment
        double edge_quality = std::min(100.0, static_cast<double>(inlier_count) / 2.0);
        double geom_quality = 100.0 / (1.0 + geometric_error);
        local_score = 0.8 * edge_quality + 0.2 * geom_quality;
    }

    // Key for identifying piece pair
    std::pair<int, int> getPiecePair() const {
        return std::make_pair(std::min(piece_a, piece_b), std::max(piece_a, piece_b));
    }
};

/**
 * Global problem state for alternating optimization
 */
struct GlobalProblem {
    std::vector<ConnectionHypothesis> hypotheses;              // All connection hypotheses
    std::map<int, Matrix4d> piece_poses;                       // Current SE(3) poses per piece
    std::map<std::pair<int,int>, std::vector<int>> piece_pair_hypotheses; // Hypotheses per piece pair

    int num_pieces;
    double spatial_weight = 0.8;      // Weight for spatial quality in objective
    double connection_weight = 0.2;   // Weight for connection strength

    GlobalProblem(int pieces) : num_pieces(pieces) {
        // Initialize all pieces at identity
        for (int i = 1; i <= pieces; i++) {
            piece_poses[i] = Matrix4d::Identity();
        }
    }

    // Get currently active hypotheses (switch_weight > threshold)
    std::vector<int> getActiveHypotheses(double threshold = 0.1) const {
        std::vector<int> active;
        for (int i = 0; i < hypotheses.size(); i++) {
            if (hypotheses[i].switch_weight > threshold) {
                active.push_back(i);
            }
        }
        return active;
    }

    // Compute current objective value
    double computeObjective();
};

/**
 * GLOBAL CONSTRAINT SQP SOLVER SYSTEM
 *
 * Implements state-of-the-art Sequential Quadratic Programming (SQP)
 * for constrained optimization on SE(3) manifold with:
 * - Soft residuals: geometry fit + contact bands
 * - Equality constraints: contact equalities + cycle closure
 * - Inequality constraints: non-penetration via SDF
 * - Proper SE(3) manifold optimization with Lie algebra
 */

struct ConstraintSystem {
    // Soft residuals (geometry + contact fit)
    VectorXd soft_residuals;           // R ∈ ℝᵐ
    SparseMatrixXd soft_jacobian;      // J ∈ ℝᵐˣ⁶ⁿ

    // Equality constraints (contact + cycle)
    VectorXd equality_constraints;     // h ∈ ℝᵖ
    SparseMatrixXd equality_jacobian;  // A ∈ ℝᵖˣ⁶ⁿ

    // Inequality constraints (non-penetration)
    VectorXd inequality_constraints;   // g ∈ ℝᵍ
    SparseMatrixXd inequality_jacobian; // G ∈ ℝᵍˣ⁶ⁿ

    // Lagrange multipliers
    VectorXd lambda_eq;    // Equality multipliers
    VectorXd lambda_ineq;  // Inequality multipliers

    // Problem dimensions
    int num_poses;         // N (number of pieces)
    int num_soft;          // M (soft residual count)
    int num_equality;      // P (equality constraint count)
    int num_inequality;    // Q (inequality constraint count)

    ConstraintSystem(int n_poses) : num_poses(n_poses) {
        lambda_eq = VectorXd::Zero(0);
        lambda_ineq = VectorXd::Zero(0);
    }
};

/**
 * Global Constraint Solver using Sequential Quadratic Programming
 * Replaces pairwise constraint solving with global optimization
 */
class GlobalConstraintSolver {
public:
    struct SQPConfig {
        int max_outer_iterations;       // Outer SQP loop
        int max_inner_iterations;       // Inner KKT solve
        double convergence_threshold;   // ||δξ|| convergence
        double constraint_tolerance;    // Constraint violation tolerance

        // Weights and penalties
        double w_fit;                   // Soft geometric consistency
        double w_contact;               // Contact band weight
        double w_cycle;                 // Cycle consistency weight
        double mu_barrier;              // Interior barrier parameter
        double rho_AL;                  // Augmented Lagrangian penalty

        // SDF parameters
        double collision_margin;        // Safety margin for non-penetration
        int sdf_samples_per_piece;      // Surface sampling density

        bool debug_mode;

        SQPConfig() : max_outer_iterations(30), max_inner_iterations(8),
                     convergence_threshold(1e-4), constraint_tolerance(1e-3),
                     w_fit(100.0), w_contact(1000.0), w_cycle(10.0),
                     mu_barrier(1e-4), rho_AL(10.0),
                     collision_margin(10.0), sdf_samples_per_piece(100),
                     debug_mode(true) {}
    };

private:
    SQPConfig config_;
    std::vector<Geom>* shard_data_;  // Reference to geometry data

public:
    GlobalConstraintSolver(const SQPConfig& cfg = SQPConfig()) : config_(cfg) {}

    /**
     * MAIN ENTRY: Solve global constraints using SQP
     * Replaces projectNonPenetration + enforceShortCycleConsistency
     */
    bool solveGlobalConstraints(GlobalProblem& problem, std::vector<Geom>& shard_geometry);

private:
    // === CONSTRAINT ASSEMBLY ===

    /**
     * Assemble all soft residuals (geometry + contact bands)
     */
    void assembleSoftResiduals(const GlobalProblem& problem, ConstraintSystem& system);

    /**
     * Assemble equality constraints (contact + cycle closure)
     */
    void assembleEqualityConstraints(const GlobalProblem& problem, ConstraintSystem& system);

    /**
     * Assemble inequality constraints (non-penetration via SDF)
     */
    void assembleInequalityConstraints(const GlobalProblem& problem, ConstraintSystem& system);

    // === INDIVIDUAL CONSTRAINT TYPES ===

    /**
     * Relative pose residuals between connected pieces
     */
    void addRelativePoseResiduals(const GlobalProblem& problem,
                                 std::vector<TripletXd>& J_triplets,
                                 std::vector<double>& residuals, int& row_idx);

    /**
     * Contact band residuals (point-to-plane distance)
     */
    void addContactBandResiduals(const GlobalProblem& problem,
                                std::vector<TripletXd>& J_triplets,
                                std::vector<double>& residuals, int& row_idx);

    /**
     * Pottery rim shape constraints (legacy RimConstraint integration)
     */
    void addRimShapeConstraints(const GlobalProblem& problem,
                               std::vector<TripletXd>& J_triplets,
                               std::vector<double>& residuals, int& row_idx);

    /**
     * Cycle closure constraints
     */
    void addCycleClosureConstraints(const GlobalProblem& problem,
                                   std::vector<TripletXd>& A_triplets,
                                   std::vector<double>& constraints, int& row_idx);

    /**
     * Non-penetration inequality constraints
     */
    void addNonPenetrationConstraints(const GlobalProblem& problem,
                                     std::vector<TripletXd>& G_triplets,
                                     std::vector<double>& constraints, int& row_idx);

    // === KKT SYSTEM SOLVER ===

    /**
     * Build and solve global KKT system
     * [ J^T J + B_hess + AL_hess   A^T   G^T ] [ δξ ]   [ rhs1 ]
     * [ A                           0     0  ] [ λ_eq ] = [ rhs2 ]
     * [ G_active                    0     0  ] [ λ_ineq]   [ rhs3 ]
     */
    bool solveKKTSystem(const ConstraintSystem& system, VectorXd& pose_increments);

    /**
     * Update SE(3) poses using Lie algebra increments
     * X[i] ← Exp(δξ[i]) ∘ X[i]
     */
    void updatePoses(GlobalProblem& problem, const VectorXd& pose_increments);

    // === SE(3) MANIFOLD OPERATIONS ===

    /**
     * SE(3) exponential map: se(3) → SE(3)
     */
    Matrix4d se3Exp(const Vector6d& xi) const;

    /**
     * SE(3) logarithm map: SE(3) → se(3)
     */
    Vector6d se3Log(const Matrix4d& T) const;

    /**
     * Relative pose residual and Jacobians on SE(3)
     * r = Log(T_measured^{-1} ∘ T_i^{-1} ∘ T_j)
     */
    void relativePoseResidualAndJacobian(const Matrix4d& T_i, const Matrix4d& T_j,
                                        const Matrix4d& T_measured,
                                        Vector6d& residual,
                                        Eigen::Matrix<double, 6, 6>& J_i,
                                        Eigen::Matrix<double, 6, 6>& J_j) const;

    /**
     * Skew-symmetric matrix for cross product
     */
    Matrix3d skewSymmetric(const Vector3d& v) const;

    // === SIGNED DISTANCE FIELD UTILITIES ===

    /**
     * Sample surface points from piece geometry
     */
    std::vector<Vector3d> sampleSurfacePoints(const Geom& piece_geometry, int num_samples);

    /**
     * Query signed distance and gradient
     */
    double querySignedDistance(const Vector3d& point, const Geom& geometry,
                              const Matrix4d& pose, Vector3d& gradient);

    // === DEBUG AND UTILITIES ===

    void debugPrintConstraintSystem(const ConstraintSystem& system, const std::string& stage);
    void debugPrintKKTSolution(const VectorXd& solution, int num_poses);
    bool checkConstraintViolations(const GlobalProblem& problem);
};

/**
 * Multi-Hypothesis Global Optimizer
 * Implements alternating pose-switch optimization
 */
class MultiHypothesisOptimizer {
public:
    struct Config {
        int max_hypotheses_per_pair;        // K in pseudocode
        int max_iterations;                  // Alternating optimization iterations
        double switch_sparsity_lambda;      // Sparsity penalty for switches
        double convergence_threshold;       // Objective change convergence
        bool debug_mode;

        // Cycle consistency parameters
        int max_cycle_length;
        double max_cycle_drift;  // Maximum allowable cycle drift (mm)

        // Collision parameters
        double collision_threshold;  // Volume overlap threshold

        // Default constructor
        Config() : max_hypotheses_per_pair(5), max_iterations(20),
                  switch_sparsity_lambda(1.0), convergence_threshold(0.01),
                  debug_mode(true), max_cycle_length(4), max_cycle_drift(2.0),
                  collision_threshold(0.15) {}
    };

private:
    Config config_;
    std::vector<Geom>* shard_data_;  // Reference to geometry data
    GlobalConstraintSolver global_solver_;  // Global constraint SQP solver

public:
    MultiHypothesisOptimizer(const Config& cfg = Config()) : config_(cfg) {
        // Configure global constraint solver
        GlobalConstraintSolver::SQPConfig sqp_cfg;
        sqp_cfg.max_outer_iterations = config_.max_iterations;
        sqp_cfg.convergence_threshold = config_.convergence_threshold;
        sqp_cfg.collision_margin = 2.0;  // 2mm safety margin
        sqp_cfg.debug_mode = config_.debug_mode;
        global_solver_ = GlobalConstraintSolver(sqp_cfg);
    }

    /**
     * MAIN ENTRY POINT: Global optimization with multi-hypothesis approach
     *
     * @param pieces Number of pottery pieces
     * @param connections All LCS connections (before aggressive pruning)
     * @param shard_geometry Geometry data for collision checking
     * @return Optimized assembly result
     */
    RankingSubgraph optimizeGlobal(int pieces,
                                  const std::vector<LCSIndex>& connections,
                                  std::vector<Geom>& shard_geometry);

private:
    /**
     * STEP A: Build multi-hypothesis set (K hypotheses per edge pair)
     */
    GlobalProblem buildMultiHypotheses(int pieces, const std::vector<LCSIndex>& connections);

    /**
     * STEP B: Global compatibility pruning
     */
    void pruneByGlobalCompatibility(GlobalProblem& problem);

    /**
     * STEP C: Alternating pose-switch optimization
     */
    void optimizeAlternating(GlobalProblem& problem);

    /**
     * Pose optimization step: fix switches, optimize SE(3) poses
     */
    void poseStep(GlobalProblem& problem);

    /**
     * Switch optimization step: fix poses, optimize binary switches
     */
    void switchStep(GlobalProblem& problem, int iteration = 0);

    /**
     * Find initial pose for a piece using best available hypothesis
     */
    Matrix4d findInitialPose(const GlobalProblem& problem, int piece_id, const std::vector<int>& active_hyps);

    /**
     * Cycle consistency checking
     */
    bool checkCycleConsistency(const GlobalProblem& problem,
                              const std::vector<int>& cycle_hypotheses) const;

    /**
     * Collision detection using existing intersection detector
     */
    bool checkCollision(const GlobalProblem& problem, int hyp_id) const;

    /**
     * Pottery axis alignment check - reject inverted/opposite axes
     * Returns true if axis check FAILED (should be pruned)
     */
    bool checkAxisAlignment(int piece_a_id, int piece_b_id) const;

    /**
     * Convert optimized global problem to RankingSubgraph result
     */
    RankingSubgraph convertToRankingResult(const GlobalProblem& problem);

    /**
     * SE(3) ↔ se(3) conversion helpers for proper Lie algebra optimization
     */
    Vector6d matrixToSE3Vector(const Matrix4d& T) const;
    Matrix4d se3VectorToMatrix(const Vector6d& xi) const;

    /**
     * GLOBAL CONSTRAINT OPTIMIZATION: Replaces pairwise hard constraints
     * Uses Sequential Quadratic Programming (SQP) for proper global optimization
     */
    bool solveGlobalConstraints(GlobalProblem& problem);

    /**
     * Initialize piece poses using ICP transformations for realistic starting positions
     */
    void initializePosesFromICP(GlobalProblem& problem);

    /**
     * Debug utilities
     */
    void debugPrintHypotheses(const GlobalProblem& problem, const std::string& stage) const;
    void debugPrintSwitches(const GlobalProblem& problem) const;
    void debugPrintCycles(const GlobalProblem& problem) const;
};

/**
 * INTEGRATION FUNCTION: Replace existing global optimization calls
 */
RankingSubgraph performMultiHypothesisOptimization(int pieces,
                                                  const std::vector<LCSIndex>& connections,
                                                  std::vector<Geom>& shard_geometry);