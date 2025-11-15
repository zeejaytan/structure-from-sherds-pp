#include "surface_overlap_detector.h"

SurfaceOverlapResult Enhanced3DOverlapDetector::checkSurfaceOverlap(
    const MatrixXd& surface1_points,  // Surface points from piece 1
    const MatrixXd& surface2_points,  // Surface points from piece 2
    bool use_normals) {

    SurfaceOverlapResult result;
    result.has_overlap = false;
    result.overlap_volume = 0.0;
    result.overlap_percentage = 0.0;
    result.overlap_point_count = 0;
    result.overlap_center = Vector3d::Zero();

    if (surface1_points.cols() == 0 || surface2_points.cols() == 0) {
        return result;
    }

    // Convert Eigen matrices to PCL point clouds for efficient spatial queries
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud1(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud2(new pcl::PointCloud<pcl::PointXYZ>);

    // Fill cloud1
    cloud1->width = surface1_points.cols();
    cloud1->height = 1;
    cloud1->points.resize(cloud1->width * cloud1->height);
    for (int i = 0; i < surface1_points.cols(); i++) {
        cloud1->points[i].x = surface1_points(0, i);
        cloud1->points[i].y = surface1_points(1, i);
        cloud1->points[i].z = surface1_points(2, i);
    }

    // Fill cloud2
    cloud2->width = surface2_points.cols();
    cloud2->height = 1;
    cloud2->points.resize(cloud2->width * cloud2->height);
    for (int i = 0; i < surface2_points.cols(); i++) {
        cloud2->points[i].x = surface2_points(0, i);
        cloud2->points[i].y = surface2_points(1, i);
        cloud2->points[i].z = surface2_points(2, i);
    }

    // Build KD-tree for efficient nearest neighbor search
    pcl::search::KdTree<pcl::PointXYZ> kdtree;
    kdtree.setInputCloud(cloud2);

    vector<int> overlap_indices1, overlap_indices2;
    Vector3d overlap_center_sum = Vector3d::Zero();

    // For each point in surface1, check if it's too close to surface2
    for (int i = 0; i < cloud1->points.size(); i++) {
        vector<int> point_indices;
        vector<float> point_distances;

        // Find nearest neighbors within distance threshold
        if (kdtree.radiusSearch(cloud1->points[i], distance_threshold_,
                              point_indices, point_distances) > 0) {
            overlap_indices1.push_back(i);

            // Accumulate overlap center
            overlap_center_sum += Vector3d(cloud1->points[i].x,
                                         cloud1->points[i].y,
                                         cloud1->points[i].z);
        }
    }

    // Calculate overlap statistics
    result.overlap_point_count = overlap_indices1.size();
    if (result.overlap_point_count > 0) {
        result.overlap_center = overlap_center_sum / result.overlap_point_count;
        result.overlap_percentage = (double)result.overlap_point_count / cloud1->points.size() * 100.0;

        // Estimate overlap volume using point density
        // Assume each point represents a small volume element
        double point_volume = pow(distance_threshold_, 3); // Rough estimate
        result.overlap_volume = result.overlap_point_count * point_volume;

        // Check if overlap exceeds threshold
        result.has_overlap = (result.overlap_volume > volume_threshold_);
    }

    return result;
}

bool Enhanced3DOverlapDetector::enhancedOverlapCheck(
    const MatrixXd& surface1_inner, const MatrixXd& surface1_outer,
    const MatrixXd& surface2_inner, const MatrixXd& surface2_outer,
    double& total_overlap_volume) {

    total_overlap_volume = 0.0;

    // Check all surface combinations
    vector<pair<MatrixXd, MatrixXd>> surface_pairs = {
        {surface1_inner, surface2_inner},
        {surface1_inner, surface2_outer},
        {surface1_outer, surface2_inner},
        {surface1_outer, surface2_outer}
    };

    bool any_overlap = false;

    for (auto& pair : surface_pairs) {
        SurfaceOverlapResult result = checkSurfaceOverlap(pair.first, pair.second);

        if (result.has_overlap) {
            any_overlap = true;
            total_overlap_volume += result.overlap_volume;

            cout << "  Surface overlap detected: " << result.overlap_point_count
                 << " points, volume: " << result.overlap_volume << endl;
        }
    }

    return any_overlap;
}

bool Enhanced3DOverlapDetector::boundingBoxOverlap(const MatrixXd& points1, const MatrixXd& points2,
                       double margin) {
    if (points1.cols() == 0 || points2.cols() == 0) return false;

    // Calculate bounding boxes
    Vector3d min1 = points1.rowwise().minCoeff();
    Vector3d max1 = points1.rowwise().maxCoeff();
    Vector3d min2 = points2.rowwise().minCoeff();
    Vector3d max2 = points2.rowwise().maxCoeff();

    // Expand by margin
    min1 = min1.array() - margin;
    max1 = max1.array() + margin;

    // Check for overlap in all 3 dimensions
    return (max1.x() >= min2.x() && max2.x() >= min1.x()) &&
           (max1.y() >= min2.y() && max2.y() >= min1.y()) &&
           (max1.z() >= min2.z() && max2.z() >= min1.z());
}

/**
 * Pottery-appropriate surface overlap detection
 * Integrates with existing GeometryValidator workflow
 */
bool checkPotterySurfaceOverlap(
    const MatrixXd& surface1_inner, const MatrixXd& surface1_outer,
    const MatrixXd& surface2_inner, const MatrixXd& surface2_outer,
    double& overlap_volume, double threshold) {

    Enhanced3DOverlapDetector detector(2.0, threshold);  // 2mm distance, user-defined volume

    bool has_overlap = detector.enhancedOverlapCheck(
        surface1_inner, surface1_outer,
        surface2_inner, surface2_outer,
        overlap_volume);

    if (has_overlap) {
        cout << "🔴 POTTERY SURFACE OVERLAP: Volume = " << overlap_volume
             << " mm³ (threshold = " << threshold << " mm³)" << endl;
    }

    return has_overlap;
}