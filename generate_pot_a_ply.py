#!/usr/bin/env python3
"""
Convert Pot A Assembly Results to PLY files for visualization
Generates individual PLY files and combined assembly PLY
"""

import numpy as np
from pathlib import Path
import colorsys

# Result directory for Pot A
RESULT_DIR = "/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result"
OUTPUT_DIR = "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/pot_a_vis"

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

def create_combined_assembly():
    """Create a single PLY file with all assembled pieces, colored by piece"""
    print("=" * 80)
    print("CREATING POT A COMBINED ASSEMBLY PLY")
    print("=" * 80)
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

        print(f"  Adding Piece {piece_num}: {len(points):6,} points - RGB({color[0]}, {color[1]}, {color[2]})")

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
    output_file = Path(OUTPUT_DIR) / "pot_a_assembly_complete.ply"
    write_ply_ascii(str(output_file), combined_points, combined_colors, combined_normals)

    print(f"✓ Combined assembly saved to: {output_file}")
    print()

    return True

def create_edges_combined():
    """Create combined PLY file for all edge features (red colored)"""
    print("=" * 80)
    print("CREATING COMBINED EDGE FEATURES PLY")
    print("=" * 80)
    print()

    edge_files = sorted(Path(RESULT_DIR).glob("*edge.xyz"))

    all_points = []
    all_normals = []
    total_edge_points = 0

    for edge_file in edge_files:
        piece_num = int(edge_file.name.split('.')[0])

        # Load edges
        points, normals = load_xyz_file(str(edge_file))

        if points is None or len(points) == 0:
            continue

        all_points.append(points)
        if normals is not None:
            all_normals.append(normals)

        total_edge_points += len(points)
        print(f"  Adding Piece {piece_num} edges: {len(points):4,} points")

    if not all_points:
        print("\nNo edge data found!")
        return False

    # Combine all edge data
    combined_points = np.vstack(all_points)
    combined_normals = np.vstack(all_normals) if all_normals else None

    # Red color for all edges
    colors = np.tile([255, 0, 0], (len(combined_points), 1))

    print()
    print(f"Total edge points: {len(combined_points):,}")
    print()

    # Write combined edges PLY
    output_file = Path(OUTPUT_DIR) / "pot_a_edges_all.ply"
    write_ply_ascii(str(output_file), combined_points, colors, combined_normals)

    print(f"✓ Combined edges saved to: {output_file}")
    print()

    return True

def main():
    print()
    print("╔" + "═" * 78 + "╗")
    print("║" + " " * 20 + "POT A ASSEMBLY → PLY VISUALIZATION" + " " * 24 + "║")
    print("╚" + "═" * 78 + "╝")
    print()

    # Create output directory
    Path(OUTPUT_DIR).mkdir(parents=True, exist_ok=True)
    print(f"Input directory:  {RESULT_DIR}")
    print(f"Output directory: {OUTPUT_DIR}")
    print()

    # Create combined assembly
    assembly_ok = create_combined_assembly()

    # Create combined edges
    edges_ok = create_edges_combined()

    # Summary
    print("=" * 80)
    print("CONVERSION COMPLETE")
    print("=" * 80)
    print()
    print(f"✓ Combined assembly:    pot_a_assembly_complete.ply")
    print(f"✓ Combined edges:       pot_a_edges_all.ply")
    print()
    print("Location: " + OUTPUT_DIR)
    print()
    print("Quick view commands:")
    print(f"  meshlab {OUTPUT_DIR}/pot_a_assembly_complete.ply")
    print(f"  meshlab {OUTPUT_DIR}/pot_a_edges_all.ply")
    print()
    print("Or view both together in CloudCompare/MeshLab")
    print()

    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main())
