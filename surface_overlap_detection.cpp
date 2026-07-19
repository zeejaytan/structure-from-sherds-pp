// Enhanced 3D Surface Overlap Detection for SFS Assembly
// Addresses the limitation where breakline-only overlap detection misses 
// major 3D volume intersections between pottery sherds

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
        bool use_normals = false) {
        
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
    
    /**
     * Enhanced overlap check that combines breakline and surface analysis
     */
    bool enhancedOverlapCheck(
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
    
    /**
     * Quick bounding box overlap check (fast pre-filter)
     */
    bool boundingBoxOverlap(const MatrixXd& points1, const MatrixXd& points2, 
                           double margin = 1.0) {
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
    
    // Setters for runtime parameter adjustment
    void setDistanceThreshold(double thresh) { distance_threshold_ = thresh; }
    void setVolumeThreshold(double thresh) { volume_threshold_ = thresh; }
};

/**
 * Replacement function for the existing OverlapCheck_3d that caused the issues
 * This version uses full 3D surface data instead of just breaklines
 */
bool ImprovedOverlapCheck_3d(
    const MatrixXd& surface1_inner, const MatrixXd& surface1_outer,
    const MatrixXd& surface2_inner, const MatrixXd& surface2_outer,
    double& area, double& size, double threshold) {
    
    Enhanced3DOverlapDetector detector(2.0, threshold);  // Use threshold from original
    
    double total_volume;
    bool has_overlap = detector.enhancedOverlapCheck(
        surface1_inner, surface1_outer,
        surface2_inner, surface2_outer, 
        total_volume);
    
    // Convert volume back to area-like metric for compatibility
    area = total_volume;
    size = total_volume / 100.0;  // Rough conversion
    
    if (has_overlap) {
        cout << "🔴 SURFACE OVERLAP DETECTED: Volume = " << total_volume 
             << " (threshold = " << threshold << ")" << endl;
    }
    
    return has_overlap;
}