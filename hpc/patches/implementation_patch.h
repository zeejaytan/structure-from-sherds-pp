/*
TOP-K TRANSFORMATION IMPLEMENTATION PATCH
Concrete header file modifications for seamless integration

Usage: Include this header after existing includes to extend functionality
*/

#ifndef TOP_K_TRANSFORMATION_PATCH_H
#define TOP_K_TRANSFORMATION_PATCH_H

#include <map>
#include <algorithm>

// Configuration - can be controlled via compiler flags
#ifndef MAX_TRANSFORM_CONFIGS
#define MAX_TRANSFORM_CONFIGS 3  // TOP-K transformations per piece pair
#endif

// ============================================================================
// PATCH 1: EXTEND LCSIndex CLASS
// ============================================================================

// Extension methods for LCSIndex - add to existing class via inheritance/composition
namespace TopKTransforms {

struct TransformSet {
    vector<Trans> options;
    int num_configs;

    TransformSet() : num_configs(0) {}

    void AddTransform(const Trans& t) {
        if (options.size() < MAX_TRANSFORM_CONFIGS) {
            options.push_back(t);
            num_configs++;
        }
    }

    const Trans& GetTransform(int index = 0) const {
        if (index >= 0 && index < num_configs && !options.empty()) {
            return options[index];
        }
        static Trans default_trans;
        return default_trans;
    }

    bool HasMultiple() const {
        return num_configs > 1;
    }
};

// Global storage for transformation sets - maps LCSIndex to TransformSet
extern std::map<std::pair<int,int>, TransformSet> g_transform_sets;

// Helper functions to manage transformation sets
inline void SetTransformOptions(const LCSIndex& lcs, const vector<Trans>& options) {
    std::pair<int,int> key = std::make_pair(lcs.shard_x_, lcs.shard_y_);
    TransformSet& set = g_transform_sets[key];

    set.options.clear();
    set.num_configs = 0;

    for (int i = 0; i < std::min(MAX_TRANSFORM_CONFIGS, (int)options.size()); i++) {
        set.AddTransform(options[i]);
    }
}

inline const Trans& GetTransformOption(const LCSIndex& lcs, int config_index = 0) {
    std::pair<int,int> key = std::make_pair(lcs.shard_x_, lcs.shard_y_);
    auto it = g_transform_sets.find(key);
    if (it != g_transform_sets.end()) {
        return it->second.GetTransform(config_index);
    }
    return lcs.trans_; // Fallback to original
}

inline bool HasMultipleConfigs(const LCSIndex& lcs) {
    std::pair<int,int> key = std::make_pair(lcs.shard_x_, lcs.shard_y_);
    auto it = g_transform_sets.find(key);
    return (it != g_transform_sets.end()) && it->second.HasMultiple();
}

} // namespace TopKTransforms

// ============================================================================
// PATCH 2: EXTEND STATE CLASS
// ============================================================================

namespace TopKTransforms {

// Global storage for state transformation configurations
extern std::map<void*, std::map<std::pair<int,int>, int>> g_state_configs;

inline void SetStateTransformConfig(const State* state, int piece_a, int piece_b, int config_index) {
    std::pair<int,int> key = std::make_pair(std::min(piece_a, piece_b), std::max(piece_a, piece_b));
    g_state_configs[(void*)state][key] = config_index;
}

inline int GetStateTransformConfig(const State* state, int piece_a, int piece_b) {
    std::pair<int,int> key = std::make_pair(std::min(piece_a, piece_b), std::max(piece_a, piece_b));
    auto state_it = g_state_configs.find((void*)state);
    if (state_it != g_state_configs.end()) {
        auto config_it = state_it->second.find(key);
        if (config_it != state_it->second.end()) {
            return config_it->second;
        }
    }
    return 0; // Default configuration
}

inline void InitializeStateConfigs(const State* state, const vector<LCSIndex>& edges, int state_index) {
    g_state_configs[(void*)state].clear();

    for (const auto& edge : edges) {
        if (HasMultipleConfigs(edge)) {
            std::pair<int,int> key = std::make_pair(edge.shard_x_, edge.shard_y_);

            // Distribute configurations across beam search states
            auto transform_it = g_transform_sets.find(key);
            if (transform_it != g_transform_sets.end()) {
                int num_configs = transform_it->second.num_configs;
                int config_index = state_index % num_configs;
                g_state_configs[(void*)state][key] = config_index;
            }
        }
    }
}

inline void CopyStateConfigs(const State* from_state, const State* to_state) {
    g_state_configs[(void*)to_state] = g_state_configs[(void*)from_state];
}

inline void CleanupStateConfigs(const State* state) {
    g_state_configs.erase((void*)state);
}

} // namespace TopKTransforms

// ============================================================================
// PATCH 3: ENHANCED FUNCTION SIGNATURES
// ============================================================================

namespace TopKTransforms {

// Enhanced TransAverage that uses selected transformation configuration
void EnhancedTransAverage(int current_node,
    vector<LCSIndex>& edges,
    Matrix3d& R,
    Vector3d& t,
    const State* state = nullptr);

// Generate multiple transformation options for a piece pair
void GenerateTransformOptions(LCSIndex& match,
    const vector<CorPair>& correspondences,
    int max_options = MAX_TRANSFORM_CONFIGS);

// Initialize transformation configurations during beam search
void InitializeBuildState(State* state, int state_index, const vector<LCSIndex>& available_edges);

} // namespace TopKTransforms

// ============================================================================
// PATCH 4: BACKWARD COMPATIBILITY MACROS
// ============================================================================

// Enable/disable TOP-K functionality via compile-time flags
#ifdef ENABLE_TOP_K_TRANSFORMS
#define USE_ENHANCED_TRANS_AVERAGE(node, edges, R, t, state) \
    TopKTransforms::EnhancedTransAverage(node, edges, R, t, state)
#define INIT_TRANSFORM_CONFIGS(state, index, edges) \
    TopKTransforms::InitializeBuildState(state, index, edges)
#else
#define USE_ENHANCED_TRANS_AVERAGE(node, edges, R, t, state) \
    TransAverage(node, edges, R, t)
#define INIT_TRANSFORM_CONFIGS(state, index, edges) \
    // No-op in original mode
#endif

#endif // TOP_K_TRANSFORMATION_PATCH_H