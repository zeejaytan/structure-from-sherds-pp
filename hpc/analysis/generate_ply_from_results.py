#!/usr/bin/env python3
"""
Convert Tray-000 Assembly Results to PLY files for visualization
Generates individual PLY files and combined assembly PLY
"""

import numpy as np
from pathlib import Path
import struct
import colorsys

# Result directory
RESULT_DIR = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result"
OUTPUT_DIR = "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/vis_out"

def get_color_for_piece(piece_num, total_pieces):
    """Generate distinct colors for each piece using HSV color space"""
    hue = piece_num / total_pieces
    rgb = colorsys.hsv_to_rgb(hue, 0.8, 0.9)
    # Convert to 0-255 range
    return [int(rgb[0] * 255), int(rgb[1] * 255), int(rgb[2] * 255)]

def write_ply_ascii(filepath, points, colors=None, normals=None):
    """Write PLY file in ASCII format"""
    num_points = len(points)

    with open(filepath, 'w') as f:
        # Write header
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {num_points}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")

        if normals is not None:
            f.write("property float nx\n")
            f.write("property float ny\n")
            f.write("property float nz\n")

        if colors is not None:
            f.write("property uchar red\n")
            f.write("property uchar green\n")
            f.write("property uchar blue\n")

        f.write("end_header\n")

        # Write data
        for i in range(num_points):
            x, y, z = points[i]
            f.write(f"{x} {y} {z}")

            if normals is not None:
                nx, ny, nz = normals[i]
                f.write(f" {nx} {ny} {nz}")

            if colors is not None:
                r, g, b = colors[i]
                f.write(f" {r} {g} {b}")

            f.write("\n")

def load_xyz_file(filepath):
    """Load XYZ file, return points and normals if available"""
    try:
        data = np.loadtxt(filepath)
        if len(data) == 0 or len(data.shape) != 2:
            return None, None

        points = data[:, :3]
        normals = data[:, 3:6] if data.shape[1] >= 6 else None

        return points, normals
    except Exception as e:
        print(f"  ERROR loading {filepath}: {e}")
        return None, None

def convert_individual_pieces():
    """Convert each assembled piece to individual PLY files"""
    print("=" * 100)
    print("CONVERTING INDIVIDUAL PIECES TO PLY")
    print("=" * 100)
    print()

    surface_files = sorted(Path(RESULT_DIR).glob("*surface.xyz"))

    converted = 0
    failed = 0

    for surface_file in surface_files:
        piece_num = int(surface_file.name.split('.')[0])

        # Load surface
        points, normals = load_xyz_file(str(surface_file))

        if points is None or len(points) == 0:
            print(f"Piece {piece_num:2d}: SKIPPED (empty or failed to load)")
            failed += 1
            continue

        # Output filename
        output_file = Path(OUTPUT_DIR) / f"piece_{piece_num:02d}_surface.ply"

        # Generate single color for this piece
        color = get_color_for_piece(piece_num, 40)
        colors = np.tile(color, (len(points), 1))

        # Write PLY
        write_ply_ascii(str(output_file), points, colors, normals)

        print(f"Piece {piece_num:2d}: ✓ {len(points):6,} points → {output_file.name}")
        converted += 1

    print()
    print(f"Converted {converted} pieces, {failed} failed/empty")
    print()
    return converted

def create_combined_assembly():
    """Create a single PLY file with all assembled pieces, colored by piece"""
    print("=" * 100)
    print("CREATING COMBINED ASSEMBLY PLY")
    print("=" * 100)
    print()

    surface_files = sorted(Path(RESULT_DIR).glob("*surface.xyz"))

    all_points = []
    all_normals = []
    all_colors = []
    piece_info = []

    for i, surface_file in enumerate(surface_files):
        piece_num = int(surface_file.name.split('.')[0])

        # Load surface
        points, normals = load_xyz_file(str(surface_file))

        if points is None or len(points) == 0:
            continue

        # Generate color for this piece
        color = get_color_for_piece(i, len(surface_files))
        colors = np.tile(color, (len(points), 1))

        all_points.append(points)
        if normals is not None:
            all_normals.append(normals)
        all_colors.append(colors)

        piece_info.append({
            'num': piece_num,
            'points': len(points),
            'color': color
        })

        print(f"  Adding Piece {piece_num:2d}: {len(points):6,} points - RGB({color[0]}, {color[1]}, {color[2]})")

    if not all_points:
        print("\nERROR: No valid pieces to combine!")
        return False

    # Combine all data
    combined_points = np.vstack(all_points)
    combined_colors = np.vstack(all_colors)
    combined_normals = np.vstack(all_normals) if all_normals else None

    print()
    print(f"Total points: {len(combined_points):,}")
    print(f"Total pieces: {len(piece_info)}")
    print()

    # Write combined PLY
    output_file = Path(OUTPUT_DIR) / "tray000_assembly_all_pieces.ply"
    write_ply_ascii(str(output_file), combined_points, combined_colors, combined_normals)

    print(f"✓ Combined assembly saved to: {output_file}")
    print()

    return True

def create_edges_ply():
    """Create PLY files for edge features (red colored)"""
    print("=" * 100)
    print("CONVERTING EDGE FEATURES TO PLY")
    print("=" * 100)
    print()

    edge_files = sorted(Path(RESULT_DIR).glob("*edge.xyz"))

    converted = 0

    for edge_file in edge_files:
        piece_num = int(edge_file.name.split('.')[0])

        # Load edges
        points, normals = load_xyz_file(str(edge_file))

        if points is None or len(points) == 0:
            continue

        # Output filename
        output_file = Path(OUTPUT_DIR) / f"piece_{piece_num:02d}_edges.ply"

        # Red color for edges
        colors = np.tile([255, 0, 0], (len(points), 1))

        # Write PLY
        write_ply_ascii(str(output_file), points, colors, normals)

        print(f"Piece {piece_num:2d}: ✓ {len(points):4,} edge points → {output_file.name}")
        converted += 1

    print()
    print(f"Converted {converted} edge files")
    print()
    return converted

def create_visualization_guide():
    """Create a text file with visualization instructions"""
    guide_file = Path(OUTPUT_DIR) / "VISUALIZATION_GUIDE.txt"

    with open(guide_file, 'w') as f:
        f.write("=" * 100 + "\n")
        f.write("TRAY-000 ASSEMBLY VISUALIZATION GUIDE\n")
        f.write("=" * 100 + "\n\n")

        f.write("Generated PLY Files:\n")
        f.write("-" * 100 + "\n\n")

        f.write("1. COMBINED ASSEMBLY:\n")
        f.write("   tray000_assembly_all_pieces.ply - All assembled pieces colored by piece\n\n")

        f.write("2. INDIVIDUAL PIECES (Surfaces):\n")
        f.write("   piece_01_surface.ply, piece_03_surface.ply, etc.\n")
        f.write("   Each piece colored uniquely\n\n")

        f.write("3. EDGE FEATURES:\n")
        f.write("   piece_01_edges.ply, piece_03_edges.ply, etc.\n")
        f.write("   Edge features colored red\n\n")

        f.write("=" * 100 + "\n")
        f.write("VIEWING OPTIONS\n")
        f.write("=" * 100 + "\n\n")

        f.write("Option 1: MeshLab (Recommended)\n")
        f.write("-" * 100 + "\n")
        f.write("  meshlab tray000_assembly_all_pieces.ply\n\n")
        f.write("  Controls:\n")
        f.write("    - Trackball: Rotate view\n")
        f.write("    - Shift+Mouse: Pan\n")
        f.write("    - Scroll: Zoom\n")
        f.write("    - Render → Lighting → Toggle lighting\n")
        f.write("    - Render → Render Mode → Points/Flat/Smooth\n\n")

        f.write("Option 2: CloudCompare\n")
        f.write("-" * 100 + "\n")
        f.write("  cloudcompare tray000_assembly_all_pieces.ply\n\n")

        f.write("Option 3: Open3D (Python)\n")
        f.write("-" * 100 + "\n")
        f.write("  import open3d as o3d\n")
        f.write("  pcd = o3d.io.read_point_cloud('tray000_assembly_all_pieces.ply')\n")
        f.write("  o3d.visualization.draw_geometries([pcd])\n\n")

        f.write("=" * 100 + "\n")
        f.write("COLOR SCHEME\n")
        f.write("=" * 100 + "\n\n")
        f.write("Each piece is assigned a unique color using HSV color space:\n")
        f.write("  - Hue varies from 0° to 360° based on piece order\n")
        f.write("  - Saturation: 80%\n")
        f.write("  - Value: 90%\n")
        f.write("  - Edge features: Red (RGB: 255, 0, 0)\n\n")

        f.write("=" * 100 + "\n")
        f.write("DATASET INFORMATION\n")
        f.write("=" * 100 + "\n\n")
        f.write("Dataset: Tray-000 (40 pieces total, 21 assembled)\n")
        f.write("Algorithm: Legacy SFS++ (original author's code)\n")
        f.write("Coordinate Units: Millimeters\n")
        f.write("Assembly Date: 2025-10-24\n\n")

        f.write("Assembled Pieces: 1, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 16, 17, 18, 23, 28, 31, 33, 35, 40\n")
        f.write("Missing Pieces: 2, 11, 15, 19, 20, 21, 22, 24, 25, 26, 27, 29, 30, 32, 34, 36, 37, 38, 39\n\n")

        f.write("=" * 100 + "\n")

    print(f"✓ Visualization guide saved to: {guide_file}")
    print()

def main():
    print()
    print("╔══════════════════════════════════════════════════════════════════════════════════════════════╗")
    print("║               TRAY-000 ASSEMBLY RESULTS → PLY CONVERSION                                    ║")
    print("╚══════════════════════════════════════════════════════════════════════════════════════════════╝")
    print()

    # Create output directory
    Path(OUTPUT_DIR).mkdir(parents=True, exist_ok=True)
    print(f"Output directory: {OUTPUT_DIR}")
    print()

    # Convert individual pieces
    num_surfaces = convert_individual_pieces()

    # Convert edges
    num_edges = create_edges_ply()

    # Create combined assembly
    create_combined_assembly()

    # Create visualization guide
    create_visualization_guide()

    # Summary
    print("=" * 100)
    print("CONVERSION COMPLETE")
    print("=" * 100)
    print()
    print(f"✓ Individual surfaces: {num_surfaces} PLY files")
    print(f"✓ Individual edges:    {num_edges} PLY files")
    print(f"✓ Combined assembly:   1 PLY file (tray000_assembly_all_pieces.ply)")
    print(f"✓ Visualization guide: VISUALIZATION_GUIDE.txt")
    print()
    print(f"Total files created:   {num_surfaces + num_edges + 2}")
    print()
    print("Location: " + OUTPUT_DIR)
    print()
    print("Quick view command:")
    print(f"  meshlab {OUTPUT_DIR}/tray000_assembly_all_pieces.ply")
    print()

    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main())
