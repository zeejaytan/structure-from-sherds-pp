#ifndef TEST_GGCE_H
#define TEST_GGCE_H

#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include "../class/global_connectivity_engine.h"
#include "../class/enhanced_ranking_system.h"

/**
 * Comprehensive Unit Test Suite for Global Graph Connectivity Enhancement (GGCE)
 *
 * This test suite provides thorough validation of all GGCE components:
 * - Configuration system validation
 * - Spatial indexing accuracy
 * - Bridge detection algorithms
 * - Merge validation framework
 * - Inter-graph merge engine
 * - Enhanced ranking system integration
 * - Performance benchmarking
 * - Regression detection
 *
 * Design Principles:
 * - Self-contained tests (no external dependencies)
 * - Comprehensive coverage of edge cases
 * - Performance regression detection
 * - Clear pass/fail criteria
 * - Detailed failure diagnostics
 */

class GGCETestSuite {
public:
    struct TestResult {
        std::string test_name;
        bool passed;
        std::string error_message;
        double execution_time_ms;
        std::string details;

        TestResult(const std::string& name)
            : test_name(name), passed(false), execution_time_ms(0.0) {}

        void setSuccess(const std::string& detail = "") {
            passed = true;
            details = detail;
        }

        void setFailure(const std::string& error, const std::string& detail = "") {
            passed = false;
            error_message = error;
            details = detail;
        }

        std::string toString() const {
            std::ostringstream oss;
            oss << "[" << (passed ? "PASS" : "FAIL") << "] " << test_name;
            oss << " (" << execution_time_ms << "ms)";
            if (!passed && !error_message.empty()) {
                oss << " - ERROR: " << error_message;
            }
            if (!details.empty()) {
                oss << " - " << details;
            }
            return oss.str();
        }
    };

    struct TestSummary {
        int total_tests;
        int passed_tests;
        int failed_tests;
        double total_time_ms;
        std::vector<TestResult> results;

        TestSummary() : total_tests(0), passed_tests(0), failed_tests(0), total_time_ms(0.0) {}

        void addResult(const TestResult& result) {
            results.push_back(result);
            total_tests++;
            total_time_ms += result.execution_time_ms;

            if (result.passed) {
                passed_tests++;
            } else {
                failed_tests++;
            }
        }

        double getSuccessRate() const {
            return total_tests > 0 ? (100.0 * passed_tests / total_tests) : 0.0;
        }

        std::string toString() const {
            std::ostringstream oss;
            oss << "=== GGCE Test Summary ===\n";
            oss << "Tests: " << total_tests << " total, " << passed_tests << " passed, " << failed_tests << " failed\n";
            oss << "Success Rate: " << getSuccessRate() << "%\n";
            oss << "Total Time: " << total_time_ms << "ms\n";
            oss << "Average Time: " << (total_tests > 0 ? total_time_ms / total_tests : 0.0) << "ms per test\n\n";

            oss << "Individual Results:\n";
            for (const auto& result : results) {
                oss << result.toString() << "\n";
            }

            return oss.str();
        }
    };

    /**
     * Run all GGCE tests
     */
    static TestSummary runAllTests();

    /**
     * Configuration system tests
     */
    static TestResult testConfigurationSystem();
    static TestResult testConfigurationValidation();
    static TestResult testEnvironmentVariableLoading();

    /**
     * Spatial indexing tests
     */
    static TestResult testSpatialIndexBasicFunctionality();
    static TestResult testSpatialIndexPerformance();
    static TestResult testSpatialIndexAccuracy();

    /**
     * Bridge detection tests
     */
    static TestResult testBridgeDetectionBasic();
    static TestResult testBridgeDetectionWithNoConnections();
    static TestResult testBridgeDetectionWithMultipleComponents();
    static TestResult testBridgePrioritization();

    /**
     * Merge validation tests
     */
    static TestResult testMergeValidationGeometric();
    static TestResult testMergeValidationConsistency();
    static TestResult testMergeValidationIntersection();
    static TestResult testMergeValidationConnectivity();

    /**
     * Inter-graph merge engine tests
     */
    static TestResult testMergeEngineBasic();
    static TestResult testMergeEngineSequencing();
    static TestResult testMergeEngineErrorHandling();

    /**
     * Enhanced ranking system tests
     */
    static TestResult testEnhancedStateManagerIntegration();
    static TestResult testBackwardCompatibility();
    static TestResult testFeatureFlagControl();
    static TestResult testFallbackBehavior();

    /**
     * Performance and regression tests
     */
    static TestResult testPerformanceWithLargeDatasets();
    static TestResult testMemoryUsage();
    static TestResult testRegressionDetection();

    /**
     * Utility functions for test data generation
     */
    static std::vector<RankingSubgraph> createTestComponents(int num_components, int pieces_per_component);
    static std::vector<Geom> createTestGeometry(int num_pieces, int points_per_piece = 100);
    static std::vector<LCSIndex> createTestLCSReference(int num_pieces, double match_probability = 0.3);
    static GGCEConfiguration createTestConfiguration();

private:
    /**
     * Test execution helper
     */
    template<typename TestFunction>
    static TestResult executeTest(const std::string& test_name, TestFunction test_func);

    /**
     * Assertion helpers
     */
    static void assertEqual(int expected, int actual, const std::string& message);
    static void assertEqual(double expected, double actual, double tolerance, const std::string& message);
    static void assertTrue(bool condition, const std::string& message);
    static void assertFalse(bool condition, const std::string& message);
    static void assertNotNull(void* ptr, const std::string& message);

    /**
     * Mock data generators
     */
    static Vector3d generateRandomPoint(double range = 100.0);
    static Matrix4d generateRandomTransformation();
    static LCSIndex generateTestMatch(int piece1, int piece2, int inliers = 50);
};

/**
 * Integration Test Framework
 *
 * Tests GGCE integration with real-world scenarios
 */
class GGCEIntegrationTestSuite {
public:
    struct IntegrationTestResult {
        std::string test_name;
        bool passed;
        std::string error_message;
        int initial_components;
        int final_components;
        double connectivity_improvement;
        double processing_time_ms;

        IntegrationTestResult(const std::string& name)
            : test_name(name), passed(false), initial_components(0),
              final_components(0), connectivity_improvement(0.0), processing_time_ms(0.0) {}

        std::string toString() const {
            std::ostringstream oss;
            oss << "[" << (passed ? "PASS" : "FAIL") << "] " << test_name << "\n";
            oss << "  Components: " << initial_components << " -> " << final_components << "\n";
            oss << "  Improvement: " << connectivity_improvement << "\n";
            oss << "  Time: " << processing_time_ms << "ms\n";
            if (!passed && !error_message.empty()) {
                oss << "  Error: " << error_message << "\n";
            }
            return oss.str();
        }
    };

    /**
     * Run integration tests using synthetic data
     */
    static std::vector<IntegrationTestResult> runIntegrationTests();

    /**
     * Test scenarios
     */
    static IntegrationTestResult testTwoComponentMerge();
    static IntegrationTestResult testMultipleComponentMerge();
    static IntegrationTestResult testPartialMergeScenario();
    static IntegrationTestResult testNoMergeScenario();
    static IntegrationTestResult testHighFragmentationScenario();

    /**
     * Stress tests
     */
    static IntegrationTestResult testLargeAssemblyStress();
    static IntegrationTestResult testPerformanceUnderLoad();

private:
    /**
     * Create realistic test scenarios
     */
    static std::vector<RankingSubgraph> createTwoComponentScenario();
    static std::vector<RankingSubgraph> createMultiComponentScenario();
    static std::vector<RankingSubgraph> createHighFragmentationScenario();

    /**
     * Validate test results
     */
    static bool validateMergeResult(const std::vector<RankingSubgraph>& before,
                                   const std::vector<RankingSubgraph>& after);
};

/**
 * Performance Benchmark Suite
 */
class GGCEPerformanceBenchmark {
public:
    struct BenchmarkResult {
        std::string benchmark_name;
        int dataset_size;
        double average_time_ms;
        double min_time_ms;
        double max_time_ms;
        double std_deviation_ms;
        double throughput_assemblies_per_second;
        bool performance_acceptable;

        std::string toString() const {
            std::ostringstream oss;
            oss << "Benchmark: " << benchmark_name << "\n";
            oss << "  Dataset Size: " << dataset_size << " assemblies\n";
            oss << "  Average Time: " << average_time_ms << "ms\n";
            oss << "  Min/Max Time: " << min_time_ms << "/" << max_time_ms << "ms\n";
            oss << "  Std Deviation: " << std_deviation_ms << "ms\n";
            oss << "  Throughput: " << throughput_assemblies_per_second << " assemblies/sec\n";
            oss << "  Performance: " << (performance_acceptable ? "ACCEPTABLE" : "NEEDS IMPROVEMENT") << "\n";
            return oss.str();
        }
    };

    /**
     * Run comprehensive performance benchmarks
     */
    static std::vector<BenchmarkResult> runBenchmarks();

    /**
     * Specific benchmarks
     */
    static BenchmarkResult benchmarkBasicConnectivity();
    static BenchmarkResult benchmarkComplexAssemblies();
    static BenchmarkResult benchmarkMemoryUsage();
    static BenchmarkResult benchmarkScalability();

private:
    /**
     * Performance measurement utilities
     */
    static double measureExecutionTime(std::function<void()> func);
    static std::vector<double> runMultipleTimes(std::function<void()> func, int iterations);
    static double calculateStandardDeviation(const std::vector<double>& times);

    /**
     * Performance thresholds
     */
    static constexpr double MAX_ACCEPTABLE_TIME_MS = 1000.0; // 1 second max
    static constexpr double MAX_ACCEPTABLE_MEMORY_MB = 100.0; // 100MB max
    static constexpr int MIN_THROUGHPUT_PER_SECOND = 1; // At least 1 assembly per second
};

/**
 * Test Data Manager
 *
 * Manages test datasets and provides consistent test data for all tests
 */
class GGCETestDataManager {
public:
    /**
     * Get standard test datasets
     */
    static std::vector<RankingSubgraph> getSimpleThreeComponentDataset();
    static std::vector<RankingSubgraph> getComplexMultiComponentDataset();
    static std::vector<RankingSubgraph> getHighFragmentationDataset();
    static std::vector<RankingSubgraph> getLargeScaleDataset();

    /**
     * Get corresponding geometry and LCS data
     */
    static std::vector<Geom> getTestGeometry(const std::string& dataset_name);
    static std::vector<LCSIndex> getTestLCSReference(const std::string& dataset_name);

    /**
     * Validate test data consistency
     */
    static bool validateTestData(const std::vector<RankingSubgraph>& components,
                                const std::vector<Geom>& geometry,
                                const std::vector<LCSIndex>& lcs_reference);

private:
    /**
     * Internal test data generation
     */
    static std::vector<RankingSubgraph> generateComponents(int num_components,
                                                          const std::vector<int>& pieces_per_component);
    static std::vector<Geom> generateGeometry(int num_pieces, const std::vector<Vector3d>& centroids);
    static std::vector<LCSIndex> generateLCSMatches(const std::vector<RankingSubgraph>& components,
                                                   double inter_component_match_probability = 0.1);
};

#endif // TEST_GGCE_H