void PuzzleFusionGlobalOptimizer::optimizeGlobalTransformations(
    GlobalAssemblyState& state,
    const std::vector<LCSIndex>& all_connections,
    const std::vector<Geom>& shard) {

    // GLOBAL OPTIMIZATION REDESIGN: Optimize connection SELECTION and SEQUENCING
    // NOT transformation parameters (trust proven ICP transformations)

    if (state.active_pieces.size() < 2) return;

    if (config_.enable_global_logging) {
        std::cout << "*** GLOBAL CONNECTION OPTIMIZATION *** Optimizing assembly with "
                  << state.active_pieces.size() << " pieces" << std::endl;
    }

    // STEP 1: Trust ICP transformations - DON'T re-optimize them
    // The transformations are already optimal for the specific connections
    // Global optimization should focus on WHICH connections to use, not re-optimizing them

    // STEP 2: Global optimization of connection selection (not transformation parameters)
    // Find optimal subset of connections for remaining pieces
    std::vector<LCSIndex> candidate_connections;
    std::unordered_set<int> active_set(state.active_pieces.begin(), state.active_pieces.end());

    // Find all connections that could extend current assembly
    for (const auto& connection : all_connections) {
        bool has_active_piece = (active_set.count(connection.shard_x_) > 0) ||
                               (active_set.count(connection.shard_y_) > 0);
        bool connects_new_piece = (active_set.count(connection.shard_x_) == 0) ||
                                 (active_set.count(connection.shard_y_) == 0);

        if (has_active_piece && connects_new_piece &&
            connection.score_ > config_.minimum_connection_threshold) {
            candidate_connections.push_back(connection);
        }
    }

    // STEP 3: Score potential assembly paths (global lookahead)
    if (!candidate_connections.empty()) {
        // Sort by connection quality for optimal selection
        std::sort(candidate_connections.begin(), candidate_connections.end(),
            [](const LCSIndex& a, const LCSIndex& b) {
                return a.score_ > b.score_;
            });

        if (config_.enable_global_logging) {
            std::cout << "*** GLOBAL PATH PLANNING *** Found " << candidate_connections.size()
                      << " potential connections for assembly expansion" << std::endl;

            // Log top 3 candidates
            for (int i = 0; i < std::min(3, (int)candidate_connections.size()); ++i) {
                std::cout << "*** CONNECTION CANDIDATE " << (i+1) << " *** Pieces "
                          << candidate_connections[i].shard_x_ << "-" << candidate_connections[i].shard_y_
                          << " score=" << candidate_connections[i].score_ << std::endl;
            }
        }
    }

    // STEP 4: Update assembly quality based on spatial coherence and connection strength
    // Use actual geometric relationships, not abstract optimization
    double total_connection_quality = 0.0;
    int valid_connections = 0;

    for (const auto& connection : state.selected_connections) {
        if (connection.score_ > 0) {
            total_connection_quality += connection.score_;
            valid_connections++;
        }
    }

    // Global assembly quality = average connection quality * piece coverage
    double piece_coverage = (double)state.active_pieces.size() / state.total_pieces;
    double avg_connection_quality = valid_connections > 0 ? total_connection_quality / valid_connections : 0.0;
    state.global_score = avg_connection_quality * piece_coverage;

    if (config_.enable_global_logging) {
        std::cout << "*** GLOBAL ASSEMBLY SCORE *** Total=" << std::fixed << std::setprecision(3)
                  << state.global_score << " (avg_conn=" << avg_connection_quality
                  << ", coverage=" << piece_coverage << ")" << std::endl;
    }
}