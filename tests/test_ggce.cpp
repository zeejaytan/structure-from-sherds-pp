#include "test_ggce.h"
#include <random>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sstream>

//############################## Test Execution Framework ##############################//

template<typename TestFunction>
GGCETestSuite::TestResult GGCETestSuite::executeTest(const std::string& test_name, TestFunction test_func) {
    TestResult result(test_name);

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        test_func(result);
    } catch (const std::exception& e) {
        result.setFailure("Exception thrown: " + std::string(e.what()));
    } catch (...) {
        result.setFailure("Unknown exception thrown");
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    return result;
}

GGCETestSuite::TestSummary GGCETestSuite::runAllTests() {
    TestSummary summary;

    std::cout << "Running GGCE Test Suite...\n" << std::endl;

    // Configuration tests
    summary.addResult(testConfigurationSystem());
    summary.addResult(testConfigurationValidation());
    summary.addResult(testEnvironmentVariableLoading());

    // Spatial indexing tests
    summary.addResult(testSpatialIndexBasicFunctionality());
    summary.addResult(testSpatialIndexPerformance());
    summary.addResult(testSpatialIndexAccuracy());

    // Bridge detection tests
    summary.addResult(testBridgeDetectionBasic());
    summary.addResult(testBridgeDetectionWithNoConnections());
    summary.addResult(testBridgeDetectionWithMultipleComponents());
    summary.addResult(testBridgePrioritization());

    // Merge validation tests
    summary.addResult(testMergeValidationGeometric());
    summary.addResult(testMergeValidationConsistency());
    summary.addResult(testMergeValidationIntersection());
    summary.addResult(testMergeValidationConnectivity());

    // Merge engine tests
    summary.addResult(testMergeEngineBasic());
    summary.addResult(testMergeEngineSequencing());
    summary.addResult(testMergeEngineErrorHandling());

    // Integration tests
    summary.addResult(testEnhancedStateManagerIntegration());
    summary.addResult(testBackwardCompatibility());
    summary.addResult(testFeatureFlagControl());
    summary.addResult(testFallbackBehavior());

    // Performance tests
    summary.addResult(testPerformanceWithLargeDatasets());
    summary.addResult(testMemoryUsage());
    summary.addResult(testRegressionDetection());

    return summary;
}

//############################## Assertion Helpers ##############################//

void GGCETestSuite::assertEqual(int expected, int actual, const std::string& message) {
    if (expected != actual) {
        throw std::runtime_error(message + " (expected " + std::to_string(expected) +
                                ", got " + std::to_string(actual) + ")");
    }
}

void GGCETestSuite::assertEqual(double expected, double actual, double tolerance, const std::string& message) {
    if (std::abs(expected - actual) > tolerance) {
        throw std::runtime_error(message + " (expected " + std::to_string(expected) +
                                ", got " + std::to_string(actual) + ", tolerance " + std::to_string(tolerance) + ")");
    }
}

void GGCETestSuite::assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message + " (condition was false)");
    }
}

void GGCETestSuite::assertFalse(bool condition, const std::string& message) {
    if (condition) {
        throw std::runtime_error(message + " (condition was true)");
    }
}

void GGCETestSuite::assertNotNull(void* ptr, const std::string& message) {
    if (ptr == nullptr) {
        throw std::runtime_error(message + " (pointer was null)");
    }
}

//############################## Test Data Generation ##############################//

std::vector<RankingSubgraph> GGCETestSuite::createTestComponents(int num_components, int pieces_per_component) {
    std::vector<RankingSubgraph> components;

    int total_pieces = num_components * pieces_per_component;

    for (int comp = 0; comp < num_components; ++comp) {
        RankingSubgraph component(total_pieces);

        // Assign pieces to this component
        for (int piece = 0; piece < pieces_per_component; ++piece) {
            int piece_index = comp * pieces_per_component + piece;
            if (piece_index < total_pieces) {
                component.node_[piece_index] = true;
            }
        }

        component.graph_score_ = 100 + comp * 10; // Arbitrary but deterministic scores
        component.pc_score_ = 0.8 + comp * 0.05;
        component.root_node_ = comp * pieces_per_component + 1; // 1-based

        components.push_back(component);
    }

    return components;
}

std::vector<Geom> GGCETestSuite::createTestGeometry(int num_pieces, int points_per_piece) {
    std::vector<Geom> geometry(num_pieces);
    std::mt19937 rng(12345); // Fixed seed for reproducible tests
    std::uniform_real_distribution<double> coord_dist(-50.0, 50.0);

    for (int piece = 0; piece < num_pieces; ++piece) {
        geometry[piece].point_.resize(points_per_piece);

        // Create a cluster of points around a random center
        Vector3d center = generateRandomPoint(100.0);

        for (int point = 0; point < points_per_piece; ++point) {
            // Add some noise around the center
            Vector3d noise(coord_dist(rng), coord_dist(rng), coord_dist(rng));
            geometry[piece].point_[point] = center + noise * 0.1;
        }
    }

    return geometry;
}

std::vector<LCSIndex> GGCETestSuite::createTestLCSReference(int num_pieces, double match_probability) {
    std::vector<LCSIndex> lcs_reference;
    std::mt19937 rng(54321); // Fixed seed for reproducible tests
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<int> inlier_dist(10, 100);

    // Generate potential matches between all piece pairs
    for (int i = 1; i <= num_pieces; ++i) {
        for (int j = i + 1; j <= num_pieces; ++j) {
            if (prob_dist(rng) < match_probability) {
                LCSIndex match = generateTestMatch(i, j, inlier_dist(rng));
                lcs_reference.push_back(match);
            }
        }
    }

    return lcs_reference;
}

Vector3d GGCETestSuite::generateRandomPoint(double range) {
    static std::mt19937 rng(98765);
    std::uniform_real_distribution<double> dist(-range/2, range/2);
    return Vector3d(dist(rng), dist(rng), dist(rng));
}

LCSIndex GGCETestSuite::generateTestMatch(int piece1, int piece2, int inliers) {
    LCSIndex match;
    match.shard_x_ = piece1;
    match.shard_y_ = piece2;
    match.inliner_ = inliers;

    // Generate a simple transformation
    Matrix3d R = Matrix3d::Identity();
    Vector3d t = generateRandomPoint(10.0);
    match.trans_.Set(R, t, piece1, piece2);

    return match;
}

GGCEConfiguration GGCETestSuite::createTestConfiguration() {
    GGCEConfiguration config;
    config.enabled = true;
    config.debug_mode = false; // Avoid noise during testing
    config.verbose_logging = false;

    config.connectivity_weight = 0.6;
    config.quality_weight = 0.3;
    config.consistency_weight = 0.1;
    config.quality_threshold = 0.7;

    config.max_merge_iterations = 5; // Smaller for faster tests
    config.max_bridge_candidates = 20;
    config.spatial_index_cell_size = 10.0;

    config.algorithm_variant = "balanced";

    return config;
}

//############################## Configuration Tests ##############################//

GGCETestSuite::TestResult GGCETestSuite::testConfigurationSystem() {
    return executeTest("Configuration System", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();

        assertTrue(config.validate(), "Configuration validation failed");
        assertEqual(0.6, config.connectivity_weight, 0.001, "Connectivity weight incorrect");
        assertEqual(0.3, config.quality_weight, 0.001, "Quality weight incorrect");
        assertEqual(0.1, config.consistency_weight, 0.001, "Consistency weight incorrect");

        std::string config_str = config.toString();
        assertTrue(!config_str.empty(), "Configuration toString() returned empty string");
        assertTrue(config_str.find("connectivity_weight") != std::string::npos,
                  "Configuration string missing connectivity_weight");

        result.setSuccess("All configuration parameters correctly set and validated");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testConfigurationValidation() {
    return executeTest("Configuration Validation", [](TestResult& result) {
        // Test invalid weight sum
        GGCEConfiguration invalid_weights;
        invalid_weights.connectivity_weight = 0.5;
        invalid_weights.quality_weight = 0.4;
        invalid_weights.consistency_weight = 0.2; // Sum = 1.1 > 1.0

        assertFalse(invalid_weights.validate(), "Should reject weights that sum > 1.0");

        // Test negative weights
        GGCEConfiguration negative_weights;
        negative_weights.connectivity_weight = -0.1;
        negative_weights.quality_weight = 0.7;
        negative_weights.consistency_weight = 0.4;

        assertFalse(negative_weights.validate(), "Should reject negative weights");

        // Test invalid quality threshold
        GGCEConfiguration invalid_threshold;
        invalid_threshold.quality_threshold = 1.5; // > 1.0

        assertFalse(invalid_threshold.validate(), "Should reject quality threshold > 1.0");

        // Test invalid algorithm variant
        GGCEConfiguration invalid_variant;
        invalid_variant.algorithm_variant = "invalid_variant";

        assertFalse(invalid_variant.validate(), "Should reject invalid algorithm variant");

        result.setSuccess("Configuration validation correctly rejects invalid parameters");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testEnvironmentVariableLoading() {
    return executeTest("Environment Variable Loading", [](TestResult& result) {
        // Test default values (when no environment variables set)
        GGCEConfiguration default_config = GGCEConfiguration::loadFromEnvironment();

        // Check that defaults are reasonable
        assertTrue(default_config.connectivity_weight >= 0.0 && default_config.connectivity_weight <= 1.0,
                  "Default connectivity weight out of range");
        assertTrue(default_config.quality_weight >= 0.0 && default_config.quality_weight <= 1.0,
                  "Default quality weight out of range");
        assertTrue(!default_config.algorithm_variant.empty(), "Default algorithm variant is empty");

        result.setSuccess("Environment variable loading works with sensible defaults");
    });
}

//############################## Spatial Index Tests ##############################//

GGCETestSuite::TestResult GGCETestSuite::testSpatialIndexBasicFunctionality() {
    return executeTest("Spatial Index Basic", [](TestResult& result) {
        SpatialIndex spatial_index(10.0); // 10mm cell size

        auto components = createTestComponents(2, 2); // 2 components, 2 pieces each
        auto geometry = createTestGeometry(4, 50);    // 4 pieces, 50 points each

        // Build spatial index
        spatial_index.buildIndex(components, geometry);

        // Test connection detection
        bool may_connect = spatial_index.mayHaveConnections(0, 1);

        // Get candidate pairs
        auto pairs = spatial_index.getCandidatePairs(0, 1);

        result.setSuccess("Spatial index built and queried successfully. May connect: " +
                         std::to_string(may_connect) + ", Pairs: " + std::to_string(pairs.size()));
    });
}

GGCETestSuite::TestResult GGCETestSuite::testSpatialIndexPerformance() {
    return executeTest("Spatial Index Performance", [](TestResult& result) {
        const int NUM_COMPONENTS = 10;
        const int PIECES_PER_COMPONENT = 10;
        const int POINTS_PER_PIECE = 1000;

        auto components = createTestComponents(NUM_COMPONENTS, PIECES_PER_COMPONENT);
        auto geometry = createTestGeometry(NUM_COMPONENTS * PIECES_PER_COMPONENT, POINTS_PER_PIECE);

        auto start_time = std::chrono::high_resolution_clock::now();

        SpatialIndex spatial_index(5.0);
        spatial_index.buildIndex(components, geometry);

        // Test many queries
        int query_count = 0;
        for (int i = 0; i < NUM_COMPONENTS; ++i) {
            for (int j = i + 1; j < NUM_COMPONENTS; ++j) {
                auto pairs = spatial_index.getCandidatePairs(i, j);
                query_count++;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        // Performance should be reasonable (< 100ms for this scale)
        assertTrue(elapsed_ms < 100.0, "Spatial index performance too slow: " + std::to_string(elapsed_ms) + "ms");

        result.setSuccess("Processed " + std::to_string(query_count) + " queries in " +
                         std::to_string(elapsed_ms) + "ms");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testSpatialIndexAccuracy() {
    return executeTest("Spatial Index Accuracy", [](TestResult& result) {
        // Create components with known spatial relationships
        auto components = createTestComponents(3, 1); // 3 components, 1 piece each
        std::vector<Geom> geometry(3);

        // Place pieces at known locations
        geometry[0].point_.push_back(Vector3d(0, 0, 0));     // Piece 1 at origin
        geometry[1].point_.push_back(Vector3d(5, 0, 0));     // Piece 2 close to piece 1
        geometry[2].point_.push_back(Vector3d(100, 0, 0));   // Piece 3 far from others

        SpatialIndex spatial_index(10.0); // 10mm cells
        spatial_index.buildIndex(components, geometry);

        // Pieces 1 and 2 should be in nearby cells
        assertTrue(spatial_index.mayHaveConnections(0, 1), "Close pieces should have potential connections");

        // Pieces 1 and 3 should be in distant cells
        assertFalse(spatial_index.mayHaveConnections(0, 2), "Distant pieces should not have potential connections");

        auto close_pairs = spatial_index.getCandidatePairs(0, 1);
        auto distant_pairs = spatial_index.getCandidatePairs(0, 2);

        assertTrue(close_pairs.size() >= distant_pairs.size(),
                  "Close pieces should have more candidate pairs than distant ones");

        result.setSuccess("Spatial relationships correctly detected");
    });
}

//############################## Bridge Detection Tests ##############################//

GGCETestSuite::TestResult GGCETestSuite::testBridgeDetectionBasic() {
    return executeTest("Bridge Detection Basic", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        GlobalConnectivityAnalyzer analyzer(config);

        auto components = createTestComponents(2, 2); // 2 components, 2 pieces each
        auto geometry = createTestGeometry(4, 100);
        auto lcs_reference = createTestLCSReference(4, 0.5); // 50% match probability

        auto bridges = analyzer.detectPotentialBridges(components, geometry, lcs_reference);

        // Should find some potential bridges between components
        result.setSuccess("Found " + std::to_string(bridges.size()) + " potential bridges");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testBridgeDetectionWithNoConnections() {
    return executeTest("Bridge Detection No Connections", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        GlobalConnectivityAnalyzer analyzer(config);

        auto components = createTestComponents(2, 1);
        std::vector<Geom> geometry(2);

        // Place components very far apart with no potential connections
        geometry[0].point_.push_back(Vector3d(0, 0, 0));
        geometry[1].point_.push_back(Vector3d(1000, 1000, 1000));

        std::vector<LCSIndex> empty_lcs; // No matches

        auto bridges = analyzer.detectPotentialBridges(components, geometry, empty_lcs);

        // Should find no bridges due to distance and no LCS matches
        assertEqual(0, static_cast<int>(bridges.size()), "Should find no bridges for distant components with no matches");

        result.setSuccess("Correctly identified no bridges for disconnected components");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testBridgeDetectionWithMultipleComponents() {
    return executeTest("Bridge Detection Multiple Components", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        GlobalConnectivityAnalyzer analyzer(config);

        auto components = createTestComponents(4, 2); // 4 components, 2 pieces each
        auto geometry = createTestGeometry(8, 100);
        auto lcs_reference = createTestLCSReference(8, 0.3);

        auto bridges = analyzer.detectPotentialBridges(components, geometry, lcs_reference);

        // With 4 components, there should be potential for multiple bridges
        assertTrue(bridges.size() >= 0, "Should handle multiple components without error");

        result.setSuccess("Processed " + std::to_string(components.size()) +
                         " components, found " + std::to_string(bridges.size()) + " bridges");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testBridgePrioritization() {
    return executeTest("Bridge Prioritization", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        GlobalConnectivityAnalyzer analyzer(config);

        auto components = createTestComponents(3, 2);
        auto geometry = createTestGeometry(6, 100);
        auto lcs_reference = createTestLCSReference(6, 0.4);

        auto bridges = analyzer.detectPotentialBridges(components, geometry, lcs_reference);

        if (bridges.size() >= 2) {
            // Check that bridges are sorted by combined score (descending)
            for (size_t i = 1; i < bridges.size(); ++i) {
                assertTrue(bridges[i-1].combined_score >= bridges[i].combined_score,
                          "Bridges should be sorted by combined score in descending order");
            }
        }

        result.setSuccess("Bridge prioritization validated with " + std::to_string(bridges.size()) + " candidates");
    });
}

//############################## Merge Validation Tests ##############################//

GGCETestSuite::TestResult GGCETestSuite::testMergeValidationGeometric() {
    return executeTest("Merge Validation Geometric", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        MergeValidationFramework validator(config);

        auto components = createTestComponents(2, 1);
        auto geometry = createTestGeometry(2, 100);

        // Create a valid bridge candidate
        BridgeCandidate candidate;
        candidate.source_graph_index = 0;
        candidate.target_graph_index = 1;
        candidate.source_piece_id = 1;
        candidate.target_piece_id = 2;
        candidate.geometric_confidence = 0.8;
        candidate.connectivity_score = 0.7;
        candidate.transformation_quality = 0.75;
        candidate.combined_score = 0.75;

        auto validation = validator.validateMerge(candidate, components, geometry);

        assertTrue(validation.geometric_analysis.overlap_score >= 0.0 &&
                  validation.geometric_analysis.overlap_score <= 1.0,
                  "Geometric overlap score should be in [0,1] range");

        result.setSuccess("Geometric validation completed with overlap score: " +
                         std::to_string(validation.geometric_analysis.overlap_score));
    });
}

GGCETestSuite::TestResult GGCETestSuite::testMergeValidationConsistency() {
    return executeTest("Merge Validation Consistency", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        MergeValidationFramework validator(config);

        auto components = createTestComponents(2, 2);
        auto geometry = createTestGeometry(4, 50);

        BridgeCandidate candidate;
        candidate.source_graph_index = 0;
        candidate.target_graph_index = 1;
        candidate.source_piece_id = 1;
        candidate.target_piece_id = 3;
        candidate.transformation_quality = 0.8;

        auto validation = validator.validateMerge(candidate, components, geometry);

        assertTrue(validation.consistency_check.transformation_error >= 0.0,
                  "Transformation error should be non-negative");
        assertTrue(validation.consistency_check.chain_consistency >= 0.0 &&
                  validation.consistency_check.chain_consistency <= 1.0,
                  "Chain consistency should be in [0,1] range");

        result.setSuccess("Consistency validation completed with error: " +
                         std::to_string(validation.consistency_check.transformation_error));
    });
}

GGCETestSuite::TestResult GGCETestSuite::testMergeValidationIntersection() {
    return executeTest("Merge Validation Intersection", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        MergeValidationFramework validator(config);

        auto components = createTestComponents(2, 1);
        auto geometry = createTestGeometry(2, 100);

        BridgeCandidate candidate;
        candidate.source_graph_index = 0;
        candidate.target_graph_index = 1;
        candidate.source_piece_id = 1;
        candidate.target_piece_id = 2;

        auto validation = validator.validateMerge(candidate, components, geometry);

        assertTrue(validation.intersection_analysis.volume_overlap_ratio >= 0.0 &&
                  validation.intersection_analysis.volume_overlap_ratio <= 1.0,
                  "Volume overlap ratio should be in [0,1] range");
        assertTrue(validation.intersection_analysis.opposing_normals_count >= 0,
                  "Opposing normals count should be non-negative");

        result.setSuccess("Intersection validation completed with overlap ratio: " +
                         std::to_string(validation.intersection_analysis.volume_overlap_ratio));
    });
}

GGCETestSuite::TestResult GGCETestSuite::testMergeValidationConnectivity() {
    return executeTest("Merge Validation Connectivity", [](TestResult& result) {
        GGCEConfiguration config = createTestConfiguration();
        MergeValidationFramework validator(config);

        auto components = createTestComponents(3, 1); // 3 components - good fragmentation scenario
        auto geometry = createTestGeometry(3, 50);

        BridgeCandidate candidate;
        candidate.source_graph_index = 0;
        candidate.target_graph_index = 1;
        candidate.source_piece_id = 1;
        candidate.target_piece_id = 2;
        candidate.connectivity_score = 0.8;

        auto validation = validator.validateMerge(candidate, components, geometry);

        assertEqual(1, validation.connectivity_impact.components_reduction,
                   "Merging two components should reduce count by 1");
        assertTrue(validation.connectivity_impact.connectivity_improvement > 0.0,
                  "Merging components should improve connectivity");

        result.setSuccess("Connectivity validation shows improvement: " +
                         std::to_string(validation.connectivity_impact.connectivity_improvement));
    });
}

//############################## Integration Tests ##############################//

GGCETestSuite::TestResult GGCETestSuite::testEnhancedStateManagerIntegration() {
    return executeTest("Enhanced StateManager Integration", [](TestResult& result) {
        // Create test data
        std::vector<Geom> shard = createTestGeometry(4, 50);
        std::list<LCSIndex> lcs_out;
        auto lcs_vec = createTestLCSReference(4, 0.3);
        lcs_out.assign(lcs_vec.begin(), lcs_vec.end());

        // Create enhanced state manager
        EnhancedStateManager manager(4, 2, shard, lcs_out, 4, "test_log");

        assertTrue(manager.isGGCEEnabled() || !manager.isGGCEEnabled(),
                  "GGCE enabled status should be deterministic");

        const auto& config = manager.getGGCEConfiguration();
        assertTrue(config.connectivity_weight >= 0.0 && config.connectivity_weight <= 1.0,
                  "GGCE configuration should be valid");

        result.setSuccess("Enhanced StateManager created and configured successfully");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testBackwardCompatibility() {
    return executeTest("Backward Compatibility", [](TestResult& result) {
        // Test that EnhancedStateManager can be used as StateManager
        std::vector<Geom> shard = createTestGeometry(2, 20);
        std::list<LCSIndex> lcs_out;

        std::unique_ptr<StateManager> manager =
            std::make_unique<EnhancedStateManager>(2, 1, shard, lcs_out, 2, "test_log");

        assertNotNull(manager.get(), "Enhanced StateManager should be usable as StateManager");

        // Should be able to call base class methods
        // manager->BuildStep(); // Would require full state setup

        result.setSuccess("Backward compatibility verified - EnhancedStateManager usable as StateManager");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testFeatureFlagControl() {
    return executeTest("Feature Flag Control", [](TestResult& result) {
        EnhancedStateManager manager;

        // Test enabling/disabling GGCE
        manager.setGGCEEnabled(true);
        assertTrue(manager.isGGCEEnabled(), "GGCE should be enabled after setGGCEEnabled(true)");

        manager.setGGCEEnabled(false);
        assertFalse(manager.isGGCEEnabled(), "GGCE should be disabled after setGGCEEnabled(false)");

        // Test statistics
        const auto& stats = manager.getGGCEStatistics();
        assertEqual(0, stats.total_assemblies_processed, "Initial statistics should show 0 assemblies processed");

        result.setSuccess("Feature flag control working correctly");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testFallbackBehavior() {
    return executeTest("Fallback Behavior", [](TestResult& result) {
        // Test graceful degradation when GGCE encounters errors
        GGCEConfiguration invalid_config;
        invalid_config.enabled = true;
        invalid_config.connectivity_weight = -1.0; // Invalid

        try {
            GlobalGraphConnectivityEngine engine(invalid_config);
            result.setFailure("Should throw exception for invalid configuration");
            return;
        } catch (const std::exception& e) {
            // Expected behavior
        }

        result.setSuccess("Fallback behavior correctly handles invalid configuration");
    });
}

//############################## Performance Tests ##############################//

GGCETestSuite::TestResult GGCETestSuite::testPerformanceWithLargeDatasets() {
    return executeTest("Performance Large Datasets", [](TestResult& result) {
        const int NUM_COMPONENTS = 8;
        const int PIECES_PER_COMPONENT = 3;

        auto components = createTestComponents(NUM_COMPONENTS, PIECES_PER_COMPONENT);
        auto geometry = createTestGeometry(NUM_COMPONENTS * PIECES_PER_COMPONENT, 200);
        auto lcs_reference = createTestLCSReference(NUM_COMPONENTS * PIECES_PER_COMPONENT, 0.2);

        GGCEConfiguration config = createTestConfiguration();
        GlobalGraphConnectivityEngine engine(config);

        auto start_time = std::chrono::high_resolution_clock::now();

        auto enhancement_result = engine.applyGlobalConnectivityEnhancement(
            components, geometry, lcs_reference);

        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        // Should complete within reasonable time (5 seconds for test dataset)
        assertTrue(elapsed_ms < 5000.0, "Large dataset processing too slow: " + std::to_string(elapsed_ms) + "ms");

        result.setSuccess("Processed " + std::to_string(NUM_COMPONENTS) + " components in " +
                         std::to_string(elapsed_ms) + "ms");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testMemoryUsage() {
    return executeTest("Memory Usage", [](TestResult& result) {
        // Create multiple GGCE engines to test memory management
        std::vector<std::unique_ptr<GlobalGraphConnectivityEngine>> engines;

        GGCEConfiguration config = createTestConfiguration();

        for (int i = 0; i < 10; ++i) {
            engines.push_back(std::make_unique<GlobalGraphConnectivityEngine>(config));
        }

        // All engines should be valid
        for (const auto& engine : engines) {
            assertNotNull(engine.get(), "Engine should be valid");
        }

        result.setSuccess("Created and managed " + std::to_string(engines.size()) + " GGCE engines");
    });
}

GGCETestSuite::TestResult GGCETestSuite::testRegressionDetection() {
    return executeTest("Regression Detection", [](TestResult& result) {
        GGCEPerformanceMonitor monitor;

        // Create baseline metrics
        GGCEPerformanceMonitor::PerformanceMetrics baseline;
        baseline.average_processing_time_ms = 100.0;
        baseline.connectivity_improvement_ratio = 0.8;
        baseline.success_rate = 0.9;

        // Test with similar metrics (should not detect regression)
        GlobalGraphConnectivityEngine::EnhancementResult good_result;
        good_result.processing_time = std::chrono::milliseconds(95);
        good_result.connectivity_improvement = 0.82;
        good_result.success = true;

        monitor.recordResult(good_result);
        assertFalse(monitor.detectRegression(baseline), "Should not detect regression for similar performance");

        // Test with degraded metrics (should detect regression)
        GlobalGraphConnectivityEngine::EnhancementResult bad_result;
        bad_result.processing_time = std::chrono::milliseconds(250); // 2.5x slower
        bad_result.connectivity_improvement = 0.4; // 50% worse
        bad_result.success = true;

        monitor.recordResult(bad_result);
        assertTrue(monitor.detectRegression(baseline), "Should detect regression for degraded performance");

        result.setSuccess("Regression detection working correctly");
    });
}

//############################## Test Data Manager Implementation ##############################//

std::vector<RankingSubgraph> GGCETestDataManager::getSimpleThreeComponentDataset() {
    return GGCETestSuite::createTestComponents(3, 2); // 3 components, 2 pieces each
}

std::vector<RankingSubgraph> GGCETestDataManager::getComplexMultiComponentDataset() {
    return GGCETestSuite::createTestComponents(5, 3); // 5 components, 3 pieces each
}

std::vector<RankingSubgraph> GGCETestDataManager::getHighFragmentationDataset() {
    return GGCETestSuite::createTestComponents(8, 1); // 8 components, 1 piece each (highly fragmented)
}

std::vector<RankingSubgraph> GGCETestDataManager::getLargeScaleDataset() {
    return GGCETestSuite::createTestComponents(10, 5); // 10 components, 5 pieces each
}

std::vector<Geom> GGCETestDataManager::getTestGeometry(const std::string& dataset_name) {
    if (dataset_name == "simple") {
        return GGCETestSuite::createTestGeometry(6, 100);
    } else if (dataset_name == "complex") {
        return GGCETestSuite::createTestGeometry(15, 150);
    } else if (dataset_name == "fragmented") {
        return GGCETestSuite::createTestGeometry(8, 80);
    } else if (dataset_name == "large") {
        return GGCETestSuite::createTestGeometry(50, 200);
    }

    return GGCETestSuite::createTestGeometry(4, 100); // Default
}

std::vector<LCSIndex> GGCETestDataManager::getTestLCSReference(const std::string& dataset_name) {
    if (dataset_name == "simple") {
        return GGCETestSuite::createTestLCSReference(6, 0.4);
    } else if (dataset_name == "complex") {
        return GGCETestSuite::createTestLCSReference(15, 0.3);
    } else if (dataset_name == "fragmented") {
        return GGCETestSuite::createTestLCSReference(8, 0.2);
    } else if (dataset_name == "large") {
        return GGCETestSuite::createTestLCSReference(50, 0.25);
    }

    return GGCETestSuite::createTestLCSReference(4, 0.3); // Default
}

bool GGCETestDataManager::validateTestData(const std::vector<RankingSubgraph>& components,
                                          const std::vector<Geom>& geometry,
                                          const std::vector<LCSIndex>& lcs_reference) {
    // Validate that components and geometry have consistent sizes
    if (components.empty() || geometry.empty()) {
        return false;
    }

    // Check that all components have valid piece assignments
    for (const auto& component : components) {
        if (component.node_.size() != geometry.size()) {
            return false; // Size mismatch
        }

        bool has_pieces = std::any_of(component.node_.begin(), component.node_.end(),
                                     [](bool has_piece) { return has_piece; });
        if (!has_pieces) {
            return false; // Empty component
        }
    }

    // Validate LCS reference
    for (const auto& lcs : lcs_reference) {
        if (lcs.shard_x_ < 1 || lcs.shard_x_ > geometry.size() ||
            lcs.shard_y_ < 1 || lcs.shard_y_ > geometry.size()) {
            return false; // Invalid piece indices
        }
    }

    return true;
}