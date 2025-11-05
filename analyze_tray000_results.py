#!/usr/bin/env python3
"""
Analyze Tray-000 Assembly Results from Legacy SFS++ (Non-interactive)
Generates statistics and analysis without requiring graphical display
"""

import numpy as np
import os
from pathlib import Path
from datetime import datetime

# Result directory
RESULT_DIR = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result"

def load_xyz_file(filepath):
    """Load XYZ point cloud file"""
    try:
        data = np.loadtxt(filepath)
        if len(data) == 0:
            return None, 0
        if len(data.shape) == 1:
            return data, 1
        return data, len(data)
    except Exception as e:
        return None, 0

def load_transformation_matrix(filepath):
    """Load transformation matrix from file"""
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
            matrix = []
            for line in lines:
                row = [float(x) for x in line.strip().split()]
                matrix.append(row)
            return np.array(matrix)
    except Exception as e:
        return None

def analyze_assembly():
    """Analyze the assembly results"""
    print("=" * 100)
    print(" " * 30 + "TRAY-000 ASSEMBLY ANALYSIS")
    print("=" * 100)
    print()
    print(f"Result Directory: {RESULT_DIR}")
    print(f"Analysis Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()

    # Find all result files
    surface_files = sorted(Path(RESULT_DIR).glob("*surface.xyz"))
    edge_files = sorted(Path(RESULT_DIR).glob("*edge.xyz"))
    transform_files = sorted(Path(RESULT_DIR).glob("T_*.txt"))

    print("=" * 100)
    print("FILE COUNTS")
    print("=" * 100)
    print(f"  Surface files:        {len(surface_files)}")
    print(f"  Edge files:           {len(edge_files)}")
    print(f"  Transformation files: {len(transform_files)}")
    print()

    # Analyze each surface file
    print("=" * 100)
    print("ASSEMBLED PIECES ANALYSIS")
    print("=" * 100)
    print()
    print(f"{'Piece':<8} {'Surface Points':<15} {'Edge Points':<15} {'Size (KB)':<12} {'Status':<10}")
    print("-" * 100)

    piece_data = []
    total_surface_points = 0
    total_edge_points = 0
    successful_pieces = 0

    for surface_file in surface_files:
        piece_num = int(surface_file.name.split('.')[0])

        # Load surface
        surface_data, surface_points = load_xyz_file(str(surface_file))
        surface_size = surface_file.stat().st_size

        # Load corresponding edge file
        edge_file = Path(RESULT_DIR) / f"{piece_num}. edge.xyz"
        edge_points = 0
        if edge_file.exists():
            edge_data, edge_points = load_xyz_file(str(edge_file))

        # Determine status
        if surface_points > 0:
            status = "✓ OK"
            successful_pieces += 1
            total_surface_points += surface_points
            total_edge_points += edge_points
        else:
            status = "✗ EMPTY"

        piece_data.append({
            'num': piece_num,
            'surface_points': surface_points,
            'edge_points': edge_points,
            'size': surface_size,
            'status': status
        })

        print(f"{piece_num:<8} {surface_points:<15,} {edge_points:<15,} {surface_size/1024:<12.1f} {status:<10}")

    print("-" * 100)
    print(f"{'TOTAL':<8} {total_surface_points:<15,} {total_edge_points:<15,} {'':<12} {successful_pieces}/{len(surface_files)} OK")
    print()

    # Assembly summary
    print("=" * 100)
    print("ASSEMBLY SUMMARY")
    print("=" * 100)
    print(f"  Input Dataset:          Tray-000 (40 pieces)")
    print(f"  Successfully Assembled: {successful_pieces} pieces ({successful_pieces/40*100:.1f}%)")
    print(f"  Failed to Assemble:     {40 - successful_pieces} pieces")
    print(f"  Total Surface Points:   {total_surface_points:,}")
    print(f"  Total Edge Points:      {total_edge_points:,}")
    print(f"  Avg Points/Piece:       {total_surface_points/max(successful_pieces,1):,.0f} surface, {total_edge_points/max(successful_pieces,1):,.0f} edge")
    print()

    # Transformation analysis
    print("=" * 100)
    print("TRANSFORMATION MATRICES")
    print("=" * 100)
    print()
    print(f"Total transformation files: {len(transform_files)}")
    print()

    if transform_files:
        print("Transformation pairs:")
        for tf in sorted(transform_files)[:20]:  # Show first 20
            print(f"  {tf.name}")
        if len(transform_files) > 20:
            print(f"  ... and {len(transform_files) - 20} more")
    print()

    # Piece connectivity analysis
    print("=" * 100)
    print("CONNECTIVITY ANALYSIS")
    print("=" * 100)
    print()

    assembled_pieces = [p['num'] for p in piece_data if p['surface_points'] > 0]
    missing_pieces = [i for i in range(1, 41) if i not in assembled_pieces]

    print(f"Assembled pieces ({len(assembled_pieces)}):")
    print(f"  {', '.join(map(str, sorted(assembled_pieces)))}")
    print()

    print(f"Missing pieces ({len(missing_pieces)}):")
    if missing_pieces:
        print(f"  {', '.join(map(str, sorted(missing_pieces)))}")
    else:
        print(f"  None - All pieces assembled!")
    print()

    # Size distribution
    print("=" * 100)
    print("POINT CLOUD SIZE DISTRIBUTION")
    print("=" * 100)
    print()

    valid_pieces = [p for p in piece_data if p['surface_points'] > 0]
    if valid_pieces:
        surface_counts = [p['surface_points'] for p in valid_pieces]
        print(f"  Minimum:  {min(surface_counts):,} points")
        print(f"  Maximum:  {max(surface_counts):,} points")
        print(f"  Mean:     {np.mean(surface_counts):,.0f} points")
        print(f"  Median:   {np.median(surface_counts):,.0f} points")
        print(f"  Std Dev:  {np.std(surface_counts):,.0f} points")
    print()

    # Comparison with input data
    print("=" * 100)
    print("INPUT vs OUTPUT COMPARISON")
    print("=" * 100)
    print()

    input_dir = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp"

    # Sample a few pieces to compare
    for piece_num in [1, 13, 28, 40]:
        if piece_num in assembled_pieces:
            piece_info = next(p for p in piece_data if p['num'] == piece_num)

            # Load input surface
            input_surface = Path(input_dir) / "Surfaces" / f"Tray-000_Piece_{piece_num:02d}_Surface_0.xyz"
            if input_surface.exists():
                input_data, input_points = load_xyz_file(str(input_surface))
                output_points = piece_info['surface_points']

                print(f"Piece {piece_num:02d}:")
                print(f"  Input:  {input_points:,} points (Surface_0)")
                print(f"  Output: {output_points:,} points (assembled)")
                print(f"  Ratio:  {output_points/max(input_points,1):.2f}x")
                print()

    print("=" * 100)
    print("ANALYSIS COMPLETE")
    print("=" * 100)
    print()
    print("Next Steps:")
    print("  1. Visualize results: python visualize_tray000_results.py")
    print("  2. Try modified SFS++ system with pottery-aware validation")
    print("  3. Adjust assembly parameters and re-run")
    print()

    return piece_data, assembled_pieces, missing_pieces

if __name__ == "__main__":
    piece_data, assembled, missing = analyze_assembly()
