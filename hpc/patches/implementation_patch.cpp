/*
TOP-K TRANSFORMATION IMPLEMENTATION PATCH
Concrete implementation file for seamless integration

Usage: Compile this with existing source files
*/

#include "implementation_patch.h"
#include <cmath>

// ============================================================================
// GLOBAL STORAGE INITIALIZATION
// ============================================================================

namespace TopKTransforms {

// Global storage definitions
std::map<std::pair<int,int>, TransformSet> g_transform_sets;
std::map<void*, std::map<std::pair<int,int>, int>> g_state_configs;

// ============================================================================
// ENHANCED TRANSAVERAGE IMPLEMENTATION
// ============================================================================

void EnhancedTransAverage(int current_node,
    vector<LCSIndex>& edges,
    Matrix3d& R,
    Vector3d& t,
    const State* state)
{
    // PRESERVE EXACT ORIGINAL LOGIC - only change transformation selection
    int num_edge = edges.size();
    Vector3d w = { 0, 0, 0 }, t_avg = { 0, 0, 0 };

    R = Matrix3d::Zero();

    Matrix4d T_ref_inv;
    Matrix3d R_ref, R_avg;
    Vector3d t_ref = { 0, 0, 0 };

    // NEW: Get transformation configuration for this piece pair
    int config_index = 0;
    if (state != nullptr) {
        config_index = GetStateTransformConfig(state, current_node, edges[0].shard_y_);
    }

    // Use selected transformation configuration
    const Trans& selected_transform = GetTransformOption(edges[0], config_index);

    if (edges[0].trans_.index_ == current_node) {
        selected_transform.InvOut(T_ref_inv);
        selected_transform.Output(R_ref, t_ref);
    }
    else {
        selected_transform.Output(T_ref_inv);
        selected_transform.InvOut(R_ref, t_ref);
    }

    // CONTINUE WITH ORIGINAL TRANSAVERAGE LOGIC
    R_ref = T_ref_inv.block<3, 3>(0, 0);
    t_ref = T_ref_inv.block<3, 1>(0, 3);

    for (int i = 0; i < num_edge; i++) {
        Matrix4d T_i;
        Matrix3d R_i;
        Vector3d t_i;

        // Get transformation for this edge with selected configuration
        int edge_config = (state != nullptr) ?
            GetStateTransformConfig(state, edges[i].trans_.index_, edges[i].trans_.toward_) : 0;
        const Trans& edge_transform = GetTransformOption(edges[i], edge_config);

        if (edges[i].trans_.index_ == current_node) {
            edge_transform.Output(T_i);
        }
        else {
            edge_transform.InvOut(T_i);
        }

        R_i = T_i.block<3, 3>(0, 0);
        t_i = T_i.block<3, 1>(0, 3);

        Matrix3d R_rel = R_ref.transpose() * R_i;

        // Convert to axis-angle for averaging
        Vector3d w_i = R_rel.eulerAngles(2, 1, 0); // ZYX convention
        w += w_i;

        Vector3d t_rel = R_ref.transpose() * (t_i - t_ref);
        t_avg += t_rel;
    }

    // Average rotations and translations
    w /= num_edge;
    t_avg /= num_edge;

    // Convert back to rotation matrix
    Matrix3d R_avg_rel;
    R_avg_rel = AngleAxisd(w(2), Vector3d::UnitZ()) *
                AngleAxisd(w(1), Vector3d::UnitY()) *
                AngleAxisd(w(0), Vector3d::UnitX());

    R = R_ref * R_avg_rel;
    t = R_ref * t_avg + t_ref;
}

// ============================================================================
// TRANSFORMATION GENERATION
// ============================================================================

void GenerateTransformOptions(LCSIndex& match,
    const vector<CorPair>& correspondences,
    int max_options)
{
    vector<Trans> transform_options;

    // Start with the original best transformation
    transform_options.push_back(match.trans_);

    // Generate geometric variations for additional options
    Matrix3d R_base;
    Vector3d t_base;
    match.trans_.Output(R_base, t_base);

    for (int i = 1; i < max_options && transform_options.size() < max_options; i++) {
        Trans variant_transform;

        // Method 1: Small angular perturbations
        double angle_variation = (i * 3.0) * M_PI / 180.0; // ±3, ±6 degrees
        Vector3d rotation_axis = Vector3d::UnitZ(); // Primary rotation axis

        // Alternate rotation axes for variety
        if (i % 3 == 1) rotation_axis = Vector3d::UnitY();
        if (i % 3 == 2) rotation_axis = Vector3d::UnitX();

        Matrix3d rotation_perturbation = AngleAxisd(angle_variation, rotation_axis).toRotationMatrix();
        Matrix3d R_variant = R_base * rotation_perturbation;

        // Method 2: Small translational perturbations
        Vector3d translation_perturbation;
        translation_perturbation << (i * 1.0), (i * 0.8), (i * 0.5); // mm-scale variations
        Vector3d t_variant = t_base + translation_perturbation;

        variant_transform.Set(R_variant, t_variant, match.shard_x_, match.shard_y_);
        transform_options.push_back(variant_transform);
    }

    // Store all transformation options
    SetTransformOptions(match, transform_options);
}

// ============================================================================
// BEAM SEARCH INTEGRATION
// ============================================================================

void InitializeBuildState(State* state, int state_index, const vector<LCSIndex>& available_edges) {
    InitializeStateConfigs(state, available_edges, state_index);

#ifdef DEBUG_TOP_K_TRANSFORMS
    cout << "State " << state_index << " initialized with configurations:" << endl;
    for (const auto& edge : available_edges) {
        if (HasMultipleConfigs(edge)) {
            int config = GetStateTransformConfig(state, edge.shard_x_, edge.shard_y_);
            cout << "  Pieces " << edge.shard_x_ << "-" << edge.shard_y_
                 << ": using config " << config << endl;
        }
    }
#endif
}

} // namespace TopKTransforms

// ============================================================================
// INTEGRATION HOOKS FOR EXISTING CODE
// ============================================================================

/*
INTEGRATION POINTS:

1. In feature_matching.cpp, after computing transformations:
   TopKTransforms::GenerateTransformOptions(lcs_match, correspondences);

2. In ranking_system.cpp, in BuildState function:
   TopKTransforms::InitializeBuildState(&state, ext_index, available_edges);

3. In ranking_system.cpp, replace TransAverage calls:
   USE_ENHANCED_TRANS_AVERAGE(current_node + 1, edges, R_p, t_p, &state);

4. In State copy constructor/assignment:
   TopKTransforms::CopyStateConfigs(&original_state, &new_state);

5. In State destructor or cleanup:
   TopKTransforms::CleanupStateConfigs(&state);
*/

// ============================================================================
// BACKWARD COMPATIBILITY WRAPPER
// ============================================================================

// Wrapper for original TransAverage function
extern "C" void TransAverage_Original(int current_node,
    vector<LCSIndex>& edges,
    Matrix3d& R,
    Vector3d& t)
{
    // Call enhanced version with null state (uses default configurations)
    TopKTransforms::EnhancedTransAverage(current_node, edges, R, t, nullptr);
}