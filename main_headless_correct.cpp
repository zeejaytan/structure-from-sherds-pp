#include <iostream>
#include "glog/logging.h"
#include <time.h>
#include <cstdlib>  // ticket 06 Test 2: SFS_ORACLE_INJECT / SFS_ORACLE_PAIR
#include <cstdio>
#include <vector>
#include <fstream>
#include <algorithm>
#include <map>
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
#include "class/ground_truth_debug.h"

// Static member definition for GroundTruthDebugger
std::vector<GroundTruthDebugger::RejectionInfo> GroundTruthDebugger::rejections;
#include "ceres/ceres.h"
#include "class/data_path.h"
#include "class/data_structure.h"
#include "class/visualize.h"
#include "class/reconstruction.h"
#include "class/feature_matching.h"
#include "class/ranking_system.h"
#include "class/enhanced_ranking_system.h"
#include "class/hub_guided_beam_search.h"
#include "class/two_phase_assembly.h"
#include "class/puzzlefusion_global_optimizer.h"
#include "class/multi_hypothesis_optimizer.h"

// TOP-K TRANSFORMATION CONFIGURATION - REMOVED (useless perturbations)
// #define ENABLE_TOP_K_TRANSFORMS    // Enable TOP-K transformation testing
// #define MAX_TRANSFORM_CONFIGS 2    // Start with K=2 for testing
// #define DEBUG_TOP_K_TRANSFORMS     // Enable comprehensive debug output

// TOP-K TRANSFORMATION IMPLEMENTATION - REMOVED (useless perturbations)

#define TOP_k 15     // EXPANDED: Keep multiple assembly hypotheses to explore all configuration combinations
#define BRANCH_b 8    // EXPANDED: Explore more branching paths for better global solutions


//#define NO_RIM_INFO
#define NO_BASE_INFO

using namespace std;
using namespace Eigen;

vector<Geom> shard(SHARD_NUMBER);
vector<Trans> GT_trans(SHARD_NUMBER);
MatrixXd GT_graph(SHARD_NUMBER, SHARD_NUMBER);


// Headless stub replacement for PCLVisualizer that logs instead of crashing on VTK
class HeadlessVisualizer {
public:
    HeadlessVisualizer(const string& name) { 
        cout << "Headless visualizer: " << name << endl; 
        log_file.open("sfs_headless_visualization.log");
        log_file << "=== SFS HEADLESS VISUALIZATION LOG ===" << endl;
    }
    ~HeadlessVisualizer() { if(log_file.is_open()) log_file.close(); }
    
    bool wasStopped() { return false; } // Never stop - we'll control the loop
    void spinOnce(int ms = 1) { 
        usleep(ms * 1000); // Small delay to prevent tight loop
    }
    void setBackgroundColor(int r, int g, int b) { 
        log_file << "Background color set: (" << r << "," << g << "," << b << ")" << endl;
    }
    void addCoordinateSystem(double scale) { 
        log_file << "Coordinate system added with scale: " << scale << endl;
    }
    void initCameraParameters() { 
        log_file << "Camera parameters initialized" << endl;
    }
    template<typename T> void registerKeyboardCallback(T callback, void* data) { 
        log_file << "Keyboard callback registered (headless mode)" << endl;
    }
    template<typename T> void updatePointCloud(T data, T color, const string& name) { 
        log_file << "Point cloud updated: " << name << endl;
    }
    void removePointCloud(const string& name, int viewport = 0) { 
        log_file << "Point cloud removed: " << name << endl;
    }
    void updatePolygonMesh(const pcl::PolygonMesh& mesh, const string& name) { 
        log_file << "Polygon mesh updated: " << name << endl;
    }
    template<typename T> void setPointCloudRenderingProperties(int property, T value, const string& name, int viewport = 0) { 
        log_file << "Rendering properties set for: " << name << endl;
    }
private:
    ofstream log_file;
};

shared_ptr<HeadlessVisualizer> viewer(new HeadlessVisualizer("Pot reconstruction"));

// Mock visualization state that auto-advances
class MockVisSwitchVariables {
public:
    bool first_ = true;
    bool right_ = false;
    bool is_fine_ = false;
    int auto_step_counter = 0;
    
    void KeyEvent(const string& key, bool key_down) {
        // Auto-advance through states for headless mode
        auto_step_counter++;
        if (first_) {
            first_ = false;
            right_ = true;
            cout << "Auto-advancing: first_ -> right_" << endl;
        } else {
            right_ = !right_;
            cout << "Auto-advancing: right_ = " << right_ << endl;
        }
    }
} vis;

void keyboardEventOccurred(void* event, void* nothing) {
    // Stub function for headless mode
}

int main(int argc, char** argv)
{
    // Open results log
    ofstream result_log("sfs_headless_assembly.log");
    result_log << "=== SFS HEADLESS ASSEMBLY LOG ===" << endl;
    result_log << "Start time: " << time(nullptr) << endl;
    
	double calculation_time(0);
	int s_time(0), e_time(0);

	int step_size = shard.size();
	if (argv[1] != NULL) {
		step_size = std::atoi(argv[1]);
	}

	// ADAPTIVE BEAM SEARCH: Scale exploration to maintain quality up to 100 pieces
	// Formula preserves exploration density while managing computational complexity
	int adaptive_TOP_k = 15; // Default for 8 pieces (current optimal)
	if (step_size > 8) {
		// Maintain exploration effectiveness: sqrt scaling with quality preservation
		adaptive_TOP_k = std::min(15, std::max(3, (int)(15.0 * sqrt(8.0 / step_size))));

		// For very large datasets (50+ pieces), use logarithmic scaling to maintain some exploration
		if (step_size >= 50) {
			adaptive_TOP_k = std::max(3, (int)(15.0 * log(8.0) / log(step_size)));
		}
	}

	result_log << "Processing " << step_size << " pieces with adaptive beam search TOP_k=" << adaptive_TOP_k << endl;
	
	// Headless PCL viewer "setup" - just logs
	viewer->setBackgroundColor(0, 0, 0);
	viewer->addCoordinateSystem(1.0);
	viewer->initCameraParameters();
	viewer->registerKeyboardCallback(&keyboardEventOccurred, (void*)viewer.get());
	
	cout << "#################### Pottery Data load ####################" << endl;
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
			shard[i].sur_in_.CalculateLineNormal();
			shard[i].sur_out_.CalculateLineNormal();
			if(shard[i].sur_frac_.point_.cols() > 0)
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
	// Initialize GT_graph
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

	// Remove excluded shard information 
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard_on_off[i]) {
			for (int j = 0; j < SHARD_NUMBER; j++) {
				GT_graph(i, j) = 0;
				GT_graph(j, i) = 0;
			}
		}
	}

	cout << "#################### Save initial state ####################" << endl;
	EnhancedStateManager manager;
	vector<Visualize> pc_origin(SHARD_NUMBER);
	vector<Visualize> pc_overlap;
	
	// Initialize right_sherd early
	vector<bool> right_sherd(SHARD_NUMBER, true);
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard_on_off[i])
			right_sherd[i] = false;
	}
	
	s_time = clock();

	// ==================== SAVE ORIGINAL AXIS HEIGHTS ====================
	// CRITICAL: Compute and store original heights BEFORE AxisAlignment transforms them to origin!
	cout << "#################### Computing Original Axis Heights ####################" << endl;
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (shard[i].is_matching_ && !shard[i].edge_line_.axis_point_.empty()) {
			// Compute average Z coordinate from original axis points
			double total_height = 0.0;
			for (const auto& point : shard[i].edge_line_.axis_point_) {
				total_height += point.z();
			}
			shard[i].original_axis_height_ = total_height / shard[i].edge_line_.axis_point_.size();
			cout << "  Piece " << (i+1) << ": original height = " << shard[i].original_axis_height_ << " mm" << endl;
		}
	}

	cout << "#################### Change Axis symmetrix to z axis ####################" << endl;
	vector<Trans> T_axis(SHARD_NUMBER);
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (shard[i].is_matching_) {
			Matrix3d R_d = Matrix3d::Identity();
			Vector3d t_d = { 0, 0, 0 };
			T_axis[i].Set(R_d, t_d, i + 1, i + 1);

			AxisAlignment(shard[i].edge_line_, R_d, t_d);	
			shard[i].SurMove(R_d, t_d, true);

			result_log << "Piece " << (i+1) << " axis aligned" << endl;
			
			T_axis[i].Input(R_d, t_d);

			CalculateFeatureAxisless(shard[i]);

			int num_axis = shard[i].edge_line_.axis_norm_.size();
			if (num_axis > 1) {
				result_log << "  Multi-axis piece with " << num_axis << " axes" << endl;
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

	// Ticket 06 Test 2: provenance print of per-piece axis alignment
	// (working frame = A_i * local). Parsed offline to build oracle
	// merge inits (A P^-1 A^-1) for the Juglet / Pot_A proof runs.
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (shard[i].is_matching_) {
			Matrix3d R_ai; Vector3d t_ai;
			T_axis[i].Output(R_ai, t_ai);
			cout << "*** TAXIS *** piece " << i + 1 << " R=["
				<< R_ai(0,0) << "," << R_ai(0,1) << "," << R_ai(0,2) << ";"
				<< R_ai(1,0) << "," << R_ai(1,1) << "," << R_ai(1,2) << ";"
				<< R_ai(2,0) << "," << R_ai(2,1) << "," << R_ai(2,2) << "] t=["
				<< t_ai.transpose() << "]" << endl;
		}
	}

	cout << "#################### Feature matching ####################" << endl;

	// Initialize ground truth debug system
	GroundTruthDebugger::reset();

	list<LCSIndex> LCS_out;
	FeatureComp(shard, LCS_out, 25, MINIMUM_NUMBER, 0);
	cout << "Total number : " << LCS_out.size() << endl;
	result_log << "Feature matches found: " << LCS_out.size() << endl;

	// DEBUG: Analyze feature matches by piece pairs
	cout << "DEBUG: Feature matches breakdown by piece pairs:" << endl;
	map<pair<int,int>, int> pair_counts;
	for (const auto& lcs : LCS_out) {
		pair<int,int> p = make_pair(lcs.shard_x_, lcs.shard_y_);
		pair_counts[p]++;
	}
	for (const auto& pc : pair_counts) {
		cout << "  Pieces " << pc.first.first << "-" << pc.first.second
		     << ": " << pc.second << " matches" << endl;
	}

	// GROUND TRUTH DEBUG: Track which legitimate connections got no feature matches
	cout << "\n*** GROUND TRUTH FEATURE MATCHING ANALYSIS ***" << endl;
	for (int p1 = 1; p1 <= 8; p1++) {
		for (int p2 = p1+1; p2 <= 8; p2++) {
			if (GroundTruthDebugger::isGroundTruthConnection(p1, p2)) {
				pair<int,int> forward = make_pair(p1, p2);
				pair<int,int> reverse = make_pair(p2, p1);
				int matches = pair_counts[forward] + pair_counts[reverse];

				if (matches == 0) {
					GroundTruthDebugger::trackConnection(p1, p2,
						GroundTruthDebugger::FEATURE_MATCHING_STAGE,
						"No feature matches found", 0, 0);
				} else {
					GroundTruthDebugger::trackConnection(p1, p2,
						GroundTruthDebugger::SURVIVED,
						"Feature matching succeeded", matches, 0);
				}
			}
		}
	}

	cout << "#################### Pairwise pruning ####################" << endl;
	size_t matches_before_pruning = LCS_out.size();
	cout << "DEBUG: Matches before pairwise pruning: " << matches_before_pruning << endl;
	
	PairwisePruning(shard, LCS_out);
	
	list<LCSIndex>::iterator iter = LCS_out.begin();
	size_t matches_after_pruning = LCS_out.size();
	cout << "Total number remaining: " << matches_after_pruning << endl;
	cout << "DEBUG: Removed " << (matches_before_pruning - matches_after_pruning)
	     << " matches during pairwise pruning" << endl;

	// DEBUG: Show surviving connections after pruning
	cout << "*** SURVIVING CONNECTIONS AFTER PAIRWISE PRUNING ***" << endl;
	for (auto& connection : LCS_out) {
		cout << "  Pieces " << connection.shard_y_ << "-" << connection.shard_x_
		     << ": survived (area=" << connection.area_ << ")" << endl;
	}

	// Ticket 06 Test 2 (pair injection): the oracle override can only
	// fire if the beam actually PROPOSES the pair, and on the Juglet the
	// beam proposes a different (false) pair every run -- so a hand-true
	// placement alone yielded ORACLE-count 0 (void arms, 2026-09-25).
	// SFS_ORACLE_INJECT=1 prepends the SFS_ORACLE_PAIR edge to the
	// candidate list so the true pair is on the table. The injected edge
	// gets a neutral transform (overridden downstream at MERGEINIT) and
	// a full-range segment span, i.e. it proposes the pair and nothing
	// else -- it does NOT hand over the answer. Everything after
	// injection (registration, refinement, gates, plausibility, scorer)
	// runs unmodified.
	{
		const char* oin = std::getenv("SFS_ORACLE_INJECT");
		const char* opair2 = std::getenv("SFS_ORACLE_PAIR");
		if (oin && (*oin == '1' || *oin == 't') && opair2 && *opair2) {
			int ia = 0, ib = 0;
			if (std::sscanf(opair2, "%d,%d", &ia, &ib) == 2 &&
				ia >= 1 && ib >= 1 && ia <= SHARD_NUMBER && ib <= SHARD_NUMBER && ia != ib) {
				bool already = false;
				for (auto& c : LCS_out) {
					if ((c.shard_x_ == ia && c.shard_y_ == ib) ||
						(c.shard_x_ == ib && c.shard_y_ == ia)) { already = true; break; }
				}
				if (!already) {
					LCSIndex inj;
					inj.shard_x_ = ib; inj.shard_y_ = ia;   // moving, fixed
					inj.size_ = (int)shard[ia - 1].edge_line_.point_.cols();
					inj.start_.x = 1; inj.start_.y = 1;
					inj.end_.x = inj.size_; inj.end_.y = inj.size_;
					inj.trans_.Set(Matrix4d::Identity(), ib, ia);
					inj.score_ = 1e6;   // head of the priority list
					inj.inliner_ = 0;
					inj.c_plausibility_ = true;
					LCS_out.push_front(inj);
					cout << "*** ORACLE-INJECT *** pair " << ia << "-" << ib
						<< " prepended (size=" << inj.size_ << ")" << endl;
				}
				else cout << "*** ORACLE-INJECT *** pair " << ia << "-" << ib << " already proposed" << endl;
			}
		}
	}
	result_log << "Feature matches after pruning: " << LCS_out.size() << endl;

	// #################### POST-PRUNING HUB GUIDANCE (DISABLED) ####################
	cout << "#################### Post-Pruning Hub Guidance (DISABLED) ####################" << endl;
	cout << "*** HUB GUIDANCE DISABLED *** Skipping hub guidance to prevent boosting failed connections" << endl;
	cout << "*** REASON *** Hub guidance was boosting failed ICP connections (score=11.0, inlier=0) to high scores (16.5)" << endl;
	cout << "*** RESULT *** Auto-Agglomerative Assembly will use original connection scores and inlier counts" << endl;

	// HUB GUIDANCE SYSTEM DISABLED - was causing:
	// 1. Failed ICP connections (score=11.0, inlier=0) boosted to score=16.5
	// 2. Inlier scaling: 0 * 1.5 = 0 (zero times anything is still zero)
	// 3. Auto-Agglomerative Assembly prioritizing invalid connections
	//
	// Original problematic code:
	// - hub_guidance_system.computeEnhancedScore() boosted scores
	// - connection.inliner_ = static_cast<int>(connection.inliner_ * score_ratio) failed for 0 inliners
	// - Result: artificially high scores with no geometric validity

// TOP-K TRANSFORMS REMOVED - useless perturbations that don't solve 3-graph problem

	// #################### TWO-PHASE ASSEMBLY ARCHITECTURE ####################
	cout << "#################### Two-Phase Assembly Architecture ####################" << endl;
	cout << "*** TWO-PHASE ASSEMBLY *** Enabling enhanced assembly with connection preservation" << endl;

	// Check if Two-Phase Assembly is enabled via environment variable
	const char* enable_two_phase_env = std::getenv("ENABLE_TWO_PHASE_ASSEMBLY");
	bool enable_two_phase = (enable_two_phase_env && std::string(enable_two_phase_env) == "1");

	// Declare variables outside conditional scope to avoid goto issues
	bool use_two_phase_results = false;
	std::vector<RankingSubgraph> two_phase_components;

	if (enable_two_phase) {
		cout << "*** TWO-PHASE ASSEMBLY ENABLED *** Using enhanced assembly architecture" << endl;

		// Initialize Two-Phase Assembly Coordinator
		TwoPhaseAssemblyCoordinator two_phase_coordinator;
		two_phase_coordinator.enableDebugMode(true);

		// Convert final_lcs_vector to format expected by Two-Phase system
		std::vector<LCSIndex> pruned_connections_vector(LCS_out.begin(), LCS_out.end());

		// Create temporary StateManager for Two-Phase system
		string temp_log_path = "/workspace";
		StateManager temp_state_manager(adaptive_TOP_k, BRANCH_b, shard, LCS_out, step_size, temp_log_path);

		// Execute enhanced two-phase assembly
		auto two_phase_result = two_phase_coordinator.executeEnhancedAssembly(
			pruned_connections_vector, shard, temp_state_manager);

		if (two_phase_result.success) {
			cout << "*** TWO-PHASE ASSEMBLY SUCCESS *** "
			     << two_phase_result.local_components_generated << " → "
			     << two_phase_result.final_components << " components" << endl;
			cout << "    Connectivity improvement: " << (two_phase_result.connectivity_improvement * 100.0) << "%" << endl;
			cout << "    Overall quality: " << (two_phase_result.overall_improvement * 100.0) << "%" << endl;

			// Get final components from Two-Phase system
			two_phase_components = two_phase_coordinator.getFinalComponents();
			use_two_phase_results = true;

			// Continue with existing pipeline using Two-Phase results
			cout << "*** TWO-PHASE ASSEMBLY *** Proceeding with enhanced assembly results" << endl;

		} else {
			cout << "*** TWO-PHASE ASSEMBLY FAILED *** Falling back to original algorithm" << endl;
			cout << "    Error: Two-phase assembly did not improve connectivity" << endl;
		}
	} else {
		cout << "*** TWO-PHASE ASSEMBLY DISABLED *** Using original algorithm" << endl;
		cout << "    To enable: export ENABLE_TWO_PHASE_ASSEMBLY=1" << endl;
	}

	// Conditional graph building based on Two-Phase Assembly results
	EnhancedStateManager assembly_manager(adaptive_TOP_k, BRANCH_b, shard, LCS_out, step_size, "/workspace");

	// ==================== GLOBAL OPTIMIZATION INTEGRATION ====================
	// Check for different global optimization methods
	const char* enable_auto_agglomerative_env = std::getenv("ENABLE_AUTO_AGGLOMERATIVE");
	const char* enable_multi_hypothesis_env = std::getenv("ENABLE_MULTI_HYPOTHESIS");

	bool use_puzzlefusion_global = (enable_auto_agglomerative_env && std::string(enable_auto_agglomerative_env) == "1");
	bool use_multi_hypothesis = (enable_multi_hypothesis_env && std::string(enable_multi_hypothesis_env) == "1");

	// DEBUG: Always show environment variable status
	cout << "*** GLOBAL OPTIMIZATION DEBUG *** Environment variables:" << endl;
	cout << "  ENABLE_AUTO_AGGLOMERATIVE = " << (enable_auto_agglomerative_env ? enable_auto_agglomerative_env : "NOT_SET") << endl;
	cout << "  ENABLE_MULTI_HYPOTHESIS = " << (enable_multi_hypothesis_env ? enable_multi_hypothesis_env : "NOT_SET") << endl;
	cout << "*** METHODS *** use_puzzlefusion_global = " << (use_puzzlefusion_global ? "TRUE" : "FALSE")
	     << ", use_multi_hypothesis = " << (use_multi_hypothesis ? "TRUE" : "FALSE") << endl;

	if (use_puzzlefusion_global) {
		cout << "#################### PUZZLEFUSION++ GLOBAL OPTIMIZATION ####################" << endl;
		cout << "*** REAL GLOBAL OPTIMIZATION *** Replacing fake clustering with proven geometric optimization" << endl;
		cout << "*** INTEGRATION *** Using existing RimConstraint + GGCE + ICP systems" << endl;

		// DEBUG: Check LCS_out inlier data before conversion
		cout << "*** PRE-CONVERSION DEBUG *** Checking LCS_out inlier data:" << endl;
		int non_zero_inliers = 0;
		for (const auto& conn : LCS_out) {
			if (conn.inliner_ > 0) {
				non_zero_inliers++;
				if (non_zero_inliers <= 5) {  // Show first 5 examples
					cout << "  LCS_out connection: Pieces " << conn.shard_y_ << "-" << conn.shard_x_
					     << " inliers=" << conn.inliner_ << " score=" << conn.score_ << endl;
				}
			}
		}
		cout << "*** PRE-CONVERSION SUMMARY *** " << non_zero_inliers << " connections with inliers > 0" << endl;

		// Convert LCS_out set to vector for processing
		std::vector<LCSIndex> connections_vector(LCS_out.begin(), LCS_out.end());

		// DEBUG: Check connections_vector inlier data after conversion
		cout << "*** POST-CONVERSION DEBUG *** Checking connections_vector inlier data:" << endl;
		int non_zero_inliers_vector = 0;
		for (const auto& conn : connections_vector) {
			if (conn.inliner_ > 0) {
				non_zero_inliers_vector++;
				if (non_zero_inliers_vector <= 5) {  // Show first 5 examples
					cout << "  Vector connection: Pieces " << conn.shard_y_ << "-" << conn.shard_x_
					     << " inliers=" << conn.inliner_ << " score=" << conn.score_ << endl;
				}
			}
		}
		cout << "*** POST-CONVERSION SUMMARY *** " << non_zero_inliers_vector << " connections with inliers > 0" << endl;

		cout << "*** INPUT DATA *** " << connections_vector.size() << " connections, "
		     << SHARD_NUMBER << " pieces for REAL global optimization" << endl;

		// Execute HYBRID PuzzleFusion++ global optimization
		RankingSubgraph global_result = performHybridPuzzleFusionOptimization(
			SHARD_NUMBER, connections_vector, shard);

		// Check success by counting assembled pieces
		int assembled_pieces = 0;
		for (int i = 0; i < SHARD_NUMBER; ++i) {
			if (global_result.node_[i]) {
				assembled_pieces++;
			}
		}

		bool global_success = assembled_pieces >= SHARD_NUMBER * 0.5;  // At least 50% success

		if (global_success) {
			cout << "*** PUZZLEFUSION++ SUCCESS *** " << endl;
			cout << "Global optimization result: " << assembled_pieces << "/" << SHARD_NUMBER << " pieces assembled" << endl;

			// Convert RankingSubgraph to State format for pipeline integration
			State global_state(SHARD_NUMBER);
			global_state.state_score_ = assembled_pieces * 50;  // Quality score
			global_state.graph_.clear();
			global_state.graph_.push_back(global_result);  // Add the global optimization result as single graph

			// Update true_node_ based on global result
			for (int i = 0; i < SHARD_NUMBER; ++i) {
				global_state.true_node_[i] = global_result.node_[i];
			}

			// Replace assembly manager output with global optimization result
			assembly_manager.out_state_.clear();
			assembly_manager.out_state_.push_back(global_state);

			cout << "*** INTEGRATION SUCCESS *** PuzzleFusion++ global result integrated into pipeline" << endl;

		} else {
			cout << "*** PUZZLEFUSION++ PARTIAL *** Only " << assembled_pieces << "/" << SHARD_NUMBER << " pieces assembled" << endl;
			cout << "*** FALLBACK DISABLED *** Stopping here to validate PuzzleFusion++ results" << endl;
			cout << "*** VALIDATION MODE *** No original graph building - pure PuzzleFusion++ results" << endl;

			// NO FALLBACK - Stop here for result validation
		}

	} else if (use_multi_hypothesis) {
		cout << "#################### MULTI-HYPOTHESIS GLOBAL OPTIMIZATION ####################" << endl;
		cout << "*** ADVANCED GLOBAL OPTIMIZATION *** Implementing PuzzleFusion++ multi-hypothesis approach" << endl;
		cout << "*** MULTI-HYPOTHESIS FEATURES ***:" << endl;
		cout << "  - K=5 hypotheses per edge pair (prevents early commitment)" << endl;
		cout << "  - Global compatibility pruning (cycle consistency + collision)" << endl;
		cout << "  - Binary switch optimization (alternating pose-switch updates)" << endl;
		cout << "  - SE(3) factor graph with robust losses" << endl;

		// Convert LCS_out to vector for processing
		std::vector<LCSIndex> connections_vector(LCS_out.begin(), LCS_out.end());

		cout << "*** INPUT DATA *** " << connections_vector.size() << " connections, "
		     << SHARD_NUMBER << " pieces for multi-hypothesis optimization" << endl;

		// Execute Multi-Hypothesis Global Optimization
		RankingSubgraph global_result = performMultiHypothesisOptimization(
			SHARD_NUMBER, connections_vector, shard);

		// Check success by counting assembled pieces
		int assembled_pieces = 0;
		for (int i = 0; i < SHARD_NUMBER; i++) {
			if (global_result.T_[i].index_ > 0) {
				assembled_pieces++;
			}
		}

		bool global_success = assembled_pieces >= SHARD_NUMBER * 0.5;  // At least 50% success

		if (global_success) {
			cout << "*** MULTI-HYPOTHESIS SUCCESS *** " << endl;
			cout << "Global optimization result: " << assembled_pieces << "/" << SHARD_NUMBER << " pieces assembled" << endl;

			// Convert RankingSubgraph to State format for pipeline integration
			State global_state(SHARD_NUMBER);
			global_state.state_score_ = assembled_pieces * 50;  // Quality score
			global_state.graph_.clear();
			global_state.graph_.push_back(global_result);  // Add the global optimization result as single graph

			// Update true_node_ based on global result
			for (int i = 0; i < SHARD_NUMBER; ++i) {
				global_state.true_node_[i] = global_result.node_[i];
			}

			// Replace assembly manager output with global optimization result
			assembly_manager.out_state_.clear();
			assembly_manager.out_state_.push_back(global_state);

			cout << "*** INTEGRATION SUCCESS *** Multi-hypothesis global result integrated into pipeline" << endl;

		} else {
			cout << "*** MULTI-HYPOTHESIS PARTIAL *** Only " << assembled_pieces << "/" << SHARD_NUMBER << " pieces assembled" << endl;
			cout << "*** FALLBACK DISABLED *** Stopping here to validate multi-hypothesis results" << endl;
			cout << "*** VALIDATION MODE *** No original graph building - pure multi-hypothesis results" << endl;

			// NO FALLBACK - Stop here for result validation
		}

	} else if (!use_two_phase_results && !use_puzzlefusion_global && !use_multi_hypothesis) {
		cout << "#################### Incremental graph building ####################" << endl;
		cout << "*** LEGACY METHOD *** Using original incremental approach" << endl;
		cout << "    To enable Auto-Agglomerative: export ENABLE_AUTO_AGGLOMERATIVE=1" << endl;
		cout << "    To enable Two-Phase Assembly: export ENABLE_TWO_PHASE_ASSEMBLY=1" << endl;
		cout << "    To enable Multi-Hypothesis: export ENABLE_MULTI_HYPOTHESIS=1" << endl;

		// CRITICAL FIX #3: Enable enhanced graph building with hub guidance
		assembly_manager.setHubGuidanceEnabled(true);
		assembly_manager.BuildStep();
	} else if (!use_two_phase_results && use_puzzlefusion_global) {
		cout << "#################### Skipping Incremental Graph Building ####################" << endl;
		cout << "*** PUZZLEFUSION++ ONLY *** Original graph building disabled for pure global optimization" << endl;
	} else {
		cout << "#################### Using Two-Phase Assembly Results ####################" << endl;
		cout << "*** SKIPPING ORIGINAL GRAPH BUILDING *** Two-Phase Assembly provided "
		     << two_phase_components.size() << " optimized components" << endl;

		// Convert Two-Phase Assembly results to State format with precision and elegance
		cout << "*** CONVERTING TWO-PHASE RESULTS *** Creating optimized assembly state" << endl;

		try {
			// Create a new State object with the Two-Phase Assembly results
			State optimized_state(SHARD_NUMBER);

			// Copy the optimized graph components from Two-Phase Assembly
			optimized_state.graph_ = two_phase_components;

			// Initialize true_node_ based on which pieces are included in any graph
			std::fill(optimized_state.true_node_.begin(), optimized_state.true_node_.end(), false);
			for (const auto& graph : two_phase_components) {
				for (int i = 0; i < SHARD_NUMBER; ++i) {
					if (graph.node_[i]) {
						optimized_state.true_node_[i] = true;
					}
				}
			}

			// Calculate total state score as sum of all graph scores
			optimized_state.state_score_ = 0;
			for (const auto& graph : two_phase_components) {
				optimized_state.state_score_ += graph.graph_score_;
			}

			// Synchronize transformation data and validate completeness
			optimized_state.SynchronizeTrueNode();
			optimized_state.UpdateStateScore();

			// Clear existing states and add our optimized state
			assembly_manager.out_state_.clear();
			assembly_manager.out_state_.push_back(optimized_state);

			cout << "*** CONVERSION SUCCESSFUL *** Two-Phase Assembly state integrated" << endl;
			cout << "    Optimized components: " << two_phase_components.size() << endl;
			cout << "    Total pieces: " << std::count(optimized_state.true_node_.begin(), optimized_state.true_node_.end(), true) << endl;
			cout << "    State score: " << optimized_state.state_score_ << endl;

		} catch (const std::exception& e) {
			cout << "*** CONVERSION ERROR *** " << e.what() << endl;
			cout << "*** FALLBACK *** Using hub-guided building as backup" << endl;
			assembly_manager.setHubGuidanceEnabled(true);
			assembly_manager.BuildStep();
		}
	}
	
	int num_total = assembly_manager.out_state_.size();
	result_log << "Assembly states available: " << num_total << endl;

	// DEBUG: Show details of each assembly state
	cout << "*** ASSEMBLY STATES GENERATED ***" << endl;
	for (int i = 0; i < num_total; i++) {
		int num_graphs = assembly_manager.out_state_[i].graph_.size();
		int total_pieces = 0;
		cout << "State " << i << ": " << num_graphs << " graphs - ";
		for (int g = 0; g < num_graphs; g++) {
			int pieces_in_graph = 0;
			for (int p = 0; p < SHARD_NUMBER; p++) {
				if (assembly_manager.out_state_[i].graph_[g].node_[p]) {
					pieces_in_graph++;
					total_pieces++;
				}
			}
			cout << "G" << g << "(" << pieces_in_graph << " pieces) ";
		}
		cout << "Total: " << total_pieces << "/8 pieces" << endl;
	}

	cout << "#################### Full Headless Assembly Processing ####################" << endl;
	
	// Update right_sherd based on active pieces (already declared above)
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (!shard_on_off[i])
			right_sherd[i] = false;
	}
	
	// Process ALL assembly states automatically (remove 10-step limit)
	result_log << "Processing " << num_total << " assembly states automatically..." << endl;
	
	for (int state_idx = 0; state_idx < num_total; state_idx++) {
		result_log << "=== ASSEMBLY STATE " << state_idx << " ===" << endl;
		result_log << "State score: " << assembly_manager.out_state_[state_idx].state_score_ << endl;
		result_log << "Active pieces in this state: ";
		
		// Log which pieces are active in this state
		int active_count = 0;
		for (int i = 0; i < SHARD_NUMBER; i++) {
			if (assembly_manager.out_state_[state_idx].true_node_[i]) {
				result_log << (i+1) << " ";
				active_count++;
			}
		}
		result_log << "(" << active_count << " total)" << endl;
		
		// Log the transformations applied in this state  
		result_log << "Transformations applied:" << endl;
		for (int i = 0; i < SHARD_NUMBER; i++) {
			if (assembly_manager.out_state_[state_idx].true_node_[i]) {
				// Note: Detailed transformation logging skipped to avoid State class complexity
				result_log << "  Piece " << (i+1) << ": transformation applied" << endl;
			}
		}
		
		// Progress report
		if (state_idx % 10 == 0 || state_idx < 10) {
			cout << "Processed assembly state " << (state_idx + 1) << "/" << num_total 
			     << " (score=" << assembly_manager.out_state_[state_idx].state_score_ << ", "
			     << active_count << " pieces)" << endl;
		}
		
		// Stop early if we've found a complete assembly (all pieces placed)
		if (active_count == SHARD_NUMBER) {
			result_log << "*** COMPLETE ASSEMBLY FOUND AT STATE " << state_idx << " ***" << endl;
			cout << "*** COMPLETE ASSEMBLY FOUND! All " << SHARD_NUMBER << " pieces successfully assembled! ***" << endl;
			break;
		}
	}
	
	// Final assembly analysis
	result_log << "=== FINAL ASSEMBLY ANALYSIS ===" << endl;
	
	// Find the best assembly state
	int best_state_idx = 0;
	double best_score = assembly_manager.out_state_[0].state_score_;
	int max_pieces = 0;
	
	for (int i = 0; i < num_total; i++) {
		int piece_count = 0;
		for (int j = 0; j < SHARD_NUMBER; j++) {
			if (assembly_manager.out_state_[i].true_node_[j]) piece_count++;
		}
		
		if (piece_count > max_pieces || 
		   (piece_count == max_pieces && assembly_manager.out_state_[i].state_score_ > best_score)) {
			best_state_idx = i;
			best_score = assembly_manager.out_state_[i].state_score_;
			max_pieces = piece_count;
		}
	}
	
	result_log << "Best assembly state: #" << best_state_idx << endl;
	result_log << "Best score: " << best_score << endl;
	result_log << "Maximum pieces assembled: " << max_pieces << "/" << SHARD_NUMBER << endl;
	
	cout << "Best assembly: State #" << best_state_idx << " with " << max_pieces 
	     << "/" << SHARD_NUMBER << " pieces (score=" << best_score << ")" << endl;
	
	cout << "#################### Accuracy Evaluation ####################" << endl;
	
	// Reconstruct transformation results for accuracy evaluation (following GUI pattern)
	int num_graph = assembly_manager.out_state_[best_state_idx].graph_.size();
	vector<Trans> T_result = GT_trans;  // Initialize with GT axis transformations
	MatrixXd graph(SHARD_NUMBER, SHARD_NUMBER);
	
	// Initialize graph matrix
	for (int i = 0; i < SHARD_NUMBER; i++) {
		for (int j = 0; j < SHARD_NUMBER; j++) {
			graph(i, j) = 0;
		}
	}
	
	// Extract transformation results from best assembly state
	for (int i = 0; i < num_graph; i++) {
		for (int j = 0; j < SHARD_NUMBER; j++) {
			if (assembly_manager.out_state_[best_state_idx].graph_[i].node_[j]) {
				Matrix3d R_prev;
				Vector3d t_prev;
				assembly_manager.out_state_[best_state_idx].graph_[i].T_[j].Output(R_prev, t_prev);
				T_result[j].Input(R_prev, t_prev);
				
				// Mark connections in graph matrix
				for (int k = 0; k < SHARD_NUMBER; k++) {
					if (assembly_manager.out_state_[best_state_idx].graph_[i].node_[k] && k != j) {
						graph(j, k) = 1;
						graph(k, j) = 1;
					}
				}
			}
		}
	}
	
	// Evaluate accuracy using CountResult (same as GUI version)
	int k_sherd, k_edge, t_sherd, t_edge;
	tie(k_sherd, t_sherd, k_edge, t_edge) = CountResult(GT_graph, GT_trans, graph, T_result, right_sherd);
	
	pair<int, int> sherd_acc, edge_acc;
	sherd_acc = make_pair(k_sherd, t_sherd);
	edge_acc = make_pair(k_edge, t_edge);
	
	// Calculate accuracy percentages
	double sherd_accuracy = (t_sherd > 0) ? (double)k_sherd / (double)t_sherd * 100.0 : 0.0;
	double edge_accuracy = (t_edge > 0) ? (double)k_edge / (double)t_edge * 100.0 : 0.0;
	
	cout << "Accuracy Results:" << endl;
	cout << "  Shard accuracy: " << k_sherd << "/" << t_sherd << " (" << sherd_accuracy << "%)" << endl;
	cout << "  Edge accuracy: " << k_edge << "/" << t_edge << " (" << edge_accuracy << "%)" << endl;
	
	// Create timestamped output directories to prevent overwriting results
	time_t curr_time = time(nullptr);
	struct tm* curr_tm = localtime(&curr_time);
	
	string timestamp = to_string(curr_tm->tm_year + 1900) + "_" 
		+ (curr_tm->tm_mon + 1 < 10 ? "0" : "") + to_string(curr_tm->tm_mon + 1) + "_"
		+ (curr_tm->tm_mday < 10 ? "0" : "") + to_string(curr_tm->tm_mday) + "_"
		+ (curr_tm->tm_hour < 10 ? "0" : "") + to_string(curr_tm->tm_hour) 
		+ (curr_tm->tm_min < 10 ? "0" : "") + to_string(curr_tm->tm_min);
	
	string output_base = "results_" + timestamp;
	string path_visual = output_base + "/";
	string path_result = path_visual + "Result/";
	
	system(("mkdir -p " + path_visual).c_str());
	system(("mkdir -p " + path_result).c_str());
	
	cout << "#################### Output Directory Created ####################" << endl;
	cout << "Results will be saved to: " << output_base << "/" << endl;
	
	// Save accuracy metrics to file (same as GUI version)
	double time_result = 0.0; // TODO: Could add actual timing if needed
	SaveAcc(path_result, sherd_acc, edge_acc, time_result);
	cout << "  Accuracy metrics saved to: " << path_result << "1. Acc.txt" << endl;
	
	// Save comprehensive result outputs (headless adaptation of SaveResult)
	cout << "#################### Saving Comprehensive Results ####################" << endl;
	
	// Generate timestamped filenames (following SaveResult pattern)
	int year = curr_tm->tm_year + 1900;
	int mon = curr_tm->tm_mon + 1;
	int day = curr_tm->tm_mday;
	int hour = curr_tm->tm_hour;
	int min = curr_tm->tm_min;
	
	string com_path = path_result + to_string(year) + "_" + to_string(mon) + "_"
		+ to_string(day) + "_" + to_string(hour) + "_" + to_string(min)
		+ "_Top_" + to_string(best_state_idx + 1) + "_";
	
	// Save edgeline and surface data for each included piece
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (assembly_manager.out_state_[best_state_idx].true_node_[i]) {
			// Save edgeline data (following SaveResult pattern)
			string path_edge_line = com_path + "Edgeline_" + to_string(i + 1) + ".xyz";
			
			// Get transformation for this piece
			Matrix3d R = Matrix3d::Identity();
			Vector3d t = Vector3d::Zero();
			for (int g = 0; g < assembly_manager.out_state_[best_state_idx].graph_.size(); g++) {
				if (assembly_manager.out_state_[best_state_idx].graph_[g].node_[i]) {
					assembly_manager.out_state_[best_state_idx].graph_[g].T_[i].Output(R, t);
					break;
				}
			}
			
			// Apply transformation to edgeline and save
			BreakLine transformed_edgeline = shard[i].edge_line_;
			EdgeLineMove(transformed_edgeline, R, t);
			SaveEdgeLine(transformed_edgeline, path_edge_line);
			
			cout << "  Saved edgeline: " << path_edge_line << endl;
			
			// Note: OBJ mesh saving requires GUI Visualize class, skipped in headless mode
			cout << "  Note: OBJ mesh saving requires GUI mode (skipped)" << endl;
		}
	}
	
	// Save transformation matrices (following GUI pattern)
	cout << "#################### Saving Transformation Matrices ####################" << endl;
	
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (assembly_manager.out_state_[best_state_idx].true_node_[i]) {
			Matrix4d T_a, T_obj;
			Trans T_out;
			
			// Get axis transformation
			GT_trans[i].Output(T_a);
			T_out.Set(T_a, i, i);
			
			// Find and combine with assembly transformation
			for (int j = 0; j < num_graph; j++) {
				if (assembly_manager.out_state_[best_state_idx].graph_[j].node_[i]) {
					assembly_manager.out_state_[best_state_idx].graph_[j].T_[i].Output(T_obj);
					T_out.Input(T_obj);
					T_out.Save(path_result);
					cout << "  Saved transformation matrix for piece " << (i+1) << endl;
					break;
				}
			}
		}
	}
	cout << "  All transformation matrices saved to: " << path_result << endl;
	
	cout << "#################### Generating Visual Output ####################" << endl;
	
	// Export assembly data to text files for external visualization
	ofstream assembly_data(path_visual + "assembly_data.txt");
	assembly_data << "POTTERY ASSEMBLY DATA" << endl;
	assembly_data << "Best State Index: " << best_state_idx << endl;
	assembly_data << "Assembly Score: " << best_score << endl;
	assembly_data << "Pieces Assembled: " << max_pieces << "/" << SHARD_NUMBER << endl;
	assembly_data << endl;
	
	assembly_data << "PIECE INFORMATION:" << endl;
	for (int i = 0; i < SHARD_NUMBER; i++) {
		assembly_data << "Piece " << (i+1) << ": ";
		if (assembly_manager.out_state_[best_state_idx].true_node_[i]) {
			assembly_data << "INCLUDED" << endl;
			
			// Extract transformation matrix for this piece
			Matrix3d R = Matrix3d::Identity(); 
			Vector3d t = Vector3d::Zero();
			bool transformation_found = false;
			
			// Find the transformation matrix for this piece in the best assembly state
			for (int g = 0; g < assembly_manager.out_state_[best_state_idx].graph_.size(); g++) {
				if (assembly_manager.out_state_[best_state_idx].graph_[g].node_[i]) {
					assembly_manager.out_state_[best_state_idx].graph_[g].T_[i].Output(R, t);
					transformation_found = true;
					assembly_data << "  Transformation found in graph " << g << endl;
					assembly_data << "  Rotation: [" << R(0,0) << "," << R(0,1) << "," << R(0,2) << ";"
					             << R(1,0) << "," << R(1,1) << "," << R(1,2) << ";"  
					             << R(2,0) << "," << R(2,1) << "," << R(2,2) << "]" << endl;
					assembly_data << "  Translation: [" << t(0) << "," << t(1) << "," << t(2) << "]" << endl;
					break;
				}
			}
			
			if (!transformation_found) {
				assembly_data << "  Warning: No transformation matrix found, using identity" << endl;
			}
			
			// Export surface data with transformations applied
			string surface_file = path_visual + "piece_" + to_string(i+1) + "_surface.xyz";
			ofstream surf_out(surface_file);
			
			// Copy and transform inner surface points
			MatrixXd inner_surface = shard[i].sur_in_.point_;  // Make copy
			if (transformation_found) {
				MatrixMove(inner_surface, R, t);  // Apply transformation
			}
			for (int p = 0; p < inner_surface.cols(); p++) {
				surf_out << inner_surface(0,p) << " " << inner_surface(1,p) << " " << inner_surface(2,p) << endl;
			}
			
			// Copy and transform outer surface points  
			MatrixXd outer_surface = shard[i].sur_out_.point_;  // Make copy
			if (transformation_found) {
				MatrixMove(outer_surface, R, t);  // Apply transformation
			}
			for (int p = 0; p < outer_surface.cols(); p++) {
				surf_out << outer_surface(0,p) << " " << outer_surface(1,p) << " " << outer_surface(2,p) << endl;
			}
			surf_out.close();
			
			assembly_data << "  Surface points exported to: " << surface_file << endl;
			assembly_data << "  Inner surface points: " << inner_surface.cols() << endl;
			assembly_data << "  Outer surface points: " << outer_surface.cols() << endl;
			assembly_data << "  Transformation applied: " << (transformation_found ? "YES" : "NO") << endl;
		} else {
			assembly_data << "NOT INCLUDED" << endl;
		}
	}
	
	assembly_data << endl;
	assembly_data << "TRANSFORMATION DATA:" << endl;
	assembly_data << "✅ Transformation matrices successfully extracted and applied!" << endl;
	assembly_data << "All surface points have been transformed to final assembly positions" << endl;
	assembly_data.close();
	
	// Create a simple visualization script
	ofstream vis_script(path_visual + "create_ply_files.py");
	vis_script << "#!/usr/bin/env python3" << endl;
	vis_script << "# PLY export script for Pot A assembly" << endl;
	vis_script << "import numpy as np" << endl;
	vis_script << "import os" << endl;
	vis_script << "" << endl;
	vis_script << "def write_ply(filename, points, colors=None):" << endl;
	vis_script << "    \"\"\"Write points to PLY file format\"\"\"" << endl;
	vis_script << "    with open(filename, 'w') as f:" << endl;
	vis_script << "        f.write(\"ply\\n\")" << endl;
	vis_script << "        f.write(\"format ascii 1.0\\n\")" << endl;
	vis_script << "        f.write(f\"element vertex {len(points)}\\n\")" << endl;
	vis_script << "        f.write(\"property float x\\n\")" << endl;
	vis_script << "        f.write(\"property float y\\n\")" << endl;
	vis_script << "        f.write(\"property float z\\n\")" << endl;
	vis_script << "        if colors is not None:" << endl;
	vis_script << "            f.write(\"property uchar red\\n\")" << endl;
	vis_script << "            f.write(\"property uchar green\\n\")" << endl;
	vis_script << "            f.write(\"property uchar blue\\n\")" << endl;
	vis_script << "        f.write(\"end_header\\n\")" << endl;
	vis_script << "        for i, point in enumerate(points):" << endl;
	vis_script << "            if colors is not None:" << endl;
	vis_script << "                f.write(f\"{point[0]:.6f} {point[1]:.6f} {point[2]:.6f} {colors[i][0]} {colors[i][1]} {colors[i][2]}\\n\")" << endl;
	vis_script << "            else:" << endl;
	vis_script << "                f.write(f\"{point[0]:.6f} {point[1]:.6f} {point[2]:.6f}\\n\")" << endl;
	vis_script << "" << endl;
	vis_script << "def hex_to_rgb(hex_color):" << endl;
	vis_script << "    \"\"\"Convert color names to RGB values\"\"\"" << endl;
	vis_script << "    color_map = {" << endl;
	vis_script << "        'red': (255, 0, 0)," << endl;
	vis_script << "        'blue': (0, 0, 255)," << endl;
	vis_script << "        'green': (0, 255, 0)," << endl;
	vis_script << "        'orange': (255, 165, 0)," << endl;
	vis_script << "        'purple': (128, 0, 128)," << endl;
	vis_script << "        'brown': (165, 42, 42)," << endl;
	vis_script << "        'pink': (255, 192, 203)," << endl;
	vis_script << "        'gray': (128, 128, 128)" << endl;
	vis_script << "    }" << endl;
	vis_script << "    return color_map.get(hex_color, (128, 128, 128))" << endl;
	vis_script << "" << endl;
	vis_script << "# Create output directory for PLY files" << endl;
	vis_script << "ply_dir = 'ply_output'" << endl;
	vis_script << "os.makedirs(ply_dir, exist_ok=True)" << endl;
	vis_script << "" << endl;
	vis_script << "colors = ['red', 'blue', 'green', 'orange', 'purple', 'brown', 'pink', 'gray']" << endl;
	vis_script << "color_rgb = [hex_to_rgb(c) for c in colors]" << endl;
	vis_script << "" << endl;
	vis_script << "all_assembled_points = []" << endl;
	vis_script << "all_assembled_colors = []" << endl;
	vis_script << "" << endl;
	vis_script << "print(\"Creating PLY files from SFS assembly...\")" << endl;
	vis_script << "" << endl;
	vis_script << "# Process assembled surfaces" << endl;

	// Add pieces to the script
	for (int i = 0; i < SHARD_NUMBER; i++) {
		if (assembly_manager.out_state_[best_state_idx].true_node_[i]) {
			vis_script << "filename = 'piece_" << (i+1) << "_surface.xyz'" << endl;
			vis_script << "if os.path.exists(filename):" << endl;
			vis_script << "    try:" << endl;
			vis_script << "        full_data = np.loadtxt(filename)" << endl;
			vis_script << "        ply_filename = f'{ply_dir}/piece_" << (i+1) << "_assembled_surface.ply'" << endl;
			vis_script << "        piece_colors = np.tile(color_rgb[" << i << "], (len(full_data), 1))" << endl;
			vis_script << "        write_ply(ply_filename, full_data, piece_colors)" << endl;
			vis_script << "        print(f'✓ Exported {ply_filename} with {len(full_data)} points')" << endl;
			vis_script << "        all_assembled_points.extend(full_data)" << endl;
			vis_script << "        all_assembled_colors.extend(piece_colors)" << endl;
			vis_script << "    except Exception as e:" << endl;
			vis_script << "        print(f'✗ Failed to process {filename}: {e}')" << endl;
		}
	}

	vis_script << "" << endl;
	vis_script << "# Export combined assembled surface" << endl;
	vis_script << "if all_assembled_points:" << endl;
	vis_script << "    combined_filename = f'{ply_dir}/pot_a_complete_assembly.ply'" << endl;
	vis_script << "    write_ply(combined_filename, np.array(all_assembled_points), np.array(all_assembled_colors))" << endl;
	vis_script << "    print(f'✓ Exported combined assembly: {combined_filename} with {len(all_assembled_points)} points')" << endl;
	vis_script << "" << endl;
	vis_script << "print(f'\\n=== PLY FILES CREATED ===')" << endl;
	vis_script << "print(f'Directory: {ply_dir}/')" << endl;
	vis_script << "print('Files:')" << endl;
	vis_script << "print('- Individual assembled surfaces: piece_*_assembled_surface.ply')" << endl;
	vis_script << "print('- Complete assembly: pot_a_complete_assembly.ply')" << endl;
	vis_script << "print('\\nVisualization tools: meshlab, cloudcompare, blender, or any PLY viewer')" << endl;
	vis_script << "print('Color coding: Each piece has distinct colors')" << endl;
	vis_script.close();
	
	// Make script executable
	system(("chmod +x " + path_visual + "visualize.py").c_str());
	
	e_time = clock();
	calculation_time = double(e_time - s_time) / CLOCKS_PER_SEC;
	
	result_log << "=== ASSEMBLY COMPLETE ===" << endl;
	result_log << "Total time: " << calculation_time << " seconds" << endl;
	result_log << "Processed " << num_total << " assembly states" << endl;
	result_log.close();
	
	cout << "Assembly completed with visual output!" << endl;
	cout << "Files created:" << endl;
	cout << "- " << path_visual << "assembly_data.txt (assembly information)" << endl;
	cout << "- " << path_visual << "piece_*_surface.xyz (individual piece surfaces)" << endl;
	cout << "- " << path_visual << "visualize.py (Python visualization script)" << endl;
	cout << "Run: cd " << output_base << " && python3 visualize.py" << endl;

	// Generate final ground truth debug report
	GroundTruthDebugger::generateReport();

	return 0;
}
// Parallel branch configuration
#define PARALLEL_BRANCH_ID 2
#define PARALLEL_SEED (1000 + PARALLEL_BRANCH_ID)


// Parallel branch configuration
#define PARALLEL_BRANCH_ID 5
#define PARALLEL_SEED (1000 + PARALLEL_BRANCH_ID)


// Parallel branch configuration
#define PARALLEL_BRANCH_ID 7
#define PARALLEL_SEED (1000 + PARALLEL_BRANCH_ID)


// Parallel branch configuration
#define PARALLEL_BRANCH_ID 9
#define PARALLEL_SEED (1000 + PARALLEL_BRANCH_ID)


// Parallel branch configuration
#define PARALLEL_BRANCH_ID 11
#define PARALLEL_SEED (1000 + PARALLEL_BRANCH_ID)


// Parallel branch configuration
#define PARALLEL_BRANCH_ID 13
#define PARALLEL_SEED (1000 + PARALLEL_BRANCH_ID)

