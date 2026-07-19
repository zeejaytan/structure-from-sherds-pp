#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

struct SurfacePoint {
    double x, y, z;
    double nx, ny, nz;
};

vector<SurfacePoint> loadSurfaceXYZ(const string& filename) {
    vector<SurfacePoint> points;
    ifstream file(filename);
    
    if (!file.is_open()) {
        throw runtime_error("Cannot open surface file: " + filename);
    }
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        SurfacePoint pt;
        iss >> pt.x >> pt.y >> pt.z >> pt.nx >> pt.ny >> pt.nz;
        points.push_back(pt);
    }
    
    cout << "Loaded " << points.size() << " points from " << filename << endl;
    return points;
}

void generatePCDSurfaceF(const vector<SurfacePoint>& surface_points, const string& output_pcd) {
    ofstream file(output_pcd);
    if (!file.is_open()) {
        throw runtime_error("Cannot create PCD file: " + output_pcd);
    }
    
    // Write PCD header
    file << "# .PCD v0.7 - Point Cloud Data file format\n";
    file << "VERSION 0.7\n";
    file << "FIELDS x y z normal_x normal_y normal_z curvature\n";
    file << "SIZE 4 4 4 4 4 4 4\n";
    file << "TYPE F F F F F F F\n";
    file << "COUNT 1 1 1 1 1 1 1\n";
    file << "WIDTH " << surface_points.size() << "\n";
    file << "HEIGHT 1\n";
    file << "VIEWPOINT 0 0 0 1 0 0 0\n";
    file << "POINTS " << surface_points.size() << "\n";
    file << "DATA ascii\n";
    
    // Write point data with curvature = 0 (same as original)
    for (const auto& pt : surface_points) {
        file << fixed << setprecision(6)
             << pt.x << " " << pt.y << " " << pt.z << " "
             << pt.nx << " " << pt.ny << " " << pt.nz << " 0\n";
    }
    
    file.close();
    cout << "Generated PCD Surface_F with " << surface_points.size() << " points: " << output_pcd << endl;
}

void generateXYZSurfaceF(const vector<SurfacePoint>& surface_points, const string& output_xyz) {
    ofstream file(output_xyz);
    if (!file.is_open()) {
        throw runtime_error("Cannot create XYZ file: " + output_xyz);
    }
    
    // Write XYZ data (same format as original NURBS Surface_F)
    for (const auto& pt : surface_points) {
        file << fixed << setprecision(6)
             << pt.x << " " << pt.y << " " << pt.z << " "
             << pt.nx << " " << pt.ny << " " << pt.nz << " \n";
    }
    
    file.close();
    cout << "Generated XYZ Surface_F with " << surface_points.size() << " points: " << output_xyz << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <input_surface_0.xyz> <output_surface_f.pcd> <output_surface_f.xyz>" << endl;
        cout << "This generates Surface_F files that match Surface_0 coordinates and normals" << endl;
        return 1;
    }
    
    string input_surface = argv[1];
    string output_pcd = argv[2]; 
    string output_xyz = argv[3];
    
    try {
        cout << "=== NURBS-Compatible Surface_F Generation ===" << endl;
        
        // Load Surface_0 data (which has correct NURBS-compatible TPS data)
        vector<SurfacePoint> surface_points = loadSurfaceXYZ(input_surface);
        
        // Generate Surface_F files with identical data but proper formats
        generatePCDSurfaceF(surface_points, output_pcd);
        generateXYZSurfaceF(surface_points, output_xyz);
        
        cout << "✅ Successfully generated NURBS-compatible Surface_F files" << endl;
        cout << "   - Same X,Y coordinates as original NURBS" << endl;
        cout << "   - TPS-fitted Z coordinates" << endl; 
        cout << "   - Preserved original normals" << endl;
        cout << "   - " << surface_points.size() << " points (matching Surface_0)" << endl;
        
    } catch (const exception& e) {
        cerr << "❌ Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}