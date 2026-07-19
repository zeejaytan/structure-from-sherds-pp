#!/usr/bin/env python3
"""
Comprehensive Data Comparison: Sample vs Our Generated Data
Compares ALL pieces, ALL features to understand systemic differences
"""

import os
import numpy as np
# import pandas as pd  # Not available, using pure Python
from pathlib import Path

def extract_match_counts(log_file):
    """Extract all piece-to-piece match counts from job log"""
    matches = {}
    try:
        with open(log_file, 'r') as f:
            for line in f:
                if 'Pieces ' in line and 'matches' in line:
                    # Parse "Pieces 3-2: 17 matches"
                    parts = line.strip().split()
                    if len(parts) >= 3:
                        piece_pair = parts[1].rstrip(':')
                        match_count = int(parts[2])
                        matches[piece_pair] = match_count
    except Exception as e:
        print(f"Error reading {log_file}: {e}")
    return matches

def analyze_breakline_files(breakline_dir, piece_ids=[1,2,3,4,5,6,7,8]):
    """Analyze breakline files for geometric statistics"""
    stats = {}
    
    for piece_id in piece_ids:
        piece_stats = {'breaklines': []}
        
        for breakline_id in [0, 1]:
            filename = f"Pot_A_Piece_{piece_id:02d}_Breakline_{breakline_id}.pcd"
            filepath = os.path.join(breakline_dir, filename)
            
            if os.path.exists(filepath):
                try:
                    breakline_stats = analyze_pcd_file(filepath)
                    piece_stats['breaklines'].append(breakline_stats)
                except Exception as e:
                    print(f"Error analyzing {filepath}: {e}")
                    piece_stats['breaklines'].append({'error': str(e)})
            else:
                piece_stats['breaklines'].append({'missing': True})
        
        stats[f'piece_{piece_id}'] = piece_stats
    
    return stats

def analyze_pcd_file(filepath):
    """Extract statistics from PCD file"""
    stats = {
        'filename': os.path.basename(filepath),
        'file_size': os.path.getsize(filepath),
        'point_count': 0,
        'curvature_stats': {},
        'spatial_stats': {},
        'normal_stats': {}
    }
    
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
            
        # Extract point count
        for line in lines:
            if line.startswith('POINTS'):
                stats['point_count'] = int(line.split()[1])
                break
        
        # Extract data points
        data_started = False
        points = []
        
        for line in lines:
            if line.startswith('DATA'):
                data_started = True
                continue
            
            if data_started and line.strip():
                try:
                    values = [float(x) for x in line.strip().split()]
                    if len(values) >= 7:  # x y z nx ny nz curvature
                        points.append(values)
                except ValueError:
                    continue
        
        if points:
            points = np.array(points)
            
            # Spatial statistics (x, y, z)
            stats['spatial_stats'] = {
                'x_range': [float(points[:, 0].min()), float(points[:, 0].max())],
                'y_range': [float(points[:, 1].min()), float(points[:, 1].max())],
                'z_range': [float(points[:, 2].min()), float(points[:, 2].max())],
                'centroid': [float(points[:, 0].mean()), float(points[:, 1].mean()), float(points[:, 2].mean())]
            }
            
            # Normal statistics (nx, ny, nz)
            stats['normal_stats'] = {
                'nx_range': [float(points[:, 3].min()), float(points[:, 3].max())],
                'ny_range': [float(points[:, 4].min()), float(points[:, 4].max())],
                'nz_range': [float(points[:, 5].min()), float(points[:, 5].max())],
                'normal_magnitude_avg': float(np.sqrt(points[:, 3]**2 + points[:, 4]**2 + points[:, 5]**2).mean())
            }
            
            # Curvature statistics
            curvatures = points[:, 6]
            stats['curvature_stats'] = {
                'min': float(curvatures.min()),
                'max': float(curvatures.max()),
                'mean': float(curvatures.mean()),
                'std': float(curvatures.std()),
                'unique_values': len(np.unique(curvatures)),
                'is_placeholder': len(np.unique(curvatures)) <= 2  # Check if all same values
            }
    
    except Exception as e:
        stats['error'] = str(e)
    
    return stats

def compare_datasets():
    """Main comparison function"""
    print("=== COMPREHENSIVE DATA COMPARISON ===")
    print("Comparing Sample Data vs Our Generated Data")
    print()
    
    # Paths
    sample_breaklines = "/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Breaklines"
    our_breaklines = "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Breaklines"
    
    sample_log = "/data/gpfs/projects/punim2657/sfs_main/sfs_original_baseline_15167065.out"
    our_log = "/data/gpfs/projects/punim2657/sfs_main/sfs_nurbs_timestamped_15385608.out"  # Recent job before tuning
    
    print("📊 ANALYZING MATCH COUNTS...")
    sample_matches = extract_match_counts(sample_log)
    our_matches = extract_match_counts(our_log)
    
    print("\n=== MATCH COUNT COMPARISON ===")
    all_pairs = set(list(sample_matches.keys()) + list(our_matches.keys()))
    
    print(f"{'Pair':<8} {'Sample':<8} {'Our':<8} {'Diff':<8} {'Status'}")
    print("-" * 50)
    
    problematic_pairs = []
    for pair in sorted(all_pairs):
        sample_count = sample_matches.get(pair, 0)
        our_count = our_matches.get(pair, 0)
        diff = our_count - sample_count
        
        if abs(diff) > 5:  # Significant difference
            status = "⚠️  MAJOR"
            problematic_pairs.append((pair, sample_count, our_count, diff))
        elif abs(diff) > 2:
            status = "⚠️  MINOR"
        else:
            status = "✅ OK"
        
        print(f"{pair:<8} {sample_count:<8} {our_count:<8} {diff:<8} {status}")
    
    print(f"\n🚨 PROBLEMATIC PAIRS: {len(problematic_pairs)}")
    for pair, sample, our, diff in problematic_pairs:
        print(f"  {pair}: {sample} → {our} ({diff:+d})")
    
    print("\n📊 ANALYZING GEOMETRIC DATA...")
    sample_stats = analyze_breakline_files(sample_breaklines)
    our_stats = analyze_breakline_files(our_breaklines)
    
    print("\n=== GEOMETRIC COMPARISON ===")
    
    # Compare key problematic pieces (2=Blue, 3=Green) 
    for piece_id in [2, 3]:
        piece_key = f'piece_{piece_id}'
        print(f"\n--- PIECE {piece_id} ({'Blue' if piece_id == 2 else 'Green'}) ---")
        
        if piece_key in sample_stats and piece_key in our_stats:
            sample_piece = sample_stats[piece_key]
            our_piece = our_stats[piece_key]
            
            for breakline_id in [0, 1]:
                if breakline_id < len(sample_piece['breaklines']) and breakline_id < len(our_piece['breaklines']):
                    sample_bl = sample_piece['breaklines'][breakline_id]
                    our_bl = our_piece['breaklines'][breakline_id]
                    
                    print(f"  Breakline {breakline_id}:")
                    if 'error' not in sample_bl and 'error' not in our_bl:
                        print(f"    Points: {sample_bl.get('point_count', 0)} → {our_bl.get('point_count', 0)}")
                        print(f"    File size: {sample_bl.get('file_size', 0)} → {our_bl.get('file_size', 0)} bytes")
                        
                        # Curvature comparison
                        sample_curv = sample_bl.get('curvature_stats', {})
                        our_curv = our_bl.get('curvature_stats', {})
                        
                        print(f"    Curvature unique values: {sample_curv.get('unique_values', 0)} → {our_curv.get('unique_values', 0)}")
                        print(f"    Curvature range: [{sample_curv.get('min', 0):.2e}, {sample_curv.get('max', 0):.2e}] → [{our_curv.get('min', 0):.2e}, {our_curv.get('max', 0):.2e}]")
                        
                        # Spatial comparison
                        sample_spatial = sample_bl.get('spatial_stats', {})
                        our_spatial = our_bl.get('spatial_stats', {})
                        
                        if sample_spatial and our_spatial:
                            sample_centroid = sample_spatial.get('centroid', [0,0,0])
                            our_centroid = our_spatial.get('centroid', [0,0,0])
                            centroid_diff = np.sqrt(sum([(a-b)**2 for a,b in zip(sample_centroid, our_centroid)]))
                            print(f"    Centroid distance: {centroid_diff:.2f}")
    
    # Overall statistics
    print("\n=== OVERALL COMPARISON SUMMARY ===")
    total_sample_matches = sum(sample_matches.values())
    total_our_matches = sum(our_matches.values())
    print(f"Total matches: {total_sample_matches} → {total_our_matches} ({total_our_matches - total_sample_matches:+d})")
    
    # Check for systematic issues
    print("\n🔍 SYSTEMATIC ISSUES DETECTED:")
    print(f"1. High-difference pairs: {len(problematic_pairs)} pairs with >5 match difference")
    print(f"2. False positives: Pairs with 0→high matches")
    print(f"3. False negatives: Pairs with high→0 matches")
    
    # Generate recommendations
    print("\n💡 RECOMMENDATIONS:")
    if len(problematic_pairs) > 0:
        print("1. Focus on geometric preprocessing differences causing match count variations")
        print("2. Investigate breakline extraction differences in high-variation pieces")
        print("3. Consider systematic parameter adjustment rather than surgical fixes")
    
    print("\n=== Analysis Complete ===")

if __name__ == "__main__":
    compare_datasets()