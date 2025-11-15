#ifndef CONNECTIVITY_OPTIMIZER_H
#define CONNECTIVITY_OPTIMIZER_H

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include "ranking_system.h"

/**
 * Global Connectivity Optimizer
 * 
 * Evaluates the global connectivity impact of connection decisions
 * to prevent local optima and ensure optimal graph structure.
 * 
 * Key Principles:
 * 1. No averaging or approximations - precise calculations only
 * 2. Considers complete graph topology, not just local connections  
 * 3. Prevents fragmentation and promotes hub connectivity
 * 4. Works with any vessel type and fragment count
 */
class ConnectivityOptimizer {
public:
    ConnectivityOptimizer();
    ~ConnectivityOptimizer();

    /**
     * Calculate global connectivity score for a potential connection
     * @param chunk The connection chunk to evaluate
     * @param current_graph_index Index of the graph making the connection
     * @param all_graphs All graphs in the current state
     * @param lcs_reference Complete LCS match list
     * @return Global connectivity score (higher = better for global connectivity)
     */
    double CalculateGlobalConnectivityScore(
        const Chunk& chunk,
        int current_graph_index,
        const std::vector<RankingSubgraph>& all_graphs,
        const std::vector<LCSIndex>& lcs_reference
    );

    /**
     * Evaluate if a connection would create or prevent fragmentation
     * @param chunk The connection chunk
     * @param current_graph_index Index of current graph
     * @param all_graphs All graphs in state
     * @return Fragmentation impact score (negative = creates fragmentation)
     */
    double EvaluateFragmentationImpact(
        const Chunk& chunk,
        int current_graph_index,
        const std::vector<RankingSubgraph>& all_graphs,
        const std::vector<LCSIndex>& lcs_reference
    );

    /**
     * Calculate hub strength - how well connected a piece is to the overall assembly
     * @param piece_id ID of the piece (1-based)
     * @param all_graphs All graphs in state
     * @param lcs_reference Complete LCS match list
     * @return Hub strength score
     */
    double CalculateHubStrength(
        int piece_id,
        const std::vector<RankingSubgraph>& all_graphs,
        const std::vector<LCSIndex>& lcs_reference
    );

    /**
     * Predict connectivity improvement if this connection is made
     * @param chunk Connection to evaluate
     * @param current_graph_index Current graph index
     * @param all_graphs All graphs
     * @param lcs_reference LCS matches
     * @return Predicted improvement in global connectivity
     */
    double PredictConnectivityImprovement(
        const Chunk& chunk,
        int current_graph_index,
        const std::vector<RankingSubgraph>& all_graphs,
        const std::vector<LCSIndex>& lcs_reference
    );

    /**
     * Check if making this connection would enable future high-value connections
     * @param chunk Connection to evaluate
     * @param current_graph_index Current graph
     * @param all_graphs All graphs
     * @param lcs_reference LCS matches
     * @return Future connectivity potential score
     */
    double EvaluateFutureConnectivityPotential(
        const Chunk& chunk,
        int current_graph_index,
        const std::vector<RankingSubgraph>& all_graphs,
        const std::vector<LCSIndex>& lcs_reference
    );

private:
    /**
     * Build adjacency matrix from LCS matches
     * @param lcs_reference Complete LCS match list
     * @param num_pieces Number of pieces in assembly
     * @return Adjacency matrix with connection strengths
     */
    std::vector<std::vector<double>> BuildAdjacencyMatrix(
        const std::vector<LCSIndex>& lcs_reference,
        int num_pieces
    );

    /**
     * Calculate connected components in current graph state
     * @param all_graphs All graphs
     * @param num_pieces Total number of pieces
     * @return Number of connected components
     */
    int CalculateConnectedComponents(
        const std::vector<RankingSubgraph>& all_graphs,
        int num_pieces
    );

    /**
     * Find all pieces that would be reachable if this connection is made
     * @param piece_id Starting piece
     * @param adjacency_matrix Connection matrix
     * @param all_graphs Current graph state
     * @param proposed_connection Additional connection to consider
     * @return Set of reachable piece IDs
     */
    std::set<int> FindReachablePieces(
        int piece_id,
        const std::vector<std::vector<double>>& adjacency_matrix,
        const std::vector<RankingSubgraph>& all_graphs,
        const Chunk* proposed_connection = nullptr
    );

    /**
     * Calculate the total strength of connections from a piece
     * @param piece_id Piece to analyze (1-based)
     * @param adjacency_matrix Connection strength matrix
     * @return Total connection strength
     */
    double CalculateTotalConnectionStrength(
        int piece_id,
        const std::vector<std::vector<double>>& adjacency_matrix
    );

    /**
     * Check if two pieces are in the same connected component
     * @param piece1 First piece ID (1-based)
     * @param piece2 Second piece ID (1-based) 
     * @param all_graphs Current graph state
     * @return true if connected, false if in different components
     */
    bool ArePiecesConnected(
        int piece1,
        int piece2,
        const std::vector<RankingSubgraph>& all_graphs
    );

    /**
     * Get the graph index that contains a specific piece
     * @param piece_id Piece ID (1-based)
     * @param all_graphs All graphs
     * @return Graph index, or -1 if piece not found
     */
    int GetGraphContainingPiece(
        int piece_id,
        const std::vector<RankingSubgraph>& all_graphs
    );

    /**
     * Count number of pieces in a graph
     * @param graph Graph to analyze
     * @return Number of pieces in graph
     */
    int CountPiecesInGraph(const RankingSubgraph& graph);

    // Configuration parameters
    static constexpr double FRAGMENTATION_PENALTY_WEIGHT = 2.0;
    static constexpr double HUB_CONNECTIVITY_WEIGHT = 1.5;
    static constexpr double FUTURE_POTENTIAL_WEIGHT = 1.0;
    static constexpr double MIN_HUB_STRENGTH_THRESHOLD = 0.1;
};

#endif // CONNECTIVITY_OPTIMIZER_H