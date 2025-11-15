#include "connectivity_optimizer.h"
#include <iostream>
#include <queue>
#include <unordered_set>

ConnectivityOptimizer::ConnectivityOptimizer() {
    // Constructor - no initialization needed for stateless design
}

ConnectivityOptimizer::~ConnectivityOptimizer() {
    // Destructor - no cleanup needed
}

double ConnectivityOptimizer::CalculateGlobalConnectivityScore(
    const Chunk& chunk,
    int current_graph_index,
    const std::vector<RankingSubgraph>& all_graphs,
    const std::vector<LCSIndex>& lcs_reference
) {
    // Input validation
    if (current_graph_index < 0 || current_graph_index >= all_graphs.size()) {
        std::cout << "ConnectivityOptimizer Error: Invalid graph index " << current_graph_index << std::endl;
        return 0.0;
    }
    
    if (chunk.i_edge.empty()) {
        return 0.0;  // No connections to evaluate
    }

    int num_pieces = 0;
    for (const auto& graph : all_graphs) {
        num_pieces = std::max(num_pieces, static_cast<int>(graph.node_.size()));
    }
    
    if (num_pieces == 0) {
        return 0.0;
    }

    // Build adjacency matrix for precise connectivity analysis
    std::vector<std::vector<double>> adjacency_matrix = BuildAdjacencyMatrix(lcs_reference, num_pieces);
    
    // Calculate individual metrics with precise weights
    double fragmentation_impact = EvaluateFragmentationImpact(chunk, current_graph_index, all_graphs, lcs_reference);
    double hub_strength = CalculateHubStrength(chunk.node, all_graphs, lcs_reference);
    double connectivity_improvement = PredictConnectivityImprovement(chunk, current_graph_index, all_graphs, lcs_reference);
    double future_potential = EvaluateFutureConnectivityPotential(chunk, current_graph_index, all_graphs, lcs_reference);
    
    // Combine metrics with precise weighting (no averaging!)
    double global_score = 
        fragmentation_impact * FRAGMENTATION_PENALTY_WEIGHT +
        hub_strength * HUB_CONNECTIVITY_WEIGHT +
        connectivity_improvement +
        future_potential * FUTURE_POTENTIAL_WEIGHT;
    
    return global_score;
}

double ConnectivityOptimizer::EvaluateFragmentationImpact(
    const Chunk& chunk,
    int current_graph_index,
    const std::vector<RankingSubgraph>& all_graphs,
    const std::vector<LCSIndex>& lcs_reference
) {
    int num_pieces = all_graphs.empty() ? 0 : all_graphs[0].node_.size();
    if (num_pieces == 0) return 0.0;
    
    // Count current connected components
    int current_components = CalculateConnectedComponents(all_graphs, num_pieces);
    
    // Analyze what happens if this connection is made
    double fragmentation_score = 0.0;
    
    if (chunk.graph_index == -1) {
        // Outside edge - connecting to unconnected piece
        // This always reduces fragmentation (good)
        fragmentation_score = 1.0;
    } else if (chunk.graph_index != current_graph_index) {
        // Inside edge - connecting two separate graphs  
        // This reduces fragmentation significantly (very good)
        fragmentation_score = 2.0;
        
        // Additional bonus if connecting small isolated graphs
        int current_graph_size = CountPiecesInGraph(all_graphs[current_graph_index]);
        int target_graph_size = CountPiecesInGraph(all_graphs[chunk.graph_index]);
        
        // Bonus for connecting small isolated pieces (prevents small fragment isolation)
        if (current_graph_size <= 2 || target_graph_size <= 2) {
            fragmentation_score += 1.0;
        }
    } else {
        // Connecting within same graph - no fragmentation impact
        fragmentation_score = 0.0;
    }
    
    return fragmentation_score;
}

double ConnectivityOptimizer::CalculateHubStrength(
    int piece_id,
    const std::vector<RankingSubgraph>& all_graphs,
    const std::vector<LCSIndex>& lcs_reference
) {
    if (piece_id <= 0 || lcs_reference.empty()) {
        return 0.0;
    }
    
    int num_pieces = all_graphs.empty() ? 0 : all_graphs[0].node_.size();
    if (num_pieces == 0 || piece_id > num_pieces) {
        return 0.0;
    }
    
    // Build adjacency matrix for precise connection strength calculation
    std::vector<std::vector<double>> adjacency_matrix = BuildAdjacencyMatrix(lcs_reference, num_pieces);
    
    // Calculate total connection strength for this piece
    double total_strength = CalculateTotalConnectionStrength(piece_id, adjacency_matrix);
    
    // Count number of distinct connections (connectivity diversity)
    int connection_count = 0;
    for (int i = 0; i < num_pieces; i++) {
        if (i != (piece_id - 1) && adjacency_matrix[piece_id - 1][i] > 0) {
            connection_count++;
        }
    }
    
    // Hub strength = connection strength * connectivity diversity
    // Higher diversity is better for global connectivity
    double diversity_factor = static_cast<double>(connection_count) / std::max(1.0, static_cast<double>(num_pieces - 1));
    double hub_strength = total_strength * (1.0 + diversity_factor);
    
    return hub_strength;
}

double ConnectivityOptimizer::PredictConnectivityImprovement(
    const Chunk& chunk,
    int current_graph_index,
    const std::vector<RankingSubgraph>& all_graphs,
    const std::vector<LCSIndex>& lcs_reference
) {
    int num_pieces = all_graphs.empty() ? 0 : all_graphs[0].node_.size();
    if (num_pieces == 0) return 0.0;
    
    // Calculate current global connectivity
    std::vector<std::vector<double>> adjacency_matrix = BuildAdjacencyMatrix(lcs_reference, num_pieces);
    std::set<int> current_reachable = FindReachablePieces(chunk.node, adjacency_matrix, all_graphs);
    double current_connectivity = static_cast<double>(current_reachable.size()) / num_pieces;
    
    // Calculate predicted connectivity after this connection
    std::set<int> predicted_reachable = FindReachablePieces(chunk.node, adjacency_matrix, all_graphs, &chunk);
    double predicted_connectivity = static_cast<double>(predicted_reachable.size()) / num_pieces;
    
    // Return improvement in connectivity
    double improvement = predicted_connectivity - current_connectivity;
    
    // Bonus for connections that significantly increase reachability
    if (improvement > 0.2) {  // More than 20% improvement
        improvement *= 1.5;  // Amplify the benefit
    }
    
    return improvement;
}

double ConnectivityOptimizer::EvaluateFutureConnectivityPotential(
    const Chunk& chunk,
    int current_graph_index,
    const std::vector<RankingSubgraph>& all_graphs,
    const std::vector<LCSIndex>& lcs_reference
) {
    int num_pieces = all_graphs.empty() ? 0 : all_graphs[0].node_.size();
    if (num_pieces == 0) return 0.0;
    
    // Build adjacency matrix
    std::vector<std::vector<double>> adjacency_matrix = BuildAdjacencyMatrix(lcs_reference, num_pieces);
    
    double future_potential = 0.0;
    
    // Evaluate what new connections become possible if this connection is made
    int target_piece = -1;
    if (chunk.graph_index == -1) {
        // Outside connection - target is the unconnected piece
        target_piece = chunk.node;
    } else {
        // Inside connection - find the piece from the other graph
        const auto& target_graph = all_graphs[chunk.graph_index];
        for (int i = 0; i < num_pieces; i++) {
            if (target_graph.node_[i] && (i + 1) != chunk.node) {
                target_piece = i + 1;
                break;  // Take the first piece from target graph
            }
        }
    }
    
    if (target_piece > 0 && target_piece <= num_pieces) {
        // Count high-value connections that would become available
        const auto& current_graph = all_graphs[current_graph_index];
        for (int i = 0; i < num_pieces; i++) {
            if (current_graph.node_[i]) {
                int current_piece = i + 1;
                double connection_strength = adjacency_matrix[current_piece - 1][target_piece - 1];
                if (connection_strength > 0) {
                    // This connection would become part of a connected component
                    // Weight by connection strength
                    future_potential += connection_strength / 100.0;  // Normalize
                }
            }
        }
    }
    
    return future_potential;
}

std::vector<std::vector<double>> ConnectivityOptimizer::BuildAdjacencyMatrix(
    const std::vector<LCSIndex>& lcs_reference,
    int num_pieces
) {
    // Initialize matrix with zeros
    std::vector<std::vector<double>> matrix(num_pieces, std::vector<double>(num_pieces, 0.0));
    
    // Fill matrix with connection strengths from LCS matches
    for (const auto& lcs : lcs_reference) {
        if (lcs.shard_x_ > 0 && lcs.shard_x_ <= num_pieces && 
            lcs.shard_y_ > 0 && lcs.shard_y_ <= num_pieces &&
            lcs.shard_x_ != lcs.shard_y_) {
            
            int x = lcs.shard_x_ - 1;  // Convert to 0-based indexing
            int y = lcs.shard_y_ - 1;
            
            // Use inliner score as connection strength (symmetric matrix)
            matrix[x][y] += static_cast<double>(lcs.inliner_);
            matrix[y][x] += static_cast<double>(lcs.inliner_);
        }
    }
    
    return matrix;
}

int ConnectivityOptimizer::CalculateConnectedComponents(
    const std::vector<RankingSubgraph>& all_graphs,
    int num_pieces
) {
    if (num_pieces == 0) return 0;
    
    std::vector<bool> visited(num_pieces, false);
    int components = 0;
    
    // Mark all pieces that are in graphs as visited in their components
    for (const auto& graph : all_graphs) {
        bool graph_has_pieces = false;
        for (int i = 0; i < num_pieces; i++) {
            if (graph.node_[i] && !visited[i]) {
                if (!graph_has_pieces) {
                    components++;
                    graph_has_pieces = true;
                }
                visited[i] = true;
            }
        }
    }
    
    // Count isolated pieces (not in any graph)
    for (int i = 0; i < num_pieces; i++) {
        if (!visited[i]) {
            components++;
        }
    }
    
    return components;
}

std::set<int> ConnectivityOptimizer::FindReachablePieces(
    int piece_id,
    const std::vector<std::vector<double>>& adjacency_matrix,
    const std::vector<RankingSubgraph>& all_graphs,
    const Chunk* proposed_connection
) {
    std::set<int> reachable;
    if (piece_id <= 0 || piece_id > adjacency_matrix.size()) {
        return reachable;
    }
    
    int num_pieces = adjacency_matrix.size();
    std::vector<bool> visited(num_pieces, false);
    std::queue<int> to_visit;
    
    // Start BFS from the given piece
    to_visit.push(piece_id - 1);  // Convert to 0-based
    visited[piece_id - 1] = true;
    reachable.insert(piece_id);
    
    while (!to_visit.empty()) {
        int current = to_visit.front();
        to_visit.pop();
        
        // Check all possible connections
        for (int next = 0; next < num_pieces; next++) {
            if (visited[next] || current == next) continue;
            
            bool are_connected = false;
            
            // Check if connected through existing graphs
            if (ArePiecesConnected(current + 1, next + 1, all_graphs)) {
                are_connected = true;
            }
            // Check if connected through proposed connection
            else if (proposed_connection != nullptr) {
                int prop_piece1 = proposed_connection->node - 1;  // Convert to 0-based
                // Find the other piece in the proposed connection
                int prop_piece2 = -1;
                if (proposed_connection->graph_index >= 0 && 
                    proposed_connection->graph_index < all_graphs.size()) {
                    const auto& target_graph = all_graphs[proposed_connection->graph_index];
                    for (int i = 0; i < num_pieces; i++) {
                        if (target_graph.node_[i] && i != prop_piece1) {
                            prop_piece2 = i;
                            break;
                        }
                    }
                }
                
                if (prop_piece2 >= 0) {
                    if ((current == prop_piece1 && next == prop_piece2) ||
                        (current == prop_piece2 && next == prop_piece1)) {
                        are_connected = true;
                    }
                }
            }
            // Check adjacency matrix for direct connections
            else if (adjacency_matrix[current][next] > 0) {
                are_connected = true;
            }
            
            if (are_connected) {
                visited[next] = true;
                to_visit.push(next);
                reachable.insert(next + 1);  // Convert back to 1-based
            }
        }
    }
    
    return reachable;
}

double ConnectivityOptimizer::CalculateTotalConnectionStrength(
    int piece_id,
    const std::vector<std::vector<double>>& adjacency_matrix
) {
    if (piece_id <= 0 || piece_id > adjacency_matrix.size()) {
        return 0.0;
    }
    
    double total = 0.0;
    int piece_index = piece_id - 1;  // Convert to 0-based
    
    for (size_t i = 0; i < adjacency_matrix[piece_index].size(); i++) {
        total += adjacency_matrix[piece_index][i];
    }
    
    return total;
}

bool ConnectivityOptimizer::ArePiecesConnected(
    int piece1,
    int piece2,
    const std::vector<RankingSubgraph>& all_graphs
) {
    if (piece1 == piece2) return true;
    
    // Check if both pieces are in the same graph
    for (const auto& graph : all_graphs) {
        if (piece1 > 0 && piece1 <= graph.node_.size() &&
            piece2 > 0 && piece2 <= graph.node_.size()) {
            if (graph.node_[piece1 - 1] && graph.node_[piece2 - 1]) {
                return true;
            }
        }
    }
    
    return false;
}

int ConnectivityOptimizer::GetGraphContainingPiece(
    int piece_id,
    const std::vector<RankingSubgraph>& all_graphs
) {
    for (size_t i = 0; i < all_graphs.size(); i++) {
        if (piece_id > 0 && piece_id <= all_graphs[i].node_.size()) {
            if (all_graphs[i].node_[piece_id - 1]) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}

int ConnectivityOptimizer::CountPiecesInGraph(const RankingSubgraph& graph) {
    int count = 0;
    for (bool has_piece : graph.node_) {
        if (has_piece) count++;
    }
    return count;
}