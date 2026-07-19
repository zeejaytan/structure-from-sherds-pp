#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include "sfspreproc-docker/class/data_structure.h"
#include "sfspreproc-docker/class/axis_estimation.h"

using namespace std;
using namespace Eigen;

// Function to write axis to file in the required format
void WriteAxisToFile(const string& output_file, 
                     const Vector3d& axis_normal, 
                     const Vector3d& axis_point) {
    ofstream file(output_file);
    if (!file.is_open()) {
        cerr << "Error: Cannot create axis file: " << output_file << endl;
        return;
    }
    
    // Write in format: dx dy dz px py pz
    file << fixed << setprecision(6)
         << axis_normal.x() << " " << axis_normal.y() << " " << axis_normal.z() << " "
         << axis_point.x() << " " << axis_point.y() << " " << axis_point.z() << endl;
    
    file.close();
    cout << "Axis written to: " << output_file << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <pot_name> <piece_id>" << endl;
        cout << "Example: " << argv[0] << " Pot_A 01" << endl;
        return 1;
    }
    
    string pot_name = argv[1];
    string piece_id = argv[2];
    
    // Construct file paths
    string surface_0_path = string("sfspreproc-docker/Dataset/SfS_pp/Surfaces/") + 
                           pot_name + "_Piece_" + piece_id + "_Surface_0.xyz";
    string surface_1_path = string("sfspreproc-docker/Dataset/SfS_pp/Surfaces/") + 
                           pot_name + "_Piece_" + piece_id + "_Surface_1.xyz";
    string axis_output_path = string("sfspreproc-docker/Dataset/SfS_pp/Axes/") + 
                             pot_name + "_Piece_" + piece_id + "_Axis.xyz";
    
    cout << "=== PotSAC Axis Extraction ===" << endl;
    cout << "Processing: " << pot_name << " Piece " << piece_id << endl;
    cout << "Surface 0: " << surface_0_path << endl;
    cout << "Surface 1: " << surface_1_path << endl;
    cout << "Output: " << axis_output_path << endl;
    
    try {
        // Create geometry object
        Geom geometry;
        
        // Load surfaces
        cout << "Loading surfaces..." << endl;
        LoadSurface(&geometry, surface_1_path, surface_0_path);
        
        cout << "Inner surface points: " << geometry.sur_in_.point_.cols() << endl;
        cout << "Outer surface points: " << geometry.sur_out_.point_.cols() << endl;
        
        // Compute axis using PotSAC algorithm
        Vector3d axis_point, axis_normal;
        cout << "Computing axis using PotSAC algorithm..." << endl;
        
        ComputePotSACAxis(&geometry,
                         axis_point, 
                         axis_normal,
                         1000,     // num_iterations
                         4,        // num_threads  
                         1.0,      // inlier_threshold
                         true,     // use_both_surfaces
                         true);    // refine_axis
        
        cout << "Axis computed successfully!" << endl;
        cout << "Direction: [" << axis_normal.x() << ", " << axis_normal.y() << ", " << axis_normal.z() << "]" << endl;
        cout << "Position: [" << axis_point.x() << ", " << axis_point.y() << ", " << axis_point.z() << "]" << endl;
        
        // Write axis to file
        WriteAxisToFile(axis_output_path, axis_normal, axis_point);
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}