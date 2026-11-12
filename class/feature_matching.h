#pragma once
#ifndef _FEATURE_MATCHING_H_
#define _FEATURE_MATCHING_H_

#include <omp.h>
#include "../class/data_structure.h"
#include "../class/filter.h"
#include "../class/reconstruction.h"
#include "../class/visualize.h"

class RankingSubgraph;

// TICKET 335 switches (see feature_matching.cpp): both default off.
extern bool g_sfs_coverage;              // SFS_SCORE_COVERAGE=1
extern bool g_sfs_axis_sign_requested;   // SFS_AXIS_SIGN=1, before the load-time guard
extern bool g_sfs_axis_sign;
extern long g_sfs_basket_erased_15;	// TICKET 335: measurement only
extern long g_sfs_cand_erased_15;
extern long g_sfs_cand_erased_10;             // SFS_AXIS_SIGN=1 and the guard accepted the set
extern bool g_sfs_stage_trace;           // TICKET 336: SFS_STAGE_TRACE=1, read-only dumps
void InitSfsSwitches();                  // reads both env switches once, prints both banners
void SfsTraceMat(const Trans& T);         // TICKET 336: 12 tab-separated numbers, no newline

void NoOverrange(vector<Geom>& shard,
	LCSIndex& reout);
void NoOverrange(vector<BreakLine>& breakline,
	LCSIndex& reout);

MatrixXd Quantization(MatrixXd& a,
	vector<double>& stepsize);

void LCSPointAverage(LCSpoint& cluster,
	LCSpoint& point,
	int num,
	bool rough = false);
void LCSPointAverage(list<LCSIndex>& cls,
	LCSpoint& middle);

list<LCSIndex> Clustering(list<LCSIndex>& c_result,
	int cls_threshold = 20);

LCSIndex Representative(list<LCSIndex>& cls,
	LCSpoint& middle);

list<LCSIndex> LCS(BreakLine& X,
	BreakLine& Y,
	vector<double>& Q_size,
	int windowsize,
	int axis_x,
	int axis_y);

list<LCSIndex> LCSGraphBuilding(BreakLine& X,
	BreakLine& Y,
	vector<double>& Q_size,
	int windowsize,
	int axis_x,
	int axis_y,
	vector<vector<bool>>& matched_index,
	const int index_X,
	const int index_Y);

void MatrixRearrangeCCW(BreakLine& L0);

MatrixXd CalculateLeastSquare(vector<Vector3d>& data,
	int& start,
	int& end);

// Make correspondence based on L1
void MakeCorWOTree(vector<CorIndex>& cor,
	BreakLine& L0,
	BreakLine& L1,
	vector<int>& region);	

bool DetectIntersection(BreakLine& L0,
	BreakLine& L1,
	CorIndex& cor,
	vector<Vector3d>& pc);

void SortRegion(vector<int>& region,
	const int& num);

bool OverlapCheck_3d(BreakLine& L0,
	BreakLine& L1,
	double& area,
	double& size,
	double threshold);
bool OverlapCheck_3d(BreakLine& L0,
	BreakLine& L1,
	double& area,
	double& size,
	double threshold,
	vector<Vector3d>& out);
bool OverlapCheck_3d(BreakLine& L0,
	BreakLine& L1,
	double& area,
	double& size,
	double threshold,
	MatrixXd& out);

bool ProfileChecking(vector<Vector3d>& profile,
	double bin_size,
	double threshold);

bool ProfileCheckingWithInlier(vector<Vector3d>& profile,
	double bin_size,
	double threshold,
	double th_inlier,
	int& inlier);

void MatchingScore(double& score,
	const vector<Corres>& COR);

void CountInlier(int& inlier,
	const vector<Corres>& COR,
	double threshold = 0.8,
	double angle_th = 0.175);
void CountInlier(int& inlier,
	const vector<Corres>& COR,
	int num_shard,
	double threshold = 0.8,
	double angle_th = 0.175);
void CountInlier(int& inlier,
	const vector<Corres>& COR,
	const vector<double>& num_points,
	double threshold = 0.8,
	double angle_th = 0.175);

bool CountPCInlier(int& inlier,
	const vector<bool>& true_node,
	const vector<Geom>& shard,
	const int& current_node,
	double bin_size,
	double threshold,
	bool total_count = false);
bool CountPCInlier(int& inlier,
	const vector<bool>& true_node,
	const vector<bool>& merge_node,
	const vector<Geom>& shard,
	double bin_size,
	double threshold);

void FeatureComp(vector<Geom>& a,
	list<LCSIndex>& LCS_out,
	double size,
	int windowsize,
	int mode);

void FeatureCompGraphBuilding(vector<Geom>& a,
	list<LCSIndex>& LCS_out,
	double size,
	int windowsize, 
	vector<vector<bool>>& matched_index);

// ref : reference interval, add : additional interval
void Merge(LCSIndex& ref,
	LCSIndex& add);	

void MergeSimPair(list<LCSIndex>& LCS_out);

void PairwisePruning(vector<Geom>& shard,
	list<LCSIndex>& LCS_out);

void RegistrationPruning(vector<Geom>& shard,
	list<LCSIndex>& LCS_out,
	const vector<RankingSubgraph>& graph,
	int g_index,
	int step_counter);	// Step counter : for debugging

bool MergeOverlapTest(vector<Geom>& shard,
	const vector<RankingSubgraph>& graph,
	int g_index,
	int merge_index);

bool isOutside(const LCSIndex& lcs,
	const vector<RankingSubgraph>& graph,
	const int& g_index,
	int c_node,
	int& merge_index);

#endif