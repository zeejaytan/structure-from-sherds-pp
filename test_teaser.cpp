#include <teaser/registration.h>
#include <Eigen/Dense>
#include <iostream>
#include <vector>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TEASER++ Integration Verification Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Configure TEASER++ parameters
    teaser::RobustRegistrationSolver::Params params;
    params.noise_bound = 0.05;  // 50mm noise tolerance
    params.cbar2 = 1.0;
    params.estimate_scaling = false;  // Rigid registration only (no scaling)
    params.rotation_estimation_algorithm =
        teaser::RobustRegistrationSolver::ROTATION_ESTIMATION_ALGORITHM::GNC_TLS;
    params.rotation_gnc_factor = 1.4;
    params.rotation_max_iterations = 100;
    params.rotation_cost_threshold = 1e-12;

    std::cout << "✓ TEASER++ parameters configured:" << std::endl;
    std::cout << "  Noise bound: " << params.noise_bound << " m" << std::endl;
    std::cout << "  Rotation algorithm: GNC-TLS (Graduated Non-Convexity)" << std::endl;
    std::cout << "  Scaling estimation: " << (params.estimate_scaling ? "enabled" : "disabled") << std::endl;
    std::cout << std::endl;

    // Create solver
    teaser::RobustRegistrationSolver solver(params);
    std::cout << "✓ TEASER++ solver initialized successfully" << std::endl;
    std::cout << std::endl;

    // Create simple test point clouds (3D rotation + translation)
    Eigen::Matrix<double, 3, Eigen::Dynamic> src(3, 5);
    Eigen::Matrix<double, 3, Eigen::Dynamic> dst(3, 5);

    // Source points (simple pattern)
    src << 0.0, 1.0, 0.0, 0.0, 0.5,
           0.0, 0.0, 1.0, 0.0, 0.5,
           0.0, 0.0, 0.0, 1.0, 0.5;

    // Known transformation: 90° rotation around Z axis + translation
    Eigen::Matrix3d R_true;
    R_true << 0, -1,  0,
              1,  0,  0,
              0,  0,  1;
    Eigen::Vector3d t_true(0.1, 0.2, 0.3);

    // Apply transformation to create destination points
    dst = R_true * src;
    for (int i = 0; i < dst.cols(); ++i) {
        dst.col(i) += t_true;
    }

    std::cout << "Test data created:" << std::endl;
    std::cout << "  Source points: " << src.cols() << std::endl;
    std::cout << "  Destination points: " << dst.cols() << std::endl;
    std::cout << "  True rotation: 90° around Z-axis" << std::endl;
    std::cout << "  True translation: [0.1, 0.2, 0.3]" << std::endl;
    std::cout << std::endl;

    // Run TEASER++ registration
    std::cout << "Running TEASER++ registration..." << std::endl;
    solver.solve(src, dst);

    auto solution = solver.getSolution();

    std::cout << "✓ Registration completed" << std::endl;
    std::cout << std::endl;

    // Display results
    std::cout << "========================================" << std::endl;
    std::cout << "Registration Results:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Estimated Rotation Matrix:" << std::endl;
    std::cout << solution.rotation << std::endl;
    std::cout << std::endl;

    std::cout << "Estimated Translation:" << std::endl;
    std::cout << solution.translation.transpose() << std::endl;
    std::cout << std::endl;

    std::cout << "Scale estimate: " << solution.scale << std::endl;
    std::cout << std::endl;

    // Verify accuracy
    double rotation_error = (solution.rotation - R_true).norm();
    double translation_error = (solution.translation - t_true).norm();

    std::cout << "========================================" << std::endl;
    std::cout << "Accuracy Verification:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Rotation error: " << rotation_error << std::endl;
    std::cout << "Translation error: " << translation_error << std::endl;
    std::cout << std::endl;

    bool success = (rotation_error < 1e-6 && translation_error < 1e-6);

    if (success) {
        std::cout << "✓✓✓ TEASER++ INTEGRATION TEST PASSED ✓✓✓" << std::endl;
        std::cout << "TEASER++ is working correctly and ready for rim assembly!" << std::endl;
    } else {
        std::cout << "✗✗✗ TEASER++ INTEGRATION TEST FAILED ✗✗✗" << std::endl;
        std::cout << "Registration accuracy below expected threshold" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return success ? 0 : 1;
}
