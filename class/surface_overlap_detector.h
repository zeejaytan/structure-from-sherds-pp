#pragma once

#include <iostream>
#include <vector>
#include <Eigen/Dense>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/search/kdtree.h>

using namespace std;
using namespace Eigen;

struct SurfaceOverlapResult {
    bool has_overlap;
    double overlap_volume;
    double overlap_percentage;
    int overlap_point_count;
    Vector3d overlap_center;
};

/**
 * Enhanced 3D Surface Overlap Detector for Pottery Assembly
 * Integrates with existing GeometryValidator for pottery-aware overlap detection
 */
class Enhanced3DOverlapDetector {
private:
    double distance_threshold_;
    double volume_threshold_;

public:
    Enhanced3DOverlapDetector(double dist_thresh = 2.0, double vol_thresh = 1000.0)
        : distance_threshold_(dist_thresh), volume_threshold_(vol_thresh) {}

    /**
     * Check for 3D surface overlap between two pottery sherds
     * Uses surface point clouds instead of just breaklines
     */
    SurfaceOverlapResult checkSurfaceOverlap(
        const MatrixXd& surface1_points,  // Surface points from piece 1
        const MatrixXd& surface2_points,  // Surface points from piece 2
        bool use_normals = false);

    /**
     * Enhanced overlap check that combines breakline and surface analysis
     */
    bool enhancedOverlapCheck(
        const MatrixXd& surface1_inner, const MatrixXd& surface1_outer,
        const MatrixXd& surface2_inner, const MatrixXd& surface2_outer,
        double& total_overlap_volume);

    /**
     * Quick bounding box overlap check (fast pre-filter)
     */
    bool boundingBoxOverlap(const MatrixXd& points1, const MatrixXd& points2,
                           double margin = 1.0);

    // Setters for runtime parameter adjustment
    void setDistanceThreshold(double thresh) { distance_threshold_ = thresh; }
    void setVolumeThreshold(double thresh) { volume_threshold_ = thresh; }
};

/**
 * Pottery-appropriate surface overlap detection
 * Integrates with existing GeometryValidator workflow
 */
bool checkPotterySurfaceOverlap(
    const MatrixXd& surface1_inner, const MatrixXd& surface1_outer,
    const MatrixXd& surface2_inner, const MatrixXd& surface2_outer,
    double& overlap_volume, double threshold = 1000.0);