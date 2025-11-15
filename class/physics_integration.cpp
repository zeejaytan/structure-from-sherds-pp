/**
 * PHYSICS INTEGRATION - REPLACE BROKEN LOCAL SCORING
 * =================================================
 *
 * This file shows exactly how to replace the broken computeObjective()
 * function in multi_hypothesis_optimizer.cpp with physics-based optimization.
 *
 * CRITICAL FIX: The current system optimizes sum(all_hypothesis_scores)
 * instead of selecting one hypothesis per piece pair with physics constraints.
 */

#include "multi_hypothesis_optimizer.h"
#include "physics_based_optimizer.h"
#include <iostream>

using namespace std;

/**
 * STEP 1: Replace the broken computeObjective() function
 *
 * CURRENT BROKEN CODE (lines 26-50 in multi_hypothesis_optimizer.cpp):
 *
 * double GlobalProblem::computeObjective() const {
 *     for (const auto& hyp : hypotheses) {
 *         spatial_score += hyp.switch_weight * hyp.local_score;        // WRONG!
 *         connection_score += hyp.switch_weight * hyp.inlier_count;    // WRONG!
 *     }
 * }
 *
 * This sums ALL active hypotheses instead of selecting one per piece pair.
 * Result: 124 hypotheses → 27 connections covering only 3 piece pairs.
 */

// FIXED VERSION:
double GlobalProblem::computePhysicsObjective() const {

    cout << "🚨 REPLACING BROKEN LOCAL SCORING WITH PHYSICS CONSTRAINTS" << endl;

    // Initialize physics optimizer
    PhysicsBasedOptimizer::Config physics_config;
    physics_config.pose_consistency_weight = 1.0;
    physics_config.breakline_alignment_weight = 0.8;
    physics_config.collision_penalty_weight = 100.0;
    physics_config.curvature_continuity_weight = 0.5;

    PhysicsBasedOptimizer physics_optimizer(physics_config);

    // Convert current hypotheses to physics-aware format
    vector<int> current_selection = extractActiveHypotheses();

    // Compute physics-based objective instead of broken sum
    double physics_score = physics_optimizer.computePhysicsObjective(current_selection);

    cout << "📊 Physics score: " << physics_score << " (lower is better)" << endl;

    return physics_score;  // Return physics score, not broken local sum
}

/**
 * STEP 2: Replace the discrete selection logic
 *
 * CURRENT PROBLEM: System converts ALL hypotheses to connections
 * PHYSICS FIX: Select one hypothesis per piece pair using MILP
 */

RankingSubgraph MultiHypothesisOptimizer::optimizeWithPhysicsConstraints(
    int pieces,
    const std::vector<LCSIndex>& connections,
    std::vector<Geom>& shard_geometry) {

    cout << "\n🏺 PHYSICS-BASED POTTERY ASSEMBLY (REPLACING BROKEN SYSTEM)" << endl;
    cout << "===========================================" << endl;

    // STEP A: Initialize physics optimizer
    PhysicsBasedOptimizer::Config config;
    config.pose_consistency_weight = 1.0;      // Critical: SE(3) pose consistency
    config.breakline_alignment_weight = 0.8;   // Match actual breakline geometry
    config.collision_penalty_weight = 100.0;   // Prevent piece intersections
    config.curvature_continuity_weight = 0.5;  // Surface smoothness
    config.topology_weight = 0.3;              // Connected vessel structure

    PhysicsBasedOptimizer physics_optimizer(config);

    // STEP B: Replace broken optimization with physics-based selection
    cout << "🔧 Replacing broken local scoring with physics constraints" << endl;
    RankingSubgraph result = physics_optimizer.optimizeWithPhysics(pieces, connections, shard_geometry);

    cout << "✅ Physics-based optimization complete" << endl;
    return result;
}

/**
 * STEP 3: Integration points in existing code
 *
 * MODIFY: multi_hypothesis_optimizer.cpp line 94
 * CHANGE: RankingSubgraph result = convertToRankingResult(problem);
 * TO:     RankingSubgraph result = optimizeWithPhysicsConstraints(pieces, connections, shard_geometry);
 */

/**
 * DEMONSTRATION: Why physics constraints find ground truth
 * ========================================================
 */

void demonstratePhysicsAdvantage() {
    cout << "\n🎯 WHY PHYSICS CONSTRAINTS FIND GROUND TRUTH CONNECTIONS" << endl;
    cout << "=========================================================" << endl;

    cout << "❌ CURRENT BROKEN SYSTEM:" << endl;
    cout << "   • Optimizes: sum(all_hypothesis_inlier_counts)" << endl;
    cout << "   • Result: Selects 20+ hypotheses from pairs (1-4), (1-5), (2-3)" << endl;
    cout << "   • Coverage: Only 3 piece pairs out of 21 possible" << endl;
    cout << "   • Ground truth recall: 13.3% (2/15 connections)" << endl;

    cout << "\n✅ PHYSICS-BASED SYSTEM:" << endl;
    cout << "   • Optimizes: SE(3) pose consistency + breakline alignment + physics" << endl;
    cout << "   • Constraint: Exactly one hypothesis per piece pair" << endl;
    cout << "   • Result: Global poses must be consistent across all connections" << endl;
    cout << "   • Coverage: All 21 piece pairs with proper physics" << endl;

    cout << "\n🔑 KEY INSIGHT:" << endl;
    cout << "   Ground truth connections may have LOWER inlier counts but" << endl;
    cout << "   they satisfy GLOBAL PHYSICS CONSTRAINTS that fake connections violate:" << endl;
    cout << "   • SE(3) pose composition: T_ij * T_jk = T_ik" << endl;
    cout << "   • Breakline geometry matching: Normals align, curvature continuous" << endl;
    cout << "   • No physical intersections in final assembly" << endl;
    cout << "   • Connected vessel topology" << endl;

    cout << "\n📈 EXPECTED RESULTS:" << endl;
    cout << "   • Ground truth recall: 80%+ (12+/15 connections)" << endl;
    cout << "   • Precision: 70%+ (minimal false positives)" << endl;
    cout << "   • F1 score improvement: 200%+" << endl;
}

/**
 * CRITICAL IMPLEMENTATION NOTES
 * =============================
 */

void implementationNotes() {
    cout << "\n⚠️  CRITICAL IMPLEMENTATION REQUIREMENTS" << endl;
    cout << "========================================" << endl;

    cout << "1. LIBRARY DEPENDENCIES:" << endl;
    cout << "   • Eigen3: SE(3) pose optimization" << endl;
    cout << "   • PCL: Point cloud processing, curvature estimation" << endl;
    cout << "   • Optional: g2o or Ceres for robust pose graph optimization" << endl;
    cout << "   • Optional: Gurobi/CPLEX for optimal MILP solving" << endl;

    cout << "\n2. DATA INTEGRATION:" << endl;
    cout << "   • Extract breakline geometry from existing PCD files" << endl;
    cout << "   • Load surface meshes from XYZ files for collision detection" << endl;
    cout << "   • Integrate with existing Geom structure" << endl;

    cout << "\n3. OPTIMIZATION STRATEGY:" << endl;
    cout << "   • Phase 1: Discrete selection (branch-and-bound or MILP)" << endl;
    cout << "   • Phase 2: Continuous pose optimization given fixed selection" << endl;
    cout << "   • Phase 3: Refinement with physics validation" << endl;

    cout << "\n4. PERFORMANCE CONSIDERATIONS:" << endl;
    cout << "   • Hypothesis pruning: Remove obviously infeasible connections" << endl;
    cout << "   • Spatial indexing: KD-trees for efficient collision detection" << endl;
    cout << "   • Parallel evaluation: Multiple hypothesis evaluations in parallel" << endl;

    cout << "\n5. VALIDATION:" << endl;
    cout << "   • Test on Pot A data: Should achieve 12+/15 ground truth connections" << endl;
    cout << "   • Visualize final assembly: No intersections, proper alignment" << endl;
    cout << "   • Compare metrics: Precision, recall, F1 vs current system" << endl;
}

/**
 * IMMEDIATE TESTING STRATEGY
 * =========================
 */

void testingStrategy() {
    cout << "\n🧪 IMMEDIATE TESTING STRATEGY" << endl;
    cout << "=============================" << endl;

    cout << "PHASE 1: Proof of Concept (1-2 days)" << endl;
    cout << "   • Implement discrete selection only (one hypothesis per pair)" << endl;
    cout << "   • Use existing inlier counts but enforce selection constraints" << endl;
    cout << "   • Test on job 16065461 data: Should improve from 3 to 21 pairs" << endl;

    cout << "\nPHASE 2: SE(3) Pose Consistency (2-3 days)" << endl;
    cout << "   • Add pose graph optimization constraints" << endl;
    cout << "   • Penalize inconsistent global poses across connections" << endl;
    cout << "   • Expected: Further improvement in ground truth recall" << endl;

    cout << "\nPHASE 3: Full Physics (1 week)" << endl;
    cout << "   • Add breakline geometry matching" << endl;
    cout << "   • Add collision detection" << endl;
    cout << "   • Add curvature continuity" << endl;
    cout << "   • Target: 80%+ ground truth recall with high precision" << endl;

    cout << "\n🎯 SUCCESS CRITERIA:" << endl;
    cout << "   • Ground truth connections found: 12+/15 (vs current 2/15)" << endl;
    cout << "   • False positives: <5 (vs current 12)" << endl;
    cout << "   • Assembly visualization: Coherent vessel shape" << endl;
    cout << "   • Physics validation: No intersections, proper alignment" << endl;
}

/**
 * MAIN INTEGRATION FUNCTION
 * =========================
 */

int main() {
    cout << "🚀 PHYSICS-BASED POTTERY ASSEMBLY INTEGRATION" << endl;
    cout << "==============================================" << endl;

    demonstratePhysicsAdvantage();
    implementationNotes();
    testingStrategy();

    cout << "\n✅ NEXT STEP: Replace computeObjective() with computePhysicsObjective()" << endl;
    cout << "📝 Location: multi_hypothesis_optimizer.cpp lines 26-50" << endl;

    return 0;
}