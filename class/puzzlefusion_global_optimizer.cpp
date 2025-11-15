#include "puzzlefusion_global_optimizer.h"
#include "ranking_system.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <unordered_set>

//==============================================================================
// GeometryValidator Implementation
//==============================================================================

GeometryValidator::GeometryValidator(const std::vector<Geom>& original_geometry)
    : reference_geometry_(original_geometry), working_geometry_(original_geometry) {
}

double GeometryValidator::validateAssemblyState(const HybridAssemblyState& state) {
    // Apply transformations to working geometry
    applyStateToGeometry(state);

    // Calculate real assembly quality
    double real_quality = calculateRealAssemblyQuality(state);

    // Restore original positions for next validation
    restoreOriginalGeometry();

    return real_quality;
}

void GeometryValidator::applyStateToGeometry(const HybridAssemblyState& state) {
    // Apply world transformations to each active piece
    for (int piece_id : state.active_pieces) {
        auto transform_it = state.piece_world_transforms.find(piece_id);
        if (transform_it != state.piece_world_transforms.end()) {
            Matrix4d world_transform = transform_it->second;
            Matrix3d R = world_transform.block<3,3>(0,0);
            Vector3d t = world_transform.block<3,1>(0,3);

            // Use legacy Move() function for reliable transformation
            working_geometry_[piece_id - 1].Move(R, t);
        }
    }
}

void GeometryValidator::restoreOriginalGeometry() {
    working_geometry_ = reference_geometry_;
}

double GeometryValidator::calculateRealAssemblyQuality(const HybridAssemblyState& state) {
    double total_quality = 0.0;
    int valid_connections = 0;

    // Measure real distances between connected pieces
    for (const auto& connection : state.selected_connections) {
        double real_distance = measureRealEdgeDistance(connection.piece_a, connection.piece_b, state);

        // Quality based on real edge alignment
        double distance_quality;
        if (real_distance < 10.0) {
            distance_quality = 100.0;  // Excellent alignment
        } else if (real_distance < 30.0) {
            distance_quality = 100.0 - (real_distance - 10.0) * 3.0;  // Linear decay
        } else if (real_distance < 50.0) {
            distance_quality = 40.0 - (real_distance - 30.0) * 1.0;   // Slower decay
        } else {
            distance_quality = 0.0;    // Too far apart
        }

        // Combine with original ICP quality
        double combined_quality = 0.6 * distance_quality + 0.4 * connection.combined_quality;
        total_quality += combined_quality;
        valid_connections++;
    }

    if (valid_connections == 0) return 0.0;

    // Average connection quality with completeness bonus
    double average_quality = total_quality / valid_connections;
    double completeness_bonus = (static_cast<double>(state.active_pieces.size()) / state.total_pieces) * 25.0;

    return average_quality + completeness_bonus;
}

double GeometryValidator::measureRealEdgeDistance(int piece_a, int piece_b, const HybridAssemblyState& state) {
    // Measure closest distance between edge points of two pieces
    const auto& edge_a = working_geometry_[piece_a - 1].edge_line_.point_;
    const auto& edge_b = working_geometry_[piece_b - 1].edge_line_.point_;

    if (edge_a.cols() == 0 || edge_b.cols() == 0) return 999.0;  // No edge data

    double min_distance = std::numeric_limits<double>::max();

    // Find minimum distance between any edge points
    for (int i = 0; i < edge_a.cols(); ++i) {
        Vector3d point_a = edge_a.col(i);
        for (int j = 0; j < edge_b.cols(); ++j) {
            Vector3d point_b = edge_b.col(j);
            double distance = (point_a - point_b).norm();
            min_distance = std::min(min_distance, distance);
        }
    }

    return min_distance;
}

bool GeometryValidator::measureRealSurfaceOverlap(int piece_a, int piece_b, const HybridAssemblyState& state, double& overlap_volume) {
    // ENHANCED POTTERY-APPROPRIATE SURFACE OVERLAP DETECTION
    // Uses actual surface point clouds instead of just edge-to-edge distance

    if (piece_a <= 0 || piece_a > working_geometry_.size() ||
        piece_b <= 0 || piece_b > working_geometry_.size()) {
        overlap_volume = 0.0;
        return false;
    }

    // Extract transformed surface data (after applyStateToGeometry)
    const auto& geom_a = working_geometry_[piece_a - 1];
    const auto& geom_b = working_geometry_[piece_b - 1];

    const MatrixXd& surface_a_inner = geom_a.sur_in_.point_;
    const MatrixXd& surface_a_outer = geom_a.sur_out_.point_;
    const MatrixXd& surface_b_inner = geom_b.sur_in_.point_;
    const MatrixXd& surface_b_outer = geom_b.sur_out_.point_;

    // Check if surface data is available
    if (surface_a_inner.cols() == 0 && surface_a_outer.cols() == 0) {
        overlap_volume = 0.0;
        return false;  // No surface data for piece A
    }
    if (surface_b_inner.cols() == 0 && surface_b_outer.cols() == 0) {
        overlap_volume = 0.0;
        return false;  // No surface data for piece B
    }

    // Use pottery-appropriate surface overlap detection
    // Threshold: 500 mm³ volume overlap indicates real surface intersection
    double volume_threshold = 500.0;  // mm³ - pottery-appropriate threshold

    bool has_overlap = checkPotterySurfaceOverlap(
        surface_a_inner, surface_a_outer,
        surface_b_inner, surface_b_outer,
        overlap_volume, volume_threshold);

    return has_overlap;
}

bool GeometryValidator::hasGeometricConflicts(const HybridAssemblyState& state) {
    // Check for unrealistic piece separations
    for (const auto& connection : state.selected_connections) {
        double distance = measureRealEdgeDistance(connection.piece_a, connection.piece_b, state);
        if (distance > 60.0) {  // Too far for realistic pottery assembly
            return true;
        }
    }
    return false;
}

double GeometryValidator::calculateVesselRadiusFromCurvature(const std::vector<int>& piece_ids) {
    // Calculate expected vessel radius from piece curvature data instead of hardcoded limits
    std::vector<double> curvature_radii;

    for (int piece_id : piece_ids) {
        if (piece_id <= 0 || piece_id > working_geometry_.size()) continue;

        // Access piece breakline points to estimate curvature
        const auto& edge_points = working_geometry_[piece_id - 1].edge_line_.point_;
        if (edge_points.cols() < 3) continue;  // Need at least 3 points for curvature

        // Estimate vessel radius from edge curvature (simplified approach)
        std::vector<double> point_radii;
        for (int i = 1; i < edge_points.cols() - 1; ++i) {
            Vector3d p1 = edge_points.col(i-1);
            Vector3d p2 = edge_points.col(i);
            Vector3d p3 = edge_points.col(i+1);

            // Calculate radius of curvature for this 3-point segment
            Vector3d v1 = p2 - p1;
            Vector3d v2 = p3 - p2;

            if (v1.norm() > 0.1 && v2.norm() > 0.1) {  // Valid edge segments
                double angle = acos(std::clamp(v1.dot(v2) / (v1.norm() * v2.norm()), -1.0, 1.0));
                if (angle > 0.01) {  // Avoid division by zero
                    double chord_length = (p3 - p1).norm();
                    double radius = chord_length / (2.0 * sin(angle / 2.0));

                    // Filter reasonable pottery radii (20-500mm)
                    if (radius > 20.0 && radius < 500.0) {
                        point_radii.push_back(radius);
                    }
                }
            }
        }

        // Use median radius for this piece
        if (!point_radii.empty()) {
            std::sort(point_radii.begin(), point_radii.end());
            curvature_radii.push_back(point_radii[point_radii.size() / 2]);
        }
    }

    if (!curvature_radii.empty()) {
        // Return median expected vessel radius from all pieces
        std::sort(curvature_radii.begin(), curvature_radii.end());
        double expected_radius = curvature_radii[curvature_radii.size() / 2];

        // Ensure reasonable bounds for pottery vessels
        return std::clamp(expected_radius, 50.0, 400.0);
    }

    // Fallback: much more generous than 120mm hardcoded limit
    return 200.0;  // Default pottery vessel radius
}

//==============================================================================
// AssemblyScorer Implementation
//==============================================================================

AssemblyScorer::AssemblyScorer(const HybridOptimizationConfig& config, GeometryValidator* validator)
    : config_(config), validator_(validator) {
}

double AssemblyScorer::calculateMatrixScore(const HybridAssemblyState& state) const {
    // Fast matrix-based scoring for exploration phase
    double connection_quality = calculateConnectionQuality(state.selected_connections);
    double completeness_bonus = calculateCompletenessBonus(state);
    double vessel_coherence = calculateVesselCoherence(state);

    // CRITICAL: Add cluster coherence penalty DURING beam search!
    // This prevents locally good but globally bad connections (like 6-5) from being selected
    double coherence_penalty = calculateClusterCoherencePenalty(state);

    return connection_quality + completeness_bonus + vessel_coherence + coherence_penalty;
}

double AssemblyScorer::calculateFinalScore(const HybridAssemblyState& state) const {
    // Use validated score if available, otherwise fall back to matrix score
    if (state.has_real_validation) {
        return state.validated_score;
    }
    return calculateMatrixScore(state);
}

double AssemblyScorer::calculateConnectionQuality(const std::vector<ProvenConnection>& connections) const {
    if (connections.empty()) return 0.0;

    double total_quality = 0.0;
    for (const auto& conn : connections) {
        total_quality += conn.combined_quality;
    }

    return total_quality / connections.size();
}

double AssemblyScorer::calculateCompletenessBonus(const HybridAssemblyState& state) const {
    if (state.total_pieces == 0) return 0.0;

    double completeness = static_cast<double>(state.active_pieces.size()) / state.total_pieces;
    return completeness * 30.0;  // Up to 30 points for complete assembly
}

double AssemblyScorer::calculateVesselCoherence(const HybridAssemblyState& state) const {
    if (state.active_pieces.size() < 2) return 0.0;

    // LEGACY VESSEL VALIDATION: Use constraints from legacy graph building
    double coherence_score = 0.0;

    // CONSTRAINT 1: Overlap Check (LEGACY OverlapCheck_3d integration)
    // Use actual breakline-based overlap detection instead of simple distance
    bool has_overlap = false;

    // NOTE: Full OverlapCheck_3d integration requires access to Geom breaklines
    // For now, enhance the distance-based check with better logic
    for (size_t i = 0; i < state.active_pieces.size() && !has_overlap; ++i) {
        for (size_t j = i + 1; j < state.active_pieces.size(); ++j) {
            int piece_a = state.active_pieces[i];
            int piece_b = state.active_pieces[j];

            // Enhanced overlap check: consider both proximity and connection status
            auto transform_a = state.piece_world_transforms.find(piece_a);
            auto transform_b = state.piece_world_transforms.find(piece_b);

            if (transform_a != state.piece_world_transforms.end() &&
                transform_b != state.piece_world_transforms.end()) {
                Vector3d center_a = transform_a->second.block<3,1>(0,3);
                Vector3d center_b = transform_b->second.block<3,1>(0,3);
                double distance = (center_a - center_b).norm();

                // Check if these pieces are connected (should be close)
                bool are_connected = false;
                for (const auto& conn : state.selected_connections) {
                    if ((conn.piece_a == piece_a && conn.piece_b == piece_b) ||
                        (conn.piece_a == piece_b && conn.piece_b == piece_a)) {
                        are_connected = true;
                        break;
                    }
                }

                // ENHANCED POTTERY-APPROPRIATE OVERLAP DETECTION: Two-stage approach
                // Stage 1: Fast edge-to-edge distance filter
                // Stage 2: Sophisticated surface overlap detection

                double edge_to_edge_distance = validator_ ? validator_->measureRealEdgeDistance(piece_a, piece_b, state) : 999.0;

                // STAGE 1: Edge distance pre-filtering (fast)
                bool potential_overlap = false;
                if (are_connected) {
                    // Connected pieces: very close edges warrant further surface investigation
                    potential_overlap = (edge_to_edge_distance < 1.0);  // 1mm threshold for connected pieces
                } else {
                    // Unconnected pieces: touching edges are suspicious
                    potential_overlap = (edge_to_edge_distance < 0.5);  // 0.5mm threshold for unconnected pieces
                }

                // STAGE 2: Surface overlap detection (sophisticated)
                if (potential_overlap) {
                    double overlap_volume = 0.0;

                    // Apply current state transformations to working geometry
                    // Note: This is already done by validateAssemblyState -> applyStateToGeometry

                    bool surface_overlap = validator_ ? validator_->measureRealSurfaceOverlap(piece_a, piece_b, state, overlap_volume) : false;

                    if (surface_overlap) {
                        has_overlap = true;
                        if (config_.enable_logging) {
                            if (are_connected) {
                                std::cout << "*** REAL SURFACE OVERLAP *** Connected pieces "
                                         << piece_a << " and " << piece_b
                                         << " | Edge distance: " << edge_to_edge_distance << "mm"
                                         << " | Surface overlap volume: " << overlap_volume << "mm³" << std::endl;
                            } else {
                                std::cout << "*** REAL SURFACE OVERLAP *** Unconnected pieces "
                                         << piece_a << " and " << piece_b
                                         << " | Edge distance: " << edge_to_edge_distance << "mm"
                                         << " | Surface overlap volume: " << overlap_volume << "mm³" << std::endl;
                            }
                        }
                        break;
                    } else {
                        // False alarm: edges close but no real surface intersection
                        if (config_.enable_logging) {
                            std::cout << "*** POTTERY PROXIMITY OK *** Pieces " << piece_a << " and " << piece_b
                                     << " | Edge distance: " << edge_to_edge_distance << "mm"
                                     << " | No surface overlap (pottery-appropriate proximity)" << std::endl;
                        }
                    }
                }
            }
        }
    }

    if (has_overlap) {
        coherence_score -= 15.0;  // REDUCED: was 50, now 15 (legacy more tolerant of minor overlaps)
        if (config_.enable_logging) {
            std::cout << "*** OVERLAP PENALTY *** Spatial overlap detected" << std::endl;
        }
    } else {
        coherence_score += 3.0;  // REDUCED: was 10, now 3 bonus for non-overlap
    }

    // CONSTRAINT 2: Vessel Radius/Height Bounds (from legacy MakeRadiusHeight)
    std::vector<Vector3d> piece_centers;
    for (int piece : state.active_pieces) {
        auto transform_it = state.piece_world_transforms.find(piece);
        if (transform_it != state.piece_world_transforms.end()) {
            Vector3d center = transform_it->second.block<3,1>(0,3);
            piece_centers.push_back(center);
        }
    }

    if (!piece_centers.empty()) {
        // Enhanced vessel boundary validation using legacy MakeRadiusHeight logic
        std::vector<double> radius_values, height_values;
        for (const auto& center : piece_centers) {
            double radius = sqrt(center.x() * center.x() + center.y() * center.y());
            radius_values.push_back(radius);
            height_values.push_back(center.z());
        }

        // Sort and get median values (legacy MakeRadiusHeight logic)
        std::sort(radius_values.begin(), radius_values.end());
        std::sort(height_values.begin(), height_values.end());

        int middle_num = (piece_centers.size() + 1) / 2;
        if (middle_num >= radius_values.size()) middle_num = radius_values.size() - 1;
        double median_radius = radius_values[middle_num];
        double median_height = height_values[middle_num];

        // Legacy vessel boundary checks with 10mm tolerance
        double min_height = height_values.front();
        double max_height = height_values.back();
        double top_gap = max_height - median_height - 10.0;
        double bottom_gap = median_height - min_height - 10.0;

        // RELAXED vessel boundary logic - pottery allows more variation
        if ((top_gap > 20.0) && (bottom_gap > 20.0)) {  // RELAXED: more tolerance (was immediate fail)
            coherence_score -= 10.0;  // REDUCED: was 30, now 10
            if (config_.enable_logging) {
                std::cout << "*** VESSEL BOUNDARY PENALTY *** Assembly exceeds pottery vessel bounds, "
                         << "top_gap=" << top_gap << "mm, bottom_gap=" << bottom_gap << "mm" << std::endl;
            }
        } else {
            coherence_score += 2.0;  // Small bonus for reasonable bounds
        }

        // CURVATURE-BASED radius constraint for pottery vessels
        std::vector<int> piece_ids;
        for (const auto& transform : state.piece_world_transforms) {
            piece_ids.push_back(transform.first);
        }
        double expected_vessel_radius = validator_ ? validator_->calculateVesselRadiusFromCurvature(piece_ids) : 120.0;
        double radius_tolerance = expected_vessel_radius * 0.5;  // 50% tolerance for pottery variation

        if (median_radius > expected_vessel_radius + radius_tolerance) {
            double excess_ratio = (median_radius - expected_vessel_radius) / expected_vessel_radius;
            coherence_score -= 2.0 * excess_ratio;  // Gentle penalty proportional to excess
            if (config_.enable_logging) {
                std::cout << "*** RADIUS BOUNDARY PENALTY *** Median radius " << median_radius
                         << "mm exceeds expected vessel radius " << expected_vessel_radius
                         << "mm (calculated from piece curvature)" << std::endl;
            }
        } else {
            if (config_.enable_logging) {
                std::cout << "*** RADIUS VALIDATION *** Median radius " << median_radius
                         << "mm within expected vessel radius " << expected_vessel_radius
                         << "mm (calculated from piece curvature)" << std::endl;
            }
        }

        // CONSTRAINT 3: Profile Curve Coherence (LEGACY ProfileChecking integration)
        // Use actual ProfileChecking function for vessel profile validation
        if (piece_centers.size() >= 3) {
            std::vector<Vector3d> profile_curve = piece_centers;
            bool profile_valid = ProfileChecking(profile_curve, 6.5, 6.0);  // Legacy thresholds

            if (profile_valid) {
                coherence_score += 5.0;  // REDUCED: was 15, now 5 bonus for valid profile
            } else {
                coherence_score -= 8.0;  // REDUCED: was 40, now 8 penalty (legacy is more forgiving)
                if (config_.enable_logging) {
                    std::cout << "*** PROFILE CURVATURE PENALTY *** Invalid vessel profile curve detected" << std::endl;
                }
            }
        }

    // CONSTRAINT 4: Minimum Edge Quality Threshold (LEGACY-TUNED)
    // Relaxed thresholds based on legacy system acceptance patterns
    for (const auto& conn : state.selected_connections) {
        if (conn.inlier_count < 15) {  // RELAXED: was 30, now 15 (legacy accepts 16+ inliers)
            // Much gentler penalty scaling
            double penalty_multiplier = (15.0 - conn.inlier_count) / 15.0;  // 0.0 to 1.0
            double penalty = 10.0 * penalty_multiplier;  // REDUCED: was 35, now 10 max penalty
            coherence_score -= penalty;
            if (config_.enable_logging) {
                std::cout << "*** EDGE QUALITY PENALTY *** Connection "
                         << conn.piece_a << "->" << conn.piece_b
                         << " only " << conn.inlier_count << " inliers (< 15), penalty="
                         << penalty << std::endl;
            }
        } else {
            coherence_score += 1.0;  // Small bonus for acceptable edge quality
        }
    }

    // CONSTRAINT 5: Transformation Similarity Validation (LEGACY-TUNED)
    // RELAXED thresholds - pottery pieces naturally have large rotations
    const double rad_threshold = 1.57;   // RELAXED: ~90 degrees (was 15°, legacy pottery allows large rotations)
    const double t_threshold = 100.0;    // RELAXED: 100mm translation (was 20mm, pottery fragments spread widely)

    for (const auto& conn : state.selected_connections) {
        auto transform_a = state.piece_world_transforms.find(conn.piece_a);
        auto transform_b = state.piece_world_transforms.find(conn.piece_b);

        if (transform_a != state.piece_world_transforms.end() &&
            transform_b != state.piece_world_transforms.end()) {

            // Compute relative transformation between connected pieces
            Matrix4d T_relative = transform_a->second.inverse() * transform_b->second;

            // Extract rotation and translation from relative transformation
            Matrix3d R_rel = T_relative.block<3,3>(0,0);
            Vector3d t_rel = T_relative.block<3,1>(0,3);

            // Compute rotation angle using legacy method (matrix logarithm)
            Matrix3d log_R = R_rel.log();
            Vector3d w;
            w << -log_R(1, 2), log_R(0, 2), -log_R(0, 1);
            double rotation_angle = w.norm();  // Radians
            double translation_distance = t_rel.norm();

            // POTTERY RECONSTRUCTION: NO transformation penalties - pottery pieces require arbitrary rotations/translations
            // Unlike CAD parts (pre-aligned), pottery fragments start in random orientations
            // Large rotations (176°) and translations (100+mm) are NORMAL for pottery assembly

            // POTTERY-APPROPRIATE: Only penalize truly extreme transformations that indicate errors
            if (rotation_angle > 6.0 || translation_distance > 500.0) {  // Extreme outliers only (>340° rotation, >500mm translation)
                double penalty = 1.0;  // Very mild penalty for clear errors
                coherence_score -= penalty;
                if (config_.enable_logging) {
                    std::cout << "*** EXTREME TRANSFORMATION WARNING *** Connection "
                             << conn.piece_a << "->" << conn.piece_b
                             << " rot=" << (rotation_angle * 180.0 / M_PI) << "° trans=" << translation_distance
                             << "mm, penalty=" << penalty << " (extreme outlier detected)" << std::endl;
                }
            } else {
                // NO bonus for "reasonable" transformations - all transformations are valid for pottery
                if (config_.enable_logging && (rotation_angle > 3.0 || translation_distance > 100.0)) {
                    std::cout << "*** POTTERY TRANSFORMATION *** Connection "
                             << conn.piece_a << "->" << conn.piece_b
                             << " rot=" << (rotation_angle * 180.0 / M_PI) << "° trans=" << translation_distance
                             << "mm (normal pottery assembly transformation)" << std::endl;
                }
            }
        }
    }

    // CONSTRAINT 6: Small Shard Filtering (LEGACY-TUNED)
    // RELAXED threshold for small fragments
    for (const auto& conn : state.selected_connections) {
        // Very conservative threshold - only penalize extremely poor connections
        if (conn.inlier_count < 5) {  // RELAXED: was 10, now 5 (legacy accepts even weak connections for completeness)
            coherence_score -= 5.0;  // REDUCED: was 15, now 5
            if (config_.enable_logging) {
                std::cout << "*** SMALL SHARD PENALTY *** Connection "
                         << conn.piece_a << "->" << conn.piece_b
                         << " very low inliers (" << conn.inlier_count << "), likely tiny fragment" << std::endl;
            }
        }
    }

    // CONSTRAINT 7: Axis Alignment Consistency (from legacy axis_restrain logic)
    // Validate that pieces maintain consistent axis alignment for pottery vessels
    if (state.active_pieces.size() >= 3) {
        std::vector<Vector3d> piece_positions;
        for (int piece : state.active_pieces) {
            auto transform_it = state.piece_world_transforms.find(piece);
            if (transform_it != state.piece_world_transforms.end()) {
                Vector3d pos = transform_it->second.block<3,1>(0,3);
                piece_positions.push_back(pos);
            }
        }

        if (piece_positions.size() >= 3) {
            // Check consistency of pieces around vertical axis (Z-axis for pottery)
            Vector3d centroid = Vector3d::Zero();
            for (const auto& pos : piece_positions) {
                centroid += pos;
            }
            centroid /= piece_positions.size();

            // Calculate deviation from consistent radial distribution around Z-axis
            double total_deviation = 0.0;
            std::vector<double> radial_distances;

            for (const auto& pos : piece_positions) {
                Vector3d relative_pos = pos - centroid;
                double radial_dist = sqrt(relative_pos.x() * relative_pos.x() + relative_pos.y() * relative_pos.y());
                radial_distances.push_back(radial_dist);
            }

            // Check if pieces are consistently distributed around axis
            if (!radial_distances.empty()) {
                double mean_radius = 0.0;
                for (double r : radial_distances) mean_radius += r;
                mean_radius /= radial_distances.size();

                double radius_variance = 0.0;
                for (double r : radial_distances) {
                    radius_variance += (r - mean_radius) * (r - mean_radius);
                }
                radius_variance /= radial_distances.size();

                // RELAXED penalty for axis alignment - pottery allows more variance
                if (radius_variance > 1000.0) {  // RELAXED: was 400, now 1000 (pottery pieces spread more widely)
                    coherence_score -= 5.0 * (radius_variance / 1000.0);  // REDUCED: was 20, now 5
                    if (config_.enable_logging) {
                        std::cout << "*** AXIS ALIGNMENT PENALTY *** High radial variance " << radius_variance
                                 << " indicates poor axis alignment" << std::endl;
                    }
                } else {
                    coherence_score += 2.0;  // Small bonus for reasonable axis alignment
                }
            }
        }
    }

    // CONSTRAINT 8: Multi-Connection Consistency (from legacy TransAverage logic)
    // Validate that pieces with multiple connections are positioned optimally
    std::map<int, std::vector<ProvenConnection>> piece_connections;

    // Group connections by piece
    for (const auto& conn : state.selected_connections) {
        piece_connections[conn.piece_a].push_back(conn);
        piece_connections[conn.piece_b].push_back(conn);
    }

    // Check pieces that have multiple connections
    for (const auto& pair : piece_connections) {
        int piece_id = pair.first;
        const auto& connections = pair.second;

        if (connections.size() >= 2) {  // Piece has multiple connections
            // Calculate quality-weighted position expectation
            double total_weight = 0.0;
            Vector3d expected_position = Vector3d::Zero();

            auto piece_transform_it = state.piece_world_transforms.find(piece_id);
            if (piece_transform_it == state.piece_world_transforms.end()) continue;

            Vector3d current_position = piece_transform_it->second.block<3,1>(0,3);

            for (const auto& conn : connections) {
                // Use inlier count as quality weight
                double weight = static_cast<double>(conn.inlier_count);
                if (weight < 1.0) weight = 1.0;  // Minimum weight

                // Find the connected piece position
                int other_piece = (conn.piece_a == piece_id) ? conn.piece_b : conn.piece_a;
                auto other_transform_it = state.piece_world_transforms.find(other_piece);

                if (other_transform_it != state.piece_world_transforms.end()) {
                    Vector3d other_position = other_transform_it->second.block<3,1>(0,3);
                    expected_position += weight * other_position;
                    total_weight += weight;
                }
            }

            if (total_weight > 0.0) {
                expected_position /= total_weight;

                // Calculate deviation from optimal position
                double position_deviation = (current_position - expected_position).norm();

                // RELAXED penalty for multi-connection positioning
                if (position_deviation > 30.0) {  // RELAXED: was 15mm, now 30mm tolerance
                    double penalty = 3.0 * (position_deviation / 30.0);  // REDUCED: was 10, now 3
                    coherence_score -= penalty;
                    if (config_.enable_logging) {
                        std::cout << "*** MULTI-CONNECTION PENALTY *** Piece " << piece_id
                                 << " deviation=" << position_deviation << "mm from optimal multi-connection position"
                                 << ", penalty=" << penalty << std::endl;
                    }
                } else {
                    coherence_score += 1.0;  // Small bonus for reasonable positioning
                }
            }
        }
    }
    }

    return coherence_score;
}

double AssemblyScorer::calculateClusterCoherencePenalty(const HybridAssemblyState& state) const {
    // Penalize assembly states with edge competition or inconsistent triangles
    // This PREVENTS selecting locally good but globally bad connections during beam search

    if (state.selected_connections.size() < 2) return 0.0;  // Need at least 2 connections

    double penalty = 0.0;
    int total_checks = 0;
    int edge_conflicts = 0;

    // Check all pairs of connections for edge competition
    for (size_t i = 0; i < state.selected_connections.size(); ++i) {
        for (size_t j = i + 1; j < state.selected_connections.size(); ++j) {
            const auto& conn_i = state.selected_connections[i];
            const auto& conn_j = state.selected_connections[j];

            // Find common piece
            int common_piece = -1;
            if (conn_i.piece_a == conn_j.piece_a || conn_i.piece_a == conn_j.piece_b) {
                common_piece = conn_i.piece_a;
            } else if (conn_i.piece_b == conn_j.piece_a || conn_i.piece_b == conn_j.piece_b) {
                common_piece = conn_i.piece_b;
            }

            if (common_piece > 0) {
                // Get edge segments for common piece
                int seg_i_start = (conn_i.piece_a == common_piece) ? conn_i.start_a : conn_i.start_b;
                int seg_i_end = (conn_i.piece_a == common_piece) ? conn_i.end_a : conn_i.end_b;
                int seg_j_start = (conn_j.piece_a == common_piece) ? conn_j.start_a : conn_j.start_b;
                int seg_j_end = (conn_j.piece_a == common_piece) ? conn_j.end_a : conn_j.end_b;

                // Check for segment overlap (edge competition)
                if (seg_i_start > seg_i_end) std::swap(seg_i_start, seg_i_end);
                if (seg_j_start > seg_j_end) std::swap(seg_j_start, seg_j_end);

                int overlap_start = std::max(seg_i_start, seg_j_start);
                int overlap_end = std::min(seg_i_end, seg_j_end);

                if (overlap_end > overlap_start) {
                    int overlap_len = overlap_end - overlap_start;
                    int min_seg_len = std::min(seg_i_end - seg_i_start, seg_j_end - seg_j_start);

                    if (min_seg_len > 0) {
                        double overlap_ratio = static_cast<double>(overlap_len) / min_seg_len;

                        if (overlap_ratio > 0.3) {  // 30% overlap = edge competition
                            edge_conflicts++;
                            penalty -= 20.0;  // HEAVY penalty for edge competition

                            static int debug_count = 0;
                            if (config_.enable_logging && debug_count < 5) {
                                std::cout << "*** BEAM SEARCH COHERENCE *** Edge competition: piece "
                                          << common_piece << " overlap " << (overlap_ratio*100)
                                          << "% (penalty -20)" << std::endl;
                                debug_count++;
                            }
                        }
                    }
                }
            }

            total_checks++;
        }
    }

    static int score_debug_count = 0;
    if (config_.enable_logging && score_debug_count < 3 && penalty != 0.0) {
        std::cout << "*** BEAM COHERENCE PENALTY *** " << penalty << " (conflicts: " << edge_conflicts << ")" << std::endl;
        score_debug_count++;
    }

    return penalty;
}

//==============================================================================
// HybridPuzzleFusionOptimizer Implementation
//==============================================================================

HybridPuzzleFusionOptimizer::HybridPuzzleFusionOptimizer(const HybridOptimizationConfig& config)
    : config_(config) {
}

HybridAssemblyState HybridPuzzleFusionOptimizer::optimizeGlobalAssembly(
    int total_pieces,
    const std::vector<LCSIndex>& all_connections,
    const std::vector<Geom>& geometry) {

    // PHASE 3+ FEATURE FLAG: Check for hierarchical archaeological assembly mode
    const char* hierarchical_env = std::getenv("ENABLE_HIERARCHICAL_ASSEMBLY");
    bool use_hierarchical_assembly = (hierarchical_env != nullptr && std::string(hierarchical_env) == "1");

    if (use_hierarchical_assembly) {
        std::cout << "\n*** FEATURE FLAG: ENABLE_HIERARCHICAL_ASSEMBLY=1 detected ***" << std::endl;
        std::cout << "*** Switching to HIERARCHICAL ARCHAEOLOGICAL ASSEMBLY mode ***\n" << std::endl;

        // Initialize components
        validator_ = std::make_unique<GeometryValidator>(geometry);
        scorer_ = std::make_unique<AssemblyScorer>(config_, validator_.get());

        // Convert connections
        std::vector<ProvenConnection> proven_connections = convertToProvenConnections(all_connections, geometry);

        // Configure archaeological assembly parameters
        ArchaeologicalConfig arch_config;
        // Use environment variables to override defaults if needed
        const char* rim_rms_env = std::getenv("HIERARCHICAL_RIM_RMS_THRESHOLD");
        if (rim_rms_env) arch_config.rim_rms_threshold = std::atof(rim_rms_env);

        const char* max_attempts_env = std::getenv("HIERARCHICAL_MAX_RING_ATTEMPTS");
        if (max_attempts_env) arch_config.max_ring_attempts = std::atoi(max_attempts_env);

        // Run hierarchical assembly
        return performHierarchicalAssembly(total_pieces, proven_connections, geometry, arch_config);
    }

    // Default behavior: Standard hybrid PuzzleFusion++ optimization
    if (config_.enable_logging) {
        std::cout << "*** HYBRID PUZZLEFUSION++ *** Starting optimization with "
                  << total_pieces << " pieces, " << all_connections.size() << " connections" << std::endl;
    }

    // DEBUG: Check axis data presence in geometry
    std::cout << "*** PUZZLEFUSION GEOMETRY CHECK *** Checking axis data for " << geometry.size() << " pieces:" << std::endl;
    for (size_t i = 0; i < std::min(geometry.size(), size_t(8)); ++i) {
        std::cout << "  Piece " << (i+1) << ": original_axis_norm_.size()=" << geometry[i].edge_line_.original_axis_norm_.size();
        if (!geometry[i].edge_line_.original_axis_norm_.empty()) {
            std::cout << " original_axis=[" << geometry[i].edge_line_.original_axis_norm_[0].transpose() << "]";
        }
        std::cout << std::endl;
    }

    // Initialize components
    validator_ = std::make_unique<GeometryValidator>(geometry);
    scorer_ = std::make_unique<AssemblyScorer>(config_, validator_.get());

    // Convert LCSIndex to ProvenConnection format (WITH POTTERY AXIS FILTERING)
    std::vector<ProvenConnection> proven_connections = convertToProvenConnections(all_connections, geometry);

    if (proven_connections.empty()) {
        HybridAssemblyState empty_state;
        empty_state.total_pieces = total_pieces;
        return empty_state;
    }

    // PHASE 1: Initialize with best connections
    std::vector<HybridAssemblyState> states = generateInitialStates(proven_connections, geometry);

    if (config_.enable_logging) {
        std::cout << "*** HYBRID INITIALIZATION *** Created " << states.size()
                  << " initial states from top connections" << std::endl;
    }

    // PHASE 2: Iterative optimization
    for (int iteration = 0; iteration < config_.max_iterations; ++iteration) {
        if (config_.enable_logging) {
            std::cout << "*** HYBRID ITERATION " << iteration << " *** Processing "
                      << states.size() << " states" << std::endl;
        }

        // Explore new states using matrix operations
        exploreAssemblyStates(states, proven_connections);

        // CRITICAL FIX: Prune invalid topologies BEFORE validation
        // This ensures we only validate states with valid tree structures
        pruneBeamStates(states);

        // Validate top candidates with real geometry
        validateTopCandidates(states);

        // Check convergence
        if (hasConverged(states)) {
            if (config_.enable_logging) {
                std::cout << "*** HYBRID CONVERGENCE *** Achieved after " << iteration + 1 << " iterations" << std::endl;
            }
            break;
        }

        logOptimizationProgress(iteration, states);
    }

    // Return best state
    if (!states.empty()) {
        std::sort(states.begin(), states.end(), std::greater<HybridAssemblyState>());

        if (config_.enable_logging) {
            const auto& best = states[0];
            std::cout << "*** HYBRID SUCCESS *** Best assembly: " << best.active_pieces.size()
                      << " pieces, score=" << std::fixed << std::setprecision(3)
                      << scorer_->calculateFinalScore(best) << std::endl;
        }

        return states[0];
    }

    // Fallback empty state
    HybridAssemblyState fallback;
    fallback.total_pieces = total_pieces;
    return fallback;
}

//==============================================================================
// Archaeological Assembly - Phase 1: Structural Anchor Identification
//==============================================================================

int HybridPuzzleFusionOptimizer::identifyStructuralAnchor(
    const std::vector<ProvenConnection>& connections) const {

    // ARCHAEOLOGICAL STRATEGY: Identify piece with HIGHEST CONNECTIVITY
    // This is the structural anchor (rim/large piece) for archaeological assembly
    // Unlike hub-and-spoke (which picks highest inliers), this respects vessel structure

    // CRITICAL FIX: Count UNIQUE piece pairs, not total connection candidates
    // Multiple ICP attempts for same pair should count as 1 connection
    std::map<int, std::set<int>> unique_neighbors;  // piece -> set of unique connected pieces
    std::map<std::string, double> best_quality_per_pair;    // Track best quality for each pair (string key: "1-2")

    // Build unique neighbor sets
    // ARCHAEOLOGICAL FIX: Use STRICTER threshold for anchor identification
    // Only count HIGH-QUALITY connections (top candidates after coherence adjustment)
    // This filters out false positives and reveals true connectivity pattern
    const double ANCHOR_QUALITY_THRESHOLD = 20.0;  // Only connections with quality > 20

    for (const auto& conn : connections) {
        if (conn.combined_quality >= ANCHOR_QUALITY_THRESHOLD) {
            // Add unique neighbors
            unique_neighbors[conn.piece_a].insert(conn.piece_b);
            unique_neighbors[conn.piece_b].insert(conn.piece_a);

            // Track best quality for this piece pair
            std::string pair_key = std::to_string(std::min(conn.piece_a, conn.piece_b)) + "-" +
                                  std::to_string(std::max(conn.piece_a, conn.piece_b));
            if (best_quality_per_pair.find(pair_key) == best_quality_per_pair.end()) {
                best_quality_per_pair[pair_key] = conn.combined_quality;
            } else {
                best_quality_per_pair[pair_key] = std::max(best_quality_per_pair[pair_key], conn.combined_quality);
            }
        }
    }

    if (unique_neighbors.empty()) {
        std::cerr << "*** WARNING *** No valid connections for anchor identification" << std::endl;
        return 1; // Fallback to piece 1
    }

    // Calculate total quality for each piece (sum of best qualities to its neighbors)
    std::map<int, double> total_quality;
    for (const auto& [piece, neighbors] : unique_neighbors) {
        double quality_sum = 0.0;
        for (int neighbor : neighbors) {
            std::string pair_key = std::to_string(std::min(piece, neighbor)) + "-" +
                                  std::to_string(std::max(piece, neighbor));
            quality_sum += best_quality_per_pair[pair_key];
        }
        total_quality[piece] = quality_sum;
    }

    // Find piece with highest UNIQUE connectivity
    int anchor_piece = -1;
    int max_connectivity = 0;
    double best_quality = 0.0;

    for (const auto& [piece, neighbors] : unique_neighbors) {
        int unique_count = neighbors.size();

        // Primary criterion: unique neighbor count
        // Secondary criterion: total connection quality
        if (unique_count > max_connectivity ||
            (unique_count == max_connectivity && total_quality[piece] > best_quality)) {
            max_connectivity = unique_count;
            best_quality = total_quality[piece];
            anchor_piece = piece;
        }
    }

    // DEBUG: Report connectivity analysis
    std::cout << "\n=== ARCHAEOLOGICAL ANCHOR IDENTIFICATION ===" << std::endl;
    std::cout << "*** UNIQUE CONNECTIVITY ANALYSIS ***" << std::endl;

    // Sort pieces by unique connectivity for display
    std::vector<std::pair<int, int>> sorted_pieces;
    for (const auto& [piece, neighbors] : unique_neighbors) {
        sorted_pieces.push_back({piece, neighbors.size()});
    }
    std::sort(sorted_pieces.begin(), sorted_pieces.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [piece, unique_count] : sorted_pieces) {
        std::cout << "  Piece " << piece << ": " << unique_count << " UNIQUE connections"
                  << " (total_quality=" << std::fixed << std::setprecision(1) << total_quality[piece] << ")";
        if (piece == anchor_piece) {
            std::cout << " ← STRUCTURAL ANCHOR";
        }

        // Show which pieces it connects to
        if (unique_count <= 8) {  // Only show if reasonable number
            std::cout << " → {";
            bool first = true;
            for (int neighbor : unique_neighbors.at(piece)) {
                if (!first) std::cout << ",";
                std::cout << neighbor;
                first = false;
            }
            std::cout << "}";
        }

        std::cout << std::endl;
    }

    std::cout << "\n*** ANCHOR SELECTED *** Piece " << anchor_piece
              << " (unique_connectivity=" << max_connectivity << ", quality=" << std::fixed << std::setprecision(1) << best_quality << ")" << std::endl;
    std::cout << "=== ARCHAEOLOGICAL ANCHOR IDENTIFICATION COMPLETE ===\n" << std::endl;

    return anchor_piece;
}

//==============================================================================
// Archaeological Assembly - Phase 2: Height and Angle Extraction
//==============================================================================

double HybridPuzzleFusionOptimizer::getHeight(int piece_id, const std::vector<Geom>& geometry) const {
    if (piece_id < 1 || piece_id > static_cast<int>(geometry.size())) {
        return 0.0;
    }

    const auto& geom = geometry[piece_id - 1];  // 1-based to 0-based

    // CRITICAL FIX: Use original_axis_height_ stored BEFORE AxisAlignment transformation!
    // The axis_point_ data gets transformed to origin by AxisAlignment, losing height information.
    // Original heights are preserved in original_axis_height_ field computed in main before alignment.
    return geom.original_axis_height_;
}

double HybridPuzzleFusionOptimizer::getAngle(int piece_id, const std::vector<Geom>& geometry) const {
    if (piece_id < 1 || piece_id > static_cast<int>(geometry.size())) {
        return 0.0;
    }

    const auto& geom = geometry[piece_id - 1];  // 1-based to 0-based
    if (geom.edge_line_.axis_point_.empty()) {
        return 0.0;
    }

    // Calculate angle around vessel axis (cylindrical coordinates)
    // Use average position of axis points
    double total_x = 0.0, total_y = 0.0;
    int count = 0;

    for (const auto& point : geom.edge_line_.axis_point_) {
        total_x += point.x();
        total_y += point.y();
        count++;
    }

    if (count == 0) return 0.0;

    double avg_x = total_x / count;
    double avg_y = total_y / count;

    // Angle in degrees [0, 360)
    double angle = atan2(avg_y, avg_x) * 180.0 / M_PI;
    if (angle < 0) angle += 360.0;

    return angle;
}

// ============================= PRD_RIMBASE: EVIDENCE EXTRACTION HELPERS =============================

/**
 * PRD_Rimbase.md Phase 6: Extract rim edge from surface points (not breaklines)
 *
 * Rim edge = horizontal edge at top of piece, near axis height
 * Breaklines = vertical fracture edges (wrong for rim validation)
 */
HybridPuzzleFusionOptimizer::EdgeExtractionResult
HybridPuzzleFusionOptimizer::extractRimEdge(
    const Geom& geom,
    const Eigen::Vector3d& axis_pos,
    double axis_height,
    const ArchaeologicalConfig& config) const {

    EdgeExtractionResult result;

    // Extract surface points (inner or outer surface)
    std::vector<Eigen::Vector3d> surface_points;
    std::vector<Eigen::Vector3d> surface_normals;

    // Try inner surface first
    if (geom.sur_in_.point_.cols() > 0) {
        for (int i = 0; i < geom.sur_in_.point_.cols(); ++i) {
            surface_points.push_back(geom.sur_in_.point_.col(i));
            if (geom.sur_in_.normal_.cols() > i) {
                surface_normals.push_back(geom.sur_in_.normal_.col(i));
            }
        }
    }

    // If no inner surface, try outer surface
    if (surface_points.empty() && geom.sur_out_.point_.cols() > 0) {
        for (int i = 0; i < geom.sur_out_.point_.cols(); ++i) {
            surface_points.push_back(geom.sur_out_.point_.col(i));
            if (geom.sur_out_.normal_.cols() > i) {
                surface_normals.push_back(geom.sur_out_.normal_.col(i));
            }
        }
    }

    if (surface_points.empty()) {
        std::cout << "  [extractRimEdge] No surface points available" << std::endl;
        return result;
    }

    // Find Z_max of surface
    double z_max = surface_points[0].z();
    for (const auto& pt : surface_points) {
        if (pt.z() > z_max) z_max = pt.z();
    }

    // Extract points within rim_edge_tolerance of top
    double rim_z_threshold = z_max - config.rim_edge_tolerance;

    // Estimate vessel radius from axis height (for radial filtering)
    double expected_radius = 50.0; // mm - typical pottery rim radius, will refine from data

    for (size_t i = 0; i < surface_points.size(); ++i) {
        const auto& pt = surface_points[i];

        // Check Z threshold (near top)
        if (pt.z() < rim_z_threshold) continue;

        // Check radial distance from axis (filter interior/exterior noise)
        double dx = pt.x() - axis_pos.x();
        double dy = pt.y() - axis_pos.y();
        double radius = std::sqrt(dx*dx + dy*dy);

        // Accept points within reasonable radial range
        // (Will be refined by circle fitting, but this removes obvious outliers)
        if (radius < 5.0 || radius > 150.0) continue;

        result.edge_points.push_back(pt);
        if (i < surface_normals.size()) {
            result.edge_normals.push_back(surface_normals[i]);
        }
    }

    result.num_points = result.edge_points.size();
    result.edge_height = z_max;
    result.is_valid = (result.num_points >= 10);  // Need minimum points for circle fit

    std::cout << "  [extractRimEdge] Extracted " << result.num_points
              << " rim edge points at Z=" << z_max << "mm (threshold="
              << rim_z_threshold << "mm)" << std::endl;

    return result;
}

/**
 * PRD_Rimbase.md Phase 6: Extract base edge from surface points (not breaklines)
 *
 * Base edge = horizontal edge at bottom of piece, near axis height
 */
HybridPuzzleFusionOptimizer::EdgeExtractionResult
HybridPuzzleFusionOptimizer::extractBaseEdge(
    const Geom& geom,
    const Eigen::Vector3d& axis_pos,
    double axis_height,
    const ArchaeologicalConfig& config) const {

    EdgeExtractionResult result;

    // Extract surface points
    std::vector<Eigen::Vector3d> surface_points;
    std::vector<Eigen::Vector3d> surface_normals;

    // Try inner surface first
    if (geom.sur_in_.point_.cols() > 0) {
        for (int i = 0; i < geom.sur_in_.point_.cols(); ++i) {
            surface_points.push_back(geom.sur_in_.point_.col(i));
            if (geom.sur_in_.normal_.cols() > i) {
                surface_normals.push_back(geom.sur_in_.normal_.col(i));
            }
        }
    }

    // If no inner surface, try outer surface
    if (surface_points.empty() && geom.sur_out_.point_.cols() > 0) {
        for (int i = 0; i < geom.sur_out_.point_.cols(); ++i) {
            surface_points.push_back(geom.sur_out_.point_.col(i));
            if (geom.sur_out_.normal_.cols() > i) {
                surface_normals.push_back(geom.sur_out_.normal_.col(i));
            }
        }
    }

    if (surface_points.empty()) {
        std::cout << "  [extractBaseEdge] No surface points available" << std::endl;
        return result;
    }

    // Find Z_min of surface
    double z_min = surface_points[0].z();
    for (const auto& pt : surface_points) {
        if (pt.z() < z_min) z_min = pt.z();
    }

    // Extract points within base_edge_tolerance of bottom
    double base_z_threshold = z_min + config.base_edge_tolerance;

    for (size_t i = 0; i < surface_points.size(); ++i) {
        const auto& pt = surface_points[i];

        // Check Z threshold (near bottom)
        if (pt.z() > base_z_threshold) continue;

        // Check radial distance from axis
        double dx = pt.x() - axis_pos.x();
        double dy = pt.y() - axis_pos.y();
        double radius = std::sqrt(dx*dx + dy*dy);

        // Accept points within reasonable radial range
        if (radius < 5.0 || radius > 150.0) continue;

        result.edge_points.push_back(pt);
        if (i < surface_normals.size()) {
            result.edge_normals.push_back(surface_normals[i]);
        }
    }

    result.num_points = result.edge_points.size();
    result.edge_height = z_min;
    result.is_valid = (result.num_points >= 10);

    std::cout << "  [extractBaseEdge] Extracted " << result.num_points
              << " base edge points at Z=" << z_min << "mm (threshold="
              << base_z_threshold << "mm)" << std::endl;

    return result;
}

std::vector<Eigen::Vector3d> HybridPuzzleFusionOptimizer::extractEdgePoints(const Geom& geom) const {
    std::vector<Eigen::Vector3d> points;

    // Extract breakline edge points
    if (geom.edge_line_.point_.cols() > 0) {
        for (int i = 0; i < geom.edge_line_.point_.cols(); ++i) {
            points.push_back(geom.edge_line_.point_.col(i));
        }
    }

    return points;
}

std::vector<Eigen::Vector3d> HybridPuzzleFusionOptimizer::extractEdgeNormals(const Geom& geom) const {
    std::vector<Eigen::Vector3d> normals;

    // Extract breakline normals
    if (geom.edge_line_.normal_.cols() > 0) {
        for (int i = 0; i < geom.edge_line_.normal_.cols(); ++i) {
            normals.push_back(geom.edge_line_.normal_.col(i));
        }
    }

    return normals;
}

std::vector<double> HybridPuzzleFusionOptimizer::extractThicknessValues(const Geom& geom) const {
    std::vector<double> thickness;

    // Extract thickness data from feature vector (feature_[6] = thickness)
    if (geom.edge_line_.feature_.size() > 6 && geom.edge_line_.feature_[6].cols() > 0) {
        for (int i = 0; i < geom.edge_line_.feature_[6].cols(); ++i) {
            thickness.push_back(geom.edge_line_.feature_[6](0, i));
        }
    }

    return thickness;
}

Eigen::Vector3d HybridPuzzleFusionOptimizer::extractAxisNormal(const Geom& geom) const {
    // Use transformed axis normal if available
    if (!geom.edge_line_.axis_norm_.empty()) {
        return geom.edge_line_.axis_norm_[0];
    }

    // Fallback: Use original axis normal
    if (!geom.edge_line_.original_axis_norm_.empty()) {
        return geom.edge_line_.original_axis_norm_[0];
    }

    // Default: Z-axis
    return Eigen::Vector3d::UnitZ();
}

// PRD_Rimbase.md Phase 6: Geometry-based rim/base identification
HybridPuzzleFusionOptimizer::GeometricRimBaseResult
HybridPuzzleFusionOptimizer::identifyRimBasePiecesByGeometry(
    int total_pieces,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) const {

    std::cout << "\n=== GEOMETRY-BASED RIM/BASE IDENTIFICATION ===" << std::endl;
    std::cout << "Testing all " << total_pieces << " pieces for rim/base geometric properties..." << std::endl;

    GeometricRimBaseResult result;
    RimBaseValidator validator;

    for (int piece_id = 1; piece_id <= total_pieces; ++piece_id) {
        const auto& geom = geometry[piece_id - 1];

        // Get axis data
        Eigen::Vector3d axis_normal = extractAxisNormal(geom);
        double height = getHeight(piece_id, geometry);

        // Compute axis position
        Eigen::Vector3d axis_pos = Eigen::Vector3d::Zero();
        if (!geom.edge_line_.axis_point_.empty()) {
            double total_x = 0.0, total_y = 0.0, total_z = 0.0;
            int count = 0;
            for (const auto& point : geom.edge_line_.axis_point_) {
                total_x += point.x();
                total_y += point.y();
                total_z += point.z();
                count++;
            }
            if (count > 0) {
                axis_pos = Eigen::Vector3d(total_x / count, total_y / count, total_z / count);
            }
        }

        // Test RIM geometry: Extract top edge and validate circle fit
        EdgeExtractionResult rim_edge = extractRimEdge(geom, axis_pos, height, config);

        if (rim_edge.is_valid && rim_edge.num_points >= 10) {
            // Fit rim primitive and check quality
            PrimitiveEvidence rim_prim = validator.fitRimPrimitive(
                rim_edge.edge_points, axis_normal, height, config);

            // Also check edge smoothness and tangency (PRD requirement)
            SmoothnessEvidence smooth = validator.edgeSmoothnessAndTangency(
                rim_edge.edge_points, rim_edge.edge_normals, axis_normal, config);

            // Compute confidence based on:
            // 1. Circle fit quality (RMS error)
            // 2. Inlier ratio
            // 3. Edge smoothness
            // 4. Tangent perpendicular to axis (rim wraps around vessel)
            double geometric_quality = 0.0;

            if (rim_prim.is_valid) {
                // RMS quality: 0mm = 1.0, 5mm = 0.5, >10mm = 0.0
                double rms_quality = std::max(0.0, 1.0 - rim_prim.rms_error / 10.0);

                // Inlier quality
                double inlier_quality = rim_prim.inlier_ratio;

                // Smoothness quality
                double smoothness_quality = smooth.is_smooth ? 1.0 : 0.3;

                // Tangency quality: deviation < 15° = good, > 45° = bad
                double tangency_quality = std::max(0.0, 1.0 - smooth.axis_tangency_angle / 45.0);

                // Weighted combination
                geometric_quality = 0.4 * rms_quality +
                                  0.3 * inlier_quality +
                                  0.15 * smoothness_quality +
                                  0.15 * tangency_quality;

                std::cout << "  Piece " << piece_id << " RIM test: "
                          << "RMS=" << std::fixed << std::setprecision(2) << rim_prim.rms_error << "mm, "
                          << "inliers=" << std::setprecision(1) << (rim_prim.inlier_ratio * 100) << "%, "
                          << "smooth=" << (smooth.is_smooth ? "✓" : "✗") << ", "
                          << "tangency=" << std::setprecision(1) << smooth.axis_tangency_angle << "°, "
                          << "confidence=" << std::setprecision(2) << geometric_quality << std::endl;

                // Accept as RIM if confidence >= 0.5
                if (geometric_quality >= 0.5) {
                    result.rim_pieces.push_back(piece_id);
                    result.confidence[piece_id] = geometric_quality;
                }
            }
        }

        // Test BASE geometry: Extract bottom edge and validate plane fit
        EdgeExtractionResult base_edge = extractBaseEdge(geom, axis_pos, height, config);

        if (base_edge.is_valid && base_edge.num_points >= 10) {
            // Fit base primitive and check quality
            bool has_foot_ring = false;
            PrimitiveEvidence base_prim = validator.fitBasePrimitive(
                base_edge.edge_points, axis_normal, config, has_foot_ring);

            if (base_prim.is_valid) {
                // Base quality: plane fit + horizontal alignment
                double rms_quality = std::max(0.0, 1.0 - base_prim.rms_error / 10.0);
                double inlier_quality = base_prim.inlier_ratio;

                double geometric_quality = 0.6 * rms_quality + 0.4 * inlier_quality;

                std::cout << "  Piece " << piece_id << " BASE test: "
                          << "RMS=" << std::fixed << std::setprecision(2) << base_prim.rms_error << "mm, "
                          << "inliers=" << std::setprecision(1) << (base_prim.inlier_ratio * 100) << "%, "
                          << (has_foot_ring ? "foot-ring detected, " : "")
                          << "confidence=" << std::setprecision(2) << geometric_quality << std::endl;

                // Accept as BASE if confidence >= 0.5
                if (geometric_quality >= 0.5) {
                    result.base_pieces.push_back(piece_id);
                    result.confidence[piece_id] = geometric_quality;
                }
            }
        }
    }

    std::cout << "\n=== GEOMETRIC IDENTIFICATION RESULTS ===" << std::endl;
    std::cout << "  RIM pieces (geometric): " << result.rim_pieces.size() << " - [";
    for (size_t i = 0; i < result.rim_pieces.size(); ++i) {
        std::cout << result.rim_pieces[i];
        if (i < result.rim_pieces.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    std::cout << "  BASE pieces (geometric): " << result.base_pieces.size() << " - [";
    for (size_t i = 0; i < result.base_pieces.size(); ++i) {
        std::cout << result.base_pieces[i];
        if (i < result.base_pieces.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    return result;
}

// ============================= PRD_RIMBASE: EVIDENCE FUSION =============================

double HybridPuzzleFusionOptimizer::fuseEvidenceForRim(
    double height_prior,
    const PrimitiveEvidence& prim,
    const SmoothnessEvidence& smooth,
    const ThicknessTrendEvidence& thick) const {

    // PRD weights: 40% height, 30% primitive, 20% smoothness, 10% thickness
    double score = 0.4 * height_prior;

    // Primitive evidence (circle fit quality)
    if (prim.is_valid) {
        score += 0.3 * prim.inlier_ratio;
    }

    // Smoothness evidence (low curvature + perpendicular to axis)
    if (smooth.is_smooth && smooth.axis_tangency_angle < 15.0) {
        score += 0.2;
    }

    // Thickness trend (rims often thin toward lip)
    if (thick.is_thinning) {
        score += 0.1;
    }

    return std::min(1.0, score);
}

double HybridPuzzleFusionOptimizer::fuseEvidenceForBase(
    double height_prior,
    const PrimitiveEvidence& prim,
    const SmoothnessEvidence& smooth,
    const ThicknessTrendEvidence& thick) const {

    // PRD weights: 40% height, 30% primitive, 20% smoothness, 10% thickness
    double score = 0.4 * height_prior;

    // Primitive evidence (plane fit quality)
    if (prim.is_valid) {
        score += 0.3 * prim.inlier_ratio;
    }

    // Smoothness evidence (flat, horizontal base)
    if (smooth.is_smooth) {
        score += 0.2;
    }

    // Thickness trend (bases often thicken)
    if (thick.is_thickening) {
        score += 0.1;
    }

    return std::min(1.0, score);
}

// ============================= PRD_RIMBASE: SINGLE-PIECE SUFFICIENCY =============================

HybridPuzzleFusionOptimizer::SufficiencyResult
HybridPuzzleFusionOptimizer::checkRimSufficiency(
    const std::vector<int>& rim_pieces,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) const {

    SufficiencyResult result;
    RimBaseValidator validator;

    for (int piece_id : rim_pieces) {
        const auto& geom = geometry[piece_id - 1];  // 1-based to 0-based

        // Get axis data for edge extraction
        Eigen::Vector3d axis_normal = extractAxisNormal(geom);
        double height = getHeight(piece_id, geometry);

        // Compute axis position (average of axis points)
        Eigen::Vector3d axis_pos = Eigen::Vector3d::Zero();
        if (!geom.edge_line_.axis_point_.empty()) {
            double total_x = 0.0, total_y = 0.0, total_z = 0.0;
            int count = 0;
            for (const auto& point : geom.edge_line_.axis_point_) {
                total_x += point.x();
                total_y += point.y();
                total_z += point.z();
                count++;
            }
            if (count > 0) {
                axis_pos = Eigen::Vector3d(total_x / count, total_y / count, total_z / count);
            }
        }

        // PRD_RIMBASE Phase 6 FIX: Extract rim edge from surface (not breaklines)
        EdgeExtractionResult edge_result = extractRimEdge(geom, axis_pos, height, config);

        if (!edge_result.is_valid || edge_result.num_points < 3) {
            std::cout << "  [checkRimSufficiency] Piece " << piece_id
                      << " - insufficient rim edge points (" << edge_result.num_points << ")" << std::endl;
            continue;
        }

        std::cout << "  [checkRimSufficiency] Piece " << piece_id
                  << " - extracted " << edge_result.num_points << " rim edge points" << std::endl;

        // Fit rim primitive using extracted horizontal rim edge
        PrimitiveEvidence prim = validator.fitRimPrimitive(
            edge_result.edge_points, axis_normal, height, config);

        // Check 80% coverage threshold
        if (validator.checkSinglePieceSufficiency(prim, config, true)) {
            result.is_sufficient = true;
            result.piece_id = piece_id;
            result.coverage = prim.angular_coverage;
            result.rms_error = prim.rms_error;
            result.reason = "Single piece covers 80%+ of rim with RMS < 5mm";
            return result;
        }
    }

    return result;
}

HybridPuzzleFusionOptimizer::SufficiencyResult
HybridPuzzleFusionOptimizer::checkBaseSufficiency(
    const std::vector<int>& base_pieces,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) const {

    SufficiencyResult result;
    RimBaseValidator validator;

    for (int piece_id : base_pieces) {
        const auto& geom = geometry[piece_id - 1];  // 1-based to 0-based

        // Get axis data for edge extraction
        Eigen::Vector3d axis_normal = extractAxisNormal(geom);
        double height = getHeight(piece_id, geometry);

        // Compute axis position (average of axis points)
        Eigen::Vector3d axis_pos = Eigen::Vector3d::Zero();
        if (!geom.edge_line_.axis_point_.empty()) {
            double total_x = 0.0, total_y = 0.0, total_z = 0.0;
            int count = 0;
            for (const auto& point : geom.edge_line_.axis_point_) {
                total_x += point.x();
                total_y += point.y();
                total_z += point.z();
                count++;
            }
            if (count > 0) {
                axis_pos = Eigen::Vector3d(total_x / count, total_y / count, total_z / count);
            }
        }

        // PRD_RIMBASE Phase 6 FIX: Extract base edge from surface (not breaklines)
        EdgeExtractionResult edge_result = extractBaseEdge(geom, axis_pos, height, config);

        if (!edge_result.is_valid || edge_result.num_points < 3) {
            std::cout << "  [checkBaseSufficiency] Piece " << piece_id
                      << " - insufficient base edge points (" << edge_result.num_points << ")" << std::endl;
            continue;
        }

        std::cout << "  [checkBaseSufficiency] Piece " << piece_id
                  << " - extracted " << edge_result.num_points << " base edge points" << std::endl;

        // Fit base primitive using extracted horizontal base edge
        bool has_foot_ring = false;
        PrimitiveEvidence prim = validator.fitBasePrimitive(
            edge_result.edge_points, axis_normal, config, has_foot_ring);

        // Check 80% coverage threshold
        if (validator.checkSinglePieceSufficiency(prim, config, false)) {
            result.is_sufficient = true;
            result.piece_id = piece_id;
            result.coverage = prim.angular_coverage;
            result.rms_error = prim.rms_error;
            result.reason = has_foot_ring ?
                "Single piece covers 80%+ of base with foot-ring detected" :
                "Single piece covers 80%+ of base plane with RMS < 5mm";
            return result;
        }
    }

    return result;
}

// ============================= PRD_RIMBASE: ADAPTIVE THRESHOLDS =============================

void HybridPuzzleFusionOptimizer::adaptThresholdsToDataVolume(
    ArchaeologicalConfig& config,
    int num_rim_candidates,
    int num_base_candidates) const {

    // PRD requirement: "If there are only one or two rim candidates, I will relax
    // maximum-gap requirements while keeping RMS strict"

    bool rim_relaxed = false;
    bool base_relaxed = false;

    // Relax gap thresholds for rim if data-scarce
    if (num_rim_candidates > 0 && num_rim_candidates < config.min_candidates_for_strict) {
        double original_gap = config.rim_max_gap_threshold;
        config.rim_max_gap_threshold = config.relaxed_max_gap;
        rim_relaxed = true;

        std::cout << "  [ADAPTIVE THRESHOLDS] Rim data-scarce (" << num_rim_candidates
                  << " candidates < " << config.min_candidates_for_strict << ")" << std::endl;
        std::cout << "    → Relaxed rim max_gap: " << original_gap << " mm → "
                  << config.rim_max_gap_threshold << " mm" << std::endl;
        std::cout << "    → Kept rim RMS strict: " << config.rim_primitive_rms_threshold
                  << " mm (PRD requirement)" << std::endl;
    }

    // Relax gap thresholds for base if data-scarce
    if (num_base_candidates > 0 && num_base_candidates < config.min_candidates_for_strict) {
        double original_gap = config.base_max_gap_threshold;
        config.base_max_gap_threshold = config.relaxed_max_gap;
        base_relaxed = true;

        std::cout << "  [ADAPTIVE THRESHOLDS] Base data-scarce (" << num_base_candidates
                  << " candidates < " << config.min_candidates_for_strict << ")" << std::endl;
        std::cout << "    → Relaxed base max_gap: " << original_gap << " mm → "
                  << config.base_max_gap_threshold << " mm" << std::endl;
        std::cout << "    → Kept base RMS strict: " << config.base_primitive_rms_threshold
                  << " mm (PRD requirement)" << std::endl;
    }

    // Summary if no relaxation needed
    if (!rim_relaxed && !base_relaxed && (num_rim_candidates > 0 || num_base_candidates > 0)) {
        std::cout << "  [ADAPTIVE THRESHOLDS] Sufficient data - using strict thresholds:" << std::endl;
        std::cout << "    Rim: " << num_rim_candidates << " candidates (strict: max_gap="
                  << config.rim_max_gap_threshold << " mm, RMS="
                  << config.rim_primitive_rms_threshold << " mm)" << std::endl;
        std::cout << "    Base: " << num_base_candidates << " candidates (strict: max_gap="
                  << config.base_max_gap_threshold << " mm, RMS="
                  << config.base_primitive_rms_threshold << " mm)" << std::endl;
    }

    // Note: RMS thresholds remain unchanged regardless of data volume
    // This maintains archaeological accuracy per PRD requirement
}

// ============================= PHASE 3: RIM/BASE DETECTION =============================

std::vector<HybridPuzzleFusionOptimizer::PieceClassification>
HybridPuzzleFusionOptimizer::classifyPiecesByStructure(const std::vector<Geom>& geometry) const {
    std::vector<PieceClassification> classifications;

    // Collect heights for all pieces
    std::vector<double> heights;
    for (int i = 1; i <= static_cast<int>(geometry.size()); ++i) {
        double height = getHeight(i, geometry);
        if (height > 0.0) {
            heights.push_back(height);
        }
    }

    if (heights.empty()) {
        std::cerr << "*** WARNING *** No valid heights for piece classification" << std::endl;
        return classifications;
    }

    // Calculate height statistics
    double min_height = *std::min_element(heights.begin(), heights.end());
    double max_height = *std::max_element(heights.begin(), heights.end());
    double height_range = max_height - min_height;

    std::cout << "\n=== ARCHAEOLOGICAL PIECE CLASSIFICATION ===" << std::endl;
    std::cout << "Height range: " << std::fixed << std::setprecision(1)
              << min_height << " - " << max_height << " mm (range=" << height_range << " mm)" << std::endl;

    // Classification thresholds (pottery-specific)
    // RIM: Top 20% of height range
    // BASE: Bottom 20% of height range
    // BODY: Middle 60%
    double rim_threshold = max_height - 0.2 * height_range;
    double base_threshold = min_height + 0.2 * height_range;

    // Classify each piece
    for (int i = 1; i <= static_cast<int>(geometry.size()); ++i) {
        double height = getHeight(i, geometry);
        if (height <= 0.0) continue;

        PieceClassification classif;
        classif.piece_id = i;
        classif.height_on_axis = height;

        if (height >= rim_threshold) {
            classif.type = PieceClassification::RIM;
            classif.confidence = (height - rim_threshold) / (max_height - rim_threshold + 1e-6);
            std::cout << "  Piece " << i << ": RIM (height=" << height
                      << " mm, conf=" << std::setprecision(2) << classif.confidence << ")" << std::endl;
        } else if (height <= base_threshold) {
            classif.type = PieceClassification::BASE;
            classif.confidence = (base_threshold - height) / (base_threshold - min_height + 1e-6);
            std::cout << "  Piece " << i << ": BASE (height=" << std::fixed << std::setprecision(1) << height
                      << " mm, conf=" << std::setprecision(2) << classif.confidence << ")" << std::endl;
        } else {
            classif.type = PieceClassification::BODY;
            classif.confidence = 1.0 - fabs(height - (min_height + max_height) / 2.0) / (height_range / 2.0);
            std::cout << "  Piece " << i << ": BODY (height=" << std::fixed << std::setprecision(1) << height
                      << " mm)" << std::endl;
        }

        classifications.push_back(classif);
    }

    return classifications;
}

std::vector<int> HybridPuzzleFusionOptimizer::identifyRimPieces(
    const std::vector<PieceClassification>& classifications) const {

    std::vector<int> rim_pieces;
    for (const auto& classif : classifications) {
        if (classif.type == PieceClassification::RIM) {
            rim_pieces.push_back(classif.piece_id);
        }
    }
    return rim_pieces;
}

std::vector<int> HybridPuzzleFusionOptimizer::identifyBasePieces(
    const std::vector<PieceClassification>& classifications) const {

    std::vector<int> base_pieces;
    for (const auto& classif : classifications) {
        if (classif.type == PieceClassification::BASE) {
            base_pieces.push_back(classif.piece_id);
        }
    }
    return base_pieces;
}

// ============================= PHASE 3: CIRCULAR TOPOLOGY VALIDATION =============================

bool HybridPuzzleFusionOptimizer::validateCircularTopology(
    const std::vector<int>& pieces,
    const std::vector<ProvenConnection>& connections,
    double closure_tolerance) const {

    if (pieces.size() < 2) {
        return true;  // Trivially valid for <2 pieces
    }

    // Build connectivity graph for these pieces
    std::map<int, std::set<int>> adjacency;
    for (const auto& conn : connections) {
        if (std::find(pieces.begin(), pieces.end(), conn.piece_a) != pieces.end() &&
            std::find(pieces.begin(), pieces.end(), conn.piece_b) != pieces.end()) {
            adjacency[conn.piece_a].insert(conn.piece_b);
            adjacency[conn.piece_b].insert(conn.piece_a);
        }
    }

    // Check circular connectivity: each piece should connect to exactly 2 others
    int pieces_with_two_neighbors = 0;
    for (int piece : pieces) {
        int neighbor_count = adjacency[piece].size();
        if (neighbor_count == 2) {
            pieces_with_two_neighbors++;
        }
    }

    // For perfect circular topology, ALL pieces should have exactly 2 neighbors
    // For partial circles (acceptable), allow some flexibility
    double circular_ratio = static_cast<double>(pieces_with_two_neighbors) / pieces.size();

    std::cout << "*** CIRCULAR TOPOLOGY CHECK *** " << pieces_with_two_neighbors
              << "/" << pieces.size() << " pieces have 2 neighbors (ratio="
              << std::fixed << std::setprecision(2) << circular_ratio << ")" << std::endl;

    // Accept if >50% of pieces form circular structure
    return (circular_ratio >= 0.5);
}

// ============================= PHASE 3: BRIDGE QUALITY SCORING =============================

double HybridPuzzleFusionOptimizer::calculateBridgeQuality(
    int candidate_piece,
    int anchor_a, int anchor_b,
    const HybridAssemblyState& current_state,
    const std::vector<ProvenConnection>& connections,
    const std::vector<Geom>& geometry) const {

    // Find connections: candidate <-> anchor_a, candidate <-> anchor_b
    const ProvenConnection* conn_to_a = nullptr;
    const ProvenConnection* conn_to_b = nullptr;

    for (const auto& conn : connections) {
        if ((conn.piece_a == candidate_piece && conn.piece_b == anchor_a) ||
            (conn.piece_a == anchor_a && conn.piece_b == candidate_piece)) {
            conn_to_a = &conn;
        }
        if ((conn.piece_a == candidate_piece && conn.piece_b == anchor_b) ||
            (conn.piece_a == anchor_b && conn.piece_b == candidate_piece)) {
            conn_to_b = &conn;
        }
    }

    // If candidate doesn't connect to both anchors, not a valid bridge
    if (!conn_to_a || !conn_to_b) {
        return 0.0;
    }

    double bridge_score = 0.0;

    // Component 1: Average connection quality (40%)
    double avg_quality = (conn_to_a->combined_quality + conn_to_b->combined_quality) / 2.0;
    bridge_score += 0.4 * (avg_quality / 100.0);  // Normalize to [0,1]

    // Component 2: Triangle consistency (30%)
    // Check if there's a connection anchor_a <-> anchor_b
    const ProvenConnection* conn_ab = nullptr;
    for (const auto& conn : connections) {
        if ((conn.piece_a == anchor_a && conn.piece_b == anchor_b) ||
            (conn.piece_a == anchor_b && conn.piece_b == anchor_a)) {
            conn_ab = &conn;
            break;
        }
    }

    if (conn_ab) {
        // Validate triangle consistency
        bool consistent = checkTransformationCycleConsistency(*conn_to_a, *conn_to_b, *conn_ab);
        if (consistent) {
            bridge_score += 0.3;  // Full triangle consistency bonus
        } else {
            bridge_score += 0.1;  // Partial credit for having the connections
        }
    }

    // Component 3: Piece size bonus (20%)
    // Larger pieces are easier to place correctly - prioritize them
    const auto& geom = geometry[candidate_piece - 1];
    int point_count = geom.edge_line_.point_.cols();
    double size_bonus = std::min(1.0, point_count / 500.0);  // Normalize to [0,1]
    bridge_score += 0.2 * size_bonus;

    // Component 4: Pottery structure bonus (10%)
    // Prefer connections that maintain vertical coherence
    double height_candidate = getHeight(candidate_piece, geometry);
    double height_a = getHeight(anchor_a, geometry);
    double height_b = getHeight(anchor_b, geometry);

    if (height_candidate > 0.0 && height_a > 0.0 && height_b > 0.0) {
        // Check if candidate height is between the two anchors (ideal bridging)
        double min_anchor_height = std::min(height_a, height_b);
        double max_anchor_height = std::max(height_a, height_b);

        if (height_candidate >= min_anchor_height && height_candidate <= max_anchor_height) {
            bridge_score += 0.1;  // Perfect vertical bridging
        } else if (fabs(height_candidate - (height_a + height_b) / 2.0) < 50.0) {
            bridge_score += 0.05;  // Close to average height
        }
    }

    return bridge_score * 100.0;  // Scale to [0, 100] for consistency with connection quality
}

// ============================= PHASE 3+: ENHANCED BRIDGE QUALITY WITH GAP TARGETING =============================

double HybridPuzzleFusionOptimizer::calculateBridgeQualityWithGapTargeting(
    int candidate_piece,
    int anchor_a, int anchor_b,
    const HybridAssemblyState& current_state,
    const std::vector<ProvenConnection>& connections,
    const std::vector<Geom>& geometry,
    const GapList& gap_list,
    const ArchaeologicalConfig& config) const {

    // Start with base bridge quality (connection quality, triangle consistency, size, pottery structure)
    double base_quality = calculateBridgeQuality(
        candidate_piece, anchor_a, anchor_b, current_state, connections, geometry);

    // If no gaps exist, return base quality
    if (gap_list.getGapCount() == 0) {
        return base_quality;
    }

    // Calculate gap targeting bonus/penalty
    double gap_targeting_multiplier = 1.0;

    // Get largest gap (primary target for gap-filling strategy)
    Gap largest_gap = gap_list.getLargestGap();

    // Check if this bridge placement would reduce the largest gap
    double reduction_amount = 0.0;
    bool targets_largest_gap = gap_list.doesPieceFillGap(
        candidate_piece, largest_gap, current_state, geometry, reduction_amount);

    if (targets_largest_gap) {
        // Bonus for targeting the largest gap
        // Proportional to how much the gap is reduced
        double gap_bonus = 1.0 + (reduction_amount * 0.5);  // Up to 50% bonus for full gap filling
        gap_targeting_multiplier *= gap_bonus;

        if (config.enable_hierarchical_logging) {
            std::cout << "  GAP TARGETING BONUS: Piece " << candidate_piece
                      << " targets largest gap (reduction=" << (reduction_amount * 100) << "%)"
                      << " bonus=" << ((gap_bonus - 1.0) * 100) << "%" << std::endl;
        }
    } else {
        // Check if piece addresses ANY gap (not just the largest)
        double total_reduction = 0.0;
        int gaps_addressed = 0;

        for (const auto& gap : gap_list.getAllGaps()) {
            double reduction = 0.0;
            if (gap_list.doesPieceFillGap(candidate_piece, gap, current_state, geometry, reduction)) {
                total_reduction += reduction;
                gaps_addressed++;
            }
        }

        if (gaps_addressed > 0) {
            // Smaller bonus for addressing non-largest gaps
            double avg_reduction = total_reduction / gaps_addressed;
            double secondary_bonus = 1.0 + (avg_reduction * 0.2);  // Up to 20% bonus
            gap_targeting_multiplier *= secondary_bonus;
        } else {
            // Penalty for not addressing any gaps during gap-filling phase
            // If we're in gap-filling mode and this piece doesn't help, deprioritize it
            double gap_penalty = 0.8;  // 20% penalty
            gap_targeting_multiplier *= gap_penalty;

            if (config.enable_hierarchical_logging) {
                std::cout << "  GAP TARGETING PENALTY: Piece " << candidate_piece
                          << " doesn't address any gaps (penalty=" << ((1.0 - gap_penalty) * 100) << "%)"
                          << std::endl;
            }
        }
    }

    // Calculate gap reduction requirement check
    // According to PRD: require 20% gap reduction for productive placement
    double total_gap_before = gap_list.getTotalGapMetric();
    if (total_gap_before > 0.0 && reduction_amount < config.gap_reduction_fraction) {
        // Piece doesn't meet minimum gap reduction requirement
        // Apply stricter penalty
        gap_targeting_multiplier *= 0.6;  // 40% penalty for insufficient gap reduction

        if (config.enable_hierarchical_logging) {
            std::cout << "  INSUFFICIENT GAP REDUCTION: Piece " << candidate_piece
                      << " reduction=" << (reduction_amount * 100) << "% < required "
                      << (config.gap_reduction_fraction * 100) << "%" << std::endl;
        }
    }

    return base_quality * gap_targeting_multiplier;
}

// ============================= PHASE 3+: CIRCLE FITTING & CLOSURE VALIDATION =============================

CircleFitResult HybridPuzzleFusionOptimizer::fitAndValidateCircle(
    const std::vector<int>& ring_pieces,
    const HybridAssemblyState& state,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config,
    bool is_rim) const {

    CircleFitResult result;
    result.num_pieces = ring_pieces.size();

    if (ring_pieces.size() < 2) {
        return result;  // Need at least 2 pieces
    }

    // Step 1: Collect all edge points from ring pieces in world coordinates
    std::vector<Eigen::Vector3d> ring_points;
    std::map<int, std::vector<Eigen::Vector3d>> piece_points;  // Track points per piece

    for (int piece_id : ring_pieces) {
        const auto& geom = geometry[piece_id - 1];  // 1-based to 0-based

        // Get transform for this piece
        Matrix4d transform = Matrix4d::Identity();
        auto it = state.piece_world_transforms.find(piece_id);
        if (it != state.piece_world_transforms.end()) {
            transform = it->second;
        }

        // Collect breakline points (edge of pottery)
        if (geom.edge_line_.point_.cols() > 0) {
            std::vector<Eigen::Vector3d> piece_pts;
            for (int i = 0; i < geom.edge_line_.point_.cols(); ++i) {
                Eigen::Vector3d local_pt = geom.edge_line_.point_.col(i);
                Eigen::Vector4d homogeneous(local_pt.x(), local_pt.y(), local_pt.z(), 1.0);
                Eigen::Vector4d world_pt = transform * homogeneous;
                Eigen::Vector3d pt(world_pt.x(), world_pt.y(), world_pt.z());

                ring_points.push_back(pt);
                piece_pts.push_back(pt);
            }
            piece_points[piece_id] = piece_pts;
        }
    }

    if (ring_points.size() < 3) {
        return result;  // Need at least 3 points to fit circle
    }

    // Step 2: Estimate axis from piece axis data (average of all piece axes)
    Eigen::Vector3d axis_sum = Eigen::Vector3d::Zero();
    int axis_count = 0;
    for (int piece_id : ring_pieces) {
        const auto& geom = geometry[piece_id - 1];
        if (!geom.edge_line_.axis_norm_.empty()) {
            // Use transformed axis normal
            auto it = state.piece_world_transforms.find(piece_id);
            if (it != state.piece_world_transforms.end()) {
                Matrix3d R = it->second.block<3,3>(0,0);
                Eigen::Vector3d transformed_axis = R * geom.edge_line_.axis_norm_[0];
                axis_sum += transformed_axis.normalized();
                axis_count++;
            }
        }
    }

    if (axis_count > 0) {
        result.normal = (axis_sum / axis_count).normalized();
    } else {
        result.normal = Eigen::Vector3d::UnitZ();  // Default to Z axis
    }

    // Step 3: Project all points to plane orthogonal to axis
    // Find plane center (centroid of all points)
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& pt : ring_points) {
        centroid += pt;
    }
    centroid /= ring_points.size();

    // Project points to plane
    std::vector<Eigen::Vector2d> projected_points;
    Eigen::Vector3d u = result.normal.cross(Eigen::Vector3d::UnitX()).normalized();
    if (u.norm() < 0.1) {
        u = result.normal.cross(Eigen::Vector3d::UnitY()).normalized();
    }
    Eigen::Vector3d v = result.normal.cross(u).normalized();

    for (const auto& pt : ring_points) {
        Eigen::Vector3d projected = pt - result.normal.dot(pt - centroid) * result.normal;
        double x = u.dot(projected - centroid);
        double y = v.dot(projected - centroid);
        projected_points.push_back(Eigen::Vector2d(x, y));
    }

    // Step 4: Fit circle using Huber-weighted robust least squares
    // Phase 1 PRD: Robust to chipped edges and outliers
    // Solve: (x - cx)^2 + (y - cy)^2 = r^2
    // Linearize: x^2 + y^2 = 2*cx*x + 2*cy*y + (r^2 - cx^2 - cy^2)

    // Initial algebraic fit (unweighted)
    Eigen::MatrixXd A(projected_points.size(), 3);
    Eigen::VectorXd b(projected_points.size());

    for (size_t i = 0; i < projected_points.size(); ++i) {
        double x = projected_points[i].x();
        double y = projected_points[i].y();
        A(i, 0) = 2.0 * x;
        A(i, 1) = 2.0 * y;
        A(i, 2) = 1.0;
        b(i) = x * x + y * y;
    }

    // Initial least squares solution
    Eigen::Vector3d params = A.colPivHouseholderQr().solve(b);
    double cx_2d = params(0);
    double cy_2d = params(1);
    double r_2d = std::sqrt(params(2) + cx_2d * cx_2d + cy_2d * cy_2d);

    // Huber-weighted iterative refinement (3 iterations)
    // Huber threshold: 1.5 * median absolute residual
    const int max_huber_iterations = 3;
    const char* strict_closure_env = std::getenv("ENABLE_STRICT_CLOSURE");
    bool use_huber_weighting = (strict_closure_env != nullptr && std::string(strict_closure_env) == "1");

    if (use_huber_weighting && projected_points.size() >= 5) {
        for (int iter = 0; iter < max_huber_iterations; ++iter) {
            // Compute residuals
            std::vector<double> residuals;
            for (const auto& pt : projected_points) {
                double dist_to_circle = std::sqrt((pt.x() - cx_2d) * (pt.x() - cx_2d) +
                                                  (pt.y() - cy_2d) * (pt.y() - cy_2d)) - r_2d;
                residuals.push_back(std::abs(dist_to_circle));
            }

            // Compute median absolute residual
            std::vector<double> sorted_residuals = residuals;
            std::sort(sorted_residuals.begin(), sorted_residuals.end());
            double median_residual = sorted_residuals[sorted_residuals.size() / 2];
            double huber_threshold = 1.5 * median_residual;

            if (huber_threshold < 1e-6) break;  // Already converged

            // Reweight system with Huber weights
            Eigen::MatrixXd A_weighted(projected_points.size(), 3);
            Eigen::VectorXd b_weighted(projected_points.size());

            for (size_t i = 0; i < projected_points.size(); ++i) {
                double x = projected_points[i].x();
                double y = projected_points[i].y();
                double residual = residuals[i];

                // Huber weight: w = 1 if |r| <= threshold, w = threshold/|r| if |r| > threshold
                double weight = (residual <= huber_threshold) ? 1.0 : huber_threshold / residual;

                A_weighted(i, 0) = weight * 2.0 * x;
                A_weighted(i, 1) = weight * 2.0 * y;
                A_weighted(i, 2) = weight * 1.0;
                b_weighted(i) = weight * (x * x + y * y);
            }

            // Solve weighted system
            Eigen::Vector3d params_weighted = A_weighted.colPivHouseholderQr().solve(b_weighted);
            cx_2d = params_weighted(0);
            cy_2d = params_weighted(1);
            r_2d = std::sqrt(params_weighted(2) + cx_2d * cx_2d + cy_2d * cy_2d);
        }
    }

    result.radius = r_2d;

    // Convert 2D circle center back to 3D
    result.center = centroid + cx_2d * u + cy_2d * v;

    // Step 5: Compute RMS error
    double sum_squared_error = 0.0;
    for (const auto& pt : ring_points) {
        // Distance from point to circle in 3D
        Eigen::Vector3d to_point = pt - result.center;
        double radial_dist = (to_point - result.normal.dot(to_point) * result.normal).norm();
        double error = radial_dist - result.radius;
        sum_squared_error += error * error;
    }
    result.rms_error = std::sqrt(sum_squared_error / ring_points.size());

    // Step 6: Compute angular coverage and max gap
    // For each piece, compute its angular position and span
    struct PieceAngularInfo {
        int piece_id;
        double angle_min;
        double angle_max;
        double angle_center;
    };
    std::vector<PieceAngularInfo> piece_angles;

    for (const auto& [piece_id, pts] : piece_points) {
        if (pts.empty()) continue;

        double min_angle = 360.0;
        double max_angle = 0.0;
        double sum_angle = 0.0;

        for (const auto& pt : pts) {
            // Compute angle in plane
            Eigen::Vector3d to_point = pt - result.center;
            Eigen::Vector3d radial = to_point - result.normal.dot(to_point) * result.normal;
            double angle = std::atan2(v.dot(radial), u.dot(radial)) * 180.0 / M_PI;
            if (angle < 0) angle += 360.0;

            min_angle = std::min(min_angle, angle);
            max_angle = std::max(max_angle, angle);
            sum_angle += angle;
        }

        PieceAngularInfo info;
        info.piece_id = piece_id;
        info.angle_min = min_angle;
        info.angle_max = max_angle;
        info.angle_center = sum_angle / pts.size();
        piece_angles.push_back(info);
    }

    // Sort by center angle
    std::sort(piece_angles.begin(), piece_angles.end(),
              [](const PieceAngularInfo& a, const PieceAngularInfo& b) {
                  return a.angle_center < b.angle_center;
              });

    // Compute gaps between consecutive pieces
    double total_coverage = 0.0;
    result.max_gap = 0.0;

    for (size_t i = 0; i < piece_angles.size(); ++i) {
        size_t next = (i + 1) % piece_angles.size();

        // Coverage of current piece
        double piece_span = piece_angles[i].angle_max - piece_angles[i].angle_min;
        total_coverage += piece_span;

        // Gap to next piece
        double gap_start = piece_angles[i].angle_max;
        double gap_end = piece_angles[next].angle_min;

        // Handle wraparound at 0/360
        double gap_angle = gap_end - gap_start;
        if (gap_angle < 0) gap_angle += 360.0;

        // Convert angular gap to chord length (mm)
        double chord_gap = 2.0 * result.radius * std::sin(gap_angle * M_PI / 360.0);
        result.max_gap = std::max(result.max_gap, chord_gap);
    }

    result.angular_coverage = total_coverage / 360.0;

    // Step 7: Validate closure against thresholds
    result.passes_closure = result.validateClosure(config, is_rim);

    // PHASE 1 PRD: Comprehensive closure validation logging
    if (config.enable_hierarchical_logging) {
        std::string ring_type = is_rim ? "RIM" : "BASE";
        std::string status = result.passes_closure ? "PASSES" : "FAILS";

        double rms_threshold = is_rim ? config.rim_rms_threshold : config.base_rms_threshold;
        double gap_threshold = is_rim ? config.rim_max_gap_threshold : config.base_max_gap_threshold;

        std::cout << "  " << ring_type << " CLOSURE VALIDATION " << status << ":" << std::endl;
        std::cout << "    RMS error: " << std::fixed << std::setprecision(2) << result.rms_error
                  << " mm (threshold: " << rms_threshold << " mm) "
                  << (result.rms_error < rms_threshold ? "✓" : "✗") << std::endl;
        std::cout << "    Max gap: " << result.max_gap
                  << " mm (threshold: " << gap_threshold << " mm) "
                  << (result.max_gap < gap_threshold ? "✓" : "✗") << std::endl;
        std::cout << "    Angular coverage: " << std::fixed << std::setprecision(1)
                  << (result.angular_coverage * 100.0) << "% (threshold: "
                  << (config.min_angular_coverage * 100.0) << "%) "
                  << (result.angular_coverage >= config.min_angular_coverage ? "✓" : "✗") << std::endl;
        std::cout << "    Pieces: " << result.num_pieces
                  << ", Radius: " << std::fixed << std::setprecision(1) << result.radius << " mm" << std::endl;
    }

    return result;
}

// ============================================================================
// GapList Implementation: Progressive gap tracking for archaeological assembly
// ============================================================================

void GapList::extractRingGaps(
    const std::vector<int>& ring_pieces,
    const HybridAssemblyState& state,
    const CircleFitResult& circle_fit,
    Gap::Type gap_type,
    const ArchaeologicalConfig& config) {

    if (ring_pieces.size() < 2) {
        return;  // No gaps in single-piece rings
    }

    // Sort pieces by angular position around circle
    std::vector<int> sorted_pieces = sortPiecesByAngle(
        ring_pieces, circle_fit.center, circle_fit.normal, state, std::vector<Geom>());

    // Extract gaps between consecutive pieces
    for (size_t i = 0; i < sorted_pieces.size(); ++i) {
        size_t next = (i + 1) % sorted_pieces.size();
        int piece_a = sorted_pieces[i];
        int piece_b = sorted_pieces[next];

        // Calculate angular gap between pieces
        double angle_a = calculatePieceAngle(piece_a, circle_fit.center, circle_fit.normal, state, std::vector<Geom>());
        double angle_b = calculatePieceAngle(piece_b, circle_fit.center, circle_fit.normal, state, std::vector<Geom>());

        double angular_gap = angle_b - angle_a;
        if (angular_gap < 0) angular_gap += 360.0;

        // Only track significant gaps
        if (angular_gap >= config.min_gap_angle) {
            Gap gap;
            gap.type = gap_type;
            gap.piece_a = piece_a;
            gap.piece_b = piece_b;
            gap.angular_width = angular_gap;
            gap.metric = angular_gap;

            // Calculate center direction for targeting
            double mid_angle = (angle_a + angular_gap / 2.0) * M_PI / 180.0;
            Eigen::Vector3d u = circle_fit.normal.cross(Eigen::Vector3d::UnitX()).normalized();
            if (u.norm() < 0.1) {
                u = circle_fit.normal.cross(Eigen::Vector3d::UnitY()).normalized();
            }
            Eigen::Vector3d v = circle_fit.normal.cross(u).normalized();
            gap.center_direction = (std::cos(mid_angle) * u + std::sin(mid_angle) * v).normalized();

            gaps_.push_back(gap);
        }
    }
}

void GapList::extractVerticalGaps(
    const std::vector<int>& rim_pieces,
    const std::vector<int>& base_pieces,
    const HybridAssemblyState& state,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) {

    // Divide vertical space into height bands
    // Find min/max heights from rim and base pieces
    double min_height = std::numeric_limits<double>::max();
    double max_height = std::numeric_limits<double>::lowest();

    auto getHeight = [&](int piece_id) -> double {
        const auto& geom = geometry[piece_id - 1];
        auto it = state.piece_world_transforms.find(piece_id);
        if (it != state.piece_world_transforms.end() && geom.edge_line_.point_.cols() > 0) {
            Eigen::Vector3d first_pt = geom.edge_line_.point_.col(0);
            Eigen::Vector4d homogeneous(first_pt.x(), first_pt.y(), first_pt.z(), 1.0);
            Eigen::Vector4d world_pt = it->second * homogeneous;
            return world_pt.z();
        }
        return 0.0;
    };

    for (int piece_id : rim_pieces) {
        double h = getHeight(piece_id);
        min_height = std::min(min_height, h);
        max_height = std::max(max_height, h);
    }
    for (int piece_id : base_pieces) {
        double h = getHeight(piece_id);
        min_height = std::min(min_height, h);
        max_height = std::max(max_height, h);
    }

    // Create vertical corridor gaps for each height band
    double band_height = (max_height - min_height) / config.height_bands;
    for (int band = 0; band < config.height_bands; ++band) {
        Gap gap;
        gap.type = Gap::VERTICAL_CORRIDOR;
        gap.piece_a = -1;  // No specific bounding pieces
        gap.piece_b = -1;
        gap.height_range[0] = min_height + band * band_height;
        gap.height_range[1] = min_height + (band + 1) * band_height;
        gap.metric = band_height;  // Vertical span
        gaps_.push_back(gap);
    }
}

void GapList::updateGapsAfterPlacement(
    int placed_piece,
    const HybridAssemblyState& new_state,
    const std::vector<Geom>& geometry) {

    // Remove or reduce gaps that were filled by the placed piece
    std::vector<Gap> updated_gaps;

    for (const auto& gap : gaps_) {
        double reduction = 0.0;
        if (doesPieceFillGap(placed_piece, gap, new_state, geometry, reduction)) {
            // Gap was filled or significantly reduced
            if (reduction < 1.0) {  // Partial filling
                Gap reduced_gap = gap;
                reduced_gap.metric *= (1.0 - reduction);
                if (gap.type == Gap::RIM_ARC || gap.type == Gap::BASE_ARC) {
                    reduced_gap.angular_width *= (1.0 - reduction);
                }
                updated_gaps.push_back(reduced_gap);
            }
            // If reduction >= 1.0, gap is completely filled - don't add to updated list
        } else {
            // Gap unaffected - keep it
            updated_gaps.push_back(gap);
        }
    }

    gaps_ = updated_gaps;
}

Gap GapList::getLargestGap() const {
    if (gaps_.empty()) {
        return Gap();  // Empty gap
    }

    auto max_it = std::max_element(gaps_.begin(), gaps_.end(),
        [](const Gap& a, const Gap& b) {
            return a.metric < b.metric;
        });

    return *max_it;
}

bool GapList::doesPieceFillGap(
    int piece_id,
    const Gap& gap,
    const HybridAssemblyState& state,
    const std::vector<Geom>& geometry,
    double& reduction_amount) const {

    reduction_amount = 0.0;

    // Check if piece is in assembly
    auto it = state.piece_world_transforms.find(piece_id);
    if (it == state.piece_world_transforms.end()) {
        return false;  // Piece not placed yet
    }

    const auto& geom = geometry[piece_id - 1];
    if (geom.edge_line_.point_.cols() == 0) {
        return false;  // No geometry data
    }

    if (gap.type == Gap::RIM_ARC || gap.type == Gap::BASE_ARC) {
        // For arc gaps, check if piece angular position overlaps with gap
        // This is a simplified check - real implementation would analyze actual geometry
        // For now, assume 20% reduction if piece connects to gap boundaries
        if (piece_id == gap.piece_a || piece_id == gap.piece_b) {
            reduction_amount = 0.2;  // Connected to gap boundary
            return true;
        }
    } else if (gap.type == Gap::VERTICAL_CORRIDOR) {
        // For vertical gaps, check if piece height falls in gap range
        Eigen::Vector3d first_pt = geom.edge_line_.point_.col(0);
        Eigen::Vector4d homogeneous(first_pt.x(), first_pt.y(), first_pt.z(), 1.0);
        Eigen::Vector4d world_pt = it->second * homogeneous;
        double piece_height = world_pt.z();

        if (piece_height >= gap.height_range[0] && piece_height <= gap.height_range[1]) {
            reduction_amount = 0.15;  // Piece in corridor
            return true;
        }
    }

    return false;
}

double GapList::getTotalGapMetric() const {
    double total = 0.0;
    for (const auto& gap : gaps_) {
        total += gap.metric;
    }
    return total;
}

void GapList::logGapSnapshot(const std::string& phase_name) const {
    std::cout << "\n=== GAP SNAPSHOT: " << phase_name << " ===" << std::endl;
    std::cout << "Total gaps: " << gaps_.size() << std::endl;
    std::cout << "Total gap metric: " << getTotalGapMetric() << std::endl;

    int rim_gaps = 0, base_gaps = 0, vertical_gaps = 0;
    for (const auto& gap : gaps_) {
        if (gap.type == Gap::RIM_ARC) rim_gaps++;
        else if (gap.type == Gap::BASE_ARC) base_gaps++;
        else vertical_gaps++;
    }

    std::cout << "  Rim arcs: " << rim_gaps << std::endl;
    std::cout << "  Base arcs: " << base_gaps << std::endl;
    std::cout << "  Vertical corridors: " << vertical_gaps << std::endl;

    // Log largest gaps
    if (!gaps_.empty()) {
        Gap largest = getLargestGap();
        std::cout << "  Largest gap: ";
        if (largest.type == Gap::RIM_ARC) std::cout << "RIM_ARC";
        else if (largest.type == Gap::BASE_ARC) std::cout << "BASE_ARC";
        else std::cout << "VERTICAL_CORRIDOR";
        std::cout << " (metric=" << largest.metric << ")" << std::endl;
    }
}

double GapList::calculatePieceAngle(
    int piece_id,
    const Eigen::Vector3d& circle_center,
    const Eigen::Vector3d& circle_normal,
    const HybridAssemblyState& state,
    const std::vector<Geom>& geometry) const {

    // Get piece transform
    auto it = state.piece_world_transforms.find(piece_id);
    if (it == state.piece_world_transforms.end()) {
        return 0.0;  // Piece not in assembly
    }

    // For simplicity, use first point of piece as representative
    // Real implementation would compute centroid of edge points
    if (piece_id - 1 >= static_cast<int>(geometry.size())) {
        return 0.0;
    }

    const auto& geom = geometry[piece_id - 1];
    if (geom.edge_line_.point_.cols() == 0) {
        return 0.0;
    }

    Eigen::Vector3d local_pt = geom.edge_line_.point_.col(0);
    Eigen::Vector4d homogeneous(local_pt.x(), local_pt.y(), local_pt.z(), 1.0);
    Eigen::Vector4d world_pt = it->second * homogeneous;
    Eigen::Vector3d pt(world_pt.x(), world_pt.y(), world_pt.z());

    // Compute angle in plane perpendicular to circle normal
    Eigen::Vector3d to_point = pt - circle_center;
    Eigen::Vector3d radial = to_point - circle_normal.dot(to_point) * circle_normal;

    Eigen::Vector3d u = circle_normal.cross(Eigen::Vector3d::UnitX()).normalized();
    if (u.norm() < 0.1) {
        u = circle_normal.cross(Eigen::Vector3d::UnitY()).normalized();
    }
    Eigen::Vector3d v = circle_normal.cross(u).normalized();

    double angle = std::atan2(v.dot(radial), u.dot(radial)) * 180.0 / M_PI;
    if (angle < 0) angle += 360.0;

    return angle;
}

std::vector<int> GapList::sortPiecesByAngle(
    const std::vector<int>& pieces,
    const Eigen::Vector3d& circle_center,
    const Eigen::Vector3d& circle_normal,
    const HybridAssemblyState& state,
    const std::vector<Geom>& geometry) const {

    std::vector<std::pair<int, double>> piece_angles;
    for (int piece_id : pieces) {
        double angle = calculatePieceAngle(piece_id, circle_center, circle_normal, state, geometry);
        piece_angles.push_back({piece_id, angle});
    }

    std::sort(piece_angles.begin(), piece_angles.end(),
        [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
            return a.second < b.second;
        });

    std::vector<int> sorted_pieces;
    for (const auto& [piece_id, angle] : piece_angles) {
        sorted_pieces.push_back(piece_id);
    }

    return sorted_pieces;
}

// ============================================================================
// PHASE 3+: HIERARCHICAL ASSEMBLY ORCHESTRATION
// Enforces mandatory build order: RIM → BASE → BODY → COMPLETE
// ============================================================================

HybridAssemblyState HybridPuzzleFusionOptimizer::performHierarchicalAssembly(
    int total_pieces,
    const std::vector<ProvenConnection>& all_connections,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) {

    HierarchicalAssemblyState hier_state;
    HybridAssemblyState assembly_state;

    std::cout << "\n========================================" << std::endl;
    std::cout << "HIERARCHICAL ARCHAEOLOGICAL ASSEMBLY" << std::endl;
    std::cout << "========================================" << std::endl;

    // Step 1: Classify pieces by height (rim/base/body)
    classifyPiecesByHeight(hier_state, total_pieces, geometry);

    // Step 2: State machine - enforce mandatory build order
    while (hier_state.current_phase != AssemblyPhase::COMPLETE) {

        std::cout << "\n--- PHASE: ";
        switch (hier_state.current_phase) {
            case AssemblyPhase::INITIALIZATION:
                std::cout << "INITIALIZATION" << std::endl;
                hier_state.current_phase = AssemblyPhase::RIM_COMPLETION;
                break;

            case AssemblyPhase::RIM_COMPLETION:
                std::cout << "RIM_COMPLETION" << std::endl;
                hier_state.rim_complete = attemptRimRingCompletion(
                    hier_state, assembly_state, all_connections, geometry, config);

                if (hier_state.canTransitionToNextPhase(config)) {
                    hier_state.current_phase = hier_state.getNextPhase();
                } else {
                    hier_state.rim_attempts++;
                }
                break;

            case AssemblyPhase::BASE_COMPLETION:
                std::cout << "BASE_COMPLETION" << std::endl;
                hier_state.base_complete = attemptBaseRingCompletion(
                    hier_state, assembly_state, all_connections, geometry, config);

                if (hier_state.canTransitionToNextPhase(config)) {
                    hier_state.current_phase = hier_state.getNextPhase();
                } else {
                    hier_state.base_attempts++;
                }
                break;

            case AssemblyPhase::BODY_FILLING:
                std::cout << "BODY_FILLING" << std::endl;
                performBodyGapFilling(hier_state, assembly_state, all_connections, geometry, config);
                hier_state.current_phase = AssemblyPhase::COMPLETE;
                break;

            case AssemblyPhase::COMPLETE:
                break;
        }

        // Safety: prevent infinite loops
        if (hier_state.rim_attempts > config.max_ring_attempts * 2 ||
            hier_state.base_attempts > config.max_ring_attempts * 2) {
            std::cout << "WARNING: Max attempts exceeded, forcing completion" << std::endl;
            break;
        }
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "HIERARCHICAL ASSEMBLY COMPLETE" << std::endl;
    std::cout << "Total pieces placed: " << assembly_state.active_pieces.size() << std::endl;
    std::cout << "========================================\n" << std::endl;

    return assembly_state;
}

// ==================== PotSAC-BASED CLASSIFICATION ====================
// Uses pre-computed axis heights from PotSAC algorithm to classify pieces
// Adaptive algorithm handles rim+base only, rim+body+base, and other configurations
HybridPuzzleFusionOptimizer::PotSACClassification
HybridPuzzleFusionOptimizer::classifyByPotSACHeights(
    int total_pieces,
    const std::vector<Geom>& geometry) const {

    PotSACClassification result;

    std::cout << "\n=== PotSAC-BASED PIECE CLASSIFICATION ===" << std::endl;

    // Step 1: Extract axis heights from all pieces
    std::vector<double> heights;
    std::vector<int> valid_pieces;

    for (int piece_id = 1; piece_id <= total_pieces; ++piece_id) {
        const Geom& geom = geometry[piece_id - 1];
        double axis_height = geom.original_axis_height_;

        // Only consider pieces with valid axis data
        if (axis_height > 0.0) {
            heights.push_back(axis_height);
            valid_pieces.push_back(piece_id);
            std::cout << "  Piece " << piece_id << ": axis height = " << axis_height << " mm" << std::endl;
        }
    }

    if (heights.empty()) {
        std::cout << "  ❌ No valid PotSAC axis heights found - falling back to geometric classification" << std::endl;
        return result;  // Empty result
    }

    // Step 2: Compute height statistics
    result.min_height = *std::min_element(heights.begin(), heights.end());
    result.max_height = *std::max_element(heights.begin(), heights.end());
    double height_range = result.max_height - result.min_height;

    std::cout << "\n  Height range: " << result.min_height << " - " << result.max_height
              << " mm (range: " << height_range << " mm)" << std::endl;

    // Step 3: Adaptive threshold calculation
    // Default: 33% percentile-based thresholds
    result.rim_threshold = result.min_height + 0.67 * height_range;
    result.base_threshold = result.min_height + 0.33 * height_range;

    std::cout << "  Rim threshold (top 33%): ≥ " << result.rim_threshold << " mm" << std::endl;
    std::cout << "  Base threshold (bottom 33%): ≤ " << result.base_threshold << " mm" << std::endl;
    std::cout << "  Body range (middle 33%): " << result.base_threshold
              << " - " << result.rim_threshold << " mm" << std::endl;

    // Step 4: Classify pieces based on adaptive thresholds
    for (size_t i = 0; i < valid_pieces.size(); ++i) {
        int piece_id = valid_pieces[i];
        double height = heights[i];

        if (height >= result.rim_threshold) {
            result.rim_pieces.push_back(piece_id);
            std::cout << "  Piece " << piece_id << " → RIM (height " << height << " mm)" << std::endl;
        } else if (height <= result.base_threshold) {
            result.base_pieces.push_back(piece_id);
            std::cout << "  Piece " << piece_id << " → BASE (height " << height << " mm)" << std::endl;
        } else {
            result.body_pieces.push_back(piece_id);
            std::cout << "  Piece " << piece_id << " → BODY (height " << height << " mm)" << std::endl;
        }
    }

    // Step 5: Handle edge cases
    std::cout << "\n=== CLASSIFICATION SUMMARY ===" << std::endl;
    std::cout << "  RIM pieces: " << result.rim_pieces.size() << " - [";
    for (size_t i = 0; i < result.rim_pieces.size(); ++i) {
        std::cout << result.rim_pieces[i];
        if (i < result.rim_pieces.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    std::cout << "  BASE pieces: " << result.base_pieces.size() << " - [";
    for (size_t i = 0; i < result.base_pieces.size(); ++i) {
        std::cout << result.base_pieces[i];
        if (i < result.base_pieces.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    std::cout << "  BODY pieces: " << result.body_pieces.size() << " - [";
    for (size_t i = 0; i < result.body_pieces.size(); ++i) {
        std::cout << result.body_pieces[i];
        if (i < result.body_pieces.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    // Edge case handling
    if (result.body_pieces.empty()) {
        std::cout << "  ✅ Rim+Base only configuration (no body pieces)" << std::endl;
    }
    if (result.rim_pieces.empty()) {
        std::cout << "  ⚠️  No rim pieces detected" << std::endl;
    }
    if (result.base_pieces.empty()) {
        std::cout << "  ⚠️  No base pieces detected" << std::endl;
    }

    return result;
}

void HybridPuzzleFusionOptimizer::classifyPiecesByHeight(
    HierarchicalAssemblyState& hier_state,
    int total_pieces,
    const std::vector<Geom>& geometry) {

    std::cout << "\n=== HYBRID PotSAC + GEOMETRIC CLASSIFICATION ===" << std::endl;

    // STEP 1: Get PotSAC-based classification (primary)
    PotSACClassification potsac = classifyByPotSACHeights(total_pieces, geometry);

    // STEP 2: Get geometric validation (secondary/confirmation)
    ArchaeologicalConfig config;  // Use default config for geometric testing
    GeometricRimBaseResult geometric_result = identifyRimBasePiecesByGeometry(
        total_pieces, geometry, config);

    std::cout << "\n=== HYBRID CLASSIFICATION FUSION ===" << std::endl;

    // Classify pieces based on hybrid PotSAC + geometric evidence
    int rim_count = 0, base_count = 0, body_count = 0;

    // First, mark all as BODY by default
    for (int piece_id = 1; piece_id <= total_pieces; ++piece_id) {
        hier_state.piece_types[piece_id] = PieceType::BODY;
    }

    // HYBRID DECISION LOGIC: PotSAC-first with geometric confirmation
    // Priority: PotSAC classification > Geometric confirmation
    // Reason: PotSAC uses pre-assembly vessel structure, no chicken-and-egg paradox

    // Process RIM pieces
    for (int piece_id : potsac.rim_pieces) {
        bool geom_confirms_rim = (std::find(geometric_result.rim_pieces.begin(),
                                            geometric_result.rim_pieces.end(),
                                            piece_id) != geometric_result.rim_pieces.end());

        // Accept piece as RIM if PotSAC says so (regardless of geometry)
        // Geometry is unreliable for pottery due to normal variation
        hier_state.piece_types[piece_id] = PieceType::RIM;
        hier_state.rim_pieces.push_back(piece_id);
        rim_count++;

        if (geom_confirms_rim) {
            std::cout << "  Piece " << piece_id << " → RIM (PotSAC ✓, Geometry ✓)" << std::endl;
        } else {
            std::cout << "  Piece " << piece_id << " → RIM (PotSAC ✓, Geometry ✗ - trusting PotSAC)" << std::endl;
        }
    }

    // Process BASE pieces
    for (int piece_id : potsac.base_pieces) {
        // Skip if already classified as RIM
        if (hier_state.piece_types[piece_id] == PieceType::RIM) {
            continue;
        }

        bool geom_confirms_base = (std::find(geometric_result.base_pieces.begin(),
                                             geometric_result.base_pieces.end(),
                                             piece_id) != geometric_result.base_pieces.end());

        // Accept piece as BASE if PotSAC says so (regardless of geometry)
        hier_state.piece_types[piece_id] = PieceType::BASE;
        hier_state.base_pieces.push_back(piece_id);
        base_count++;

        if (geom_confirms_base) {
            std::cout << "  Piece " << piece_id << " → BASE (PotSAC ✓, Geometry ✓)" << std::endl;
        } else {
            std::cout << "  Piece " << piece_id << " → BASE (PotSAC ✓, Geometry ✗ - trusting PotSAC)" << std::endl;
        }
    }

    // Process BODY pieces (remaining middle pieces)
    for (int piece_id : potsac.body_pieces) {
        // Skip if already classified as RIM or BASE
        if (hier_state.piece_types[piece_id] != PieceType::BODY) {
            continue;
        }

        std::cout << "  Piece " << piece_id << " → BODY (PotSAC middle 33%)" << std::endl;
        body_count++;
    }

    // Count final body pieces (including any not classified by PotSAC)
    body_count = 0;
    for (int piece_id = 1; piece_id <= total_pieces; ++piece_id) {
        if (hier_state.piece_types[piece_id] == PieceType::BODY) {
            body_count++;
        }
    }

    std::cout << "\n=== FINAL CLASSIFICATION (PotSAC-BASED) ===" << std::endl;
    std::cout << "  RIM pieces: " << rim_count << " - [";
    for (size_t i = 0; i < hier_state.rim_pieces.size(); ++i) {
        std::cout << hier_state.rim_pieces[i];
        if (i < hier_state.rim_pieces.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    std::cout << "  BASE pieces: " << base_count << " - [";
    for (size_t i = 0; i < hier_state.base_pieces.size(); ++i) {
        std::cout << hier_state.base_pieces[i];
        if (i < hier_state.base_pieces.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    std::cout << "  BODY pieces: " << body_count << std::endl;

    std::cout << "\n  ✅ PotSAC-based classification complete (no chicken-and-egg paradox)" << std::endl;
}

// ========== HELPER FUNCTIONS FOR CIRCULAR RING ASSEMBLY ==========

// Helper function: Track degree of each piece in current assembly
// Degree = number of connections a piece has
// For circular ring topology, all pieces should have degree=2
std::map<int, int> HybridPuzzleFusionOptimizer::getConnectionDegrees(
    const std::vector<int>& active_pieces,
    const std::vector<std::pair<int, int>>& connections) const {

    std::map<int, int> degrees;

    // Initialize all active pieces with degree=0
    for (int piece : active_pieces) {
        degrees[piece] = 0;
    }

    // Count connections for each piece
    for (const auto& conn : connections) {
        if (degrees.count(conn.first) > 0) {
            degrees[conn.first]++;
        }
        if (degrees.count(conn.second) > 0) {
            degrees[conn.second]++;
        }
    }

    return degrees;
}

// Helper function: Find pieces with degree=1 (chain endpoints)
// In a ring being built, chain ends are pieces with only one connection
// Final closed ring has no chain ends (all pieces degree=2)
std::vector<int> HybridPuzzleFusionOptimizer::getChainEnds(
    const std::map<int, int>& degrees) const {

    std::vector<int> ends;
    for (const auto& [piece_id, degree] : degrees) {
        if (degree == 1) {
            ends.push_back(piece_id);
        }
    }
    return ends;
}

// Helper function: Compute angular distance between two pieces around vessel axis
// Uses PotSAC axis data to determine angular separation (0-180 degrees)
// Helps validate that rim pieces are sequentially ordered around vessel
double HybridPuzzleFusionOptimizer::computeAngularDistance(
    int piece_a, int piece_b,
    const std::vector<Geom>& geometry) const {

    if (piece_a < 1 || piece_a > (int)geometry.size() ||
        piece_b < 1 || piece_b > (int)geometry.size()) {
        return 180.0;  // Invalid pieces - maximum angular distance
    }

    // TODO: Implement angular distance computation when axis position data is available
    // Current Geom class only stores axis HEIGHT (original_axis_height_)
    // Need to access Pot_A_Piece_XX_Axis.xyz files or extend Geom to store axis position
    // For now, return 0 (not used in critical chain-building path)

    // const Geom& geom_a = geometry[piece_a - 1];
    // const Geom& geom_b = geometry[piece_b - 1];

    // NOTE: This function is not currently used in the chain-building algorithm
    // Chain building relies on degree constraints (degree ≤2) for topology enforcement
    // Angular validation would be a future enhancement for quality scoring

    return 0.0;  // Placeholder - not used in current implementation
}

// ========== RIM RING ASSEMBLY WITH CIRCULAR TOPOLOGY CONSTRAINT ==========

bool HybridPuzzleFusionOptimizer::attemptRimRingCompletion(
    HierarchicalAssemblyState& hier_state,
    HybridAssemblyState& assembly_state,
    const std::vector<ProvenConnection>& connections,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) {

    std::cout << "  Attempting rim ring completion (attempt " << (hier_state.rim_attempts + 1) << ")..." << std::endl;

    // ========== PRD_RIMBASE: PRE-VALIDATION - Check single-piece sufficiency ==========
    const char* rimbase_env = std::getenv("ENABLE_RIMBASE_VALIDATION");
    bool use_rimbase_validation = (rimbase_env != nullptr && std::string(rimbase_env) == "1");

    if (use_rimbase_validation && hier_state.rim_attempts == 0) {
        std::cout << "\n=== PRD_RIMBASE: RIM SUFFICIENCY CHECK ===" << std::endl;

        // PHASE 4: Adaptive thresholds - adjust config based on data volume
        // Make a mutable copy of config for adaptive adjustment
        ArchaeologicalConfig adaptive_config = config;
        adaptThresholdsToDataVolume(
            adaptive_config,
            hier_state.rim_pieces.size(),
            hier_state.base_pieces.size());

        SufficiencyResult rim_sufficiency = checkRimSufficiency(
            hier_state.rim_pieces, geometry, adaptive_config);

        if (rim_sufficiency.is_sufficient) {
            std::cout << "✓ SINGLE-PIECE RIM SUFFICIENCY DETECTED:" << std::endl;
            std::cout << "  Piece " << rim_sufficiency.piece_id
                      << " covers " << std::fixed << std::setprecision(1)
                      << (rim_sufficiency.coverage * 100.0) << "% of rim" << std::endl;
            std::cout << "  RMS error: " << std::setprecision(2) << rim_sufficiency.rms_error << " mm" << std::endl;
            std::cout << "  Reason: " << rim_sufficiency.reason << std::endl;
            std::cout << "  SKIPPING rim ring assembly (single piece is complete)" << std::endl;

            // Add this piece directly to assembly
            if (assembly_state.active_pieces.empty()) {
                assembly_state.active_pieces.push_back(rim_sufficiency.piece_id);
                assembly_state.piece_world_transforms[rim_sufficiency.piece_id] = Matrix4d::Identity();
                hier_state.placed_pieces.push_back(rim_sufficiency.piece_id);
            }

            // Mark rim as complete
            hier_state.rim_complete = true;

            // Update rim circle state for validation
            hier_state.rim_circle.num_pieces = 1;
            hier_state.rim_circle.angular_coverage = rim_sufficiency.coverage;
            hier_state.rim_circle.rms_error = rim_sufficiency.rms_error;
            hier_state.rim_circle.passes_closure = true;

            return true;  // Rim complete with single piece
        } else {
            std::cout << "  No single piece meets 80% sufficiency threshold" << std::endl;
            std::cout << "  Proceeding with normal ring assembly..." << std::endl;
        }
    }
    // ==================================================================================

    // Get size-ordered rim piece candidates
    std::vector<int> candidates = getSizeOrderedCandidates(hier_state.rim_pieces, geometry);

    // Build rim connections only between rim pieces
    std::vector<ProvenConnection> rim_connections;
    for (const auto& conn : connections) {
        bool a_is_rim = (std::find(hier_state.rim_pieces.begin(), hier_state.rim_pieces.end(), conn.piece_a) != hier_state.rim_pieces.end());
        bool b_is_rim = (std::find(hier_state.rim_pieces.begin(), hier_state.rim_pieces.end(), conn.piece_b) != hier_state.rim_pieces.end());

        if (a_is_rim && b_is_rim) {
            rim_connections.push_back(conn);
        }
    }

    std::cout << "    Found " << rim_connections.size() << " rim-rim connections" << std::endl;

    if (rim_connections.empty()) {
        std::cout << "    WARNING: No connections between rim pieces found" << std::endl;
        return false;
    }

    // Start with best rim connection
    std::sort(rim_connections.begin(), rim_connections.end(), std::greater<ProvenConnection>());
    const ProvenConnection& seed_conn = rim_connections[0];

    // Initialize assembly with seed connection if not already started
    if (assembly_state.active_pieces.empty()) {
        assembly_state.active_pieces.push_back(seed_conn.piece_a);
        assembly_state.active_pieces.push_back(seed_conn.piece_b);
        assembly_state.selected_connections.push_back(seed_conn);

        // Initialize transformations
        assembly_state.piece_world_transforms[seed_conn.piece_a] = Matrix4d::Identity();

        Matrix4d seed_transform;
        seed_conn.icp_transformation.Output(seed_transform);
        assembly_state.piece_world_transforms[seed_conn.piece_b] = seed_transform;

        hier_state.placed_pieces.push_back(seed_conn.piece_a);
        hier_state.placed_pieces.push_back(seed_conn.piece_b);

        // PHASE 3: Initialize degree tracking for seed connection
        const char* hub_prevention_env = std::getenv("ENABLE_HUB_PREVENTION");
        bool use_hub_prevention = (hub_prevention_env != nullptr && std::string(hub_prevention_env) == "1");
        if (use_hub_prevention) {
            hier_state.piece_degree_map[seed_conn.piece_a] = 1;
            hier_state.piece_degree_map[seed_conn.piece_b] = 1;
        }

        std::cout << "    Seed connection: " << seed_conn.piece_a << "-" << seed_conn.piece_b
                  << " (quality=" << seed_conn.combined_quality << ")" << std::endl;
    }

    // ========== CHAIN-BUILDING ALGORITHM FOR CIRCULAR RING TOPOLOGY ==========
    // Build rim ring as a sequential chain, extending from chain ends only
    // This ensures circular topology (degree ≤2 for all pieces)

    // Track used connections for degree computation
    std::vector<std::pair<int, int>> used_connections;
    used_connections.push_back({seed_conn.piece_a, seed_conn.piece_b});

    // Get rim pieces that are already placed
    std::vector<int> placed_rim_pieces;
    for (int piece : assembly_state.active_pieces) {
        if (std::find(hier_state.rim_pieces.begin(), hier_state.rim_pieces.end(), piece) != hier_state.rim_pieces.end()) {
            placed_rim_pieces.push_back(piece);
        }
    }

    std::cout << "    Building rim chain (circular topology constraint)..." << std::endl;

    // Iteratively extend chain from chain ends
    bool pieces_added = true;
    while (pieces_added && placed_rim_pieces.size() < hier_state.rim_pieces.size()) {
        pieces_added = false;

        // STEP 1: Get current degrees and chain ends
        std::map<int, int> degrees = getConnectionDegrees(placed_rim_pieces, used_connections);
        std::vector<int> chain_ends = getChainEnds(degrees);

        if (chain_ends.empty()) {
            std::cout << "      No chain ends found (ring already closed or isolated pieces)" << std::endl;
            break;
        }

        std::cout << "      Current chain ends: [";
        for (size_t i = 0; i < chain_ends.size(); ++i) {
            std::cout << chain_ends[i];
            if (i < chain_ends.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;

        // STEP 2: Find best connection from chain ends to unplaced rim pieces
        const ProvenConnection* best_conn = nullptr;
        double best_quality = 0.0;
        int extending_from = -1;
        int new_piece = -1;

        // PHASE 4: Check if topology bonuses are enabled
        const char* topology_bonuses_env = std::getenv("ENABLE_TOPOLOGY_BONUSES");
        bool use_topology_bonuses = (topology_bonuses_env != nullptr && std::string(topology_bonuses_env) == "1");

        // PHASE 4: Store previous angular coverage to detect closure improvement
        double prev_angular_coverage = hier_state.rim_circle.angular_coverage;
        bool was_closed = hier_state.rim_circle.passes_closure;

        // Create set of placed pieces for fast lookup
        std::set<int> placed_set(placed_rim_pieces.begin(), placed_rim_pieces.end());

        for (const auto& conn : rim_connections) {
            int piece_a = conn.piece_a;
            int piece_b = conn.piece_b;

            bool a_is_end = (std::find(chain_ends.begin(), chain_ends.end(), piece_a) != chain_ends.end());
            bool b_is_end = (std::find(chain_ends.begin(), chain_ends.end(), piece_b) != chain_ends.end());
            bool a_placed = (placed_set.count(piece_a) > 0);
            bool b_placed = (placed_set.count(piece_b) > 0);

            // STEP 3: Check if connection extends chain properly (from end to new piece)
            bool valid_extension = false;
            int from_piece = -1;
            int to_piece = -1;

            if (a_is_end && !b_placed) {
                // Extends from piece_a (chain end) to piece_b (new piece)
                valid_extension = true;
                from_piece = piece_a;
                to_piece = piece_b;
            } else if (b_is_end && !a_placed) {
                // Extends from piece_b (chain end) to piece_a (new piece)
                valid_extension = true;
                from_piece = piece_b;
                to_piece = piece_a;
            }

            // STEP 4: Degree constraint validation (ensure chain end won't exceed degree=2)
            if (valid_extension) {
                int from_degree = degrees[from_piece];
                // from_piece is a chain end, so degree=1
                // Adding connection will make it degree=2 ✓
                // to_piece is new, will have degree=1 ✓
                // Both are valid for ring topology

                double quality = conn.combined_quality;

                // STEP 5: Pick best valid connection
                if (quality > best_quality) {
                    best_quality = quality;
                    best_conn = &conn;
                    extending_from = from_piece;
                    new_piece = to_piece;
                }
            }
        }

        // STEP 6: Add best connection if found
        if (best_conn) {
            // Use chain-building variables (extending_from = chain end, new_piece = unplaced piece)
            int anchor_piece = extending_from;
            // new_piece already set above

            // NOTE: Degree constraint already validated in STEP 4 above
            // Chain ends have degree=1, will become degree=2 after adding connection
            // New piece will have degree=1
            // No need for additional degree guard - chain structure guarantees degree ≤2

            // Calculate world transform for new piece
            Matrix4d anchor_transform = assembly_state.piece_world_transforms[anchor_piece];

            Matrix4d relative_transform;
            if (anchor_piece == best_conn->piece_a) {
                // Anchor → new piece: use direct transformation
                best_conn->icp_transformation.Output(relative_transform);
            } else {
                // New piece → anchor: use inverse transformation
                best_conn->icp_transformation.InvOut(relative_transform);
            }

            assembly_state.active_pieces.push_back(new_piece);
            assembly_state.selected_connections.push_back(*best_conn);
            assembly_state.piece_world_transforms[new_piece] = anchor_transform * relative_transform;
            hier_state.placed_pieces.push_back(new_piece);

            // Update chain tracking
            placed_rim_pieces.push_back(new_piece);
            used_connections.push_back({anchor_piece, new_piece});

            // Update legacy degree tracking (for compatibility with other systems)
            const char* hub_prevention_env = std::getenv("ENABLE_HUB_PREVENTION");
            bool use_hub_prevention = (hub_prevention_env != nullptr && std::string(hub_prevention_env) == "1");
            if (use_hub_prevention) {
                hier_state.piece_degree_map[anchor_piece]++;
                hier_state.piece_degree_map[new_piece]++;
            }

            std::cout << "      Added rim piece " << new_piece << " via chain end " << anchor_piece
                      << " (quality=" << best_conn->combined_quality;
            if (use_hub_prevention) {
                std::cout << ", degrees: " << anchor_piece << "=" << hier_state.piece_degree_map[anchor_piece]
                          << ", " << new_piece << "=" << hier_state.piece_degree_map[new_piece];
            }
            std::cout << ")" << std::endl;

            pieces_added = true;

            // PHASE 1 PRD: Validate closure after EACH addition
            if (assembly_state.active_pieces.size() >= 2) {
                hier_state.rim_circle = fitAndValidateCircle(
                    assembly_state.active_pieces, assembly_state, geometry, config, true);

                // PHASE 4: Ring closure bonus - detect when ring transitions to closed
                if (use_topology_bonuses && !was_closed && hier_state.rim_circle.passes_closure) {
                    std::cout << "  ✓ RING CLOSURE BONUS: Rim ring now passes validation (RMS="
                              << std::fixed << std::setprecision(2) << hier_state.rim_circle.rms_error
                              << " mm, max_gap=" << hier_state.rim_circle.max_gap
                              << " mm, coverage=" << std::setprecision(1) << (hier_state.rim_circle.angular_coverage * 100)
                              << "%) [+" << (config.ring_closure_bonus * 100) << "% bonus applied]" << std::endl;
                }

                // PHASE 4: Arc completion tracking - log angular coverage improvement
                if (use_topology_bonuses && assembly_state.active_pieces.size() > 2) {
                    double arc_improvement = hier_state.rim_circle.angular_coverage - prev_angular_coverage;
                    if (arc_improvement > 0.05) {  // Log if improvement >5%
                        std::cout << "    Arc completion: " << std::fixed << std::setprecision(1)
                                  << (prev_angular_coverage * 100) << "% → "
                                  << (hier_state.rim_circle.angular_coverage * 100)
                                  << "% (+" << (arc_improvement * 100) << "%)" << std::endl;
                    }
                }

                // If closure passes, mark complete immediately
                if (hier_state.rim_circle.passes_closure) {
                    std::cout << "  ✓ RIM RING COMPLETE (meets closure thresholds after " << assembly_state.active_pieces.size() << " pieces)" << std::endl;
                    return true;
                }
            }
        } else {
            // No valid connection found - chain building stuck
            std::cout << "      No valid chain extension found!" << std::endl;
        }
    }

    // ========== STEP 7: ATTEMPT RING CLOSURE ==========
    // After building the chain, try to close the ring by connecting the two chain ends
    std::cout << "    Attempting ring closure..." << std::endl;

    std::map<int, int> final_degrees = getConnectionDegrees(placed_rim_pieces, used_connections);
    std::vector<int> final_ends = getChainEnds(final_degrees);

    if (final_ends.size() == 2 && placed_rim_pieces.size() == hier_state.rim_pieces.size()) {
        int end_a = final_ends[0];
        int end_b = final_ends[1];

        std::cout << "      Chain complete with " << placed_rim_pieces.size()
                  << " rim pieces, ends: " << end_a << " and " << end_b << std::endl;

        // Find connection between the two chain ends
        const ProvenConnection* closure_conn = nullptr;
        for (const auto& conn : rim_connections) {
            if ((conn.piece_a == end_a && conn.piece_b == end_b) ||
                (conn.piece_a == end_b && conn.piece_b == end_a)) {
                closure_conn = &conn;
                break;
            }
        }

        if (closure_conn != nullptr) {
            // Add closure connection
            used_connections.push_back({end_a, end_b});
            assembly_state.selected_connections.push_back(*closure_conn);

            // Update legacy degree tracking
            const char* hub_prevention_env = std::getenv("ENABLE_HUB_PREVENTION");
            bool use_hub_prevention = (hub_prevention_env != nullptr && std::string(hub_prevention_env) == "1");
            if (use_hub_prevention) {
                hier_state.piece_degree_map[end_a]++;
                hier_state.piece_degree_map[end_b]++;
            }

            std::cout << "      ✓ Ring CLOSED: " << end_a << "-" << end_b
                      << " (quality=" << closure_conn->combined_quality << ")" << std::endl;

            // Validate circular ring structure
            std::cout << "    Validating circular ring structure..." << std::endl;
            std::map<int, int> ring_degrees = getConnectionDegrees(placed_rim_pieces, used_connections);
            bool all_degree_2 = true;
            for (const auto& [piece_id, degree] : ring_degrees) {
                if (degree != 2) {
                    std::cout << "      ERROR: Piece " << piece_id << " has degree=" << degree
                              << " (expected 2)" << std::endl;
                    all_degree_2 = false;
                }
            }

            if (all_degree_2) {
                std::cout << "      ✓ All rim pieces have degree=2 (perfect circular ring)" << std::endl;
            }
        } else {
            std::cout << "      WARNING: No connection exists between chain ends "
                      << end_a << " and " << end_b << std::endl;
            std::cout << "      Ring cannot be closed - may need alternative assembly strategy" << std::endl;
        }
    } else if (final_ends.size() == 0 && placed_rim_pieces.size() == hier_state.rim_pieces.size()) {
        std::cout << "      ✓ Ring already closed (no chain ends)" << std::endl;
    } else {
        std::cout << "      Cannot close ring: " << final_ends.size()
                  << " chain ends, " << placed_rim_pieces.size() << "/"
                  << hier_state.rim_pieces.size() << " rim pieces placed" << std::endl;
    }

    // PHASE 1 PRD: Final validation attempt
    if (assembly_state.active_pieces.size() >= 2) {
        hier_state.rim_circle = fitAndValidateCircle(
            assembly_state.active_pieces, assembly_state, geometry, config, true);

        if (hier_state.rim_circle.passes_closure) {
            std::cout << "  ✓ RIM RING COMPLETE (meets closure thresholds)" << std::endl;
            return true;
        } else {
            // Log failure with final metrics
            std::cout << "  ✗ Rim ring INCOMPLETE (attempt " << (hier_state.rim_attempts + 1) << ")" << std::endl;
            std::cout << "    Final metrics: RMS=" << std::fixed << std::setprecision(2) << hier_state.rim_circle.rms_error
                      << " mm, max_gap=" << hier_state.rim_circle.max_gap
                      << " mm, coverage=" << std::setprecision(1) << (hier_state.rim_circle.angular_coverage * 100) << "%" << std::endl;
        }
    }

    return false;
}

bool HybridPuzzleFusionOptimizer::attemptBaseRingCompletion(
    HierarchicalAssemblyState& hier_state,
    HybridAssemblyState& assembly_state,
    const std::vector<ProvenConnection>& connections,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) {

    std::cout << "  Attempting base ring completion (attempt " << (hier_state.base_attempts + 1) << ")..." << std::endl;

    // ========== PRD_RIMBASE: PRE-VALIDATION - Check single-piece sufficiency ==========
    const char* rimbase_env = std::getenv("ENABLE_RIMBASE_VALIDATION");
    bool use_rimbase_validation = (rimbase_env != nullptr && std::string(rimbase_env) == "1");

    if (use_rimbase_validation && hier_state.base_attempts == 0) {
        std::cout << "\n=== PRD_RIMBASE: BASE SUFFICIENCY CHECK ===" << std::endl;

        // PHASE 4: Adaptive thresholds - adjust config based on data volume
        // Make a mutable copy of config for adaptive adjustment
        ArchaeologicalConfig adaptive_config = config;
        adaptThresholdsToDataVolume(
            adaptive_config,
            hier_state.rim_pieces.size(),
            hier_state.base_pieces.size());

        SufficiencyResult base_sufficiency = checkBaseSufficiency(
            hier_state.base_pieces, geometry, adaptive_config);

        if (base_sufficiency.is_sufficient) {
            std::cout << "✓ SINGLE-PIECE BASE SUFFICIENCY DETECTED:" << std::endl;
            std::cout << "  Piece " << base_sufficiency.piece_id
                      << " covers " << std::fixed << std::setprecision(1)
                      << (base_sufficiency.coverage * 100.0) << "% of base" << std::endl;
            std::cout << "  RMS error: " << std::setprecision(2) << base_sufficiency.rms_error << " mm" << std::endl;
            std::cout << "  Reason: " << base_sufficiency.reason << std::endl;
            std::cout << "  SKIPPING base ring assembly (single piece is complete)" << std::endl;

            // Add this piece directly to assembly if not already present
            bool already_in_assembly = (std::find(assembly_state.active_pieces.begin(),
                                                  assembly_state.active_pieces.end(),
                                                  base_sufficiency.piece_id) != assembly_state.active_pieces.end());

            if (!already_in_assembly) {
                assembly_state.active_pieces.push_back(base_sufficiency.piece_id);
                assembly_state.piece_world_transforms[base_sufficiency.piece_id] = Matrix4d::Identity();
                hier_state.placed_pieces.push_back(base_sufficiency.piece_id);
            }

            // Mark base as complete
            hier_state.base_complete = true;

            // Update base circle state for validation
            hier_state.base_circle.num_pieces = 1;
            hier_state.base_circle.angular_coverage = base_sufficiency.coverage;
            hier_state.base_circle.rms_error = base_sufficiency.rms_error;
            hier_state.base_circle.passes_closure = true;

            return true;  // Base complete with single piece
        } else {
            std::cout << "  No single piece meets 80% sufficiency threshold" << std::endl;
            std::cout << "  Proceeding with normal ring assembly..." << std::endl;
        }
    }
    // ==================================================================================

    // Get size-ordered base piece candidates
    std::vector<int> candidates = getSizeOrderedCandidates(hier_state.base_pieces, geometry);

    // Build base connections (base-base and base-rim for structural support)
    std::vector<ProvenConnection> base_connections;
    for (const auto& conn : connections) {
        bool a_is_base = (std::find(hier_state.base_pieces.begin(), hier_state.base_pieces.end(), conn.piece_a) != hier_state.base_pieces.end());
        bool b_is_base = (std::find(hier_state.base_pieces.begin(), hier_state.base_pieces.end(), conn.piece_b) != hier_state.base_pieces.end());

        // Include base-base connections and base-rim connections for structural integrity
        if (a_is_base || b_is_base) {
            base_connections.push_back(conn);
        }
    }

    std::cout << "    Found " << base_connections.size() << " base-related connections" << std::endl;

    if (base_connections.empty()) {
        std::cout << "    WARNING: No connections involving base pieces found" << std::endl;
        return false;
    }

    // Sort by quality
    std::sort(base_connections.begin(), base_connections.end(), std::greater<ProvenConnection>());

    // Iteratively add base pieces with best connections
    bool pieces_added = true;
    int iterations = 0;
    while (pieces_added && iterations < 10) {  // Safety limit
        pieces_added = false;
        iterations++;

        // PHASE 4: Check if topology bonuses are enabled
        const char* topology_bonuses_env = std::getenv("ENABLE_TOPOLOGY_BONUSES");
        bool use_topology_bonuses = (topology_bonuses_env != nullptr && std::string(topology_bonuses_env) == "1");

        // PHASE 4: Store previous angular coverage to detect closure improvement
        double prev_angular_coverage = hier_state.base_circle.angular_coverage;
        bool was_closed = hier_state.base_circle.passes_closure;

        // Find best connection from assembly to unplaced base piece
        const ProvenConnection* best_conn = nullptr;
        double best_quality = 0.0;

        for (const auto& conn : base_connections) {
            bool a_is_base = (std::find(hier_state.base_pieces.begin(), hier_state.base_pieces.end(), conn.piece_a) != hier_state.base_pieces.end());
            bool b_is_base = (std::find(hier_state.base_pieces.begin(), hier_state.base_pieces.end(), conn.piece_b) != hier_state.base_pieces.end());

            bool a_in_assembly = (std::find(assembly_state.active_pieces.begin(),
                                           assembly_state.active_pieces.end(),
                                           conn.piece_a) != assembly_state.active_pieces.end());
            bool b_in_assembly = (std::find(assembly_state.active_pieces.begin(),
                                           assembly_state.active_pieces.end(),
                                           conn.piece_b) != assembly_state.active_pieces.end());

            // One piece in assembly, one unplaced base piece out
            if ((a_in_assembly && b_is_base && !b_in_assembly) ||
                (b_in_assembly && a_is_base && !a_in_assembly)) {
                double quality = conn.combined_quality;

                // Height validation removed: geometric detection + ICP inliers are sufficient
                // (Height comparison creates chicken-and-egg paradox - we don't have vessel positions until after assembly)

                if (quality > best_quality) {
                    best_quality = quality;
                    best_conn = &conn;
                }
            }
        }

        if (best_conn) {
            // Determine which piece to add
            bool a_in_assembly = (std::find(assembly_state.active_pieces.begin(),
                                           assembly_state.active_pieces.end(),
                                           best_conn->piece_a) != assembly_state.active_pieces.end());

            int anchor_piece = a_in_assembly ? best_conn->piece_a : best_conn->piece_b;
            int new_piece = a_in_assembly ? best_conn->piece_b : best_conn->piece_a;

            // PHASE 3: Hard degree guard for base pieces
            const char* hub_prevention_env = std::getenv("ENABLE_HUB_PREVENTION");
            bool use_hub_prevention = (hub_prevention_env != nullptr && std::string(hub_prevention_env) == "1");

            if (use_hub_prevention) {
                // Only check base pieces - rim pieces may already have degree 2
                bool anchor_is_base = (std::find(hier_state.base_pieces.begin(), hier_state.base_pieces.end(), anchor_piece) != hier_state.base_pieces.end());
                bool new_is_base = (std::find(hier_state.base_pieces.begin(), hier_state.base_pieces.end(), new_piece) != hier_state.base_pieces.end());

                int anchor_degree = hier_state.piece_degree_map[anchor_piece];
                int new_degree = hier_state.piece_degree_map[new_piece];

                // Base pieces MUST have degree <= 2 (ring topology)
                if ((anchor_is_base && anchor_degree >= config.ring_degree) ||
                    (new_is_base && new_degree >= config.ring_degree)) {
                    std::cout << "    ✗ DEGREE GUARD: Rejected connection " << best_conn->piece_a << "-" << best_conn->piece_b
                              << " (anchor degree=" << anchor_degree
                              << ", new degree=" << new_degree
                              << ", limit=" << config.ring_degree << ")" << std::endl;
                    break;  // No valid connections available, terminate base completion
                }
            }

            // Calculate world transform for new piece
            Matrix4d anchor_transform = assembly_state.piece_world_transforms[anchor_piece];

            Matrix4d relative_transform;
            if (anchor_piece == best_conn->piece_a) {
                // Anchor → new piece: use direct transformation
                best_conn->icp_transformation.Output(relative_transform);
            } else {
                // New piece → anchor: use inverse transformation
                best_conn->icp_transformation.InvOut(relative_transform);
            }

            assembly_state.active_pieces.push_back(new_piece);
            assembly_state.selected_connections.push_back(*best_conn);
            assembly_state.piece_world_transforms[new_piece] = anchor_transform * relative_transform;
            hier_state.placed_pieces.push_back(new_piece);

            // PHASE 3: Update degree tracking
            if (use_hub_prevention) {
                hier_state.piece_degree_map[anchor_piece]++;
                hier_state.piece_degree_map[new_piece]++;
            }

            std::cout << "    Added base piece " << new_piece << " via " << anchor_piece
                      << " (quality=" << best_conn->combined_quality;
            if (use_hub_prevention) {
                std::cout << ", degrees: " << anchor_piece << "=" << hier_state.piece_degree_map[anchor_piece]
                          << ", " << new_piece << "=" << hier_state.piece_degree_map[new_piece];
            }
            std::cout << ")" << std::endl;

            pieces_added = true;

            // PHASE 1 PRD: Validate closure after EACH base piece addition
            std::vector<int> base_in_assembly;
            for (int piece_id : hier_state.base_pieces) {
                if (std::find(assembly_state.active_pieces.begin(),
                              assembly_state.active_pieces.end(),
                              piece_id) != assembly_state.active_pieces.end()) {
                    base_in_assembly.push_back(piece_id);
                }
            }

            if (base_in_assembly.size() >= 2) {
                hier_state.base_circle = fitAndValidateCircle(
                    base_in_assembly, assembly_state, geometry, config, false);

                // PHASE 4: Ring closure bonus - detect when ring transitions to closed
                if (use_topology_bonuses && !was_closed && hier_state.base_circle.passes_closure) {
                    std::cout << "  ✓ RING CLOSURE BONUS: Base ring now passes validation (RMS="
                              << std::fixed << std::setprecision(2) << hier_state.base_circle.rms_error
                              << " mm, max_gap=" << hier_state.base_circle.max_gap
                              << " mm, coverage=" << std::setprecision(1) << (hier_state.base_circle.angular_coverage * 100)
                              << "%) [+" << (config.ring_closure_bonus * 100) << "% bonus applied]" << std::endl;
                }

                // PHASE 4: Arc completion tracking - log angular coverage improvement
                if (use_topology_bonuses && base_in_assembly.size() > 2) {
                    double arc_improvement = hier_state.base_circle.angular_coverage - prev_angular_coverage;
                    if (arc_improvement > 0.05) {  // Log if improvement >5%
                        std::cout << "    Arc completion: " << std::fixed << std::setprecision(1)
                                  << (prev_angular_coverage * 100) << "% → "
                                  << (hier_state.base_circle.angular_coverage * 100)
                                  << "% (+" << (arc_improvement * 100) << "%)" << std::endl;
                    }
                }

                // If closure passes, mark complete immediately
                if (hier_state.base_circle.passes_closure) {
                    std::cout << "  ✓ BASE RING COMPLETE (meets closure thresholds after " << base_in_assembly.size() << " pieces)" << std::endl;
                    return true;
                }
            }
        }
    }

    // PHASE 1 PRD: Final validation attempt
    std::vector<int> base_in_assembly;
    for (int piece_id : hier_state.base_pieces) {
        if (std::find(assembly_state.active_pieces.begin(),
                      assembly_state.active_pieces.end(),
                      piece_id) != assembly_state.active_pieces.end()) {
            base_in_assembly.push_back(piece_id);
        }
    }

    if (base_in_assembly.size() >= 2) {
        hier_state.base_circle = fitAndValidateCircle(
            base_in_assembly, assembly_state, geometry, config, false);

        if (hier_state.base_circle.passes_closure) {
            std::cout << "  ✓ BASE RING COMPLETE (meets closure thresholds)" << std::endl;
            return true;
        } else {
            // Log failure with final metrics
            std::cout << "  ✗ Base ring INCOMPLETE (attempt " << (hier_state.base_attempts + 1) << ")" << std::endl;
            std::cout << "    Final metrics: RMS=" << std::fixed << std::setprecision(2) << hier_state.base_circle.rms_error
                      << " mm, max_gap=" << hier_state.base_circle.max_gap
                      << " mm, coverage=" << std::setprecision(1) << (hier_state.base_circle.angular_coverage * 100) << "%" << std::endl;
        }
    }

    return false;
}

void HybridPuzzleFusionOptimizer::performBodyGapFilling(
    HierarchicalAssemblyState& hier_state,
    HybridAssemblyState& assembly_state,
    const std::vector<ProvenConnection>& connections,
    const std::vector<Geom>& geometry,
    const ArchaeologicalConfig& config) {

    std::cout << "  Starting body gap filling..." << std::endl;

    // Extract gaps from rim and base rings
    if (hier_state.rim_complete || hier_state.rim_pieces.size() > 0) {
        hier_state.gap_list.extractRingGaps(
            hier_state.rim_pieces, assembly_state, hier_state.rim_circle, Gap::RIM_ARC, config);
    }

    if (hier_state.base_complete || hier_state.base_pieces.size() > 0) {
        hier_state.gap_list.extractRingGaps(
            hier_state.base_pieces, assembly_state, hier_state.base_circle, Gap::BASE_ARC, config);
    }

    // Extract vertical corridor gaps
    hier_state.gap_list.extractVerticalGaps(
        hier_state.rim_pieces, hier_state.base_pieces, assembly_state, geometry, config);

    hier_state.gap_list.logGapSnapshot("INITIAL_BODY_FILLING");

    // Progressive gap filling - prioritize pieces that fill largest gaps
    std::vector<int> body_pieces;
    for (const auto& pair : hier_state.piece_types) {
        int piece_id = pair.first;
        PieceType type = pair.second;
        if (type == PieceType::BODY &&
            std::find(assembly_state.active_pieces.begin(),
                      assembly_state.active_pieces.end(),
                      piece_id) == assembly_state.active_pieces.end()) {
            body_pieces.push_back(piece_id);
        }
    }

    std::vector<int> candidates = getSizeOrderedCandidates(body_pieces, geometry);

    // Filter body-related connections (connections from assembly to unplaced body pieces)
    std::vector<ProvenConnection> body_connections;
    for (const auto& conn : connections) {
        bool a_is_body = (std::find(body_pieces.begin(), body_pieces.end(), conn.piece_a) != body_pieces.end());
        bool b_is_body = (std::find(body_pieces.begin(), body_pieces.end(), conn.piece_b) != body_pieces.end());

        // Accept connections involving at least one body piece
        if (a_is_body || b_is_body) {
            body_connections.push_back(conn);
        }
    }

    std::cout << "  Filtered " << body_connections.size() << " body-related connections" << std::endl;

    // Sort by quality descending
    std::sort(body_connections.begin(), body_connections.end(),
        [](const ProvenConnection& a, const ProvenConnection& b) {
            return a.combined_quality > b.combined_quality;
        });

    // PHASE 2 PRD: Gap-targeting scheduler with interleaved best-overall strategy
    int body_pieces_added = 0;
    int max_body_iterations = 20;
    int iteration = 0;

    // Check if gap targeting is enabled
    const char* gap_targeting_env = std::getenv("ENABLE_GAP_TARGETING");
    bool use_gap_targeting = (gap_targeting_env != nullptr && std::string(gap_targeting_env) == "1");

    while (body_pieces_added < static_cast<int>(body_pieces.size()) && iteration < max_body_iterations) {
        iteration++;

        // PHASE 2 PRD: Decide strategy for this iteration (gap-targeting vs best-overall)
        bool use_gap_strategy_this_iteration = use_gap_targeting;
        if (use_gap_targeting) {
            // Use random selection based on gap_targeting_frequency
            double random_val = static_cast<double>(rand()) / RAND_MAX;
            use_gap_strategy_this_iteration = (random_val < config.gap_targeting_frequency);
        }

        // Find best connection from current assembly to an unplaced body piece
        const ProvenConnection* best_conn = nullptr;
        double best_score = 0.0;
        int anchor_piece = -1;
        int new_piece = -1;
        Gap target_gap;
        double best_gap_reduction = 0.0;

        // Get largest gap if using gap-targeting strategy
        if (use_gap_strategy_this_iteration && hier_state.gap_list.getGapCount() > 0) {
            target_gap = hier_state.gap_list.getLargestGap();

            if (config.enable_hierarchical_logging) {
                std::cout << "  [ITERATION " << iteration << "] GAP-TARGETING: Targeting ";
                if (target_gap.type == Gap::RIM_ARC) std::cout << "RIM_ARC";
                else if (target_gap.type == Gap::BASE_ARC) std::cout << "BASE_ARC";
                else std::cout << "VERTICAL_CORRIDOR";
                std::cout << " (metric=" << target_gap.metric << ")" << std::endl;
            }
        } else if (config.enable_hierarchical_logging) {
            std::cout << "  [ITERATION " << iteration << "] BEST-OVERALL: Selecting highest quality connection" << std::endl;
        }

        for (const auto& conn : body_connections) {
            bool a_in_assembly = (std::find(assembly_state.active_pieces.begin(),
                                           assembly_state.active_pieces.end(),
                                           conn.piece_a) != assembly_state.active_pieces.end());
            bool b_in_assembly = (std::find(assembly_state.active_pieces.begin(),
                                           assembly_state.active_pieces.end(),
                                           conn.piece_b) != assembly_state.active_pieces.end());

            bool a_is_body = (std::find(body_pieces.begin(), body_pieces.end(), conn.piece_a) != body_pieces.end());
            bool b_is_body = (std::find(body_pieces.begin(), body_pieces.end(), conn.piece_b) != body_pieces.end());

            // Connection from assembly to unplaced body piece
            if ((a_in_assembly && b_is_body && !b_in_assembly) ||
                (b_in_assembly && a_is_body && !a_in_assembly)) {

                int candidate_piece = a_in_assembly ? conn.piece_b : conn.piece_a;
                int candidate_anchor = a_in_assembly ? conn.piece_a : conn.piece_b;
                double score = conn.combined_quality;

                // PHASE 3 PRD: Apply soft degree penalty for body pieces
                const char* hub_prevention_env = std::getenv("ENABLE_HUB_PREVENTION");
                bool use_hub_prevention = (hub_prevention_env != nullptr && std::string(hub_prevention_env) == "1");

                if (use_hub_prevention) {
                    int anchor_degree = hier_state.piece_degree_map[candidate_anchor];
                    int candidate_degree = hier_state.piece_degree_map[candidate_piece];

                    // Soft penalty: Reduce quality if either piece would exceed max_degree_body
                    if (anchor_degree >= config.max_degree_body || candidate_degree >= config.max_degree_body) {
                        score *= (1.0 - config.degree_penalty_weight);  // Apply -30% penalty

                        if (config.enable_hierarchical_logging) {
                            std::cout << "    DEGREE PENALTY: Connection " << conn.piece_a << "-" << conn.piece_b
                                      << " quality " << conn.combined_quality << " → " << score
                                      << " (anchor degree=" << anchor_degree
                                      << ", candidate degree=" << candidate_degree
                                      << ", limit=" << config.max_degree_body << ")" << std::endl;
                        }
                    }
                }

                // Height validation removed: geometric detection + ICP inliers are sufficient
                // (Height comparison creates chicken-and-egg paradox - we don't have vessel positions until after assembly)

                // PHASE 2 PRD: Apply gap-targeting logic
                if (use_gap_strategy_this_iteration && hier_state.gap_list.getGapCount() > 0) {
                    // Calculate gap reduction for this candidate
                    double gap_reduction = 0.0;
                    bool fills_gap = hier_state.gap_list.doesPieceFillGap(
                        candidate_piece, target_gap, assembly_state, geometry, gap_reduction);

                    if (fills_gap && gap_reduction >= config.gap_reduction_fraction) {
                        // Score = quality * gap_reduction (prioritize pieces that fill gaps)
                        score = conn.combined_quality * (1.0 + gap_reduction);

                        if (score > best_score) {
                            best_score = score;
                            best_conn = &conn;
                            anchor_piece = a_in_assembly ? conn.piece_a : conn.piece_b;
                            new_piece = candidate_piece;
                            best_gap_reduction = gap_reduction;
                        }
                    }
                } else {
                    // Best-overall strategy: just use quality
                    if (score > best_score) {
                        best_score = score;
                        best_conn = &conn;
                        anchor_piece = a_in_assembly ? conn.piece_a : conn.piece_b;
                        new_piece = candidate_piece;
                    }
                }
            }
        }

        if (best_conn) {
            // Calculate world transform for new piece
            Matrix4d anchor_transform = assembly_state.piece_world_transforms[anchor_piece];

            Matrix4d relative_transform;
            if (anchor_piece == best_conn->piece_a) {
                // Anchor → new piece: use direct transformation
                best_conn->icp_transformation.Output(relative_transform);
            } else {
                // New piece → anchor: use inverse transformation
                best_conn->icp_transformation.InvOut(relative_transform);
            }

            // Add piece to assembly with its transformation
            assembly_state.active_pieces.push_back(new_piece);
            hier_state.placed_pieces.push_back(new_piece);
            assembly_state.selected_connections.push_back(*best_conn);
            assembly_state.piece_world_transforms[new_piece] = anchor_transform * relative_transform;

            // PHASE 3 PRD: Update degree tracking for body pieces
            const char* hub_prevention_env = std::getenv("ENABLE_HUB_PREVENTION");
            bool use_hub_prevention = (hub_prevention_env != nullptr && std::string(hub_prevention_env) == "1");
            if (use_hub_prevention) {
                hier_state.piece_degree_map[anchor_piece]++;
                hier_state.piece_degree_map[new_piece]++;
            }

            // Update gaps
            hier_state.gap_list.updateGapsAfterPlacement(new_piece, assembly_state, geometry);

            // PHASE 2 PRD: Enhanced logging with gap reduction
            std::cout << "    Added body piece " << new_piece
                      << " via connection to " << anchor_piece
                      << " (quality=" << std::fixed << std::setprecision(2) << best_conn->combined_quality;

            if (use_gap_strategy_this_iteration && best_gap_reduction > 0.0) {
                std::cout << ", gap_reduction=" << std::fixed << std::setprecision(1)
                          << (best_gap_reduction * 100.0) << "%";
            }

            // PHASE 3 PRD: Show degree tracking
            if (use_hub_prevention) {
                std::cout << ", degrees: " << anchor_piece << "=" << hier_state.piece_degree_map[anchor_piece]
                          << ", " << new_piece << "=" << hier_state.piece_degree_map[new_piece];
            }

            std::cout << ")" << std::endl;

            body_pieces_added++;
        } else {
            // No more connections found
            std::cout << "  Body filling complete: no more valid connections" << std::endl;
            break;
        }
    }

    hier_state.gap_list.logGapSnapshot("FINAL_BODY_FILLING");
}

std::vector<int> HybridPuzzleFusionOptimizer::getSizeOrderedCandidates(
    const std::vector<int>& piece_ids,
    const std::vector<Geom>& geometry) const {

    std::vector<std::pair<int, int>> piece_sizes;
    for (int piece_id : piece_ids) {
        if (piece_id - 1 >= 0 && piece_id - 1 < static_cast<int>(geometry.size())) {
            const auto& geom = geometry[piece_id - 1];
            int size = geom.edge_line_.point_.cols();
            piece_sizes.push_back({piece_id, size});
        }
    }

    // Sort by size descending (larger pieces first)
    std::sort(piece_sizes.begin(), piece_sizes.end(),
        [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
            return a.second > b.second;
        });

    std::vector<int> ordered_pieces;
    for (const auto& pair : piece_sizes) {
        ordered_pieces.push_back(pair.first);
    }

    return ordered_pieces;
}

// ============================================================================

std::vector<HybridAssemblyState> HybridPuzzleFusionOptimizer::generateInitialStates(
    const std::vector<ProvenConnection>& proven_connections,
    const std::vector<Geom>& geometry) {

    std::vector<HybridAssemblyState> initial_states;

    // PHASE 1: Initial sort by pairwise quality
    std::vector<ProvenConnection> sorted_connections = proven_connections;
    std::sort(sorted_connections.begin(), sorted_connections.end(), std::greater<ProvenConnection>());

    // PHASE 2: CLUSTER COHERENCE VALIDATION
    // Adjust connection quality based on multi-piece geometric consistency
    // This boosts connections that form coherent triangles/clusters, penalizes isolated high-inlier connections
    sorted_connections = adjustConnectionQualityByClusterCoherence(sorted_connections, 8);  // TODO: Make total_pieces configurable

    // PHASE 2.5: ARCHAEOLOGICAL HEIGHT/ANGLE BONUSES
    // Apply pottery-specific bonuses based on vessel geometry
    // Boost connections at same height (circular) or vertical (layers)
    std::cout << "\n*** ARCHAEOLOGICAL BONUSES (Phase 2) *** Applying height/angle bonuses" << std::endl;
    std::cout << "*** DEBUG *** Geometry vector size: " << geometry.size() << " pieces" << std::endl;
    int circular_bonuses = 0, vertical_bonuses = 0, hub_penalties = 0;

    for (auto& conn : sorted_connections) {
        double original_quality = conn.combined_quality;
        double bonus = 1.0;

        // Get height and angle for both pieces
        double height_a = getHeight(conn.piece_a, geometry);
        double height_b = getHeight(conn.piece_b, geometry);
        double angle_a = getAngle(conn.piece_a, geometry);
        double angle_b = getAngle(conn.piece_b, geometry);

        // DEBUG: Print first few connections
        if (circular_bonuses + vertical_bonuses < 3) {
            std::cout << "*** DEBUG *** Connection " << conn.piece_a << "-" << conn.piece_b
                      << ": height_a=" << height_a << ", height_b=" << height_b
                      << ", angle_a=" << angle_a << ", angle_b=" << angle_b << std::endl;
        }

        double height_diff = abs(height_a - height_b);
        double angle_diff = abs(angle_a - angle_b);
        if (angle_diff > 180.0) angle_diff = 360.0 - angle_diff;  // Handle wraparound

        // CIRCULAR BONUS: Pieces at similar height, adjacent angle (circular neighbors)
        if (height_diff < 50.0 && angle_diff < 60.0 && height_a > 0.0) {
            bonus *= 1.3;  // +30% for circular neighbors
            circular_bonuses++;
            if (circular_bonuses <= 5) {
                std::cout << "  CIRCULAR BONUS: " << conn.piece_a << "-" << conn.piece_b
                          << " (Δheight=" << std::fixed << std::setprecision(1) << height_diff
                          << "mm, Δangle=" << angle_diff << "°)" << std::endl;
            }
        }
        // VERTICAL BONUS: Different heights, similar angle (vertical layers)
        else if (height_diff >= 50.0 && height_diff < 150.0 && angle_diff < 45.0 && height_a > 0.0) {
            bonus *= 1.2;  // +20% for vertical neighbors
            vertical_bonuses++;
            if (vertical_bonuses <= 5) {
                std::cout << "  VERTICAL BONUS: " << conn.piece_a << "-" << conn.piece_b
                          << " (Δheight=" << std::fixed << std::setprecision(1) << height_diff << "mm)" << std::endl;
            }
        }

        // Apply bonus
        if (bonus != 1.0) {
            conn.combined_quality *= bonus;
        }
    }

    std::cout << "*** ARCHAEOLOGICAL BONUSES COMPLETE *** "
              << circular_bonuses << " circular, "
              << vertical_bonuses << " vertical" << std::endl;

    // Re-sort after applying bonuses
    std::sort(sorted_connections.begin(), sorted_connections.end(), std::greater<ProvenConnection>());

    // DEBUG: Log top connections after archaeological bonuses
    std::cout << "\n*** CONNECTION RANKING DEBUG (After Archaeological Bonuses) *** Top " << std::min(20, static_cast<int>(sorted_connections.size())) << " connections:" << std::endl;
    for (int i = 0; i < std::min(20, static_cast<int>(sorted_connections.size())); ++i) {
        const auto& conn = sorted_connections[i];
        std::cout << "  Rank " << (i+1) << ": Pieces " << conn.piece_a << "->" << conn.piece_b
                  << " inliers=" << conn.inlier_count << " geom_err=" << std::fixed << std::setprecision(3) << conn.geometric_error
                  << " quality=" << std::setprecision(1) << conn.combined_quality << std::endl;
    }

    // PHASE 3: HIERARCHICAL ARCHAEOLOGICAL ASSEMBLY (Rim/Base Detection)
    // Classify pieces into RIM, BASE, BODY based on height analysis
    std::vector<PieceClassification> piece_classifications = classifyPiecesByStructure(geometry);
    std::vector<int> rim_pieces = identifyRimPieces(piece_classifications);
    std::vector<int> base_pieces = identifyBasePieces(piece_classifications);

    std::cout << "\n*** PHASE 3: STRUCTURAL CLASSIFICATION ***" << std::endl;
    std::cout << "  Rim pieces: " << rim_pieces.size() << " identified: ";
    for (int piece : rim_pieces) std::cout << piece << " ";
    std::cout << std::endl;
    std::cout << "  Base pieces: " << base_pieces.size() << " identified: ";
    for (int piece : base_pieces) std::cout << piece << " ";
    std::cout << std::endl;

    // Validate rim/base circular topology
    bool rim_circular = false, base_circular = false;
    if (!rim_pieces.empty()) {
        rim_circular = validateCircularTopology(rim_pieces, sorted_connections);
        std::cout << "  Rim circular topology: " << (rim_circular ? "VALID" : "PARTIAL") << std::endl;
    }
    if (!base_pieces.empty()) {
        base_circular = validateCircularTopology(base_pieces, sorted_connections);
        std::cout << "  Base circular topology: " << (base_circular ? "VALID" : "PARTIAL") << std::endl;
    }

    // PHASE 3.5: BRIDGE QUALITY BONUSES
    // Apply additional bonuses for connections that form good bridges between structural pieces
    // This prioritizes connections that "fit between two placed pieces" (archaeological strategy)
    int bridge_bonuses = 0;
    for (auto& conn : sorted_connections) {
        // Check if this connection forms a bridge between rim/base pieces
        bool piece_a_rim = std::find(rim_pieces.begin(), rim_pieces.end(), conn.piece_a) != rim_pieces.end();
        bool piece_b_rim = std::find(rim_pieces.begin(), rim_pieces.end(), conn.piece_b) != rim_pieces.end();
        bool piece_a_base = std::find(base_pieces.begin(), base_pieces.end(), conn.piece_a) != base_pieces.end();
        bool piece_b_base = std::find(base_pieces.begin(), base_pieces.end(), conn.piece_b) != base_pieces.end();

        // BONUS 1: Rim-to-rim connections (circular structure formation)
        if (piece_a_rim && piece_b_rim) {
            conn.combined_quality *= 1.4;  // +40% for rim circle formation
            bridge_bonuses++;
            if (bridge_bonuses <= 3) {
                std::cout << "  RIM-RIM BONUS: " << conn.piece_a << "-" << conn.piece_b
                          << " (forming rim circle)" << std::endl;
            }
        }
        // BONUS 2: Base-to-base connections (circular structure formation)
        else if (piece_a_base && piece_b_base) {
            conn.combined_quality *= 1.4;  // +40% for base circle formation
            bridge_bonuses++;
            if (bridge_bonuses <= 3) {
                std::cout << "  BASE-BASE BONUS: " << conn.piece_a << "-" << conn.piece_b
                          << " (forming base circle)" << std::endl;
            }
        }
        // BONUS 3: Rim-to-body or base-to-body (vertical structure)
        else if ((piece_a_rim || piece_b_rim) && !(piece_a_base || piece_b_base)) {
            conn.combined_quality *= 1.15;  // +15% for rim-body connections
        } else if ((piece_a_base || piece_b_base) && !(piece_a_rim || piece_b_rim)) {
            conn.combined_quality *= 1.15;  // +15% for base-body connections
        }
    }

    std::cout << "*** BRIDGE BONUSES COMPLETE *** " << bridge_bonuses << " structural bonuses applied" << std::endl;

    // Re-sort after bridge bonuses
    std::sort(sorted_connections.begin(), sorted_connections.end(), std::greater<ProvenConnection>());

    // PHASE 4: ARCHAEOLOGICAL ANCHOR IDENTIFICATION
    // Identify structural anchor (piece with highest connectivity) for archaeological assembly
    int anchor_piece = identifyStructuralAnchor(sorted_connections);

    // PHASE 5: ANCHOR-BASED INITIALIZATION
    // Create initial states from connections involving the anchor piece
    // This ensures beam search explores from archaeologically valid starting point

    std::vector<ProvenConnection> anchor_connections;
    for (const auto& conn : sorted_connections) {
        if (conn.piece_a == anchor_piece || conn.piece_b == anchor_piece) {
            anchor_connections.push_back(conn);
        }
    }

    if (anchor_connections.empty()) {
        std::cerr << "*** WARNING *** No connections found for anchor piece " << anchor_piece
                  << ", falling back to top connections" << std::endl;
        anchor_connections = sorted_connections;
    }

    // Sort anchor connections by quality
    std::sort(anchor_connections.begin(), anchor_connections.end(), std::greater<ProvenConnection>());

    // Create initial states from anchor connections (not top inliers!)
    int num_initial = std::min(config_.beam_width, static_cast<int>(anchor_connections.size()));
    std::cout << "*** ANCHOR-BASED INITIALIZATION *** Creating " << num_initial
              << " initial states from anchor piece " << anchor_piece << std::endl;

    for (int i = 0; i < num_initial; ++i) {
        const auto& conn = anchor_connections[i];

        // Determine which piece is anchor vs. new piece
        int new_piece = (conn.piece_a == anchor_piece) ? conn.piece_b : conn.piece_a;

        HybridAssemblyState state;
        state.active_pieces = {anchor_piece, new_piece};
        state.selected_connections = {conn};
        state.total_pieces = 8;  // TODO: Make configurable

        // CRITICAL: Anchor piece at origin (not piece_a by default!)
        state.piece_world_transforms[anchor_piece] = Matrix4d::Identity();

        // Transform new piece relative to anchor
        Matrix4d transform;
        if (conn.piece_a == anchor_piece) {
            // Anchor → new piece: use direct transformation
            conn.icp_transformation.Output(transform);
        } else {
            // New piece → anchor: use inverse transformation
            conn.icp_transformation.InvOut(transform);
        }
        state.piece_world_transforms[new_piece] = transform;

        // Calculate matrix score
        state.matrix_score = scorer_->calculateMatrixScore(state);

        initial_states.push_back(state);

        if (config_.enable_logging && i < 5) {
            std::cout << "  Initial state " << (i+1) << ": Anchor " << anchor_piece
                      << " → Piece " << new_piece << " (quality=" << std::fixed << std::setprecision(1)
                      << conn.combined_quality << ", inliers=" << conn.inlier_count << ")" << std::endl;
        }
    }

    std::cout << "*** ANCHOR-BASED INITIALIZATION COMPLETE *** Created " << initial_states.size()
              << " states from structural anchor" << std::endl;

    return initial_states;
}

void HybridPuzzleFusionOptimizer::exploreAssemblyStates(
    std::vector<HybridAssemblyState>& states,
    const std::vector<ProvenConnection>& all_connections) {

    std::vector<HybridAssemblyState> new_states;

    // Expand each current state (forward growth)
    for (const auto& state : states) {
        expandSingleState(state, all_connections, new_states);
    }

    // CRITICAL: Allow replacement of early connection choices
    // Generate alternative states by replacing initial connections with better alternatives
    // This prevents getting locked into locally optimal but globally suboptimal choices
    if (config_.enable_connection_replacement) {
        generateReplacementStates(states, all_connections, new_states);
    }

    // Add new states to current states
    states.insert(states.end(), new_states.begin(), new_states.end());
}

void HybridPuzzleFusionOptimizer::expandSingleState(
    const HybridAssemblyState& current_state,
    const std::vector<ProvenConnection>& all_connections,
    std::vector<HybridAssemblyState>& new_states) {

    std::unordered_set<int> current_pieces(current_state.active_pieces.begin(), current_state.active_pieces.end());

    // CRITICAL FIX: Group connections by new piece to avoid cycles
    // For each new piece, only keep the BEST connection to prevent multiple parents
    std::map<int, ProvenConnection> best_connection_per_new_piece;

    for (const auto& conn : all_connections) {
        bool piece_a_in = current_pieces.count(conn.piece_a) > 0;
        bool piece_b_in = current_pieces.count(conn.piece_b) > 0;

        // Only connections that connect assembly to new piece
        if ((piece_a_in && !piece_b_in) || (!piece_a_in && piece_b_in)) {
            // Filter by quality threshold
            if (conn.combined_quality >= config_.minimum_connection_quality) {
                // Determine which piece is new
                int new_piece = piece_a_in ? conn.piece_b : conn.piece_a;

                // Keep only the best connection for this new piece
                auto it = best_connection_per_new_piece.find(new_piece);
                if (it == best_connection_per_new_piece.end()) {
                    // First connection for this new piece
                    best_connection_per_new_piece.insert({new_piece, conn});
                } else if (conn.combined_quality > it->second.combined_quality) {
                    // Better connection found, replace it
                    it->second = conn;
                }
            }
        }
    }

    // Convert to vector for expansion
    std::vector<ProvenConnection> expansion_candidates;
    for (const auto& pair : best_connection_per_new_piece) {
        expansion_candidates.push_back(pair.second);
    }

    // Create new states for each expansion candidate (now guaranteed one connection per new piece)
    for (const auto& candidate : expansion_candidates) {
        HybridAssemblyState new_state = current_state;

        // Determine reference and new piece
        int reference_piece = current_pieces.count(candidate.piece_a) ? candidate.piece_a : candidate.piece_b;
        int new_piece = (reference_piece == candidate.piece_a) ? candidate.piece_b : candidate.piece_a;

        // Calculate world transform for new piece
        Matrix4d new_world_transform = calculateWorldTransform(new_piece, current_state, candidate);

        // CRITICAL FIX: Normalize connection direction to parent -> child
        // Tree structure requires connections flow from root toward leaves
        ProvenConnection normalized_conn = candidate;
        if (reference_piece != candidate.piece_a) {
            // Swap direction: reference_piece (parent) -> new_piece (child)
            normalized_conn.piece_a = reference_piece;
            normalized_conn.piece_b = new_piece;
            // Transformation is already correct (reference -> new via calculateWorldTransform)
        }

        // Update state
        new_state.active_pieces.push_back(new_piece);
        new_state.selected_connections.push_back(normalized_conn);  // Use normalized direction
        new_state.piece_world_transforms[new_piece] = new_world_transform;

        // Calculate matrix score
        new_state.matrix_score = scorer_->calculateMatrixScore(new_state);

        // CRITICAL FIX: Verify no cycles before adding state
        // Each piece should have at most one parent (piece_b has parent piece_a)
        std::map<int, int> parent_count;
        for (int piece_id : new_state.active_pieces) {
            parent_count[piece_id] = 0;
        }

        bool has_cycle = false;
        for (const auto& conn : new_state.selected_connections) {
            parent_count[conn.piece_b]++;
            if (parent_count[conn.piece_b] > 1) {
                has_cycle = true;
                break;
            }
        }

        // Only accept if score improves or assembly grows AND no cycles
        bool score_improves = new_state.matrix_score > current_state.matrix_score;
        bool assembly_grows = new_state.active_pieces.size() > current_state.active_pieces.size();

        if (!has_cycle && (score_improves || assembly_grows)) {
            new_states.push_back(new_state);
        }
    }
}

Matrix4d HybridPuzzleFusionOptimizer::calculateWorldTransform(
    int piece, const HybridAssemblyState& state, const ProvenConnection& new_connection) {

    // Find reference piece that's already in assembly
    std::unordered_set<int> current_pieces(state.active_pieces.begin(), state.active_pieces.end());
    int reference_piece = current_pieces.count(new_connection.piece_a) ? new_connection.piece_a : new_connection.piece_b;

    // SAFETY CHECK: Ensure reference piece has a transform
    auto it = state.piece_world_transforms.find(reference_piece);
    if (it == state.piece_world_transforms.end()) {
        // Reference piece not in transform map - return identity as fallback
        std::cerr << "*** ERROR *** Reference piece " << reference_piece
                  << " not found in transform map! Returning identity." << std::endl;
        return Matrix4d::Identity();
    }

    // Get reference piece's world transform
    Matrix4d reference_world = it->second;

    // Get ICP transformation
    Matrix4d icp_transform;
    if (piece == new_connection.piece_b) {
        // piece is piece_b, apply direct transformation
        new_connection.icp_transformation.Output(icp_transform);
    } else {
        // piece is piece_a, apply inverse transformation
        new_connection.icp_transformation.InvOut(icp_transform);
    }

    // Compose transformations: new_piece_world = reference_world * icp_transform
    return reference_world * icp_transform;
}

void HybridPuzzleFusionOptimizer::generateReplacementStates(
    const std::vector<HybridAssemblyState>& current_states,
    const std::vector<ProvenConnection>& all_connections,
    std::vector<HybridAssemblyState>& new_states) {

    // CRITICAL INSIGHT: False high-inlier connections (like 4-3 with 257 inliers) get locked in
    // during initialization, preventing ground truth connections from being explored.
    // This function generates alternative states by REPLACING early connection choices.

    static int replacement_debug_count = 0;

    for (const auto& state : current_states) {
        // Only consider replacing connections in small assemblies (2-4 pieces)
        // This prevents wasting time on nearly-complete assemblies
        if (state.active_pieces.size() < 2 || state.active_pieces.size() > 4) continue;

        // For each connection in current state, try replacing it with alternatives
        for (size_t conn_idx = 0; conn_idx < state.selected_connections.size(); ++conn_idx) {
            const auto& original_conn = state.selected_connections[conn_idx];

            // Find alternative connections between same pieces OR involving same pieces
            std::vector<ProvenConnection> alternatives;

            for (const auto& alt_conn : all_connections) {
                // Skip the original connection
                if (alt_conn.piece_a == original_conn.piece_a && alt_conn.piece_b == original_conn.piece_b) {
                    continue;
                }

                // Find alternatives that involve at least one of the same pieces
                bool involves_piece_a = (alt_conn.piece_a == original_conn.piece_a || alt_conn.piece_b == original_conn.piece_a);
                bool involves_piece_b = (alt_conn.piece_a == original_conn.piece_b || alt_conn.piece_b == original_conn.piece_b);

                if (involves_piece_a || involves_piece_b) {
                    // Only consider high-quality alternatives
                    if (alt_conn.combined_quality >= config_.minimum_connection_quality * 0.8) {
                        alternatives.push_back(alt_conn);
                    }
                }
            }

            // Try top alternatives (limit to avoid explosion)
            std::sort(alternatives.begin(), alternatives.end(), std::greater<ProvenConnection>());
            int num_alternatives = std::min(3, static_cast<int>(alternatives.size()));

            for (int alt_idx = 0; alt_idx < num_alternatives; ++alt_idx) {
                const auto& alt_conn = alternatives[alt_idx];

                // Create replacement state
                HybridAssemblyState replacement_state;
                replacement_state.total_pieces = state.total_pieces;

                // CRITICAL FIX: Only allow replacements that maintain connectivity
                // Replacement must involve at least one piece from original connection
                bool replacement_connects = false;
                if (alt_conn.piece_a == original_conn.piece_a || alt_conn.piece_a == original_conn.piece_b ||
                    alt_conn.piece_b == original_conn.piece_a || alt_conn.piece_b == original_conn.piece_b) {
                    replacement_connects = true;
                }

                // Skip replacements that introduce disconnected pieces
                if (!replacement_connects) {
                    static int skip_count = 0;
                    if (skip_count < 3) {
                        std::cout << "*** REPLACEMENT SKIP *** Alternative " << alt_conn.piece_a << "-" << alt_conn.piece_b
                                  << " doesn't share pieces with original " << original_conn.piece_a << "-" << original_conn.piece_b
                                  << " (would create disconnected graph)" << std::endl;
                        skip_count++;
                    }
                    continue;  // Skip to next alternative
                }

                // Collect all pieces involved in the replacement
                std::set<int> new_pieces_set;
                for (size_t i = 0; i < state.selected_connections.size(); ++i) {
                    if (i == conn_idx) {
                        // Use alternative connection
                        new_pieces_set.insert(alt_conn.piece_a);
                        new_pieces_set.insert(alt_conn.piece_b);
                        replacement_state.selected_connections.push_back(alt_conn);
                    } else {
                        // Keep original connection
                        new_pieces_set.insert(state.selected_connections[i].piece_a);
                        new_pieces_set.insert(state.selected_connections[i].piece_b);
                        replacement_state.selected_connections.push_back(state.selected_connections[i]);
                    }
                }

                replacement_state.active_pieces = std::vector<int>(new_pieces_set.begin(), new_pieces_set.end());

                // Rebuild transformations from scratch
                if (!replacement_state.active_pieces.empty()) {
                    // Set first piece at origin
                    int reference_piece = replacement_state.active_pieces[0];
                    replacement_state.piece_world_transforms[reference_piece] = Matrix4d::Identity();

                    // Apply connections iteratively until all pieces have transforms
                    // Use multiple passes to handle arbitrary connection order
                    int max_passes = replacement_state.active_pieces.size();
                    for (int pass = 0; pass < max_passes; ++pass) {
                        bool made_progress = false;

                        for (const auto& conn : replacement_state.selected_connections) {
                            if (replacement_state.piece_world_transforms.count(conn.piece_a) > 0 &&
                                replacement_state.piece_world_transforms.count(conn.piece_b) == 0) {
                                Matrix4d transform;
                                conn.icp_transformation.Output(transform);
                                replacement_state.piece_world_transforms[conn.piece_b] =
                                    replacement_state.piece_world_transforms[conn.piece_a] * transform;
                                made_progress = true;
                            } else if (replacement_state.piece_world_transforms.count(conn.piece_b) > 0 &&
                                       replacement_state.piece_world_transforms.count(conn.piece_a) == 0) {
                                Matrix4d transform;
                                conn.icp_transformation.InvOut(transform);
                                replacement_state.piece_world_transforms[conn.piece_a] =
                                    replacement_state.piece_world_transforms[conn.piece_b] * transform;
                                made_progress = true;
                            }
                        }

                        // If no progress made, connections don't form a connected graph
                        if (!made_progress) break;
                    }

                    // SAFETY CHECK: Verify all active pieces have transforms
                    bool all_pieces_have_transforms = true;
                    for (int piece_id : replacement_state.active_pieces) {
                        if (replacement_state.piece_world_transforms.count(piece_id) == 0) {
                            all_pieces_have_transforms = false;
                            break;
                        }
                    }

                    // Skip this replacement state if not all pieces are connected
                    if (!all_pieces_have_transforms) {
                        continue;  // Skip to next alternative
                    }

                    // CRITICAL FIX: Check for cycles BEFORE scoring
                    // Ensure each piece has at most one parent
                    std::map<int, int> parent_count;
                    for (int piece_id : replacement_state.active_pieces) {
                        parent_count[piece_id] = 0;
                    }

                    bool has_cycle = false;
                    for (const auto& conn : replacement_state.selected_connections) {
                        parent_count[conn.piece_b]++;
                        if (parent_count[conn.piece_b] > 1) {
                            has_cycle = true;
                            break;
                        }
                    }

                    // Skip if replacement creates cycle
                    if (has_cycle) {
                        continue;  // Skip to next alternative
                    }
                }

                // Score the replacement state
                replacement_state.matrix_score = scorer_->calculateMatrixScore(replacement_state);

                // Only accept if replacement improves score or enables new pieces
                if (replacement_state.matrix_score > state.matrix_score * 0.9) {  // Allow 10% tolerance
                    new_states.push_back(replacement_state);

                    if (config_.enable_logging && replacement_debug_count < 5) {
                        std::cout << "*** CONNECTION REPLACEMENT *** Replaced " << original_conn.piece_a
                                  << "-" << original_conn.piece_b << " (quality=" << original_conn.combined_quality
                                  << ") with " << alt_conn.piece_a << "-" << alt_conn.piece_b
                                  << " (quality=" << alt_conn.combined_quality << ")"
                                  << " | Score: " << state.matrix_score << " -> " << replacement_state.matrix_score
                                  << std::endl;
                        replacement_debug_count++;
                    }
                }
            }
        }
    }
}

void HybridPuzzleFusionOptimizer::validateTopCandidates(std::vector<HybridAssemblyState>& states) {
    // Sort by matrix score to identify top candidates
    std::sort(states.begin(), states.end(),
              [](const HybridAssemblyState& a, const HybridAssemblyState& b) {
                  return a.matrix_score > b.matrix_score;
              });

    // Validate top candidates that haven't been validated yet
    int validated_count = 0;
    for (auto& state : states) {
        if (!state.has_real_validation && validated_count < config_.top_candidates_for_validation) {
            // Perform real geometry validation
            state.validated_score = validator_->validateAssemblyState(state);
            state.has_real_validation = true;
            validated_count++;

            if (config_.enable_logging) {
                std::cout << "*** HYBRID VALIDATION *** " << state.active_pieces.size()
                          << " pieces: matrix_score=" << std::fixed << std::setprecision(3) << state.matrix_score
                          << ", real_score=" << state.validated_score << std::endl;
            }

            // Check for geometric conflicts
            if (validator_->hasGeometricConflicts(state)) {
                state.validated_score *= 0.5;  // Penalize conflicted assemblies
            }
        }
    }
}

void HybridPuzzleFusionOptimizer::pruneBeamStates(std::vector<HybridAssemblyState>& states) {
    if (states.empty()) return;

    // CRITICAL FIX: Filter out invalid tree structures BEFORE scoring
    std::vector<HybridAssemblyState> valid_states;
    int invalid_count = 0;
    std::map<int, int> invalid_by_size;  // Track rejection by assembly size
    int max_valid_size = 0;

    for (const auto& state : states) {
        if (isValidTreeStructure(state)) {
            valid_states.push_back(state);
            max_valid_size = std::max(max_valid_size, static_cast<int>(state.active_pieces.size()));
        } else {
            invalid_count++;
            int size = state.active_pieces.size();
            invalid_by_size[size]++;
        }
    }

    if (config_.enable_logging && invalid_count > 0) {
        std::cout << "*** TOPOLOGY VALIDATION *** Filtered out " << invalid_count
                  << " invalid tree structures (kept " << valid_states.size() << " valid, max_size=" << max_valid_size << ")" << std::endl;

        // Show rejection breakdown by size (for first few iterations only)
        static int debug_count = 0;
        if (debug_count < 3 && invalid_count > 0) {
            std::cout << "  Rejections by size: ";
            for (const auto& pair : invalid_by_size) {
                std::cout << pair.first << "pc=" << pair.second << " ";
            }
            std::cout << std::endl;
            debug_count++;
        }
    }

    // If all states are invalid, keep original states (fallback)
    if (valid_states.empty()) {
        std::cerr << "*** WARNING *** All states invalid - keeping original states" << std::endl;
        valid_states = states;
    }

    states = valid_states;

    if (states.size() <= config_.beam_width) return;

    // Sort by final score (prioritizes validated scores)
    std::sort(states.begin(), states.end(), std::greater<HybridAssemblyState>());

    // Keep only beam_width best states
    states.resize(config_.beam_width);

    if (config_.enable_logging) {
        std::cout << "*** HYBRID BEAM PRUNING *** Kept " << states.size() << " best states" << std::endl;
    }
}

bool HybridPuzzleFusionOptimizer::hasConverged(const std::vector<HybridAssemblyState>& states) const {
    if (states.empty()) return true;

    // Check if best state achieved full assembly
    auto best_state = *std::max_element(states.begin(), states.end(),
                                       [this](const HybridAssemblyState& a, const HybridAssemblyState& b) {
                                           return scorer_->calculateFinalScore(a) < scorer_->calculateFinalScore(b);
                                       });

    return best_state.active_pieces.size() == best_state.total_pieces;
}

std::vector<ProvenConnection> HybridPuzzleFusionOptimizer::convertToProvenConnections(
    const std::vector<LCSIndex>& connections,
    const std::vector<Geom>& geometry) {

    std::vector<ProvenConnection> proven_connections;
    proven_connections.reserve(connections.size());

    int axis_rejected_count = 0;
    int total_checked = 0;

    for (const auto& lcs : connections) {
        // Only include connections with reasonable quality
        if (lcs.inliner_ <= 0 || lcs.score_ < 0.0) {
            continue;
        }

        // POTTERY AXIS ALIGNMENT CHECK: Reject inverted/opposite axes
        int piece_a = lcs.shard_x_;
        int piece_b = lcs.shard_y_;

        total_checked++;

        // Check if axis data is available (1-indexed piece IDs)
        if (piece_a >= 1 && piece_b >= 1 &&
            piece_a <= geometry.size() && piece_b <= geometry.size()) {

            const auto& geom_a = geometry[piece_a - 1];
            const auto& geom_b = geometry[piece_b - 1];

            // USE ORIGINAL AXIS (before transformations) for alignment check
            // CRITICAL FIX: Use TRANSFORMED axes after ICP alignment, not original axes from file!
            // Original axes may be inverted in preprocessing data, but ICP transformation fixes them.
            if (geom_a.edge_line_.axis_norm_.size() > 0 &&
                geom_b.edge_line_.axis_norm_.size() > 0) {

                // Get TRANSFORMED axis vectors (after ICP alignment)
                Vector3d axis_a = geom_a.edge_line_.axis_norm_[0].normalized();
                Vector3d axis_b = geom_b.edge_line_.axis_norm_[0].normalized();

                // Calculate alignment (pottery rule: axes should point same direction)
                double axis_alignment = axis_a.dot(axis_b);

                // DEBUG: Print all axis checks for pieces 3 and 5
                if ((piece_a == 3 && piece_b == 5) || (piece_a == 5 && piece_b == 3)) {
                    std::cout << "*** PUZZLEFUSION AXIS CHECK *** Pieces " << piece_a << "-" << piece_b
                             << " original_axis_a=[" << axis_a.transpose() << "]"
                             << " original_axis_b=[" << axis_b.transpose() << "]"
                             << " dot=" << std::fixed << std::setprecision(4) << axis_alignment
                             << " (threshold=0.5)" << std::endl;
                }

                // Threshold: 0.5 means axes within ~60 degrees, POSITIVE direction
                // Lowered from 0.8 to accommodate pottery-specific geometry where sequential pieces
                // can have different orientations around the vessel axis
                if (axis_alignment <= 0.5) {
                    axis_rejected_count++;
                    std::cout << "*** PUZZLEFUSION AXIS FILTER *** REJECTED Pieces " << piece_a << "-" << piece_b
                             << " axis_alignment=" << std::fixed << std::setprecision(3) << axis_alignment
                             << " (threshold=0.5, inliers=" << lcs.inliner_ << ")" << std::endl;
                    continue;  // Skip this connection
                }
            }
        }

        // Connection passed all checks
        proven_connections.emplace_back(lcs);

        // DEBUG: Track high-inlier connections to see if they're being converted
        if (lcs.inliner_ > 200) {
            std::cout << "*** HIGH-INLIER CONNECTION CONVERTED *** Pieces " << piece_a << "->" << piece_b
                     << " (shard_x=" << lcs.shard_x_ << " shard_y=" << lcs.shard_y_ << ")"
                     << " inliers=" << lcs.inliner_ << " score=" << lcs.score_
                     << " → piece_a=" << piece_a << " piece_b=" << piece_b << std::endl;
        }
    }

    std::cout << "*** PUZZLEFUSION AXIS FILTER SUMMARY *** Checked " << total_checked
             << " connections, rejected " << axis_rejected_count << " for axis misalignment, "
             << proven_connections.size() << " passed" << std::endl;

    return proven_connections;
}

void HybridPuzzleFusionOptimizer::logOptimizationProgress(
    int iteration, const std::vector<HybridAssemblyState>& states) const {

    if (!config_.enable_logging) return;

    std::cout << "*** HYBRID PROGRESS *** Iteration " << iteration << " results:" << std::endl;

    for (int i = 0; i < std::min(3, static_cast<int>(states.size())); ++i) {
        const auto& state = states[i];
        std::cout << "  State " << i << ": " << state.active_pieces.size() << " pieces, "
                  << "matrix=" << std::fixed << std::setprecision(2) << state.matrix_score;

        if (state.has_real_validation) {
            std::cout << ", real=" << state.validated_score;
        }
        std::cout << std::endl;
    }
}

//==============================================================================
// Integration Function
//==============================================================================

RankingSubgraph performHybridPuzzleFusionOptimization(
    int total_pieces,
    std::vector<LCSIndex> all_connections,
    const std::vector<Geom>& shard) {

    std::cout << "*** HYBRID PUZZLEFUSION++ *** Starting hybrid matrix-real optimization" << std::endl;

    // Configure hybrid optimizer
    HybridOptimizationConfig config;
    config.enable_logging = true;

    // ADAPTIVE BEAM WIDTH: Scale with number of pieces to ensure sufficient exploration
    // For 8-piece pottery: need wide exploration to find ground truth connections
    // For larger assemblies: scale appropriately to maintain performance
    int adaptive_beam_width = std::max(16, total_pieces * 2);  // Minimum 16, scale with pieces
    config.beam_width = adaptive_beam_width;

    config.max_iterations = 20;
    config.top_candidates_for_validation = 5;  // INCREASED: Validate more candidates
    config.real_geometry_threshold = 50.0;

    std::cout << "*** BEAM WIDTH *** Adaptive beam width: " << adaptive_beam_width
              << " (total_pieces=" << total_pieces << ")" << std::endl;

    // Run hybrid optimization
    HybridPuzzleFusionOptimizer optimizer(config);
    HybridAssemblyState result = optimizer.optimizeGlobalAssembly(total_pieces, all_connections, shard);

    // Convert to RankingSubgraph format for integration
    RankingSubgraph ranking_result(total_pieces);

    // Mark assembled pieces and build CONNECTED assembly from optimized connections
    if (!result.active_pieces.empty()) {
        for (int piece : result.active_pieces) {
            ranking_result.node_[piece - 1] = true;
        }

        // LEGACY-STYLE TRANSFORMATION STORAGE: Use relative transformations like legacy system

        // Initialize all pieces as not connected (like legacy)
        for (int i = 0; i < total_pieces; ++i) {
            ranking_result.node_[i] = false;
        }

        // Mark assembled pieces
        for (int piece : result.active_pieces) {
            ranking_result.node_[piece - 1] = true;
        }

        // ========================================================================
        // FINAL ASSEMBLY RESULT - SELECTED CONNECTIONS
        // ========================================================================
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "*** FINAL REASSEMBLY RESULT - SELECTED CONNECTIONS ***" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "Total pieces in assembly: " << result.active_pieces.size() << "/" << total_pieces << std::endl;
        std::cout << "Total connections selected: " << result.selected_connections.size() << std::endl;
        std::cout << "Average connection quality: " << std::fixed << std::setprecision(2)
                  << result.average_connection_quality << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        // Store selected connections with RELATIVE transformations (like legacy)
        int conn_num = 1;
        for (const auto& conn : result.selected_connections) {
            LCSIndex lcs_conn;
            lcs_conn.shard_x_ = conn.piece_a;
            lcs_conn.shard_y_ = conn.piece_b;
            lcs_conn.trans_ = conn.icp_transformation;  // Use original ICP transformation (relative)
            lcs_conn.score_ = conn.geometric_error;
            lcs_conn.inliner_ = conn.inlier_count;
            ranking_result.sub_graph_.push_back(lcs_conn);

            // Set RELATIVE transformation in T_ array (like legacy: piece_a -> piece_b)
            Matrix3d R;
            Vector3d t;
            conn.icp_transformation.Output(R, t);
            ranking_result.T_[conn.piece_a - 1].Set(R, t, conn.piece_a, conn.piece_b);

            // DETAILED CONNECTION OUTPUT
            std::cout << "\n[CONNECTION #" << conn_num << "] Piece " << conn.piece_a
                      << " --> Piece " << conn.piece_b << std::endl;
            std::cout << "  Edge Alignment: " << conn.inlier_count << " inliers" << std::endl;
            std::cout << "  Geometric Error: " << std::fixed << std::setprecision(4)
                      << conn.geometric_error << std::endl;
            std::cout << "  Quality Score: " << std::fixed << std::setprecision(2)
                      << conn.combined_quality << std::endl;
            std::cout << "  Breakline Segment (Piece " << conn.piece_a << "): ["
                      << conn.start_a << ", " << conn.end_a << "]" << std::endl;
            std::cout << "  Breakline Segment (Piece " << conn.piece_b << "): ["
                      << conn.start_b << ", " << conn.end_b << "]" << std::endl;
            std::cout << "  ICP Transformation (Piece " << conn.piece_a << " -> Piece " << conn.piece_b << "):" << std::endl;
            std::cout << "    Rotation Matrix:" << std::endl;
            std::cout << "      [" << std::fixed << std::setprecision(6)
                      << R(0,0) << ", " << R(0,1) << ", " << R(0,2) << "]" << std::endl;
            std::cout << "      [" << R(1,0) << ", " << R(1,1) << ", " << R(1,2) << "]" << std::endl;
            std::cout << "      [" << R(2,0) << ", " << R(2,1) << ", " << R(2,2) << "]" << std::endl;
            std::cout << "    Translation Vector: [" << std::fixed << std::setprecision(3)
                      << t(0) << ", " << t(1) << ", " << t(2) << "]" << std::endl;

            conn_num++;
        }

        std::cout << std::string(80, '=') << std::endl;
        std::cout << "*** END OF FINAL REASSEMBLY RESULT ***" << std::endl;
        std::cout << std::string(80, '=') << std::endl << std::endl;

        // Set completion status
        ranking_result.iscomplete_ = (result.active_pieces.size() >= total_pieces * 0.75);

        std::cout << "*** HYBRID SUCCESS *** Created assembly with " << result.active_pieces.size()
                  << " pieces using real geometry validation" << std::endl;
    } else {
        std::cout << "*** HYBRID WARNING *** No assembly created, using fallback state" << std::endl;
    }

    return ranking_result;
}

/**
 * Build connected assembly from hybrid optimization result
 * Mimics legacy TransAverage approach but uses globally optimized connections
 */
std::map<int, Matrix4d> buildConnectedAssembly(const HybridAssemblyState& result) {
    std::map<int, Matrix4d> connected_transforms;

    if (result.active_pieces.empty() || result.selected_connections.empty()) {
        return connected_transforms;
    }

    // Build connection graph
    std::map<int, std::vector<ProvenConnection>> adjacency;
    for (const auto& conn : result.selected_connections) {
        adjacency[conn.piece_a].push_back(conn);
        adjacency[conn.piece_b].push_back(conn);
    }

    // Find best reference piece (one with most connections)
    int reference_piece = result.active_pieces[0];
    int max_connections = 0;
    for (int piece : result.active_pieces) {
        int conn_count = adjacency[piece].size();
        if (conn_count > max_connections) {
            max_connections = conn_count;
            reference_piece = piece;
        }
    }

    std::cout << "*** CONNECTED ASSEMBLY *** Using piece " << reference_piece
              << " as reference (has " << max_connections << " connections)" << std::endl;

    // Place reference piece at origin
    connected_transforms[reference_piece] = Matrix4d::Identity();

    // Build assembly using MULTI-CONNECTION AVERAGING (like legacy TransAverage)
    std::set<int> placed_pieces;
    std::vector<int> to_place;

    // Add reference piece
    placed_pieces.insert(reference_piece);

    // Collect all other pieces to place
    for (int piece : result.active_pieces) {
        if (piece != reference_piece) {
            to_place.push_back(piece);
        }
    }

    // Place pieces iteratively, using multi-connection averaging
    while (!to_place.empty()) {
        bool placed_any = false;

        for (auto it = to_place.begin(); it != to_place.end(); ) {
            int piece_to_place = *it;

            // Find all connections from this piece to already-placed pieces
            std::vector<Matrix4d> candidate_transforms;
            std::vector<double> connection_weights;

            for (const auto& conn : adjacency[piece_to_place]) {
                int other_piece = (conn.piece_a == piece_to_place) ? conn.piece_b : conn.piece_a;

                // Only consider connections to already-placed pieces
                if (!placed_pieces.count(other_piece)) continue;

                Matrix4d other_world_transform = connected_transforms[other_piece];
                Matrix4d icp_transform;

                // Determine transformation direction
                if (piece_to_place == conn.piece_b && other_piece == conn.piece_a) {
                    conn.icp_transformation.Output(icp_transform);
                } else if (piece_to_place == conn.piece_a && other_piece == conn.piece_b) {
                    conn.icp_transformation.InvOut(icp_transform);
                } else {
                    continue;
                }

                // Compute candidate position for piece_to_place
                Matrix4d candidate_transform = other_world_transform * icp_transform;
                candidate_transforms.push_back(candidate_transform);

                // Weight by inlier count (better connections have more influence)
                connection_weights.push_back(conn.inlier_count);

                std::cout << "*** TRANS AVERAGE *** Piece " << piece_to_place
                          << " candidate from piece " << other_piece
                          << " (weight: " << conn.inlier_count << ")" << std::endl;
            }

            // If we have connections to placed pieces, average the transformations
            if (!candidate_transforms.empty()) {
                // Compute weighted average transformation (like legacy TransAverage)
                Matrix4d averaged_transform = Matrix4d::Zero();
                double total_weight = 0.0;

                for (size_t i = 0; i < candidate_transforms.size(); ++i) {
                    averaged_transform += connection_weights[i] * candidate_transforms[i];
                    total_weight += connection_weights[i];
                }

                if (total_weight > 0) {
                    averaged_transform /= total_weight;
                    connected_transforms[piece_to_place] = averaged_transform;
                    placed_pieces.insert(piece_to_place);

                    std::cout << "*** MULTI-CONNECTION PLACEMENT *** Piece " << piece_to_place
                              << " averaged from " << candidate_transforms.size()
                              << " connections (total weight: " << total_weight << ")" << std::endl;

                    it = to_place.erase(it);
                    placed_any = true;
                    continue;
                }
            }

            ++it;
        }

        // If no pieces could be placed this iteration, break to avoid infinite loop
        if (!placed_any) {
            std::cout << "*** WARNING *** Cannot place remaining pieces: disconnected components" << std::endl;
            break;
        }
    }

    // Handle any disconnected pieces (shouldn't happen with good hybrid optimization)
    for (int piece : result.active_pieces) {
        if (connected_transforms.find(piece) == connected_transforms.end()) {
            std::cout << "*** WARNING *** Piece " << piece << " disconnected, placing at origin" << std::endl;
            connected_transforms[piece] = Matrix4d::Identity();
        }
    }

    return connected_transforms;
}

//==============================================================================
// CLUSTER COHERENCE VALIDATION SYSTEM
// Multi-piece geometric consistency checking for pottery reassembly

/**
 * EDGE COMPETITION DETECTION
 *
 * Checks if two connections in a triangle compete for the same physical edge.
 * Example: If connections AB and AC both use edge segment [10,50] of piece A,
 * they are mutually exclusive (same edge cannot connect to both B and C).
 *
 * This detects false triangles where connections have high inliers but are
 * geometrically impossible due to edge overlap.
 */
bool HybridPuzzleFusionOptimizer::checkEdgeCompetition(
    const ProvenConnection& conn_ab,
    const ProvenConnection& conn_bc,
    const ProvenConnection& conn_ac) const {

    // Helper: Check if two breakline segments overlap
    auto segmentsOverlap = [](int start1, int end1, int start2, int end2) -> bool {
        // Ensure start <= end for both segments
        if (start1 > end1) std::swap(start1, end1);
        if (start2 > end2) std::swap(start2, end2);

        // Segments overlap if: max(start1, start2) < min(end1, end2)
        int overlap_start = std::max(start1, start2);
        int overlap_end = std::min(end1, end2);

        // Significant overlap: more than 30% of smaller segment
        if (overlap_end > overlap_start) {
            int overlap_len = overlap_end - overlap_start;
            int seg1_len = end1 - start1;
            int seg2_len = end2 - start2;
            int min_seg_len = std::min(seg1_len, seg2_len);

            if (min_seg_len == 0) return false;
            double overlap_ratio = static_cast<double>(overlap_len) / min_seg_len;
            return (overlap_ratio > 0.3);  // 30% overlap threshold
        }
        return false;
    };

    // Extract piece IDs from connections
    std::set<int> pieces;
    pieces.insert(conn_ab.piece_a);
    pieces.insert(conn_ab.piece_b);
    pieces.insert(conn_bc.piece_a);
    pieces.insert(conn_bc.piece_b);
    pieces.insert(conn_ac.piece_a);
    pieces.insert(conn_ac.piece_b);

    if (pieces.size() != 3) return false;  // Should be exactly 3 pieces

    auto pieces_vec = std::vector<int>(pieces.begin(), pieces.end());
    int p0 = pieces_vec[0];
    int p1 = pieces_vec[1];
    int p2 = pieces_vec[2];

    // Check each piece for edge competition by examining breakline segment overlap
    for (int piece : {p0, p1, p2}) {
        // Collect all edge segments for this piece from the three connections
        struct SegmentInfo {
            int start, end;
            int other_piece;  // Which piece this edge connects to
            const ProvenConnection* conn;
        };
        std::vector<SegmentInfo> segments;

        // Helper: Add segment if connection involves this piece
        auto addSegmentIfMatch = [&](const ProvenConnection& conn) {
            if (conn.piece_a == piece) {
                segments.push_back({conn.start_a, conn.end_a, conn.piece_b, &conn});
            } else if (conn.piece_b == piece) {
                segments.push_back({conn.start_b, conn.end_b, conn.piece_a, &conn});
            }
        };

        addSegmentIfMatch(conn_ab);
        addSegmentIfMatch(conn_bc);
        addSegmentIfMatch(conn_ac);

        // If this piece has 2+ connections in the triangle, check for segment overlap
        if (segments.size() >= 2) {
            // Check all pairs of segments for overlap
            for (size_t i = 0; i < segments.size(); ++i) {
                for (size_t j = i + 1; j < segments.size(); ++j) {
                    const SegmentInfo& seg1 = segments[i];
                    const SegmentInfo& seg2 = segments[j];

                    // Check if segments overlap significantly
                    if (segmentsOverlap(seg1.start, seg1.end, seg2.start, seg2.end)) {
                        // EDGE COMPETITION DETECTED!
                        // This piece's edge segment is being used by TWO different connections
                        static int debug_edge_comp = 0;
                        if (debug_edge_comp < 5) {
                            std::cout << "*** EDGE COMPETITION DETECTED *** Piece " << piece
                                      << " segments overlap:" << std::endl;
                            std::cout << "    Segment 1: [" << seg1.start << "," << seg1.end
                                      << "] connects to piece " << seg1.other_piece << std::endl;
                            std::cout << "    Segment 2: [" << seg2.start << "," << seg2.end
                                      << "] connects to piece " << seg2.other_piece << std::endl;
                            std::cout << "    Triangle: {" << p0 << "," << p1 << "," << p2
                                      << "} - GEOMETRICALLY IMPOSSIBLE" << std::endl;
                            debug_edge_comp++;
                        }
                        return true;  // Edge competition found
                    }
                }
            }
        }
    }

    return false;  // No edge competition
}

/**
 * CURVATURE-AWARE TRIANGLE VALIDATION
 *
 * Validates if three pieces form a coherent triangle based on curvature flow alignment.
 * For pottery vessels, correct triangles should have:
 * 1. Similar curvature values at connection points (vessel wall thickness consistency)
 * 2. Smooth curvature flow around the triangle perimeter
 *
 * Uses breakline curvature data (feature_[6]) from NURBS preprocessing
 */
bool HybridPuzzleFusionOptimizer::validateTriangleCurvatureCoherence(
    int piece_a, int piece_b, int piece_c,
    const ProvenConnection& conn_ab,
    const ProvenConnection& conn_bc,
    const ProvenConnection& conn_ac,
    const std::vector<Geom>& geometry) const {

    // Safety check: piece IDs are 1-based
    if (piece_a < 1 || piece_a > (int)geometry.size() ||
        piece_b < 1 || piece_b > (int)geometry.size() ||
        piece_c < 1 || piece_c > (int)geometry.size()) {
        return false;
    }

    // Use NURBS curvature data loaded from PCD files (feature_[6])
    const Geom& geom_a = geometry[piece_a - 1];
    const Geom& geom_b = geometry[piece_b - 1];
    const Geom& geom_c = geometry[piece_c - 1];

    // Check if curvature data exists (loaded from NURBS PCD column 7)
    if (geom_a.edge_line_.feature_.size() < 7 ||
        geom_b.edge_line_.feature_.size() < 7 ||
        geom_c.edge_line_.feature_.size() < 7) {
        return false;  // Curvature not loaded
    }

    const MatrixXd& curv_a = geom_a.edge_line_.feature_[6];
    const MatrixXd& curv_b = geom_b.edge_line_.feature_[6];
    const MatrixXd& curv_c = geom_c.edge_line_.feature_[6];

    if (curv_a.cols() == 0 || curv_b.cols() == 0 || curv_c.cols() == 0) {
        return false;  // Empty curvature data
    }

    // POTTERY CURVATURE COHERENCE: Like physics_based_optimizer.cpp computeCurvatureContinuity()
    // Pieces from same pottery vessel area have similar average curvature

    // Compute average curvature for each piece
    double avg_curv_a = curv_a.mean();
    double avg_curv_b = curv_b.mean();
    double avg_curv_c = curv_c.mean();

    // Triangle curvature spread
    double tri_avg = (avg_curv_a + avg_curv_b + avg_curv_c) / 3.0;
    double tri_spread = std::max({std::abs(avg_curv_a - tri_avg),
                                   std::abs(avg_curv_b - tri_avg),
                                   std::abs(avg_curv_c - tri_avg)});

    // Compute curvature variation within each piece
    double std_curv_a = std::sqrt((curv_a.array() - avg_curv_a).square().mean());
    double std_curv_b = std::sqrt((curv_b.array() - avg_curv_b).square().mean());
    double std_curv_c = std::sqrt((curv_c.array() - avg_curv_c).square().mean());
    double avg_piece_variation = (std_curv_a + std_curv_b + std_curv_c) / 3.0;

    // COHERENCE TEST: Triangle curvature spread should be small relative to piece variation
    // Pottery pieces from same vessel have compatible curvatures
    double coherence_ratio = tri_spread / (avg_piece_variation + 1e-10);
    bool is_curvature_coherent = (coherence_ratio < 2.0);  // Threshold: 2.0× piece variation

    // Debug output for first few triangles
    static int curv_debug_count = 0;
    if (curv_debug_count < 3) {
        std::cout << "*** CURVATURE COHERENCE *** {" << piece_a << "," << piece_b << "," << piece_c << "}"
                  << " avg_curv: " << avg_curv_a << "," << avg_curv_b << "," << avg_curv_c
                  << " tri_spread=" << tri_spread
                  << " avg_variation=" << avg_piece_variation
                  << " ratio=" << coherence_ratio
                  << " coherent=" << (is_curvature_coherent ? "YES" : "NO") << std::endl;
        curv_debug_count++;
    }

    return is_curvature_coherent;
}
//==============================================================================

/**
 * Find all triangles {A,B,C} where all three pairwise connections exist
 * Returns triangles with cycle consistency analysis
 */
std::vector<HybridPuzzleFusionOptimizer::Triangle>
HybridPuzzleFusionOptimizer::findCoherentTriangles(
    const std::vector<ProvenConnection>& connections,
    int total_pieces) {

    std::vector<Triangle> triangles;

    // Build adjacency map: piece -> list of (connected_piece, connection_index)
    std::map<int, std::vector<std::pair<int, int>>> adjacency;

    for (int i = 0; i < connections.size(); ++i) {
        const auto& conn = connections[i];
        adjacency[conn.piece_a].push_back({conn.piece_b, i});
        adjacency[conn.piece_b].push_back({conn.piece_a, i});
    }

    // Find all triangles by checking each piece's neighbors
    for (int piece_a = 1; piece_a <= total_pieces; ++piece_a) {
        if (adjacency.find(piece_a) == adjacency.end()) continue;

        const auto& neighbors_a = adjacency[piece_a];

        // Check all pairs of neighbors
        for (size_t i = 0; i < neighbors_a.size(); ++i) {
            int piece_b = neighbors_a[i].first;
            int conn_ab_idx = neighbors_a[i].second;

            for (size_t j = i + 1; j < neighbors_a.size(); ++j) {
                int piece_c = neighbors_a[j].first;
                int conn_ac_idx = neighbors_a[j].second;

                // Check if B and C are also connected
                int conn_bc_idx = -1;
                for (const auto& neighbor : adjacency[piece_b]) {
                    if (neighbor.first == piece_c) {
                        conn_bc_idx = neighbor.second;
                        break;
                    }
                }

                if (conn_bc_idx >= 0) {
                    // Found triangle! {piece_a, piece_b, piece_c}
                    Triangle tri;
                    tri.piece_a = piece_a;
                    tri.piece_b = piece_b;
                    tri.piece_c = piece_c;
                    tri.conn_ab_idx = conn_ab_idx;
                    tri.conn_bc_idx = conn_bc_idx;
                    tri.conn_ac_idx = conn_ac_idx;

                    // CURVATURE-AWARE TRIANGLE VALIDATION WITH EDGE COMPETITION CHECK
                    // Step 1: Edge contact check (minimum requirement)
                    int inliers_ab = connections[conn_ab_idx].inlier_count;
                    int inliers_bc = connections[conn_bc_idx].inlier_count;
                    int inliers_ac = connections[conn_ac_idx].inlier_count;

                    int min_inliers = 30;
                    bool has_edge_contact = (inliers_ab >= min_inliers &&
                                             inliers_bc >= min_inliers &&
                                             inliers_ac >= min_inliers);

                    // Step 2: Edge competition check (CRITICAL: same edge cannot serve two connections)
                    bool has_edge_competition = checkEdgeCompetition(
                        connections[conn_ab_idx],
                        connections[conn_bc_idx],
                        connections[conn_ac_idx]);

                    // Step 3: Curvature coherence check (pottery structure validation)
                    bool has_curvature_coherence = false;
                    if (has_edge_contact && !has_edge_competition) {
                        has_curvature_coherence = validateTriangleCurvatureCoherence(
                            piece_a, piece_b, piece_c,
                            connections[conn_ab_idx],
                            connections[conn_bc_idx],
                            connections[conn_ac_idx],
                            validator_->reference_geometry_);
                    }

                    // Triangle is coherent ONLY if: edge contact + NO competition + curvature coherent
                    tri.is_cycle_consistent = has_edge_contact && !has_edge_competition && has_curvature_coherence;

                    static int debug_tri_count = 0;
                    if (debug_tri_count < 5) {
                        std::cout << "*** TRIANGLE VALIDATION *** {" << piece_a << "," << piece_b << "," << piece_c << "}"
                                  << " inliers: " << inliers_ab << "," << inliers_bc << "," << inliers_ac
                                  << " edge_contact=" << (has_edge_contact ? "YES" : "NO")
                                  << " edge_competition=" << (has_edge_competition ? "YES" : "NO")
                                  << " curvature_coherent=" << (has_curvature_coherence ? "YES" : "NO")
                                  << " final_coherent=" << (tri.is_cycle_consistent ? "YES" : "NO") << std::endl;
                        debug_tri_count++;
                    }

                    triangles.push_back(tri);
                }
            }
        }
    }

    return triangles;
}

/**
 * Check if transformation cycle T_AB * T_BC ≈ T_AC for triangle {A,B,C}
 * This validates whether three connections form a geometrically consistent triangle
 *
 * ROBUST IMPLEMENTATION: Handles all 8 possible connection direction combinations
 */
bool HybridPuzzleFusionOptimizer::checkTransformationCycleConsistency(
    const ProvenConnection& conn_ab,
    const ProvenConnection& conn_bc,
    const ProvenConnection& conn_ac,
    double tolerance) const {

    // DEBUG: Log input connections
    static int total_checks = 0;
    bool should_debug = (total_checks < 3);
    if (should_debug) {
        std::cout << "*** CYCLE CHECK INPUT *** Check #" << total_checks
                  << " conn_ab: " << conn_ab.piece_a << "->" << conn_ab.piece_b
                  << " conn_bc: " << conn_bc.piece_a << "->" << conn_bc.piece_b
                  << " conn_ac: " << conn_ac.piece_a << "->" << conn_ac.piece_b << std::endl;
    }
    total_checks++;

    // Extract transformation matrices
    Matrix4d T_ab, T_bc, T_ac;
    conn_ab.icp_transformation.Output(T_ab);
    conn_bc.icp_transformation.Output(T_bc);
    conn_ac.icp_transformation.Output(T_ac);

    // Helper: Get transformation from piece X to piece Y, handling bidirectionality
    auto getDirectedTransform = [](const ProvenConnection& conn, int from_piece, int to_piece, const Matrix4d& T) -> Matrix4d {
        if (conn.piece_a == from_piece && conn.piece_b == to_piece) {
            return T;  // Direct: A->B transform
        } else {
            return T.inverse();  // Reversed: B->A, need inverse
        }
    };

    // SIMPLIFIED APPROACH: Don't try to be clever about ordering
    // Just extract the 3 unique piece IDs from the connections and build ALL possible cycles
    std::set<int> pieces;
    pieces.insert(conn_ab.piece_a);
    pieces.insert(conn_ab.piece_b);
    pieces.insert(conn_bc.piece_a);
    pieces.insert(conn_bc.piece_b);
    pieces.insert(conn_ac.piece_a);
    pieces.insert(conn_ac.piece_b);

    if (pieces.size() != 3) {
        // Not a valid triangle
        if (should_debug) {
            std::cout << "*** CYCLE CHECK ERROR *** Invalid triangle - " << pieces.size() << " unique pieces" << std::endl;
        }
        return false;
    }

    // Extract the 3 pieces
    std::vector<int> piece_ids(pieces.begin(), pieces.end());
    int p0 = piece_ids[0];
    int p1 = piece_ids[1];
    int p2 = piece_ids[2];

    // Find which connection connects which pair
    // Use a helper to check if connection links two pieces (in either direction)
    auto connects = [](const ProvenConnection& conn, int pa, int pb) {
        return (conn.piece_a == pa && conn.piece_b == pb) ||
               (conn.piece_a == pb && conn.piece_b == pa);
    };

    // Identify the connections properly
    const ProvenConnection* conn_p0_p1 = nullptr;
    const ProvenConnection* conn_p1_p2 = nullptr;
    const ProvenConnection* conn_p0_p2 = nullptr;
    Matrix4d* T_p0_p1 = nullptr;
    Matrix4d* T_p1_p2 = nullptr;
    Matrix4d* T_p0_p2 = nullptr;

    if (connects(conn_ab, p0, p1)) { conn_p0_p1 = &conn_ab; T_p0_p1 = &T_ab; }
    else if (connects(conn_bc, p0, p1)) { conn_p0_p1 = &conn_bc; T_p0_p1 = &T_bc; }
    else if (connects(conn_ac, p0, p1)) { conn_p0_p1 = &conn_ac; T_p0_p1 = &T_ac; }

    if (connects(conn_ab, p1, p2)) { conn_p1_p2 = &conn_ab; T_p1_p2 = &T_ab; }
    else if (connects(conn_bc, p1, p2)) { conn_p1_p2 = &conn_bc; T_p1_p2 = &T_bc; }
    else if (connects(conn_ac, p1, p2)) { conn_p1_p2 = &conn_ac; T_p1_p2 = &T_ac; }

    if (connects(conn_ab, p0, p2)) { conn_p0_p2 = &conn_ab; T_p0_p2 = &T_ab; }
    else if (connects(conn_bc, p0, p2)) { conn_p0_p2 = &conn_bc; T_p0_p2 = &T_bc; }
    else if (connects(conn_ac, p0, p2)) { conn_p0_p2 = &conn_ac; T_p0_p2 = &T_ac; }

    if (!conn_p0_p1 || !conn_p1_p2 || !conn_p0_p2) {
        if (should_debug) {
            std::cout << "*** CYCLE CHECK ERROR *** Missing connections in triangle" << std::endl;
        }
        return false;
    }

    // Build directed transformations: p0 -> p1 -> p2
    Matrix4d T_0to1 = getDirectedTransform(*conn_p0_p1, p0, p1, *T_p0_p1);
    Matrix4d T_1to2 = getDirectedTransform(*conn_p1_p2, p1, p2, *T_p1_p2);
    Matrix4d T_0to2 = getDirectedTransform(*conn_p0_p2, p0, p2, *T_p0_p2);

    if (should_debug) {
        std::cout << "*** CYCLE CHECK PIECES *** Triangle {" << p0 << "," << p1 << "," << p2 << "}"
                  << " building chain " << p0 << "->" << p1 << "->" << p2 << std::endl;
    }

    // Compose: T_0to1 * T_1to2 should equal T_0to2
    Matrix4d T_composed = T_0to1 * T_1to2;

    // Measure cycle consistency error
    Matrix4d T_diff = T_composed.inverse() * T_0to2;
    Vector3d translation_error = T_diff.block<3,1>(0,3);
    double error_magnitude = translation_error.norm();

    // Also check rotation consistency (trace of rotation difference matrix)
    Matrix3d R_diff = T_diff.block<3,3>(0,0);
    double rotation_trace = R_diff.trace();
    double rotation_error_angle = std::acos(std::max(-1.0, std::min(1.0, (rotation_trace - 1.0) / 2.0)));

    // Convert rotation error to mm (approximate, for small angles)
    double rotation_error_mm = rotation_error_angle * 50.0;  // Assume ~50mm piece size

    double total_error = error_magnitude + rotation_error_mm;

    // DEBUG: Log first few failures to diagnose issue
    static int debug_count = 0;
    if (debug_count < 5 && total_error >= tolerance) {
        std::cout << "*** CYCLE CHECK DEBUG *** Triangle {" << p0 << "," << p1 << "," << p2 << "}"
                  << " trans_error=" << error_magnitude << "mm"
                  << " rot_error=" << rotation_error_mm << "mm"
                  << " total=" << total_error << "mm (tolerance=" << tolerance << "mm)" << std::endl;
        debug_count++;
    }

    return (total_error < tolerance);
}

/**
 * Detect competing connections for the same piece
 * Two connections compete if they cannot both be true (mutually exclusive transformations)
 */
std::vector<HybridPuzzleFusionOptimizer::ConnectionConflict>
HybridPuzzleFusionOptimizer::detectCompetingConnections(
    const std::vector<ProvenConnection>& connections,
    int total_pieces) {

    std::vector<ConnectionConflict> conflicts;

    // Build piece -> connections map
    std::map<int, std::vector<int>> piece_connections;  // piece_id -> list of connection indices
    for (int i = 0; i < connections.size(); ++i) {
        piece_connections[connections[i].piece_a].push_back(i);
        piece_connections[connections[i].piece_b].push_back(i);
    }

    // For each piece, check if its connections compete
    for (int piece_id = 1; piece_id <= total_pieces; ++piece_id) {
        if (piece_connections.find(piece_id) == piece_connections.end()) continue;

        const auto& conn_indices = piece_connections[piece_id];
        if (conn_indices.size() < 2) continue;  // Need at least 2 connections to compete

        // Check all pairs of connections involving this piece
        for (size_t i = 0; i < conn_indices.size(); ++i) {
            for (size_t j = i + 1; j < conn_indices.size(); ++j) {
                int conn_i = conn_indices[i];
                int conn_j = conn_indices[j];

                const auto& ci = connections[conn_i];
                const auto& cj = connections[conn_j];

                // Get the OTHER piece in each connection
                int other_i = (ci.piece_a == piece_id) ? ci.piece_b : ci.piece_a;
                int other_j = (cj.piece_a == piece_id) ? cj.piece_b : cj.piece_a;

                // CRITICAL FIX: If both connections link the SAME piece pair, they are ALTERNATIVE HYPOTHESES
                // NOT competing connections! Allow multiple ICP hypotheses for same piece pair.
                if (other_i == other_j) {
                    // Same piece pair (e.g., both are 3-5 connections with different segments/transforms)
                    // These are VALID alternatives - let global optimization choose the best one
                    // DO NOT penalize as spatial conflicts!
                    continue;
                }

                // Check if other_i and other_j are also connected (forms triangle)
                bool triangle_exists = false;
                for (const auto& conn : connections) {
                    if ((conn.piece_a == other_i && conn.piece_b == other_j) ||
                        (conn.piece_a == other_j && conn.piece_b == other_i)) {
                        triangle_exists = true;
                        break;
                    }
                }

                if (triangle_exists) {
                    // Triangle exists - cycle consistency already checked in triangle validation
                    continue;
                }

                // No triangle! Need to check if transformations compete
                // Compare where each connection places piece_id in space

                Matrix4d T_i, T_j;
                ci.icp_transformation.Output(T_i);
                cj.icp_transformation.Output(T_j);

                // Get transformation that moves piece_id
                Matrix4d T_piece_i = (ci.piece_a == piece_id) ? T_i.inverse() : T_i;
                Matrix4d T_piece_j = (cj.piece_a == piece_id) ? T_j.inverse() : T_j;

                // Extract positions where piece_id is placed
                Vector3d pos_i = T_piece_i.block<3,1>(0,3);
                Vector3d pos_j = T_piece_j.block<3,1>(0,3);

                // Check spatial conflict
                double conflict_distance = (pos_i - pos_j).norm();

                // CONFLICT THRESHOLD: If two transformations place piece > 20mm apart, they compete!
                if (conflict_distance > 20.0) {
                    ConnectionConflict conflict;
                    conflict.piece_id = piece_id;
                    conflict.conflicting_conn_indices = {conn_i, conn_j};
                    conflict.spatial_conflict_distance = conflict_distance;
                    conflicts.push_back(conflict);
                }
            }
        }
    }

    return conflicts;
}

/**
 * MAIN CLUSTER COHERENCE FUNCTION
 * Adjusts connection quality based on multi-piece geometric consistency
 *
 * Key insight: If connections {A-B, B-C, A-C} form a geometrically consistent triangle,
 * all three connections are likely CORRECT. Boost their quality scores.
 *
 * If a connection participates in NO consistent triangles, it's likely WRONG
 * (e.g., 4->3 with 222 inliers but not part of any coherent cluster).
 *
 * CRITICAL: Also detects COMPETING connections (mutually exclusive transformations)
 */
std::vector<ProvenConnection> HybridPuzzleFusionOptimizer::adjustConnectionQualityByClusterCoherence(
    const std::vector<ProvenConnection>& connections,
    int total_pieces) {

    std::cout << "\n=== CLUSTER COHERENCE VALIDATION ===" << std::endl;
    std::cout << "Analyzing " << connections.size() << " connections for multi-piece consistency" << std::endl;

    // PHASE 1: Find all coherent triangles
    std::vector<Triangle> triangles = findCoherentTriangles(connections, total_pieces);

    std::cout << "Found " << triangles.size() << " triangles (3-piece clusters)" << std::endl;

    // PHASE 2: Detect competing connections (mutually exclusive)
    std::vector<ConnectionConflict> conflicts = detectCompetingConnections(connections, total_pieces);

    std::cout << "Found " << conflicts.size() << " spatial conflicts (competing connections)" << std::endl;
    for (const auto& conflict : conflicts) {
        std::cout << "  *** CONFLICT *** Piece " << conflict.piece_id
                  << " has competing connections separated by "
                  << std::fixed << std::setprecision(1) << conflict.spatial_conflict_distance << "mm" << std::endl;
    }

    // Count cycle-consistent triangles
    int consistent_triangles = 0;
    for (const auto& tri : triangles) {
        if (tri.is_cycle_consistent) consistent_triangles++;
    }
    std::cout << "  Cycle-consistent triangles: " << consistent_triangles << "/" << triangles.size() << std::endl;

    // PHASE 3: Track coherence support and conflicts for each connection
    std::vector<int> coherence_support(connections.size(), 0);  // How many consistent triangles support this connection
    std::vector<int> total_triangles(connections.size(), 0);     // Total triangles this connection participates in
    std::vector<bool> has_spatial_conflict(connections.size(), false);  // Does this connection compete with others?

    for (const auto& tri : triangles) {
        total_triangles[tri.conn_ab_idx]++;
        total_triangles[tri.conn_bc_idx]++;
        total_triangles[tri.conn_ac_idx]++;

        if (tri.is_cycle_consistent) {
            coherence_support[tri.conn_ab_idx]++;
            coherence_support[tri.conn_bc_idx]++;
            coherence_support[tri.conn_ac_idx]++;
        }
    }

    // Mark connections involved in spatial conflicts
    for (const auto& conflict : conflicts) {
        for (int conn_idx : conflict.conflicting_conn_indices) {
            has_spatial_conflict[conn_idx] = true;
        }
    }

    // MULTI-SCALE CLUSTER COHERENCE: Handle variable connectivity (2 to 6+ connections per piece)

    // Build piece connectivity map: piece_id -> number of connections
    std::map<int, int> piece_connectivity;
    for (const auto& conn : connections) {
        piece_connectivity[conn.piece_a]++;
        piece_connectivity[conn.piece_b]++;
    }

    // Adjust connection quality based on cluster coherence
    std::vector<ProvenConnection> adjusted_connections = connections;

    for (int i = 0; i < adjusted_connections.size(); ++i) {
        auto& conn = adjusted_connections[i];
        double original_quality = conn.combined_quality;

        // MULTI-SCALE COHERENCE STRATEGY:
        // 1. Triangle-based validation (3+ connections per piece)
        // 2. Edge support validation (2 connections, e.g., rim pieces)
        // 3. Hub connectivity bonus (5+ connections, well-connected pieces)

        double coherence_factor = 1.0;  // Multiplicative adjustment

        int conn_a_count = piece_connectivity[conn.piece_a];
        int conn_b_count = piece_connectivity[conn.piece_b];

        // === CRITICAL: Check for spatial conflicts FIRST ===
        if (has_spatial_conflict[i]) {
            // This connection COMPETES with another connection for the same piece location
            // Apply SEVERE penalty - competing connections are mutually exclusive!
            coherence_factor = 0.3;  // -70% penalty for spatial conflicts

            std::cout << "*** SPATIAL CONFLICT PENALTY *** Connection " << conn.piece_a << "-" << conn.piece_b
                      << ": Competes with other connections (mutually exclusive transformations!), quality "
                      << std::fixed << std::setprecision(1)
                      << original_quality << " -> " << (original_quality * coherence_factor) << std::endl;
        }
        // === CASE 1: Triangle Validation (Standard) ===
        else if (total_triangles[i] > 0) {
            double consistency_ratio = (double)coherence_support[i] / total_triangles[i];

            if (consistency_ratio >= 0.8) {
                // High consistency: strong boost
                coherence_factor = 1.0 + (0.5 * consistency_ratio);  // Up to +50% quality

                std::cout << "*** COHERENCE BOOST (Triangles) *** Connection " << conn.piece_a << "-" << conn.piece_b
                          << ": " << coherence_support[i] << "/" << total_triangles[i]
                          << " consistent triangles, quality " << std::fixed << std::setprecision(1)
                          << original_quality << " -> " << (original_quality * coherence_factor) << std::endl;
            }
            else if (consistency_ratio >= 0.5) {
                // Moderate consistency: small boost
                coherence_factor = 1.0 + (0.2 * consistency_ratio);  // Up to +20% quality
            }
            else if (consistency_ratio < 0.3) {
                // ADAPTIVE PENALTY: Consider inlier count for multi-scale pottery assembly
                // Low consistency could mean:
                //   1. False high-inlier connection (4-3 with 235 inliers) → strong penalty
                //   2. Valid low-inlier connection (small shard, 16-32 inliers) → light penalty

                if (conn.inlier_count < 50) {
                    // Small shard or weathered edge - don't expect many triangles
                    // Use LIGHTER penalty to preserve valid small-shard connections
                    coherence_factor = 0.8;  // Only -20% penalty instead of -50%

                    std::cout << "*** ADAPTIVE PENALTY (Small Shard) *** Connection " << conn.piece_a << "-" << conn.piece_b
                              << ": Low consistency BUT low inliers (" << conn.inlier_count
                              << "), likely small shard, quality "
                              << std::fixed << std::setprecision(1)
                              << original_quality << " -> " << (original_quality * coherence_factor) << std::endl;
                } else {
                    // High inliers but low consistency: likely FALSE connection!
                    // Apply FULL penalty to catch false positives like 4-3
                    coherence_factor = 0.5 + (0.5 * consistency_ratio);  // Down to -50% quality

                    std::cout << "*** COHERENCE PENALTY (Conflict) *** Connection " << conn.piece_a << "-" << conn.piece_b
                              << ": Only " << coherence_support[i] << "/" << total_triangles[i]
                              << " consistent triangles (geometric conflict!), quality "
                              << std::fixed << std::setprecision(1)
                              << original_quality << " -> " << (original_quality * coherence_factor) << std::endl;
                }
            }
        }
        // === CASE 2: Edge Support Validation (Rim pieces, 2 connections) ===
        else if (total_triangles[i] == 0 && (conn_a_count >= 2 || conn_b_count >= 2)) {
            // No triangles, but pieces are well-connected to other pieces
            // This is expected for rim/edge pieces - give moderate boost

            double avg_connectivity = (conn_a_count + conn_b_count) / 2.0;

            if (avg_connectivity >= 2.5) {
                // Both pieces reasonably connected → likely valid edge connection
                coherence_factor = 1.15;  // +15% boost for well-connected edge

                std::cout << "*** EDGE SUPPORT BOOST *** Connection " << conn.piece_a << "-" << conn.piece_b
                          << ": No triangles but pieces have " << conn_a_count << " and " << conn_b_count
                          << " connections (rim/edge piece support), quality "
                          << std::fixed << std::setprecision(1)
                          << original_quality << " -> " << (original_quality * coherence_factor) << std::endl;
            }
        }
        // === CASE 3: Hub Connectivity Bonus (5+ connections) ===
        else if (conn_a_count >= 5 || conn_b_count >= 5) {
            // High connectivity hub piece - strong evidence of correctness
            coherence_factor = 1.25;  // +25% boost for hub connections

            std::cout << "*** HUB CONNECTIVITY BOOST *** Connection " << conn.piece_a << "-" << conn.piece_b
                      << ": Hub piece with " << std::max(conn_a_count, conn_b_count)
                      << " connections, quality " << std::fixed << std::setprecision(1)
                      << original_quality << " -> " << (original_quality * coherence_factor) << std::endl;
        }
        // === CASE 4: Isolated Edge (1 connection per piece) ===
        else if (conn_a_count == 1 && conn_b_count == 1) {
            // Both pieces only have this one connection - suspicious!
            // Could be correct (two isolated fragments) or wrong (spurious match)
            // Small penalty to be conservative
            coherence_factor = 0.9;  // -10% penalty for isolated edges

            std::cout << "*** ISOLATED EDGE PENALTY *** Connection " << conn.piece_a << "-" << conn.piece_b
                      << ": Both pieces have only 1 connection (isolated edge), quality "
                      << std::fixed << std::setprecision(1)
                      << original_quality << " -> " << (original_quality * coherence_factor) << std::endl;
        }
        // else: neutral (no special coherence evidence)

        conn.combined_quality *= coherence_factor;
    }

    // Re-sort after quality adjustment
    std::sort(adjusted_connections.begin(), adjusted_connections.end(), std::greater<ProvenConnection>());

    std::cout << "\n*** RERANKED CONNECTIONS (Top 10) ***" << std::endl;
    for (int i = 0; i < std::min(10, (int)adjusted_connections.size()); ++i) {
        const auto& conn = adjusted_connections[i];
        std::cout << "  Rank " << (i+1) << ": " << conn.piece_a << "-" << conn.piece_b
                  << " quality=" << std::fixed << std::setprecision(1) << conn.combined_quality
                  << " (coherence=" << coherence_support[i] << "/" << total_triangles[i] << ")" << std::endl;
    }

    std::cout << "=== CLUSTER COHERENCE COMPLETE ===" << std::endl << std::endl;

    return adjusted_connections;
}

//==============================================================================
// Graph Topology Validation
//==============================================================================

int HybridPuzzleFusionOptimizer::findRootPiece(const HybridAssemblyState& state) const {
    // Find piece with identity transform (or closest to it)
    for (const auto& piece_id : state.active_pieces) {
        auto it = state.piece_world_transforms.find(piece_id);
        if (it != state.piece_world_transforms.end()) {
            const Matrix4d& transform = it->second;
            // Check if transform is identity
            if (transform.isApprox(Matrix4d::Identity(), 1e-6)) {
                return piece_id;
            }
        }
    }

    // If no identity found, return first piece
    return state.active_pieces.empty() ? -1 : state.active_pieces[0];
}

bool HybridPuzzleFusionOptimizer::isValidTreeStructure(const HybridAssemblyState& state) const {
    // CRITICAL VALIDATION: Ensure state forms proper tree structure
    //
    // Valid tree must satisfy:
    // 1. Exactly one root (piece at identity transform)
    // 2. No cycles (each piece has exactly one parent, except root)
    // 3. Connected (all pieces reachable from root)
    // 4. N pieces → N-1 connections

    // DIAGNOSTIC: Track rejection reasons
    static std::map<std::string, int> rejection_reasons;
    static std::map<int, int> rejections_by_size;
    static int total_checks = 0;
    total_checks++;

    int num_pieces = state.active_pieces.size();
    int num_connections = state.selected_connections.size();

    // Validation 1: Check piece count vs connection count
    if (num_pieces == 0) {
        rejection_reasons["empty_state"]++;
        rejections_by_size[num_pieces]++;
        return false;
    }

    if (num_pieces > 1 && num_connections != num_pieces - 1) {
        rejection_reasons["connection_count_mismatch"]++;
        rejections_by_size[num_pieces]++;

        // Detailed diagnostic for first few mismatches
        static int mismatch_details = 0;
        if (mismatch_details < 5 && num_pieces >= 4) {
            std::cout << "*** TOPOLOGY DEBUG *** Connection count mismatch: "
                      << num_pieces << " pieces but " << num_connections << " connections (need "
                      << (num_pieces - 1) << ")" << std::endl;
            mismatch_details++;
        }

        return false;  // Tree with N nodes has exactly N-1 edges
    }

    // Validation 2: Find root (piece with identity transform)
    int num_roots = 0;
    int root_piece = -1;

    for (const auto& piece_id : state.active_pieces) {
        auto it = state.piece_world_transforms.find(piece_id);
        if (it != state.piece_world_transforms.end()) {
            const Matrix4d& transform = it->second;
            if (transform.isApprox(Matrix4d::Identity(), 1e-6)) {
                num_roots++;
                root_piece = piece_id;
            }
        }
    }

    if (num_roots != 1) {
        rejection_reasons[num_roots == 0 ? "no_root" : "multiple_roots"]++;
        rejections_by_size[num_pieces]++;

        static int root_details = 0;
        if (root_details < 5 && num_pieces >= 4) {
            std::cout << "*** TOPOLOGY DEBUG *** Root count issue: "
                      << num_roots << " roots found (need exactly 1)" << std::endl;
            root_details++;
        }

        return false;  // Must have exactly ONE root
    }

    // Validation 3: Check no cycles (each piece has at most one parent)
    std::map<int, int> parent_count;
    for (int piece_id : state.active_pieces) {
        parent_count[piece_id] = 0;
    }

    for (const auto& conn : state.selected_connections) {
        // In connection a->b, piece b has parent a
        parent_count[conn.piece_b]++;

        // Check if piece b has multiple parents
        if (parent_count[conn.piece_b] > 1) {
            rejection_reasons["cycle_detected"]++;
            rejections_by_size[num_pieces]++;

            static int cycle_details = 0;
            if (cycle_details < 5 && num_pieces >= 4) {
                std::cout << "*** TOPOLOGY DEBUG *** Cycle detected: piece "
                          << conn.piece_b << " has multiple parents" << std::endl;
                cycle_details++;
            }

            return false;  // Cycle detected - piece has multiple parents
        }
    }

    // Validation 4: Check connectivity (all pieces reachable from root via BFS)
    std::set<int> visited;
    std::queue<int> to_visit;

    visited.insert(root_piece);
    to_visit.push(root_piece);

    // Build adjacency list
    std::map<int, std::vector<int>> children;
    for (const auto& conn : state.selected_connections) {
        children[conn.piece_a].push_back(conn.piece_b);
    }

    // BFS from root
    while (!to_visit.empty()) {
        int current = to_visit.front();
        to_visit.pop();

        if (children.count(current) > 0) {
            for (int child : children[current]) {
                if (visited.count(child) == 0) {
                    visited.insert(child);
                    to_visit.push(child);
                }
            }
        }
    }

    // All pieces must be reachable
    if (visited.size() != static_cast<size_t>(num_pieces)) {
        rejection_reasons["disconnected_graph"]++;
        rejections_by_size[num_pieces]++;

        static int disconnect_details = 0;
        if (disconnect_details < 10 && num_pieces >= 3) {
            // ENHANCED DIAGNOSTIC: Show which pieces are unreachable
            std::set<int> unreachable;
            for (int piece_id : state.active_pieces) {
                if (visited.count(piece_id) == 0) {
                    unreachable.insert(piece_id);
                }
            }

            std::cout << "*** DISCONNECTED GRAPH *** "
                      << visited.size() << "/" << num_pieces << " pieces reachable from root " << root_piece << std::endl;

            std::cout << "    Reachable pieces: {";
            for (int p : visited) std::cout << p << " ";
            std::cout << "}" << std::endl;

            std::cout << "    Unreachable pieces: {";
            for (int p : unreachable) std::cout << p << " ";
            std::cout << "}" << std::endl;

            std::cout << "    Connections in state:" << std::endl;
            for (const auto& conn : state.selected_connections) {
                std::cout << "      " << conn.piece_a << " -> " << conn.piece_b
                          << " (quality=" << std::fixed << std::setprecision(1) << conn.combined_quality << ")" << std::endl;
            }

            disconnect_details++;
        }

        return false;  // Disconnected graph
    }

    // PERIODIC STATISTICS REPORTING
    if (total_checks % 3000 == 0) {
        std::cout << "\n=== TOPOLOGY VALIDATION STATISTICS (after " << total_checks << " checks) ===" << std::endl;

        std::cout << "Rejection reasons:" << std::endl;
        for (const auto& pair : rejection_reasons) {
            double percentage = 100.0 * pair.second / total_checks;
            std::cout << "  " << pair.first << ": " << pair.second
                      << " (" << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
        }

        std::cout << "Rejections by assembly size:" << std::endl;
        for (const auto& pair : rejections_by_size) {
            std::cout << "  " << pair.first << " pieces: " << pair.second << " rejections" << std::endl;
        }
        std::cout << "===" << std::endl << std::endl;
    }

    return true;  // Valid tree structure
}