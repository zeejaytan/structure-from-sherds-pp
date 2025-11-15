#ifndef PHYSICS_BASED_MINIMAL_TEST_H
#define PHYSICS_BASED_MINIMAL_TEST_H

#include "multi_hypothesis_optimizer.h"
#include <map>
#include <set>
#include <iostream>

/**
 * MINIMAL PHYSICS-BASED TEST
 * ==========================
 *
 * This is a minimal replacement for the broken computeObjective() function.
 *
 * KEY FIX: Instead of summing ALL active hypotheses (broken approach):
 *   spatial_score += hyp.switch_weight * hyp.local_score;  // WRONG!
 *
 * We implement discrete selection: ONE hypothesis per piece pair.
 * This alone should improve from 3 piece pairs to 21 piece pairs coverage.
 */

class PhysicsBasedMinimalTest {
public:
    struct TestResult {
        double objective_score;
        int piece_pairs_covered;
        int hypotheses_selected;
        std::vector<std::pair<int,int>> selected_pairs;
        std::string method_used;
    };

    // Minimal discrete selection: One hypothesis per piece pair
    static double computeDiscreteObjective(const GlobalProblem& problem);

    // Test function to compare approaches
    static TestResult compareApproaches(const GlobalProblem& problem);

    // Helper: Group hypotheses by piece pair
    static std::map<std::pair<int,int>, std::vector<int>> groupHypothesesByPair(const GlobalProblem& problem);

    // Helper: Select best hypothesis per pair
    static std::vector<int> selectBestPerPair(const GlobalProblem& problem);

    // Debug: Print selection results
    static void debugSelection(const GlobalProblem& problem, const std::vector<int>& selected_indices);
};

/**
 * INTEGRATION FUNCTION: Replace broken computeObjective()
 *
 * This function can be directly integrated into multi_hypothesis_optimizer.cpp
 * to replace the broken computeObjective() method.
 */
double computePhysicsObjectiveMinimal(const GlobalProblem& problem);

#endif // PHYSICS_BASED_MINIMAL_TEST_H