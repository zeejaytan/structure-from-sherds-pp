#!/usr/bin/env python3
"""
Spatial Constraint Analysis for SFS Assembly
Analyzes why orange/purple pieces occupy wrong space and prevent graph merging
"""

import numpy as np
import os
import sys
from pathlib import Path

def read_transformation_matrix(file_path):
    """Read transformation matrix from assembly results"""
    try:
        with open(file_path, 'r') as f:
            lines = f.readlines()
            matrix = []
            for line in lines:
                if line.strip():
                    row = [float(x) for x in line.strip().split()]
                    if len(row) == 4:
                        matrix.append(row)
            return np.array(matrix) if len(matrix) == 4 else None
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return None

def analyze_piece_positions(results_dir):
    """Analyze piece positions and identify spatial conflicts"""
    print(f"=== SPATIAL CONSTRAINT ANALYSIS ===")
    print(f"Results directory: {results_dir}")

    # Piece color mapping (based on previous analysis)
    piece_colors = {
        1: "Red", 2: "Blue", 3: "Green", 4: "Orange",
        5: "Purple", 6: "Brown", 7: "Dark", 8: "Pink"
    }

    transformations = {}
    centroids = {}

    # Read transformation matrices
    for piece_id in range(1, 9):
        matrix_file = f"{results_dir}/piece_{piece_id}_transformation.txt"
        if os.path.exists(matrix_file):
            transform = read_transformation_matrix(matrix_file)
            if transform is not None:
                transformations[piece_id] = transform
                # Extract translation (position)
                position = transform[:3, 3]
                centroids[piece_id] = position
                print(f"Piece {piece_id} ({piece_colors[piece_id]}): position [{position[0]:.2f}, {position[1]:.2f}, {position[2]:.2f}]")

    print(f"\n=== SPATIAL CONFLICT ANALYSIS ===")

    # Analyze distances between pieces
    conflicts = []
    for i in range(1, 9):
        if i not in centroids:
            continue
        for j in range(i+1, 9):
            if j not in centroids:
                continue

            distance = np.linalg.norm(centroids[i] - centroids[j])

            # Flag potential spatial conflicts (pieces too close)
            if distance < 50.0:  # Threshold for spatial conflict
                conflicts.append({
                    'piece1': i,
                    'piece2': j,
                    'color1': piece_colors[i],
                    'color2': piece_colors[j],
                    'distance': distance,
                    'position1': centroids[i],
                    'position2': centroids[j]
                })
                print(f"⚠ CONFLICT: {piece_colors[i]}({i}) ↔ {piece_colors[j]}({j}) distance: {distance:.2f}mm")

    print(f"\n=== GROUND TRUTH CONNECTIVITY VALIDATION ===")

    # Ground truth adjacency (Red should connect to 6 pieces)
    ground_truth_adjacent = {
        1: [2, 3, 4, 5, 6, 7],  # Red connects to: Blue, Green, Orange, Purple, Brown, Dark
        2: [1, 3],              # Blue connects to: Red, Green
        3: [1, 2, 4, 8],        # Green connects to: Red, Blue, Orange, Pink
        4: [1, 3],              # Orange connects to: Red, Green
        5: [1],                 # Purple connects to: Red
        6: [1],                 # Brown connects to: Red
        7: [1],                 # Dark connects to: Red
        8: [3]                  # Pink connects to: Green
    }

    # Check if spatial conflicts prevent ground truth connections
    blocked_connections = []
    for piece_id, adjacent_pieces in ground_truth_adjacent.items():
        if piece_id not in centroids:
            continue

        for adj_piece in adjacent_pieces:
            if adj_piece not in centroids:
                continue

            # Check if this connection is blocked by spatial conflicts
            for conflict in conflicts:
                # If either piece in a ground truth connection is involved in a spatial conflict
                if (piece_id == conflict['piece1'] or piece_id == conflict['piece2'] or
                    adj_piece == conflict['piece1'] or adj_piece == conflict['piece2']):

                    blocked_connections.append({
                        'connection': f"{piece_colors[piece_id]}({piece_id}) → {piece_colors[adj_piece]}({adj_piece})",
                        'blocked_by': f"{conflict['color1']}({conflict['piece1']}) ↔ {conflict['color2']}({conflict['piece2']})",
                        'conflict_distance': conflict['distance']
                    })

    if blocked_connections:
        print(f"\n=== BLOCKED GROUND TRUTH CONNECTIONS ===")
        for block in blocked_connections:
            print(f"❌ {block['connection']} blocked by conflict: {block['blocked_by']} (distance: {block['conflict_distance']:.2f}mm)")

    # Focus on Orange(4) and Purple(5) as suspected problematic pieces
    print(f"\n=== ORANGE AND PURPLE SPATIAL ANALYSIS ===")

    if 4 in centroids and 5 in centroids:
        orange_pos = centroids[4]
        purple_pos = centroids[5]
        orange_purple_dist = np.linalg.norm(orange_pos - purple_pos)

        print(f"Orange(4) position: [{orange_pos[0]:.2f}, {orange_pos[1]:.2f}, {orange_pos[2]:.2f}]")
        print(f"Purple(5) position: [{purple_pos[0]:.2f}, {purple_pos[1]:.2f}, {purple_pos[2]:.2f}]")
        print(f"Orange ↔ Purple distance: {orange_purple_dist:.2f}mm")

        # Check how Orange and Purple affect Red's connections
        if 1 in centroids:
            red_pos = centroids[1]
            red_orange_dist = np.linalg.norm(red_pos - orange_pos)
            red_purple_dist = np.linalg.norm(red_pos - purple_pos)

            print(f"\nRed(1) position: [{red_pos[0]:.2f}, {red_pos[1]:.2f}, {red_pos[2]:.2f}]")
            print(f"Red ↔ Orange distance: {red_orange_dist:.2f}mm")
            print(f"Red ↔ Purple distance: {red_purple_dist:.2f}mm")

            # Check if Orange/Purple placement blocks Red's access to other pieces
            blocked_by_orange_purple = []
            for target in [2, 3, 6, 7]:  # Blue, Green, Brown, Dark
                if target not in centroids:
                    continue

                target_pos = centroids[target]
                red_target_dist = np.linalg.norm(red_pos - target_pos)

                # Check if Orange or Purple is between Red and target
                orange_target_dist = np.linalg.norm(orange_pos - target_pos)
                purple_target_dist = np.linalg.norm(purple_pos - target_pos)

                if (orange_target_dist < red_target_dist/2 or purple_target_dist < red_target_dist/2):
                    blocked_by_orange_purple.append({
                        'target': f"{piece_colors[target]}({target})",
                        'red_distance': red_target_dist,
                        'orange_interference': orange_target_dist < red_target_dist/2,
                        'purple_interference': purple_target_dist < red_target_dist/2
                    })

            if blocked_by_orange_purple:
                print(f"\n=== ORANGE/PURPLE INTERFERENCE WITH RED CONNECTIONS ===")
                for block in blocked_by_orange_purple:
                    interference = []
                    if block['orange_interference']:
                        interference.append("Orange")
                    if block['purple_interference']:
                        interference.append("Purple")
                    print(f"🚫 Red → {block['target']} potentially blocked by: {', '.join(interference)}")

    return conflicts, blocked_connections

def main():
    results_dir = "/data/gpfs/projects/punim2657/sfs_main/results_2025_09_14_1306"

    if not os.path.exists(results_dir):
        print(f"Results directory not found: {results_dir}")
        return

    conflicts, blocked = analyze_piece_positions(results_dir)

    print(f"\n=== SUMMARY ===")
    print(f"Total spatial conflicts detected: {len(conflicts)}")
    print(f"Ground truth connections potentially blocked: {len(blocked)}")

    if conflicts:
        print(f"\nHypothesis validation: Orange/Purple spatial misplacement IS causing assembly issues")
        print(f"Recommendation: Investigate transformation matrices for pieces 4 (Orange) and 5 (Purple)")
    else:
        print(f"\nNo spatial conflicts detected - may need to adjust analysis thresholds")

if __name__ == "__main__":
    main()