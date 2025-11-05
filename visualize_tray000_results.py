#!/usr/bin/env python3
"""
Visualize Tray-000 Assembly Results from Legacy SFS++
"""

import numpy as np
import open3d as o3d
import os
import sys
from pathlib import Path

# Result directory
RESULT_DIR = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result"

def load_xyz_file(filepath):
    """Load XYZ point cloud file (x y z format or x y z nx ny nz format)"""
    try:
        data = np.loadtxt(filepath)
        if len(data) == 0:
            print(f"  WARNING: Empty file: {filepath}")
            return None

        # Handle both 3-column (x y z) and 6-column (x y z nx ny nz) formats
        if data.shape[1] >= 3:
            points = data[:, :3]
            pcd = o3d.geometry.PointCloud()
            pcd.points = o3d.utility.Vector3dVector(points)

            # If normals are present, use them
            if data.shape[1] >= 6:
                normals = data[:, 3:6]
                pcd.normals = o3d.utility.Vector3dVector(normals)
            else:
                pcd.estimate_normals()

            return pcd
        else:
            print(f"  WARNING: Invalid data shape: {data.shape}")
            return None
    except Exception as e:
        print(f"  ERROR loading {filepath}: {e}")
        return None

def get_color_for_piece(piece_num, total_pieces):
    """Generate distinct colors for each piece using HSV color space"""
    import colorsys
    hue = piece_num / total_pieces
    rgb = colorsys.hsv_to_rgb(hue, 0.8, 0.9)
    return rgb

def visualize_all_pieces():
    """Visualize all assembled pieces together"""
    print("=" * 80)
    print("TRAY-000 ASSEMBLY VISUALIZATION - ALL PIECES")
    print("=" * 80)
    print()

    # Find all surface files
    surface_files = sorted(Path(RESULT_DIR).glob("*.surface.xyz"))

    if not surface_files:
        print(f"ERROR: No surface files found in {RESULT_DIR}")
        print("Looking for files matching pattern: *.surface.xyz")
        print()
        print("Available files:")
        for f in os.listdir(RESULT_DIR):
            print(f"  {f}")
        return

    print(f"Found {len(surface_files)} assembled surface files")
    print()

    all_clouds = []
    piece_info = []

    for i, filepath in enumerate(surface_files):
        filename = filepath.name
        # Extract piece number from filename (e.g., "1. surface.xyz" -> 1)
        piece_num = int(filename.split('.')[0])

        print(f"Loading piece {piece_num}: {filename}")
        pcd = load_xyz_file(str(filepath))

        if pcd is not None and len(pcd.points) > 0:
            # Assign color to piece
            color = get_color_for_piece(i, len(surface_files))
            pcd.paint_uniform_color(color)

            all_clouds.append(pcd)
            piece_info.append({
                'num': piece_num,
                'points': len(pcd.points),
                'color': color
            })
            print(f"  ✓ Loaded {len(pcd.points)} points")
        else:
            print(f"  ✗ Failed to load or empty file")
        print()

    if not all_clouds:
        print("ERROR: No valid point clouds loaded!")
        return

    print("=" * 80)
    print("ASSEMBLY SUMMARY")
    print("=" * 80)
    print(f"Total pieces loaded: {len(all_clouds)}")
    print(f"Total points: {sum(info['points'] for info in piece_info):,}")
    print()
    print("Piece breakdown:")
    for info in piece_info:
        print(f"  Piece {info['num']:2d}: {info['points']:6,} points - RGB{tuple(int(c*255) for c in info['color'])}")
    print()

    # Visualize
    print("Opening 3D visualization...")
    print()
    print("Visualization Controls:")
    print("  - Mouse left: Rotate")
    print("  - Mouse right: Zoom")
    print("  - Mouse middle: Pan")
    print("  - Q: Quit")
    print("  - +/-: Increase/decrease point size")
    print()

    o3d.visualization.draw_geometries(
        all_clouds,
        window_name=f"Tray-000 Assembly - {len(all_clouds)} pieces",
        width=1600,
        height=1200,
        point_show_normal=False
    )

def visualize_individual_piece(piece_num):
    """Visualize a single assembled piece"""
    print(f"Visualizing Piece {piece_num}...")

    surface_file = Path(RESULT_DIR) / f"{piece_num}. surface.xyz"
    edge_file = Path(RESULT_DIR) / f"{piece_num}. edge.xyz"

    clouds = []

    # Load surface
    if surface_file.exists():
        print(f"Loading surface: {surface_file.name}")
        surface = load_xyz_file(str(surface_file))
        if surface is not None and len(surface.points) > 0:
            surface.paint_uniform_color([0.8, 0.8, 0.8])  # Light gray
            clouds.append(surface)
            print(f"  ✓ Surface: {len(surface.points)} points")

    # Load edges
    if edge_file.exists():
        print(f"Loading edges: {edge_file.name}")
        edges = load_xyz_file(str(edge_file))
        if edges is not None and len(edges.points) > 0:
            edges.paint_uniform_color([1.0, 0.0, 0.0])  # Red
            clouds.append(edges)
            print(f"  ✓ Edges: {len(edges.points)} points")

    if not clouds:
        print(f"ERROR: No data found for piece {piece_num}")
        return

    print()
    o3d.visualization.draw_geometries(
        clouds,
        window_name=f"Tray-000 Piece {piece_num}",
        width=1200,
        height=900,
        point_show_normal=False
    )

def list_available_pieces():
    """List all available assembled pieces"""
    print("=" * 80)
    print("AVAILABLE ASSEMBLED PIECES")
    print("=" * 80)
    print()

    surface_files = sorted(Path(RESULT_DIR).glob("*.surface.xyz"))

    if not surface_files:
        print(f"No assembled pieces found in: {RESULT_DIR}")
        return []

    piece_numbers = []
    for filepath in surface_files:
        filename = filepath.name
        piece_num = int(filename.split('.')[0])
        file_size = filepath.stat().st_size

        # Count points
        try:
            data = np.loadtxt(str(filepath))
            num_points = len(data) if len(data.shape) == 2 else 1
        except:
            num_points = 0

        piece_numbers.append(piece_num)
        status = "✓" if num_points > 0 else "✗ EMPTY"
        print(f"  Piece {piece_num:2d}: {num_points:6,} points ({file_size/1024:.1f} KB) {status}")

    print()
    print(f"Total: {len(piece_numbers)} pieces assembled")
    print()

    return piece_numbers

def main():
    print()
    print("╔════════════════════════════════════════════════════════════════════════════╗")
    print("║         TRAY-000 LEGACY SFS++ ASSEMBLY VISUALIZATION                      ║")
    print("╚════════════════════════════════════════════════════════════════════════════╝")
    print()

    if not os.path.exists(RESULT_DIR):
        print(f"ERROR: Result directory not found: {RESULT_DIR}")
        return 1

    # List available pieces
    piece_numbers = list_available_pieces()

    if not piece_numbers:
        print("No pieces to visualize!")
        return 1

    # Menu
    print("=" * 80)
    print("VISUALIZATION OPTIONS")
    print("=" * 80)
    print()
    print("1. View all pieces together (colored by piece)")
    print("2. View individual piece (with surfaces and edges)")
    print("3. View specific pieces together")
    print("0. Exit")
    print()

    choice = input("Select option [1]: ").strip()
    if not choice:
        choice = "1"

    if choice == "1":
        visualize_all_pieces()
    elif choice == "2":
        piece_str = input(f"Enter piece number {piece_numbers}: ").strip()
        try:
            piece_num = int(piece_str)
            if piece_num in piece_numbers:
                visualize_individual_piece(piece_num)
            else:
                print(f"ERROR: Piece {piece_num} not available")
        except ValueError:
            print("ERROR: Invalid piece number")
    elif choice == "3":
        pieces_str = input("Enter piece numbers (comma-separated): ").strip()
        try:
            selected = [int(p.strip()) for p in pieces_str.split(',')]
            # Similar to visualize_all_pieces but filtered
            print("Not implemented yet - use option 1 to see all pieces")
        except ValueError:
            print("ERROR: Invalid input")
    elif choice == "0":
        print("Exiting...")
    else:
        print("Invalid option")

    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nVisualization cancelled by user")
        sys.exit(1)
