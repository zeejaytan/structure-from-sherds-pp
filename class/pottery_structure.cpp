#include "pottery_structure.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>

// ===============================================================================
// RimModel Implementation
// ===============================================================================

double RimModel::rimHeightResidual(const Eigen::Vector3d& point) const {
    // Project point onto vessel axis to get height coordinate
    double h = axis.dot(point - center);
    return std::abs(h - height);
}

double RimModel::rimRadiusResidual(const Eigen::Vector3d& point) const {
    // Get height and compute expected radius (constant or linear gradient)
    double h = axis.dot(point - center);
    double expected_radius = radius + radius_gradient * (h - height);

    // Compute radial distance from axis
    Eigen::Vector3d to_point = point - center;
    Eigen::Vector3d radial = to_point - (axis.dot(to_point)) * axis;
    double actual_radius = radial.norm();

    return std::abs(actual_radius - expected_radius);
}

// ===============================================================================
// BaseModel Implementation
// ===============================================================================

double BaseModel::basePlaneResidual(const Eigen::Vector3d& point) const {
    return std::abs(normal.dot(point) - plane_d);
}

double BaseModel::baseRingResidual(const Eigen::Vector3d& point) const {
    if (ring_radius <= 0.0) return 0.0; // No ring detected

    // Project point onto base plane, compute distance to ring center
    Eigen::Vector3d projected = point - normal * (normal.dot(point) - plane_d);
    double distance_to_center = (projected - ring_center).norm();
    return std::abs(distance_to_center - ring_radius);
}

bool BaseModel::isAboveBase(const Eigen::Vector3d& point) const {
    return normal.dot(point) >= (plane_d - 1.0); // 1mm tolerance
}

// ===============================================================================
// PotteryStructureDetector Implementation
// ===============================================================================

PotteryStructureDetector::PotteryStructureDetector() {
    // Initialize with invalid models
    rim_model_.confidence = 0.0;
    base_model_.confidence = 0.0;
}

void PotteryStructureDetector::analyzeVesselStructure(const std::vector<Geom>& pieces) {
    std::cout << "🏺 POTTERY STRUCTURE DETECTION: Analyzing vessel geometry..." << std::endl;

    // Step 1: Extract pottery edges (use existing breakline detection)
    pottery_edges_ = extractPotteryEdges(pieces);
    std::cout << "   Found " << pottery_edges_.size() << " pottery edges" << std::endl;

    // Step 2: Get vessel axis (assume it's already computed from axis estimation)
    // For now, use vertical axis - in production this would come from existing axis estimation
    vessel_axis_ = Eigen::Vector3d(0, 0, 1);

    // Step 3: Score each edge for rim/base likelihood
    for (auto& edge : pottery_edges_) {
        scoreEdgeLikelihood(edge, vessel_axis_);
    }

    // Step 4: Build consensus models
    rim_model_ = buildRimConsensus(pottery_edges_, vessel_axis_);
    base_model_ = buildBaseConsensus(pottery_edges_);

    std::cout << "   Rim model confidence: " << rim_model_.confidence << std::endl;
    std::cout << "   Base model confidence: " << base_model_.confidence << std::endl;
}

std::vector<EdgeInfo> PotteryStructureDetector::extractPotteryEdges(const std::vector<Geom>& pieces) {
    std::vector<EdgeInfo> edges;

    for (int i = 0; i < pieces.size(); ++i) {
        const auto& piece = pieces[i];

        // Extract boundary points from edge lines (use existing breakline data)
        const auto& breakline = piece.edge_line_; // Use edge line for pottery structure analysis
        if (breakline.point_.cols() < 10) continue;

        EdgeInfo edge;
        edge.piece_id = i;

        // Sample points from the breakline (edge data for pottery analysis)
        int num_points = std::min(50, (int)breakline.point_.cols());
        for (int j = 0; j < num_points; j += 2) {
            if (j < breakline.point_.cols()) {
                Eigen::Vector3d point = breakline.point_.col(j);
                edge.points.push_back(point);

                if (j < breakline.normal_.cols()) {
                    Eigen::Vector3d normal = breakline.normal_.col(j);
                    edge.normals.push_back(normal);
                }
            }
        }

        if (edge.points.size() >= 5) {  // Need minimum points for analysis
            edges.push_back(edge);
        }
    }

    return edges;
}

void PotteryStructureDetector::scoreEdgeLikelihood(EdgeInfo& edge, const Eigen::Vector3d& axis) {
    // Compute rim likelihood
    edge.rim_likelihood = computeRimLikelihood(edge, axis);

    // Compute base likelihood
    edge.base_likelihood = computeBaseLikelihood(edge);

    // Store geometric properties for debugging
    edge.height_variance = evaluateHeightPlanarity(edge.points, axis);

    // Compute average height for circularity test
    std::vector<double> heights;
    for (const auto& p : edge.points) {
        heights.push_back(axis.dot(p));
    }
    double avg_height = std::accumulate(heights.begin(), heights.end(), 0.0) / heights.size();
    edge.circularity_error = evaluateCircularity(edge.points, axis, avg_height);

    if (!edge.normals.empty()) {
        edge.tangency_error = evaluateRimTangency(edge.points, edge.normals, axis);
    }
}

double PotteryStructureDetector::computeRimLikelihood(const EdgeInfo& edge, const Eigen::Vector3d& axis) {
    double height_score = evaluateHeightPlanarity(edge.points, axis);

    // Get average height for this edge
    std::vector<double> heights;
    for (const auto& p : edge.points) {
        heights.push_back(axis.dot(p));
    }
    double avg_height = std::accumulate(heights.begin(), heights.end(), 0.0) / heights.size();

    double circularity_score = evaluateCircularity(edge.points, axis, avg_height);
    double tangency_score = edge.normals.empty() ? 0.5 :
                           evaluateRimTangency(edge.points, edge.normals, axis);

    // Convert errors to scores (0 = bad, 1 = good)
    double height_confidence = std::exp(-height_score / config_.rim_height_tolerance);
    double circularity_confidence = std::exp(-circularity_score / config_.rim_circularity_tolerance);
    double tangency_confidence = std::exp(-tangency_score / config_.rim_tangency_tolerance);

    // Weighted average
    return (height_confidence * 0.4 + circularity_confidence * 0.4 + tangency_confidence * 0.2);
}

double PotteryStructureDetector::evaluateHeightPlanarity(const std::vector<Eigen::Vector3d>& points,
                                                        const Eigen::Vector3d& axis) {
    std::vector<double> heights;
    for (const auto& p : points) {
        heights.push_back(axis.dot(p));
    }

    double mean_height = std::accumulate(heights.begin(), heights.end(), 0.0) / heights.size();
    double variance = 0.0;
    for (double h : heights) {
        variance += (h - mean_height) * (h - mean_height);
    }
    variance /= heights.size();

    return std::sqrt(variance); // Return standard deviation in mm
}

double PotteryStructureDetector::evaluateCircularity(const std::vector<Eigen::Vector3d>& points,
                                                     const Eigen::Vector3d& axis, double height) {
    // Project points onto plane perpendicular to axis at given height
    Eigen::Vector3d plane_center = axis * height;

    // Find best-fit circle center in the plane
    Eigen::Vector3d center_2d = Eigen::Vector3d::Zero();
    std::vector<Eigen::Vector3d> projected_points;

    for (const auto& p : points) {
        // Project onto plane perpendicular to axis
        Eigen::Vector3d projected = p - axis * (axis.dot(p) - height);
        projected_points.push_back(projected);
        center_2d += projected;
    }
    center_2d /= projected_points.size();

    // Compute radius as average distance to center
    double avg_radius = 0.0;
    for (const auto& p : projected_points) {
        avg_radius += (p - center_2d).norm();
    }
    avg_radius /= projected_points.size();

    // Compute circularity error as std deviation from average radius
    double error = 0.0;
    for (const auto& p : projected_points) {
        double r = (p - center_2d).norm();
        error += (r - avg_radius) * (r - avg_radius);
    }
    error /= projected_points.size();

    return std::sqrt(error); // Return RMS error in mm
}

double PotteryStructureDetector::evaluateRimTangency(const std::vector<Eigen::Vector3d>& points,
                                                     const std::vector<Eigen::Vector3d>& normals,
                                                     const Eigen::Vector3d& axis) {
    if (normals.size() != points.size()) return 1.0; // High error if no normals

    double tangency_error = 0.0;
    for (size_t i = 0; i < normals.size(); ++i) {
        // Rim normals should be roughly radial (perpendicular to axis)
        double axial_component = std::abs(normals[i].dot(axis));
        tangency_error += axial_component; // Should be near 0 for radial normals
    }

    return tangency_error / normals.size();
}

double PotteryStructureDetector::computeBaseLikelihood(const EdgeInfo& edge) {
    double planarity_score = evaluateBasePlanarity(edge.points);
    double height_score = evaluateBaseHeight(edge.points, vessel_axis_);

    // Convert errors to confidences
    double planarity_confidence = std::exp(-planarity_score / config_.base_planarity_tolerance);
    double height_confidence = height_score; // Already a confidence score

    return (planarity_confidence * 0.7 + height_confidence * 0.3);
}

double PotteryStructureDetector::evaluateBasePlanarity(const std::vector<Eigen::Vector3d>& points) {
    if (points.size() < 3) return 1000.0; // High error

    // Fit plane using SVD
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& p : points) {
        centroid += p;
    }
    centroid /= points.size();

    // Build covariance matrix
    Eigen::Matrix3d cov = Eigen::Matrix3d::Zero();
    for (const auto& p : points) {
        Eigen::Vector3d centered = p - centroid;
        cov += centered * centered.transpose();
    }

    // Find plane normal as eigenvector with smallest eigenvalue
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(cov);
    Eigen::Vector3d normal = solver.eigenvectors().col(0);

    // Compute RMS distance to plane
    double plane_d = normal.dot(centroid);
    double error = 0.0;
    for (const auto& p : points) {
        double dist = std::abs(normal.dot(p) - plane_d);
        error += dist * dist;
    }
    error /= points.size();

    return std::sqrt(error);
}

double PotteryStructureDetector::evaluateBaseHeight(const std::vector<Eigen::Vector3d>& points,
                                                   const Eigen::Vector3d& axis) {
    // Base pieces should be at low heights - compute quantile score
    std::vector<double> heights;
    for (const auto& p : points) {
        heights.push_back(axis.dot(p));
    }

    std::sort(heights.begin(), heights.end());
    double median_height = heights[heights.size() / 2];

    // This is a placeholder - in production, compare against global height distribution
    // For now, just return moderate confidence
    return 0.6;
}

RimModel PotteryStructureDetector::buildRimConsensus(const std::vector<EdgeInfo>& edges,
                                                     const Eigen::Vector3d& axis) {
    RimModel model;
    model.axis = axis;
    model.confidence = 0.0;

    // Find edges with high rim likelihood
    std::vector<const EdgeInfo*> rim_candidates;
    for (const auto& edge : edges) {
        if (edge.rim_likelihood > config_.min_edge_confidence) {
            rim_candidates.push_back(&edge);
        }
    }

    if (rim_candidates.size() < 2) {
        std::cout << "   Insufficient rim candidates for consensus" << std::endl;
        return model; // Low confidence
    }

    // Compute consensus rim height
    std::vector<double> rim_heights;
    std::vector<Eigen::Vector3d> rim_points;

    for (const auto* edge : rim_candidates) {
        for (const auto& p : edge->points) {
            double h = axis.dot(p);
            rim_heights.push_back(h);
            rim_points.push_back(p);
        }
    }

    std::sort(rim_heights.begin(), rim_heights.end());
    model.height = rim_heights[rim_heights.size() / 2]; // Median height

    // Compute consensus rim center and radius at rim height
    Eigen::Vector3d rim_center = Eigen::Vector3d::Zero();
    std::vector<Eigen::Vector3d> rim_plane_points;

    for (const auto& p : rim_points) {
        double h = axis.dot(p);
        if (std::abs(h - model.height) < config_.rim_height_tolerance) {
            // Project onto plane at rim height
            Eigen::Vector3d projected = p - axis * (h - model.height);
            rim_plane_points.push_back(projected);
            rim_center += projected;
        }
    }

    if (rim_plane_points.size() < 5) {
        return model; // Not enough points for reliable consensus
    }

    rim_center /= rim_plane_points.size();
    model.center = rim_center;

    // Compute average radius
    double avg_radius = 0.0;
    for (const auto& p : rim_plane_points) {
        Eigen::Vector3d radial = p - rim_center;
        radial = radial - axis * axis.dot(radial); // Remove axial component
        avg_radius += radial.norm();
    }
    model.radius = avg_radius / rim_plane_points.size();
    model.radius_gradient = 0.0; // Start with cylindrical assumption

    // Compute confidence based on consensus quality
    double radius_variance = 0.0;
    for (const auto& p : rim_plane_points) {
        Eigen::Vector3d radial = p - rim_center;
        radial = radial - axis * axis.dot(radial);
        double r = radial.norm();
        radius_variance += (r - model.radius) * (r - model.radius);
    }
    radius_variance /= rim_plane_points.size();

    model.confidence = std::exp(-std::sqrt(radius_variance) / config_.rim_circularity_tolerance);
    model.confidence = std::min(1.0, model.confidence);

    return model;
}

BaseModel PotteryStructureDetector::buildBaseConsensus(const std::vector<EdgeInfo>& edges) {
    BaseModel model;
    model.confidence = 0.0;
    model.ring_radius = 0.0; // No ring initially

    // Find edges with high base likelihood
    std::vector<const EdgeInfo*> base_candidates;
    for (const auto& edge : edges) {
        if (edge.base_likelihood > config_.min_edge_confidence) {
            base_candidates.push_back(&edge);
        }
    }

    if (base_candidates.empty()) {
        return model; // No base detected
    }

    // Collect all base points
    std::vector<Eigen::Vector3d> base_points;
    for (const auto* edge : base_candidates) {
        for (const auto& p : edge->points) {
            base_points.push_back(p);
        }
    }

    if (base_points.size() < 6) {
        return model; // Not enough points
    }

    // Fit plane to base points using SVD
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& p : base_points) {
        centroid += p;
    }
    centroid /= base_points.size();

    Eigen::Matrix3d cov = Eigen::Matrix3d::Zero();
    for (const auto& p : base_points) {
        Eigen::Vector3d centered = p - centroid;
        cov += centered * centered.transpose();
    }

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(cov);
    model.normal = solver.eigenvectors().col(0);
    model.plane_d = model.normal.dot(centroid);

    // Compute confidence based on planarity
    double planarity_error = 0.0;
    for (const auto& p : base_points) {
        double dist = std::abs(model.normal.dot(p) - model.plane_d);
        planarity_error += dist * dist;
    }
    planarity_error = std::sqrt(planarity_error / base_points.size());

    model.confidence = std::exp(-planarity_error / config_.base_planarity_tolerance);
    model.confidence = std::min(1.0, model.confidence);

    return model;
}

double PotteryStructureDetector::evaluatePotteryConstraints(int piece_a, int piece_b,
                                                          const GlobalProblem& problem,
                                                          int hypothesis_idx) {
    double constraint_penalty = 0.0;

    if (!hasValidRim() && !hasValidBase()) {
        return 0.0; // No pottery constraints available
    }

    // Get transformation for this hypothesis
    const auto& hyp = problem.hypotheses[hypothesis_idx];
    Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();
    hyp.icp_transformation.Output(transform);

    // Apply transformation to piece points and evaluate constraints
    // This is a simplified version - in production would evaluate all boundary points

    if (hasValidRim()) {
        // Check rim constraint violations
        // For rim pieces, transformed points should satisfy rim height and radius
        for (const auto& edge : pottery_edges_) {
            if (edge.piece_id == piece_a || edge.piece_id == piece_b) {
                if (edge.rim_likelihood > 0.7) {
                    // High confidence rim edge - evaluate constraints
                    for (const auto& p : edge.points) {
                        Eigen::Vector4d p_homo(p.x(), p.y(), p.z(), 1.0);
                        Eigen::Vector3d p_transformed = (transform * p_homo).head<3>();

                        double height_error = rim_model_.rimHeightResidual(p_transformed);
                        double radius_error = rim_model_.rimRadiusResidual(p_transformed);

                        constraint_penalty += (height_error / config_.rim_height_tolerance) +
                                            (radius_error / config_.rim_circularity_tolerance);
                    }
                }
            }
        }
    }

    if (hasValidBase()) {
        // Check base constraint violations
        for (const auto& edge : pottery_edges_) {
            if (edge.piece_id == piece_a || edge.piece_id == piece_b) {
                if (edge.base_likelihood > 0.7) {
                    // High confidence base edge - evaluate constraints
                    for (const auto& p : edge.points) {
                        Eigen::Vector4d p_homo(p.x(), p.y(), p.z(), 1.0);
                        Eigen::Vector3d p_transformed = (transform * p_homo).head<3>();

                        double plane_error = base_model_.basePlaneResidual(p_transformed);
                        constraint_penalty += plane_error / config_.base_planarity_tolerance;

                        // Penalty for points below base (impossible)
                        if (!base_model_.isAboveBase(p_transformed)) {
                            constraint_penalty += 10.0; // Large penalty for impossible geometry
                        }
                    }
                }
            }
        }
    }

    return constraint_penalty;
}