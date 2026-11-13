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
#include <cstdlib>
#include <cmath>
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

// TICKET 332: the LCS window is a switch, not a constant (mirrors main.cpp:211).
// The join test (class/feature_matching.cpp:294) accepts a pair only when the LCS run along the
// two edge lines is LONGER than the window, so the window is a physical length expressed in
// dots: 6 dots at the shipped 1.9 mm sampling is ~12.6 mm, and 12 dots at the doubled 0.95 mm
// sampling keeps the same physical window (~12.4 mm) while the sampling doubles.
// SFS_LCS_WINDOW unset or unparseable -> MINIMUM_NUMBER, i.e. today's behaviour, byte for byte.
// MINIMUM_NUMBER itself is deliberately NOT changed: it also gates registration at
// class/feature_matching.cpp:1015 and :1045 (dummy_inlier > MINIMUM_NUMBER / 2).
static int ReadLcsWindowFromEnv()
{
	int window = MINIMUM_NUMBER;
	const char* raw = std::getenv("SFS_LCS_WINDOW");
	if (raw == NULL || *raw == '\0')
		return window;
	char* end = NULL;
	const long parsed = std::strtol(raw, &end, 10);
	if (end == raw || (end != NULL && *end != '\0') || parsed < 1 || parsed > 1000) {
		cout << "[LCS-332] SFS_LCS_WINDOW='" << raw << "' is not a positive integer <= 1000; "
		     << "using MINIMUM_NUMBER=" << MINIMUM_NUMBER << endl;
		return window;
	}
	return static_cast<int>(parsed);
}

// TICKET 337: search width as a switch pair (top-N states kept, s_ branches explored per
// state). Read here and handed to StateManager; the library #defines are not touched, so
// unset is exactly TOP_k / BRANCH_b.
static int ReadWidthFromEnv(const char* name, int fallback, int lo, int hi)
{
	const char* raw = std::getenv(name);
	if (raw == NULL || *raw == '\0') return fallback;
	char* end = NULL;
	const long parsed = std::strtol(raw, &end, 10);
	if (end == raw || (end != NULL && *end != '\0') || parsed < lo || parsed > hi) {
		cout << "[SFS-337] " << name << "='" << raw << "' is not an integer in [" << lo << "," << hi
			<< "]; using " << fallback << endl;
		return fallback;
	}
	return static_cast<int>(parsed);
}

int main(int argc, char** argv)
{
	double calculation_time(0);
	int s_time(0), e_time(0);

	int step_size = shard.size();
	if (argv[1] != NULL) {
		step_size = std::atoi(argv[1]);
	}

	// TICKET 335: both ranking switches are read once here; unset -> legacy, and both
	// banners are printed before any geometry is touched.
	InitSfsSwitches();

	// TICKET 332: the effective LCS window, printed once so every log states its own regime.
	const int lcs_window = ReadLcsWindowFromEnv();
	cout << "LCS window: " << lcs_window << endl;

	// TICKET 337: the search width, printed once so every log states its own regime.
	const int state_topk = ReadWidthFromEnv("SFS_TOPK", TOP_k, 1, 200);
	const int state_branch = ReadWidthFromEnv("SFS_BRANCH", BRANCH_b, 1, 200);
	cout << "state width: topk=" << state_topk << " branch=" << state_branch << endl;

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

	// TICKET 335: keep each piece's STORED axis direction, before AxisAlignment maps every
	// piece's axis onto +z. The sign guard needs it to measure the sign structure.
	vector<Vector3d> axis_orig(SHARD_NUMBER, Vector3d::Zero());
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard[i].edge_line_.axis_norm_.empty()) {
			axis_orig[i] = shard[i].edge_line_.axis_norm_[0];
			if (axis_orig[i].norm() > 0) axis_orig[i].normalize();
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
	// TICKET 337: the GT-joined piece table the state trace tags every priority entry with.
	// Filled only when the trace is on; class/* reads this table and never a GT file.
	if (g_sfs_state_trace) {
		g_sfs_tr_gt.assign(SHARD_NUMBER, vector<char>(SHARD_NUMBER, 0));
		for (int i = 0; i < SHARD_NUMBER; i++)
			for (int j = 0; j < SHARD_NUMBER; j++)
				g_sfs_tr_gt[i][j] = (GT_graph(i, j) > 0.0) ? 1 : 0;
	}
	cout << GT_graph << endl;

	// TICKET 335: the sign guard. It needs the GT poses (loaded just above) because a
	// piece's axis sign has no other reference at load. Refusal is one line and the whole
	// run falls back to the legacy folded axis test; the guard fraction is a stated
	// starting value (0.25), not a knob.
	if (g_sfs_axis_sign_requested) {
		double frac = 0.0;
		int np = 0;
		const bool ok = SignAxisGuard(axis_orig, GT_trans, GT_graph, 0.25, frac, np);
		g_sfs_axis_sign = ok;
		if (!ok)
			cout << "SIGN-AXIS: refused, axes are sign-arbitrary (" << (frac * 100.0)
			     << "% of pairs > 90 deg)" << endl;
		else
			cout << "SIGN-AXIS: accepted, axes are direction-consistent (" << (frac * 100.0)
			     << "% of pairs > 90 deg, n=" << np << " GT pairs)" << endl;
	}

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

	// TICKET 336: the per-piece axis alignment that every saved pose is composed with.
	if (g_sfs_stage_trace) {
		for (int i = 0; i < SHARD_NUMBER; i++) {
			cout << "[SFS-336]\tAXIS\t" << (i + 1);
			SfsTraceMat(T_axis[i]);
			cout << endl;
		}
	}

	cout << "#################### Feature matching ####################" << endl;
	////#################### Feature matching ####################//
	list<LCSIndex> LCS_out;
	FeatureComp(shard, LCS_out, 25, lcs_window, 0);
	cout << "Total number : " << LCS_out.size() << endl;

	cout << "#################### Pairwise pruning ####################" << endl;
	PairwisePruning(shard, LCS_out);

	list<LCSIndex>::iterator iter = LCS_out.begin();
	cout << "Total number pruned : " << LCS_out.size() << endl;

	// TICKET 338 (Part A): every pruned candidate, with its placement error against GT in the
	// axis-aligned frame the search actually uses. G_axis = T_axis[y].GT[y]^-1.GT[x].T_axis[x]^-1
	// is the GT relative placement conjugated into the axis frame; rot is conjugation-invariant
	// and the breakline RMS is rigid-invariant, so these equal the original-frame PlaceErr numbers
	// (placeerr_check.py). GT is read here in the shim only; class/* never sees it. Measurement only.
	if (g_sfs_edge_trace) {
		for (list<LCSIndex>::iterator ci = LCS_out.begin(); ci != LCS_out.end(); ++ci) {
			const int x = ci->shard_x_, y = ci->shard_y_;
			Matrix4d M, Ay, Ax, GTy_inv, GTx, Ax_inv;
			ci->trans_.Output(M);
			T_axis[y - 1].Output(Ay);
			T_axis[x - 1].Output(Ax);
			T_axis[x - 1].InvOut(Ax_inv);
			GT_trans[y - 1].InvOut(GTy_inv);
			GT_trans[x - 1].Output(GTx);
			Matrix4d G_axis = Ay * GTy_inv * GTx * Ax_inv;
			Matrix4d D = M * G_axis.inverse();
			double cc = (D.block<3, 3>(0, 0).trace() - 1.0) * 0.5;
			if (cc < -1.0) cc = -1.0;
			if (cc > 1.0) cc = 1.0;
			const double rot = acos(cc) * 180.0 / 3.14159265358979323846;
			double mat = -1.0;
			const MatrixXd& P = shard[x - 1].edge_line_.point_;
			if (P.cols() > 0) {
				double sum = 0.0; int n = 0;
				for (int c = 0; c < P.cols(); c++) {
					Vector3d p(P(0, c), P(1, c), P(2, c));
					Vector3d a = M.block<3, 3>(0, 0) * p + M.block<3, 1>(0, 3);
					Vector3d b = G_axis.block<3, 3>(0, 0) * p + G_axis.block<3, 1>(0, 3);
					sum += (a - b).squaredNorm(); n++;
				}
				if (n > 0) mat = sqrt(sum / n);
			}
			const int gt_edge = (GT_graph(y - 1, x - 1) > 0.0) ? 1 : 0;
			cout << "[SFS-338]\tCAND338\t" << ci->index_ << "\t" << y << "\t" << x
				<< "\t" << ci->score_ << "\t" << ci->inliner_
				<< "\t" << (ci->axis_raw_ * 180.0 / 3.14159265358979323846)
				<< "\t" << gt_edge << "\t" << rot << "\t" << mat << endl;
		}
		cout << "[SFS-338]\tCANDTOTAL\t" << LCS_out.size() << endl;
	}

	int count_move_state(0);

	cout << "#################### Incremental graph building ####################" << endl;
	//////#################### Incremental graph building ####################//
	StateManager manager(state_topk, state_branch, shard, LCS_out, step_size, path + "Result");
	manager.BuildStep();
	if (manager.out_state_.empty()) {
		cout << "SFS-NO-CANDIDATES no assembly state to save; exit 0" << endl;
		return 0;
	}

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

	// TICKET 335: measurement only -- how often the unchanged cuts fired. Printed only when a
	// switch is on, so the legacy path's output is unchanged.
	if (g_sfs_coverage || g_sfs_axis_sign_requested)
		cout << "[SFS-335] gates: baskets_erased_1.5=" << g_sfs_basket_erased_15
		<< " cands_erased_1.5=" << g_sfs_cand_erased_15
		<< " cands_erased_10=" << g_sfs_cand_erased_10
		<< " coverage=" << (g_sfs_coverage ? 1 : 0)
		<< " signed=" << (g_sfs_axis_sign ? 1 : 0) << endl;

	// TICKET 336: what the winning state is, before its matrices are written.
	if (g_sfs_stage_trace) {
		const State& out = manager.out_state_[count_move_state];
		cout << "[SFS-336]\tOUTSTATE\t" << count_move_state << "\t" << out.state_score_
			<< "\t" << out.graph_.size() << endl;
		for (size_t j = 0; j < out.graph_.size(); j++) {
			cout << "[SFS-336]\tCLUSTER\t" << j << "\t" << out.graph_[j].root_node_
				<< "\t" << out.graph_[j].graph_score_ << "\t";
			int first = 1;
			for (size_t k = 0; k < out.graph_[j].node_.size(); k++) {
				if (out.graph_[j].node_[k]) { cout << (first ? "" : ",") << (k + 1); first = 0; }
			}
			cout << endl;
		}
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
					if (g_sfs_stage_trace) {
						cout << "[SFS-336]\tFINAL\t" << (i + 1) << "\t" << j;
						SfsTraceMat(T_out);
						cout << endl;
					}
					T_out.Save(path_result);
					break;
				}
			}
		}
		cout << "#################### Saving transformation matrix over : " << path_result << endl;
	}

	return 0;
}
