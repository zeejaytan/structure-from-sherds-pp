#pragma once
#ifndef _RECONSTRUCTION_H_
#define _RECONSTRUCTION_H_

#include <omp.h>
#include "ceres/cost_function.h"
#include "ceres/ceres.h"
#include "ceres/rotation.h"
#include "../class/data_structure.h"
#include "../class/feature_matching.h"
#include "../class/axis_estimation.h"
#include <pcl/common/common_headers.h>
#include <pcl/features/normal_3d.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/io/vtk_lib_io.h>
#include <pcl/io/impl/vtk_lib_io.hpp>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/console/parse.h>
#include <pcl/common/transforms.h>
class RankingSubgraph;

#define CERES_FUNC_TOL			1.0e-3
#define MINIMUM_NUMBER			6
#define INLIER_THRESHOLD		1.5
#define ANGLE_THRESHOLD			0.262

// Ticket 333: the agreement gate is an ABSOLUTE distance in millimetres, so it does not
// follow the object's scale -- at 3x the same 1.5 mm is a third of the physical tolerance.
// SFS_INLIER_MM lets one run override the gate without editing the shipped default: unset,
// empty, unparseable or out-of-range values all keep INLIER_THRESHOLD, so a run without the
// variable behaves exactly like the shipped binary. It is read once at start-up in
// reconstruction.cpp and printed once as "inlier gate: <x> mm"; both consumers of
// INLIER_THRESHOLD below use SFS_INLIER_MM instead of the literal.
#define SFS_INLIER_MM_MIN	0.05
#define SFS_INLIER_MM_MAX	100.0
extern double SFS_INLIER_MM;

// TICKET 357 (SFS_XY_BREAK_NORMALS): compare breakline normals ORTHOGONALLY to the pot axis.
// g_sfs357_xy_axis is the pot axis expressed in the frame the breakline data are currently in;
// it is set inside Icp() only (reconstruction.cpp) and is the ZERO vector everywhere else, which
// every consumer below treats as "switch off" and answers with the exact expression it used
// before this ticket, so an off run is bit-identical to the shipped binary.
extern Vector3d g_sfs357_xy_axis;

inline bool Sfs357XYOn() { return g_sfs357_xy_axis.squaredNorm() > 1e-12; }

// cos of the angle between two normals measured in the plane orthogonal to the pot axis.  When
// either normal is (almost) parallel to the axis the projected direction is noise, so the plain
// 3D dot is used for that pair.
inline double Sfs357NormDot(const Vector3d& n_A, const Vector3d& n_B)
{
	if (!Sfs357XYOn()) return n_A.dot(n_B);
	Vector3d a = n_A - n_A.dot(g_sfs357_xy_axis) * g_sfs357_xy_axis;
	Vector3d b = n_B - n_B.dot(g_sfs357_xy_axis) * g_sfs357_xy_axis;
	if (a.norm() < 0.1 || b.norm() < 0.1) return n_A.dot(n_B);
	a.normalize(); b.normalize();
	double c = a.dot(b);
	return c > 1.0 ? 1.0 : (c < -1.0 ? -1.0 : c);
}

// The folded angle the inlier counters use, i.e. the same acos the legacy code took, except under
// the switch, where it is taken between the two projected normals.
inline double Sfs357NormAngle(const Vector3d& n_A, const Vector3d& n_B)
{
	if (!Sfs357XYOn()) return acos(n_A.dot(n_B));
	return acos(Sfs357NormDot(n_A, n_B));
}

typedef pcl::PointCloud<pcl::PointNormal> pc_cloud;
typedef pc_cloud::Ptr ptr_cloud;

//############################## Other function ##############################//
bool isConverge(const Matrix4d& T,
	double rad_threshold,
	double t_threshold);	
bool isConverge(const Matrix3d& R,
	const Vector3d t,
	double rad_threshold,
	double t_threshold);

double isClose2Eye(const Matrix4d& T);
double isClose2Eye(const Matrix3d& R, const Vector3d& t);

void AxisAlignment(BreakLine& data,
	Matrix3d& R_out,
	Vector3d& t_out,
	int axis_index = 0);

// TICKET 335: load-time sign guard for the raw-angle (SFS_AXIS_SIGN) axis test.
// Measures, over every GT-joined pair of the piece set, the RAW angle between the two
// pieces' stored axis directions carried into the vessel frame by each piece's own GT
// transform. Returns true when the axes are direction-consistent (at most threshold_frac
// of the joined pairs read beyond 90 deg), i.e. when an un-folded gate is admissible.
// frac_beyond and n_pairs are returned for the log line.
bool SignAxisGuard(const vector<Vector3d>& axis_dir,
	const vector<Trans>& GT_trans,
	const MatrixXd& GT_graph,
	double threshold_frac,
	double& frac_beyond,
	int& n_pairs);

void RejectOutlier(Corres& cor,
	double dist_TH,
	double angle_TH);

void MakeBlock(BreakLine& L,
	BreakLine& output,
	int start,
	int end);

// make_section : For using intervals from LCS out, we need to divide the breakline data..
// A = shard_y, B = shard_x 
void MakeSection(vector<BreakLine>& L,
	BreakLine& p_A,
	BreakLine& p_B, 
	const LCSIndex& index);	
void MakeSection(vector<Geom>& shard,
	BreakLine& p_A,
	BreakLine& p_B, 
	LCSIndex& index);

void ChangeFormat(const BreakLine& in, ptr_cloud out);

// A = shard_y, B = shard_x
void MakeCor(Corres& c_in,
	BreakLine& in_A,
	BreakLine& in_B,
	bool onetoone);	
void MakeCor(Corres& c_in, 
	Corres& c_compare, 
	BreakLine& A, 
	BreakLine& B, 
	int K = 1);

void MakeCorWOBuildTree(Corres& c_in,
	BreakLine& in_A,
	BreakLine& in_B,
	bool onetoone);

void MakeRadiusHeight(double& r,
	double& h,
	vector<Vector3d>& data);

bool isEdgeRemoved(const MatrixXd& table, const vector<LCSIndex>& edges);

void MakeSingleCorres(vector<Corres>& COR,
	vector<BreakLine>& L,
	const vector<bool>& true_node,
	const int& c_node,
	bool onetoone);

void MakeMergeCorres(vector<Corres>& COR,
	vector<BreakLine>& L,
	const vector<bool>& true_node,
	const vector<bool>& merge_node,
	bool onetoone);

void MakeMultiCorres(vector<Corres>& COR,
	vector<Geom>& shard,
	const vector<bool>& true_node,
	MatrixXd& simple_graph,
	MatrixXd& table,
	bool onetoone);

void MakeCorWithSur(vector<Corres>& COR_line,
	vector<Corres>& COR_sur,
	vector<Corres>& COR_frac,
	vector<Geom>& shard,
	const vector<bool>& true_node,
	MatrixXd& table,
	bool onetoone);

bool inlierCalculate(int& inlier,
	const vector<bool>& true_node,
	const vector<Corres>& cor,
	const vector<BreakLine>& L,
	bool volume_weight = false);
bool inlierCalculate(int& inlier,
	const vector<bool>& true_node,
	const vector<Corres>& cor,
	const vector<Geom>& shard,
	bool volume_weight = false);

void AxisConsistency(vector<BreakLine>& L,
	ceres::Problem& problem,
	ceres::LossFunction* loss_axis,
	double* s,
	double* trans,
	double w_a,
	int index);
void AxisConsistency(vector<Geom>& shard,
	ceres::Problem& problem,
	ceres::LossFunction* loss_axis,
	double* s,
	double* trans,
	double w_a,
	int index);

void MakeRimData(vector<Vector3d>& rim_data,
	const vector<bool>& true_node,
	const vector<BreakLine>& L);
void MakeRimData(vector<Vector3d>& rim_data,
	const vector<bool>& true_node,
	const vector<Geom>& shard);

void RimConstraint(vector<BreakLine>& L,
	ceres::Problem& problem,
	ceres::LossFunction* loss_rim,
	double* s,
	double* trans,
	double* R_rim,
	double* H_rim,
	double w_r,
	double w_h,
	int index);

void P2LConstraint(vector<Corres>& COR,
	ceres::Problem& problem,
	ceres::LossFunction* loss_dist,
	ceres::LossFunction* loss_norm,
	double** s,
	double** trans,
	double w_line,
	double w_n);
void P2LConstraintFixed(vector<Corres>& COR,
	ceres::Problem& problem,
	ceres::LossFunction* loss_dist,
	ceres::LossFunction* loss_norm,
	double* s,
	double* trans,
	double w_line,
	double w_n);

void P2PConstraint(vector<Corres>& COR,
	ceres::Problem& problem,
	ceres::LossFunction* loss_dist,
	ceres::LossFunction* loss_norm,
	double** s,
	double** trans,
	double w_d,
	double w_n);
void P2PConstraintFixed(vector<Corres>& COR,
	ceres::Problem& problem,
	ceres::LossFunction* loss_dist,
	ceres::LossFunction* loss_norm,
	double* s,
	double* trans,
	double w_d,
	double w_n);

void SurConstraint(vector<Corres>& COR,
	ceres::Problem& problem,
	ceres::LossFunction* loss_dist,
	double** s,
	double** trans,
	double weight);

void UsePreCorres(Corres& cor_in,
	vector<BreakLine>& L,
	BreakLine& p_A,
	BreakLine& p_B,
	const LCSIndex& lcs,
	int c_node = 0,
	bool fix = false);

void UpdateTrans(BreakLine& L,
	double* s,
	double* trans,
	vector<Matrix3d>& R,
	vector<Vector3d>& t,
	vector<Matrix3d>& R_i,
	vector<Vector3d>& t_i,
	int index);
void UpdateTrans(Geom& shard,
	double* s,
	double* trans,
	vector<Matrix3d>& R,
	vector<Vector3d>& t,
	vector<Matrix3d>& R_i,
	vector<Vector3d>& t_i,
	int index);

void FillMatchedPoints(const vector<Corres>& COR,
	vector<vector<bool>>& table);

void Icp(vector<BreakLine>& L,
	vector<Matrix3d>& R,
	vector<Vector3d>& t,
	const vector<LCSIndex>& m_LCS,
	CycleNode& cycle,
	bool pre_cor,
	bool onetoone);

void Registration(vector<BreakLine>& L,
	vector<Matrix3d>& R,
	vector<Vector3d>& t,
	LCSIndex& lcs,
	const vector<bool>& true_node);
void Registration(vector<BreakLine>& L,
	vector<Matrix3d>& R,
	vector<Vector3d>& t,
	LCSIndex& lcs,
	const vector<bool>& true_node,
	const vector<bool>& merge_node);

void IcpIncGraphAxis(
	vector<Geom>& shard,
	vector<Matrix3d>& R,
	vector<Vector3d>& t,
	RankingSubgraph& graph,
	vector<RankingSubgraph>& pregraph,
	int& inlier,
	bool rim,
	bool axis);

void IcpFine(
	vector<Geom>& shard,
	vector<Matrix3d>& R,
	vector<Vector3d>& t,
	vector<bool>& true_node,
	MatrixXd& graph);

//############################## Cost function ##############################//
class BiaxialCaoErrorWithFixedAxis {
public:
	BiaxialCaoErrorWithFixedAxis(
		const Eigen::Ref<Eigen::Matrix<double, 3, 1>>& point,
		const Eigen::Ref<Eigen::Matrix<double, 3, 1>>& normal,
		const double weight
	) : point_(point), normal_(normal), weight_(weight) {};

	template <typename T>

	bool operator() (
		const T* const omega,
		const T* const trans,
		T* residual
		) const {

		// Transform surface point
		T transformedPoint[3] { (T)0.0, (T)0.0, (T)0.0 };
		T point[3] { T(point_[0]), T(point_[1]), T(point_[2]) };
		ceres::AngleAxisRotatePoint(omega, point, transformedPoint);
		transformedPoint[0] += trans[0];
		transformedPoint[1] += trans[1];
		transformedPoint[2] += trans[2];

		// Transform normal
		T transformedNormal[3] = { (T)0.0, (T)0.0, (T)0.0 };
		T normal[3] = { T(normal_[0]), T(normal_[1]), T(normal_[2]) };
		ceres::AngleAxisRotatePoint(omega, normal, transformedNormal);

		// Set axis point and normal fixed to the origin and the positive z-vector respectively.
		T axisPoint[3] = { (T)0.0, (T)0.0, (T)0.0 };
		T axisNormal[3] = { (T)0.0, (T)0.0, (T)1.0 };

		// First compute Cao error.
		T T1[3], T2[3], T3[3];

		// Compute T1
		T1[0] = transformedPoint[0] - axisPoint[0];
		T1[1] = transformedPoint[1] - axisPoint[1];
		T1[2] = transformedPoint[2] - axisPoint[2];

		// Compute T2 & T3.
		ceres::CrossProduct(T1, axisNormal, T2);
		// T normal[3]{ T(normal_[0]), T(normal_[1]), T(normal_[2]) };
		ceres::CrossProduct(transformedNormal, axisNormal, T3);

		T scale = ceres::sqrt((T2[0] * T2[0] + T2[1] * T2[1] + T2[2] * T2[2])
			/ (T3[0] * T3[0] + T3[1] * T3[1] + T3[2] * T3[2]));

		T res1[3];
		res1[0] = T2[0] - scale * T3[0];
		res1[1] = T2[1] - scale * T3[1];
		res1[2] = T2[2] - scale * T3[2];
		// jhh37: added sqrt (different from PotSAC)
		T dist1 = ceres::sqrt(res1[0] * res1[0] + res1[1] * res1[1] + res1[2] * res1[2]);

		T res2[3];
		res2[0] = T2[0] + scale * T3[0];
		res2[1] = T2[1] + scale * T3[1];
		res2[2] = T2[2] + scale * T3[2];
		// jhh37: added sqrt (different from PotSAC)
		T dist2 = ceres::sqrt(res2[0] * res2[0] + res2[1] * res2[1] + res2[2] * res2[2]);

		T c = std::min(dist1, dist2);
		T s{ T(100.0) };

		residual[0] = T(weight_)*(c - 1.0 / s * ceres::log(ceres::exp(s * -(dist1 - c)) +
			ceres::exp(s * -(dist2 - c))));

		return true;
	}

private:
	Eigen::Vector3d point_;
	Eigen::Vector3d normal_;
	double weight_;
};

class CostFuncDist {
public:
	CostFuncDist() {}

	CostFuncDist(CorPair one, double d)
		: one_pair_(one), weight_d_(d) { 	}

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const s_B, const Type* const trans_A, const Type* const trans_B, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		ceres::AngleAxisToRotationMatrix(s_B, dummy);
		Eigen::Matrix<Type, 3, 3> R_B(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, t_B, tempn, temp, line_norm_A, line_norm_B;
		t_A << trans_A[0], trans_A[1], trans_A[2];
		t_B << trans_B[0], trans_B[1], trans_B[2];

		temp = (R_A * one_pair_.p_A + t_A) - (R_B * one_pair_.p_B + t_B);

		residual[0] = (Type)weight_d_ * temp(0);
		residual[1] = (Type)weight_d_ * temp(1);
		residual[2] = (Type)weight_d_ * temp(2);
		return true;
	}

private:
	CorPair one_pair_;
	double weight_d_;
};

class CostFuncFixedDist {
public:
	CostFuncFixedDist() {}

	CostFuncFixedDist(CorPair one, double d)
		: one_pair_(one), weight_d_(d) { 	}

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const trans_A, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, temp;
		t_A << trans_A[0], trans_A[1], trans_A[2];
		temp = (R_A * one_pair_.p_A + t_A) - one_pair_.p_B;

		residual[0] = (Type)weight_d_ * temp(0);
		residual[1] = (Type)weight_d_ * temp(1);
		residual[2] = (Type)weight_d_ * temp(2);
		return true;
	}

private:
	CorPair one_pair_ ;
	double weight_d_;
};

class CostFuncLineDist {
public:
	CostFuncLineDist() {}

	CostFuncLineDist(CorPair one, double l)
		: one_pair_(one), weight_l_(l) { 	}

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const s_B, const Type* const trans_A, const Type* const trans_B, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		ceres::AngleAxisToRotationMatrix(s_B, dummy);
		Eigen::Matrix<Type, 3, 3> R_B(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, t_B, tempn, temp, line_norm_A, line_norm_B, norm_A, norm_B;
		t_A << trans_A[0], trans_A[1], trans_A[2];
		t_B << trans_B[0], trans_B[1], trans_B[2];

		temp = (R_A * one_pair_.p_A + t_A) - (R_B * one_pair_.p_B + t_B);

		norm_A = (R_A * one_pair_.n_A);
		norm_B = (R_B * one_pair_.n_B);
		line_norm_A = R_A * one_pair_.line_n_A;
		line_norm_B = R_B * one_pair_.line_n_B;

		Type pl_dist_A = line_norm_A(0) * temp(0) + line_norm_A(1) * temp(1) + line_norm_A(2) * temp(2);
		Type pl_dist_B = line_norm_B(0) * temp(0) + line_norm_B(1) * temp(1) + line_norm_B(2) * temp(2);
		Type sur_dist_A = temp(0) * norm_B(0) + temp(1) * norm_B(1) + temp(2) * norm_B(2);
		Type sur_dist_B = temp(0) * norm_A(0) + temp(1) * norm_A(1) + temp(2) * norm_A(2);


		residual[0] = (Type)weight_l_ * pl_dist_A;
		residual[1] = (Type)weight_l_ * pl_dist_B;
		residual[2] = (Type)weight_l_ * sur_dist_A;
		residual[3] = (Type)weight_l_ * sur_dist_B;
		return true;
	}

private:
	CorPair one_pair_;
	double weight_l_;
};

class CostFuncFixedLineDist {
public:
	CostFuncFixedLineDist() {}

	CostFuncFixedLineDist(CorPair one, double l)
		: one_pair_(one), weight_l_(l) { 	}

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const trans_A, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, tempn, temp, line_norm_A, norm_A;
		t_A << trans_A[0], trans_A[1], trans_A[2];

		temp = (R_A * one_pair_.p_A + t_A) - one_pair_.p_B;

		norm_A = (R_A * one_pair_.n_A);
		line_norm_A = R_A * one_pair_.line_n_A;

		Type pl_dist_A = (Type)(line_norm_A(0) * temp(0) + line_norm_A(1) * temp(1) + line_norm_A(2) * temp(2));
		Type sur_dist_A = (Type)(temp(0) * norm_A(0) + temp(1) * norm_A(1) + temp(2) * norm_A(2));

		residual[0] = (Type)weight_l_ * pl_dist_A;
		residual[1] = (Type)weight_l_ * sur_dist_A;

		return true;
	}

private:
	CorPair one_pair_;
	double weight_l_;
};

class CostFuncNorm {
public:
	CostFuncNorm() {}

	CostFuncNorm(CorPair one, double n)
		: one_pair_(one), weight_n_(n) { 	}

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const s_B, const Type* const trans_A, const Type* const trans_B, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		ceres::AngleAxisToRotationMatrix(s_B, dummy);
		Eigen::Matrix<Type, 3, 3> R_B(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, t_B, tempn;
		t_A << trans_A[0], trans_A[1], trans_A[2];
		t_B << trans_B[0], trans_B[1], trans_B[2];

		tempn = (R_A * one_pair_.n_A) - (R_B * one_pair_.n_B);

		// TICKET 357 (SFS_XY_BREAK_NORMALS): keep only the part of both normals that is orthogonal
		// to the pot axis, seen through the same incremental rotation as the normals themselves.
		// Off (the default) g_sfs357_xy_axis is zero and this block does not run.
		if (Sfs357XYOn()) {
			Eigen::Matrix<Type, 3, 1> u;
			u << (Type)g_sfs357_xy_axis(0), (Type)g_sfs357_xy_axis(1), (Type)g_sfs357_xy_axis(2);
			u = R_A * u;
			tempn = tempn - u * u.dot(tempn);
		}

		for (int j = 0; j < 3; j++) {
			residual[j] = (Type)weight_n_ * tempn(j);
		}
		return true;
	}

private:
	CorPair one_pair_;
	double weight_n_;
};

class CostFuncFixedNorm {
public:
	CostFuncFixedNorm() {}

	CostFuncFixedNorm(CorPair one, double n)
		: one_pair_(one), weight_n_(n) { 	}

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const trans_A, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, tempn;
		t_A << trans_A[0], trans_A[1], trans_A[2];

		tempn = (R_A * one_pair_.n_A) - one_pair_.n_B;

		// TICKET 357 (SFS_XY_BREAK_NORMALS): as in CostFuncNorm, drop the axis component.  Off: no-op.
		if (Sfs357XYOn()) {
			Eigen::Matrix<Type, 3, 1> u;
			u << (Type)g_sfs357_xy_axis(0), (Type)g_sfs357_xy_axis(1), (Type)g_sfs357_xy_axis(2);
			u = R_A * u;
			tempn = tempn - u * u.dot(tempn);
		}

		for (int j = 0; j < 3; j++) {
			residual[j] = (Type)weight_n_ * tempn(j);
		}
		return true;
	}

private:
	CorPair one_pair_;
	double weight_n_;
};

// TICKET 357 (SFS_AXIS_LOCK_ICP): hold the candidate axis pair rigidly for the whole Icp.
// residual[0..2]: the two axis directions stay aligned (u_A == sign * u_B), weight 1e3;
// residual[3..5]: the moving axis ORIGIN stays on the fixed axis LINE -- the component of the
// origin-to-origin offset perpendicular to u_A is driven to zero, weight 10.  No loss function:
// this is a hard lock, and the data themselves are already placed on it at iter 0 (Icp's init).
class CandidateAxisLockFunctor {
public:
	CandidateAxisLockFunctor(const Vector3d& o_A, const Vector3d& u_A,
		const Vector3d& o_B, const Vector3d& u_B, double sign)
		: mo_A_(o_A), mu_A_(u_A.normalized()), mo_B_(o_B), mu_B_(u_B.normalized()), msign_(sign) { }

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const s_B, const Type* const trans_A, const Type* const trans_B, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		ceres::AngleAxisToRotationMatrix(s_B, dummy);
		Eigen::Matrix<Type, 3, 3> R_B(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, t_B, u_A, u_B, o_A, o_B;
		t_A << trans_A[0], trans_A[1], trans_A[2];
		t_B << trans_B[0], trans_B[1], trans_B[2];
		u_A << (Type)mu_A_(0), (Type)mu_A_(1), (Type)mu_A_(2);
		u_B << (Type)mu_B_(0), (Type)mu_B_(1), (Type)mu_B_(2);
		o_A << (Type)mo_A_(0), (Type)mo_A_(1), (Type)mo_A_(2);
		o_B << (Type)mo_B_(0), (Type)mo_B_(1), (Type)mo_B_(2);

		Eigen::Matrix<Type, 3, 1> ax = R_A * u_A;
		Eigen::Matrix<Type, 3, 1> bx = R_B * u_B;
		Eigen::Matrix<Type, 3, 1> d = (R_B * o_B + t_B) - (R_A * o_A + t_A);
		Type along = ax.dot(d);

		for (int j = 0; j < 3; j++) {
			residual[j] = (Type)1000.0 * (ax(j) - (Type)msign_ * bx(j));
			residual[j + 3] = (Type)10.0 * (d(j) - along * ax(j));
		}
		return true;
	}

private:
	Vector3d mo_A_, mu_A_, mo_B_, mu_B_;
	double msign_;
};

class CostFuncRim {
public:
	CostFuncRim() {}

	CostFuncRim(MatrixXd pA, double w_r, double w_h)
		: point_(pA), weight_r_(w_r), weight_h_(w_h) { 	}

	template <typename Type>
	bool operator()(const Type* const s, const Type* const trans, const Type* const radius, const Type* const height, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s, dummy);
		Eigen::Matrix<Type, 3, 3> R_i(dummy);

		Eigen::Matrix<Type, 3, 1> t_i;
		t_i << trans[0], trans[1], trans[2];

		Eigen::Matrix<Type, 3, 1> tmp;
		tmp = R_i * point_ + t_i;
		Type r_tmp(0), h_tmp(0);

		r_tmp = sqrt(tmp(0) * tmp(0) + tmp(1) * tmp(1));	// sqrt(x^2 + y^2)
		h_tmp = tmp(2);

		Type r = *radius;
		Type h = *height;

		residual[0] = (Type)weight_r_ * (*radius - r_tmp);
		residual[1] = (Type)weight_h_ * (*height - h_tmp);

		return true;
	}

private:
	MatrixXd point_;
	double weight_r_, weight_h_;
};

class CostFunctorSur {
public:
	CostFunctorSur() {}

	CostFunctorSur(CorPair one, double d)
		: one_pair_(one), weight_(d) { 	}

	template <typename Type>
	bool operator()(const Type* const s_A, const Type* const s_B, const Type* const trans_A, const Type* const trans_B, Type* residual) const {
		Type dummy[9];

		ceres::AngleAxisToRotationMatrix(s_A, dummy);
		Eigen::Matrix<Type, 3, 3> R_A(dummy);

		ceres::AngleAxisToRotationMatrix(s_B, dummy);
		Eigen::Matrix<Type, 3, 3> R_B(dummy);

		Eigen::Matrix<Type, 3, 1> t_A, t_B, temp, n_A, n_B;
		t_A << trans_A[0], trans_A[1], trans_A[2];
		t_B << trans_B[0], trans_B[1], trans_B[2];

		temp = (R_A * one_pair_.p_A + t_A) - (R_B * one_pair_.p_B + t_B);
		n_A = R_A * one_pair_.n_A;
		n_B = R_B * one_pair_.n_B;

		residual[0] = (Type)weight_ * (n_B(0) * temp(0) + n_B(1) * temp(1) + n_B(2) * temp(2));
		residual[1] = (Type)weight_ * (n_A(0) * temp(0) + n_A(1) * temp(1) + n_A(2) * temp(2));

		return true;
	}

private:
	CorPair one_pair_;
	double weight_;
};

#endif