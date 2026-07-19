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
    
    void loadFromOBJ(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Cannot open file: " + filename);
        }
        
        vertices.clear();
        faces.clear();
        
        string line;
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            istringstream iss(line);
            string prefix;
            iss >> prefix;
            
            if (prefix == "v") {
                double x, y, z;
                iss >> x >> y >> z;
                vertices.push_back(Vector3d(x, y, z));
            }
            else if (prefix == "f") {
                string v1, v2, v3;
                iss >> v1 >> v2 >> v3;
                int i1 = stoi(v1.substr(0, v1.find('/'))) - 1;
                int i2 = stoi(v2.substr(0, v2.find('/'))) - 1;
                int i3 = stoi(v3.substr(0, v3.find('/'))) - 1;
                faces.push_back(Vector3i(i1, i2, i3));
            }
        }
        
        // Compute center and scale
        Vector3d min_pt = vertices[0];
        Vector3d max_pt = vertices[0];
        for (const auto& v : vertices) {
            min_pt = min_pt.cwiseMin(v);
            max_pt = max_pt.cwiseMax(v);
        }
        center = (min_pt + max_pt) / 2.0;
        scale = (max_pt - min_pt).norm();
    }
};

struct OriginalSurfaceData {
    vector<Vector3d> points;
    vector<Vector3d> normals;
    
    void loadFromXYZ(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Cannot open original surface file: " + filename);
        }
        
        points.clear();
        normals.clear();
        
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            istringstream iss(line);
            double x, y, z, nx, ny, nz;
            iss >> x >> y >> z >> nx >> ny >> nz;
            points.push_back(Vector3d(x, y, z));
            normals.push_back(Vector3d(nx, ny, nz).normalized());
        }
        
        cout << "Loaded " << points.size() << " points from original NURBS surface" << endl;
    }
};

class NURBSCompatibleTPSFitter {
private:
    vector<Vector3d> control_points_;
    VectorXd coefficients_;
    double regularization_;
    Vector3d mesh_center_;
    double mesh_scale_;
    
    double tps_basis(double r) const {
        if (r < 1e-10) return 0.0;
        return r * r * log(r);
    }
    
    double tps_basis_dx(double dx, double dy, double r) const {
        if (r < 1e-10) return 0.0;
        return dx * (2.0 * log(r) + 1.0);
    }
    
    double tps_basis_dy(double dx, double dy, double r) const {
        if (r < 1e-10) return 0.0;
        return dy * (2.0 * log(r) + 1.0);
    }

public:
    NURBSCompatibleTPSFitter(double regularization = 1e-6) : regularization_(regularization) {}
    
    void fit(const vector<Vector3d>& mesh_vertices, const Vector3d& center, double scale) {
        mesh_center_ = center;
        mesh_scale_ = scale;
        
        // Use mesh vertices as control points but subsample for efficiency
        control_points_.clear();
        int step = max(1, (int)(mesh_vertices.size() / 500));  // Limit to ~500 control points
        
        for (size_t i = 0; i < mesh_vertices.size(); i += step) {
            control_points_.push_back(mesh_vertices[i]);
        }
        
        int n = control_points_.size();
        cout << "Using " << n << " control points for TPS fitting" << endl;
        
        // Create system matrix A
        MatrixXd A(n + 3, n + 3);
        A.setZero();
        
        // Fill K matrix (TPS kernel)
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i == j) {
                    A(i, j) = regularization_;
                } else {
                    double r = (control_points_[i] - control_points_[j]).norm();
                    A(i, j) = tps_basis(r);
                }
            }
        }
        
        // Fill P matrix (polynomial part)
        for (int i = 0; i < n; i++) {
            A(i, n) = A(n, i) = 1.0;
            A(i, n + 1) = A(n + 1, i) = control_points_[i].x();
            A(i, n + 2) = A(n + 2, i) = control_points_[i].y();
        }
        
        // Create RHS vector
        VectorXd b(n + 3);
        for (int i = 0; i < n; i++) {
            b(i) = control_points_[i].z();
        }
        b(n) = b(n + 1) = b(n + 2) = 0.0;
        
        // Solve system
        coefficients_ = A.ldlt().solve(b);
        cout << "TPS fitting completed" << endl;
    }
    
    double evaluate(double x, double y) const {
        double result = coefficients_(control_points_.size()) + 
                       coefficients_(control_points_.size() + 1) * x + 
                       coefficients_(control_points_.size() + 2) * y;
        
        for (size_t i = 0; i < control_points_.size(); i++) {
            double r = sqrt(pow(x - control_points_[i].x(), 2) + pow(y - control_points_[i].y(), 2));
            result += coefficients_(i) * tps_basis(r);
        }
        
        return result;
    }
    
    Vector3d computeNormal(double x, double y) const {
        int n = control_points_.size();
        double dz_dx = coefficients_(n + 1);
        double dz_dy = coefficients_(n + 2);
        
        for (int i = 0; i < n; i++) {
            double dx = x - control_points_[i].x();
            double dy = y - control_points_[i].y();
            double r = sqrt(dx*dx + dy*dy);
            
            if (r > 1e-10) {
                dz_dx += coefficients_(i) * tps_basis_dx(dx, dy, r);
                dz_dy += coefficients_(i) * tps_basis_dy(dx, dy, r);
            }
        }
        
        Vector3d normal(-dz_dx, -dz_dy, 1.0);
        return normal.normalized();
    }
    
    void generateNURBSCompatibleSurface(const OriginalSurfaceData& original_data, 
                                       const string& output_file, int surface_id) {
        ofstream outfile(output_file);
        if (!outfile.is_open()) {
            throw runtime_error("Cannot create output file: " + output_file);
        }
        
        cout << "Generating NURBS-compatible TPS surface with " << original_data.points.size() << " points" << endl;
        
        // Use the original NURBS point distribution with TPS Z but PRESERVE ORIGINAL NORMALS
        for (size_t i = 0; i < original_data.points.size(); i++) {
            const Vector3d& orig_point = original_data.points[i];
            const Vector3d& orig_normal = original_data.normals[i];
            
            // Evaluate TPS at the original X,Y coordinates
            double tps_z = evaluate(orig_point.x(), orig_point.y());
            
            // CRITICAL FIX: Use original NURBS normal instead of TPS-computed normal
            // This preserves the surface geometry characteristics that feature matching expects
            outfile << fixed << setprecision(6) 
                    << orig_point.x() << " " << orig_point.y() << " " << tps_z << " "
                    << orig_normal.x() << " " << orig_normal.y() << " " << orig_normal.z() << endl;
        }
        
        outfile.close();
        cout << "Generated surface file with ORIGINAL NORMALS: " << output_file << endl;
    }
};

void processFragmentNURBSCompatible(int fragment_id) {
    cout << "\n=== Processing Fragment " << fragment_id << " with NURBS Compatibility ===" << endl;
    
    // Load mesh
    string mesh_file = string("sfspreproc-docker/Dataset/SfS_pp/Mesh/Pot_A_Piece_") + 
                      (fragment_id < 10 ? "0" : "") + to_string(fragment_id) + "_Mesh.obj";
    
    MeshData mesh;
    mesh.loadFromOBJ(mesh_file);
    cout << "Loaded mesh with " << mesh.vertices.size() << " vertices" << endl;
    
    // Load original NURBS surfaces for reference
    string orig_surface0 = string("sfspreproc-docker/Dataset/SfS_pp/Surfaces/Original_Sample_Surfaces/Pot_A_Piece_") + 
                          (fragment_id < 10 ? "0" : "") + to_string(fragment_id) + "_Surface_0.xyz";
    string orig_surface1 = string("sfspreproc-docker/Dataset/SfS_pp/Surfaces/Original_Sample_Surfaces/Pot_A_Piece_") + 
                          (fragment_id < 10 ? "0" : "") + to_string(fragment_id) + "_Surface_1.xyz";
    
    OriginalSurfaceData original_surf0, original_surf1;
    original_surf0.loadFromXYZ(orig_surface0);
    original_surf1.loadFromXYZ(orig_surface1);
    
    // Fit TPS to mesh
    NURBSCompatibleTPSFitter tps_fitter(1e-6);
    tps_fitter.fit(mesh.vertices, mesh.center, mesh.scale);
    
    // Generate surfaces with NURBS-compatible sampling
    string output_file0 = string("sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_") + 
                         (fragment_id < 10 ? "0" : "") + to_string(fragment_id) + "_Surface_0.xyz";
    string output_file1 = string("sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_") + 
                         (fragment_id < 10 ? "0" : "") + to_string(fragment_id) + "_Surface_1.xyz";
    
    tps_fitter.generateNURBSCompatibleSurface(original_surf0, output_file0, 0);
    tps_fitter.generateNURBSCompatibleSurface(original_surf1, output_file1, 1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <fragment_id>" << endl;
        return 1;
    }
    
    int fragment_id = stoi(argv[1]);
    
    try {
        processFragmentNURBSCompatible(fragment_id);
        cout << "\n✅ Successfully generated NURBS-compatible TPS surfaces for fragment " << fragment_id << endl;
    }
    catch (const exception& e) {
        cerr << "❌ Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}