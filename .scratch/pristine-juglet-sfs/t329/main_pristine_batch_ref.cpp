// INFRA shim for 01-pristine-pot-a-run (O4 Rank 2 fallback, declared INFRA).
//
// WHY THIS FILE EXISTS: upstream main.cpp (SeongJong-Yoo/structure-from-sherds-pp
// @9195c01) is GUI-only — every post-BuildStep step (fine registration,
// CountResult/SaveAcc scoring, SaveResult, transform saving) runs only inside
// `while (!viewer->wasStopped())` on keyboard flags, and the PCL/VTK render
// path segfaults under Xvfb on Spartan (diag job 32976970: exit 139/134/139 on
// three GL env variants). So the pristine tree cannot score headless unmodified.
//
// FIDELITY: this file is upstream main.cpp with ONLY viewer/display statements
// removed — no algorithm, threshold, weight, solver, or order change:
//   - dropped: viewer construction, keyboard callback + registration, viewer
//     setting calls, UpdateData/AddPointCloud/MeshTransform/TurnoffandReturn/
//     VisCurrentState/VisSwitchVariables/ObjVisualize/ReverseFine display calls,
//     and the wasStopped() loop itself;
//   - kept VERBATIM (including quirks such as the uninitialised graph_dummy
//     read): data load, NO_BASE_INFO block, GT load, MakePointCloud/MakeMesh,
//     axis alignment + SurMove + CalculateFeatureAxisless + multi-axis block,
//     FeatureComp, PairwisePruning, StateManager::BuildStep, co-first check,
//     right_sherd init, and the straight-line space(1st)+f+s+g key sequence for
//     state 0 (the top-ranked assembly).
// The pinned tree is untouched (git status clean); this file lives outside it.

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
#include "class/visualize.h"
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

int main(int argc, char** argv)
{
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
		// Consider multi-axis shards at the same time
		if(shard_on_off[i])	{
			shard[i].edge_line_.ReadPCDFileWithInfo(file_path[i]);
			if (shard[i].edge_line_.point_.cols() < 50) {
				shard_on_off[i] = false;
				shard[i].edge_line_.Remove();
				continue;
			}
			shard[i].edge_line_.CalculateLineNormal();
			int breakline_points = shard[i].edge_line_.point_.cols();
			max_breakline_points = max(max_breakline_points, breakline_points);
			shard[i].LoadSurface(surface_in[i], surface_out[i], surface_fr[i]);
			shard[i].is_matching_ = true;
			shard[i].sur_frac_.CalculateLineNormal();
		}
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
		//cout << single_graph << endl;

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

	cout << "#################### Save initial state ####################" << endl;
	//#################### Save initial state ####################//
	vector<Visualize> pc_origin(SHARD_NUMBER);
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (shard_on_off[i]) {
			std::string pointName = "origin_" + std::to_string(i + 1);
			pc_origin[i].MakePointCloud(shard[i].edge_line_.point_, shard[i].edge_line_.normal_, pointName);
			pointName = "o_Mesh" + std::to_string(i + 1);
			pc_origin[i].MakeMesh(obj_path[i], pointName);
		}
	}

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

			// (display-only UpdateData/AddPointCloud/MeshTransform dropped)
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
	int count_move_state(0);

	cout << "#################### Incremental graph building ####################" << endl;
	//////#################### Incremental graph building ####################//
	StateManager manager(TOP_k, BRANCH_b, shard, LCS_out, step_size, path + "Result");
	manager.BuildStep();

	e_time = clock();
	double time_result = (e_time - s_time) / 1000;
	cout << "Time after data load : " << time_result << "sec" << endl;

	if (manager.out_state_.size() > 1) {
		if (manager.out_state_[0].state_score_ == manager.out_state_[1].state_score_)
			cout << "!!!!!!!!!! There is co-first ranked results check both results !!!!!!!!!!" << endl;
	}

	count_move_state = 0;
	iter = LCS_out.begin();
	vector<Trans> T_fine(SHARD_NUMBER);
	vector<bool> right_sherd(SHARD_NUMBER, true);
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard_on_off[i])
			right_sherd[i] = false;
	}

	// Straight-line equivalent of the space(1st)+f+s+g key sequence on state 0.
	// --- space (first_): reset right_sherd (count_move_state == 0 path) ---
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (shard_on_off[i])
			right_sherd[i] = true;
		else
			right_sherd[i] = false;
	}

	// --- f (fine_): fine registration + CountResult + SaveAcc ---
	{
		cout << "#################### Performing Fine Registration ####################" << endl;
		int num_graph = manager.out_state_[count_move_state].graph_.size();
		vector<Trans> T_result = T_axis;
		vector<Matrix3d> R_fine(SHARD_NUMBER);
		vector<Vector3d> t_fine(SHARD_NUMBER);
		MatrixXd graph(SHARD_NUMBER, SHARD_NUMBER);
		for (int i = 0; i < SHARD_NUMBER; i++) {
			T_fine[i].Set(Matrix4d::Identity());
			R_fine[i] = Matrix3d::Identity();
			t_fine[i] = { 0, 0, 0 };
			for (int j = 0; j < SHARD_NUMBER; j++) {
				graph(i, j) = 0;
			}
		}

		// ########## Restore the solution
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
			MatrixXd graph_dummy(SHARD_NUMBER, SHARD_NUMBER);

			for (int j = 0; j < SHARD_NUMBER; j++) {
				if (manager.out_state_[count_move_state].graph_[i].node_[j]) {
					T_fine[j].Input(R_fine[j], t_fine[j]);
					T_result[j].Input(R_fine[j], t_fine[j]);
					Matrix3d R_dummy = Matrix3d::Identity();
					Vector3d t_dummy = { 0, double(200 * i), 0 };
					EdgeLineMove(manager.shard_[j].edge_line_, R_dummy, t_dummy);

					T_fine[j].Output(R_dummy, t_dummy);
					t_dummy[1] = t_dummy[1] + 200 * i;
				}
				for (int k = 0; k < SHARD_NUMBER; k++) {
					if (graph_dummy(j, k))
						graph(j, k) = 1;
				}
			}
		}
		string path_result = path + "Result/";
		int k_sherd, k_edge, t_sherd, t_edge;
		tie(k_sherd, t_sherd, k_edge, t_edge)
			= CountResult(GT_graph, GT_trans,
				graph, T_result,
				right_sherd);

		pair<int, int> sherd_acc, edge_acc;
		sherd_acc = make_pair(k_sherd, t_sherd);
		edge_acc = make_pair(k_edge, t_edge);
		SaveAcc(path_result, sherd_acc, edge_acc, time_result);
	}

	// --- s (save_): SaveResult ---
	{
		string path_result = path + "Result/";
		SaveResult(pc_origin, manager, count_move_state, path_result);
		cout << "#################### Result save finish : " << path_result << endl;
	}

	// --- g (ground_): save transformation matrices ---
	{
		string path_result = path + "Result/";
		int num_graph = manager.out_state_[count_move_state].graph_.size();
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
		cout << "#################### Saving transformation matrix over : " << path_result << endl;
	}

	return 0;
}
