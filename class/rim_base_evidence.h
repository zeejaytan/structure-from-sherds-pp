#pragma once
#ifndef _RIM_BASE_EVIDENCE_H_
#define _RIM_BASE_EVIDENCE_H_

#include <Eigen/Dense>
#include <vector>
#include <string>

// Forward declaration
struct ArchaeologicalConfig;

/**
 * Evidence from primitive geometric fitting (circle for rim, plane for base)
 * Used to validate that a piece's edge geometry matches expected pottery structure
 */
struct PrimitiveEvidence {
    double rms_error;           // mm - RMS fit residual
    double inlier_ratio;        // [0,1] - Fraction of points within threshold
    double angular_coverage;    // [0,1] - Fraction of circle/annulus covered
    Eigen::Vector3d center;     // 3D center position
    Eigen::Vector3d normal;     // Normal vector (axis for rim, plane normal for base)
    double radius;              // mm - fitted radius (rim circle or base circle)
    bool is_valid;              // Overall validity flag

    PrimitiveEvidence() :
        rms_error(999.0),
        inlier_ratio(0.0),
        angular_coverage(0.0),
        center(Eigen::Vector3d::Zero()),
        normal(Eigen::Vector3d::UnitZ()),
        radius(0.0),
        is_valid(false) {}
};

/**
 * Evidence from edge smoothness and tangency analysis
 * Rim edges should be smooth (low curvature) and perpendicular to vessel axis
 */
struct SmoothnessEvidence {
    double curvature_noise;     // Standard deviation of curvature values
    double axis_tangency_angle; // degrees - How perpendicular edge is to axis
    bool is_smooth;             // Passes smoothness threshold

    SmoothnessEvidence() :
        curvature_noise(999.0),
        axis_tangency_angle(90.0),
        is_smooth(false) {}
};

/**
 * Evidence from wall thickness trends along edge
 * Rims often thin toward lip, bases often thicken
 */
struct ThicknessTrendEvidence {
    double gradient_consistency; // Correlation of thickness along edge
    double mean_thickness;       // mm - Average wall thickness
    bool is_thinning;           // True if rim-like thinning detected
    bool is_thickening;         // True if base-like thickening detected

    ThicknessTrendEvidence() :
        gradient_consistency(0.0),
        mean_thickness(0.0),
        is_thinning(false),
        is_thickening(false) {}
};

/**
 * RimBaseValidator: Independent geometric validators for rim/base classification
 *
 * Implements PRD_Rimbase.md requirements:
 * - Primitive fitting (circle for rim, plane+annulus for base)
 * - Edge smoothness and tangency checks
 * - Thickness trend analysis
 * - Single-piece sufficiency detection
 *
 * Design: Each validator operates independently and returns structured evidence
 * that can be fused with height-based priors for final classification
 */
class RimBaseValidator {
public:
    RimBaseValidator() = default;
    ~RimBaseValidator() = default;

    /**
     * Fit rim primitive (3D circle perpendicular to vessel axis)
     *
     * @param edge_points - Edge points in world coordinates
     * @param axis_normal - Vessel axis direction
     * @param height - Piece height on axis (for logging)
     * @param config - Archaeological configuration parameters
     * @return PrimitiveEvidence with circle fit quality metrics
     *
     * Uses Huber-weighted robust fitting (same as fitAndValidateCircle)
     * Projects points to plane perpendicular to axis, fits 2D circle, validates fit
     */
    PrimitiveEvidence fitRimPrimitive(
        const std::vector<Eigen::Vector3d>& edge_points,
        const Eigen::Vector3d& axis_normal,
        double height,
        const ArchaeologicalConfig& config) const;

    /**
     * Fit base primitive (plane + optional foot-ring annulus)
     *
     * @param edge_points - Edge points in world coordinates
     * @param axis_normal - Vessel axis direction
     * @param config - Archaeological configuration parameters
     * @param has_foot_ring - [OUT] Set to true if annulus detected
     * @return PrimitiveEvidence with plane/annulus fit quality metrics
     *
     * First fits plane (bases are typically flat)
     * Then attempts annulus fit (two concentric circles) for foot-ring detection
     * Accepts either good plane alone OR good plane+annulus
     */
    PrimitiveEvidence fitBasePrimitive(
        const std::vector<Eigen::Vector3d>& edge_points,
        const Eigen::Vector3d& axis_normal,
        const ArchaeologicalConfig& config,
        bool& has_foot_ring) const;

    /**
     * Analyze edge smoothness and tangency to vessel axis
     *
     * @param edge_points - Edge points in world coordinates
     * @param edge_normals - Surface normals at edge points
     * @param axis_normal - Vessel axis direction
     * @param config - Archaeological configuration parameters
     * @return SmoothnessEvidence with curvature and tangency metrics
     *
     * Rim edges should be:
     * - Smooth (low curvature variation) - not chipped/irregular
     * - Perpendicular to axis (tangent wraps around axis)
     */
    SmoothnessEvidence edgeSmoothnessAndTangency(
        const std::vector<Eigen::Vector3d>& edge_points,
        const std::vector<Eigen::Vector3d>& edge_normals,
        const Eigen::Vector3d& axis_normal,
        const ArchaeologicalConfig& config) const;

    /**
     * Analyze thickness trend along edge
     *
     * @param thickness_values - Wall thickness samples along edge
     * @param config - Archaeological configuration parameters
     * @return ThicknessTrendEvidence with trend direction and consistency
     *
     * Pottery-specific trends:
     * - Rims: Often thin toward lip (negative gradient)
     * - Bases: Often thicken (positive gradient)
     * - Uses Pearson correlation to measure trend consistency
     */
    ThicknessTrendEvidence thicknessTrend(
        const std::vector<double>& thickness_values,
        const ArchaeologicalConfig& config) const;

    /**
     * Check if single piece is sufficient (80%+ coverage)
     *
     * @param primitive - Primitive fit evidence
     * @param config - Archaeological configuration parameters
     * @param is_rim - True for rim check, false for base check
     * @return True if piece covers 80%+ with good fit quality
     *
     * PRD requirement: Detect when one fragment covers most of rim/base
     * Prevents unnecessary ring assembly attempts on complete structures
     */
    bool checkSinglePieceSufficiency(
        const PrimitiveEvidence& primitive,
        const ArchaeologicalConfig& config,
        bool is_rim) const;

private:
    /**
     * Fit 2D circle to projected points using Huber-weighted least squares
     * Returns center (cx, cy) and radius in 2D plane coordinate system
     */
    void fitCircle2DHuber(
        const std::vector<Eigen::Vector2d>& points,
        double& cx, double& cy, double& radius,
        double& rms_error, double& inlier_ratio,
        const ArchaeologicalConfig& config) const;

    /**
     * Fit plane to 3D points using Huber-weighted least squares
     * Returns plane normal and distance from origin
     */
    void fitPlaneHuber(
        const std::vector<Eigen::Vector3d>& points,
        Eigen::Vector3d& normal, double& distance,
        double& rms_error, double& inlier_ratio,
        const ArchaeologicalConfig& config) const;

    /**
     * Compute Pearson correlation coefficient
     */
    double computeCorrelation(
        const std::vector<double>& x,
        const std::vector<double>& y) const;

    /**
     * Compute angular coverage of points around circle with gap detection
     * Returns fraction [0,1] of circle perimeter covered
     *
     * FIX (2025-10-18): Gap-aware calculation to prevent false sufficiency
     * detection. Detects large gaps between consecutive points to avoid
     * counting empty sections as "covered".
     *
     * @param points - Projected 2D points around circle center
     * @param cx, cy - Circle center in 2D
     * @param gap_threshold_deg - Maximum angular gap (degrees) to consider arc continuous
     * @return Actual coverage fraction [0,1], excluding gaps > threshold
     */
    double computeAngularCoverage(
        const std::vector<Eigen::Vector2d>& points,
        double cx, double cy,
        double gap_threshold_deg = 30.0) const;
};

#endif // _RIM_BASE_EVIDENCE_H_
