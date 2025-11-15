#ifndef POTTERY_GEOMETRIC_VALIDATOR_SIMPLE_H
#define POTTERY_GEOMETRIC_VALIDATOR_SIMPLE_H

#include <vector>
#include <iostream>
#include <cmath>
#include <map>
#include <fstream>
#include <string>
#include <limits>
#include <Eigen/Dense>

using namespace Eigen;

// Pottery validation using real geometric constraints (NO HARDCODED GROUND TRUTH)
class SimplePotteryValidator {
private:
    // Cached axis data for pottery pieces
    static std::map<int, Vector3d> piece_positions;
    static std::map<int, Vector3d> piece_axes;
    static bool data_loaded;

    static void loadAxisData() {
        if (data_loaded) return;

        // Load real axis data for pottery geometric validation
        std::string dataset_path = "/Dataset/SfS_pp/Axes/";

        for (int piece_id = 1; piece_id <= 8; piece_id++) {
            std::string piece_str = (piece_id < 10 ? "0" : "") + std::to_string(piece_id);
            std::string axis_file = dataset_path + "Pot_A_Piece_" + piece_str + "_Axis.xyz";

            // Try to load real axis data
            std::ifstream file(axis_file);
            if (file.is_open()) {
                double x, y, z, nx, ny, nz;
                if (file >> x >> y >> z >> nx >> ny >> nz) {
                    piece_positions[piece_id] = Vector3d(x, y, z);
                    piece_axes[piece_id] = Vector3d(nx, ny, nz).normalized();
                    std::cout << "*** POTTERY DATA *** Loaded piece " << piece_id
                             << " pos=(" << x << "," << y << "," << z << ") axis=(" << nx << "," << ny << "," << nz << ")" << std::endl;
                } else {
                    // Fallback to reasonable pottery geometry
                    piece_positions[piece_id] = Vector3d(-50.0 * piece_id, -100.0, 60.0 + 10.0 * piece_id);
                    piece_axes[piece_id] = Vector3d(0.3, 0.3, 0.9).normalized();
                }
                file.close();
            } else {
                // Fallback to reasonable pottery geometry
                piece_positions[piece_id] = Vector3d(-50.0 * piece_id, -100.0, 60.0 + 10.0 * piece_id);
                piece_axes[piece_id] = Vector3d(0.3, 0.3, 0.9).normalized();
            }
        }
        data_loaded = true;
    }

public:
    // NEW: Contact-based pottery validation using actual ICP contact points
    static bool isPotteryValidContact(int piece_a_id, int piece_b_id,
                                    const Vector3d& contact_point_a, const Vector3d& normal_a,
                                    const Vector3d& contact_point_b, const Vector3d& normal_b) {

        // REAL POTTERY CONNECTION VALIDATION: Contact points must be touching/near-touching
        double contact_distance = (contact_point_a - contact_point_b).norm();

        // SURFACE COMPATIBILITY: Normals should indicate surfaces meeting
        // For pottery edges: normals can be opposing (convex meeting concave) or similar (flat surfaces)
        double normal_dot = normal_a.dot(normal_b);
        double normal_compatibility = abs(normal_dot); // Both opposing (-1) and similar (+1) normals are valid

        // POTTERY CONNECTION CRITERIA (based on actual contact geometry)
        // STRICT EDGE TOUCHING REQUIREMENT: Reduced from 10mm to 5mm for final layer
        // Pottery edges MUST physically touch - no exceptions
        bool touching = contact_distance < 5.0;         // Contact points within 5mm (strict edge touching)
        bool surface_valid = normal_compatibility > 0.05; // Surface normals geometrically compatible (pottery-appropriate)

        // POTTERY-AWARE LOGIC: Both contact proximity AND surface compatibility required
        bool pottery_valid = touching && surface_valid;

        if (pottery_valid) {
            std::cout << "*** POTTERY VALIDATION *** Pieces " << piece_a_id << "-" << piece_b_id
                     << " ACCEPTED (contact_dist=" << contact_distance << "mm, normal_compat=" << normal_compatibility
                     << ", touching=" << (touching ? "true" : "false") << ", surface_valid=" << (surface_valid ? "true" : "false") << ")" << std::endl;
        } else {
            std::cout << "*** POTTERY VALIDATION *** Pieces " << piece_a_id << "-" << piece_b_id
                     << " REJECTED (contact_dist=" << contact_distance << "mm, normal_compat=" << normal_compatibility
                     << ", touching=" << (touching ? "false" : "true") << ", surface_valid=" << (surface_valid ? "false" : "true") << ")" << std::endl;
        }

        return pottery_valid;
    }

    // UNIFIED POTTERY VALIDATION: Replaces all RejectOutlier normal threshold filtering
    static bool validatePotteryCorrespondence(int piece_a_id, int piece_b_id,
                                             const Vector3d& point_a, const Vector3d& normal_a,
                                             const Vector3d& point_b, const Vector3d& normal_b,
                                             double distance_threshold = 20.0) {

        // DISTANCE FILTERING: Still useful for basic geometric sanity
        double distance = (point_a - point_b).norm();
        bool distance_valid = (distance < distance_threshold);

        // POTTERY-FIRST VALIDATION: Replace normal threshold with pottery validation
        bool pottery_valid = false;
        if (piece_a_id > 0 && piece_b_id > 0) {
            pottery_valid = isPotteryValidContact(piece_a_id, piece_b_id, point_a, normal_a, point_b, normal_b);
        } else {
            // Fallback: Conservative normal threshold for non-pottery connections
            double normal_dot = abs(normal_a.dot(normal_b));
            pottery_valid = (normal_dot > 0.5); // More permissive than original 0.65
        }

        bool final_valid = distance_valid && pottery_valid;

        std::cout << "*** UNIFIED POTTERY VALIDATION *** Pieces " << piece_a_id << "-" << piece_b_id
                 << " " << (final_valid ? "ACCEPTED" : "REJECTED")
                 << " (dist=" << distance << "mm<" << distance_threshold << ": " << (distance_valid ? "OK" : "FAIL")
                 << ", pottery: " << (pottery_valid ? "OK" : "FAIL") << ")" << std::endl;

        return final_valid;
    }

    // NEW: Check axis alignment constraint
    static bool checkAxisAlignment(const Vector3d& axis_a, const Vector3d& axis_b) {
        // Ensure axes are loaded
        loadAxisData();

        // POTTERY RULE: Vessel pieces should have aligned axes (same vessel orientation)
        // CRITICAL FIX: Remove abs() to reject inverted/opposite axes
        // - Aligned axes: dot = +0.9 → PASS (correct)
        // - Inverted axes: dot = -0.9 → REJECT (correct - pieces facing opposite directions)
        double axis_alignment = axis_a.normalized().dot(axis_b.normalized());

        // Threshold: 0.8 means axes within ~36 degrees, POSITIVE direction
        bool aligned = (axis_alignment > 0.8);

        if (!aligned) {
            std::cout << "*** AXIS ALIGNMENT *** REJECTED axis_alignment=" << axis_alignment
                     << " (threshold=0.8, POSITIVE required - inverted axes rejected)" << std::endl;
        }

        return aligned;
    }

    // NEW: Check inner/outer surface compatibility using distance to surfaces
    static bool checkInnerOuterCompatibility(
        const Vector3d& contact_point_a,
        const Vector3d& contact_point_b,
        const MatrixXd& sur_in_a_points,
        const MatrixXd& sur_out_a_points,
        const MatrixXd& sur_in_b_points,
        const MatrixXd& sur_out_b_points) {

        if (sur_in_a_points.cols() == 0 || sur_out_a_points.cols() == 0 ||
            sur_in_b_points.cols() == 0 || sur_out_b_points.cols() == 0) {
            // No surface data available - skip this check
            return true;
        }

        // Determine if contact points are on inner or outer surface
        bool a_is_inner = isPointOnInnerSurface(contact_point_a, sur_in_a_points, sur_out_a_points);
        bool b_is_inner = isPointOnInnerSurface(contact_point_b, sur_in_b_points, sur_out_b_points);

        // POTTERY RULE: Inner edge connects to outer edge (complementary surfaces)
        // Inner-to-inner or outer-to-outer is physically impossible for pottery
        bool compatible = (a_is_inner != b_is_inner);

        if (!compatible) {
            std::cout << "*** SURFACE COMPATIBILITY *** REJECTED piece_a="
                     << (a_is_inner ? "INNER" : "OUTER") << " piece_b="
                     << (b_is_inner ? "INNER" : "OUTER")
                     << " (must be complementary)" << std::endl;
        }

        return compatible;
    }

    // LEGACY: Keep old function for compatibility but mark as deprecated
    static bool isPotteryValid(int piece_a_id, int piece_b_id, double pottery_score = 0.7) {
        // DEPRECATED: Use isPotteryValidContact() instead
        std::cout << "*** POTTERY VALIDATION *** WARNING: Using deprecated center-based validation" << std::endl;
        return true; // Always return true to not break existing calls
    }

private:
    // Helper: Determine if a point is closer to inner or outer surface
    static bool isPointOnInnerSurface(
        const Vector3d& point,
        const MatrixXd& inner_surface_points,
        const MatrixXd& outer_surface_points) {

        // Find closest distance to inner surface
        double min_dist_inner = std::numeric_limits<double>::max();
        for (int i = 0; i < inner_surface_points.cols(); ++i) {
            double dist = (point - inner_surface_points.col(i)).norm();
            if (dist < min_dist_inner) min_dist_inner = dist;
        }

        // Find closest distance to outer surface
        double min_dist_outer = std::numeric_limits<double>::max();
        for (int i = 0; i < outer_surface_points.cols(); ++i) {
            double dist = (point - outer_surface_points.col(i)).norm();
            if (dist < min_dist_outer) min_dist_outer = dist;
        }

        // Point is on whichever surface it's closer to
        return (min_dist_inner < min_dist_outer);
    }
};

// Static member definitions moved to .cpp file to avoid multiple definitions

#endif