#!/usr/bin/env python3

import numpy as np

def analyze_ground_truth_connectivity():
    """Analyze the ground truth adjacency matrix for Pot A"""

    # Ground truth adjacency matrix from Pot_A_simple_graph.txt
    # Rows: piece 1-8, Columns: piece 1-8
    adjacency_matrix = np.array([
        [0, 1, 1, 1, 1, 1, 1, 0],  # Piece 1 connections
        [1, 0, 0, 1, 1, 0, 0, 1],  # Piece 2 connections
        [1, 0, 0, 0, 1, 1, 0, 0],  # Piece 3 connections
        [1, 1, 0, 0, 0, 1, 1, 1],  # Piece 4 connections
        [1, 1, 1, 0, 0, 0, 0, 0],  # Piece 5 connections
        [1, 0, 1, 1, 0, 0, 1, 0],  # Piece 6 connections
        [1, 0, 0, 1, 0, 1, 0, 0],  # Piece 7 connections
        [0, 1, 0, 1, 0, 0, 0, 0],  # Piece 8 connections
    ])

    print("=== GROUND TRUTH CONNECTIVITY ANALYSIS ===")
    print("Piece Color Mapping:")
    print("  1 = Red, 2 = Blue, 3 = Green, 4 = Orange")
    print("  5 = Purple, 6 = Brown, 7 = Pink, 8 = Dark")
    print()

    # Analyze each piece's connections
    pieces = ['Red(1)', 'Blue(2)', 'Green(3)', 'Orange(4)', 'Purple(5)', 'Brown(6)', 'Pink(7)', 'Dark(8)']

    for i in range(8):
        connections = []
        for j in range(8):
            if adjacency_matrix[i][j] == 1:
                connections.append(pieces[j])

        print(f"{pieces[i]} should connect to: {', '.join(connections) if connections else 'NONE'}")

    print("\n=== CRITICAL MISSING CONNECTIONS IDENTIFIED ===")

    # Check specific issues mentioned by user
    print("User-identified issues:")
    print(f"• Red-Blue connection: {'✅ REQUIRED' if adjacency_matrix[0][1] else '❌ NOT REQUIRED'}")
    print(f"• Red-Brown connection: {'✅ REQUIRED' if adjacency_matrix[0][5] else '❌ NOT REQUIRED'}")
    print(f"• Pink isolation: Pink should connect to {[pieces[j] for j in range(8) if adjacency_matrix[6][j] == 1]}")

    print("\n=== CONNECTIVITY HUB ANALYSIS ===")
    connection_counts = np.sum(adjacency_matrix, axis=1)
    for i, count in enumerate(connection_counts):
        print(f"{pieces[i]}: {count} connections - {'HUB' if count >= 5 else 'REGULAR' if count >= 3 else 'PERIPHERAL'}")

    print(f"\nMost connected piece: {pieces[np.argmax(connection_counts)]} with {np.max(connection_counts)} connections")

    print("\n=== EXPECTED GRAPH STRUCTURE ===")
    print("With proper connectivity, all pieces should form ONE unified graph:")
    print("Red(1) is the main hub connecting to 6 other pieces!")
    print("This should eliminate any 3-graph fragmentation.")

    return adjacency_matrix

def compare_with_current_results():
    """Compare with our current SFS results"""
    print("\n" + "="*50)
    print("COMPARISON WITH CURRENT SFS RESULTS")
    print("="*50)

    # Current SFS results from assembly_data.txt
    current_graphs = {
        0: ['Blue(2)', 'Green(3)'],           # Graph 0: 2 pieces
        1: ['Red(1)', 'Orange(4)', 'Purple(5)', 'Brown(6)', 'Dark(8)'],  # Graph 1: 5 pieces
        2: ['Pink(7)']                        # Graph 2: 1 piece (ISOLATED)
    }

    print("CURRENT SFS RESULT (3 SEPARATE GRAPHS):")
    for graph_id, pieces in current_graphs.items():
        print(f"  Graph {graph_id}: {', '.join(pieces)}")

    print("\nGROUND TRUTH REQUIREMENT (1 UNIFIED GRAPH):")
    print("  All 8 pieces should be in a single connected component")

    print("\n=== ROOT CAUSE ANALYSIS ===")
    print("❌ MISSING: Red(1) ↔ Blue(2) connection")
    print("❌ MISSING: Red(1) ↔ Green(3) connection")
    print("❌ MISSING: Pink(7) ↔ Red(1) connection")
    print("❌ MISSING: Pink(7) ↔ Orange(4) connection")
    print("❌ MISSING: Pink(7) ↔ Brown(6) connection")

    print("\nThese missing connections prevent:")
    print("• Graph 0 (Blue-Green) from connecting to Graph 1 (Red hub)")
    print("• Graph 2 (Pink) from connecting to any other pieces")

if __name__ == "__main__":
    adjacency_matrix = analyze_ground_truth_connectivity()
    compare_with_current_results()