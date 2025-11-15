#include "intersection_detector.h"
#include <pcl/features/normal_3d.h>
#include <pcl/surface/convex_hull.h>
#include <pcl/filters/random_sample.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/octree/octree_pointcloud.h>
#include <pcl/io/ply_io.h>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>

IntersectionDetector::IntersectionDetector(const Config& config)
    : config_(config), analysis_counter_(0) {

    // Create debug output directory if needed
    if (config_.save_debug_meshes) {
        mkdir(config_.debug_output_dir.c_str(), 0755);
    }

    if (config_.enable_debug_output) {
        cout << "IntersectionDetector initialized with configuration:" << endl;
        cout << "  Max volume overlap ratio: " << config_.max_volume_overlap_ratio << endl;
        cout << "  Critical volume overlap ratio: " << config_.critical_volume_overlap_ratio << endl;
        cout << "  Point inside tolerance: " << config_.point_inside_tolerance << "mm" << endl;
        cout << "  Surface proximity threshold: " << config_.surface_proximity_threshold << "mm" << endl;
        cout << "  Voxel size: " << config_.voxel_size << "mm" << endl;
    }
}

IntersectionDetector::~IntersectionDetector() {
    if (config_.enable_debug_output) {
        cout << "IntersectionDetector: Processed " << analysis_counter_ << " intersection analyses" << endl;
    }
}

IntersectionDetector::IntersectionResult IntersectionDetector::DetectIntersection(
    const Geom& piece1,
    const Geom& piece2,
    const Matrix4d& transformation,
    int piece1_id,
    int piece2_id) const {

    auto start_time = std::chrono::high_resolution_clock::now();

    IntersectionResult result;
    analysis_counter_++;

    if (config_.enable_debug_output) {
        cout << "\n=== INTERSECTION ANALYSIS ===" << endl;
        cout << "Analyzing pieces " << piece1_id << " and " << piece2_id << endl;
    }

    try {
        // Convert geometries to PCL point clouds with normals
        auto cloud1_with_normals = ConvertBreakLineToCloud(piece1.edge_line_);
        auto cloud2_with_normals = ConvertBreakLineToCloud(piece2.edge_line_);

        if (!cloud1_with_normals || !cloud2_with_normals) {
            result.rejection_reason = "Failed to convert geometries to point clouds";
            return result;
        }

        // Extract XYZ-only clouds for volumetric analysis
        auto cloud1 = pcl::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
        auto cloud2 = pcl::make_shared<pcl::PointCloud<pcl::PointXYZ>>();

        pcl::copyPointCloud(*cloud1_with_normals, *cloud1);
        pcl::copyPointCloud(*cloud2_with_normals, *cloud2);

        // Apply transformation to piece2
        auto transformed_cloud2 = ApplyTransformation(cloud2, transformation);
        auto transformed_cloud2_with_normals = pcl::make_shared<pcl::PointCloud<pcl::PointNormal>>();
        pcl::copyPointCloud(*cloud2_with_normals, *transformed_cloud2_with_normals);

        // Apply same transformation to normals cloud
        Eigen::Matrix4f transform_f = transformation.cast<float>();
        pcl::transformPointCloudWithNormals(*transformed_cloud2_with_normals, *transformed_cloud2_with_normals, transform_f);

        result.total_points_analyzed = cloud1->size();

        // Subsample if clouds are too large for performance
        if (cloud1->size() > config_.max_sample_points) {
            cloud1 = SubsampleCloud(cloud1, config_.sample_ratio);
            transformed_cloud2 = SubsampleCloud(transformed_cloud2, config_.sample_ratio);
        }

        if (config_.enable_debug_output) {
            cout << "Point cloud sizes: piece1=" << cloud1->size()
                 << ", piece2=" << transformed_cloud2->size() << endl;
        }

        // Perform multiple intersection analysis methods

        // 1. Volumetric intersection analysis using octree
        AnalyzeVolumetricIntersection(cloud1, transformed_cloud2, result);

        // 2. Point-in-mesh containment analysis
        AnalyzePointInMeshIntersection(cloud1, transformed_cloud2, result);

        // 3. Surface proximity and normal analysis
        AnalyzeSurfaceProximity(cloud1_with_normals, transformed_cloud2_with_normals, result);

        // 4. Calculate overall confidence score
        result.confidence_score = CalculateConfidenceScore(result);

        // 5. Make final intersection decision - FIXED: Add proximity-based acceptance
        // Accept connection if good proximity exists (legitimate fracture connection)
        bool has_good_proximity = (result.proximity_points_count >= 2) &&
                                 (result.avg_proximity_distance <= config_.surface_proximity_threshold);

        // Reject only if severe intersection AND no good proximity
        bool has_severe_intersection = (result.volume_overlap_ratio > config_.max_volume_overlap_ratio) ||
                                     (result.inside_points_ratio > config_.inside_points_ratio_threshold) ||
                                     (result.volume_overlap_ratio > config_.critical_volume_overlap_ratio);

        // FIXED LOGIC: Accept if good proximity exists, even with some overlap
        result.has_intersection = has_severe_intersection && !has_good_proximity;

        // Set rejection reason if intersection detected
        if (result.has_intersection) {
            if (result.volume_overlap_ratio > config_.critical_volume_overlap_ratio) {
                result.rejection_reason = "Critical volume overlap: " +
                    to_string(result.volume_overlap_ratio * 100) + "% (no compensating proximity)";
            } else if (result.volume_overlap_ratio > config_.max_volume_overlap_ratio) {
                result.rejection_reason = "Volume overlap exceeds threshold: " +
                    to_string(result.volume_overlap_ratio * 100) + "% (no compensating proximity)";
            } else if (result.inside_points_ratio > config_.inside_points_ratio_threshold) {
                result.rejection_reason = "Too many points inside other piece: " +
                    to_string(result.inside_points_ratio * 100) + "% (no compensating proximity)";
            }
        } else if (has_good_proximity) {
            result.rejection_reason = "ACCEPTED: Good proximity found (" +
                to_string(result.proximity_points_count) + " points, avg " +
                to_string(result.avg_proximity_distance) + "mm)";
        }

        // Save debug data if requested
        if (config_.save_debug_meshes) {
            SaveDebugData(result, cloud1, transformed_cloud2, piece1_id, piece2_id);
        }

    } catch (const std::exception& e) {
        if (config_.enable_debug_output) {
            cout << "Error in intersection analysis: " << e.what() << endl;
        }
        result.rejection_reason = "Analysis error: " + string(e.what());
        result.has_intersection = false; // Conservative: allow connection if analysis fails
    }

    // Record analysis time
    auto end_time = std::chrono::high_resolution_clock::now();
    result.analysis_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    if (config_.enable_debug_output) {
        LogAnalysisDetails(result, piece1_id, piece2_id);
    }

    return result;
}

bool IntersectionDetector::HasSignificantIntersection(const Geom& piece1,
                                                     const Geom& piece2,
                                                     const Matrix4d& transformation) const {
    auto result = DetectIntersection(piece1, piece2, transformation, 0, 0);
    return result.has_intersection;
}

void IntersectionDetector::AnalyzeVolumetricIntersection(
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2,
    IntersectionResult& result) const {

    if (config_.enable_debug_output) {
        cout << "Performing volumetric intersection analysis..." << endl;
    }

    // Create octree for spatial partitioning
    pcl::octree::OctreePointCloudSearch<pcl::PointXYZ> octree(config_.voxel_size);
    octree.setInputCloud(cloud1);
    octree.addPointsFromInputCloud();

    int intersection_points = 0;
    double total_intersection_volume = 0.0;
    vector<Vector3d> intersection_centers;

    // Check each point in cloud2 for proximity to cloud1
    for (const auto& point : cloud2->points) {
        vector<int> pointIndices;
        vector<float> pointDistances;

        // Find points in cloud1 within voxel distance
        if (octree.radiusSearch(point, config_.voxel_size, pointIndices, pointDistances) > 0) {
            intersection_points++;

            // Calculate local intersection volume (approximation)
            double local_volume = config_.voxel_size * config_.voxel_size * config_.voxel_size;
            total_intersection_volume += local_volume;

            // Record intersection center
            intersection_centers.push_back(Vector3d(point.x, point.y, point.z));
        }

        result.octree_nodes_traversed++;
    }

    // Calculate volume overlap ratio
    double cloud2_volume = CalculatePointCloudVolume(cloud2);
    if (cloud2_volume > 0) {
        result.volume_overlap_ratio = total_intersection_volume / cloud2_volume;
    }

    result.estimated_intersection_volume = total_intersection_volume;

    // Calculate intersection bounding box
    if (!intersection_centers.empty()) {
        Vector3d min_pt = intersection_centers[0];
        Vector3d max_pt = intersection_centers[0];

        for (const auto& pt : intersection_centers) {
            min_pt = min_pt.cwiseMin(pt);
            max_pt = max_pt.cwiseMax(pt);
        }

        result.intersection_center = (min_pt + max_pt) / 2.0;
        result.intersection_extents = max_pt - min_pt;

        // Store sample intersection points for visualization
        int sample_count = std::min(10, (int)intersection_centers.size());
        result.sample_intersection_points.clear();
        for (int i = 0; i < sample_count; i++) {
            int idx = i * intersection_centers.size() / sample_count;
            result.sample_intersection_points.push_back(intersection_centers[idx]);
        }
    }

    if (config_.enable_debug_output) {
        cout << "Volumetric analysis: " << intersection_points << " intersection points, "
             << "volume overlap ratio: " << (result.volume_overlap_ratio * 100) << "%" << endl;
    }
}

void IntersectionDetector::AnalyzePointInMeshIntersection(
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2,
    IntersectionResult& result) const {

    if (config_.enable_debug_output) {
        cout << "Performing point-in-mesh containment analysis..." << endl;
    }

    int points_inside = 0;

    // For each point in cloud1, check if it's inside the mesh represented by cloud2
    for (const auto& point : cloud1->points) {
        Vector3d pt(point.x, point.y, point.z);
        if (IsPointInsideMesh(pt, cloud2)) {
            points_inside++;
        }
    }

    result.points_inside_count = points_inside;
    result.inside_points_ratio = (double)points_inside / cloud1->size();

    if (config_.enable_debug_output) {
        cout << "Point-in-mesh analysis: " << points_inside << "/" << cloud1->size()
             << " points inside (" << (result.inside_points_ratio * 100) << "%)" << endl;
    }
}

void IntersectionDetector::AnalyzeSurfaceProximity(
    const pcl::PointCloud<pcl::PointNormal>::Ptr& cloud1,
    const pcl::PointCloud<pcl::PointNormal>::Ptr& cloud2,
    IntersectionResult& result) const {

    if (config_.enable_debug_output) {
        cout << "Performing surface proximity analysis..." << endl;
    }

    // Build KD-tree for cloud2
    pcl::KdTreeFLANN<pcl::PointNormal> kdtree;
    kdtree.setInputCloud(cloud2);

    int proximity_points = 0;
    int opposing_normals = 0;
    double total_distance = 0.0;
    double min_distance = std::numeric_limits<double>::max();
    double total_normal_alignment = 0.0;

    // For each point in cloud1, find closest point in cloud2
    for (const auto& point1 : cloud1->points) {
        vector<int> indices(1);
        vector<float> distances(1);

        if (kdtree.nearestKSearch(point1, 1, indices, distances) > 0) {
            double distance = sqrt(distances[0]);

            if (distance < config_.surface_proximity_threshold) {
                proximity_points++;
                total_distance += distance;
                min_distance = std::min(min_distance, distance);

                // Analyze normal alignment
                const auto& point2 = cloud2->points[indices[0]];
                Vector3d normal1(point1.normal_x, point1.normal_y, point1.normal_z);
                Vector3d normal2(point2.normal_x, point2.normal_y, point2.normal_z);

                double dot_product = normal1.dot(normal2);
                total_normal_alignment += dot_product;

                if (dot_product < config_.surface_normal_alignment_threshold) {
                    opposing_normals++;
                }
            }
        }
    }

    result.proximity_points_count = proximity_points;
    if (proximity_points > 0) {
        result.avg_proximity_distance = total_distance / proximity_points;
        result.avg_normal_alignment = total_normal_alignment / proximity_points;
        result.min_proximity_distance = min_distance;
    }
    result.opposing_normal_pairs = opposing_normals;

    if (config_.enable_debug_output) {
        cout << "Surface proximity: " << proximity_points << " close points, "
             << "avg distance: " << result.avg_proximity_distance << "mm, "
             << "opposing normals: " << opposing_normals << endl;
    }
}

double IntersectionDetector::CalculateConfidenceScore(const IntersectionResult& result) const {
    double confidence = 0.0;

    // Volume overlap contributes most to confidence
    confidence += result.volume_overlap_ratio * 0.4;

    // Point inside ratio contributes significantly
    confidence += result.inside_points_ratio * 0.3;

    // Surface proximity contributes moderately
    if (result.proximity_points_count > config_.min_proximity_points) {
        double proximity_factor = std::min(1.0, (double)result.proximity_points_count / config_.min_proximity_points);
        confidence += proximity_factor * 0.2;
    }

    // Opposing normals contribute to confidence
    if (result.total_points_analyzed > 0) {
        double opposing_ratio = (double)result.opposing_normal_pairs / result.total_points_analyzed;
        confidence += opposing_ratio * 0.1;
    }

    return std::min(1.0, confidence);
}

pcl::PointCloud<pcl::PointNormal>::Ptr IntersectionDetector::ConvertBreakLineToCloud(
    const BreakLine& breakline) const {

    auto cloud = pcl::make_shared<pcl::PointCloud<pcl::PointNormal>>();

    int num_points = breakline.point_.cols();
    cloud->points.resize(num_points);
    cloud->width = num_points;
    cloud->height = 1;
    cloud->is_dense = false;

    for (int i = 0; i < num_points; i++) {
        cloud->points[i].x = breakline.point_(0, i);
        cloud->points[i].y = breakline.point_(1, i);
        cloud->points[i].z = breakline.point_(2, i);

        // Add normals if available
        if (breakline.normal_.cols() > i) {
            cloud->points[i].normal_x = breakline.normal_(0, i);
            cloud->points[i].normal_y = breakline.normal_(1, i);
            cloud->points[i].normal_z = breakline.normal_(2, i);
        } else {
            // Default normal
            cloud->points[i].normal_x = 0.0;
            cloud->points[i].normal_y = 0.0;
            cloud->points[i].normal_z = 1.0;
        }
    }

    return cloud;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr IntersectionDetector::ApplyTransformation(
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud,
    const Matrix4d& transformation) const {

    auto transformed_cloud = pcl::make_shared<pcl::PointCloud<pcl::PointXYZ>>();

    Eigen::Matrix4f transform_f = transformation.cast<float>();
    pcl::transformPointCloud(*cloud, *transformed_cloud, transform_f);

    return transformed_cloud;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr IntersectionDetector::SubsampleCloud(
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud,
    double sample_ratio) const {

    auto filtered_cloud = pcl::make_shared<pcl::PointCloud<pcl::PointXYZ>>();

    pcl::RandomSample<pcl::PointXYZ> sampler;
    sampler.setInputCloud(cloud);
    sampler.setSample(static_cast<int>(cloud->size() * sample_ratio));
    sampler.filter(*filtered_cloud);

    return filtered_cloud;
}

double IntersectionDetector::CalculatePointCloudVolume(
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud) const {

    if (cloud->size() < 4) return 0.0;

    try {
        // Calculate convex hull volume as approximation
        pcl::ConvexHull<pcl::PointXYZ> hull;
        pcl::PointCloud<pcl::PointXYZ> hull_points;
        std::vector<pcl::Vertices> polygons;

        hull.setInputCloud(cloud);
        hull.reconstruct(hull_points, polygons);

        // Approximate volume calculation
        if (hull_points.size() >= 4) {
            // Use bounding box volume as rough approximation
            pcl::PointXYZ min_pt, max_pt;
            pcl::getMinMax3D(hull_points, min_pt, max_pt);

            double dx = max_pt.x - min_pt.x;
            double dy = max_pt.y - min_pt.y;
            double dz = max_pt.z - min_pt.z;

            return dx * dy * dz * 0.5; // Factor for non-rectangular shapes
        }
    } catch (...) {
        // Fallback to bounding box
    }

    // Fallback: bounding box volume
    pcl::PointXYZ min_pt, max_pt;
    pcl::getMinMax3D(*cloud, min_pt, max_pt);

    double dx = max_pt.x - min_pt.x;
    double dy = max_pt.y - min_pt.y;
    double dz = max_pt.z - min_pt.z;

    return dx * dy * dz;
}

bool IntersectionDetector::IsPointInsideMesh(const Vector3d& point,
                                            const pcl::PointCloud<pcl::PointXYZ>::Ptr& mesh_cloud) const {

    // Simplified ray casting approach
    // Cast rays in multiple directions and count intersections

    pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;
    kdtree.setInputCloud(mesh_cloud);

    pcl::PointXYZ query_point(point.x(), point.y(), point.z());

    // Find nearby points
    vector<int> indices;
    vector<float> distances;

    if (kdtree.radiusSearch(query_point, config_.point_inside_tolerance, indices, distances) > 0) {
        // If we have points very close to the query point, consider it inside
        return distances[0] < config_.point_inside_tolerance;
    }

    return false;
}

double IntersectionDetector::CalculateBoundingBoxIntersection(
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2) const {

    // Get bounding boxes
    pcl::PointXYZ min1, max1, min2, max2;
    pcl::getMinMax3D(*cloud1, min1, max1);
    pcl::getMinMax3D(*cloud2, min2, max2);

    // Calculate intersection
    double x_overlap = std::max(0.0, std::min((double)max1.x, (double)max2.x) - std::max((double)min1.x, (double)min2.x));
    double y_overlap = std::max(0.0, std::min((double)max1.y, (double)max2.y) - std::max((double)min1.y, (double)min2.y));
    double z_overlap = std::max(0.0, std::min((double)max1.z, (double)max2.z) - std::max((double)min1.z, (double)min2.z));

    return x_overlap * y_overlap * z_overlap;
}

void IntersectionDetector::SaveDebugData(const IntersectionResult& result,
                                        const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud1,
                                        const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud2,
                                        int piece1_id, int piece2_id) const {

    try {
        string filename_base = config_.debug_output_dir + "intersection_" +
                               to_string(piece1_id) + "_" + to_string(piece2_id) + "_";

        // Save piece clouds
        pcl::io::savePLYFile(filename_base + "piece1.ply", *cloud1);
        pcl::io::savePLYFile(filename_base + "piece2.ply", *cloud2);

        // Save intersection points if available
        if (!result.sample_intersection_points.empty()) {
            pcl::PointCloud<pcl::PointXYZ> intersection_cloud;
            for (const auto& pt : result.sample_intersection_points) {
                pcl::PointXYZ pcl_pt(pt.x(), pt.y(), pt.z());
                intersection_cloud.points.push_back(pcl_pt);
            }
            intersection_cloud.width = intersection_cloud.points.size();
            intersection_cloud.height = 1;
            intersection_cloud.is_dense = false;

            pcl::io::savePLYFile(filename_base + "intersections.ply", intersection_cloud);
        }

    } catch (const std::exception& e) {
        if (config_.enable_debug_output) {
            cout << "Failed to save debug data: " << e.what() << endl;
        }
    }
}

void IntersectionDetector::LogAnalysisDetails(const IntersectionResult& result,
                                             int piece1_id, int piece2_id) const {
    cout << "*** INTERSECTION ANALYSIS RESULT ***" << endl;
    cout << "Pieces " << piece1_id << "-" << piece2_id << ": ";

    if (result.has_intersection) {
        cout << "INTERSECTION DETECTED" << endl;
        cout << "  Reason: " << result.rejection_reason << endl;
    } else {
        cout << "NO INTERSECTION" << endl;
    }

    cout << "  Volume overlap: " << (result.volume_overlap_ratio * 100) << "%" << endl;
    cout << "  Points inside: " << result.points_inside_count << "/"
         << result.total_points_analyzed << " (" << (result.inside_points_ratio * 100) << "%)" << endl;
    cout << "  Proximity points: " << result.proximity_points_count << endl;
    cout << "  Confidence: " << (result.confidence_score * 100) << "%" << endl;
    cout << "  Analysis time: " << result.analysis_time_ms << "ms" << endl;
}