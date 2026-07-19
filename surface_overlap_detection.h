#ifndef SURFACE_OVERLAP_DETECTION_H
#define SURFACE_OVERLAP_DETECTION_H

#include <Eigen/Dense>
#include <vector>

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
    Enhanced3DOverlapDetector(double dist_thresh = 2.0, double vol_thresh = 1000.0);
    
    SurfaceOverlapResult checkSurfaceOverlap(
        const MatrixXd& surface1_points,
        const MatrixXd& surface2_points,
        bool use_normals = false);
    
    bool enhancedOverlapCheck(
        const MatrixXd& surface1_inner, const MatrixXd& surface1_outer,
        const MatrixXd& surface2_inner, const MatrixXd& surface2_outer,
        double& total_overlap_volume);
    
    bool boundingBoxOverlap(const MatrixXd& points1, const MatrixXd& points2, 
                           double margin = 1.0);
    
    void setDistanceThreshold(double thresh);
    void setVolumeThreshold(double thresh);
};

// Replacement for existing OverlapCheck_3d function
bool ImprovedOverlapCheck_3d(
    const MatrixXd& surface1_inner, const MatrixXd& surface1_outer,
    const MatrixXd& surface2_inner, const MatrixXd& surface2_outer,
    double& area, double& size, double threshold);

#endif // SURFACE_OVERLAP_DETECTION_H