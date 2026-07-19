#include <iostream>
#include "glog/logging.h"
#include <time.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <Eigen/Dense>
#include <Eigen/Core>
#include <boost/thread/thread.hpp>
#include <pcl/common/common_headers.h>
#include <pcl/features/normal_3d.h> 
#include <pcl/io/pcd_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/io/vtk_lib_io.h>
#include <pcl/io/impl/vtk_lib_io.hpp>
// Remove PCL visualization includes
#include <pcl/console/parse.h>
#include <pcl/common/transforms.h>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include "ceres/ceres.h"
#include "class/data_path.h"
#include "class/data_structure.h"
// Remove visualize.h include
#include "class/reconstruction.h"
#include "class/feature_matching.h"			
#include "class/ranking_system.h"

#define TOP_k 5
#define BRANCH_b 3

//#define NO_RIM_INFO
#define NO_BASE_INFO

using namespace std;
using namespace Eigen;

vector<Geom> shard(SHARD_NUMBER);
vector<Trans> GT_trans(SHARD_NUMBER);
MatrixXd GT_graph(SHARD_NUMBER, SHARD_NUMBER);

// Remove PCL visualizer - not needed for batch mode

int main(int argc, char** argv)
{
	cout << "#################### SfS Batch Mode (No Visualization) ####################" << endl;
	double calculation_time(0);
	int s_time(0), e_time(0);

	int step_size = shard.size(); 
	if (argv[1] != NULL) {
		step_size = std::atoi(argv[1]);
	}
	
	cout << "#################### Pottery Data load ####################" << endl;
	//#################### Pottery Data load ####################//
	s_time = clock();
	for (int i = 0; i < SHARD_NUMBER; i++) {
		cout << "Attempting to read file: " << axis_path[i] << endl;
		ifstream in(axis_path[i]);
		if (!in.is_open()) {
			cout << "Error: Could not open file " << axis_path[i] << endl;
			continue;
		} else {
			cout << "File opened successfully" << endl;
		}
		
		double x, y, z, nx, ny, nz;
		int point_count = 0;
		while (in >> x >> y >> z >> nx >> ny >> nz) {
			point_count++;
		}
		cout << "Successfully read " << point_count << " points" << endl;
		in.close();
		
		// Load axis data properly
		in.open(axis_path[i]);
		in >> x >> y >> z >> nx >> ny >> nz;
		in.close();
		shard[i].axis_.center_ = { x, y, z };
		shard[i].axis_.normal_ = { nx, ny, nz };

		// Load breaklines
		cout << "Attempting to read PCD file: " << file_path[i] << endl;
		if (pcl::io::loadPCDFile<pcl::PointXYZRGBNormal>(file_path[i], shard[i].edge_line_.point_cloud_) == -1) {
			cout << "Error: Could not read PCD file " << file_path[i] << endl;
			continue;
		} else {
			cout << "File opened successfully" << endl;
		}
		
		// Convert to internal format
		int total_points = shard[i].edge_line_.point_cloud_.points.size();
		cout << "Found " << total_points << " total points" << endl;
		cout << "Processing " << total_points << " points" << endl;
		
		shard[i].edge_line_.point_.resize(total_points);
		shard[i].edge_line_.normal_.resize(total_points);
		
		for (int j = 0; j < total_points; j++) {
			shard[i].edge_line_.point_[j] = {
				shard[i].edge_line_.point_cloud_.points[j].x,
				shard[i].edge_line_.point_cloud_.points[j].y,
				shard[i].edge_line_.point_cloud_.points[j].z
			};
			shard[i].edge_line_.normal_[j] = {
				shard[i].edge_line_.point_cloud_.points[j].normal_x,
				shard[i].edge_line_.point_cloud_.points[j].normal_y,
				shard[i].edge_line_.point_cloud_.points[j].normal_z
			};
		}
		cout << "Found " << total_points << " unique points" << endl;
		cout << "Successfully processed PCD file" << endl;

		// Load surface data
		cout << "Attempting to read XYZ file: " << surface_in[i] << endl;
		LoadSurface(shard[i].surface_in_, surface_in[i]);
		cout << "Attempting to read XYZ file: " << surface_out[i] << endl;
		LoadSurface(shard[i].surface_out_, surface_out[i]);
		cout << "Attempting to read PCD file: " << surface_fr[i] << endl;
		if (pcl::io::loadPCDFile<pcl::PointXYZRGBNormal>(surface_fr[i], shard[i].surface_fr_.point_cloud_) == -1) {
			cout << "Error: Could not open file " << surface_fr[i] << endl;
		}
	}

	cout << "#################### Ground Truth data load ####################" << endl;
	//#################### Ground Truth data load ####################//
	LoadGroundTruth(GT_trans, GT_graph);
	cout << GT_graph << endl;

	cout << "#################### Save initial state ####################" << endl;
	//#################### Save initial state ####################//
	// Skip visualization setup - no viewer needed
	
	vector<Trans> T_axis(SHARD_NUMBER);
	vector<LCSRM> LCS_out(SHARD_NUMBER);

	cout << "#################### Change Axis symmetrix to z axis ####################" << endl;
	//#################### Change Axis symmetrix to z axis ####################//
	for (int i = 0; i < SHARD_NUMBER; i++) {
		Matrix3d R_d;	Vector3d t_d;
		if (shard_on_off[i]) {
			AxisAlignment(shard[i].edge_line_, R_d, t_d);	
			shard[i].SurMove(R_d, t_d, true);
			T_axis[i].Input(R_d, t_d);		// Save transformation matrix to z-axis
			CalculateFeatureAxisless(shard[i]);
		}
	}

	cout << "#################### Feature matching ####################" << endl;
	//#################### Feature matching ####################//
	FeatureMatching(shard, LCS_out);

	cout << "#################### Pairwise pruning ####################" << endl;
	//#################### Pairwise pruning ####################//
	Pruning(LCS_out);

	cout << "#################### Incremental graph building ####################" << endl;
	//////#################### Incremental graph building ####################//
	StateManager manager(TOP_k, BRANCH_b, shard, LCS_out, step_size, path + "Result");
	manager.BuildStep();

	int num_total = manager.out_state_.size();
	cout << "#################### Total result : " << num_total << " ####################" << endl;

	e_time = clock();
	double time_result = (e_time - s_time) / 1000;
	cout << "Time after data load : " << time_result << "sec" << endl;

	if (manager.out_state_.size() > 1) {
		if (manager.out_state_[0].state_score_ == manager.out_state_[1].state_score_)
			cout << "!!!!!!!!!! There is co-first ranked results check both results !!!!!!!!!!" << endl;
	}

	// Automatically save results for the best reconstruction (index 0)
	cout << "#################### Automatic Result Saving ####################" << endl;
	int count_move_state = 0;  // Use best result
	string path_result = path + "Result/";
	
	// Create dummy visualize objects for compatibility
	vector<Visualize> pc_origin(SHARD_NUMBER);
	vector<bool> right_sherd(SHARD_NUMBER, true);
	
	// Save the reconstruction result
	SaveResult(pc_origin, manager, count_move_state, path_result);
	cout << "#################### Result save finish : " << path_result << endl;

	// Save transformation matrices  
	int num_graph = manager.out_state_[count_move_state].graph_.size();
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (manager.out_state_[count_move_state].true_node_[i]) {
			Trans T_out;
			T_out.name_ = "Piece_" + to_string(i + 1) + "_T";
			for (int j = 0; j < num_graph; j++) {
				if (manager.out_state_[count_move_state].graph_[j].node_[i]) {
					Matrix4d T_obj;
					manager.out_state_[count_move_state].graph_[j].T_[i].Output(T_obj);
					T_out.Input(T_obj);
					T_out.Save(path_result);
					break;
				}
			}
		}
	}
	cout << "#################### Saving transformation matrix over : " << path_result << endl;

	// Calculate and save accuracy
	MatrixXd graph = MatrixXd::Zero(SHARD_NUMBER, SHARD_NUMBER);
	vector<Trans> T_result = T_axis;
	
	pair<int, int> sherd_acc, edge_acc;
	int k_sherd, k_edge, t_sherd, t_edge;
	tie(k_sherd, t_sherd, k_edge, t_edge) = CountResult(GT_graph, GT_trans, graph, T_result, right_sherd);
	
	sherd_acc = make_pair(k_sherd, t_sherd);
	edge_acc = make_pair(k_edge, t_edge);
	SaveAcc(path_result, sherd_acc, edge_acc, time_result);

	cout << "#################### SfS Batch Processing Complete ####################" << endl;
	cout << "Results saved to: " << path_result << endl;
	cout << "Shard accuracy: " << k_sherd << "/" << t_sherd << endl;
	cout << "Edge accuracy: " << k_edge << "/" << t_edge << endl;
	cout << "Total processing time: " << time_result << " seconds" << endl;

	return 0;
}