#include "rim_base_evidence.h"
#include "puzzlefusion_global_optimizer.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>

// ============================ HELPER FUNCTIONS ============================

/**
 * Fit 2D circle to projected points using Huber-weighted robust least squares
 * Based on existing fitAndValidateCircle() implementation
 */
void RimBaseValidator::fitCircle2DHuber(
    const std::vector<Eigen::Vector2d>& points,
    double& cx, double& cy, double& radius,
    double& rms_error, double& inlier_ratio,
    const ArchaeologicalConfig& config) const {

    if (points.size() < 3) {
        cx = cy = radius = rms_error = 0.0;
        inlier_ratio = 0.0;
        return;
    }

    // Step 1: Initial algebraic fit (unweighted)
    // Linearize: x^2 + y^2 = 2*cx*x + 2*cy*y + (r^2 - cx^2 - cy^2)
    Eigen::MatrixXd A(points.size(), 3);
    Eigen::VectorXd b(points.size());

    for (size_t i = 0; i < points.size(); ++i) {
        double x = points[i].x();
        double y = points[i].y();
        A(i, 0) = 2.0 * x;
        A(i, 1) = 2.0 * y;
        A(i, 2) = 1.0;
        b(i) = x * x + y * y;
    }

    Eigen::Vector3d params = A.colPivHouseholderQr().solve(b);
    cx = params(0);
    cy = params(1);
    double c = params(2);
    radius = std::sqrt(cx * cx + cy * cy + c);

    // Step 2: Compute residuals and MAD (for Huber threshold)
    std::vector<double> residuals;
    for (const auto& pt : points) {
        double dist = std::sqrt((pt.x() - cx) * (pt.x() - cx) +
                               (pt.y() - cy) * (pt.y() - cy));
        residuals.push_back(std::abs(dist - radius));
    }

    // MAD = median absolute deviation
    std::vector<double> sorted_residuals = residuals;
    std::sort(sorted_residuals.begin(), sorted_residuals.end());
    double median = sorted_residuals[sorted_residuals.size() / 2];
    std::vector<double> abs_devs;
    for (double r : sorted_residuals) {
        abs_devs.push_back(std::abs(r - median));
    }
    std::sort(abs_devs.begin(), abs_devs.end());
    double mad = abs_devs[abs_devs.size() / 2];

    // Huber threshold (1.4826 * MAD for normal distribution)
    double huber_threshold = 1.4826 * mad + 1e-6;

    // Step 3: Huber-weighted refinement (3 iterations)
    for (int iter = 0; iter < 3; ++iter) {
        Eigen::MatrixXd A_weighted(points.size(), 3);
        Eigen::VectorXd b_weighted(points.size());

        for (size_t i = 0; i < points.size(); ++i) {
            double x = points[i].x();
            double y = points[i].y();
            double residual = residuals[i];

            // Huber weight
            double weight = (residual <= huber_threshold) ? 1.0 : huber_threshold / residual;

            A_weighted(i, 0) = 2.0 * x * weight;
            A_weighted(i, 1) = 2.0 * y * weight;
            A_weighted(i, 2) = 1.0 * weight;
            b_weighted(i) = (x * x + y * y) * weight;
        }

        params = A_weighted.colPivHouseholderQr().solve(b_weighted);
        cx = params(0);
        cy = params(1);
        c = params(2);
        radius = std::sqrt(cx * cx + cy * cy + c);

        // Recompute residuals
        residuals.clear();
        for (const auto& pt : points) {
            double dist = std::sqrt((pt.x() - cx) * (pt.x() - cx) +
                                   (pt.y() - cy) * (pt.y() - cy));
            residuals.push_back(std::abs(dist - radius));
        }
    }

    // Step 4: Compute final RMS and inlier ratio
    double sum_sq = 0.0;
    int inlier_count = 0;
    double inlier_threshold = config.rim_primitive_rms_threshold;  // Use config threshold

    for (double r : residuals) {
        sum_sq += r * r;
        if (r < inlier_threshold) {
            inlier_count++;
        }
    }

    rms_error = std::sqrt(sum_sq / residuals.size());
    inlier_ratio = static_cast<double>(inlier_count) / residuals.size();
}

/**
 * Fit plane to 3D points using Huber-weighted least squares
 * Plane equation: normal · (point - point_on_plane) = 0
 */
void RimBaseValidator::fitPlaneHuber(
    const std::vector<Eigen::Vector3d>& points,
    Eigen::Vector3d& normal, double& distance,
    double& rms_error, double& inlier_ratio,
    const ArchaeologicalConfig& config) const {

    if (points.size() < 3) {
        normal = Eigen::Vector3d::UnitZ();
        distance = rms_error = 0.0;
        inlier_ratio = 0.0;
        return;
    }

    // Step 1: Compute centroid
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& pt : points) {
        centroid += pt;
    }
    centroid /= points.size();

    // Step 2: Initial PCA-based plane fit
    // Build covariance matrix
    Eigen::Matrix3d cov = Eigen::Matrix3d::Zero();
    for (const auto& pt : points) {
        Eigen::Vector3d centered = pt - centroid;
        cov += centered * centered.transpose();
    }
    cov /= points.size();

    // Plane normal = eigenvector corresponding to smallest eigenvalue
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigen_solver(cov);
    normal = eigen_solver.eigenvectors().col(0);  // Smallest eigenvalue first
    distance = normal.dot(centroid);

    // Step 3: Compute residuals and MAD
    std::vector<double> residuals;
    for (const auto& pt : points) {
        double dist = std::abs(normal.dot(pt) - distance);
        residuals.push_back(dist);
    }

    std::vector<double> sorted_residuals = residuals;
    std::sort(sorted_residuals.begin(), sorted_residuals.end());
    double median = sorted_residuals[sorted_residuals.size() / 2];
    std::vector<double> abs_devs;
    for (double r : sorted_residuals) {
        abs_devs.push_back(std::abs(r - median));
    }
    std::sort(abs_devs.begin(), abs_devs.end());
    double mad = abs_devs[abs_devs.size() / 2];
    double huber_threshold = 1.4826 * mad + 1e-6;

    // Step 4: Huber-weighted refinement (3 iterations)
    for (int iter = 0; iter < 3; ++iter) {
        Eigen::Matrix3d cov_weighted = Eigen::Matrix3d::Zero();
        double total_weight = 0.0;

        for (size_t i = 0; i < points.size(); ++i) {
            double residual = residuals[i];
            double weight = (residual <= huber_threshold) ? 1.0 : huber_threshold / residual;

            Eigen::Vector3d centered = points[i] - centroid;
            cov_weighted += weight * (centered * centered.transpose());
            total_weight += weight;
        }

        cov_weighted /= total_weight;

        eigen_solver.compute(cov_weighted);
        normal = eigen_solver.eigenvectors().col(0);
        distance = normal.dot(centroid);

        // Recompute residuals
        residuals.clear();
        for (const auto& pt : points) {
            double dist = std::abs(normal.dot(pt) - distance);
            residuals.push_back(dist);
        }
    }

    // Step 5: Compute final RMS and inlier ratio
    double sum_sq = 0.0;
    int inlier_count = 0;
    double inlier_threshold = config.base_primitive_rms_threshold;

    for (double r : residuals) {
        sum_sq += r * r;
        if (r < inlier_threshold) {
            inlier_count++;
        }
    }

    rms_error = std::sqrt(sum_sq / residuals.size());
    inlier_ratio = static_cast<double>(inlier_count) / residuals.size();
}

/**
 * Compute angular coverage of points around circle center with gap detection
 * Returns fraction [0,1] of 360 degrees covered
 *
 * FIX (2025-10-18): Gap-aware calculation prevents false sufficiency detection
 * OLD BUG: Calculated max-min span, ignoring gaps (e.g., points at [10°, 350°] = 340° coverage)
 * NEW FIX: Detects gaps > threshold, sums only continuous arcs
 */
double RimBaseValidator::computeAngularCoverage(
    const std::vector<Eigen::Vector2d>& points,
    double cx, double cy,
    double gap_threshold_deg) const {

    if (points.size() < 2) {
        return 0.0;
    }

    // Compute angle for each point
    std::vector<double> angles;
    for (const auto& pt : points) {
        double dx = pt.x() - cx;
        double dy = pt.y() - cy;
        double angle = std::atan2(dy, dx);  // [-π, π]
        if (angle < 0) angle += 2.0 * M_PI;  // [0, 2π]
        angles.push_back(angle * 180.0 / M_PI);  // Convert to degrees [0, 360)
    }

    std::sort(angles.begin(), angles.end());

    // DEBUG: Print first/last 5 angles to understand distribution
    std::cout << "[ANGLE DISTRIBUTION] First 5 angles: ";
    for (size_t i = 0; i < std::min(size_t(5), angles.size()); ++i) {
        std::cout << std::fixed << std::setprecision(1) << angles[i] << "° ";
    }
    std::cout << std::endl << "[ANGLE DISTRIBUTION] Last 5 angles: ";
    for (size_t i = std::max(size_t(0), angles.size() - 5); i < angles.size(); ++i) {
        std::cout << std::fixed << std::setprecision(1) << angles[i] << "° ";
    }
    std::cout << std::endl;

    // Detect continuous arcs by finding gaps
    std::vector<std::pair<double, double>> arcs;  // (start_angle, end_angle) pairs

    double arc_start = angles[0];
    double arc_end = angles[0];

    double max_gap = 0.0;
    for (size_t i = 1; i < angles.size(); ++i) {
        double gap = angles[i] - angles[i-1];
        if (gap > max_gap) max_gap = gap;

        if (gap <= gap_threshold_deg) {
            // Continuous arc - extend current arc
            arc_end = angles[i];
        } else {
            // Large gap detected - save current arc and start new one
            arcs.push_back({arc_start, arc_end});
            arc_start = angles[i];
            arc_end = angles[i];
        }
    }
    // Save final arc
    arcs.push_back({arc_start, arc_end});

    // DEBUG: Print gap detection results
    std::cout << "[GAP DETECTION] " << points.size() << " points → " << arcs.size() << " continuous arcs" << std::endl;
    for (size_t i = 0; i < arcs.size(); ++i) {
        double arc_span = (arcs[i].second >= arcs[i].first) ?
                         (arcs[i].second - arcs[i].first) :
                         (360.0 - arcs[i].first + arcs[i].second);
        std::cout << "  Arc " << i << ": [" << std::fixed << std::setprecision(1)
                  << arcs[i].first << "° → " << arcs[i].second << "°] span="
                  << arc_span << "°" << std::endl;
    }

    // Check wraparound gap (from last angle back to first through 0°)
    double wraparound_gap = (360.0 - angles.back()) + angles.front();
    if (wraparound_gap > max_gap) max_gap = wraparound_gap;

    std::cout << "[GAP ANALYSIS] Largest gap between consecutive points: " << std::fixed
              << std::setprecision(1) << max_gap << "° (threshold=" << gap_threshold_deg << "°)" << std::endl;

    if (wraparound_gap <= gap_threshold_deg && arcs.size() >= 2) {
        // Merge first and last arcs (they're actually continuous around 0°)
        // Extend first arc to include last arc's span
        double merged_start = arcs.back().first;
        double merged_end = arcs[0].second;

        // Remove separate arcs and create merged wraparound arc
        arcs.erase(arcs.begin());  // Remove first arc
        arcs.pop_back();           // Remove last arc
        arcs.push_back({merged_start, merged_end});  // Add merged arc
    }

    // Sum total coverage from all continuous arcs
    double total_coverage = 0.0;

    for (const auto& arc : arcs) {
        double arc_span;

        if (arc.second >= arc.first) {
            // Normal arc (doesn't wrap around 0°)
            arc_span = arc.second - arc.first;
        } else {
            // Wraparound arc (goes from arc.first through 360° back to arc.second)
            arc_span = (360.0 - arc.first) + arc.second;
        }

        total_coverage += arc_span;
    }

    double coverage_fraction = total_coverage / 360.0;
    std::cout << "[COVERAGE RESULT] Total: " << total_coverage << "° → "
              << std::fixed << std::setprecision(1) << (coverage_fraction * 100.0)
              << "% coverage (gap_threshold=" << gap_threshold_deg << "°)" << std::endl;

    return coverage_fraction;  // Fraction [0,1]
}

/**
 * Compute Pearson correlation coefficient between two sequences
 */
double RimBaseValidator::computeCorrelation(
    const std::vector<double>& x,
    const std::vector<double>& y) const {

    if (x.size() != y.size() || x.size() < 2) {
        return 0.0;
    }

    double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
    double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / y.size();

    double numerator = 0.0;
    double sum_sq_x = 0.0;
    double sum_sq_y = 0.0;

    for (size_t i = 0; i < x.size(); ++i) {
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        numerator += dx * dy;
        sum_sq_x += dx * dx;
        sum_sq_y += dy * dy;
    }

    double denominator = std::sqrt(sum_sq_x * sum_sq_y);
    if (denominator < 1e-6) {
        return 0.0;
    }

    return numerator / denominator;
}

// ============================ PRIMITIVE FITTING ============================

PrimitiveEvidence RimBaseValidator::fitRimPrimitive(
    const std::vector<Eigen::Vector3d>& edge_points,
    const Eigen::Vector3d& axis_normal,
    double height,
    const ArchaeologicalConfig& config) const {

    PrimitiveEvidence evidence;

    if (edge_points.size() < 3) {
        return evidence;  // Invalid - not enough points
    }

    // Step 1: Compute centroid for projection plane
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& pt : edge_points) {
        centroid += pt;
    }
    centroid /= edge_points.size();

    // Step 2: Create orthonormal basis for plane perpendicular to axis
    Eigen::Vector3d u = axis_normal.cross(Eigen::Vector3d::UnitX()).normalized();
    if (u.norm() < 0.1) {
        u = axis_normal.cross(Eigen::Vector3d::UnitY()).normalized();
    }
    Eigen::Vector3d v = axis_normal.cross(u).normalized();

    // Step 3: Project points to 2D plane
    std::vector<Eigen::Vector2d> projected_points;
    for (const auto& pt : edge_points) {
        // Project to plane perpendicular to axis
        Eigen::Vector3d projected = pt - axis_normal.dot(pt - centroid) * axis_normal;
        double x = u.dot(projected - centroid);
        double y = v.dot(projected - centroid);
        projected_points.push_back(Eigen::Vector2d(x, y));
    }

    // Step 4: Fit circle using Huber-weighted robust least squares
    double cx_2d, cy_2d;
    fitCircle2DHuber(projected_points, cx_2d, cy_2d, evidence.radius,
                     evidence.rms_error, evidence.inlier_ratio, config);

    std::cout << "[CIRCLE FIT] Fitted center in 2D: (" << std::fixed << std::setprecision(2)
              << cx_2d << ", " << cy_2d << "), radius=" << evidence.radius << " mm" << std::endl;

    // Step 5: Convert 2D circle center back to 3D
    evidence.center = centroid + cx_2d * u + cy_2d * v;
    evidence.normal = axis_normal;

    // Step 6: Compute angular coverage (with gap detection)
    evidence.angular_coverage = computeAngularCoverage(
        projected_points, cx_2d, cy_2d, config.angular_gap_threshold);

    // Step 7: Validate quality
    evidence.is_valid = (evidence.rms_error < config.rim_primitive_rms_threshold &&
                        evidence.inlier_ratio >= config.min_inlier_ratio);

    return evidence;
}

PrimitiveEvidence RimBaseValidator::fitBasePrimitive(
    const std::vector<Eigen::Vector3d>& edge_points,
    const Eigen::Vector3d& axis_normal,
    const ArchaeologicalConfig& config,
    bool& has_foot_ring) const {

    PrimitiveEvidence evidence;
    has_foot_ring = false;

    if (edge_points.size() < 3) {
        return evidence;  // Invalid - not enough points
    }

    // Step 1: Fit plane to edge points
    Eigen::Vector3d plane_normal;
    double plane_distance;
    fitPlaneHuber(edge_points, plane_normal, plane_distance,
                  evidence.rms_error, evidence.inlier_ratio, config);

    // Step 2: Check if plane is approximately horizontal (base should be flat)
    // Plane normal should align with axis (both point up/down)
    double axis_alignment = std::abs(plane_normal.dot(axis_normal));

    // If plane is not horizontal enough, reject as base
    if (axis_alignment < 0.8) {  // cos(~37°) threshold
        return evidence;  // Invalid base
    }

    evidence.normal = plane_normal;
    evidence.center = plane_distance * plane_normal;  // Point on plane closest to origin

    // Step 3: Project points to fitted plane and compute perimeter circle
    std::vector<Eigen::Vector2d> projected_points;
    Eigen::Vector3d u = plane_normal.cross(Eigen::Vector3d::UnitX()).normalized();
    if (u.norm() < 0.1) {
        u = plane_normal.cross(Eigen::Vector3d::UnitY()).normalized();
    }
    Eigen::Vector3d v = plane_normal.cross(u).normalized();

    for (const auto& pt : edge_points) {
        // Project to plane
        Eigen::Vector3d projected = pt - plane_normal.dot(pt - evidence.center) * plane_normal;
        double x = u.dot(projected - evidence.center);
        double y = v.dot(projected - evidence.center);
        projected_points.push_back(Eigen::Vector2d(x, y));
    }

    // Step 4: Fit circle to projected points (base perimeter)
    double cx_2d, cy_2d;
    double circle_rms, circle_inlier_ratio;
    fitCircle2DHuber(projected_points, cx_2d, cy_2d, evidence.radius,
                     circle_rms, circle_inlier_ratio, config);

    // Update 3D center
    evidence.center = evidence.center + cx_2d * u + cy_2d * v;

    // Step 5: Compute angular coverage (with gap detection)
    evidence.angular_coverage = computeAngularCoverage(
        projected_points, cx_2d, cy_2d, config.angular_gap_threshold);

    // Step 6: Attempt foot-ring (annulus) detection (optional)
    // Foot ring = two concentric circles (inner and outer radius)
    // Check if points cluster into two radial bands
    std::vector<double> radii;
    for (const auto& pt : projected_points) {
        double r = std::sqrt((pt.x() - cx_2d) * (pt.x() - cx_2d) +
                            (pt.y() - cy_2d) * (pt.y() - cy_2d));
        radii.push_back(r);
    }

    std::sort(radii.begin(), radii.end());
    double median_radius = radii[radii.size() / 2];
    double radial_spread = radii.back() - radii.front();

    // If radial spread is significant (5-20mm) and matches foot-ring thickness
    if (radial_spread >= config.foot_ring_min_thickness &&
        radial_spread <= config.foot_ring_max_thickness) {

        // Check if points cluster at inner and outer radii
        int inner_count = 0;
        int outer_count = 0;
        double inner_threshold = median_radius - radial_spread / 4.0;
        double outer_threshold = median_radius + radial_spread / 4.0;

        for (double r : radii) {
            if (r < inner_threshold) inner_count++;
            if (r > outer_threshold) outer_count++;
        }

        // If significant clustering at both inner/outer, it's a foot-ring
        if (inner_count > static_cast<int>(radii.size() * 0.2) &&
            outer_count > static_cast<int>(radii.size() * 0.2)) {
            has_foot_ring = true;
        }
    }

    // Step 7: Validate quality (accept plane fit as primary criterion)
    evidence.is_valid = (evidence.rms_error < config.base_primitive_rms_threshold &&
                        evidence.inlier_ratio >= config.min_inlier_ratio &&
                        axis_alignment >= 0.8);

    return evidence;
}

// ============================ SMOOTHNESS AND TANGENCY ============================

SmoothnessEvidence RimBaseValidator::edgeSmoothnessAndTangency(
    const std::vector<Eigen::Vector3d>& edge_points,
    const std::vector<Eigen::Vector3d>& edge_normals,
    const Eigen::Vector3d& axis_normal,
    const ArchaeologicalConfig& config) const {

    SmoothnessEvidence evidence;

    if (edge_points.size() < 3 || edge_normals.size() != edge_points.size()) {
        return evidence;  // Invalid
    }

    // Step 1: Compute curvature estimates along edge
    // Use simple finite differences on consecutive edge segments
    std::vector<double> curvatures;

    for (size_t i = 1; i < edge_points.size() - 1; ++i) {
        Eigen::Vector3d prev = edge_points[i - 1];
        Eigen::Vector3d curr = edge_points[i];
        Eigen::Vector3d next = edge_points[i + 1];

        Eigen::Vector3d tangent1 = (curr - prev).normalized();
        Eigen::Vector3d tangent2 = (next - curr).normalized();

        // Curvature ≈ angle change / arc length
        double angle_change = std::acos(std::max(-1.0, std::min(1.0, tangent1.dot(tangent2))));
        double arc_length = (curr - prev).norm() + (next - curr).norm();

        if (arc_length > 1e-6) {
            curvatures.push_back(angle_change / arc_length);
        }
    }

    // Step 2: Compute curvature noise (standard deviation)
    if (!curvatures.empty()) {
        double mean = std::accumulate(curvatures.begin(), curvatures.end(), 0.0) / curvatures.size();
        double variance = 0.0;
        for (double k : curvatures) {
            variance += (k - mean) * (k - mean);
        }
        evidence.curvature_noise = std::sqrt(variance / curvatures.size());
    }

    // Step 3: Compute tangency to axis
    // Rim edge should wrap around axis → tangent perpendicular to axis
    std::vector<double> tangency_angles;

    for (size_t i = 1; i < edge_points.size(); ++i) {
        Eigen::Vector3d tangent = (edge_points[i] - edge_points[i - 1]).normalized();

        // Angle between tangent and axis (should be ~90° for rim)
        double dot = std::abs(tangent.dot(axis_normal));
        double angle = std::acos(std::max(0.0, std::min(1.0, dot))) * 180.0 / M_PI;

        // Measure deviation from 90° (perpendicular)
        tangency_angles.push_back(std::abs(angle - 90.0));
    }

    // Average tangency deviation
    if (!tangency_angles.empty()) {
        evidence.axis_tangency_angle = std::accumulate(tangency_angles.begin(),
                                                       tangency_angles.end(), 0.0) / tangency_angles.size();
    }

    // Step 4: Validate smoothness
    evidence.is_smooth = (evidence.curvature_noise < config.max_curvature_noise &&
                         evidence.axis_tangency_angle < config.max_tangency_angle);

    return evidence;
}

// ============================ THICKNESS TRENDS ============================

ThicknessTrendEvidence RimBaseValidator::thicknessTrend(
    const std::vector<double>& thickness_values,
    const ArchaeologicalConfig& config) const {

    ThicknessTrendEvidence evidence;

    if (thickness_values.size() < 3) {
        return evidence;  // Not enough data
    }

    // Step 1: Compute mean thickness
    evidence.mean_thickness = std::accumulate(thickness_values.begin(),
                                              thickness_values.end(), 0.0) / thickness_values.size();

    // Step 2: Compute thickness gradient (correlation with position along edge)
    std::vector<double> positions;
    for (size_t i = 0; i < thickness_values.size(); ++i) {
        positions.push_back(static_cast<double>(i));
    }

    evidence.gradient_consistency = computeCorrelation(positions, thickness_values);

    // Step 3: Determine trend direction
    // Positive correlation = thickening (base-like)
    // Negative correlation = thinning (rim-like)
    if (std::abs(evidence.gradient_consistency) >= config.thickness_gradient_threshold) {
        if (evidence.gradient_consistency > 0) {
            evidence.is_thickening = true;
        } else {
            evidence.is_thinning = true;
        }
    }

    return evidence;
}

// ============================ SINGLE-PIECE SUFFICIENCY ============================

bool RimBaseValidator::checkSinglePieceSufficiency(
    const PrimitiveEvidence& primitive,
    const ArchaeologicalConfig& config,
    bool is_rim) const {

    // Check if single piece covers 80%+ of rim/base with good fit quality
    double rms_threshold = is_rim ? config.rim_primitive_rms_threshold : config.base_primitive_rms_threshold;

    return (primitive.is_valid &&
            primitive.angular_coverage >= config.min_coverage_for_completion &&
            primitive.rms_error < rms_threshold &&
            primitive.inlier_ratio >= config.min_inlier_ratio);
}
