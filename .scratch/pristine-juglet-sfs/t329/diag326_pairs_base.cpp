// Ticket 326 diagnosis harness. NOT the assembler and never used to score.
//
// Links the pristine 9195c01 class objects (built from /opt/sfs-pristine inside the
// image) and, for every edge of the ground-truth graph, reports where that join dies:
//   GTPAIR  : the gates evaluated at the answer-key placement (oracle; diagnosis only)
//   LCSDIAG : whether the LCS feature conditions can produce a run along the true seam
//   CAND    : every FeatureComp candidate, its ICP outcome exactly as PairwisePruning
//             computes it, its error against the answer key, and the rule that drops it
//   MERGE   : every MergeSimPair collapse (isSimilar(0.175, 20))
//   PIECE   : per-piece size measures (scale candidates) and normal/wall convention
// Loading, axis alignment, features and FeatureComp are the shim's, verbatim.
// The pruning/merge loops are copies of PairwisePruning/MergeSimPair with logging added.
//
// Build/run: hpc/scripts/diag326.sh (inside the image, against /panel/build objects).

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
#define FLOOR 20
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

// Translation norm as isSimilarTrans sees it, and the RMS displacement of piece x's
// own breakline points between the two placements (the physical error, mm).
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

static double Median(vector<double> v)
{
	if (v.empty()) return -1;
	sort(v.begin(), v.end());
	return v[v.size() / 2];
}

static double NearestDist(const MatrixXd& P, const Vector3d& q, int& idx)
{
	double best = 1e18; idx = -1;
	for (int j = 0; j < P.cols(); j++) {
		double d = (P.col(j) - q).squaredNorm();
		if (d < best) { best = d; idx = j; }
	}
	return sqrt(best);
}

int main(int argc, char** argv)
{
	cout << "DIAG326 harness (pristine 9195c01 objects) floor=" << FLOOR << endl;
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

	//#################### PIECE: scale candidates and normal/wall convention ####################//
	cout << "PIECE\tpiece\tnpts\tspacing_med\tbl_len\tbbox_diag\tbl_r_med\tbl_h_span\tthick_med\tn_radial_pos_frac\td_sur0_med\td_sur1_med\tr_sur0_med\tr_sur1_med" << endl;
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard[i].is_matching_) continue;
		const BreakLine& L = shard[i].edge_line_;
		int n = L.point_.cols();
		vector<double> sp, rr, th, d0, d1, r0, r1;
		double len = 0;
		Vector3d mn = L.point_.rowwise().minCoeff(), mx = L.point_.rowwise().maxCoeff();
		int pos = 0;
		for (int k = 0; k < n; k++) {
			double s = (L.point_.col((k + 1) % n) - L.point_.col(k)).norm();
			sp.push_back(s); len += s;
			Vector3d p = L.point_.col(k);
			Vector2d r2(p(0), p(1));
			rr.push_back(r2.norm());
			Vector3d rh(p(0), p(1), 0); rh.normalize();
			if (L.normal_.col(k).dot(rh) > 0) pos++;
			double t6 = L.feature_[0](6, k);
			if (t6 > 0) th.push_back(t6);
			int j0, j1;
			if (shard[i].sur_in_.point_.cols() > 0) {
				d0.push_back(NearestDist(shard[i].sur_in_.point_, p, j0));
				Vector3d q = shard[i].sur_in_.point_.col(j0);
				r0.push_back(Vector2d(q(0), q(1)).norm());
			}
			if (shard[i].sur_out_.point_.cols() > 0) {
				d1.push_back(NearestDist(shard[i].sur_out_.point_, p, j1));
				Vector3d q = shard[i].sur_out_.point_.col(j1);
				r1.push_back(Vector2d(q(0), q(1)).norm());
			}
		}
		cout << "PIECE\t" << i + 1 << "\t" << n << "\t" << Median(sp) << "\t" << len << "\t" << (mx - mn).norm()
			<< "\t" << Median(rr) << "\t" << (mx(2) - mn(2)) << "\t" << Median(th) << "\t" << (double)pos / n
			<< "\t" << Median(d0) << "\t" << Median(d1) << "\t" << Median(r0) << "\t" << Median(r1) << endl;
	}

	//#################### GTPAIR: gates at the answer-key placement ####################//
	cout << "GTPAIR\ty\tx\tgap_min\tn_1to1\tn_d20\tn_dot07\tn_reject_kept\tn_inl15_15deg\tinlierCalc\tscore_at_gt\toverlap50\tarea\toverlap100\tdot_closest\tlinedot_closest\tdot_med_d3\tabsdot_med_d3\tn_d3\tn_d3_dotneg" << endl;
	for (int y = 0; y < SHARD_NUMBER; y++) {
		for (int x = y + 1; x < SHARD_NUMBER; x++) {
			if (!GT_graph(y, x) || !shard[y].is_matching_ || !shard[x].is_matching_) continue;
			BreakLine A = shard[y].edge_line_, B = shard[x].edge_line_;
			Move4(B, GTRel(x, y));
			double gap = 1e9; int ia = -1, ib = -1;
			for (int b = 0; b < B.point_.cols(); b++) {
				int j; double d = NearestDist(A.point_, B.point_.col(b), j);
				if (d < gap) { gap = d; ia = j; ib = b; }
			}
			double dot_c = A.normal_.col(ia).dot(B.normal_.col(ib));
			double ldot_c = A.line_normal_.col(ia).dot(B.line_normal_.col(ib));
			A.BuildTree(); B.BuildTree();
			Corres c; c.index_A = y + 1; c.index_B = x + 1;
			MakeCorWOBuildTree(c, A, B, true);
			int n1 = c.cor.size(), nd20 = 0, ndot = 0, nd3 = 0, nd3neg = 0;
			vector<double> dots3, adots3;
			for (auto& p : c.cor) {
				double d = (p.p_A - p.p_B).norm(), dt = p.n_A.dot(p.n_B);
				if (d < 20) nd20++;
				if (dt > 0.7) ndot++;
				if (d < 3) { nd3++; dots3.push_back(dt); adots3.push_back(fabs(dt)); if (dt < 0) nd3neg++; }
			}
			vector<Corres> cv; cv.push_back(c);
			int inl15 = 0;
			CountInlier(inl15, cv, INLIER_THRESHOLD, ANGLE_THRESHOLD);
			RejectOutlier(c, 20, 0.7);
			int nkept = c.cor.size();
			vector<Corres> cc; cc.push_back(c);
			double score = 11;
			MatchingScore(score, cc);
			vector<BreakLine> LL;
			for (int k = 0; k < SHARD_NUMBER; k++) LL.push_back(shard[k].edge_line_);
			vector<bool> tn(SHARD_NUMBER, false); tn[y] = tn[x] = true;
			int inl = 0;
			bool ic = inlierCalculate(inl, tn, cc, LL);
			A.RemoveTree(); B.RemoveTree();
			double area = 0, size = 0, area2 = 0, size2 = 0;
			BreakLine A2 = shard[y].edge_line_, B2 = shard[x].edge_line_;
			Move4(B2, GTRel(x, y));
			bool ov = OverlapCheck_3d(A2, B2, area, size, 50.0);
			bool ov100 = OverlapCheck_3d(A2, B2, area2, size2, 100.0);
			cout << "GTPAIR\t" << y + 1 << "\t" << x + 1 << "\t" << gap << "\t" << n1 << "\t" << nd20 << "\t" << ndot
				<< "\t" << nkept << "\t" << inl15 << "\t" << (ic ? inl : -1) << "\t" << score << "\t" << ov << "\t" << area
				<< "\t" << ov100 << "\t" << dot_c << "\t" << ldot_c << "\t" << Median(dots3) << "\t" << Median(adots3)
				<< "\t" << nd3 << "\t" << nd3neg << endl;
		}
	}

	//#################### LCSDIAG: can the LCS conditions run along the true seam? ####################//
	// FeatureComp orientation: y = i (fixed, original order), x = j > i (moving, reversed).
	cout << "LCSDIAG\ty\tx\tn_true_cor\tfrac_c1\tfrac_c2\tfrac_c3\tfrac_c4\tfrac_all\tlongest_true_run\tlongest_true_all_run\twindow_needed" << endl;
	{
		vector<double> Q_size = { 0.15, 0.15, 0.15, 0.2 };
		for (int y = 0; y < SHARD_NUMBER; y++) {
			for (int x = y + 1; x < SHARD_NUMBER; x++) {
				if (!GT_graph(y, x) || !shard[y].is_matching_ || !shard[x].is_matching_) continue;
				BreakLine& Y = shard[y].edge_line_;
				BreakLine& X = shard[x].edge_line_;
				X.ChangeOrder();
				BreakLine Xg = X;
				Move4(Xg, GTRel(x, y));
				MatrixXd Qx = Quantization(X.feature_[0], Q_size);
				MatrixXd Qy = Quantization(Y.feature_[0], Q_size);
				int m = Y.point_.cols(), nX = X.point_.cols();
				// true correspondence: for each y point, nearest x point at GT within 3 mm
				vector<int> tc(m, -1);
				int ntc = 0;
				for (int i = 0; i < m; i++) {
					int j; double d = NearestDist(Xg.point_, Y.point_.col(i), j);
					if (d < 3.0) { tc[i] = j; ntc++; }
				}
				int c1 = 0, c2 = 0, c3 = 0, c4 = 0, call = 0, run = 0, best = 0, runa = 0, besta = 0;
				int prev_j = -10;
				for (int i = 0; i < m; i++) {
					int j = tc[i];
					if (j < 0) { run = runa = 0; prev_j = -10; continue; }
					double C0 = fabs(Qx(0, j) - Qy(0, i)), C1 = fabs(Qx(1, j) - Qy(1, i)), C2 = fabs(Qx(2, j) - Qy(2, i));
					double C3 = fabs(Qx(1, j) + Qy(1, i)), C4 = fabs(Qx(2, j) + Qy(2, i));
					bool k1 = C0 < 3 * Q_size[0];
					bool k2 = (C1 < 2.5 * Q_size[1]) || (C3 < 2.5 * Q_size[1]);
					bool k3 = (C2 < 2.5 * Q_size[2]) || (C4 < 2.5 * Q_size[2]);
					bool k4 = true;
					if ((Qx(3, j) > 0) && (Qy(3, i) > 0)) k4 = fabs(Qx(3, j) - Qy(3, i)) <= 4 * Q_size[3];
					c1 += k1; c2 += k2; c3 += k3; c4 += k4;
					bool all = k1 && k2 && k3 && k4;
					call += all;
					bool diag = (j == prev_j + 1);
					run = diag ? run + 1 : 1;
					runa = all ? (diag ? runa + 1 : 1) : 0;
					best = max(best, run); besta = max(besta, runa);
					prev_j = j;
				}
				X.ChangeOrder();
				double nn = max(1, ntc);
				cout << "LCSDIAG\t" << y + 1 << "\t" << x + 1 << "\t" << ntc << "\t" << c1 / nn << "\t" << c2 / nn << "\t" << c3 / nn
					<< "\t" << c4 / nn << "\t" << call / nn << "\t" << best << "\t" << besta << "\t" << MINIMUM_NUMBER + 1 << endl;
			}
		}
	}

	//#################### CAND: FeatureComp + PairwisePruning with logging ####################//
	list<LCSIndex> LCS_out;
	FeatureComp(shard, LCS_out, 25, MINIMUM_NUMBER, 0);
	cout << "Total number : " << LCS_out.size() << endl;

	// truth of the LCS correspondence itself, at GT
	map<int, double> lcs_true_frac;
	for (auto& c : LCS_out) {
		BreakLine pA = shard[c.shard_y_ - 1].edge_line_, pB = shard[c.shard_x_ - 1].edge_line_;
		LCSIndex cc = c;
		MakeSection(shard, pA, pB, cc);
		Move4(pB, GTRel(c.shard_x_ - 1, c.shard_y_ - 1));
		int ok = 0;
		for (int k = 0; k < pA.point_.cols(); k++) if ((pA.point_.col(k) - pB.point_.col(k)).norm() < 3.0) ok++;
		lcs_true_frac[c.index_] = pA.point_.cols() ? (double)ok / pA.point_.cols() : 0;
	}

	{
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

		cout << "CAND\tidx\ty\tx\tgt_edge\tsize\tlcs_true_frac\tscore\tinlier\toverlap\tarea\taxis_angle\trot_err_deg\tt_err_norm\trms_err_mm" << endl;
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
			double axis_angle = acos(axis_move_normal.dot(L[index_f].axis_norm_[iter->axis_index_y_]));
			axis_angle = min(axis_angle, 3.14159 - axis_angle);
			iter->axis_angle_ = axis_angle;
			EdgeLineMove(L[index_f], R_fix_i_1, t_fix_i_1);
			EdgeLineMove(L[index_m], R_fix_i_1, t_fix_i_1);
			double A_dummy(0), length(0);
			bool overlap = OverlapCheck_3d(L[iter->shard_y_ - 1], L[iter->shard_x_ - 1], A_dummy, length, 50.0);
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
				<< "\t" << iter->size_ << "\t" << lcs_true_frac[iter->index_] << "\t" << iter->score_ << "\t" << iter->inliner_
				<< "\t" << overlap << "\t" << A_dummy << "\t" << axis_angle << "\t" << rot << "\t" << tn << "\t" << rms << endl;

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

		// MergeSimPair, verbatim with logging
		cout << "MERGE\tkept_idx\tdropped_idx\ty\tx\tkept_score\tdropped_score\trot_diff_deg\tt_diff_norm\trms_diff_mm" << endl;
		iter = LCS_out.begin();
		for (; iter != LCS_out.end(); ) {
			list<LCSIndex>::iterator comp = iter;
			for (; comp != LCS_out.end(); ) {
				if (comp != iter) {
					if ((iter->SamePart(*comp)) && (iter->trans_.isSimilar(comp->trans_, 0.175, 20))) {
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
		cout << "SURVIVOR\tidx\ty\tx\tgt_edge\tscore\tinlier\trms_err_mm\trot_err_deg" << endl;
		for (auto& c : LCS_out) {
			double rot, tn, rms;
			PlaceErr(M4(c.trans_), GTRel(c.shard_x_ - 1, c.shard_y_ - 1), sh[c.shard_x_ - 1].edge_line_, rot, tn, rms);
			cout << "SURVIVOR\t" << c.index_ << "\t" << c.shard_y_ << "\t" << c.shard_x_ << "\t" << (int)GT_graph(c.shard_y_ - 1, c.shard_x_ - 1)
				<< "\t" << c.score_ << "\t" << c.inliner_ << "\t" << rms << "\t" << rot << endl;
		}
	}
	cout << "DIAG326 done" << endl;
	return 0;
}
