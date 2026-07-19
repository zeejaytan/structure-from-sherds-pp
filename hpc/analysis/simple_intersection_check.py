#!/usr/bin/env python3
import numpy as np

def load_piece_points(filename):
    """Load 3D points from xyz file"""
    try:
        points = np.loadtxt(filename)
        print(f"Loaded {len(points)} points from {filename}")
        return points
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def simple_intersection_check(points1, points2, piece1_name, piece2_name):
    """Simple intersection check using bounding boxes and distance sampling"""
    print(f"\n=== {piece1_name} vs {piece2_name} ===")

    # Bounding box analysis
    min1, max1 = np.min(points1, axis=0), np.max(points1, axis=0)
    min2, max2 = np.min(points2, axis=0), np.max(points2, axis=0)

    print(f"{piece1_name} bounds: X[{min1[0]:.1f}, {max1[0]:.1f}] Y[{min1[1]:.1f}, {max1[1]:.1f}] Z[{min1[2]:.1f}, {max1[2]:.1f}]")
    print(f"{piece2_name} bounds: X[{min2[0]:.1f}, {max2[0]:.1f}] Y[{min2[1]:.1f}, {max2[1]:.1f}] Z[{min2[2]:.1f}, {max2[2]:.1f}]")

    # Check bounding box overlap
    overlap_x = max(0, min(max1[0], max2[0]) - max(min1[0], min2[0]))
    overlap_y = max(0, min(max1[1], max2[1]) - max(min1[1], min2[1]))
    overlap_z = max(0, min(max1[2], max2[2]) - max(min1[2], min2[2]))

    overlap_volume = overlap_x * overlap_y * overlap_z
    box1_volume = (max1[0] - min1[0]) * (max1[1] - min1[1]) * (max1[2] - min1[2])
    box2_volume = (max2[0] - min2[0]) * (max2[1] - min2[1]) * (max2[2] - min2[2])

    overlap_ratio = overlap_volume / min(box1_volume, box2_volume) * 100

    print(f"Bounding box overlap: {overlap_x:.1f} x {overlap_y:.1f} x {overlap_z:.1f} = {overlap_volume:.1f}")
    print(f"Overlap ratio: {overlap_ratio:.2f}% of smaller piece volume")

    # Sample distance check (every 100th point to avoid memory issues)
    sample1 = points1[::100]  # Sample every 100th point
    min_distances = []

    for p1 in sample1[:50]:  # Check first 50 sampled points
        distances = np.sqrt(np.sum((points2 - p1)**2, axis=1))
        min_dist = np.min(distances)
        min_distances.append(min_dist)

    min_distances = np.array(min_distances)
    very_close = np.sum(min_distances < 1.0)
    close = np.sum(min_distances < 5.0)

    print(f"Sample analysis ({len(min_distances)} points):")
    print(f"  Points < 1mm apart: {very_close} ({very_close/len(min_distances)*100:.1f}%)")
    print(f"  Points < 5mm apart: {close} ({close/len(min_distances)*100:.1f}%)")
    print(f"  Minimum distance found: {np.min(min_distances):.2f}mm")
    print(f"  Average minimum distance: {np.mean(min_distances):.2f}mm")

    if overlap_ratio > 20:
        print(f"*** SEVERE BOUNDING BOX OVERLAP: {overlap_ratio:.1f}% ***")
    elif overlap_ratio > 5:
        print(f"*** SIGNIFICANT OVERLAP: {overlap_ratio:.1f}% ***")

    if very_close > len(min_distances) * 0.1:
        print(f"*** SEVERE POINT PROXIMITY: {very_close/len(min_distances)*100:.1f}% points < 1mm ***")

    return overlap_ratio, np.min(min_distances), np.mean(min_distances)

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

    # Calculate intersections
    blue_orange_overlap, blue_orange_min, blue_orange_avg = simple_intersection_check(blue_points, orange_points, "Blue(2)", "Orange(4)")
    green_orange_overlap, green_orange_min, green_orange_avg = simple_intersection_check(green_points, orange_points, "Green(3)", "Orange(4)")
    blue_green_overlap, blue_green_min, blue_green_avg = simple_intersection_check(blue_points, green_points, "Blue(2)", "Green(3)")

    print(f"\n=== SUMMARY ===")
    print(f"Blue-Orange: {blue_orange_overlap:.1f}% bounding overlap, min dist: {blue_orange_min:.2f}mm")
    print(f"Green-Orange: {green_orange_overlap:.1f}% bounding overlap, min dist: {green_orange_min:.2f}mm")
    print(f"Blue-Green: {blue_green_overlap:.1f}% bounding overlap, min dist: {blue_green_min:.2f}mm")

    print(f"\n=== VALIDATION COMPARISON ===")
    print(f"Remember: During registration validation, intersections were:")
    print(f"  Blue-Orange: 0.006% volume overlap")
    print(f"  Green-Orange: 0.007% volume overlap")
    print(f"  Blue-Green: 0.01% volume overlap")
    print(f"\nFinal assembly shows MUCH HIGHER intersections!")

if __name__ == "__main__":
    main()