#ifndef POTTERY_STRUCTURE_H
#define POTTERY_STRUCTURE_H

#include <vector>
#include <Eigen/Dense>
#include "data_structure.h"
#include "multi_hypothesis_optimizer.h"

/**
 * POTTERY-SPECIFIC STRUCTURE DETECTION AND CONSTRAINTS
 * ===================================================
 *
 * Implements rim/base detection and vessel-level constraints to naturally
 * limit connections to pottery-realistic assemblies (~15 connections instead of 27).
 *
 * Key insight: Use pottery domain knowledge to reject impossible connections
 * through vessel structure constraints, not arbitrary limits.
 */

struct RimModel {
    Eigen::Vector3d axis;           // Vessel symmetry axis
    Eigen::Vector3d center;         // Rim center point
    double radius;                  // Rim radius (or R0 for conical)
    double height;                  // Rim height (h_rim)
    double radius_gradient;         // dR/dh for conical rims (0 for cylindrical)
    double confidence;              // Consensus confidence [0,1]

    // Check if point should lie on rim circle
    double rimHeightResidual(const Eigen::Vector3d& point) const;
    double rimRadiusResidual(const Eigen::Vector3d& point) const;
};

struct BaseModel {
    Eigen::Vector3d normal;         // Base plane normal
    double plane_d;                 // Base plane distance: n^T * x = d
    Eigen::Vector3d ring_center;    // Foot ring center (optional)
    double ring_radius;             // Foot ring radius (0 if no ring)
    double confidence;              // Consensus confidence [0,1]

    // Check if point lies on base plane
    double basePlaneResidual(const Eigen::Vector3d& point) const;
    double baseRingResidual(const Eigen::Vector3d& point) const;
    bool isAboveBase(const Eigen::Vector3d& point) const;
};

struct EdgeInfo {
    int piece_id;
    std::vector<Eigen::Vector3d> points;
    std::vector<Eigen::Vector3d> normals;
    double rim_likelihood;          // [0,1] confidence this is rim edge
    double base_likelihood;         // [0,1] confidence this is base edge

    // Edge geometric properties
    double height_variance;         // std(height) - rims should be flat
    double circularity_error;       // distance to best-fit circle
    double tangency_error;          // deviation from horizontal tangent
};

class PotteryStructureDetector {
public:
    PotteryStructureDetector();

    // Main detection pipeline - uses existing geometry data
    void analyzeVesselStructure(const std::vector<Geom>& pieces);

    // Step 1: Extract edges using existing breakline detection
    std::vector<EdgeInfo> extractPotteryEdges(const std::vector<Geom>& pieces);

    // Step 2: Score each edge for rim/base likelihood
    void scoreEdgeLikelihood(EdgeInfo& edge, const Eigen::Vector3d& vessel_axis);

    // Step 3: Build vessel-level consensus models
    RimModel buildRimConsensus(const std::vector<EdgeInfo>& edges,
                              const Eigen::Vector3d& axis);
    BaseModel buildBaseConsensus(const std::vector<EdgeInfo>& edges);

    // Step 4: Evaluate pottery constraints for connection validation
    double evaluatePotteryConstraints(int piece_a, int piece_b,
                                     const GlobalProblem& problem,
                                     int hypothesis_idx);

    // Access detected structure
    const RimModel& getRimModel() const { return rim_model_; }
    const BaseModel& getBaseModel() const { return base_model_; }
    bool hasValidRim() const { return rim_model_.confidence > 0.5; }
    bool hasValidBase() const { return base_model_.confidence > 0.5; }

private:
    RimModel rim_model_;
    BaseModel base_model_;
    std::vector<EdgeInfo> pottery_edges_;
    Eigen::Vector3d vessel_axis_;

    // Rim detection helpers
    double computeRimLikelihood(const EdgeInfo& edge, const Eigen::Vector3d& axis);
    double evaluateHeightPlanarity(const std::vector<Eigen::Vector3d>& points,
                                  const Eigen::Vector3d& axis);
    double evaluateCircularity(const std::vector<Eigen::Vector3d>& points,
                              const Eigen::Vector3d& axis, double height);
    double evaluateRimTangency(const std::vector<Eigen::Vector3d>& points,
                              const std::vector<Eigen::Vector3d>& normals,
                              const Eigen::Vector3d& axis);

    // Base detection helpers
    double computeBaseLikelihood(const EdgeInfo& edge);
    double evaluateBasePlanarity(const std::vector<Eigen::Vector3d>& points);
    double evaluateBaseHeight(const std::vector<Eigen::Vector3d>& points,
                             const Eigen::Vector3d& axis);

    // Configuration
    struct Config {
        double rim_height_tolerance = 2.0;      // mm - rim should be coplanar
        double rim_circularity_tolerance = 3.0; // mm - distance to circle
        double base_planarity_tolerance = 2.0;  // mm - base coplanarity
        double min_edge_confidence = 0.6;       // threshold for consensus
        double rim_tangency_tolerance = 0.2;    // radians - tangent angle
    };
    Config config_;
};

#endif // POTTERY_STRUCTURE_H