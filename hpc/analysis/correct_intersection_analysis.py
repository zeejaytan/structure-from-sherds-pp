#!/usr/bin/env python3
"""
Correct intersection analysis with proper piece ID mapping:
Green = Piece 2, Orange = Piece 7
"""

import numpy as np
import os

def load_piece_points(piece_file):
    """Load piece point cloud from XYZ file"""
    if not os.path.exists(piece_file):
        print(f"File not found: {piece_file}")
        return None
    
    points = []
    with open(piece_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                coords = line.split()
                if len(coords) >= 3:
                    try:
                        x, y, z = float(coords[0]), float(coords[1]), float(coords[2])
                        points.append([x, y, z])
                    except ValueError:
                        continue
    
    return np.array(points) if points else None

def compute_bounding_box(points):
    """Compute 3D bounding box of points"""
    if points is None or len(points) == 0:
        return None
    
    min_coords = np.min(points, axis=0)
    max_coords = np.max(points, axis=0)
    
    return {
        'min': min_coords,
        'max': max_coords,
        'center': (min_coords + max_coords) / 2,
        'size': max_coords - min_coords
    }

def bounding_boxes_intersect(bbox1, bbox2, tolerance=0.1):
    """Check if two 3D bounding boxes intersect with tolerance"""
    if bbox1 is None or bbox2 is None:
        return False
    
    # Check overlap in each dimension
    for i in range(3):
        if (bbox1['max'][i] + tolerance < bbox2['min'][i] or 
            bbox2['max'][i] + tolerance < bbox1['min'][i]):
            return False
    
    return True

def compute_intersection_volume(bbox1, bbox2):
    """Compute intersection volume between two bounding boxes"""
    if not bounding_boxes_intersect(bbox1, bbox2):
        return 0.0
    
    # Compute intersection bounds
    intersect_min = np.maximum(bbox1['min'], bbox2['min'])
    intersect_max = np.minimum(bbox1['max'], bbox2['max'])
    
    # Compute intersection dimensions
    intersect_size = np.maximum(0, intersect_max - intersect_min)
    
    # Compute volume
    volume = np.prod(intersect_size)
    
    return volume

def analyze_piece_intersection():
    """Analyze GREEN-ORANGE intersection with CORRECT piece mapping"""
    
    # CORRECT PIECE MAPPING:
    # Green = Piece 2, Orange = Piece 7
    green_file = "piece_2_surface.xyz"  # GREEN = Piece 2
    orange_file = "piece_7_surface.xyz"  # ORANGE = Piece 7
    
    print("=== CORRECTED GREEN-ORANGE INTERSECTION ANALYSIS ===")
    print("✅ CORRECT PIECE MAPPING:")
    print(f"   Green = Piece 2 (file: {green_file})")
    print(f"   Orange = Piece 7 (file: {orange_file})")
    
    green_points = load_piece_points(green_file)
    orange_points = load_piece_points(orange_file)
    
    if green_points is None:
        print("❌ Failed to load GREEN (Piece 2) points")
        return
    
    if orange_points is None:
        print("❌ Failed to load ORANGE (Piece 7) points")
        return
    
    print(f"✅ Green points loaded: {len(green_points)} points")
    print(f"✅ Orange points loaded: {len(orange_points)} points")
    
    # Compute bounding boxes
    green_bbox = compute_bounding_box(green_points)
    orange_bbox = compute_bounding_box(orange_points)
    
    print(f"\n=== BOUNDING BOX ANALYSIS ===")
    print(f"GREEN (Piece 2) bounding box:")
    print(f"  Min: [{green_bbox['min'][0]:.3f}, {green_bbox['min'][1]:.3f}, {green_bbox['min'][2]:.3f}]")
    print(f"  Max: [{green_bbox['max'][0]:.3f}, {green_bbox['max'][1]:.3f}, {green_bbox['max'][2]:.3f}]")
    print(f"  Center: [{green_bbox['center'][0]:.3f}, {green_bbox['center'][1]:.3f}, {green_bbox['center'][2]:.3f}]")
    print(f"  Size: [{green_bbox['size'][0]:.3f}, {green_bbox['size'][1]:.3f}, {green_bbox['size'][2]:.3f}]")
    
    print(f"\nORANGE (Piece 7) bounding box:")
    print(f"  Min: [{orange_bbox['min'][0]:.3f}, {orange_bbox['min'][1]:.3f}, {orange_bbox['min'][2]:.3f}]")
    print(f"  Max: [{orange_bbox['max'][0]:.3f}, {orange_bbox['max'][1]:.3f}, {orange_bbox['max'][2]:.3f}]")
    print(f"  Center: [{orange_bbox['center'][0]:.3f}, {orange_bbox['center'][1]:.3f}, {orange_bbox['center'][2]:.3f}]")
    print(f"  Size: [{orange_bbox['size'][0]:.3f}, {orange_bbox['size'][1]:.3f}, {orange_bbox['size'][2]:.3f}]")
    
    # Check intersection
    intersects = bounding_boxes_intersect(green_bbox, orange_bbox, tolerance=0.0)
    intersection_volume = compute_intersection_volume(green_bbox, orange_bbox)
    
    print(f"\n=== INTERSECTION RESULTS ===")
    print(f"Bounding boxes intersect: {'🚨 YES' if intersects else '✅ NO'}")
    print(f"Intersection volume: {intersection_volume:.6f}")
    
    if intersects:
        # Compute center distance
        center_distance = np.linalg.norm(green_bbox['center'] - orange_bbox['center'])
        combined_radius = (np.linalg.norm(green_bbox['size']) + np.linalg.norm(orange_bbox['size'])) / 4
        
        print(f"Center-to-center distance: {center_distance:.3f}")
        print(f"Combined radius estimate: {combined_radius:.3f}")
        print(f"Distance ratio: {center_distance / combined_radius:.3f}")
        
        if intersection_volume > 1.0:  # Significant volume overlap
            print("🚨 SIGNIFICANT INTERSECTION DETECTED")
            print("   This confirms the visual intersection you observed!")
        else:
            print("⚠️  MINOR BOUNDARY OVERLAP DETECTED")
    else:
        center_distance = np.linalg.norm(green_bbox['center'] - orange_bbox['center'])
        print(f"Center-to-center distance: {center_distance:.3f}")
    
    # Compute closest point distance for more precision
    if len(green_points) < 50000 and len(orange_points) < 50000:  # Avoid memory issues
        print(f"\n=== CLOSEST POINT ANALYSIS ===")
        min_distance = float('inf')
        
        # Sample points to avoid memory issues
        green_sample = green_points[::max(1, len(green_points)//1000)]
        orange_sample = orange_points[::max(1, len(orange_points)//1000)]
        
        for g_point in green_sample:
            distances = np.linalg.norm(orange_sample - g_point, axis=1)
            min_dist = np.min(distances)
            if min_dist < min_distance:
                min_distance = min_dist
        
        print(f"Minimum point-to-point distance: {min_distance:.6f}")
        
        if min_distance < 0.1:
            print("🚨 POINTS ARE VERY CLOSE - CONFIRMED INTERSECTION")
            print("   This matches your visual observation!")
        elif min_distance < 1.0:
            print("⚠️  PIECES ARE CLOSE - POSSIBLE INTERSECTION") 
        else:
            print("✅ PIECES ARE SUFFICIENTLY SEPARATED")
    
    # Also check all other piece combinations for completeness
    print(f"\n=== CHECKING ALL PIECE COMBINATIONS ===")
    piece_files = [f"piece_{i}_surface.xyz" for i in range(1, 9)]
    piece_names = ["Red", "Green", "Blue", "Yellow", "Magenta", "Cyan", "Orange", "Purple"]
    
    intersection_count = 0
    for i in range(8):
        for j in range(i+1, 8):
            points_i = load_piece_points(piece_files[i])
            points_j = load_piece_points(piece_files[j])
            
            if points_i is not None and points_j is not None:
                bbox_i = compute_bounding_box(points_i)
                bbox_j = compute_bounding_box(points_j)
                
                if bounding_boxes_intersect(bbox_i, bbox_j, tolerance=0.0):
                    volume = compute_intersection_volume(bbox_i, bbox_j)
                    if volume > 0.1:  # Significant intersection
                        intersection_count += 1
                        print(f"🚨 INTERSECTION: {piece_names[i]} (Piece {i+1}) ↔ {piece_names[j]} (Piece {j+1}) - Volume: {volume:.2f}")
    
    if intersection_count == 0:
        print("✅ No significant intersections found between any pieces")
    else:
        print(f"🚨 Total intersections found: {intersection_count}")

if __name__ == "__main__":
    analyze_piece_intersection()