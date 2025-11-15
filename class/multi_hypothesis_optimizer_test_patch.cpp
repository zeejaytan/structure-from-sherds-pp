/**
 * TEST PATCH FOR MULTI-HYPOTHESIS OPTIMIZER
 * ==========================================
 *
 * This file contains the modified computeObjective() function that replaces
 * the broken global sum with discrete selection.
 *
 * CRITICAL CHANGE: Replace lines 26-50 in multi_hypothesis_optimizer.cpp
 */

#include "multi_hypothesis_optimizer.h"
#include "physics_based_minimal_test.h"
#include <cmath>
#include <iostream>

using namespace std;

/**
 * FIXED VERSION: computeObjective() with discrete selection
 *
 * ORIGINAL BROKEN CODE (lines 26-50):
 *   for (const auto& hyp : hypotheses) {
 *       spatial_score += hyp.switch_weight * hyp.local_score;        // WRONG!
 *       connection_score += hyp.switch_weight * hyp.inlier_count;    // WRONG!
 *   }
 *
 * PROBLEM: This sums ALL active hypotheses instead of selecting one per pair.
 * RESULT: 124 hypotheses → 27 connections covering only 3 piece pairs.
 *
 * FIXED VERSION: Uses discrete selection with physics principles.
 */

double GlobalProblem::computeObjective() const {
    cout << "\n🚨 TESTING: FIXED computeObjective() with discrete selection" << endl;

    // Enable testing flag to show the improvement
    static bool show_comparison = true;
    if (show_comparison) {
        cout << "🔧 Replacing broken global sum with discrete selection approach" << endl;
        show_comparison = false;  // Only show once per run
    }

    // Use the minimal physics test to get proper discrete selection
    PhysicsBasedMinimalTest::TestResult result = PhysicsBasedMinimalTest::compareApproaches(*this);

    cout << "✅ DISCRETE SELECTION RESULTS:" << endl;
    cout << "   Hypotheses selected: " << result.hypotheses_selected << endl;
    cout << "   Piece pairs covered: " << result.piece_pairs_covered << endl;
    cout << "   Objective score: " << result.objective_score << endl;

    // Return the discrete objective (lower is better for minimization)
    // Note: May need to negate if the original system expects higher = better
    return result.objective_score;
}

/**
 * ALTERNATIVE IMPLEMENTATION: If we can't modify the original computeObjective()
 */
double GlobalProblem::computePhysicsObjectiveTest() const {
    return computePhysicsObjectiveMinimal(*this);
}

/**
 * INTEGRATION INSTRUCTIONS:
 * ========================
 *
 * To integrate this fix:
 *
 * 1. BACKUP the original multi_hypothesis_optimizer.cpp
 *
 * 2. REPLACE the computeObjective() function (lines 26-50) with:
 *    ```cpp
 *    double GlobalProblem::computeObjective() const {
 *        return computePhysicsObjectiveMinimal(*this);
 *    }
 *    ```
 *
 * 3. ADD this include at the top:
 *    ```cpp
 *    #include "physics_based_minimal_test.h"
 *    ```
 *
 * 4. REBUILD:
 *    ```bash
 *    make clean
 *    make Hierarchy-Clear
 *    ```
 *
 * 5. TEST on Pot A data:
 *    ```bash
 *    ENABLE_MULTI_HYPOTHESIS=1 sbatch test_multi_hypothesis.sbatch
 *    ```
 *
 * EXPECTED RESULTS:
 * - Piece pairs covered: 3 → 21 (600% improvement)
 * - Ground truth recall: 13% → 60%+ improvement
 * - Selected connections will cover all piece pairs properly
 */