#!/usr/bin/env python3
import numpy as np
from sklearn.neighbors import NearestNeighbors
import sys

def load_piece_points(filename):
    """Load 3D points from xyz file"""
    try:
        points = np.loadtxt(filename)
        print(f"Loaded {len(points)} points from {filename}")
        return points
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def calculate_intersection_metrics(points1, points2, piece1_name, piece2_name, threshold=2.0):
    """Calculate intersection metrics between two point clouds"""
    print(f"\n=== Analyzing {piece1_name} vs {piece2_name} ===")

    # Find nearest neighbors
    nbrs = NearestNeighbors(n_neighbors=1, algorithm='ball_tree').fit(points2)
    distances, indices = nbrs.kneighbors(points1)
    distances = distances.flatten()

    # Count intersections at different thresholds
    very_close = np.sum(distances < 0.5)  # Very close (< 0.5mm)
    close = np.sum(distances < 1.0)       # Close (< 1mm)
    nearby = np.sum(distances < threshold) # Nearby (< 2mm)

    # Calculate percentages
    total_points = len(points1)
    very_close_pct = (very_close / total_points) * 100
    close_pct = (close / total_points) * 100
    nearby_pct = (nearby / total_points) * 100

    print(f"  Total points in {piece1_name}: {total_points}")
    print(f"  Very close intersections (< 0.5mm): {very_close} ({very_close_pct:.2f}%)")
    print(f"  Close intersections (< 1mm): {close} ({close_pct:.2f}%)")
    print(f"  Nearby points (< {threshold}mm): {nearby} ({nearby_pct:.2f}%)")
    print(f"  Min distance: {np.min(distances):.3f}mm")
    print(f"  Mean distance: {np.mean(distances):.3f}mm")

    # Check for severe intersections
    if very_close_pct > 5:
        print(f"  *** SEVERE INTERSECTION DETECTED: {very_close_pct:.1f}% of points < 0.5mm apart ***")
    elif close_pct > 10:
        print(f"  *** SIGNIFICANT INTERSECTION: {close_pct:.1f}% of points < 1mm apart ***")
    elif nearby_pct > 15:
        print(f"  *** SPATIAL CONFLICT: {nearby_pct:.1f}% of points < {threshold}mm apart ***")
    else:
        print(f"  No significant intersection detected")

    return {
        'very_close': very_close_pct,
        'close': close_pct,
        'nearby': nearby_pct,
        'min_distance': np.min(distances),
        'mean_distance': np.mean(distances)
    }

def main():
    base_path = "/data/gpfs/projects/punim2657/sfs_main/results_2025_09_16_0222"

    # Load piece point clouds
    blue_points = load_piece_points(f"{base_path}/piece_2_surface.xyz")
    green_points = load_piece_points(f"{base_path}/piece_3_surface.xyz")
    orange_points = load_piece_points(f"{base_path}/piece_4_surface.xyz")

    if blue_points is None or green_points is None or orange_points is None:
        print("Failed to load one or more piece files")
        return

    print("=== FINAL ASSEMBLY INTERSECTION ANALYSIS ===")
    print("Checking actual spatial conflicts in final positioned assembly")

    # Calculate intersections
    blue_orange = calculate_intersection_metrics(blue_points, orange_points, "Blue(2)", "Orange(4)")
    green_orange = calculate_intersection_metrics(green_points, orange_points, "Green(3)", "Orange(4)")
    blue_green = calculate_intersection_metrics(blue_points, green_points, "Blue(2)", "Green(3)")

    print(f"\n=== SUMMARY ===")
    print(f"Blue-Orange intersection: {blue_orange['close']:.1f}% points < 1mm")
    print(f"Green-Orange intersection: {green_orange['close']:.1f}% points < 1mm")
    print(f"Blue-Green intersection: {blue_green['close']:.1f}% points < 1mm")

    # Identify the worst intersection
    max_intersection = max(blue_orange['close'], green_orange['close'], blue_green['close'])
    if max_intersection > 10:
        print(f"\n*** CRITICAL: Maximum intersection is {max_intersection:.1f}% - WAY above validation thresholds! ***")
        if blue_orange['close'] == max_intersection:
            print("Worst intersection: Blue-Orange")
        elif green_orange['close'] == max_intersection:
            print("Worst intersection: Green-Orange")
        else:
            print("Worst intersection: Blue-Green")

if __name__ == "__main__":
    main()