#include "feature_matching.h"
#include "../class/ranking_system.h"
#include <cstdlib>
#include <string>

//############################## TICKET 335 switches ##############################//
// Two ranking levers, one switch each, default OFF. Read ONCE at start-up by
// InitSfsSwitches(); unset, empty or unparseable -> legacy. Neither switch moves a
// shipped constant: 0.436, 1.5, 10, 0.175/20, MINIMUM_NUMBER, INLIER_THRESHOLD and
// ANGLE_THRESHOLD are untouched.
//   SFS_SCORE_COVERAGE=1 : RANKING ONLY. rank_score = score_legacy / max(c, 0.05),
//                          c = matched dots / min(n_dots_i, n_dots_j), is used where two
//                          survivors compete for one slot (MergeSimPair's twin choice).
//                          The basket (1.5) and registration (10) gates keep comparing
//                          score_legacy, so every erasure verdict is identical to legacy.
//   SFS_AXIS_SIGN=1      : the axis gate uses the RAW angle (no min(a, pi - a)) at the same
//                          0.436 cut, but only if the load-time sign guard accepts the piece
//                          set (SignAxisGuard in reconstruction.cpp).
bool g_sfs_coverage = false;               // SFS_SCORE_COVERAGE=1
bool g_sfs_axis_sign_requested = false;    // SFS_AXIS_SIGN=1, before the guard
bool g_sfs_axis_sign = false;              // SFS_AXIS_SIGN=1 AND the guard accepted the set
// TICKET 335: measurement only -- how often the UNCHANGED cuts fire under each switch.
// Printed once at the end of the run by the batch shim, only when a switch is on.
long g_sfs_basket_erased_15 = 0;   // baskets erased whole by the 1.5 cut
long g_sfs_cand_erased_15 = 0;     // candidates erased by the 1.5 cut
long g_sfs_cand_erased_10 = 0;     // candidates erased by the registration 10 cut
// TICKET 336: read-only stage trace. SFS_STAGE_TRACE must be exactly "1"; any other
// value (including unset) leaves it off, and with it off none of the dump blocks below
// executes. Nothing here writes, sorts or reorders a container.
bool g_sfs_stage_trace = false;
// TICKET 337: state-level trace of the search (SFS_STATE_TRACE=1). Read-only in the same
// sense as 336: nothing here writes, sorts or reorders a container. g_sfs_tr_gt is the
// GT-joined piece table and ONLY the batch shim fills it (hpc/tools/main_pristine_batch.cpp),
// so class/* never opens a GT file.
bool g_sfs_state_trace = false;
int g_sfs_tr_step = -1;                    // step whose lines are being printed
int g_sfs_tr_next_id = 0;                  // next state id handed out
vector<vector<char>> g_sfs_tr_gt;          // [pieceA-1][pieceB-1] = 1 when GT-joined
// TICKET 338: edge-level trace (SFS_EDGE_TRACE=1). Read-only in the same sense as 336/337:
// nothing here writes, sorts or reorders a container; every dump block is guarded by this
// flag, so with it off the search executes exactly the statements it executed before.
bool g_sfs_edge_trace = false;
// TICKET 339: admission trace (SFS_ADMIT_TRACE=1). Read-only in the same sense as 336/337/338:
// nothing here writes, sorts or reorders a container, and every [SFS-339] line is guarded by this
// flag. Off (the default) prints one banner line and no [SFS-339] line at all, so the search
// executes exactly the statements it executed before.
bool g_sfs_admit_trace = false;
// TICKET 339: why CountPCInlier() last refused a pair, for the pc_* drop line. 0 = nothing set,
// 1 = a profile distance above 10, 2 = the breakline over the rim on both sides. RemoveEdgeUsingPCInlier
// resets it unconditionally before each call - a plain zero store that nothing reads while the trace
// is off - and it is only set to 1 or 2 while the trace is on.
int g_sfs339_pc_reason = 0;
// TICKET 340: candidate-level pick trace (SFS_PICK_TRACE=1). Read-only in the same sense as
// 336/337/338/339: nothing here writes, sorts or reorders a container, and every [SFS-340] line is
// guarded by this flag. Off (the default) the search executes exactly the statements it executed
// before.
bool g_sfs_pick_trace = false;
// TICKET 342: winner-level trace (SFS_WIN_TRACE=1). Read-only in the same sense as 336-340: the
// winner's edge list is walked in the order the state holds it, nothing is sorted or reordered,
// and every [SFS-342] line is guarded by this flag. Off (the default) the commit point executes
// exactly the statements it executed before.
bool g_sfs_win_trace = false;
// TICKET 341: plausibility-gate veto trace (SFS_VETO_TRACE=1). Read-only in the same sense as
// 336-342: every [SFS-341] line is guarded by this flag, and nothing here writes, sorts or reorders
// a container. Off (the default) the gate executes exactly the statements it executed before.
bool g_sfs_veto_trace = false;
// TICKET 341: the largest profile-point distance ProfileChecking saw in the bin that failed, written
// only while SFS_VETO_TRACE=1 (same pattern as g_sfs339_pc_reason). -1 means no bin failed: the profile
// was empty or matched and something else (the rim margin) refused.
double g_sfs341_pc_dist = -1.0;
// TICKET 345: step-0 shape dry run (SFS_STEP0_TRACE=1). Read-only in the same sense as 336-342: the
// gate's profile-and-rim block is reproduced for an edge the gate is asked about at step 0 (that block
// is skipped there), the three numbers are printed and discarded, and every [SFS-345] line is guarded
// by this flag. Off (the default) the gate executes exactly the statements it executed before.
bool g_sfs_step0_trace = false;
static bool g_sfs_switches_read = false;

static bool ReadSfsSwitch(const char* name, bool& bad)
{
	const char* raw = std::getenv(name);
	bad = false;
	if (raw == NULL || *raw == '\0') return false;
	if (std::string(raw) == "1") return true;
	bad = true;
	return false;
}

void InitSfsSwitches()
{
	if (g_sfs_switches_read) return;
	g_sfs_switches_read = true;
	bool bad_cov = false, bad_sign = false;
	bool bad_trace = false;
	bool bad_st = false;
	bool bad_et = false;
	bool bad_at = false;
	bool bad_pt = false;
	bool bad_wt = false;
	bool bad_vt = false;
	bool bad_s0 = false;
	g_sfs_coverage = ReadSfsSwitch("SFS_SCORE_COVERAGE", bad_cov);
	g_sfs_axis_sign_requested = ReadSfsSwitch("SFS_AXIS_SIGN", bad_sign);
	g_sfs_stage_trace = ReadSfsSwitch("SFS_STAGE_TRACE", bad_trace);
	g_sfs_state_trace = ReadSfsSwitch("SFS_STATE_TRACE", bad_st);
	g_sfs_edge_trace = ReadSfsSwitch("SFS_EDGE_TRACE", bad_et);
	g_sfs_admit_trace = ReadSfsSwitch("SFS_ADMIT_TRACE", bad_at);
	g_sfs_pick_trace = ReadSfsSwitch("SFS_PICK_TRACE", bad_pt);
	g_sfs_win_trace = ReadSfsSwitch("SFS_WIN_TRACE", bad_wt);
	g_sfs_veto_trace = ReadSfsSwitch("SFS_VETO_TRACE", bad_vt);
	g_sfs_step0_trace = ReadSfsSwitch("SFS_STEP0_TRACE", bad_s0);
	// TICKET 339: no "[SFS-339] ... is not 1" warning on purpose -- with the switch off the run
	// must print no [SFS-339] line at all, and the banner below already says which state it is in.
	(void)bad_at;
	// TICKET 340: same reasoning as 339 -- with the switch off the run must print no [SFS-340] line
	// at all, and the banner below says which state it is in.
	(void)bad_pt;
	// TICKET 342: same reasoning as 339/340 -- with the switch off the run must print no
	// [SFS-342] line at all, and the banner below says which state it is in.
	(void)bad_wt;
	// TICKET 341: same reasoning as 339/340/342 -- with the switch off the run must print no
	// [SFS-341] line at all, and the banner below says which state it is in.
	(void)bad_vt;
	// TICKET 345: same reasoning as 339/340/342/341 -- with the switch off the run must print no
	// [SFS-345] line at all, and the banner below says which state it is in.
	(void)bad_s0;
	if (bad_cov) cout << "[SFS-335] SFS_SCORE_COVERAGE is not 1; using legacy score" << endl;
	if (bad_sign) cout << "[SFS-335] SFS_AXIS_SIGN is not 1; using legacy folded axis test" << endl;
	if (bad_trace) cout << "[SFS-336] SFS_STAGE_TRACE is not 1; stage trace off" << endl;
	if (bad_st) cout << "[SFS-337] SFS_STATE_TRACE is not 1; state trace off" << endl;
	if (bad_et) cout << "[SFS-338] SFS_EDGE_TRACE is not 1; edge trace off" << endl;
	g_sfs_axis_sign = false;   // only SignAxisGuard() may switch this on
	g_sfs_basket_erased_15 = 0; g_sfs_cand_erased_15 = 0; g_sfs_cand_erased_10 = 0;
	cout << "coverage score: " << (g_sfs_coverage ? "coverage" : "legacy") << endl;
	cout << "axis sign: " << (g_sfs_axis_sign_requested ? "signed (pending sign guard)" : "legacy") << endl;
	cout << "stage trace: " << (g_sfs_stage_trace ? "on" : "off") << endl;
	cout << "state trace: " << (g_sfs_state_trace ? "on" : "off") << endl;
	cout << "edge trace: " << (g_sfs_edge_trace ? "on" : "off") << endl;
	cout << "admit trace: " << (g_sfs_admit_trace ? "on" : "off") << endl;
	cout << "pick trace: " << (g_sfs_pick_trace ? "on" : "off") << endl;
	cout << "win trace: " << (g_sfs_win_trace ? "on" : "off") << endl;
	cout << "veto trace: " << (g_sfs_veto_trace ? "on" : "off") << endl;
	cout << "step0 trace: " << (g_sfs_step0_trace ? "on" : "off") << endl;
}

// TICKET 336: the one matrix writer. Twelve 3x4 numbers, tab-separated, no newline, so a
// caller can put its own fields before and an endl after. Read-only (Trans::Output is const).
void SfsTraceMat(const Trans& T)
{
	Matrix4d M;
	T.Output(M);
	for (int r = 0; r < 3; r++)
		for (int c = 0; c < 4; c++)
			cout << "\t" << M(r, c);
}

// TICKET 335 (amended): the ONLY consumer of the coverage term. It ranks two survivors that
// compete for the same slot; it never reaches a gate. With the switch off it returns score_
// unchanged, so the unset path is arithmetically identical to the shipped code.
static double RankScore(const LCSIndex& c)
{
	if (!g_sfs_coverage) return c.score_;
	double cov = c.coverage_;
	if (!(cov > 0.0)) cov = 0.0;
	return c.score_ / max(cov, 0.05);
}

//############################## Other function ##############################//
void NoOverrange(vector<Geom>& shard, LCSIndex& reout)
{
	if (reout.start_.x < 1) {
		int plus = 1 - reout.start_.x;
		reout.start_.x += plus;
		reout.start_.y += plus;
	}

	if (reout.start_.y < 1) {
		int plus = 1 - reout.start_.y;
		reout.start_.x += plus;
		reout.start_.y += plus;
	}

	int num_x = shard[reout.shard_x_ - 1].edge_line_.point_.cols() - 1;
	int num_y = shard[reout.shard_y_ - 1].edge_line_.point_.cols() - 1;

	if (reout.end_.x > num_x - 1) {
		int minus = reout.end_.x - num_x;
		reout.end_.x -= minus;
		reout.end_.y -= minus;
	}

	if (reout.end_.y > num_y - 1) {
		int minus = reout.end_.y - num_y;
		reout.end_.x -= minus;
		reout.end_.y -= minus;
	}

	reout.size_ = reout.end_.x - reout.start_.x + 1;
	if (reout.size_ != (reout.end_.y - reout.start_.y + 1)) cout << "function : no_overrange have claculation error " << endl;
}

void NoOverrange(vector<BreakLine>& breakline, LCSIndex& reout)
{
	if (reout.start_.x < 1) {
		int plus = 1 - reout.start_.x;
		reout.start_.x += plus;
		reout.start_.y += plus;
	}

	if (reout.start_.y < 1) {
		int plus = 1 - reout.start_.y;
		reout.start_.x += plus;
		reout.start_.y += plus;
	}

	int num_x = breakline[reout.shard_x_ - 1].point_.cols() - 1;
	int num_y = breakline[reout.shard_y_ - 1].point_.cols() - 1;

	if (reout.end_.x > num_x) {
		int minus = reout.end_.x - num_x;
		reout.end_.x -= minus;
		reout.end_.y -= minus;
	}

	if (reout.end_.y > num_y) {
		int minus = reout.end_.y - num_y;
		reout.end_.x -= minus;
		reout.end_.y -= minus;
	}

	reout.size_ = reout.end_.x - reout.start_.x + 1;
	if (reout.size_ != (reout.end_.y - reout.start_.y + 1)) cout << "function : no_overrange have calculation error " << endl;
}

MatrixXd Quantization(MatrixXd& a, vector<double>& stepsize)
{
	int ncol = a.cols();
	int nrow = a.rows();

	if (a.rows() > 3) {
		nrow = 4;
	}

	MatrixXd Q_feature(nrow, ncol);

	for (int i = 0; i < ncol; i++) {
		for (int j = 0; j < nrow; j++) {
			if (j == 3) {
				if (a(6, i) > 0) {
					Q_feature(j, i) = [](double x, double y)->double {return x - fmod(x, y); }(a(6, i), stepsize[j]);
				}
				else Q_feature(j, i) = a(6, i);
			}
			else {
				Q_feature(j, i) = [](double x, double y)->double {return x - fmod(x, y); }(a(j, i), stepsize[j]);
			}
		}
	}

	return Q_feature;
}

void LCSPointAverage(LCSpoint& cluster, LCSpoint& point, int num, bool rough)
{
	if (rough) {
		cluster.x = (cluster.x + point.x) / 2;
		cluster.y = (cluster.y + point.y) / 2;
	}
	else {
		int t_x = cluster.x * num + point.x;
		int t_y = cluster.y * num + point.y;

		cluster.x = (int)(t_x / (num + 1));
		cluster.y = (int)(t_y / (num + 1));
	}	
}

void LCSPointAverage(list<LCSIndex>& cls, LCSpoint &middle)
{
	int num = cls.size();
	middle.x = middle.y = 0;
	list<LCSIndex>::iterator iter;
	for (iter = cls.begin(); iter != cls.end(); ++iter) {
		middle.x += (iter->start_.x + iter->end_.x) / 2;
		middle.y += (iter->start_.y + iter->end_.y) / 2;
	}
	middle.x = (int)(middle.x / num);
	middle.y = (int)(middle.y / num);
}

// Update
LCSIndex Representative(list<LCSIndex>& cls, LCSpoint& middle) 
{
	LCSpoint I_c;	// Center of interval
	double cost(0);
	double m_cost(0);
	LCSIndex out;

	list<LCSIndex>::iterator iter;
	for (iter = cls.begin(); iter != cls.end(); ++iter) {
		I_c.x = (iter->start_.x + iter->end_.x) / 2;
		I_c.y = (iter->start_.y + iter->end_.y) / 2;

		cost = (I_c - middle) / 2*(iter->size_);

		if (iter == cls.begin()) {
			m_cost = cost;
			out = *iter;
		}

		else if (m_cost > cost) {
			m_cost = cost;
			out = *iter;
		}
	}

	return out;
}


list<LCSIndex> Clustering(list<LCSIndex>& c_result, int cls_threshold)
{
	vector<list<LCSIndex>> cls_basket;
	list<LCSIndex> basket_temp;
	vector<LCSpoint> cls_Index;	 // Average value of clsuster
	vector<int> cls_num;
	LCSpoint cls_temp;

	//################# Clustering #################//
	list<LCSIndex>::iterator iter = c_result.begin();
	for (; iter != c_result.end(); ++iter) {
		cls_temp.x = (iter->start_.x + iter->end_.x) / 2;
		cls_temp.y = (iter->start_.y + iter->end_.y) / 2;

		for (int i = 0; i < cls_Index.size(); i++) {
			if (cls_temp - cls_Index[i] < cls_threshold) {
				iter->cluster_ = i + 1;
				LCSPointAverage(cls_Index[i], cls_temp, cls_num[i], false);
				cls_num[i] += 1;
			}
		}

		if (iter == c_result.begin()) {
			iter->cluster_ = 1;
			cls_Index.push_back(cls_temp);
			cls_num.push_back(0);
		}

		else if (iter->cluster_ == 0) {
			iter->cluster_ = cls_Index.size() + 1;
			cls_Index.push_back(cls_temp);
			cls_num.push_back(0);
		}

	}

	int numOfcls = cls_Index.size();
	cls_basket.resize(numOfcls);
	c_result.sort();
	for (iter = c_result.begin(); iter != c_result.end(); ++iter) {
		cls_basket[iter->cluster_ - 1].push_back(*iter);
	}

	list<LCSIndex> out;
	for (int i = 0; i < cls_basket.size(); i++) {
		LCSpoint middle;
		LCSIndex rep;
		LCSPointAverage(cls_basket[i], middle);
		rep = Representative(cls_basket[i], middle);
		out.push_back(rep);
	}

	return out;
}

list<LCSIndex> LCS(BreakLine& X,
	BreakLine& Y,
	vector<double>& Q_size,
	int windowsize,
	int axis_x,
	int axis_y)
{   // Create a table to store lengths of longest 
	// common suffixes of substrings.   Note that 
	// LCSuff[i][j] contains length of longest 
	// common suffix of X[0..i-1] and Y[0..j-1].  

	MatrixXd Q_x = Quantization(X.feature_[axis_x], Q_size);
	MatrixXd Q_y = Quantization(Y.feature_[axis_y], Q_size);
	int Rim_aS(0), Rim_aE(0), Rim_bS(0), Rim_bE(0);
	if (X.is_seg_rim_) {
		for (int i = 0; i < X.index_.cols(); i++) {
			if (X.index_(2, i)) {
				Rim_bS = X.index_(0, i);
				Rim_bE = X.index_(1, i);
			}
		}
	}

	if (Y.is_seg_rim_) {
		for (int i = 0; i < Y.index_.cols(); i++) {
			if (Y.index_(2, i)) {
				Rim_aS = Y.index_(0, i);
				Rim_aE = Y.index_(1, i);
			}
		}
	}

	list<LCSIndex> Out;
	list<LCSIndex>::iterator iter;
	int n = Q_x.cols();
	int m = Q_y.cols();

	MatrixXi LCSuff(m + 1, n + 1);
	LCSIndex temp;

	/* Following steps build LCSuff[m+1][n+1] in
		bottom up fashion. */
	for (int i = 0; i <= m; i++)
	{
		for (int j = 0; j <= n; j++)
		{
			double Compare[5] = { 0, 0, 0, 0, 0 };
			bool Cond1, Cond2, Cond3, Cond4;

			Cond1 = false;	// Dist condition
			Cond2 = false;	// Height condition
			Cond3 = false;	// Theta condition
			Cond4 = true;	// Thickness condition

			if (i != 0 && j != 0) {
				Compare[0] = abs(Q_x(0, j - 1) - Q_y(0, i - 1));
				Compare[1] = abs(Q_x(1, j - 1) - Q_y(1, i - 1));
				Compare[2] = abs(Q_x(2, j - 1) - Q_y(2, i - 1));
				Compare[3] = abs(Q_x(1, j - 1) + Q_y(1, i - 1));	// Change direction of axis
				Compare[4] = abs(Q_x(2, j - 1) + Q_y(2, i - 1));	// Change direction of axis

				Cond1 = Compare[0] < 3 * Q_size[0];
				Cond2 = (Compare[1] < 2.5 * Q_size[1]) || (Compare[3] < 2.5 * Q_size[1]);
				Cond3 = (Compare[2] < 2.5 * Q_size[2]) || (Compare[4] < 2.5 * Q_size[2]);

				if (X.feature_[axis_x].rows() > 3) {
					if ((Q_x(3, j - 1) > 0) && (Q_y(3, i - 1) > 0)) {
						Cond4 = abs(Q_x(3, j - 1) - Q_y(3, i - 1)) <= 4 * Q_size[3]; 
					}
				}
			}

			bool a_cdt = (Rim_aS < i - 1) & (i - 1 < Rim_aE);
			bool b_cdt = (Rim_bS < j - 1) & (j - 1 < Rim_bE);
			if (i == 0 || j == 0)
				LCSuff(i, j) = 0;
			else if (a_cdt || b_cdt)
				LCSuff(i, j) = 0;

			else if (Cond1 && Cond2 && Cond3 && Cond4)
			{
				LCSuff(i, j) = LCSuff(i - 1, j - 1) + 1;
				if (LCSuff(i, j) > windowsize) {
					temp.size_ = LCSuff(i, j);
					temp.start_.x = j - temp.size_ + 1;
					temp.end_.x = j;
					temp.start_.y = i - temp.size_ + 1;
					temp.end_.y = i;

					Out.push_back(temp);

					for (iter = Out.begin(); iter != Out.end(); ) {
						if (*iter == (temp - 1)) {
							iter = Out.erase(iter);
						}
						else iter++;
					}
				}						
			}
			else LCSuff(i, j) = 0;
		}
	}

	list<LCSIndex> pick_out;

	if (Out.size() > 0) {
		pick_out = Clustering(Out, 20);
	}

	return pick_out;
}

list<LCSIndex> LCSGraphBuilding(BreakLine& X,
	BreakLine& Y,
	vector<double>& Q_size,
	int windowsize,
	int axis_x,
	int axis_y,
	vector<vector<bool>>& matched_index,
	const int index_X,
	const int index_Y)
{
	int num_X_points = X.point_.cols();	// X is inverted
	MatrixXd Q_x = Quantization(X.feature_[axis_x], Q_size);
	MatrixXd Q_y = Quantization(Y.feature_[axis_y], Q_size);
	int Rim_aS(0), Rim_aE(0), Rim_bS(0), Rim_bE(0);
	if (X.is_seg_rim_) {
		for (int i = 0; i < X.index_.cols(); i++) {
			if (X.index_(2, i)) {
				Rim_bS = X.index_(0, i);
				Rim_bE = X.index_(1, i);
			}
		}
	}

	if (Y.is_seg_rim_) {
		for (int i = 0; i < Y.index_.cols(); i++) {
			if (Y.index_(2, i)) {
				Rim_aS = Y.index_(0, i);
				Rim_aE = Y.index_(1, i);
			}
		}
	}

	list<LCSIndex> Out;
	list<LCSIndex>::iterator iter;
	int n = Q_x.cols();
	int m = Q_y.cols();

	MatrixXi LCSuff(m + 1, n + 1);
	LCSIndex temp;

	/* Following steps build LCSuff[m+1][n+1] in
		bottom up fashion. */
	for (int i = 0; i <= m; i++)
	{
		for (int j = 0; j <= n; j++)
		{
			double Compare[5] = { 0, 0, 0, 0, 0 };
			bool Cond1, Cond2, Cond3, Cond4;

			Cond1 = false;	// Dist condition
			Cond2 = false;	// Height condition
			Cond3 = false;	// Theta condition
			Cond4 = true;	// Thickness condition

			if (i != 0 && j != 0) {
				Compare[0] = abs(Q_x(0, j - 1) - Q_y(0, i - 1));
				Compare[1] = abs(Q_x(1, j - 1) - Q_y(1, i - 1));
				Compare[2] = abs(Q_x(2, j - 1) - Q_y(2, i - 1));
				Compare[3] = abs(Q_x(1, j - 1) + Q_y(1, i - 1));	// Change direction of axis
				Compare[4] = abs(Q_x(2, j - 1) + Q_y(2, i - 1));	// Change direction of axis

				Cond1 = Compare[0] < 3 * Q_size[0];
				Cond2 = (Compare[1] < 2.5 * Q_size[1]) || (Compare[3] < 2.5 * Q_size[1]);
				Cond3 = (Compare[2] < 2.5 * Q_size[2]) || (Compare[4] < 2.5 * Q_size[2]);

				if (X.feature_[axis_x].rows() > 3) {
					if ((Q_x(3, j - 1) > 0) && (Q_y(3, i - 1) > 0)) {
						Cond4 = abs(Q_x(3, j - 1) - Q_y(3, i - 1)) <= 4 * Q_size[3];
					}
				}
			}

			bool a_cdt = (Rim_aS < i - 1) & (i - 1 < Rim_aE);
			bool b_cdt = (Rim_bS < j - 1) & (j - 1 < Rim_bE);
			if (i == 0 || j == 0)
				LCSuff(i, j) = 0;
			else if (a_cdt || b_cdt)
				LCSuff(i, j) = 0;
			else if (matched_index[index_Y][i - 1] || matched_index[index_X][num_X_points - j])
				LCSuff(i, j) = 0;
			else if (Cond1 && Cond2 && Cond3 && Cond4)
			{
				LCSuff(i, j) = LCSuff(i - 1, j - 1) + 1;
				if (LCSuff(i, j) > windowsize) {
					temp.size_ = LCSuff(i, j);
					temp.start_.x = j - temp.size_ + 1;
					temp.end_.x = j;
					temp.start_.y = i - temp.size_ + 1;
					temp.end_.y = i;

					Out.push_back(temp);

					for (iter = Out.begin(); iter != Out.end(); ) {
						if (*iter == (temp - 1)) {
							iter = Out.erase(iter);
						}
						else iter++;
					}
				}						
			}
			else LCSuff(i, j) = 0;
		}
	}

	list<LCSIndex> pick_out;

	if (Out.size() > 0) {
		pick_out = Clustering(Out, 20);
	}

	return pick_out;
}

void MatrixRearrangeCCW(BreakLine& L0)
{
	int Decision(0);
	int num_points = L0.point_.cols();
	MatrixXd Dummy = L0.point_, Dummy_N = L0.normal_;
	bool direction = true;

	if (num_points < 4) {
		cout << "Function Matrix_Rearrange Error : Wrong matrix size" << endl;
		return;
	}

	Vector3d Center(3);
	double Dot(0);

	// Set z value as 0 for projecting to xy plane. 
	for (int i = 0; i < 3; i++) {
		Center(i) = Dummy.row(i).mean();
	}
	for (int i = 0; i < num_points - 1; i++) {
		Vector3d current, front;

		if (direction) {
			current = L0.point_.col(i) - Center;
			front = L0.point_.col(i + 1) - Center;
		}
		else {
			current = L0.point_.col(i - 1) - Center;
			front = L0.point_.col(i + 1) - Center;
		}
		current(2) = front(2) = 0;

		Vector3d Cross = current.cross(front);
		Cross = Cross / Cross.norm();

		if (Cross(2) == -1) {
			Decision++;
			direction = true;
		}

		else if (Cross(2) == 1) {
			direction = true;
		}

		else {
			direction = false;
		}
	}

	if (Decision > num_points / 2) {
		L0.ChangeOrder();
	}
}

MatrixXd CalculateLeastSquare(vector<Vector3d>& data, int& start, int& end)
{
	int size = end - start + 1;

	MatrixXd A(size, 2), B(size, 1);
	for (int i = start; i < end + 1; i++) {
		A(i - start, 0) = data[i](2);
		A(i - start, 1) = 1;
		B(i - start, 0) = data[i](0);
	}
	MatrixXd pinvA = A.completeOrthogonalDecomposition().pseudoInverse();
	MatrixXd coeff = pinvA * B;

	return coeff;
}

void MakeCorWOTree(vector<CorIndex>& cor, BreakLine& L0, BreakLine& L1, vector<int>& region)
{
	//#pragma omp parallel for
	for (int i = 0; i < L1.point_.cols(); i++) {
		double len(99999);
		int p_index = 0;
		for (int j = 0; j < L0.point_.cols(); j++) {
			double len_dummy = (L1.point_.col(i) - L0.point_.col(j)).norm();
			if (len_dummy < len) {
				len = len_dummy;
				p_index = j;
			}
		}
		cor[i].len = len;
		cor[i].p_a_ = p_index;
		cor[i].p_b_ = i;
		Vector3d p_ab = L1.point_.col(i) - L0.point_.col(p_index);
		p_ab = p_ab / p_ab.norm();
		double dir = abs(p_ab.dot(L1.normal_.col(i)));
		dir = dir / (p_ab.norm() * L1.normal_.col(i).norm());
		double norm_dir = L1.normal_.col(i).dot(L0.normal_.col(p_index));
		if (len < 5) {
			region.push_back(i);
		}
		else if ((norm_dir > 0) && (len < 20)) {
			if (dir >= 0.86) {				// cos30 = 0.86
				region.push_back(i);
			}
		}
	}
}

bool DetectIntersection(BreakLine& L0,
	BreakLine& L1,
	CorIndex& cor,
	vector<Vector3d>& pc)
{
	int num_L0 = L0.point_.cols();
	int num_L1 = L1.point_.cols();
	Vector3d p_a_1, p_a_2, p_b_1, p_b_2;
	Vector3d n_a, n_b;
	p_a_1 = L0.point_.col(cor.p_a_);
	n_a = L0.normal_.col(cor.p_a_);
	p_b_1 = L1.point_.col(cor.p_b_);
	n_b = L1.normal_.col(cor.p_b_);

	if (cor.p_a_ > num_L0 - 2) {
		p_a_2 = L0.point_.col((int)(cor.p_a_ - num_L0 + 1));
	}
	else {
		p_a_2 = L0.point_.col((int)(cor.p_a_ + 1));
	}
	if (cor.p_b_ > num_L1 - 2) {
		p_b_2 = L1.point_.col((int)(cor.p_b_ - num_L1 + 1));
	}
	else {
		p_b_2 = L1.point_.col((int)(cor.p_b_ + 1));
	}

	if ((p_a_1 == p_a_2) || (p_b_1 == p_b_2)) {
		return false;
	}

	// Calculate half plane
	Vector3d hp_a = n_a.cross((p_a_2 - p_a_1));
	Vector3d hp_b = n_b.cross((p_b_2 - p_b_1));
	Vector3d c_ba = p_a_1 - p_b_1;
	double intru_a(0), intru_b(0);
	intru_a = hp_a.dot(c_ba);
	intru_b = hp_b.dot(c_ba);

	if (hp_a.dot(hp_b) > 0) {
		if (cor.len > 4)
			return false;
		cor.len = 4;
		pc.push_back(p_a_1);
		pc.push_back(p_b_1);
		return true;
	}

	else if ((intru_a < 0) && (intru_b > 0)) {
		if (cor.len > 2)
		{
			pc.push_back(p_a_1);
			pc.push_back(p_b_1);
			return true;
		}
	}
	return false;
}

void SortRegion(vector<int>& region, const int& num)
{
	for (int i = 0; i < region.size() - 1; i++) {
		if (region[i + 1] - region[i] > num / 2) {
			for (int j = 0; j < i + 1; j++) {
				region[j] = region[j] + num;
			}
			break;
		}
	}
	sort(region.begin(), region.end());
}

bool OverlapCheck_3d(BreakLine& L0,
	BreakLine& L1,
	double& area,
	double& size,
	double threshold,
	vector<Vector3d>& out)
{
	Vector3d center_L0 = { L0.point_.row(0).mean(), L0.point_.row(1).mean(), L0.point_.row(2).mean() };
	Vector3d center_L1 = { L1.point_.row(0).mean(), L1.point_.row(1).mean(), L1.point_.row(2).mean() };

	double angle = center_L0.dot(center_L1) / (center_L0.norm() * center_L1.norm());

	if ((center_L0 - center_L1).norm() < 5) {
		cout << "Overlap : Each center is too close" << endl;
		area = 1000;
		size = 1;
		return true;
	}

	vector<CorIndex> cor_a, cor_b;
	int overlap_counter(0);
	double overlap_area(0);
	int num_points_L0 = L0.point_.cols();
	int num_points_L1 = L1.point_.cols();
	cor_a.resize(num_points_L0);
	cor_b.resize(num_points_L1);
	vector<int> region_a, region_b;

	MakeCorWOTree(cor_a, L1, L0, region_a);
	MakeCorWOTree(cor_b, L0, L1, region_b);

	if ((region_a.size() < 10) || (region_b.size() < 10)) {
		return false;
	}

	SortRegion(region_a, num_points_L0);
	SortRegion(region_b, num_points_L1);

	int start_a = region_a.front(), end_a = region_a.back();
	int start_b = region_b.front(), end_b = region_b.back();
	int num_points_L1_i = L0.point_.cols();
	for (int i = start_b; i <= end_b; i++) {
		Vector3d p_a_1, p_a_2, p_b_1, p_b_2;
		Vector3d n_a, n_b;
		int range = i;
		if (range > num_points_L1 - 1) {
			range = range - num_points_L1;
		}
		bool isIntersect = DetectIntersection(L0, L1, cor_b[range], out);
		if (isIntersect) {
			overlap_counter++;
			overlap_area += cor_b[range].len;
		}
	}
	for (int i = start_a; i <= end_a; i++) {
		Vector3d p_a_1, p_a_2, p_b_1, p_b_2;
		Vector3d n_a, n_b;
		int range = i;
		if (range > num_points_L0 - 1) {
			range = range - num_points_L0;
		}
		bool isIntersect = DetectIntersection(L1, L0, cor_a[range], out);
		if (isIntersect) {
			overlap_counter++;
			overlap_area += cor_a[range].len;
		}
	}

	size = overlap_counter / 2;
	area = overlap_area / 2;

	if (area > threshold) {
		return true;
	}
	return false;
}

bool OverlapCheck_3d(BreakLine& L0,
	BreakLine& L1,
	double& area,
	double& size,
	double threshold)
{
	Vector3d center_L0 = { L0.point_.row(0).mean(), L0.point_.row(1).mean(), L0.point_.row(2).mean() };
	Vector3d center_L1 = { L1.point_.row(0).mean(), L1.point_.row(1).mean(), L1.point_.row(2).mean() };

	double angle = center_L0.dot(center_L1) / (center_L0.norm() * center_L1.norm());

	if ((center_L0 - center_L1).norm() < 5) {
		area = 1000;
		size = 1;
		return true;
	}

	vector<Vector3d> pc_dummy;
	vector<CorIndex> cor_a, cor_b;
	int overlap_counter(0);
	double overlap_area(0);
	int num_points_L0 = L0.point_.cols();
	int num_points_L1 = L1.point_.cols();
	cor_a.resize(num_points_L0);
	cor_b.resize(num_points_L1);
	vector<int> region_a, region_b;

	MakeCorWOTree(cor_a, L1, L0, region_a);
	MakeCorWOTree(cor_b, L0, L1, region_b);

	if ((region_a.size() < 5) || (region_b.size() < 5)) {
		return false;
	}

	SortRegion(region_a, num_points_L0);
	SortRegion(region_b, num_points_L1);

	int start_a = region_a.front(), end_a = region_a.back();
	int start_b = region_b.front(), end_b = region_b.back();
	int num_points_L1_i = L0.point_.cols();
	for (int i = start_b; i <= end_b; i++) {
		Vector3d p_a_1, p_a_2, p_b_1, p_b_2;
		Vector3d n_a, n_b;
		int range = i;
		if (range > num_points_L1 - 1) {
			range = range - num_points_L1;
		}
		bool isIntersect = DetectIntersection(L0, L1, cor_b[range], pc_dummy);
		if (isIntersect) {
			overlap_counter++;
			overlap_area += cor_b[range].len;
		}
	}
	for (int i = start_a; i <= end_a; i++) {
		Vector3d p_a_1, p_a_2, p_b_1, p_b_2;
		Vector3d n_a, n_b;
		int range = i;
		if (range > num_points_L0 - 1) {
			range = range - num_points_L0;
		}
		bool isIntersect = DetectIntersection(L1, L0, cor_a[range], pc_dummy);
		if (isIntersect) {
			overlap_counter++;
			overlap_area += cor_a[range].len;
		}
	}

	size = overlap_counter / 2;
	area = overlap_area / 2;

	if (area > threshold) {
		return true;
	}
	return false;
}


bool OverlapCheck_3d(BreakLine& L0, 
	BreakLine& L1,
	double& area, 
	double& size, 
	double threshold, 
	MatrixXd& out)
{
	Vector3d center_L0 = { L0.point_.row(0).mean(), L0.point_.row(1).mean(), L0.point_.row(2).mean() };
	Vector3d center_L1 = { L1.point_.row(0).mean(), L1.point_.row(1).mean(), L1.point_.row(2).mean() };

	double angle = center_L0.dot(center_L1) / (center_L0.norm() * center_L1.norm());

	if ((center_L0 - center_L1).norm() < 5) {
		cout << "Overlap : Each center is too close" << endl;
		area = 1000;
		size = 1;
		return true;
	}

	vector<Vector3d> pc_dummy;
	vector<CorIndex> cor_a, cor_b;
	int overlap_counter(0);
	double overlap_area(0);
	int num_points_L0 = L0.point_.cols();
	int num_points_L1 = L1.point_.cols();
	cor_a.resize(num_points_L0);
	cor_b.resize(num_points_L1);
	vector<int> region_a, region_b;

	MakeCorWOTree(cor_a, L1, L0, region_a);
	MakeCorWOTree(cor_b, L0, L1, region_b);

	if ((region_a.size() < 10) || (region_b.size() < 10)) {
		return false;
	}

	SortRegion(region_a, num_points_L0);
	SortRegion(region_b, num_points_L1);

	int start_a = region_a.front(), end_a = region_a.back();
	int start_b = region_b.front(), end_b = region_b.back();
	int num_points_L1_i = L0.point_.cols();

	for (int i = start_b; i <= end_b; i++) {
		Vector3d p_a_1, p_a_2, p_b_1, p_b_2;
		Vector3d n_a, n_b;
		int range = i;
		if (range > num_points_L1 - 1) {
			range = range - num_points_L1;
		}
		bool isIntersect = DetectIntersection(L0, L1, cor_b[range], pc_dummy);
		if (isIntersect) {
			overlap_counter++;
			overlap_area += cor_b[range].len;
		}
	}

	for (int i = start_a; i <= end_a; i++) {
		Vector3d p_a_1, p_a_2, p_b_1, p_b_2;
		Vector3d n_a, n_b;
		int range = i;
		if (range > num_points_L0 - 1) {
			range = range - num_points_L0;
		}
		bool isIntersect = DetectIntersection(L1, L0, cor_a[range], pc_dummy);
		if (isIntersect) {
			overlap_counter++;
			overlap_area += cor_a[range].len;
		}
	}

	size = overlap_counter / 2;
	area = overlap_area / 2;

	out.resize(3, pc_dummy.size());
	for (int i = 0; i < pc_dummy.size(); i++) {
		out.col(i) = pc_dummy[i];
	}

	if (area > threshold) {
		return true;
	}
	return false;
}

bool ProfileChecking(vector<Vector3d>& profile, double bin_size, double threshold)
{
	bool out = true;
	if (g_sfs_veto_trace) g_sfs341_pc_dist = -1.0;	// TICKET 341: no failing bin yet
	// Fist, sort profile curve based on height
	sort(profile.begin(), profile.end(), [](Vector3d a, Vector3d b) -> bool {
		return a(2) < b(2);
	});

	MinMaxValue z_total;
	z_total.max_value = profile.back()(2);
	z_total.min_value = profile.front()(2);

	int Q_size = floor((z_total.max_value - z_total.min_value) / bin_size) + 1;
	int s_index(0);
	for (int i = 0; i < Q_size; i++) {
		MinMaxValue bin;
		bin.min_value = bin_size * i + z_total.min_value;
		bin.max_value = bin_size * (i + 1) + z_total.min_value;
		
		int start = s_index;
		for (int j = s_index; j < profile.size(); j++) {
			if (profile[j](2) > bin.max_value) {
				break;
			}
			else if (profile[j](2) < bin.min_value) {
				cout << "ProfileChecking : Error - out of scope" << endl;
				break;
			}
			else {
				s_index++;
			}
		}
		int end = s_index - 1;
		
		double dist(0);
		MatrixXd coeff = CalculateLeastSquare(profile, start, end);
		double den = sqrt(coeff(0) * coeff(0) + 1);
		for (int j = start; j < end; j++) {
			if (abs(coeff(0)) > 1) {
				dist = abs((coeff(0) * profile[j](2) - profile[j](0) + coeff(1)) / den);
			}
			else {
				dist = abs(profile[j](0) - (coeff(0) * profile[j](2) + coeff(1)));
			}
			if (dist > threshold) {
				// TICKET 341: every earlier point in this bin was <= threshold, so this distance is the
				// largest the failing bin saw; the bar is `threshold` (the gate passes 7.0).
				if (g_sfs_veto_trace) g_sfs341_pc_dist = dist;
				out = false;
				break;
			}
		}
		if (!out) {
			break;
		}
	}

	return out;
}

// TICKET 345: read-only peer of ProfileChecking for the step-0 dry run. It walks the same bins with the
// same least-squares fit and the same comparison, on a COPY of the profile (so the caller's vector is
// never sorted or touched), and it writes nothing -- in particular not g_sfs341_pc_dist, which belongs to
// the veto trace. Returns the first point distance over `threshold` in the first failing bin (the number
// [SFS-341] prints as value_a); when the profile passes, returns the largest point distance it saw, which
// is always <= threshold, so a passing row is not mistaken for an unmeasured one; -1 when the profile is
// empty or no bin could be measured.
double ProfileCheckingDistance(vector<Vector3d> profile, double bin_size, double threshold)
{
	if (profile.empty()) return -1.0;
	sort(profile.begin(), profile.end(), [](Vector3d a, Vector3d b) -> bool {
		return a(2) < b(2);
	});
	double max_dist = -1.0;
	double z_max = profile.back()(2);
	double z_min = profile.front()(2);
	int Q_size = floor((z_max - z_min) / bin_size) + 1;
	int s_index(0);
	for (int i = 0; i < Q_size; i++) {
		double bin_min = bin_size * i + z_min;
		double bin_max = bin_size * (i + 1) + z_min;
		int start = s_index;
		for (int j = s_index; j < (int)profile.size(); j++) {
			if (profile[j](2) > bin_max) break;
			else if (profile[j](2) < bin_min) break;
			else s_index++;
		}
		int end = s_index - 1;
		MatrixXd coeff = CalculateLeastSquare(profile, start, end);
		double den = sqrt(coeff(0) * coeff(0) + 1);
		for (int j = start; j < end; j++) {
			double dist(0);
			if (abs(coeff(0)) > 1) {
				dist = abs((coeff(0) * profile[j](2) - profile[j](0) + coeff(1)) / den);
			}
			else {
				dist = abs(profile[j](0) - (coeff(0) * profile[j](2) + coeff(1)));
			}
			if (dist > max_dist) max_dist = dist;
			if (dist > threshold) return dist;
		}
	}
	return max_dist;
}

bool ProfileCheckingWithInlier(vector<Vector3d>& profile,
	double bin_size,
	double threshold,
	double th_inlier,
	int& inlier)
{
	int pc_inlier(0), pc_base(0);
	bool out = true;
	// Fist, sort profile curve based on height
	sort(profile.begin(), profile.end(), [](Vector3d a, Vector3d b) -> bool {
		return a(2) < b(2);
		});

	MinMaxValue z_total;
	z_total.max_value = profile.back()(2);
	z_total.min_value = profile.front()(2);

	int Q_size = floor((z_total.max_value - z_total.min_value) / bin_size) + 1;
	int s_index(0);
	for (int i = 0; i < Q_size; i++) {
		MinMaxValue bin;
		bin.min_value = bin_size * i + z_total.min_value;
		bin.max_value = bin_size * (i + 1) + z_total.min_value;

		int start = s_index;
		for (int j = s_index; j < profile.size(); j++) {
			if (profile[j](2) > bin.max_value) {
				break;
			}
			else if (profile[j](2) < bin.min_value) {
				cout << "ProfileChecking : Error - out of scope" << endl;
				break;
			}
			else {
				s_index++;
			}
		}
		int end = s_index - 1;

		double dist(0);
		MatrixXd coeff = CalculateLeastSquare(profile, start, end);
		double den = sqrt(coeff(0) * coeff(0) + 1);
		for (int j = start; j < end; j++) {
			if (abs(coeff(0)) > 1) {
				dist = abs((coeff(0) * profile[j](2) - profile[j](0) + coeff(1)) / den);
			}
			else {
				dist = abs(profile[j](0) - (coeff(0) * profile[j](2) + coeff(1)));
			}
			if (dist > threshold) {
				out = false;
				break;
			}
			else if (dist < th_inlier) {
				pc_inlier++;
			}

			pc_base++;
		}
		if (!out) {
			break;
		}
	}

	if (out) {
		double weight = pow(((double)pc_inlier / (double)pc_base), 2) * 0.2 + 1;
		inlier = (int)(weight * (double)inlier);
	}

	return out;
}

void MatchingScore(double& score, const vector<Corres>& COR)
{
	score = 0;
	double dist(0), norm(0);
	int num = COR.size(), count = 0;
	for (int i = 0; i < num; i++) {
		int size = COR[i].cor.size();
		count += size;
		//if (size > MINIMUM_NUMBER) 
		{
			for (int j = 0; j < size; j++) {
				dist += (COR[i].cor[j].p_A - COR[i].cor[j].p_B).norm();
				norm += 1 - COR[i].cor[j].n_A.dot(COR[i].cor[j].n_B);
			}
		}

	}
	double w_c = pow((double)count, 1.2);
	score = (dist + norm) / w_c;
}

void CountInlier(int& inlier, const vector<Corres>& COR, double threshold, double angle_th)
{
	inlier = 0;
	double dist(0), angle(0);
	int num = COR.size();

	for (int i = 0; i < num; i++) {
		int size = COR[i].cor.size();
		int dummy_inlier(0);
		for (int j = 0; j < size; j++) {
			dist = (COR[i].cor[j].p_A - COR[i].cor[j].p_B).norm();
			angle = acos(COR[i].cor[j].n_A.dot(COR[i].cor[j].n_B));
			angle = min(angle, 3.14159 - angle);
			if (dist < threshold && angle < angle_th) dummy_inlier++;
		}
		if (dummy_inlier > MINIMUM_NUMBER / 2)
		{
			inlier += dummy_inlier;
		}
	}
}

void CountInlier(int& inlier, 
	const vector<Corres>& COR,
	int num_shard,
	double threshold, 
	double angle_th)
{
	inlier = 0;
	double dist(0), angle(0);
	int num = COR.size();
	vector<int> shard_inlier(num_shard, 0);
	vector<int> weight(num_shard, 0);

	for (int i = 0; i < num; i++) {
		int size = COR[i].cor.size();
		int dummy_inlier(0);
		int s_x = COR[i].index_A - 1;
		int s_y = COR[i].index_B - 1;
		for (int j = 0; j < size; j++) {
			dist = (COR[i].cor[j].p_A - COR[i].cor[j].p_B).norm();
			angle = acos(COR[i].cor[j].n_A.dot(COR[i].cor[j].n_B));
			angle = min(angle, 3.14159 - angle);
			if (dist < threshold && angle < angle_th) dummy_inlier++;
		}
		if (dummy_inlier > MINIMUM_NUMBER / 2) 
		{
			shard_inlier[s_x] += dummy_inlier;
			shard_inlier[s_y] += dummy_inlier;
			weight[s_x] = weight[s_x] + 1;
			weight[s_y] = weight[s_y] + 1;
		}
	}

	for (int i = 0; i < num_shard; i++) {
		inlier += (int)(((double)weight[i] * 0.2 + 1) * (double)shard_inlier[i]);	//0.2
	}
}

void CountInlier(int& inlier,
	const vector<Corres>& COR,
	const vector<double>& num_points,
	double threshold,
	double angle_th)
{
	inlier = 0;
	double dist(0), angle(0);
	int num = COR.size();
	int num_shard = num_points.size();
	vector<int> shard_inlier(num_shard, 0);
	vector<double> weight(num_shard, 0);

	for (int i = 0; i < num; i++) {
		int size = COR[i].cor.size();
		int dummy_inlier(0);
		int s_x = COR[i].index_A - 1;
		int s_y = COR[i].index_B - 1;
		for (int j = 0; j < size; j++) {
			dist = (COR[i].cor[j].p_A - COR[i].cor[j].p_B).norm();
			angle = acos(COR[i].cor[j].n_A.dot(COR[i].cor[j].n_B));
			angle = min(angle, 3.14159 - angle);
			if (dist < threshold && angle < angle_th) dummy_inlier++;
		}

		{
			shard_inlier[s_x] += dummy_inlier;
			shard_inlier[s_y] += dummy_inlier;
		}
	}

	for (int i = 0; i < num_shard; i++) {
		if (num_points[i] == 0)
			continue;
		weight[i] = ((double)shard_inlier[i] / num_points[i]) * 0.2 + 1;
		inlier += (int)(weight[i] * (double)shard_inlier[i]);
	}

}


bool CountPCInlier(int& inlier,
	const vector<bool>& true_node,
	const vector<Geom>& shard,
	const int& current_node,
	double bin_size,
	double threshold,
	bool total_count)
{
	int num_shard = shard.size();
	bool result = true;
	bool is_rim = false;
	vector<Vector3d> rim_data;
	double H_rim(0), R_rim(0);

	//########### First build current profile curve
	vector<Vector3d> pc;
	vector<Vector3d> pc_current;
	for (int i = 0; i < num_shard; i++) {
		if (true_node[i] && (i != current_node - 1)) {
			ToCylindricalInterpolation(shard[i].edge_line_, pc, false);
			// For rim height
			if (shard[i].edge_line_.is_seg_rim_) {
				is_rim = true;
				for (int j = 0; j < shard[i].edge_line_.index_.cols(); j++) {
					if (shard[i].edge_line_.index_(2, j)) {
						int start = shard[i].edge_line_.index_(0, j);
						int end = shard[i].edge_line_.index_(1, j);
						for (int r_c = start - 1; r_c < end; r_c++) {
							rim_data.push_back(shard[i].edge_line_.point_.col(r_c));
						}
					}
					
				}
			}
		}
		else if (total_count && (i == current_node - 1)) {
			ToCylindricalInterpolation(shard[i].edge_line_, pc, false);
		}
	}
	if (pc.empty()) return result;

	sort(pc.begin(), pc.end(), [](Vector3d a, Vector3d b) -> bool {
		return a(2) < b(2);
		});

	//########## Make current shard's profile curve
	ToCylindricalInterpolation(shard[current_node - 1].edge_line_, pc_current, false);

	sort(pc_current.begin(), pc_current.end(), [](Vector3d a, Vector3d b) -> bool {
		return a(2) < b(2);
		});

	MinMaxValue pc_z_value;
	pc_z_value.min_value = pc.front()(2);
	pc_z_value.max_value = pc.back()(2);

	int start, end, Q_size = floor((pc_z_value.max_value - pc_z_value.min_value) / bin_size) + 1;
	vector<MinMaxValue> bin_list(Q_size);
	vector<MatrixXd> coeff_list(Q_size);
	int s_index(0);
	int pc_inlier(0);
	int num_points(0);
	for (int i = 0; i < Q_size; i++) {
		MinMaxValue bin;
		bin.max_value = bin_size * (double)(i + 1) + pc_z_value.min_value;
		bin.min_value = bin_size * (double)(i)+pc_z_value.min_value;
		int start = s_index;
		int end(0);

		for (int j = s_index; j < pc.size(); j++) {
			if (pc[j](2) > bin.max_value)
				break;
			s_index++;
		}
		end = s_index - 1;

		coeff_list[i] = CalculateLeastSquare(pc, start, end);
		bin_list[i].max_value = pc[end](2); bin_list[i].min_value = pc[start](2);

		if (total_count) {
			double den = sqrt(coeff_list[i](0) * coeff_list[i](0) + 1);
			for (int j = start; j < end; j++) {
				double dist(100);
				if (abs(coeff_list[i](0)) > 1) {
					dist = abs(coeff_list[i](0) * pc[j](2) - pc[j](0) + coeff_list[i](1)) / den;
				}
				else {
					dist = abs(pc[j](0) - (coeff_list[i](0) * pc[j](2) + coeff_list[i](1)));
				}
				if (dist < threshold) {
					pc_inlier++;
				}
			}
		}
	}
	if (!total_count) {
		pc_inlier = 0;
		for (int i = 0; i < pc_current.size(); i++) {
			for (int j = 0; j < Q_size; j++) {
				if ((pc_current[i](2) > bin_list[j].min_value) && (pc_current[i](2) < bin_list[j].max_value)) {
					double den = sqrt(coeff_list[j](0) * coeff_list[j](0) + 1);
					double dist(100);
					if (abs(coeff_list[j](0)) > 1) {
						dist = abs(coeff_list[j](0) * pc_current[i](2) - pc_current[i](0) + coeff_list[j](1)) / den;
					}
					else {
						dist = abs(pc_current[i](0) - (coeff_list[j](0) * pc_current[i](2) + coeff_list[j](1)));
					}
					if (dist < threshold) {
						pc_inlier++;
					}
					else if (dist > 10) {
						result = false;
						if (g_sfs_admit_trace && g_sfs339_pc_reason == 0) g_sfs339_pc_reason = 1;
					}
					num_points++;
				}
			}
		}
		//num_points = shard[current_node - 1].edge_line_.point_.cols();
	}

	if (num_points < 1) {
		num_points = 1;
	}
	double weight = pow(((double)pc_inlier / (double)num_points), 2) * 0.2 + 1;

	inlier = weight * inlier;

	// Breakline can not over rim height
	if (is_rim) {
		int up_count(0), down_count(0);
		MakeRadiusHeight(R_rim, H_rim, rim_data);
		MinMaxValue z_total;
		z_total.max_value = pc_current.back()(2);
		z_total.min_value = pc_current.front()(2);
		double top_gap = z_total.max_value - H_rim - 10;
		double bottom_gap = H_rim - z_total.min_value - 10;
		if ((top_gap > 0) && (bottom_gap > 0)) {
			result = false;
			if (g_sfs_admit_trace && g_sfs339_pc_reason == 0) g_sfs339_pc_reason = 2;
		}
	}

	return result;
}
bool CountPCInlier(int& inlier,
	const vector<bool>& true_node,
	const vector<bool>& merge_node,
	const vector<Geom>& shard,
	double bin_size,
	double threshold)
{
	int num_shard = shard.size();
	bool result = true;
	bool is_rim = false;
	vector<Vector3d> rim_data;
	double H_rim(0), R_rim(0);

	//########### First make profile curve
	vector<Vector3d> pc;
	vector<Vector3d> pc_merge;
	int num_points(0);
	int num_node(0);
	for (int i = 0; i < num_shard; i++) {
		// for debug
		if (true_node[i] && merge_node[i])
			cout << "CRITICAL ERROR : COUNTPCINLIER #####################################" << endl;
		// end debug
		if (true_node[i]) {
			ToCylindricalInterpolation(shard[i].edge_line_, pc, false);
			num_node++;
			// For rim height
			if (shard[i].edge_line_.is_seg_rim_) {
				is_rim = true;
				for (int j = 0; j < shard[i].edge_line_.index_.cols(); j++) {
					if (shard[i].edge_line_.index_(2, j)) {
						int start = shard[i].edge_line_.index_(0, j);
						int end = shard[i].edge_line_.index_(1, j);
						for (int r_c = start - 1; r_c < end; r_c++) {
							rim_data.push_back(shard[i].edge_line_.point_.col(r_c));
						}
					}

				}
			}
		}
		if (merge_node[i]) {
			ToCylindricalInterpolation(shard[i].edge_line_, pc_merge, false);
			num_node++;
		}
	}
	if (pc.empty()) 
		return result;


	sort(pc.begin(), pc.end(), [](Vector3d a, Vector3d b) -> bool {
		return a(2) < b(2);
		});
	sort(pc_merge.begin(), pc_merge.end(), [](Vector3d a, Vector3d b) -> bool {
		return a(2) < b(2);
		});

	MinMaxValue pc_z_value;
	pc_z_value.min_value = pc.front()(2);
	pc_z_value.max_value = pc.back()(2);

	int start(0), end(0), Q_size = floor((pc_z_value.max_value - pc_z_value.min_value) / bin_size) + 1;
	vector<MinMaxValue> bin_list(Q_size);
	vector<MatrixXd> coeff_list(Q_size);
	int s_index(0);
	int pc_inlier(0);

	for (int i = 0; i < Q_size; i++) {
		MinMaxValue bin;
		bin.max_value = bin_size * (double)(i + 1) + pc_z_value.min_value;
		bin.min_value = bin_size * (double)(i)+pc_z_value.min_value;
		int start = s_index;
		int end(0);

		for (int j = s_index; j < pc.size(); j++) {
			if (pc[j](2) > bin.max_value)
				break;
			s_index++;
		}
		end = s_index - 1;

		coeff_list[i] = CalculateLeastSquare(pc, start, end);
		bin_list[i].max_value = pc[end](2); bin_list[i].min_value = pc[start](2);
	}

	pc_inlier = 0;
	for (int i = 0; i < pc_merge.size(); i++) {
		for (int j = 0; j < Q_size; j++) {
			if ((pc_merge[i](2) > bin_list[j].min_value) && (pc_merge[i](2) < bin_list[j].max_value)) {
				double den = sqrt(coeff_list[j](0) * coeff_list[j](0) + 1);
				double dist(100);
				if (abs(coeff_list[j](0)) > 1) {
					dist = abs(coeff_list[j](0) * pc_merge[i](2) - pc_merge[i](0) + coeff_list[j](1)) / den;
				}
				else {
					dist = abs(pc_merge[i](0) - (coeff_list[j](0) * pc_merge[i](2) + coeff_list[j](1)));
				}
				if (dist < threshold) {
					pc_inlier++;
				}
				else if (dist > 10) {
					result = false;
					if (g_sfs_admit_trace && g_sfs339_pc_reason == 0) g_sfs339_pc_reason = 1;
				}
				num_points++;
			}
		}
	}

	if (num_points < 1) {
		num_points = 1;
	}
	double weight = pow(((double)pc_inlier / (double)num_points), 2) * 0.2 + 1;
	inlier = (weight) * inlier;

	// Breakline can not over rim height
	if (is_rim) {
		int up_count(0), down_count(0);
		MakeRadiusHeight(R_rim, H_rim, rim_data);
		MinMaxValue z_total;
		z_total.max_value = pc_merge.back()(2);
		z_total.min_value = pc_merge.front()(2);
		double top_gap = z_total.max_value - H_rim - 10;
		double bottom_gap = H_rim - z_total.min_value - 10;
		if ((top_gap > 0) && (bottom_gap > 0)) {
			result = false;
			if (g_sfs_admit_trace && g_sfs339_pc_reason == 0) g_sfs339_pc_reason = 2;
		}
	}

	return result;
}

void FeatureComp(vector<Geom>& a,
	list<LCSIndex>& LCS_out,
	double size,
	int windowsize,
	int mode)
{
	vector<double> Q_size;
	int shard_num = a.size();
	bool is_thickness = false;

	for (int i = 0; i < shard_num; i++) {
		if (!a[i].is_matching_) continue;
		if (a[i].edge_line_.feature_[0].cols() > 3) {
			is_thickness = true;
			break;
		}
	}

	if (mode == 1) {
		vector<MinMaxValue> mfeatures(3);
		int num_iter = 3;

		for (int j = 0; j < 3; j++) {
			MinMaxValue mTemp;
			for (int i = 0; i < shard_num; i++) {
				if (!a[i].is_matching_) continue;
				else {
					int num_axis = a[i].edge_line_.feature_.size();
					for (int axis_index = 0; axis_index < num_axis; axis_index++) {
						mTemp.max_value = max(a[i].edge_line_.feature_[axis_index].row(j).maxCoeff(), mTemp.max_value);
						mTemp.min_value = min(a[i].edge_line_.feature_[axis_index].row(j).minCoeff(), mTemp.min_value);
					}
				}
			}
			mfeatures[j] = mTemp;
		}

		// If there is thickness featuer.
		// 7 th row of feature_ is thickness
		if (is_thickness) {
			num_iter = 4;
			MinMaxValue mTemp;
			mTemp.min_value = 9999;
			for (int i = 0; i < shard_num; i++) {
				if (!a[i].is_matching_) continue;

				int num_axis = a[i].edge_line_.feature_.size();
				for (int axis_index = 0; axis_index < num_axis; axis_index++) {
					mTemp.max_value = max(a[i].edge_line_.feature_[axis_index].row(6).maxCoeff(), mTemp.max_value);

					for (int j = 0; j < a[i].edge_line_.feature_[axis_index].cols(); j++) {
						if (a[i].edge_line_.feature_[axis_index](6, j) < 0) {
							continue;
						}
						else {
							mTemp.min_value = min(mTemp.min_value, a[i].edge_line_.feature_[axis_index](6, j));
						}
					}
				}
			}

			mfeatures.push_back(mTemp);
		}

		for (int i = 0; i < num_iter; i++) {
			Q_size.push_back((mfeatures[i].max_value - mfeatures[i].min_value) / size);
		}
	}

	else {
		Q_size.resize(4);
		Q_size[0] = 0.15;	// D
		Q_size[1] = 0.15;	// H
		Q_size[2] = 0.15;	// Theta
		Q_size[3] = 0.2;
	}

	// i : standard piece(y axis), j : changing piece(x axis)
	for (int i = 0; i < shard_num; i++) {
		MatrixXd Temp;

		if (!a[i].is_matching_) continue;

		for (int j = i + 1; j < shard_num; j++) {

			a[j].edge_line_.ChangeOrder();
			int x_axis_size = a[j].edge_line_.axis_point_.size();
			int y_axis_size = a[i].edge_line_.axis_point_.size();

			for (int axis_x = 0; axis_x < x_axis_size; axis_x++) {
				for (int axis_y = 0; axis_y < y_axis_size; axis_y++) {
					list<LCSIndex> LCS_tmp;
					if (a[j].edge_line_.is_sane_base_ && a[i].edge_line_.is_sane_base_)
						continue;
					else if (a[j].edge_line_.is_sane_base_ && a[i].edge_line_.is_seg_base_)
						continue;
					else if (a[i].edge_line_.is_sane_base_ && a[j].edge_line_.is_seg_base_)
						continue;
					LCS_tmp = LCS(a[j].edge_line_, a[i].edge_line_, Q_size, windowsize, axis_x, axis_y);
					list<LCSIndex>::iterator iter = LCS_tmp.begin();

					int breakpointtest(0);

					for (iter = LCS_tmp.begin(); iter != LCS_tmp.end(); ++iter) {
						iter->shard_x_ = j + 1;				// Moving piece, inverted order
						iter->shard_y_ = i + 1;				// Fixed piece, original order
						iter->axis_index_x_ = axis_x;
						iter->axis_index_y_ = axis_y;
						LCSIndex reout = *iter;
						NoOverrange(a, *iter);
						iter->index_ = LCS_out.size();
						LCS_out.push_back(*iter);
					}
				}
			}

			a[j].edge_line_.ChangeOrder();
		}
	}
}



void FeatureCompGraphBuilding(vector<Geom>& a,
	list<LCSIndex>& LCS_out,
	double size,
	int windowsize,
	vector<vector<bool>>& matched_index)
{
	vector<double> Q_size;
	int shard_num = a.size();
	bool is_thickness = false;

	//#pragma omp parallel for
	for (int i = 0; i < shard_num; i++) {
		if (!a[i].is_matching_) continue;
		if (a[i].edge_line_.feature_[0].cols() > 3) {
			is_thickness = true;
			break;
		}
	}

	Q_size.resize(4);
	Q_size[0] = 0.15;	// D
	Q_size[1] = 0.15;	// H
	Q_size[2] = 0.15;	// Theta
	Q_size[3] = 0.2;

	// i : standard piece(y axis), j : changing piece(x axis)
	for (int i = 0; i < shard_num; i++) {
		MatrixXd Temp;

		if (!a[i].is_matching_) continue;

		for (int j = i + 1; j < shard_num; j++) {
			a[j].edge_line_.ChangeOrder();
			int x_axis_size = a[j].edge_line_.axis_point_.size();
			int y_axis_size = a[i].edge_line_.axis_point_.size();

			for (int axis_x = 0; axis_x < x_axis_size; axis_x++) {
				for (int axis_y = 0; axis_y < y_axis_size; axis_y++) {
					list<LCSIndex> LCS_tmp;
					if (a[j].edge_line_.is_sane_base_ && a[i].edge_line_.is_sane_base_)
						continue;
					else if (a[j].edge_line_.is_sane_base_ && a[i].edge_line_.is_seg_base_)
						continue;
					else if (a[i].edge_line_.is_sane_base_ && a[j].edge_line_.is_seg_base_)
						continue;
					LCS_tmp = LCSGraphBuilding(a[j].edge_line_, a[i].edge_line_, Q_size, windowsize, axis_x, axis_y, matched_index, j, i);
					list<LCSIndex>::iterator iter = LCS_tmp.begin();

					int breakpointtest(0);

					for (iter = LCS_tmp.begin(); iter != LCS_tmp.end(); ++iter) {
						iter->shard_x_ = j + 1;	// Moving piece, inverted order
						iter->shard_y_ = i + 1;  // Fixed piece, original order
						iter->axis_index_x_ = axis_x;
						iter->axis_index_y_ = axis_y;
						LCSIndex reout = *iter;
						NoOverrange(a, *iter);
						iter->index_ = LCS_out.size();
						LCS_out.push_back(*iter);
					}
				}
			}

			a[j].edge_line_.ChangeOrder();
		}
	}
}

// Update
void Merge(LCSIndex& ref, LCSIndex& add)
{
	LCSpoint new_s, new_e;
	int add_size(0);
	if (ref.start_.x < add.start_.x) {
		int x_size = add.end_.x - ref.start_.x + 1;
		int y_size = add.end_.y - ref.start_.y + 1;
		add_size = max(x_size, y_size) - ref.size_;

		ref.end_.x = ref.end_.x + add_size - 1;
		ref.end_.y = ref.end_.y + add_size - 1;
	}
	else {
		int x_size = ref.end_.x - add.start_.x + 1;
		int y_size = ref.end_.y - add.start_.y + 1;
		add_size = max(x_size, y_size) - ref.size_;

		ref.start_.x = ref.start_.x - add_size + 1;
		ref.start_.y = ref.start_.y - add_size + 1;
	}
}

// TICKET 336: which call site a MERGE line comes from (read-only label).
static const char* g_sfs_trace_site = "none";

void MergeSimPair(list<LCSIndex>& LCS_out)
{
	list<LCSIndex>::iterator iter = LCS_out.begin();
	for (; iter != LCS_out.end(); ) {
		list<LCSIndex>::iterator comp = iter;
		for (; comp != LCS_out.end(); ) {
			if (comp != iter) {
				if ((iter->SamePart(*comp)) && (iter->trans_.isSimilar(comp->trans_, 0.175, 20))) {
					if (g_sfs_stage_trace)
						cout << "[SFS-336]\tMERGE\t" << g_sfs_trace_site
						<< "\t" << (RankScore(*iter) < RankScore(*comp) ? iter->index_ : comp->index_)
						<< "\t" << (RankScore(*iter) < RankScore(*comp) ? comp->index_ : iter->index_)
						<< "\t" << iter->shard_y_ << "\t" << iter->shard_x_
						<< "\t" << RankScore(*iter) << "\t" << RankScore(*comp) << endl;
					if (RankScore(*iter) < RankScore(*comp)) {
						Merge(*iter, *comp);
						comp = LCS_out.erase(comp);
					}
					else {
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
}

void PairwisePruning(vector<Geom>& shard, list<LCSIndex>& LCS_out)
{
	if (g_sfs_stage_trace) g_sfs_trace_site = "pair";
	list<LCSIndex>::iterator iter = LCS_out.begin(), iter_tmp = LCS_out.begin();

	double high_score = NULL;
	vector<LCSIndex> lcsout;
	vector<list<LCSIndex>> lcs_basket;
	list<LCSIndex> lcs_dummy;
	int num_shard = shard.size();
	vector<Matrix3d> R_p, R_s;
	vector<Vector3d> t_p, t_s;
	R_p.resize(num_shard);
	R_s.resize(num_shard);
	t_p.resize(num_shard);
	t_s.resize(num_shard);

	vector<BreakLine> L; 
	L.resize(num_shard);
	for (int i = 0; i < num_shard; i++) {
		L[i] = shard[i].edge_line_;
	}

	while (iter != LCS_out.end()) {
		lcsout.push_back(*iter);
		CycleNode cycle;
		cycle.edges.push_back(lcsout.size() - 1);
		cycle.nodes.push_back(iter->shard_y_);
		cycle.nodes.push_back(iter->shard_x_);

		for (int i = 0; i < num_shard; i++) {
			R_s[i] = R_p[i] = Matrix3d::Identity();
			t_s[i] = t_p[i] = { 0, 0, 0 };
		}

		bool precor = true, one2one = true, one2one_switch = false, rim = true;
		Icp(L, R_p, t_p, lcsout, cycle, precor, one2one);

		//#################### update transformation matrix ####################//
		Matrix4d T_fix = Matrix4d::Identity();
		int index_f = iter->shard_y_ - 1;
		int index_m = iter->shard_x_ - 1;
		Matrix3d R_fix_i_1 = R_p[index_f].inverse();
		Vector3d t_fix_i_1 = -R_fix_i_1 * t_p[index_f];
		iter->trans_.Set(R_p[index_m], t_p[index_m], index_m + 1, index_f + 1);
		iter->trans_.Input(R_fix_i_1, t_fix_i_1);

		//#################### update axis ####################// 
		Matrix3d R_axis;
		Vector3d t_axis;
		iter->trans_.Output(R_axis, t_axis);
		Vector3d axis_move_normal = R_axis * L[index_m].axis_norm_[iter->axis_index_x_];
		double axis_angle = acos(axis_move_normal.dot(L[index_f].axis_norm_[iter->axis_index_y_]));
		iter->axis_raw_ = axis_angle;
		if (!g_sfs_axis_sign) axis_angle = min(axis_angle, 3.14159 - axis_angle);
		iter->axis_angle_ = axis_angle;
		EdgeLineMove(L[index_f], R_fix_i_1, t_fix_i_1);
		EdgeLineMove(L[index_m], R_fix_i_1, t_fix_i_1);

		//#################### Overlapping check ####################//
		double A_dummy(0), length(0);
		bool overlap = OverlapCheck_3d(L[iter->shard_y_ - 1], L[iter->shard_x_ - 1], A_dummy, length, 50.0); // 50
		//#################### Update LCS_out information ####################// 
		iter->overlap_ = overlap;
		iter->score_ = cycle.score;
		iter->inliner_ = cycle.inlier;
		iter->area_ = A_dummy;
		// TICKET 335: coverage, recorded for every candidate (cheap, and it enters
		// nothing unless SFS_SCORE_COVERAGE=1, where RankScore() divides by it).
		{
			const int n_f = (int)L[index_f].point_.cols();
			const int n_m = (int)L[index_m].point_.cols();
			const int n_min = min(n_f, n_m);
			iter->coverage_ = (n_min > 0) ? ((double)cycle.inlier / (double)n_min) : 1.0;
		}

		// TICKET 336: pair_enum candidate, after every per-candidate field is final.
		if (g_sfs_stage_trace) {
			cout << "[SFS-336]\tCAND\t" << iter->index_
				<< "\t" << iter->shard_y_ << "\t" << iter->shard_x_
				<< "\t" << iter->score_ << "\t" << iter->inliner_ << "\t" << iter->axis_raw_
				<< "\t" << iter->axis_angle_ << "\t" << iter->overlap_ << "\t" << iter->area_
				<< "\t" << iter->coverage_;
			Matrix4d Mc;
			iter->trans_.Output(Mc);
			for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) cout << "\t" << Mc(r, c);
			cout << endl;
		}

		//#################### Restore breakline data ####################// 
		Matrix3d R_re;
		Vector3d t_re;
		iter->trans_.InvOut(R_re, t_re);
		EdgeLineMove(L[index_m], R_re, t_re);

		if (iter == LCS_out.begin()) {
			lcs_dummy.push_back(*iter);
			++iter;
		}

		else {
			list<LCSIndex>::iterator iter_d = iter;
			iter_d--;
			if (!(iter_d->SamePart(*iter))) {
				lcs_basket.push_back(lcs_dummy);
				lcs_dummy.clear();
				lcs_dummy.push_back(*iter);
				++iter;
			}
			else {
				lcs_dummy.push_back(*iter);
				++iter;
			}
		}
		if (iter == LCS_out.end()) lcs_basket.push_back(lcs_dummy);
	}

	LCS_out.clear();
	//#################### Remove unsatisfied matching ####################//
	for (int i = 0; i < lcs_basket.size(); i++) {
		double lowest_score(9999);
		iter = lcs_basket[i].begin();
		for (; iter != lcs_basket[i].end();) {
			if (iter->axis_angle_ > 0.436) {	
				if (g_sfs_stage_trace)
					cout << "[SFS-336]\tDROP\tpair_axis\t" << iter->index_ << "\t" << iter->shard_y_
					<< "\t" << iter->shard_x_ << "\t" << iter->axis_angle_ << endl;
				iter = lcs_basket[i].erase(iter);
			}
			else 
			{
				lowest_score = min(lowest_score, iter->score_);   // TICKET 335: gates keep the legacy score
				++iter;
			}
		}

		if (lowest_score > 1.5 && !lcs_basket[i].empty()) g_sfs_basket_erased_15++;
		for (iter = lcs_basket[i].begin(); iter != lcs_basket[i].end();) {
			if (lowest_score > 1.5) {
				g_sfs_cand_erased_15++;
				if (g_sfs_stage_trace)
					cout << "[SFS-336]\tDROP\tpair_basket\t" << iter->index_ << "\t" << iter->shard_y_
					<< "\t" << iter->shard_x_ << "\t" << lowest_score << endl;
				iter = lcs_basket[i].erase(iter);
			}
			else if (iter->overlap_) {	
				if (g_sfs_stage_trace)
					cout << "[SFS-336]\tDROP\tpair_overlap\t" << iter->index_ << "\t" << iter->shard_y_
					<< "\t" << iter->shard_x_ << "\t" << iter->area_ << endl;
				iter = lcs_basket[i].erase(iter);
			}
			else {
				LCS_out.push_back(*iter);
				++iter;
			}
		}
	}

	MergeSimPair(LCS_out);

	// TICKET 336: survivors that will enter the state stage, with their matrices.
	if (g_sfs_stage_trace) {
		for (list<LCSIndex>::iterator sv = LCS_out.begin(); sv != LCS_out.end(); ++sv) {
			cout << "[SFS-336]\tSURV\t" << sv->index_ << "\t" << sv->shard_y_ << "\t" << sv->shard_x_
				<< "\t" << sv->score_ << "\t" << sv->axis_angle_;
			Matrix4d Ms;
			sv->trans_.Output(Ms);
			for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) cout << "\t" << Ms(r, c);
			cout << endl;
		}
	}

	// TICKET 335: the survivor line, so the audit can recompute the ranking from the log.
	// Printed only when a switch is on: with both unset this block does not execute.
	if (g_sfs_coverage || g_sfs_axis_sign_requested) {
		for (list<LCSIndex>::iterator sv = LCS_out.begin(); sv != LCS_out.end(); ++sv)
			cout << "[SFS-335] survivor " << sv->shard_x_ << "-" << sv->shard_y_
			<< " score_legacy=" << sv->score_
			<< " c=" << sv->coverage_
			<< " rank_score=" << RankScore(*sv)
			<< " axis_raw=" << sv->axis_raw_
			<< " axis_angle=" << sv->axis_angle_ << endl;
	}

	////#################### Remove high score matching ####################//
	int count_trans = 1;
	for (iter = LCS_out.begin(); iter != LCS_out.end();) {
		NoOverrange(shard, *iter);
		++iter;
		count_trans++;
	}
}

// ICCV->Hierarchy
void RegistrationPruning(vector<Geom>& shard,
	list<LCSIndex>& LCS_out,
	const vector<RankingSubgraph>& graph,
	int g_index,
	int step_counter)
{
	if (g_sfs_stage_trace) {
		g_sfs_trace_site = "reg";
		cout << "[SFS-336]\tPHASE\treg\t" << step_counter << "\t" << g_index << endl;
	}
	list<LCSIndex>::iterator iter = LCS_out.begin(), iter_tmp = LCS_out.begin();

	double high_score = NULL;
	vector<LCSIndex> lcsout;
	int num_shard = shard.size();
	vector<Matrix3d> R_p, R_s;
	vector<Vector3d> t_p, t_s;
	R_p.resize(num_shard);
	R_s.resize(num_shard);
	t_p.resize(num_shard);
	t_s.resize(num_shard);

	vector<BreakLine> L;
	L.resize(num_shard);
	for (int i = 0; i < num_shard; i++) {
		L[i] = shard[i].edge_line_;
	}

	while (iter != LCS_out.end()) {
		lcsout.push_back(*iter);
		CycleNode cycle;
		cycle.edges.push_back(lcsout.size() - 1);
		cycle.nodes.push_back(iter->shard_y_);
		cycle.nodes.push_back(iter->shard_x_);

		for (int i = 0; i < num_shard; i++) {
			R_s[i] = R_p[i] = Matrix3d::Identity();
			t_s[i] = t_p[i] = { 0, 0, 0 };
		}

		int index_m = iter->shard_x_ - 1;
		int index_f = iter->shard_y_ - 1;
		int axis_m = iter->axis_index_x_;
		int axis_f = iter->axis_index_y_;
		if (graph[g_index].node_[index_m]) {
			index_m = iter->shard_y_ - 1;
			index_f = iter->shard_x_ - 1;
			axis_m = iter->axis_index_y_;
			axis_f = iter->axis_index_x_;
		}
		int merge_index = -1;
		bool is_outside = isOutside(*iter, graph, g_index, index_m + 1, merge_index);

		bool precor = true, one2one = true, one2one_switch = true, rim = true;


		if (is_outside)
			Registration(L, R_p, t_p, *iter, graph[g_index].node_);
		else
			Registration(L, R_p, t_p, *iter, graph[g_index].node_, graph[merge_index].node_);

		//#################### update transformation matrix ####################//
		iter->trans_.Set(R_p[index_m], t_p[index_m], index_m + 1, index_f + 1);

		//#################### update axis ####################// 
		Matrix3d R_axis;
		Vector3d t_axis;
		iter->trans_.Output(R_axis, t_axis);
		Vector3d axis_move_normal = R_axis * L[index_m].axis_norm_[axis_m];
		double axis_angle = acos(axis_move_normal.dot(L[index_f].axis_norm_[axis_f]));
		iter->axis_raw_ = axis_angle;
		if (!g_sfs_axis_sign) axis_angle = min(axis_angle, 3.14159 - axis_angle);
		iter->axis_angle_ = axis_angle;

		//#################### Overlapping check ####################//
		double A_dummy(0), length(0);
		bool overlap = OverlapCheck_3d(L[iter->shard_y_ - 1], 
			L[iter->shard_x_ - 1],
			A_dummy, 
			length, 
			50.0); // 50
		
		//#################### Update LCS_out information ####################// 
		iter->overlap_ = overlap;
		iter->area_ = A_dummy;

		// TICKET 336: registration candidate, after trans_/axis/overlap are final.
		if (g_sfs_stage_trace) {
			cout << "[SFS-336]\tRCAND\t" << step_counter << "\t" << g_index
				<< "\t" << (is_outside ? 1 : 0) << "\t" << merge_index
				<< "\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_
				<< "\t" << iter->score_ << "\t" << iter->axis_raw_ << "\t" << iter->axis_angle_
				<< "\t" << iter->overlap_ << "\t" << iter->area_;
			Matrix4d Mr;
			iter->trans_.Output(Mr);
			for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) cout << "\t" << Mr(r, c);
			cout << endl;
		}

		//#################### Restore breakline data ####################// 
		Matrix3d R_re;
		Vector3d t_re;
		iter->trans_.InvOut(R_re, t_re);
		if(is_outside)
			EdgeLineMove(L[index_m], R_re, t_re);
		else {
			for (int i = 0; i < num_shard; i++) {
				if (graph[merge_index].node_[i])
					EdgeLineMove(L[i], R_re, t_re);
			}
		}

		iter++;
	}

	//#################### Remove unsatisfied matching ####################//
	for (iter = LCS_out.begin(); iter != LCS_out.end(); ) {
		bool s_x_base = shard[iter->shard_x_ - 1].edge_line_.is_seg_base_;
		bool s_y_base = shard[iter->shard_y_ - 1].edge_line_.is_seg_base_;
		
		if (iter->axis_angle_ > 0.436) {
			//if (s_x_base || s_y_base) 	// �̰� ���־����� �ʳ�??
			{
				if (g_sfs_stage_trace)
					cout << "[SFS-336]\tRDROP\treg_axis\t" << step_counter << "\t" << g_index
					<< "\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_
					<< "\t" << iter->axis_angle_ << endl;
				iter = LCS_out.erase(iter);
				continue;
			}
		}
		if ((iter->score_ > 10) || (iter->overlap_)) {   // TICKET 335: gates keep the legacy score
			if (iter->score_ > 10) g_sfs_cand_erased_10++;
			if (g_sfs_stage_trace)
				cout << "[SFS-336]\tRDROP\t" << (iter->score_ > 10 ? "reg_score" : "reg_overlap")
				<< "\t" << step_counter << "\t" << g_index
				<< "\t" << iter->index_ << "\t" << iter->shard_y_ << "\t" << iter->shard_x_
				<< "\t" << iter->score_ << "\t" << iter->area_ << endl;
			iter = LCS_out.erase(iter);
		}
		else
			iter++;
	}

	MergeSimPair(LCS_out);

	//#################### Remove overlap range LCS ####################//
	int count_trans = 1;
	for (iter = LCS_out.begin(); iter != LCS_out.end();) {
		NoOverrange(shard, *iter);
		++iter;
		count_trans++;
	}
}

bool MergeOverlapTest(vector<Geom>& shard,
	const vector<RankingSubgraph>& graph,
	int g_index,
	int merge_index)
{
	int num_shard = shard.size();
	bool result = false;
	// Overlap test
	for (int i = 0; i < num_shard; i++) {
		for (int j = i + 1; j < num_shard; j++) {
			double area, size;
			bool is_test = false;
			if (graph[merge_index].node_[i] && graph[g_index].node_[j])
				is_test = true;
			else if (graph[merge_index].node_[j] && graph[g_index].node_[i])
				is_test = true;
			if (is_test) {
				bool overlap = OverlapCheck_3d(shard[i].edge_line_,
					shard[j].edge_line_,
					area,
					size,
					100);
				if (overlap) {
					result = true;
					break;
				}
			}
		}
	}

	return result;
}

bool isOutside(const LCSIndex& lcs,
	const vector<RankingSubgraph>& graph,
	const int& g_index,
	int c_node,
	int& merge_index)
{
	// Classify edges whether outside or inside of the state.
	bool is_outside = true;
	merge_index = -1;
	for (int i = 0; i < graph.size(); i++) {
		if (i == g_index)
			continue;

		if (graph[i].node_[c_node - 1]) {
			is_outside = false;
			merge_index = i;
			break;
		}
	}

	return is_outside;
}


