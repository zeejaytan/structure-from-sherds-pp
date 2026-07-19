#!/usr/bin/env python3

import numpy as np
import os

def load_ground_truth_matrix(piece_num):
    """Load ground truth transformation matrix for a piece"""
    gt_file = f"/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Ground Truth/Pot_A_Piece_{piece_num}_T.txt"
    
    if not os.path.exists(gt_file):
        return None
    
    try:
        # Read the 4x4 transformation matrix
        matrix = np.loadtxt(gt_file)
        return matrix
    except Exception as e:
        print(f"Error loading ground truth for piece {piece_num}: {e}")
        return None

def parse_sfs_transformation(rotation_str, translation_str):
    """Parse SFS transformation from assembly_data.txt format"""
    try:
        # Extract rotation matrix (3x3) from string like "[0.998249,0.0389876,0.0444947;-0.0391471,0.99923,0.00272046;-0.0443543,-0.00445754,0.999006]"
        rot_clean = rotation_str.strip('[]')
        rows = rot_clean.split(';')
        rot_matrix = []
        for row in rows:
            values = [float(x) for x in row.split(',')]
            rot_matrix.append(values)
        rot_matrix = np.array(rot_matrix)
        
        # Extract translation vector from string like "[5.24098,0.557516,2.90612]"
        trans_clean = translation_str.strip('[]')
        trans_vector = np.array([float(x) for x in trans_clean.split(',')])
        
        # Create 4x4 transformation matrix
        transform = np.eye(4)
        transform[:3, :3] = rot_matrix
        transform[:3, 3] = trans_vector
        
        return transform
    except Exception as e:
        print(f"Error parsing SFS transformation: {e}")
        return None

def load_sfs_transformations():
    """Load current SFS transformations from assembly_data.txt"""
    sfs_transforms = {}
    
    try:
        with open('visual_output/assembly_data.txt', 'r') as f:
            lines = f.readlines()
        
        current_piece = None
        for i, line in enumerate(lines):
            original_line = line
            line = line.strip()
            
            if line.startswith('Piece ') and ': INCLUDED' in line:
                current_piece = int(line.split()[1].rstrip(':'))
                print(f"Processing piece {current_piece}")
            elif original_line.startswith('  Rotation:') and current_piece:
                rotation_str = original_line.split('Rotation: ')[1].strip()
                print(f"  Found rotation: {rotation_str}")
                # Look for translation on next line
                if i+1 < len(lines) and lines[i+1].startswith('  Translation:'):
                    translation_str = lines[i+1].split('Translation: ')[1].strip()
                    print(f"  Found translation: {translation_str}")
                    
                    transform = parse_sfs_transformation(rotation_str, translation_str)
                    if transform is not None:
                        sfs_transforms[current_piece] = transform
                        print(f"  Successfully parsed transformation for piece {current_piece}")
                    else:
                        print(f"  Failed to parse transformation for piece {current_piece}")
    
    except Exception as e:
        print(f"Error loading SFS transformations: {e}")
        import traceback
        traceback.print_exc()
    
    return sfs_transforms

def matrix_difference_analysis(gt_matrix, sfs_matrix):
    """Analyze differences between ground truth and SFS matrices"""
    
    # Extract rotation and translation components
    gt_rot = gt_matrix[:3, :3]
    gt_trans = gt_matrix[:3, 3]
    sfs_rot = sfs_matrix[:3, :3]
    sfs_trans = sfs_matrix[:3, 3]
    
    # Calculate rotation difference using Frobenius norm
    rot_diff = np.linalg.norm(gt_rot - sfs_rot, 'fro')
    
    # Calculate translation difference (Euclidean distance)
    trans_diff = np.linalg.norm(gt_trans - sfs_trans)
    
    # Calculate rotation angle difference
    # Using trace of relative rotation matrix
    relative_rot = gt_rot.T @ sfs_rot
    trace_val = np.trace(relative_rot)
    # Clamp trace to valid range for arccos
    trace_val = np.clip(trace_val, -1, 3)  # trace of 3x3 rotation matrix is in [-1, 3]
    angle_diff = np.arccos((trace_val - 1) / 2) * 180 / np.pi
    
    return {
        'rotation_frobenius_norm': rot_diff,
        'translation_distance': trans_diff, 
        'rotation_angle_degrees': angle_diff
    }

def compare_all_pieces():
    """Compare all pieces against ground truth"""
    
    print("GROUND TRUTH vs SFS TRANSFORMATION COMPARISON")
    print("=" * 60)
    
    sfs_transforms = load_sfs_transformations()
    
    if not sfs_transforms:
        print("❌ Could not load SFS transformations from assembly_data.txt")
        return
    
    print(f"✅ Loaded SFS transformations for {len(sfs_transforms)} pieces")
    
    # Color mapping for reference
    piece_colors = {1: "Red", 2: "Green", 3: "Blue", 4: "Yellow", 5: "Magenta", 6: "Cyan", 7: "Orange", 8: "Purple"}
    
    comparison_results = []
    
    for piece_num in range(1, 9):
        print(f"\n--- Piece {piece_num} ({piece_colors[piece_num]}) ---")
        
        # Load ground truth
        gt_matrix = load_ground_truth_matrix(piece_num)
        if gt_matrix is None:
            print(f"❌ Could not load ground truth for piece {piece_num}")
            continue
        
        # Get SFS transformation
        if piece_num not in sfs_transforms:
            print(f"❌ No SFS transformation found for piece {piece_num}")
            continue
        
        sfs_matrix = sfs_transforms[piece_num]
        
        # Compare transformations
        differences = matrix_difference_analysis(gt_matrix, sfs_matrix)
        
        # Assess severity
        severity = "🟢 GOOD"
        if differences['translation_distance'] > 50 or differences['rotation_angle_degrees'] > 30:
            severity = "🔴 POOR"
        elif differences['translation_distance'] > 20 or differences['rotation_angle_degrees'] > 15:
            severity = "🟡 MODERATE"
        
        print(f"Ground Truth Translation: [{gt_matrix[0,3]:8.2f}, {gt_matrix[1,3]:8.2f}, {gt_matrix[2,3]:8.2f}]")
        print(f"SFS Translation:         [{sfs_matrix[0,3]:8.2f}, {sfs_matrix[1,3]:8.2f}, {sfs_matrix[2,3]:8.2f}]")
        print(f"Translation Distance: {differences['translation_distance']:8.2f}")
        print(f"Rotation Angle Diff:  {differences['rotation_angle_degrees']:8.2f}°")
        print(f"Assessment: {severity}")
        
        comparison_results.append({
            'piece': piece_num,
            'color': piece_colors[piece_num],
            'severity': severity,
            **differences
        })
    
    # Summary of problematic pieces
    print(f"\n\nSUMMARY - PIECES WITH MAJOR POSITIONING ISSUES")
    print("=" * 60)
    
    problematic_pieces = [r for r in comparison_results if "🔴" in r['severity']]
    moderate_pieces = [r for r in comparison_results if "🟡" in r['severity']]
    
    if problematic_pieces:
        print("🔴 MAJOR ISSUES:")
        for result in sorted(problematic_pieces, key=lambda x: x['translation_distance'], reverse=True):
            print(f"  Piece {result['piece']} ({result['color']:<7}): "
                  f"Translation off by {result['translation_distance']:6.1f}, "
                  f"Rotation off by {result['rotation_angle_degrees']:5.1f}°")
    
    if moderate_pieces:
        print("\n🟡 MODERATE ISSUES:")
        for result in sorted(moderate_pieces, key=lambda x: x['translation_distance'], reverse=True):
            print(f"  Piece {result['piece']} ({result['color']:<7}): "
                  f"Translation off by {result['translation_distance']:6.1f}, "
                  f"Rotation off by {result['rotation_angle_degrees']:5.1f}°")
    
    good_pieces = [r for r in comparison_results if "🟢" in r['severity']]
    if good_pieces:
        print(f"\n🟢 WELL-POSITIONED PIECES: {len(good_pieces)}")
        for result in good_pieces:
            print(f"  Piece {result['piece']} ({result['color']})")
    
    # Save detailed report
    with open('visual_output/GROUND_TRUTH_COMPARISON.txt', 'w') as f:
        f.write("GROUND TRUTH vs SFS RECONSTRUCTION COMPARISON\n")
        f.write("=" * 50 + "\n\n")
        
        f.write("TRANSFORMATION ACCURACY ASSESSMENT:\n")
        for result in comparison_results:
            f.write(f"\nPiece {result['piece']} ({result['color']}):\n")
            f.write(f"  Translation distance: {result['translation_distance']:.2f}\n")
            f.write(f"  Rotation angle diff: {result['rotation_angle_degrees']:.2f}°\n")
            f.write(f"  Status: {result['severity']}\n")
        
        f.write(f"\nSUMMARY:\n")
        f.write(f"  Major issues: {len(problematic_pieces)} pieces\n")
        f.write(f"  Moderate issues: {len(moderate_pieces)} pieces\n")
        f.write(f"  Well-positioned: {len(good_pieces)} pieces\n")
    
    print(f"\n📊 Detailed comparison saved to: visual_output/GROUND_TRUTH_COMPARISON.txt")

if __name__ == "__main__":
    compare_all_pieces()