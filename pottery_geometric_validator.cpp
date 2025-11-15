#include "pottery_geometric_validator.h"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace Eigen;

// ========================================================================
// PotteryGeometricValidator Implementation
// ========================================================================

PotteryGeometricValidator::PotteryGeometricValidator(const Config& config)
    : config_(config) {}

ConnectionValidationResult PotteryGeometricValidator::validate_pottery_connection(
    const PotteryPieceData& piece1_data,
    const PotteryPieceData& piece2_data,
    int icp_inliers,
    double correspondence_quality
) const {

    ConnectionValidationResult result;

    // ===== STAGE 1: POTTERY STRUCTURE VALIDATION (PRIMARY) =====

    result.axis_alignment_score = calculate_axis_alignment_score(piece1_data, piece2_data);
    result.height_compatibility_score = calculate_height_compatibility_score(piece1_data, piece2_data);
    result.radial_proximity_score = calculate_radial_proximity_score(piece1_data, piece2_data);
    result.breakline_compatibility_score = calculate_breakline_compatibility_score(piece1_data, piece2_data);

    // Weighted pottery validity score
    result.pottery_validity_score = (
        result.axis_alignment_score * 0.30 +         // Vessel orientation consistency
        result.height_compatibility_score * 0.25 +   // Rim/body/base logic
        result.radial_proximity_score * 0.25 +       // Circular topology
        result.breakline_compatibility_score * 0.20  // Edge curve compatibility
    );

    result.is_pottery_valid = (result.pottery_validity_score >= config_.pottery_validity_threshold);

    // ===== STAGE 2: SURFACE CONTINUITY (SECONDARY, GATED BY POTTERY VALIDITY) =====

    if (result.is_pottery_valid) {
        result.surface_continuity_score = calculate_surface_continuity_score(
            icp_inliers, correspondence_quality
        );

        // Final combined score: pottery gates surface quality
        result.combined_score = result.pottery_validity_score *
                               config_.pottery_structure_weight *
                               result.surface_continuity_score *
                               config_.surface_continuity_weight;
    } else {
        result.surface_continuity_score = 0.0;
        result.combined_score = 0.0;
        result.rejection_reason = "Failed pottery structure validation (score: " +
                                 to_string(result.pottery_validity_score) +
                                 " < threshold: " + to_string(config_.pottery_validity_threshold) + ")";
    }

    return result;
}

double PotteryGeometricValidator::calculate_axis_alignment_score(
    const PotteryPieceData& p1,
    const PotteryPieceData& p2
) const {

    if (p1.axes.empty() || p2.axes.empty()) {
        return 0.5; // Neutral score if axis data missing
    }

    // Find best axis alignment between pieces
    double best_alignment = 0.0;

    for (const auto& axis1 : p1.axes) {
        for (const auto& axis2 : p2.axes) {
            // Calculate angle between axis directions
            double dot_product = abs(axis1.direction.dot(axis2.direction));
            dot_product = min(1.0, max(-1.0, dot_product)); // Clamp for numerical stability

            double angle_radians = acos(dot_product);
            double angle_degrees = angle_radians * 180.0 / M_PI;

            // Pottery vessels should have similar axis orientations
            // but allow variation for vessel shape changes (rim flare, body curves)
            double alignment_score = gaussian_falloff(angle_degrees, config_.max_axis_angle_degrees);
            best_alignment = max(best_alignment, alignment_score);
        }
    }

    return best_alignment;
}

double PotteryGeometricValidator::calculate_height_compatibility_score(
    const PotteryPieceData& p1,
    const PotteryPieceData& p2
) const {

    double height_diff = abs(p1.average_height - p2.average_height);

    // Pottery connection logic based on vessel structure:
    // 1. Same level connections (rim-rim, body-body, base-base): High score
    // 2. Adjacent level connections (rim-body, body-base): Medium score
    // 3. Distant level connections (rim-base): Low score (but not impossible)

    if (height_diff <= config_.same_level_height_threshold) {
        return 1.0; // Same level - ideal pottery connection
    } else if (height_diff <= config_.adjacent_level_height_threshold) {
        return 0.7; // Adjacent level - realistic pottery transition
    } else {
        // Distant levels - possible for large pieces spanning rim-to-base
        return gaussian_falloff(height_diff, config_.adjacent_level_height_threshold * 2.0);
    }
}

double PotteryGeometricValidator::calculate_radial_proximity_score(
    const PotteryPieceData& p1,
    const PotteryPieceData& p2
) const {

    // Calculate radial distance in vessel XY plane
    Vector2d radial_pos1 = p1.get_radial_center();
    Vector2d radial_pos2 = p2.get_radial_center();
    double radial_distance = (radial_pos1 - radial_pos2).norm();

    // Also consider angular separation around vessel axis
    double angular_diff = angular_distance(p1.angular_position, p2.angular_position);

    // Pottery pieces should be:
    // 1. Radially close (adjacent around vessel circumference)
    // 2. Angularly nearby (within reasonable arc)

    double radial_score = gaussian_falloff(radial_distance, config_.max_radial_distance);
    double angular_score = gaussian_falloff(angular_diff, config_.max_angular_separation);

    // Weight radial proximity more heavily than angular
    return radial_score * 0.7 + angular_score * 0.3;
}

double PotteryGeometricValidator::calculate_breakline_compatibility_score(
    const PotteryPieceData& p1,
    const PotteryPieceData& p2
) const {

    if (p1.breaklines.empty() || p2.breaklines.empty()) {
        return 0.8; // Neutral score if breakline data limited
    }

    double best_compatibility = 0.0;

    for (const auto& bl1 : p1.breaklines) {
        for (const auto& bl2 : p2.breaklines) {
            // Check if breaklines can form continuous pottery edge
            bool curves_compatible = bl1.is_compatible_with(bl2, config_.breakline_curve_tolerance);

            if (curves_compatible) {
                // Calculate curvature similarity for realistic pottery fracture
                double curvature_match = 0.0;
                size_t min_points = min(bl1.curvatures.size(), bl2.curvatures.size());

                for (size_t i = 0; i < min_points; ++i) {
                    double curv_diff = abs(bl1.curvatures[i] - bl2.curvatures[i]);
                    curvature_match += gaussian_falloff(curv_diff, config_.curvature_matching_tolerance);
                }

                if (min_points > 0) {
                    curvature_match /= min_points;
                }

                best_compatibility = max(best_compatibility, curvature_match);
            }
        }
    }

    return best_compatibility;
}

double PotteryGeometricValidator::calculate_surface_continuity_score(
    int icp_inliers,
    double correspondence_quality
) const {

    // Surface quality combines ICP geometric fit with correspondence reliability

    // Normalize ICP inliers (more inliers = better surface alignment)
    double inlier_score = 0.0;
    if (icp_inliers >= config_.min_icp_inliers) {
        // Logarithmic scaling - diminishing returns for very high inlier counts
        inlier_score = min(1.0, log(static_cast<double>(icp_inliers) / config_.min_icp_inliers + 1.0) / log(6.0));
    }

    // Correspondence quality reflects point-to-point matching accuracy
    double quality_score = min(1.0, max(0.0, correspondence_quality));

    return inlier_score * 0.6 + quality_score * 0.4;
}

double PotteryGeometricValidator::validate_pottery_normals(
    const std::vector<Vector3d>& normals1,
    const std::vector<Vector3d>& normals2,
    const PotteryPieceData& piece1,
    const PotteryPieceData& piece2
) const {

    if (!config_.enable_pottery_normal_validation || normals1.empty() || normals2.empty()) {
        return 1.0; // Skip validation or neutral score
    }

    // POTTERY-AWARE NORMAL VALIDATION:
    // Unlike generic 3D objects, pottery connections SHOULD have normal differences
    // due to vessel curvature, rim flare, and natural surface flow

    double total_normal_score = 0.0;
    size_t valid_comparisons = 0;

    for (const auto& n1 : normals1) {
        for (const auto& n2 : normals2) {
            double dot_product = n1.dot(n2);
            double normal_score = 0.0;

            // Context-aware normal validation based on pottery connection type
            if (is_rim_body_transition(piece1, piece2)) {
                // Rim-to-body: EXPECT large normal changes (vessel flare/curve)
                // Score HIGHER for moderate differences, don't penalize normal changes
                double angle = acos(abs(dot_product));
                normal_score = (angle > 0.3 && angle < 1.2) ? 1.0 : 0.7; // Prefer 17-69 degree changes

            } else if (is_circumferential_connection(piece1, piece2)) {
                // Around vessel circumference: EXPECT moderate normal changes
                // Due to circular vessel curvature
                if (dot_product >= config_.circumferential_normal_tolerance) {
                    normal_score = 1.0;
                } else {
                    normal_score = gaussian_falloff(acos(abs(dot_product)), M_PI/3); // Allow up to 60 degrees
                }

            } else {
                // Standard pottery connection: Similar normals preferred but not required
                if (dot_product >= config_.standard_normal_tolerance) {
                    normal_score = 1.0;
                } else {
                    normal_score = gaussian_falloff(acos(abs(dot_product)), M_PI/4); // Allow up to 45 degrees
                }
            }

            total_normal_score += normal_score;
            valid_comparisons++;
        }
    }

    return valid_comparisons > 0 ? total_normal_score / valid_comparisons : 1.0;
}

// ========================================================================
// Context Analysis Methods
// ========================================================================

bool PotteryGeometricValidator::is_rim_body_transition(
    const PotteryPieceData& p1,
    const PotteryPieceData& p2
) const {

    // Detect rim-to-body connections based on height difference and piece classification
    bool one_is_rim = p1.is_rim_piece() || p2.is_rim_piece();
    bool one_is_body = (!p1.is_rim_piece() && !p1.is_base_piece()) ||
                       (!p2.is_rim_piece() && !p2.is_base_piece());

    double height_diff = abs(p1.average_height - p2.average_height);
    bool significant_height_diff = height_diff > config_.same_level_height_threshold;

    return one_is_rim && one_is_body && significant_height_diff;
}

bool PotteryGeometricValidator::is_circumferential_connection(
    const PotteryPieceData& p1,
    const PotteryPieceData& p2
) const {

    // Detect connections around vessel circumference (same height level)
    double height_diff = abs(p1.average_height - p2.average_height);
    bool same_height_level = height_diff <= config_.same_level_height_threshold;

    double angular_diff = angular_distance(p1.angular_position, p2.angular_position);
    bool angularly_adjacent = angular_diff <= config_.max_angular_separation;

    return same_height_level && angularly_adjacent;
}

bool PotteryGeometricValidator::is_vertical_adjacency(
    const PotteryPieceData& p1,
    const PotteryPieceData& p2
) const {

    // Detect vertical connections (rim-body, body-base)
    double height_diff = abs(p1.average_height - p2.average_height);
    bool height_adjacent = height_diff > config_.same_level_height_threshold &&
                          height_diff <= config_.adjacent_level_height_threshold;

    Vector2d radial_pos1 = p1.get_radial_center();
    Vector2d radial_pos2 = p2.get_radial_center();
    double radial_distance = (radial_pos1 - radial_pos2).norm();
    bool radially_aligned = radial_distance <= config_.max_radial_distance * 0.5;

    return height_adjacent && radially_aligned;
}

// ========================================================================
// Utility Methods
// ========================================================================

double PotteryGeometricValidator::gaussian_falloff(double value, double max_value) const {
    if (max_value <= 0) return 0.0;
    double normalized = value / max_value;
    return exp(-normalized * normalized);
}

double PotteryGeometricValidator::angular_distance(double angle1, double angle2) const {
    double diff = abs(angle1 - angle2);
    // Handle wraparound (e.g., 350° and 10° are 20° apart, not 340°)
    return min(diff, 360.0 - diff);
}

Vector2d PotteryGeometricValidator::project_to_vessel_plane(const Vector3d& point) const {
    // Project 3D point to XY plane for radial distance calculations
    return Vector2d(point.x(), point.y());
}

// ========================================================================
// PotteryPieceData Implementation
// ========================================================================

Vector3d PotteryPieceData::get_primary_axis_direction() const {
    if (axes.empty()) return Vector3d(0, 0, 1); // Default vertical

    // Return the first axis direction, or average if multiple
    if (axes.size() == 1) {
        return axes[0].direction;
    } else {
        Vector3d avg_direction = Vector3d::Zero();
        for (const auto& axis : axes) {
            avg_direction += axis.direction;
        }
        avg_direction.normalize();
        return avg_direction;
    }
}

Vector2d PotteryPieceData::get_radial_center() const {
    return Vector2d(center_of_mass.x(), center_of_mass.y());
}

bool PotteryPieceData::is_rim_piece() const {
    // Classify as rim if in upper height range
    // This would need calibration based on actual pottery dimensions
    return average_height > 300.0; // Example threshold
}

bool PotteryPieceData::is_base_piece() const {
    // Classify as base if in lower height range
    return average_height < 150.0; // Example threshold
}

double PotteryPieceData::get_height_range() const {
    if (axes.empty()) return 0.0;

    double min_height = axes[0].height;
    double max_height = axes[0].height;

    for (const auto& axis : axes) {
        min_height = min(min_height, axis.height);
        max_height = max(max_height, axis.height);
    }

    return max_height - min_height;
}

// ========================================================================
// BreaklineCurve Implementation
// ========================================================================

double BreaklineCurve::get_curvature_at(size_t index) const {
    if (index >= curvatures.size()) return 0.0;
    return curvatures[index];
}

Vector3d BreaklineCurve::get_tangent_at(size_t index) const {
    if (index >= points.size() - 1) return Vector3d::Zero();

    Vector3d tangent = points[index + 1] - points[index];
    tangent.normalize();
    return tangent;
}

bool BreaklineCurve::is_compatible_with(const BreaklineCurve& other, double tolerance) const {
    // Check if two breakline curves can form a continuous pottery edge

    if (points.empty() || other.points.empty()) return false;

    // Check endpoint proximity (curves should connect)
    double min_endpoint_distance = numeric_limits<double>::max();

    vector<Vector3d> my_endpoints = {points.front(), points.back()};
    vector<Vector3d> other_endpoints = {other.points.front(), other.points.back()};

    for (const auto& my_end : my_endpoints) {
        for (const auto& other_end : other_endpoints) {
            double distance = (my_end - other_end).norm();
            min_endpoint_distance = min(min_endpoint_distance, distance);
        }
    }

    // Curves are compatible if endpoints are close enough
    return min_endpoint_distance <= tolerance * 100.0; // Scale tolerance for curve matching
}