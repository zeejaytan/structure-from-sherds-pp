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
#include <pcl/console/parse.h>
#include <pcl/common/transforms.h>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include "ceres/ceres.h"
#include "class/data_path.h"
#include "class/data_structure.h"
#include "class/reconstruction.h"
#include "class/feature_matching.h"
#include "class/ranking_system.h"

// Original beam search parameters
#define TOP_k 5
#define BRANCH_b 3

//#define NO_RIM_INFO
#define NO_BASE_INFO

using namespace std;
using namespace Eigen;

vector<Geom> shard(SHARD_NUMBER);
vector<Trans> GT_trans(SHARD_NUMBER);
MatrixXd GT_graph(SHARD_NUMBER, SHARD_NUMBER);

int main(int argc, char** argv)
{
	cout << "#################### SFS++ HEADLESS MODE ####################" << endl;
	double calculation_time(0);
	int s_time(0), e_time(0);

	int step_size = shard.size();
	if (argv[1] != NULL) {
		step_size = std::atoi(argv[1]);
	}

	cout << "#################### Pottery Data load ####################" << endl;
	//#################### Pottery Data load ####################//
	int max_breakline_points(0);
    for (int i = 0; i < SHARD_NUMBER; i++) {
        shard[i].edge_line_.ReadAxis(axis_path[i]);
        if (shard[i].edge_line_.axis_point_.empty()) {
            shard_on_off[i] = false;
            continue;
        }
        if(!shard_on_off[i]) continue;
        // Read breakline (accept both legacy SFS header and standard PCD)
        shard[i].edge_line_.ReadPCDFileWithInfo(file_path[i]);
        if (shard[i].edge_line_.point_.cols() < 50) {
            shard_on_off[i] = false;
            shard[i].edge_line_.Remove();
            continue;
        }
        // Load surfaces; fall back to in/out only if fractional surface missing
        namespace fs = std::experimental::filesystem;
        if (fs::exists(surface_fr[i])) {
            shard[i].LoadSurface(surface_in[i], surface_out[i], surface_fr[i]);
        } else {
            shard[i].LoadSurface(surface_in[i], surface_out[i]);
        }
        shard[i].is_matching_ = true;
        // If breakline normals are missing/degenerate, approximate from nearest surface normals
        if (shard[i].edge_line_.normal_.cols() != shard[i].edge_line_.point_.cols()) {
            shard[i].edge_line_.normal_.resize(3, shard[i].edge_line_.point_.cols());
            shard[i].edge_line_.normal_.setZero();
        }
        bool need_normals = false;
        for (int c = 0; c < shard[i].edge_line_.normal_.cols(); ++c) {
            if (shard[i].edge_line_.normal_(0,c)==0 && shard[i].edge_line_.normal_(1,c)==0 && shard[i].edge_line_.normal_(2,c)==0) { need_normals = true; break; }
        }
        if (need_normals && shard[i].sur_in_.point_.cols() > 0) {
            int n_bl = shard[i].edge_line_.point_.cols();
            int n_s  = shard[i].sur_in_.point_.cols();
            for (int c = 0; c < n_bl; ++c) {
                // naive nearest neighbor on surface_in
                double best = std::numeric_limits<double>::infinity();
                int bj = 0;
                for (int j = 0; j < n_s; ++j) {
                    double dx = shard[i].edge_line_.point_(0,c) - shard[i].sur_in_.point_(0,j);
                    double dy = shard[i].edge_line_.point_(1,c) - shard[i].sur_in_.point_(1,j);
                    double dz = shard[i].edge_line_.point_(2,c) - shard[i].sur_in_.point_(2,j);
                    double d2 = dx*dx + dy*dy + dz*dz;
                    if (d2 < best) { best = d2; bj = j; }
                }
                shard[i].edge_line_.normal_(0,c) = shard[i].sur_in_.normal_(0,bj);
                shard[i].edge_line_.normal_(1,c) = shard[i].sur_in_.normal_(1,bj);
                shard[i].edge_line_.normal_(2,c) = shard[i].sur_in_.normal_(2,bj);
            }
        }
        // Now compute line normals for breakline
        shard[i].edge_line_.CalculateLineNormal();
        int breakline_points = shard[i].edge_line_.point_.cols();
        max_breakline_points = max(max_breakline_points, breakline_points);
        shard[i].sur_frac_.CalculateLineNormal();
    }

#ifdef NO_RIM_INFO
	for (int i = 0; i < SHARD_NUMBER; i++) {
		shard[i].edge_line_.is_seg_rim_ = false;
	}
#endif

#ifdef NO_BASE_INFO
	for (int i = 0; i < SHARD_NUMBER; i++) {
		shard[i].edge_line_.is_seg_base_ = false;
	}
#endif

	cout << "#################### Ground Truth data load ####################" << endl;
	for (int i = 0; i < SHARD_NUMBER; i++) {
		for (int j = 0; j < SHARD_NUMBER; j++)
			GT_graph(i, j) = 0;
	}
	int start_index(0);
	for (int i = 0; i < NUM_MIXED_SHERD; i++) {
		MatrixXd single_graph;
		ifstream myfile(gt_graph_path[i]);
		string str;
		vector<string> fileContents;
		stringstream ss;
		while (getline(myfile, str)) {
			fileContents.push_back(str);
		}
		int num_raw = fileContents.size();
		single_graph.resize(num_raw, num_raw);
		for (int j = 0; j < num_raw; j++) {
			ss << fileContents[j];
			for (int k = 0; k < num_raw; k++) {
				ss >> single_graph(j, k);
			}
			ss.clear();
		}

		for (int j = start_index; j < num_raw + start_index; j++) {
			GT_trans[j].Read(gt_T_path[j]);
			for (int k = start_index; k < num_raw + start_index; k++) {
				GT_graph(j, k) = single_graph(j - start_index, k - start_index);
			}
		}
		start_index += num_raw;
	}

	//########## Remove excluded sherd information
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard_on_off[i]) {
			for (int j = 0; j < SHARD_NUMBER; j++) {
				GT_graph(i, j) = 0;
				GT_graph(j, i) = 0;
			}
		}
	}
	cout << GT_graph << endl;

	s_time = clock();

	cout << "#################### Change Axis symmetrix to z axis ####################" << endl;
	//#################### Change Axis symmetrix to z axis ####################//
	vector<Trans> T_axis(SHARD_NUMBER);
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (shard[i].is_matching_) {
			Matrix3d R_d = Matrix3d::Identity();
			Vector3d t_d = { 0, 0, 0 };
			T_axis[i].Set(R_d, t_d, i + 1, i + 1);

			// Align symmetric axis to z-axis
			AxisAlignment(shard[i].edge_line_, R_d, t_d);
			shard[i].SurMove(R_d, t_d, true);

			T_axis[i].Input(R_d, t_d);		// Save transformation matrix to z-axis

			CalculateFeatureAxisless(shard[i]);

			//######## Multi axis
			int num_axis = shard[i].edge_line_.axis_norm_.size();
			if (num_axis > 1) {
				for (int j = 1; j < num_axis; j++) {
					Matrix3d R_a, R_i;
					Vector3d t_a, t_i;
					AxisAlignment(shard[i].edge_line_, R_a, t_a, j);
					CalculateFeatureAxisless(shard[i], j);
					R_i = R_a.inverse();
					t_i = -R_i * t_a;
					shard[i].MoveWOSurface(R_i, t_i);
				}
			}
		}
	}

	cout << "#################### Feature matching ####################" << endl;
	////#################### Feature matching ####################//
	list<LCSIndex> LCS_out;
	FeatureComp(shard, LCS_out, 25, MINIMUM_NUMBER, 0);
	cout << "Total number : " << LCS_out.size() << endl;

	cout << "#################### Pairwise pruning ####################" << endl;
	PairwisePruning(shard, LCS_out);

	list<LCSIndex>::iterator iter = LCS_out.begin();
	cout << "Total number pruned : " << LCS_out.size() << endl;

	cout << "#################### Incremental graph building ####################" << endl;
	//////#################### Incremental graph building ####################//
	StateManager manager(TOP_k, BRANCH_b, shard, LCS_out, step_size, path + "Result");
    manager.BuildStep();

    int num_total = manager.out_state_.size();

    if (num_total == 0) {
        std::cerr << "No assembly states generated; exiting early." << std::endl;
        return 0;
    }

	e_time = clock();
	double time_result = (e_time - s_time) / 1000;
	cout << "Time after data load : " << time_result << "sec" << endl;

	if (manager.out_state_.size() > 1) {
		if (manager.out_state_[0].state_score_ == manager.out_state_[1].state_score_)
			cout << "!!!!!!!!!! There is co-first ranked results check both results !!!!!!!!!!" << endl;
	}

	cout << "#################### Auto-saving first ranked result (headless mode) ####################" << endl;

    int count_move_state = 0;  // Use first ranked result
	string path_result = path + "Result/";

	// Create result directory if it doesn't exist
	std::experimental::filesystem::create_directories(path_result);

	// Prepare transformation matrices
	vector<Trans> T_result = T_axis;
	vector<bool> right_sherd(SHARD_NUMBER, true);
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard_on_off[i])
			right_sherd[i] = false;
	}

	int num_graph = manager.out_state_[count_move_state].graph_.size();
	MatrixXd graph(SHARD_NUMBER, SHARD_NUMBER);
	for (int i = 0; i < SHARD_NUMBER; i++) {
		for (int j = 0; j < SHARD_NUMBER; j++) {
			graph(i, j) = 0;
		}
	}

	// Apply transformations and build graph
	for (int i = 0; i < num_graph; i++) {
		for (int j = 0; j < SHARD_NUMBER; j++) {
			Matrix3d R_prev;
			Vector3d t_prev;
			if (manager.out_state_[count_move_state].graph_[i].node_[j]) {
				manager.out_state_[count_move_state].graph_[i].T_[j].Output(R_prev, t_prev);
				manager.shard_[j].Move(R_prev, t_prev, true);
				T_result[j].Input(R_prev, t_prev);
			}
		}
		for (int j = 0; j < SHARD_NUMBER; j++) {
			for (int k = 0; k < SHARD_NUMBER; k++) {
				if (manager.out_state_[count_move_state].graph_[i].simple_graph_(j, k))
					graph(j, k) = 1;
			}
		}
	}

	// Save transformation matrices
	cout << "Saving transformation matrices..." << endl;
	for (int i = 0; i < SHARD_NUMBER; i++) {
		Matrix4d T_a, T_obj;
		Trans T_out;
		T_axis[i].Output(T_a);
		T_out.Set(T_a, i, i);
		for (int j = 0; j < num_graph; j++) {
			if (manager.out_state_[count_move_state].graph_[j].node_[i]) {
				manager.out_state_[count_move_state].graph_[j].T_[i].Output(T_obj);
				T_out.Input(T_obj);
				T_out.Save(path_result);
				break;
			}
		}
	}
	cout << "Transformation matrices saved to: " << path_result << endl;

	// Save edge lines and surfaces
	cout << "Saving edge lines and surfaces..." << endl;
	for (int j = 0; j < SHARD_NUMBER; j++) {
		if (manager.out_state_[count_move_state].true_node_[j]) {
			string edge_filename = path_result + to_string(j + 1) + ". edge.xyz";
			ofstream edge_file(edge_filename);
			if (edge_file.is_open()) {
				for (int k = 0; k < manager.shard_[j].edge_line_.point_.cols(); k++) {
					edge_file << manager.shard_[j].edge_line_.point_(0, k) << " "
							 << manager.shard_[j].edge_line_.point_(1, k) << " "
							 << manager.shard_[j].edge_line_.point_(2, k) << endl;
				}
				edge_file.close();
			}

			string surface_filename = path_result + to_string(j + 1) + ". surface.xyz";
			ofstream surface_file(surface_filename);
			if (surface_file.is_open()) {
				for (int k = 0; k < manager.shard_[j].sur_in_.point_.cols(); k++) {
					surface_file << manager.shard_[j].sur_in_.point_(0, k) << " "
							    << manager.shard_[j].sur_in_.point_(1, k) << " "
							    << manager.shard_[j].sur_in_.point_(2, k) << endl;
				}
				surface_file.close();
			}
		}
	}
	cout << "Edge lines and surfaces saved!" << endl;

	// Calculate and save accuracy
	cout << "Calculating accuracy..." << endl;
	int k_sherd, k_edge, t_sherd, t_edge;
	tie(k_sherd, t_sherd, k_edge, t_edge) = CountResult(GT_graph, GT_trans, graph, T_result, right_sherd);

	pair<int, int> sherd_acc, edge_acc;
	sherd_acc = make_pair(k_sherd, t_sherd);
	edge_acc = make_pair(k_edge, t_edge);
	SaveAcc(path_result, sherd_acc, edge_acc, time_result);

	cout << "#################### Results Summary ####################" << endl;
	cout << "Total assembly states: " << num_total << endl;
	cout << "Shard accuracy: " << k_sherd << "/" << t_sherd << endl;
	cout << "Edge accuracy: " << k_edge << "/" << t_edge << endl;
	cout << "Processing time: " << time_result << " sec" << endl;
	cout << "Results saved to: " << path_result << endl;
	cout << "#################### Headless execution complete ####################" << endl;

	return 0;
}
