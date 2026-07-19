#!/usr/bin/env python3
"""
Spatial connectivity analysis for red, blue, and green pieces
"""

import numpy as np
import os
import sys

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

def compute_min_distance(points1, points2, sample_size=1000):
    """Compute minimum distance between two point clouds with sampling for efficiency"""
    if points1 is None or points2 is None:
        return float('inf')

    # Sample points for efficiency if datasets are large
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

def get_piece_centroid(points):
    """Get centroid of point cloud"""
    if points is None or len(points) == 0:
        return None
    return np.mean(points, axis=0)

def analyze_spatial_connectivity():
    """Analyze spatial connectivity between red, blue, and green pieces"""

    # Results directory
    results_dir = "/data/gpfs/projects/punim2657/sfs_main/results_2025_09_15_0323"

    # Piece mapping based on previous analysis
    # Red = Piece 1, Blue = Piece 2, Green = Piece 3
    pieces = {
        'red': {'id': 1, 'file': f"{results_dir}/piece_1_surface.xyz", 'graph': 0},
        'blue': {'id': 2, 'file': f"{results_dir}/piece_2_surface.xyz", 'graph': 1},
        'green': {'id': 3, 'file': f"{results_dir}/piece_3_surface.xyz", 'graph': 1}
    }

    print("=== SPATIAL CONNECTIVITY ANALYSIS ===")
    print(f"Results directory: {results_dir}")
    print()

    # Load point clouds
    point_clouds = {}
    for color, info in pieces.items():
        print(f"Loading {color} piece (ID {info['id']}, Graph {info['graph']})...")
        points = load_xyz_file(info['file'])
        if points is not None:
            point_clouds[color] = points
            print(f"  Loaded {len(points)} points")
        else:
            print(f"  Failed to load!")
            return

    print()
    print("=== GRAPH ASSIGNMENT ===")
    print("Red piece (1): Graph 0")
    print("Blue piece (2): Graph 1")
    print("Green piece (3): Graph 1")
    print("=> Blue and Green are in same graph, but separate from Red")
    print()

    # Compute centroids
    print("=== PIECE CENTROIDS ===")
    centroids = {}
    for color, points in point_clouds.items():
        centroid = get_piece_centroid(points)
        centroids[color] = centroid
        print(f"{color.capitalize()} piece centroid: [{centroid[0]:.2f}, {centroid[1]:.2f}, {centroid[2]:.2f}]")

    print()

    # Compute centroid distances
    print("=== CENTROID DISTANCES ===")
    red_blue_dist = np.linalg.norm(centroids['red'] - centroids['blue'])
    red_green_dist = np.linalg.norm(centroids['red'] - centroids['green'])
    blue_green_dist = np.linalg.norm(centroids['blue'] - centroids['green'])

    print(f"Red ↔ Blue centroid distance: {red_blue_dist:.2f} units")
    print(f"Red ↔ Green centroid distance: {red_green_dist:.2f} units")
    print(f"Blue ↔ Green centroid distance: {blue_green_dist:.2f} units")
    print()

    # Compute minimum surface distances
    print("=== MINIMUM SURFACE DISTANCES ===")
    print("Computing closest surface points (this may take a moment)...")

    red_blue_min = compute_min_distance(point_clouds['red'], point_clouds['blue'])
    red_green_min = compute_min_distance(point_clouds['red'], point_clouds['green'])
    blue_green_min = compute_min_distance(point_clouds['blue'], point_clouds['green'])

    print(f"Red ↔ Blue minimum distance: {red_blue_min:.3f} units")
    print(f"Red ↔ Green minimum distance: {red_green_min:.3f} units")
    print(f"Blue ↔ Green minimum distance: {blue_green_min:.3f} units")
    print()

    # Analysis
    print("=== SPATIAL CONNECTIVITY ANALYSIS ===")
    contact_threshold = 2.0  # Based on typical archaeological sherd matching

    print(f"Using contact threshold: {contact_threshold} units")
    print()

    # Check if pieces are actually in contact spatially
    red_blue_contact = red_blue_min < contact_threshold
    red_green_contact = red_green_min < contact_threshold
    blue_green_contact = blue_green_min < contact_threshold

    print("SPATIAL CONTACT STATUS:")
    print(f"  Red ↔ Blue: {'✅ IN CONTACT' if red_blue_contact else '❌ NOT IN CONTACT'} ({red_blue_min:.3f} < {contact_threshold})")
    print(f"  Red ↔ Green: {'✅ IN CONTACT' if red_green_contact else '❌ NOT IN CONTACT'} ({red_green_min:.3f} < {contact_threshold})")
    print(f"  Blue ↔ Green: {'✅ IN CONTACT' if blue_green_contact else '❌ NOT IN CONTACT'} ({blue_green_min:.3f} < {contact_threshold})")
    print()

    print("GRAPH VS SPATIAL ANALYSIS:")
    if red_blue_contact and red_green_contact:
        print("🎯 FINDING: Red piece IS spatially connected to both Blue and Green!")
        print("   => Graph fragmentation may be due to algorithmic issues, not spatial reality")
        print("   => Red piece should serve as connectivity hub as predicted")
    elif red_blue_contact or red_green_contact:
        print("🔍 FINDING: Red piece is spatially connected to at least one other piece")
        if red_blue_contact:
            print("   => Red connects to Blue spatially")
        if red_green_contact:
            print("   => Red connects to Green spatially")
        print("   => Partial connectivity exists, algorithm may need refinement")
    else:
        print("📍 FINDING: No direct spatial contact between Red and Blue/Green pieces")
        print("   => Graph separation may reflect actual spatial reality")
        print("   => Red piece connectivity hub hypothesis needs revision")

    if blue_green_contact:
        print("✅ Blue and Green pieces ARE spatially connected (consistent with same graph)")
    else:
        print("⚠️  Blue and Green pieces are NOT spatially connected (inconsistent with same graph)")

    print()
    print("=== SUMMARY ===")
    print(f"Graph structure: Red(Graph 0) vs Blue+Green(Graph 1)")
    print(f"Spatial reality: Contact distances = {red_blue_min:.3f}, {red_green_min:.3f}, {blue_green_min:.3f}")

    return {
        'red_blue_contact': red_blue_contact,
        'red_green_contact': red_green_contact,
        'blue_green_contact': blue_green_contact,
        'distances': {
            'red_blue': red_blue_min,
            'red_green': red_green_min,
            'blue_green': blue_green_min
        }
    }

if __name__ == "__main__":
    np.random.seed(42)  # For reproducible sampling
    results = analyze_spatial_connectivity()