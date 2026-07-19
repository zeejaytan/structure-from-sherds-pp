#!/usr/bin/env python3
"""
Analyze the new PLY file to confirm Blue-Orange intersection
"""

import sys
import numpy as np
from collections import Counter

def analyze_ply_colors(ply_file):
    """Analyze RGB color distribution in PLY file"""
    print(f"=== ANALYZING NEW PLY COLOR DISTRIBUTION ===")
    print(f"File: {ply_file}")
    
    colors = []
    vertices = []
    
    # Read PLY file
    with open(ply_file, 'r') as f:
        # Skip header
        header_done = False
        vertex_count = 0
        
        for line in f:
            line = line.strip()
            
            if line.startswith('element vertex'):
                vertex_count = int(line.split()[-1])
                print(f"Expected vertices: {vertex_count}")
                
            if line == 'end_header':
                header_done = True
                continue
                
            if header_done:
                parts = line.split()
                if len(parts) >= 6:
                    try:
                        x, y, z = float(parts[0]), float(parts[1]), float(parts[2])
                        r, g, b = int(parts[3]), int(parts[4]), int(parts[5])
                        
                        vertices.append([x, y, z])
                        colors.append((r, g, b))
                        
                    except ValueError:
                        continue
    
    print(f"✅ Loaded {len(vertices)} vertices with colors")
    
    # Analyze color distribution
    color_counter = Counter(colors)
    print(f"\n=== COLOR DISTRIBUTION ===")
    
    # Sort by count (most common first)
    sorted_colors = color_counter.most_common()
    
    color_names = {
        (255, 0, 0): "RED",
        (0, 255, 0): "GREEN", 
        (0, 0, 255): "BLUE",
        (255, 165, 0): "ORANGE",
        (255, 127, 0): "ORANGE_ALT",
        (255, 255, 0): "YELLOW",
        (255, 0, 255): "MAGENTA",
        (0, 255, 255): "CYAN",
        (128, 0, 128): "PURPLE",
        (255, 192, 203): "PINK",
        (165, 42, 42): "BROWN",
        (128, 128, 128): "GRAY",
        (0, 0, 0): "BLACK",
        (255, 255, 255): "WHITE"
    }
    
    for i, ((r, g, b), count) in enumerate(sorted_colors):
        percentage = (count / len(vertices)) * 100
        color_name = color_names.get((r, g, b), f"RGB({r},{g},{b})")
        print(f"  {i+1:2d}. {color_name:12s}: {count:6d} vertices ({percentage:5.1f}%)")
        
        if i >= 10:  # Show top 10 colors
            break
    
    # Check for Blue-Orange intersection specifically
    print(f"\n=== BLUE-ORANGE INTERSECTION ANALYSIS ===")
    
    # Convert to numpy arrays for analysis
    vertices = np.array(vertices)
    colors = np.array(colors)
    
    # Find Blue and Orange pieces
    blue_mask = (colors[:, 0] == 0) & (colors[:, 1] == 0) & (colors[:, 2] == 255)
    orange_masks = [
        (colors[:, 0] == 255) & (colors[:, 1] == 165) & (colors[:, 2] == 0),  # Standard orange
        (colors[:, 0] == 255) & (colors[:, 1] == 127) & (colors[:, 2] == 0),  # Alt orange
    ]
    
    blue_vertices = vertices[blue_mask]
    print(f"Blue vertices found: {len(blue_vertices)}")
    
    for i, orange_mask in enumerate(orange_masks):
        orange_vertices = vertices[orange_mask]
        if len(orange_vertices) > 0:
            print(f"Orange{i+1} vertices found: {len(orange_vertices)}")
            
            if len(blue_vertices) > 0 and len(orange_vertices) > 0:
                # Compute bounding boxes
                blue_min, blue_max = np.min(blue_vertices, axis=0), np.max(blue_vertices, axis=0)
                orange_min, orange_max = np.min(orange_vertices, axis=0), np.max(orange_vertices, axis=0)
                
                blue_center = (blue_min + blue_max) / 2
                orange_center = (orange_min + orange_max) / 2
                
                print(f"\nBlue piece:")
                print(f"  Center: [{blue_center[0]:.1f}, {blue_center[1]:.1f}, {blue_center[2]:.1f}]")
                print(f"  Range X: [{blue_min[0]:.1f}, {blue_max[0]:.1f}]")
                print(f"  Range Y: [{blue_min[1]:.1f}, {blue_max[1]:.1f}]") 
                print(f"  Range Z: [{blue_min[2]:.1f}, {blue_max[2]:.1f}]")
                
                print(f"\nOrange{i+1} piece:")
                print(f"  Center: [{orange_center[0]:.1f}, {orange_center[1]:.1f}, {orange_center[2]:.1f}]")
                print(f"  Range X: [{orange_min[0]:.1f}, {orange_max[0]:.1f}]")
                print(f"  Range Y: [{orange_min[1]:.1f}, {orange_max[1]:.1f}]")
                print(f"  Range Z: [{orange_min[2]:.1f}, {orange_max[2]:.1f}]")
                
                # Check intersection
                intersects = True
                for dim in range(3):
                    if blue_max[dim] < orange_min[dim] or orange_max[dim] < blue_min[dim]:
                        intersects = False
                        break
                
                if intersects:
                    # Compute intersection volume
                    intersect_min = np.maximum(blue_min, orange_min)
                    intersect_max = np.minimum(blue_max, orange_max)
                    intersect_size = np.maximum(0, intersect_max - intersect_min)
                    volume = np.prod(intersect_size)
                    
                    print(f"\n🚨 BLUE-ORANGE{i+1} INTERSECTION CONFIRMED!")
                    print(f"   Intersection volume: {volume:.1f}")
                    print(f"   This matches your visual observation!")
                else:
                    center_distance = np.linalg.norm(blue_center - orange_center)
                    print(f"\n✅ Blue-Orange{i+1} pieces are separated")
                    print(f"   Center distance: {center_distance:.1f}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        ply_file = sys.argv[1]
    else:
        ply_file = "ply_output/pot_a_complete_assembly.ply"
    
    analyze_ply_colors(ply_file)