#pragma once
#ifndef _INTERSECTION_DETECTOR_H_
#define _INTERSECTION_DETECTOR_H_

#include <vector>
#include <memory>
#include <unordered_map>
#include <Eigen/Dense>
#include <pcl/common/common_headers.h>
#include <pcl/octree/octree_search.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/common/transforms.h>
#include "data_structure.h"

using namespace std;
using namespace Eigen;

/**
 * @brief Comprehensive 3D intersection detection system for post-registration validation
 *
 * This system performs detailed volumetric intersection analysis after ICP registration
 * to identify when two pieces would occupy the same 3D space, indicating false matches.
 * Uses multiple complementary methods for robust detection.
 */
class IntersectionDetector {
public:
    /**
     * @brief Configuration parameters for intersection detection
     */
    struct Config {
        // Volumetric intersection thresholds
        double max_volume_overlap_ratio;          // Max 15% volume overlap allowed
        double critical_volume_overlap_ratio;     // >25% = definitely reject

        // Point-in-mesh detection parameters
        double point_inside_tolerance;             // Distance tolerance for "inside" detection
        int min_inside_points_threshold;            // Min points to consider significant overlap
        double inside_points_ratio_threshold;      // >10% points inside = potential issue

        // Surface proximity analysis
        double surface_proximity_threshold;        // Points closer than 2mm = potential intersection
        double surface_normal_alignment_threshold; // Opposing normals threshold
        int min_proximity_points;                   // Min points in proximity for validation

        // Spatial hashing parameters
        double voxel_size;                         // 1mm voxel resolution
        int octree_resolution;                     // Octree depth resolution

        // Performance optimization
        int max_sample_points;                   // Subsample large meshes for performance
        double sample_ratio;                      // Use 30% of points for detailed analysis

        // Debug and validation
        bool enable_debug_output;               // Enable detailed logging
        bool save_debug_meshes;                 // Save intersection regions as PLY
        string debug_output_dir; // Debug output directory

        // Default constructor with initialization
        Config() :
            max_volume_overlap_ratio(0.15),
            critical_volume_overlap_ratio(0.25),
            point_inside_tolerance(0.5),
            min_inside_points_threshold(50),
            inside_points_ratio_threshold(0.1),
            surface_proximity_threshold(2.0),
            surface_normal_alignment_threshold(-0.7),
            min_proximity_points(30),
            voxel_size(1.0),
            octree_resolution(128),
            max_sample_points(10000),
            sample_ratio(0.3),
            enable_debug_output(false),
            save_debug_meshes(false),
            debug_output_dir("intersection_debug/") {}
    };

    /**
     * @brief Detailed intersection analysis result
     */
    struct IntersectionResult {
        // Overall assessment
        bool has_intersection = false;               // Primary result: intersection detected?
        double confidence_score = 0.0;              // Confidence in detection (0-1)

        // Volumetric analysis results
        double volume_overlap_ratio = 0.0;          // Fraction of piece1 volume overlapping piece2
        double estimated_intersection_volume = 0.0; // Absolute intersection volume (mm³)

        // Point-based analysis results
        int points_inside_count = 0;                // Number of piece1 points inside piece2
        int total_points_analyzed = 0;              // Total points used in analysis
        double inside_points_ratio = 0.0;          // Ratio of points inside

        // Surface proximity analysis
        int proximity_points_count = 0;            // Points in close proximity
        double avg_proximity_distance = 0.0;       // Average distance of proximity points
        double min_proximity_distance = 0.0;       // Minimum proximity distance found

        // Surface normal analysis
        int opposing_normal_pairs = 0;             // Count of opposing normal pairs
        double avg_normal_alignment = 0.0;         // Average normal dot product

        // Spatial characteristics
        Vector3d intersection_center = {0, 0, 0};  // Center of intersection region
        Vector3d intersection_extents = {0, 0, 0}; // Bounding box of intersection

        // Performance metrics
        double analysis_time_ms = 0.0;             // Time taken for analysis
        int octree_nodes_traversed = 0;            // Octree complexity metric

        // Diagnostic information
        string rejection_reason = "";              // Detailed reason for rejection
        vector<Vector3d> sample_intersection_points; // Sample intersection points for visualization

        /**
         * @brief Generate human-readable summary of intersection analysis
         */
        string GetSummary() const {
            stringstream ss;
            ss << "Intersection Analysis Result:\n";
            ss << "  Status: " << (has_intersection ? "INTERSECTION DETECTED" : "NO INTERSECTION") << "\n";
            ss << "  Confidence: " << (confidence_score * 100) << "%\n";
            ss << "  Volume overlap: " << (volume_overlap_ratio * 100) << "%\n";
            ss << "  Points inside: " << points_inside_count << "/" << total_points_analyzed
               << " (" << (inside_points_ratio * 100) << "%)\n";
            ss << "  Proximity points: " << proximity_points_count << "\n";
            ss << "  Analysis time: " << analysis_time_ms << "ms\n";
            if (!rejection_reason.empty()) {
                ss << "  Reason: " << rejection_reason << "\n";
            }
            return ss.str();
        }
    };

public:
    IntersectionDetector(const Config& config = Config());
    ~IntersectionDetector();

    /**
     * @brief Main intersection detection method - analyzes two pieces after registration
     *
     * @param piece1 First piece geometry (reference piece, remains fixed)
     * @param piece2 Second piece geometry (moving piece, will be transformed)
     * @param transformation Computed transformation matrix from ICP registration
     * @param piece1_id Piece 1 identifier for logging
     * @param piece2_id Piece 2 identifier for logging
     * @return Detailed intersection analysis result
     */
    IntersectionResult DetectIntersection(const Geom& piece1,
                                         const Geom& piece2,
                                         const Matrix4d& transformation,
                                         int piece1_id,
                                         int piece2_id) const;

    /**
     * @brief Simplified interface for quick intersection check
     *
     * @param piece1 First piece geometry
     * @param piece2 Second piece geometry
     * @param transformation Transformation matrix
     * @return True if intersection detected above threshold
     */
    bool HasSignificantIntersection(const Geom& piece1,
                                   const Geom& piece2,
                                   const Matrix4d& transformation) const;

private:
    Config config_;
    mutable int analysis_counter_;  // For debug file naming

    // Core analysis methods

    /**
     * @brief Perform volumetric intersection analysis using octree spatial partitioning
     */
    void AnalyzeVolumetricIntersection(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
                                      const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2,
                                      IntersectionResult& result) const;

    /**
     * @brief Analyze point-in-mesh containment using ray casting and spatial queries
     */
    void AnalyzePointInMeshIntersection(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
                                       const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2,
                                       IntersectionResult& result) const;

    /**
     * @brief Analyze surface proximity and normal orientation
     */
    void AnalyzeSurfaceProximity(const pcl::PointCloud<pcl::PointNormal>::Ptr& cloud1,
                                const pcl::PointCloud<pcl::PointNormal>::Ptr& cloud2,
                                IntersectionResult& result) const;

    /**
     * @brief Calculate intersection confidence score based on multiple factors
     */
    double CalculateConfidenceScore(const IntersectionResult& result) const;

    // Utility methods

    /**
     * @brief Convert Geom breakline to PCL point cloud with normals
     */
    pcl::PointCloud<pcl::PointNormal>::Ptr ConvertBreakLineToCloud(const BreakLine& breakline) const;

    /**
     * @brief Apply transformation matrix to point cloud
     */
    pcl::PointCloud<pcl::PointXYZ>::Ptr ApplyTransformation(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud,
                                                           const Matrix4d& transformation) const;

    /**
     * @brief Subsample point cloud for performance while preserving spatial distribution
     */
    pcl::PointCloud<pcl::PointXYZ>::Ptr SubsampleCloud(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud,
                                                       double sample_ratio) const;

    /**
     * @brief Calculate approximate volume of point cloud using convex hull
     */
    double CalculatePointCloudVolume(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud) const;

    /**
     * @brief Determine if a point is inside a mesh using ray casting
     */
    bool IsPointInsideMesh(const Vector3d& point,
                          const pcl::PointCloud<pcl::PointXYZ>::Ptr& mesh_cloud) const;

    /**
     * @brief Calculate bounding box intersection volume
     */
    double CalculateBoundingBoxIntersection(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
                                           const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2) const;

    // Debug and validation methods

    /**
     * @brief Save intersection analysis debug data
     */
    void SaveDebugData(const IntersectionResult& result,
                      const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
                      const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2,
                      int piece1_id, int piece2_id) const;

    /**
     * @brief Log detailed analysis information
     */
    void LogAnalysisDetails(const IntersectionResult& result,
                           int piece1_id, int piece2_id) const;
};

#endif // _INTERSECTION_DETECTOR_H_