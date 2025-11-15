#!/usr/bin/env python3
"""
Visualization script for legacy SFS++ results
Combines all pottery pieces into a single colored PLY file
"""

import numpy as np
import sys
import os

O3D = False
try:
    import open3d as o3d
    O3D = True
    print("✓ Open3D imported successfully")
except ImportError:
    print("INFO: Open3D not found; will write PLY manually and skip interactive view.")

# Configuration (can be overridden by CLI or env)
DEFAULT_RESULT_DIR = '/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/'
RESULT_DIR = os.environ.get('SFS_RESULT_DIR', DEFAULT_RESULT_DIR)
OUTPUT_DIR = None  # defaults to RESULT_DIR
NUM_PIECES = None  # auto-detect if None

# Color palette for each piece (RGB values 0-1)
COLORS = [
    [1.0, 0.0, 0.0],     # Red - Piece 1
    [0.0, 1.0, 0.0],     # Green - Piece 2
    [0.0, 0.0, 1.0],     # Blue - Piece 3
    [1.0, 1.0, 0.0],     # Yellow - Piece 4
    [1.0, 0.0, 1.0],     # Magenta - Piece 5
    [0.0, 1.0, 1.0],     # Cyan - Piece 6
    [1.0, 0.5, 0.0],     # Orange - Piece 7
    [0.5, 0.0, 1.0]      # Purple - Piece 8
]

def load_xyz_file(filepath):
    """Load XYZ point cloud file"""
    if not os.path.exists(filepath):
        print(f"WARNING: File not found: {filepath}")
        return None

    try:
        data = np.loadtxt(filepath)
        print(f"  Loaded {len(data):,} points from {os.path.basename(filepath)}")
        return data
    except Exception as e:
        print(f"ERROR loading {filepath}: {e}")
        return None

def create_colored_point_cloud(points, color):
    """Create point cloud structure with colors; Open3D if available"""
    if O3D:
        pcd = o3d.geometry.PointCloud()
        pcd.points = o3d.utility.Vector3dVector(points)
        pcd.colors = o3d.utility.Vector3dVector([color] * len(points))
        return pcd
    else:
        cols = np.tile(color, (len(points), 1))
        return {'points': np.asarray(points), 'colors': cols}

def detect_piece_ids():
    ids = []
    for name in os.listdir(RESULT_DIR):
        if name.endswith('. surface.xyz'):
            try:
                n = int(name.split('.')[0].strip())
                ids.append(n)
            except Exception:
                pass
    ids.sort()
    return ids

def combine_pieces():
    """Combine all pottery pieces into a single point cloud"""
    print("\n=== Loading Pottery Pieces ===")
    all_points = []
    all_colors = []
    piece_info = []

    piece_ids = detect_piece_ids() if NUM_PIECES is None else list(range(1, NUM_PIECES + 1))
    for i in piece_ids:
        surface_file = os.path.join(RESULT_DIR, f"{i}. surface.xyz")
        points = load_xyz_file(surface_file)

        if points is not None:
            num_points = len(points)
            all_points.append(points)
            color = COLORS[(i-1) % len(COLORS)]
            all_colors.append(np.tile(color, (num_points, 1)))
            piece_info.append({
                'piece': i,
                'points': num_points,
                'color': color
            })

    if not all_points:
        print("ERROR: No point cloud data loaded!")
        return None, None

    # Combine all pieces
    combined_points = np.vstack(all_points)
    combined_colors = np.vstack(all_colors)

    print(f"\n=== Assembly Summary ===")
    total_target = len(piece_ids)
    print(f"Total pieces loaded: {len(piece_info)}/{total_target}")
    print(f"Total points: {len(combined_points):,}")
    print(f"\nPiece breakdown:")
    color_names = ['Red','Green','Blue','Yellow','Magenta','Cyan','Orange','Purple']
    for info in piece_info:
        cname = color_names[(info['piece']-1) % len(color_names)]
        print(f"  Piece {info['piece']}: {info['points']:,} points ({cname})")

    # Create combined point cloud
    if O3D:
        combined_pcd = o3d.geometry.PointCloud()
        combined_pcd.points = o3d.utility.Vector3dVector(combined_points)
        combined_pcd.colors = o3d.utility.Vector3dVector(combined_colors)
    else:
        combined_pcd = {'points': combined_points, 'colors': combined_colors}
    return combined_pcd, piece_info

def save_ply(pcd, filename):
    """Save point cloud as PLY file (Open3D if available, else manual)."""
    out_dir = OUTPUT_DIR if OUTPUT_DIR is not None else RESULT_DIR
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir, exist_ok=True)
    filepath = os.path.join(out_dir, filename)
    try:
        if O3D:
            o3d.io.write_point_cloud(filepath, pcd)
        else:
            pts = np.asarray(pcd['points'])
            cols = np.asarray(pcd['colors'])
            cols_u8 = np.clip(cols * 255.0, 0, 255).astype(np.uint8)
            with open(filepath, 'w') as f:
                f.write('ply\nformat ascii 1.0\n')
                f.write(f'element vertex {len(pts)}\n')
                f.write('property float x\nproperty float y\nproperty float z\n')
                f.write('property uchar red\nproperty uchar green\nproperty uchar blue\n')
                f.write('end_header\n')
                for (x,y,z), (r,g,b) in zip(pts, cols_u8):
                    f.write(f'{x:.6f} {y:.6f} {z:.6f} {r} {g} {b}\n')
        file_size = os.path.getsize(filepath)
        print(f"\n✓ PLY file saved: {filepath}")
        print(f"  File size: {file_size / 1024 / 1024:.2f} MB")
        return filepath
    except Exception as e:
        print(f"ERROR saving PLY file: {e}")
        return None

def compute_statistics(pcd):
    """Compute and display point cloud statistics"""
    points = np.asarray(pcd.points) if O3D else np.asarray(pcd['points'])

    print(f"\n=== Point Cloud Statistics ===")
    print(f"Bounding box:")
    print(f"  X: [{points[:, 0].min():.2f}, {points[:, 0].max():.2f}] (range: {points[:, 0].max() - points[:, 0].min():.2f} mm)")
    print(f"  Y: [{points[:, 1].min():.2f}, {points[:, 1].max():.2f}] (range: {points[:, 1].max() - points[:, 1].min():.2f} mm)")
    print(f"  Z: [{points[:, 2].min():.2f}, {points[:, 2].max():.2f}] (range: {points[:, 2].max() - points[:, 2].min():.2f} mm)")

    center = points.mean(axis=0)
    print(f"Centroid: [{center[0]:.2f}, {center[1]:.2f}, {center[2]:.2f}]")

def visualize_interactive(pcd):
    """Launch interactive 3D viewer"""
    print("\n=== Launching Interactive Viewer ===")
    print("Controls:")
    print("  - Mouse left: Rotate")
    print("  - Mouse wheel: Zoom")
    print("  - Mouse right: Pan")
    print("  - Press 'h' for help")
    print("  - Press 'q' or ESC to quit")

    try:
        o3d.visualization.draw_geometries(
            [pcd],
            window_name="Legacy SFS++ Assembly - Pot A",
            width=1200,
            height=800,
            left=100,
            top=100
        )
    except Exception as e:
        print(f"WARNING: Could not launch interactive viewer (headless environment?): {e}")

def main():
    print("="*60)
    print("Legacy SFS++ Results Visualization")
    print("="*60)

    # CLI overrides
    args = sys.argv[1:]
    if '--result' in args:
        i = args.index('--result')
        if i + 1 < len(args):
            res = args[i+1]
            if os.path.isdir(res):
                global RESULT_DIR
                RESULT_DIR = res if res.endswith('/') else res + '/'
            else:
                print(f"ERROR: Provided --result directory not found: {res}")
                sys.exit(1)
    if '--outdir' in args:
        i = args.index('--outdir')
        if i + 1 < len(args):
            out = args[i+1]
            global OUTPUT_DIR
            OUTPUT_DIR = out

    # Check if result directory exists
    if not os.path.exists(RESULT_DIR):
        print(f"ERROR: Result directory not found: {RESULT_DIR}")
        sys.exit(1)

    print(f"Result directory: {RESULT_DIR}")

    # Load and combine all pieces
    combined_pcd, piece_info = combine_pieces()

    if combined_pcd is None:
        print("ERROR: Failed to create combined point cloud")
        sys.exit(1)

    # Compute statistics
    compute_statistics(combined_pcd)

    # Save PLY file
    ply_filename = "pot_a_assembly_legacy.ply"
    ply_path = save_ply(combined_pcd, ply_filename)

    if ply_path is None:
        print("ERROR: Failed to save PLY file")
        sys.exit(1)

    # Check accuracy report
    acc_file = os.path.join(RESULT_DIR, "1. Acc.txt")
    if os.path.exists(acc_file):
        print("\n=== Accuracy Report ===")
        with open(acc_file, 'r') as f:
            print(f.read())

    print("\n=== Visualization Complete ===")
    print(f"✓ PLY file ready: {ply_path}")
    print(f"\nTo visualize:")
    print(f"  meshlab {ply_path}")
    print(f"  OR")
    print(f"  python3 {__file__} --view")

    # Interactive visualization (if requested)
    if O3D and len(sys.argv) > 1 and sys.argv[1] in ['--view', '-v', '--visualize']:
        visualize_interactive(combined_pcd)

    print("\n" + "="*60)

if __name__ == "__main__":
    main()
