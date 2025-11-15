#ifndef POTTERY_GEOMETRIC_VALIDATOR_H
#define POTTERY_GEOMETRIC_VALIDATOR_H

#include <vector>
#include <Eigen/Dense>
#include <memory>

using namespace Eigen;

/**
 * Pottery-Aware Geometric Validation Framework
 *
 * Replaces generic surface normal thresholds with pottery-specific
 * geometric constraints that understand vessel structure, axis alignment,
 * and archaeological breakage patterns.
 */

struct PotteryAxisData {
    Vector3d position;    // 3D position on vessel axis
    Vector3d direction;   // Axis direction vector (normalized)
    double height;        // Z-coordinate (vessel height level)

    PotteryAxisData(double x, double y, double z, double dx, double dy, double dz)
        : position(x, y, z), direction(dx, dy, dz), height(z) {
        direction.normalize();
    }
};

struct BreaklineCurve {
    std::vector<Vector3d> points;
    std::vector<Vector3d> normals;
    std::vector<double> curvatures;
    double total_length;

    // Curve analysis methods
    double get_curvature_at(size_t index) const;
    Vector3d get_tangent_at(size_t index) const;
    bool is_compatible_with(const BreaklineCurve& other, double tolerance = 0.1) const;
};

struct PotteryPieceData {
    std::vector<PotteryAxisData> axes;
    std::vector<BreaklineCurve> breaklines;
    Vector3d center_of_mass;
    double average_height;
    double radial_position;  // Distance from vessel center in XY plane
    double angular_position; // Angle around vessel axis

    // Pottery-specific geometric queries
    Vector3d get_primary_axis_direction() const;
    double get_height_range() const;
    Vector2d get_radial_center() const;
    bool is_rim_piece() const;
    bool is_base_piece() const;
};

struct ConnectionValidationResult {
    double pottery_validity_score;    // [0,1] pottery structural validity
    double surface_continuity_score;  // [0,1] surface geometric quality
    double combined_score;            // pottery_validity * surface_continuity
    bool is_pottery_valid;           // pottery_validity > threshold

    // Detailed breakdown
    double axis_alignment_score;
    double height_compatibility_score;
    double radial_proximity_score;
    double breakline_compatibility_score;
    double curvature_continuity_score;

    std::string rejection_reason;
};

class PotteryGeometricValidator {
public:
    // Configuration parameters
    struct Config {
        // Pottery structure thresholds
        double pottery_validity_threshold = 0.6;
        double max_axis_angle_degrees = 45.0;
        double same_level_height_threshold = 50.0;   // mm
        double adjacent_level_height_threshold = 150.0; // mm
        double max_radial_distance = 200.0;          // mm
        double max_angular_separation = 60.0;        // degrees

        // Surface continuity parameters
        double min_icp_inliers = 15;
        double surface_continuity_weight = 0.7;
        double pottery_structure_weight = 1.0;

        // Breakline compatibility
        double breakline_curve_tolerance = 0.15;
        double curvature_matching_tolerance = 0.2;

        // Pottery-specific normal validation (replaces rigid 0.65 threshold)
        bool enable_pottery_normal_validation = true;
        double rim_body_normal_tolerance = 0.3;      // Expect large normal changes
        double circumferential_normal_tolerance = 0.5; // Moderate changes
        double standard_normal_tolerance = 0.7;      // Similar to current but relaxed
    };

    explicit PotteryGeometricValidator(const Config& config = Config{});

    /**
     * Primary validation method - replaces normal threshold filtering
     *
     * @param piece1_data Pottery-specific data for first piece
     * @param piece2_data Pottery-specific data for second piece
     * @param icp_inliers Number of ICP inliers found
     * @param correspondence_quality Quality of point correspondences
     * @return Complete validation result with detailed scoring
     */
    ConnectionValidationResult validate_pottery_connection(
        const PotteryPieceData& piece1_data,
        const PotteryPieceData& piece2_data,
        int icp_inliers,
        double correspondence_quality
    ) const;

    /**
     * Pottery-aware normal validation - replaces rigid normal threshold
     * Understands that pottery connections SHOULD have normal differences
     */
    double validate_pottery_normals(
        const std::vector<Vector3d>& normals1,
        const std::vector<Vector3d>& normals2,
        const PotteryPieceData& piece1,
        const PotteryPieceData& piece2
    ) const;

private:
    Config config_;

    // Core pottery validation algorithms
    double calculate_axis_alignment_score(
        const PotteryPieceData& p1,
        const PotteryPieceData& p2
    ) const;

    double calculate_height_compatibility_score(
        const PotteryPieceData& p1,
        const PotteryPieceData& p2
    ) const;

    double calculate_radial_proximity_score(
        const PotteryPieceData& p1,
        const PotteryPieceData& p2
    ) const;

    double calculate_breakline_compatibility_score(
        const PotteryPieceData& p1,
        const PotteryPieceData& p2
    ) const;

    double calculate_surface_continuity_score(
        int icp_inliers,
        double correspondence_quality
    ) const;

    // Pottery context analysis
    bool is_rim_body_transition(
        const PotteryPieceData& p1,
        const PotteryPieceData& p2
    ) const;

    bool is_circumferential_connection(
        const PotteryPieceData& p1,
        const PotteryPieceData& p2
    ) const;

    bool is_vertical_adjacency(
        const PotteryPieceData& p1,
        const PotteryPieceData& p2
    ) const;

    // Utility methods
    double gaussian_falloff(double value, double max_value) const;
    double angular_distance(double angle1, double angle2) const;
    Vector2d project_to_vessel_plane(const Vector3d& point) const;
};

/**
 * Integration class for existing SFS pipeline
 * Replaces current outlier rejection logic with pottery-aware validation
 */
class PotteryOutlierRejector {
public:
    explicit PotteryOutlierRejector(const PotteryGeometricValidator::Config& config = {});

    /**
     * Replaces current outlier rejection in pairwise pruning
     *
     * @param piece1_id Piece identifier
     * @param piece2_id Piece identifier
     * @param correspondences Point correspondences from ICP
     * @param piece_data_map Map of piece ID to pottery data
     * @return Filtered correspondences and validation result
     */
    std::pair<std::vector<std::pair<Vector3d, Vector3d>>, ConnectionValidationResult>
    filter_correspondences_pottery_aware(
        int piece1_id,
        int piece2_id,
        const std::vector<std::pair<Vector3d, Vector3d>>& correspondences,
        const std::map<int, PotteryPieceData>& piece_data_map
    ) const;

private:
    PotteryGeometricValidator validator_;

    // Extract pottery data from existing piece structures
    PotteryPieceData extract_pottery_data_from_piece(int piece_id) const;

    // Calculate surface normals and correspondence quality
    double calculate_correspondence_quality(
        const std::vector<std::pair<Vector3d, Vector3d>>& correspondences
    ) const;
};

#endif // POTTERY_GEOMETRIC_VALIDATOR_H