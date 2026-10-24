#!/usr/bin/env python3
"""
Generate PLY visualization files for Pot A NURBS assembly results
"""

import os
import colorsys

def get_color_for_piece(piece_num, total_pieces):
    """Generate distinct colors for each piece using HSV color space"""
    hue = piece_num / total_pieces
    rgb = colorsys.hsv_to_rgb(hue, 0.8, 0.9)
    return [int(rgb[0] * 255), int(rgb[1] * 255), int(rgb[2] * 255)]

def read_xyz_file(filepath):
    """Read XYZ file and return list of (x,y,z) tuples"""
    points = []
    if not os.path.exists(filepath):
        return points

    with open(filepath, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 3:
                try:
                    x, y, z = float(parts[0]), float(parts[1]), float(parts[2])
                    points.append((x, y, z))
                except ValueError:
                    continue
    return points

def write_ply(filepath, points_with_colors):
    """Write PLY file with colored points"""
    with open(filepath, 'w') as f:
        # Header
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {len(points_with_colors)}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")
        f.write("property uchar red\n")
        f.write("property uchar green\n")
        f.write("property uchar blue\n")
        f.write("end_header\n")

        # Data
        for x, y, z, r, g, b in points_with_colors:
            f.write(f"{x} {y} {z} {r} {g} {b}\n")

def main():
    result_dir = "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp/Result"

    # Check if results exist
    if not os.path.exists(result_dir):
        print(f"Results directory not found: {result_dir}")
        return

    print("=== Generating PLY files for NURBS Pot A assembly ===\n")

    total_pieces = 8
    all_surface_points = []
    all_edge_points = []

    # Process each piece
    for piece_num in range(1, total_pieces + 1):
        surface_file = os.path.join(result_dir, f"{piece_num}. surface.xyz")
        edge_file = os.path.join(result_dir, f"{piece_num}. edge.xyz")

        color = get_color_for_piece(piece_num - 1, total_pieces)

        # Read surface points
        surface_points = read_xyz_file(surface_file)
        if surface_points:
            print(f"Piece {piece_num}: {len(surface_points)} surface points")
            for x, y, z in surface_points:
                all_surface_points.append((x, y, z, color[0], color[1], color[2]))
        else:
            print(f"Piece {piece_num}: No surface file found")

        # Read edge points
        edge_points = read_xyz_file(edge_file)
        if edge_points:
            print(f"Piece {piece_num}: {len(edge_points)} edge points")
            for x, y, z in edge_points:
                all_edge_points.append((x, y, z, color[0], color[1], color[2]))

    # Write combined assembly PLY
    if all_surface_points:
        output_file = os.path.join(result_dir, "pot_a_assembly_complete.ply")
        write_ply(output_file, all_surface_points)
        print(f"\n✓ Created: {output_file}")
        print(f"  Total points: {len(all_surface_points)}")
    else:
        print("\n✗ No surface points found - cannot create assembly PLY")

    # Write combined edges PLY
    if all_edge_points:
        output_file = os.path.join(result_dir, "pot_a_edges_all.ply")
        write_ply(output_file, all_edge_points)
        print(f"\n✓ Created: {output_file}")
        print(f"  Total edge points: {len(all_edge_points)}")
    else:
        print("\n✗ No edge points found - cannot create edges PLY")

    print("\n=== PLY generation complete ===")

if __name__ == "__main__":
    main()
