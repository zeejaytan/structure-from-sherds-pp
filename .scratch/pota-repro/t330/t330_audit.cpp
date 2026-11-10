// Ticket 330 audit tool. NOT the assembler and never used to score.
// Derived from ticket 326's hpc/tools/diag326_pairs.cpp (loading shim, GT load and axis
// alignment verbatim); the CAND/DROP/MERGE loops are its copies of PairwisePruning.
//
// Purpose: run the SAME patched library in two regimes by setting g_len_scale at start.
// MeasureLengthScale returns early when g_len_scale > 0, so
//     g_len_scale = 60.0    -> Gate(x) = x * 60/60 = x   (pristine literals, exactly)
//     g_len_scale = 92.77   -> Gate(x) = x * 92.77/60    (the ticket-326 condition)
// Every gate in the patch reads Gate(), so this is the library's own decision path in
// both regimes: no gate is re-implemented and no threshold is hand-picked.
//
// Usage (inside the image, after hpc/scripts style build):
//     ./t330-audit <float S>            > audit_S.txt
//
// Prints: SCALE, LOAD, TOTAL, CAND, DROP, MERGE, SURVIVOR, DONE.

#include <iostream>
#include "glog/logging.h"
#include <time.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <map>
#include <Eigen/Dense>
#include <Eigen/Core>
#include <pcl/common/common_headers.h>
#include <pcl/io/pcd_io.h>
#include "ceres/ceres.h"
#include "class/data_path.h"
#include "class/data_structure.h"
#include "class/visualize.h"
#include "class/reconstruction.h"
#include "class/feature_matching.h"
#include "class/ranking_system.h"

#ifndef FLOOR
#define FLOOR 50
#endif
#define NO_BASE_INFO

using namespace std;
using namespace Eigen;

vector<Geom> shard(SHARD_NUMBER);
vector<Trans> GT_trans(SHARD_NUMBER);
MatrixXd GT_graph(SHARD_NUMBER, SHARD_NUMBER);
vector<Trans> T_axis(SHARD_NUMBER);

static Matrix4d M4(const Trans& T) { Matrix4d m; T.Output(m); return m; }

// Answer-key relative placement of piece x in piece y's axis-aligned frame.
static Matrix4d GTRel(int x, int y)
{
	return M4(T_axis[y]) * M4(GT_trans[y]).inverse() * M4(GT_trans[x]) * M4(T_axis[x]).inverse();
}

static void Move4(BreakLine& L, const Matrix4d& T)
{
	Matrix3d R = T.block<3, 3>(0, 0);
	Vector3d t = T.block<3, 1>(0, 3);
	EdgeLineMove(L, R, t);
}

static double RotDeg(const Matrix4d& E)
{
	Matrix3d R = E.block<3, 3>(0, 0);
	double c = max(-1.0, min(1.0, (R.trace() - 1.0) / 2.0));
	return acos(c) * 180.0 / M_PI;
}

// Translation norm as isSimilarTrans sees it, and the RMS displacement of x's breakline
// between the two placements (mm).
static void PlaceErr(const Matrix4d& T, const Matrix4d& G, const BreakLine& Lx,
	double& rot_deg, double& t_norm, double& rms_mm)
{
	Matrix4d E = T * G.inverse();
	rot_deg = RotDeg(E);
	t_norm = E.block<3, 1>(0, 3).norm();
	double s = 0;
	int n = Lx.point_.cols();
	for (int i = 0; i < n; i++) {
		Vector4d p; p << Lx.point_.col(i), 1;
		s += ((T * p) - (G * p)).head<3>().squaredNorm();
	}
	rms_mm = n > 0 ? sqrt(s / n) : -1;
}

// Nearest-point gap between two breaklines already expressed in the same frame (mm).
static void GapMin(const BreakLine& A, const BreakLine& B, double& gap, double& med)
{
	gap = 1e18; med = -1;
	int n = A.point_.cols(), m = B.point_.cols();
	if (n == 0 || m == 0) return;
	vector<double> d(n);
	for (int i = 0; i < n; i++) {
		double best = 1e18;
		for (int j = 0; j < m; j++) {
			double dd = (A.point_.col(i) - B.point_.col(j)).squaredNorm();
			if (dd < best) best = dd;
		}
		d[i] = sqrt(best);
		if (d[i] < gap) gap = d[i];
	}
	sort(d.begin(), d.end());
	med = d[d.size() / 2];
}

static void PrintT(const Trans& T)
{
	Matrix4d m = M4(T);
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			cout << " " << m(i, j);
}

int main(int argc, char** argv)
{
	double S = (argc > 1) ? atof(argv[1]) : 60.0;
	g_len_scale = (S > 0) ? S : 60.0;
	cout << "SCALE\t" << g_len_scale << "\tgate50=" << Gate(50.0) << "\tgate20=" << Gate(20.0)
		<< "\tgate15=" << Gate(1.5) << "\tgate5=" << Gate(5.0) << "\tgate10=" << Gate(10.0) << endl;

	//#################### Data load (shim, verbatim but for the floor) ####################//
	for (int i = 0; i < SHARD_NUMBER; i++) {
		shard[i].edge_line_.ReadAxis(axis_path[i]);
		if (shard[i].edge_line_.axis_point_.empty()) {
			shard_on_off[i] = false;
			continue;
		}
		if (shard_on_off[i]) {
			shard[i].edge_line_.ReadPCDFileWithInfo(file_path[i]);
			if (shard[i].edge_line_.point_.cols() < FLOOR) {
				shard_on_off[i] = false;
				shard[i].edge_line_.Remove();
				continue;
			}
			shard[i].edge_line_.CalculateLineNormal();
			shard[i].LoadSurface(surface_in[i], surface_out[i], surface_fr[i]);
			shard[i].is_matching_ = true;
			shard[i].sur_frac_.CalculateLineNormal();
		}
	}
#ifdef NO_BASE_INFO
	for (int i = 0; i < SHARD_NUMBER; i++) shard[i].edge_line_.is_seg_base_ = false;
#endif
	for (int i = 0; i < SHARD_NUMBER; i++)
		for (int j = 0; j < SHARD_NUMBER; j++) GT_graph(i, j) = 0;
	int start_index(0);
	for (int i = 0; i < NUM_MIXED_SHERD; i++) {
		MatrixXd single_graph;
		ifstream myfile(gt_graph_path[i]);
		string str;
		vector<string> fileContents;
		stringstream ss;
		while (getline(myfile, str)) fileContents.push_back(str);
		int num_raw = fileContents.size();
		single_graph.resize(num_raw, num_raw);
		for (int j = 0; j < num_raw; j++) {
			ss << fileContents[j];
			for (int k = 0; k < num_raw; k++) ss >> single_graph(j, k);
			ss.clear();
		}
		for (int j = start_index; j < num_raw + start_index; j++) {
			GT_trans[j].Read(gt_T_path[j]);
			for (int k = start_index; k < num_raw + start_index; k++)
				GT_graph(j, k) = single_graph(j - start_index, k - start_index);
		}
		start_index += num_raw;
	}
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard_on_off[i]) {
			for (int j = 0; j < SHARD_NUMBER; j++) { GT_graph(i, j) = 0; GT_graph(j, i) = 0; }
		}
	}
	cout << "LOAD\tn_shard=" << SHARD_NUMBER << "\tn_gt_edges=" << GT_graph.sum() << endl;

	//#################### Axis alignment + features (shim, verbatim) ####################//
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (shard[i].is_matching_) {
			Matrix3d R_d = Matrix3d::Identity();
			Vector3d t_d = { 0, 0, 0 };
			T_axis[i].Set(R_d, t_d, i + 1, i + 1);
			AxisAlignment(shard[i].edge_line_, R_d, t_d);
			shard[i].SurMove(R_d, t_d, true);
			T_axis[i].Input(R_d, t_d);
			CalculateFeatureAxisless(shard[i]);
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

	//#################### CAND / DROP / MERGE: PairwisePruning with logging ####################//
	list<LCSIndex> LCS_out;
	FeatureComp(shard, LCS_out, 25, MINIMUM_NUMBER, 0);
	cout << "TOTAL\t" << LCS_out.size() << endl;

	vector<Geom>& sh = shard;
	list<LCSIndex>::iterator iter = LCS_out.begin();
	vector<LCSIndex> lcsout;
	vector<list<LCSIndex>> lcs_basket;
	list<LCSIndex> lcs_dummy;
	int num_shard = sh.size();
	vector<Matrix3d> R_p(num_shard);
	vector<Vector3d> t_p(num_shard);
	vector<BreakLine> L(num_shard);
	for (int i = 0; i < num_shard; i++) L[i] = sh[i].edge_line_;

	cout << "CAND\tidx\ty\tx\tgt_edge\tsize\tscore\tinlier\toverlap\tarea\taxis_angle\trot_err_deg\tt_err_norm\trms_err_mm\tgap_min\td_med\tnpts_x\tnpts_y\ttrans" << endl;
	while (iter != LCS_out.end()) {
		lcsout.push_back(*iter);
		CycleNode cycle;
		cycle.edges.push_back(lcsout.size() - 1);
		cycle.nodes.push_back(iter->shard_y_);
		cycle.nodes.push_back(iter->shard_x_);
		for (int i = 0; i < num_shard; i++) { R_p[i] = Matrix3d::Identity(); t_p[i] = { 0, 0, 0 }; }
		Icp(L, R_p, t_p, lcsout, cycle, true, true);
		int index_f = iter->shard_y_ - 1;
		int index_m = iter->shard_x_ - 1;
		Matrix3d R_fix_i_1 = R_p[index_f].inverse();
		Vector3d t_fix_i_1 = -R_fix_i_1 * t_p[index_f];
		iter->trans_.Set(R_p[index_m], t_p[index_m], index_m + 1, index_f + 1);
		iter->trans_.Input(R_fix_i_1, t_fix_i_1);
		Matrix3d R_axis; Vector3d t_axis;
		iter->trans_.Output(R_axis, t_axis);
		Vector3d axis_move_normal = R_axis * L[index_m].axis_norm_[iter->axis_index_x_];
		double axis_angle = acos(max(-1.0, min(1.0, axis_move_normal.dot(L[index_f].axis_norm_[iter->axis_index_y_]))));
		axis_angle = min(axis_angle, 3.14159 - axis_angle);
		iter->axis_angle_ = axis_angle;
		EdgeLineMove(L[index_f], R_fix_i_1, t_fix_i_1);
		EdgeLineMove(L[index_m], R_fix_i_1, t_fix_i_1);
		double gap(0), dmed(0);
		GapMin(L[iter->shard_x_ - 1], L[iter->shard_y_ - 1], gap, dmed);
		double A_dummy(0), length(0);
		bool overlap = OverlapCheck_3d(L[iter->shard_y_ - 1], L[iter->shard_x_ - 1], A_dummy, length, Gate(50.0));
		iter->overlap_ = overlap;
		iter->score_ = cycle.score;
		iter->inliner_ = cycle.inlier;
		iter->area_ = A_dummy;
		Matrix3d R_re; Vector3d t_re;
		iter->trans_.InvOut(R_re, t_re);
		EdgeLineMove(L[index_m], R_re, t_re);

		double rot, tn, rms;
		PlaceErr(M4(iter->trans_), GTRel(index_m, index_f), sh[index_m].edge_line_, rot, tn, rms);
		cout << "CAND\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_ << "\t" << (int)GT_graph(index_f, index_m)
			<< "\t" << iter->size_ << "\t" << iter->score_ << "\t" << iter->inliner_
			<< "\t" << overlap << "\t" << A_dummy << "\t" << axis_angle << "\t" << rot << "\t" << tn << "\t" << rms
			<< "\t" << gap << "\t" << dmed << "\t" << L[iter->shard_x_ - 1].point_.cols() << "\t" << L[iter->shard_y_ - 1].point_.cols();
		PrintT(iter->trans_);
		cout << endl;

		if (iter == LCS_out.begin()) { lcs_dummy.push_back(*iter); ++iter; }
		else {
			list<LCSIndex>::iterator iter_d = iter; iter_d--;
			if (!(iter_d->SamePart(*iter))) {
				lcs_basket.push_back(lcs_dummy); lcs_dummy.clear(); lcs_dummy.push_back(*iter); ++iter;
			}
			else { lcs_dummy.push_back(*iter); ++iter; }
		}
		if (iter == LCS_out.end()) lcs_basket.push_back(lcs_dummy);
	}

	LCS_out.clear();
	cout << "DROP\tidx\ty\tx\treason" << endl;
	for (int i = 0; i < lcs_basket.size(); i++) {
		double lowest_score(9999);
		iter = lcs_basket[i].begin();
		for (; iter != lcs_basket[i].end();) {
			if (iter->axis_angle_ > 0.436) {
				cout << "DROP\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_ << "\taxis_angle>0.436" << endl;
				iter = lcs_basket[i].erase(iter);
			}
			else { lowest_score = min(lowest_score, iter->score_); ++iter; }
		}
		for (iter = lcs_basket[i].begin(); iter != lcs_basket[i].end();) {
			if (lowest_score > 1.5) {
				cout << "DROP\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_ << "\tbasket_lowest_score>1.5(" << lowest_score << ")" << endl;
				iter = lcs_basket[i].erase(iter);
			}
			else if (iter->overlap_) {
				cout << "DROP\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_ << "\toverlap(area=" << iter->area_ << ")" << endl;
				iter = lcs_basket[i].erase(iter);
			}
			else { LCS_out.push_back(*iter); ++iter; }
		}
	}

	// MergeSimPair, verbatim with logging; the translation bar is Gate(20)
	cout << "MERGE\tkept_idx\tdropped_idx\ty\tx\tkept_score\tdropped_score\trot_diff_deg\tt_diff_norm\trms_diff_mm" << endl;
	iter = LCS_out.begin();
	for (; iter != LCS_out.end(); ) {
		list<LCSIndex>::iterator comp = iter;
		for (; comp != LCS_out.end(); ) {
			if (comp != iter) {
				if ((iter->SamePart(*comp)) && (iter->trans_.isSimilar(comp->trans_, 0.175, Gate(20)))) {
					double rot, tn, rms;
					PlaceErr(M4(iter->trans_), M4(comp->trans_), sh[iter->shard_x_ - 1].edge_line_, rot, tn, rms);
					if (iter->score_ < comp->score_) {
						cout << "MERGE\t" << iter->index_ << "\t" << comp->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_ << "\t" << iter->score_ << "\t" << comp->score_ << "\t" << rot << "\t" << tn << "\t" << rms << endl;
						Merge(*iter, *comp);
						comp = LCS_out.erase(comp);
					}
					else {
						cout << "MERGE\t" << comp->index_ << "\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_ << "\t" << comp->score_ << "\t" << iter->score_ << "\t" << rot << "\t" << tn << "\t" << rms << endl;
						Merge(*comp, *iter);
						iter = LCS_out.erase(iter);
						comp++;
					}
				}
				else comp++;
			}
			else comp++;
		}
		iter++;
	}
	cout << "PRUNED\t" << LCS_out.size() << endl;
	cout << "SURVIVOR\tidx\ty\tx\tgt_edge\tscore\tinlier\tarea\taxis_angle\trot_err_deg\tt_err_norm\trms_err_mm\tsize\ttrans" << endl;
	for (auto& c : LCS_out) {
		double rot, tn, rms;
		PlaceErr(M4(c.trans_), GTRel(c.shard_x_ - 1, c.shard_y_ - 1), sh[c.shard_x_ - 1].edge_line_, rot, tn, rms);
		cout << "SURVIVOR\t" << c.index_ << "\t" << c.shard_y_ << "\t" << c.shard_x_ << "\t" << (int)GT_graph(c.shard_y_ - 1, c.shard_x_ - 1)
			<< "\t" << c.score_ << "\t" << c.inliner_ << "\t" << c.area_ << "\t" << c.axis_angle_ << "\t" << rot << "\t" << tn << "\t" << rms << "\t" << c.size_;
		PrintT(c.trans_);
		cout << endl;
	}
	cout << "DONE" << endl;
	return 0;
}