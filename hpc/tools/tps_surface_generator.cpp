#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <cmath>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace Eigen;

struct MeshData {
    vector<Vector3d> vertices;
    vector<Vector3i> faces;
    Vector3d center;
    double scale;
};

class TPSSurfaceGenerator {
private:
    vector<Vector3d> control_points_;
    vector<double> control_values_;
    VectorXd coefficients_;
    bool fitted_;
    
    // TPS basis function: r^2 * log(r)
    double tps_basis(double r) const {
        if (r < 1e-10) return 0.0;
        return r * r * log(r);
    }
    
    // Compute TPS basis derivatives for normal calculation
    Vector3d tps_gradient(const Vector3d& point, int control_idx) const {
        Vector3d diff = point - control_points_[control_idx];
        double r = diff.norm();
        if (r < 1e-10) return Vector3d::Zero();
        
        double factor = 2.0 * log(r) + 2.0;
        return factor * diff;
    }

public:
    TPSSurfaceGenerator() : fitted_(false) {}
    
    // Fit TPS surface to control points with associated Z values
    void fitSurface(const vector<Vector3d>& control_points, const vector<double>& z_values) {
        control_points_ = control_points;
        control_values_ = z_values;
        
        int n = control_points.size();
        if (n < 4) {
            cerr << "Error: Need at least 4 control points for TPS fitting" << endl;
            return;
        }
        
        // Build TPS system matrix
        MatrixXd A = MatrixXd::Zero(n + 3, n + 3);
        VectorXd b = VectorXd::Zero(n + 3);
        
        // Fill TPS kernel matrix
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i != j) {
                    double r = (control_points[i] - control_points[j]).norm();
                    A(i, j) = tps_basis(r);
                } else {
                    A(i, j) = 0.0; // Regularization can be added here
                }
            }
            
            // Polynomial terms (affine part)
            A(i, n) = 1.0;
            A(i, n + 1) = control_points[i].x();
            A(i, n + 2) = control_points[i].y();
            
            A(n, i) = 1.0;
            A(n + 1, i) = control_points[i].x();
            A(n + 2, i) = control_points[i].y();
            
            b(i) = z_values[i];
        }
        
        // Solve TPS system
        try {
            coefficients_ = A.colPivHouseholderQr().solve(b);
            fitted_ = true;
            cout << "TPS surface fitted with " << n << " control points" << endl;
        } catch (const exception& e) {
            cerr << "TPS fitting failed: " << e.what() << endl;
            fitted_ = false;
        }
    }
    
    // Evaluate TPS surface at given (x, y) point
    double evaluate(double x, double y) const {
        if (!fitted_) return 0.0;
        
        int n = control_points_.size();
        double result = 0.0;
        
        Vector3d query_point(x, y, 0);
        
        // TPS kernel contributions
        for (int i = 0; i < n; i++) {
            Vector3d control_2d(control_points_[i].x(), control_points_[i].y(), 0);
            double r = (query_point - control_2d).norm();
            result += coefficients_(i) * tps_basis(r);
        }
        
        // Polynomial (affine) terms
        result += coefficients_(n);              // constant
        result += coefficients_(n + 1) * x;      // linear x
        result += coefficients_(n + 2) * y;      // linear y
        
        return result;
    }
    
    // Compute surface normal at (x, y) using finite differences
    Vector3d computeNormal(double x, double y, double epsilon = 0.01) const {
        if (!fitted_) return Vector3d(0, 0, 1);
        
        double z_center = evaluate(x, y);
        double dz_dx = (evaluate(x + epsilon, y) - evaluate(x - epsilon, y)) / (2.0 * epsilon);
        double dz_dy = (evaluate(x, y + epsilon) - evaluate(x, y - epsilon)) / (2.0 * epsilon);
        
        Vector3d tangent_x(1.0, 0.0, dz_dx);
        Vector3d tangent_y(0.0, 1.0, dz_dy);
        
        Vector3d normal = tangent_x.cross(tangent_y);
        return normal.normalized();
    }
};

class MeshProcessor {
public:
    static MeshData loadOBJ(const string& filename) {
        MeshData mesh;
        ifstream file(filename);
        
        if (!file.is_open()) {
            cerr << "Error: Cannot open OBJ file: " << filename << endl;
            return mesh;
        }
        
        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            string prefix;
            iss >> prefix;
            
            if (prefix == "v") {
                // Vertex
                double x, y, z;
                iss >> x >> y >> z;
                mesh.vertices.push_back(Vector3d(x, y, z));
            } else if (prefix == "f") {
                // Face (assuming triangular faces)
                string v1_str, v2_str, v3_str;
                iss >> v1_str >> v2_str >> v3_str;
                
                // Parse vertex indices (handle both "v" and "v/vt/vn" formats)
                auto parseIndex = [](const string& s) -> int {
                    return stoi(s.substr(0, s.find('/'))) - 1; // OBJ is 1-indexed
                };
                
                int v1 = parseIndex(v1_str);
                int v2 = parseIndex(v2_str);
                int v3 = parseIndex(v3_str);
                
                mesh.faces.push_back(Vector3i(v1, v2, v3));
            }
        }
        
        file.close();
        
        // Compute mesh statistics
        if (!mesh.vertices.empty()) {
            mesh.center = Vector3d::Zero();
            for (const auto& v : mesh.vertices) {
                mesh.center += v;
            }
            mesh.center /= mesh.vertices.size();
            
            double max_dist = 0.0;
            for (const auto& v : mesh.vertices) {
                max_dist = max(max_dist, (v - mesh.center).norm());
            }
            mesh.scale = max_dist;
        }
        
        cout << "Loaded mesh: " << mesh.vertices.size() << " vertices, " 
             << mesh.faces.size() << " faces" << endl;
        return mesh;
    }
    
    // Generate control points from mesh using adaptive sampling
    static vector<Vector3d> generateControlPoints(const MeshData& mesh, int target_points = 500) {
        vector<Vector3d> control_points;
        
        if (mesh.vertices.empty()) return control_points;
        
        // Use systematic sampling across the mesh
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, mesh.vertices.size() - 1);
        
        // Start with corner/extremal points for better conditioning
        auto vertices_copy = mesh.vertices;
        
        // Sort by different criteria to get diverse points
        vector<Vector3d> extremal_points;
        
        // X extremes
        auto x_sorted = vertices_copy;
        sort(x_sorted.begin(), x_sorted.end(), [](const Vector3d& a, const Vector3d& b) {
            return a.x() < b.x();
        });
        extremal_points.push_back(x_sorted[0]);
        extremal_points.push_back(x_sorted.back());
        
        // Y extremes  
        auto y_sorted = vertices_copy;
        sort(y_sorted.begin(), y_sorted.end(), [](const Vector3d& a, const Vector3d& b) {
            return a.y() < b.y();
        });
        extremal_points.push_back(y_sorted[0]);
        extremal_points.push_back(y_sorted.back());
        
        // Z extremes
        auto z_sorted = vertices_copy;
        sort(z_sorted.begin(), z_sorted.end(), [](const Vector3d& a, const Vector3d& b) {
            return a.z() < b.z();
        });
        extremal_points.push_back(z_sorted[0]);
        extremal_points.push_back(z_sorted.back());
        
        // Add extremal points
        for (const auto& pt : extremal_points) {
            control_points.push_back(pt);
        }
        
        // Add random sampling for remaining points
        int remaining = target_points - control_points.size();
        for (int i = 0; i < remaining && i < (int)mesh.vertices.size(); i++) {
            int idx = dis(gen);
            control_points.push_back(mesh.vertices[idx]);
        }
        
        cout << "Generated " << control_points.size() << " control points" << endl;
        return control_points;
    }
};

class SurfaceExporter {
public:
    // Generate inner and outer surfaces using TPS with different parameters
    static void generateSurfaces(const string& obj_file, const string& output_prefix) {
        cout << "=== TPS Surface Generation for " << obj_file << " ===" << endl;
        
        // Load mesh
        MeshData mesh = MeshProcessor::loadOBJ(obj_file);
        if (mesh.vertices.empty()) {
            cerr << "Failed to load mesh data" << endl;
            return;
        }
        
        // Generate control points
        vector<Vector3d> control_points = MeshProcessor::generateControlPoints(mesh, 300);
        
        // Create Z-values for control points (use actual Z coordinates)
        vector<double> z_values;
        for (const auto& pt : control_points) {
            z_values.push_back(pt.z());
        }
        
        // Fit TPS surface
        TPSSurfaceGenerator tps;
        tps.fitSurface(control_points, z_values);
        
        // Generate surface point samples
        cout << "Generating surface samples..." << endl;
        
        // Find mesh bounds for sampling
        double min_x = mesh.vertices[0].x(), max_x = mesh.vertices[0].x();
        double min_y = mesh.vertices[0].y(), max_y = mesh.vertices[0].y();
        
        for (const auto& v : mesh.vertices) {
            min_x = min(min_x, v.x());
            max_x = max(max_x, v.x());
            min_y = min(min_y, v.y());
            max_y = max(max_y, v.y());
        }
        
        // Generate inner surface (Surface_0) - slightly offset inward
        generateSurfaceFile(tps, output_prefix + "_Surface_0.xyz", 
                          min_x, max_x, min_y, max_y, -2.0); // inward offset
        
        // Generate outer surface (Surface_1) - slightly offset outward  
        generateSurfaceFile(tps, output_prefix + "_Surface_1.xyz",
                          min_x, max_x, min_y, max_y, 2.0);  // outward offset
                          
        cout << "TPS surfaces generated successfully!" << endl;
    }

private:
    static void generateSurfaceFile(const TPSSurfaceGenerator& tps, 
                                  const string& output_file,
                                  double min_x, double max_x, 
                                  double min_y, double max_y,
                                  double z_offset) {
        ofstream file(output_file);
        if (!file.is_open()) {
            cerr << "Error: Cannot create output file: " << output_file << endl;
            return;
        }
        
        // Adaptive sampling density
        int samples_x = 80;  // Grid resolution
        int samples_y = 80;
        
        double dx = (max_x - min_x) / (samples_x - 1);
        double dy = (max_y - min_y) / (samples_y - 1);
        
        int point_count = 0;
        
        // Generate regular grid samples
        for (int i = 0; i < samples_x; i++) {
            for (int j = 0; j < samples_y; j++) {
                double x = min_x + i * dx;
                double y = min_y + j * dy;
                
                double z = tps.evaluate(x, y) + z_offset;
                Vector3d normal = tps.computeNormal(x, y);
                
                // Write point and normal
                file << fixed << setprecision(6) 
                     << x << " " << y << " " << z << " "
                     << normal.x() << " " << normal.y() << " " << normal.z() << "\n";
                point_count++;
            }
        }
        
        // Add some random samples for better coverage
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> x_dist(min_x, max_x);
        uniform_real_distribution<> y_dist(min_y, max_y);
        
        int random_samples = 2000; // Additional random points
        for (int i = 0; i < random_samples; i++) {
            double x = x_dist(gen);
            double y = y_dist(gen);
            
            double z = tps.evaluate(x, y) + z_offset;
            Vector3d normal = tps.computeNormal(x, y);
            
            file << fixed << setprecision(6)
                 << x << " " << y << " " << z << " "
                 << normal.x() << " " << normal.y() << " " << normal.z() << "\n";
            point_count++;
        }
        
        file.close();
        cout << "Generated " << output_file << " with " << point_count << " points" << endl;
    }
};

int main(int argc, char* argv[]) {
    cout << "=== TPS Surface Generator ===" << endl;
    cout << "Replacing NURBS surfaces with Thin Plate Spline surfaces" << endl;
    
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <piece_number>" << endl;
        cout << "Example: " << argv[0] << " 1" << endl;
        return 1;
    }
    
    string piece_num = argv[1];
    
    // Pad piece number to 2 digits
    if (piece_num.length() == 1) {
        piece_num = "0" + piece_num;
    }
    
    string mesh_file = "sfspreproc-docker/Dataset/SfS_pp/Mesh/Pot_A_Piece_" + piece_num + "_Mesh.obj";
    string output_prefix = "sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_" + piece_num;
    
    cout << "Processing: " << mesh_file << endl;
    cout << "Output prefix: " << output_prefix << endl;
    
    SurfaceExporter::generateSurfaces(mesh_file, output_prefix);
    
    return 0;
}