#!/usr/bin/env python3

# Ground truth adjacency matrix (1-indexed pieces)
gt_adjacency = [
    [0, 1, 1, 1, 1, 1, 1, 0],  # Piece 1 connects to: 2,3,4,5,6,7
    [1, 0, 0, 1, 1, 0, 0, 1],  # Piece 2 connects to: 1,4,5,8  
    [1, 0, 0, 0, 1, 1, 0, 0],  # Piece 3 connects to: 1,5,6
    [1, 1, 0, 0, 0, 1, 1, 1],  # Piece 4 connects to: 1,2,6,7,8
    [1, 1, 1, 0, 0, 0, 0, 0],  # Piece 5 connects to: 1,2,3
    [1, 0, 1, 1, 0, 0, 1, 0],  # Piece 6 connects to: 1,3,4,7
    [1, 0, 0, 1, 0, 1, 0, 0],  # Piece 7 connects to: 1,4,6
    [0, 1, 0, 1, 0, 0, 0, 0],  # Piece 8 connects to: 2,4
]

# Actual connections from latest run
actual_connections = [
    (4, 1, 76),  # Piece 4→1: 76 inliers
    (5, 1, 83),  # Piece 5→1: 83 inliers 
    (8, 5, 43),  # Piece 8→5: 43 inliers
    (4, 3, 30),  # Piece 4→3: 30 inliers
    (6, 3, 18),  # Piece 6→3: 18 inliers
    (5, 2, 24),  # Piece 5→2: 24 inliers
    (7, 1, 28),  # Piece 7→1: 28 inliers
]

print("=== GROUND TRUTH ADJACENCY ANALYSIS ===")
print()

# Count total expected connections
total_gt_connections = 0
gt_pairs = []
for i in range(8):
    for j in range(i+1, 8):  # Only count upper triangular to avoid duplicates
        if gt_adjacency[i][j] == 1:
            total_gt_connections += 1
            gt_pairs.append((i+1, j+1))

print(f"Total ground truth connections: {total_gt_connections}")
print("Ground truth adjacency pairs:")
for pair in gt_pairs:
    print(f"  Piece {pair[0]} ↔ Piece {pair[1]}")

print()
print("=== ACTUAL CONNECTIONS ANALYSIS ===")
print(f"Total actual connections: {len(actual_connections)}")
print("Actual connections (directed):")
for conn in actual_connections:
    print(f"  Piece {conn[0]} → Piece {conn[1]} ({conn[2]} inliers)")

print()
print("=== COMPARISON ANALYSIS ===")

# Convert actual connections to undirected pairs
actual_pairs = set()
for conn in actual_connections:
    pair = tuple(sorted([conn[0], conn[1]]))
    actual_pairs.add(pair)

print(f"Actual undirected pairs: {len(actual_pairs)}")
for pair in sorted(actual_pairs):
    print(f"  Piece {pair[0]} ↔ Piece {pair[1]}")

print()

# Check which ground truth connections were found
gt_pairs_set = set(gt_pairs)
found_connections = actual_pairs.intersection(gt_pairs_set)
missing_connections = gt_pairs_set - actual_pairs
incorrect_connections = actual_pairs - gt_pairs_set

print(f"✅ CORRECTLY FOUND: {len(found_connections)}/{len(gt_pairs_set)} ground truth connections")
for pair in sorted(found_connections):
    print(f"  ✅ Piece {pair[0]} ↔ Piece {pair[1]}")

print()
print(f"❌ MISSING: {len(missing_connections)} ground truth connections")
for pair in sorted(missing_connections):
    print(f"  ❌ Piece {pair[0]} ↔ Piece {pair[1]}")

print()
print(f"⚠️  INCORRECT: {len(incorrect_connections)} non-ground-truth connections")
for pair in sorted(incorrect_connections):
    print(f"  ⚠️  Piece {pair[0]} ↔ Piece {pair[1]}")

print()
print("=== ACCURACY METRICS ===")
precision = len(found_connections) / len(actual_pairs) if actual_pairs else 0
recall = len(found_connections) / len(gt_pairs_set) if gt_pairs_set else 0
f1 = 2 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0

print(f"Precision: {precision:.3f} ({len(found_connections)}/{len(actual_pairs)})")
print(f"Recall: {recall:.3f} ({len(found_connections)}/{len(gt_pairs_set)})")
print(f"F1-Score: {f1:.3f}")

