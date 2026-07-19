#!/usr/bin/env python3
# Analysis of curvature fix effectiveness for Orange-Green false positive elimination
import numpy as np
import os

def parse_pcd_curvature(filename):
    """Extract curvature values from PCD file"""
    curvatures = []
    try:
        with open(filename, 'r') as f:
            lines = f.readlines()
            
        data_start = False
        for line in lines:
            if data_start and line.strip() and not line.startswith('#'):
                parts = line.strip().split()
                if len(parts) >= 7:  # x y z nx ny nz curvature
                    curvature = float(parts[6])
                    curvatures.append(curvature)
            elif line.strip() == "DATA ascii":
                data_start = True
        return np.array(curvatures) if curvatures else None
    except Exception as e:
        print(f"Error reading {filename}: {e}")
        return None

def analyze_curvature_fix():
    print("=== CURVATURE FIX EFFECTIVENESS ANALYSIS ===")
    print("Comparing Orange (Piece 4) vs Green (Piece 3) curvature signatures")
    print()
    
    # Load fixed curvature data
    dataset_dir = "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Breaklines/"
    
    green_file = os.path.join(dataset_dir, "Pot_A_Piece_03_Breakline_0.pcd")
    orange_file = os.path.join(dataset_dir, "Pot_A_Piece_04_Breakline_0.pcd")
    
    green_curvatures = parse_pcd_curvature(green_file)
    orange_curvatures = parse_pcd_curvature(orange_file)
    
    if green_curvatures is None or orange_curvatures is None:
        print("❌ Could not load curvature data")
        return
    
    print(f"✅ Loaded Green curvatures: {len(green_curvatures)} points")
    print(f"✅ Loaded Orange curvatures: {len(orange_curvatures)} points")
    print()
    
    # Statistical analysis
    print("CURVATURE STATISTICS:")
    print(f"Green (Piece 3):")
    print(f"  Range: {np.min(green_curvatures):.6f} to {np.max(green_curvatures):.6f}")
    print(f"  Mean: {np.mean(green_curvatures):.6f}")
    print(f"  Std: {np.std(green_curvatures):.6f}")
    print()
    
    print(f"Orange (Piece 4):")
    print(f"  Range: {np.min(orange_curvatures):.6f} to {np.max(orange_curvatures):.6f}")
    print(f"  Mean: {np.mean(orange_curvatures):.6f}")
    print(f"  Std: {np.std(orange_curvatures):.6f}")
    print()
    
    # Distinctiveness analysis
    print("DISTINCTIVENESS ANALYSIS:")
    mean_diff = abs(np.mean(green_curvatures) - np.mean(orange_curvatures))
    print(f"Mean difference: {mean_diff:.6f}")
    
    # Check if values are now unique vs previously corrupted (compare first N values)
    min_len = min(len(green_curvatures), len(orange_curvatures))
    identical_values = np.allclose(green_curvatures[:min_len], orange_curvatures[:min_len], rtol=1e-6)
    print(f"Identical curvature patterns: {'YES - STILL CORRUPTED!' if identical_values else 'NO - FIXED!'}")
    
    # Sample comparison
    print()
    print("SAMPLE COMPARISON (first 5 points):")
    print("Green:  [", end="")
    for i in range(min(5, len(green_curvatures))):
        print(f"{green_curvatures[i]:.6f}", end=", " if i < 4 else "")
    print("]")
    
    print("Orange: [", end="")
    for i in range(min(5, len(orange_curvatures))):
        print(f"{orange_curvatures[i]:.6f}", end=", " if i < 4 else "")
    print("]")
    print()
    
    # Prediction for feature matching
    print("FEATURE MATCHING PREDICTION:")
    if identical_values:
        print("❌ Curvatures still identical - expect false positive matches to persist")
    elif mean_diff < 0.001:
        print("⚠️  Curvatures very similar - may still cause some false matches") 
    else:
        print("✅ Curvatures sufficiently distinct - expect false positive elimination")
        
    print()
    print("EXPECTED SFS RECONSTRUCTION OUTCOME:")
    print("- Previous result: 22 false matches between Orange-Green")
    print("- With proper curvature: Should see 0-2 matches (only real geometric connections)")
    print("- Assembly quality: No intersection artifacts between distant pieces")
    
    return green_curvatures, orange_curvatures

if __name__ == "__main__":
    analyze_curvature_fix()