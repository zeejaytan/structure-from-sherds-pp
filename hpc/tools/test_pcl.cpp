#include <iostream>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

int main() {
    // Create a simple point cloud to test PCL functionality
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    
    // Add some test points
    cloud->push_back(pcl::PointXYZ(1.0, 2.0, 3.0));
    cloud->push_back(pcl::PointXYZ(4.0, 5.0, 6.0));
    
    std::cout << "PCL test successful! Point cloud size: " << cloud->size() << std::endl;
    return 0;
}