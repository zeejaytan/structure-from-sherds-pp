#!/usr/bin/env python3
"""
Spatial Connectivity Analysis for Auto-Agglomerative Assembly Results
Check if global optimization actually connected the pieces or just put them in same graph
"""

import numpy as np
import os

def load_xyz_file(filepath):
    """Load XYZ file and return points as numpy array"""
    try:
        points = []
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    coords = line.split()
                    if len(coords) >= 3:
                        x, y, z = float(coords[0]), float(coords[1]), float(coords[2])
                        points.append([x, y, z])
        return np.array(points)
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        return None

def get_piece_bounds(points):
    """Get min/max bounds for piece"""
    if points is None or len(points) == 0:
        return None

    return {
        'x_min': np.min(points[:, 0]),
        'x_max': np.max(points[:, 0]),
        'y_min': np.min(points[:, 1]),
        'y_max': np.max(points[:, 1]),
        'z_min': np.min(points[:, 2]),
        'z_max': np.max(points[:, 2]),
        'centroid': np.mean(points, axis=0)
    }

def compute_min_distance_between_pieces(points1, points2, sample_size=1000):
    """Compute minimum distance between two point clouds"""
    if points1 is None or points2 is None:
        return float('inf')

    # Sample for efficiency
    if len(points1) > sample_size:
        indices1 = np.random.choice(len(points1), sample_size, replace=False)
        points1_sample = points1[indices1]
    else:
        points1_sample = points1

    if len(points2) > sample_size:
        indices2 = np.random.choice(len(points2), sample_size, replace=False)
        points2_sample = points2[indices2]
    else:
        points2_sample = points2

    min_dist = float('inf')
    for p1 in points1_sample:
        distances = np.sqrt(np.sum((points2_sample - p1)**2, axis=1))
        current_min = np.min(distances)
        if current_min < min_dist:
            min_dist = current_min

    return min_dist

def analyze_assembly_connectivity():
    """Analyze if pieces are actually spatially connected"""

    results_dir = "/data/gpfs/projects/punim2657/sfs_main/results_2025_09_19_2047"

    print("🔍 ANALYZING SPATIAL CONNECTIVITY OF AUTO-AGGLOMERATIVE ASSEMBLY")
    print("=" * 80)

    # Load all pieces
    pieces = {}
    piece_bounds = {}

    for piece_id in range(1, 9):
        xyz_file = os.path.join(results_dir, f"piece_{piece_id}_surface.xyz")

        if os.path.exists(xyz_file):
            points = load_xyz_file(xyz_file)
            if points is not None:
                pieces[piece_id] = points
                piece_bounds[piece_id] = get_piece_bounds(points)

                centroid = piece_bounds[piece_id]['centroid']
                print(f"Piece {piece_id}: {len(points)} points, centroid [{centroid[0]:.1f}, {centroid[1]:.1f}, {centroid[2]:.1f}]")

    print("\n" + "="*80)
    print("📏 SPATIAL BOUNDS ANALYSIS")
    print("="*80)

    for piece_id in range(1, 9):
        if piece_id in piece_bounds:
            bounds = piece_bounds[piece_id]
            print(f"\nPiece {piece_id}:")
            print(f"  X range: [{bounds['x_min']:.1f}, {bounds['x_max']:.1f}] (extent: {bounds['x_max'] - bounds['x_min']:.1f})")
            print(f"  Y range: [{bounds['y_min']:.1f}, {bounds['y_max']:.1f}] (extent: {bounds['y_max'] - bounds['y_min']:.1f})")
            print(f"  Z range: [{bounds['z_min']:.1f}, {bounds['z_max']:.1f}] (extent: {bounds['z_max'] - bounds['z_min']:.1f})")

    print("\n" + "="*80)
    print("🎯 CRITICAL CONNECTIVITY ANALYSIS")
    print("="*80)

    # Focus on Red-Blue-Green connectivity
    critical_pairs = [
        (1, 2, "Red-Blue"),
        (1, 3, "Red-Green"),
        (2, 3, "Blue-Green")
    ]

    contact_threshold = 5.0  # mm - reasonable contact distance

    print(f"Using contact threshold: {contact_threshold} mm")
    print()

    for piece1, piece2, name in critical_pairs:
        if piece1 in pieces and piece2 in pieces:
            # Centroid distance
            centroid1 = piece_bounds[piece1]['centroid']
            centroid2 = piece_bounds[piece2]['centroid']
            centroid_dist = np.linalg.norm(centroid1 - centroid2)

            # Minimum surface distance
            min_dist = compute_min_distance_between_pieces(pieces[piece1], pieces[piece2])

            # Contact analysis
            in_contact = min_dist < contact_threshold

            print(f"🔍 {name} Connection Analysis:")
            print(f"   Centroid distance: {centroid_dist:.2f} mm")
            print(f"   Minimum surface distance: {min_dist:.3f} mm")
            print(f"   Status: {'✅ IN CONTACT' if in_contact else '❌ DISCONNECTED'} (threshold: {contact_threshold} mm)")
            print()

    print("="*80)
    print("🚨 GLOBAL OPTIMIZATION EFFECTIVENESS ANALYSIS")
    print("="*80)

    # Check if pieces are clustered or scattered
    all_centroids = np.array([piece_bounds[i]['centroid'] for i in range(1, 9) if i in piece_bounds])

    if len(all_centroids) > 0:
        assembly_centroid = np.mean(all_centroids, axis=0)
        max_distance_from_center = 0

        print(f"Assembly centroid: [{assembly_centroid[0]:.1f}, {assembly_centroid[1]:.1f}, {assembly_centroid[2]:.1f}]")
        print("\nPiece distances from assembly center:")

        for i in range(1, 9):
            if i in piece_bounds:
                piece_centroid = piece_bounds[i]['centroid']
                dist_from_center = np.linalg.norm(piece_centroid - assembly_centroid)
                max_distance_from_center = max(max_distance_from_center, dist_from_center)

                print(f"  Piece {i}: {dist_from_center:.1f} mm from center")

        print(f"\nAssembly spatial extent: {max_distance_from_center:.1f} mm")

        # Check for clustering vs scattering
        if max_distance_from_center > 100:  # More than 10cm spread
            print("🚨 WARNING: Assembly appears SCATTERED - pieces not spatially unified")
        else:
            print("✅ Assembly appears CLUSTERED - pieces spatially unified")

    print("\n" + "="*80)
    print("📊 CONCLUSION")
    print("="*80)

    # Check Red-Blue connection specifically
    if 1 in pieces and 2 in pieces:
        red_blue_dist = compute_min_distance_between_pieces(pieces[1], pieces[2])

        if red_blue_dist > contact_threshold:
            print("🚨 CRITICAL ISSUE CONFIRMED:")
            print(f"   Red-Blue pieces are DISCONNECTED ({red_blue_dist:.3f} mm apart)")
            print("   Global optimization failed to achieve spatial connectivity")
            print("   System achieved logical grouping but NOT physical connection")
            print()
            print("🔧 REQUIRED ACTION:")
            print("   - Auto-Agglomerative Assembly system needs debugging")
            print("   - 6-DoF parameter denoising not achieving spatial proximity")
            print("   - Transformation matrices may not be optimally computed")

        else:
            print("✅ SUCCESS:")
            print(f"   Red-Blue pieces are CONNECTED ({red_blue_dist:.3f} mm apart)")
            print("   Global optimization successfully achieved spatial connectivity")

if __name__ == "__main__":
    np.random.seed(42)  # For reproducible sampling
    analyze_assembly_connectivity()