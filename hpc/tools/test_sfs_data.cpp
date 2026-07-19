#include <iostream>
#include <fstream>
#include <vector>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <boost/filesystem.hpp>

int main() {
    std::cout << "=== Structure from Sherds++ Data Test ===" << std::endl;
    
    // Test if we can access the pottery dataset
    std::string dataset_path = "/Dataset/SfS_pp";
    
    std::cout << "Testing dataset access at: " << dataset_path << std::endl;
    
    if (!boost::filesystem::exists(dataset_path)) {
        std::cout << "ERROR: Dataset path not found: " << dataset_path << std::endl;
        return 1;
    }
    
    std::cout << "✓ Dataset directory found" << std::endl;
    
    // Test loading a breakline point cloud
    std::string breakline_file = dataset_path + "/Breaklines/Pot_A_Piece_01_Breakline_0.pcd";
    
    if (!boost::filesystem::exists(breakline_file)) {
        std::cout << "ERROR: Breakline file not found: " << breakline_file << std::endl;
        return 1;
    }
    
    std::cout << "✓ Breakline file found: " << breakline_file << std::endl;
    
    // Test PCL loading capability
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    
    if (pcl::io::loadPCDFile<pcl::PointXYZ>(breakline_file, *cloud) == -1) {
        std::cout << "ERROR: Could not load PCD file: " << breakline_file << std::endl;
        return 1;
    }
    
    std::cout << "✓ Successfully loaded breakline point cloud" << std::endl;
    std::cout << "  Points in cloud: " << cloud->size() << std::endl;
    
    if (cloud->size() > 0) {
        std::cout << "  First point: (" 
                  << cloud->points[0].x << ", " 
                  << cloud->points[0].y << ", " 
                  << cloud->points[0].z << ")" << std::endl;
    }
    
    // Test axis file access
    std::string axis_file = dataset_path + "/Axes/Pot_A_Piece_01_Axis.xyz";
    
    if (!boost::filesystem::exists(axis_file)) {
        std::cout << "WARNING: Axis file not found: " << axis_file << std::endl;
    } else {
        std::cout << "✓ Axis file found: " << axis_file << std::endl;
        
        // Read first line of axis file
        std::ifstream file(axis_file);
        if (file.is_open()) {
            std::string line;
            if (std::getline(file, line)) {
                std::cout << "  Axis data sample: " << line << std::endl;
            }
            file.close();
        }
    }
    
    std::cout << std::endl;
    std::cout << "=== PCL and SfS++ Data Access Test SUCCESSFUL ===" << std::endl;
    std::cout << "PCL is working correctly with pottery fragment data!" << std::endl;
    
    return 0;
}