#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;

// TPS basis function: r^2 * log(r)
double tps_basis(double r) {
    if (r < 1e-10) return 0.0;
    return r * r * log(r);
}

// Derivatives of TPS basis function for normal computation
double tps_basis_dx(double dx, double dy, double r) {
    if (r < 1e-10) return 0.0;
    return dx * (2.0 * log(r) + 1.0);
}

double tps_basis_dy(double dx, double dy, double r) {
    if (r < 1e-10) return 0.0;
    return dy * (2.0 * log(r) + 1.0);
}

class TPSSurface {
private:
    vector<Vector3d> control_points_;
    VectorXd coefficients_;
    
public:
    void loadControlPoints(const string& surface_file) {
        ifstream file(surface_file);
        if (!file.is_open()) {
            cerr << "Error: Cannot open surface file: " << surface_file << endl;
            return;
        }
        
        control_points_.clear();
        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            double x, y, z;
            if (iss >> x >> y >> z) {
                control_points_.push_back(Vector3d(x, y, z));
            }
        }
        file.close();
        
        cout << "Loaded " << control_points_.size() << " control points" << endl;
        
        // Fit TPS surface to control points
        fitSurface();
    }
    
private:
    void fitSurface() {
        int n = control_points_.size();
        if (n < 3) {
            cerr << "Error: Need at least 3 control points" << endl;
            return;
        }
        
        // Build TPS system matrix
        MatrixXd A = MatrixXd::Zero(n + 3, n + 3);
        VectorXd b = VectorXd::Zero(n + 3);
        
        // Fill TPS matrix
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i != j) {
                    Vector2d diff = control_points_[i].head<2>() - control_points_[j].head<2>();
                    double r = diff.norm();
                    A(i, j) = tps_basis(r);
                }
            }
            
            // Polynomial terms
            A(i, n) = 1.0;
            A(i, n+1) = control_points_[i].x();
            A(i, n+2) = control_points_[i].y();
            
            A(n, i) = 1.0;
            A(n+1, i) = control_points_[i].x();
            A(n+2, i) = control_points_[i].y();
            
            // Right hand side (Z values)
            b(i) = control_points_[i].z();
        }
        
        // Solve for coefficients
        coefficients_ = A.colPivHouseholderQr().solve(b);
        
        cout << "TPS surface fitted with " << coefficients_.size() << " coefficients" << endl;
    }
    
public:
    // Evaluate TPS surface at point (x, y)
    double evaluateZ(double x, double y) const {
        if (coefficients_.size() == 0) return 0.0;
        
        int n = control_points_.size();
        double z = 0.0;
        
        // TPS basis contributions
        for (int i = 0; i < n; i++) {
            Vector2d diff(x - control_points_[i].x(), y - control_points_[i].y());
            double r = diff.norm();
            z += coefficients_(i) * tps_basis(r);
        }
        
        // Polynomial contributions
        z += coefficients_(n);          // constant
        z += coefficients_(n+1) * x;    // linear x
        z += coefficients_(n+2) * y;    // linear y
        
        return z;
    }
    
    // Compute surface normal at point (x, y) using TPS derivatives
    Vector3d computeNormal(double x, double y) const {
        if (coefficients_.size() == 0) return Vector3d(0, 0, 1);
        
        int n = control_points_.size();
        double dz_dx = 0.0;
        double dz_dy = 0.0;
        
        // Compute partial derivatives
        for (int i = 0; i < n; i++) {
            double dx = x - control_points_[i].x();
            double dy = y - control_points_[i].y();
            double r = sqrt(dx*dx + dy*dy);
            
            if (r > 1e-10) {
                dz_dx += coefficients_(i) * tps_basis_dx(dx, dy, r);
                dz_dy += coefficients_(i) * tps_basis_dy(dx, dy, r);
            }
        }
        
        // Add polynomial derivative contributions
        dz_dx += coefficients_(n+1);  // d/dx of linear x term
        dz_dy += coefficients_(n+2);  // d/dy of linear y term
        
        // Normal vector is (-dz/dx, -dz/dy, 1) normalized
        Vector3d normal(-dz_dx, -dz_dy, 1.0);
        return normal.normalized();
    }
    
    // Generate surface points with proper TPS normals
    void generateSurfaceWithNormals(const string& output_pcd, const string& output_xyz, 
                                   int grid_size_x = 100, int grid_size_y = 100) const {
        if (control_points_.empty()) {
            cerr << "No control points loaded" << endl;
            return;
        }
        
        // Find bounding box
        double min_x = control_points_[0].x(), max_x = control_points_[0].x();
        double min_y = control_points_[0].y(), max_y = control_points_[0].y();
        
        for (const auto& pt : control_points_) {
            min_x = min(min_x, pt.x());
            max_x = max(max_x, pt.x());
            min_y = min(min_y, pt.y());
            max_y = max(max_y, pt.y());
        }
        
        // Add some margin
        double margin_x = (max_x - min_x) * 0.1;
        double margin_y = (max_y - min_y) * 0.1;
        min_x -= margin_x; max_x += margin_x;
        min_y -= margin_y; max_y += margin_y;
        
        vector<Vector3d> surface_points;
        vector<Vector3d> surface_normals;
        
        // Generate grid of points
        for (int i = 0; i < grid_size_x; i++) {
            for (int j = 0; j < grid_size_y; j++) {
                double x = min_x + (max_x - min_x) * i / (grid_size_x - 1);
                double y = min_y + (max_y - min_y) * j / (grid_size_y - 1);
                
                double z = evaluateZ(x, y);
                Vector3d normal = computeNormal(x, y);
                
                surface_points.push_back(Vector3d(x, y, z));
                surface_normals.push_back(normal);
            }
        }
        
        cout << "Generated " << surface_points.size() << " surface points with TPS normals" << endl;
        
        // Save as XYZ file
        ofstream xyz_file(output_xyz);
        if (xyz_file.is_open()) {
            for (const auto& pt : surface_points) {
                xyz_file << pt.x() << " " << pt.y() << " " << pt.z() << "\n";
            }
            xyz_file.close();
        }
        
        // Save as PCD file with proper format
        ofstream pcd_file(output_pcd);
        if (pcd_file.is_open()) {
            pcd_file << "# .PCD v0.7 - Point Cloud Data file format\n";
            pcd_file << "VERSION 0.7\n";
            pcd_file << "FIELDS x y z normal_x normal_y normal_z curvature\n";
            pcd_file << "SIZE 4 4 4 4 4 4 4\n";
            pcd_file << "TYPE F F F F F F F\n";
            pcd_file << "COUNT 1 1 1 1 1 1 1\n";
            pcd_file << "WIDTH " << surface_points.size() << "\n";
            pcd_file << "HEIGHT 1\n";
            pcd_file << "VIEWPOINT 0 0 0 1 0 0 0\n";
            pcd_file << "POINTS " << surface_points.size() << "\n";
            pcd_file << "DATA ascii\n";
            
            for (size_t i = 0; i < surface_points.size(); i++) {
                const Vector3d& pt = surface_points[i];
                const Vector3d& n = surface_normals[i];
                double curvature = 0.0;  // Could compute from second derivatives
                
                pcd_file << pt.x() << " " << pt.y() << " " << pt.z() << " "
                        << n.x() << " " << n.y() << " " << n.z() << " " << curvature << "\n";
            }
            pcd_file.close();
        }
        
        cout << "Saved TPS surface with normals to: " << output_pcd << " and " << output_xyz << endl;
    }
};

int main(int argc, char** argv) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <input_surface.xyz> <output_surface_f.pcd> <output_surface_f.xyz>" << endl;
        cout << "Example: " << argv[0] << " Surface_0.xyz Surface_F.pcd Surface_F.xyz" << endl;
        return 1;
    }
    
    string input_surface = argv[1];
    string output_pcd = argv[2];
    string output_xyz = argv[3];
    
    cout << "=== Computing TPS Surface Normals ===" << endl;
    cout << "Input TPS surface: " << input_surface << endl;
    cout << "Output PCD: " << output_pcd << endl;
    cout << "Output XYZ: " << output_xyz << endl;
    cout << endl;
    
    TPSSurface tps;
    tps.loadControlPoints(input_surface);
    tps.generateSurfaceWithNormals(output_pcd, output_xyz, 50, 40);  // 2000 points
    
    cout << "✅ TPS normals computation complete!" << endl;
    return 0;
}