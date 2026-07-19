#!/usr/bin/env python3

import numpy as np
import os

def load_surface_data(filename):
    """Load surface data from XYZ file"""
    try:
        data = np.loadtxt(filename)
        return data[:, :3]  # X, Y, Z coordinates
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def analyze_piece_positions():
    """Analyze spatial positioning of each piece"""
    
    print("ASSEMBLY POSITIONING ANALYSIS")
    print("=" * 50)
    
    piece_info = {
        1: "Red", 2: "Green", 3: "Blue", 4: "Yellow", 
        5: "Magenta", 6: "Cyan", 7: "Orange", 8: "Purple"
    }
    
    piece_data = {}
    
    # Load and analyze each piece
    for piece_num in range(1, 9):
        surface_file = f"visual_output/piece_{piece_num}_surface.xyz"
        
        if os.path.exists(surface_file):
            points = load_surface_data(surface_file)
            
            if points is not None:
                # Calculate spatial statistics
                centroid = np.mean(points, axis=0)
                bounds = {
                    'x_min': np.min(points[:, 0]), 'x_max': np.max(points[:, 0]),
                    'y_min': np.min(points[:, 1]), 'y_max': np.max(points[:, 1]),
                    'z_min': np.min(points[:, 2]), 'z_max': np.max(points[:, 2])
                }
                
                piece_data[piece_num] = {
                    'points': points,
                    'centroid': centroid,
                    'bounds': bounds,
                    'color': piece_info[piece_num]
                }
                
                print(f"\nPiece {piece_num} ({piece_info[piece_num]}):")
                print(f"  Points: {len(points):,}")
                print(f"  Centroid: [{centroid[0]:8.2f}, {centroid[1]:8.2f}, {centroid[2]:8.2f}]")
                print(f"  X range: [{bounds['x_min']:8.2f}, {bounds['x_max']:8.2f}] (span: {bounds['x_max']-bounds['x_min']:6.2f})")
                print(f"  Y range: [{bounds['y_min']:8.2f}, {bounds['y_max']:8.2f}] (span: {bounds['y_max']-bounds['y_min']:6.2f})")
                print(f"  Z range: [{bounds['z_min']:8.2f}, {bounds['z_max']:8.2f}] (span: {bounds['z_max']-bounds['z_min']:6.2f})")
    
    return piece_data

def check_intersections(piece_data):
    """Check for spatial intersections between pieces"""
    
    print(f"\n\nINTERSECTION ANALYSIS")
    print("=" * 50)
    
    intersections = []
    
    for i in range(1, 9):
        for j in range(i+1, 9):
            if i in piece_data and j in piece_data:
                bounds_i = piece_data[i]['bounds']
                bounds_j = piece_data[j]['bounds']
                
                # Check 3D bounding box overlap
                x_overlap = not (bounds_i['x_max'] < bounds_j['x_min'] or bounds_j['x_max'] < bounds_i['x_min'])
                y_overlap = not (bounds_i['y_max'] < bounds_j['y_min'] or bounds_j['y_max'] < bounds_i['y_min'])
                z_overlap = not (bounds_i['z_max'] < bounds_j['z_min'] or bounds_j['z_max'] < bounds_i['z_min'])
                
                if x_overlap and y_overlap and z_overlap:
                    # Calculate overlap volumes
                    x_intersect = min(bounds_i['x_max'], bounds_j['x_max']) - max(bounds_i['x_min'], bounds_j['x_min'])
                    y_intersect = min(bounds_i['y_max'], bounds_j['y_max']) - max(bounds_i['y_min'], bounds_j['y_min'])
                    z_intersect = min(bounds_i['z_max'], bounds_j['z_max']) - max(bounds_i['z_min'], bounds_j['z_min'])
                    
                    overlap_volume = x_intersect * y_intersect * z_intersect
                    
                    intersections.append({
                        'pieces': (i, j),
                        'colors': (piece_data[i]['color'], piece_data[j]['color']),
                        'overlap_volume': overlap_volume,
                        'x_intersect': x_intersect,
                        'y_intersect': y_intersect,
                        'z_intersect': z_intersect
                    })
    
    # Sort by overlap volume (largest first)
    intersections.sort(key=lambda x: x['overlap_volume'], reverse=True)
    
    print(f"Found {len(intersections)} piece intersections:\n")
    
    for idx, intersection in enumerate(intersections[:10]):  # Show top 10
        i, j = intersection['pieces']
        colors = intersection['colors']
        vol = intersection['overlap_volume']
        
        severity = "🔴 MAJOR" if vol > 1000 else "🟡 MINOR" if vol > 100 else "🟢 SMALL"
        
        print(f"{idx+1:2}. Piece {i} ({colors[0]}) ↔ Piece {j} ({colors[1]})")
        print(f"    {severity} - Overlap Volume: {vol:10.2f}")
        print(f"    Dimensions: {intersection['x_intersect']:6.2f} × {intersection['y_intersect']:6.2f} × {intersection['z_intersect']:6.2f}")
        print()
    
    return intersections

def analyze_specific_issues(piece_data):
    """Analyze the specific issues mentioned: Yellow, Blue, Cyan positioning"""
    
    print(f"\nSPECIFIC ISSUE ANALYSIS")
    print("=" * 50)
    
    # Yellow = Piece 4, Blue = Piece 3, Cyan = Piece 6, Purple = Piece 8
    issues = {
        'Yellow (Piece 4)': 4,
        'Blue (Piece 3)': 3,
        'Cyan (Piece 6)': 6,
        'Purple (Piece 8)': 8
    }
    
    for issue_name, piece_num in issues.items():
        if piece_num in piece_data:
            centroid = piece_data[piece_num]['centroid']
            print(f"\n{issue_name}:")
            print(f"  Centroid Position: [{centroid[0]:8.2f}, {centroid[1]:8.2f}, {centroid[2]:8.2f}]")
            
            # Check distances to other problematic pieces
            for other_name, other_num in issues.items():
                if other_num != piece_num and other_num in piece_data:
                    other_centroid = piece_data[other_num]['centroid']
                    distance = np.linalg.norm(centroid - other_centroid)
                    print(f"  Distance to {other_name}: {distance:8.2f}")

def generate_positioning_report():
    """Generate comprehensive positioning analysis report"""
    
    piece_data = analyze_piece_positions()
    intersections = check_intersections(piece_data)
    analyze_specific_issues(piece_data)
    
    # Save detailed report
    with open('visual_output/ASSEMBLY_POSITIONING_ANALYSIS.txt', 'w') as f:
        f.write("SFS ASSEMBLY POSITIONING ANALYSIS REPORT\n")
        f.write("=" * 50 + "\n\n")
        
        f.write("MAJOR ISSUES IDENTIFIED:\n")
        f.write("1. Yellow (Piece 4) and Blue (Piece 3) intersection\n")
        f.write("2. Direct intersections with Purple (Piece 8)\n")
        f.write("3. Cyan (Piece 6) positioning inaccuracy\n\n")
        
        f.write("TOP INTERSECTIONS (by overlap volume):\n")
        for i, intersection in enumerate(intersections[:5]):
            piece_i, piece_j = intersection['pieces']
            colors = intersection['colors']
            vol = intersection['overlap_volume']
            f.write(f"{i+1}. Piece {piece_i} ({colors[0]}) ↔ Piece {piece_j} ({colors[1]}): {vol:.2f} volume overlap\n")
        
        f.write(f"\nRECOMMENDATIONS:\n")
        f.write("1. Review transformation matrix computation for pieces 3, 4, 6, 8\n")
        f.write("2. Check feature matching quality for problematic piece pairs\n")
        f.write("3. Validate ground truth data for comparison\n")
        f.write("4. Consider assembly order optimization\n")
    
    print(f"\n📊 Detailed report saved to: visual_output/ASSEMBLY_POSITIONING_ANALYSIS.txt")

if __name__ == "__main__":
    generate_positioning_report()