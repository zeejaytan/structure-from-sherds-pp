#!/usr/bin/env python3
"""
Analyze the RGB colors in the PLY file to identify piece color mapping
"""

import numpy as np
from collections import Counter

def analyze_ply_colors(ply_file):
    """Analyze RGB color distribution in PLY file"""
    print(f"=== ANALYZING PLY COLOR DISTRIBUTION ===")
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
        
        if i >= 15:  # Show top 15 colors
            break
    
    # Try to identify piece regions by color
    print(f"\n=== PIECE IDENTIFICATION BY COLOR ===")
    
    # Convert to numpy arrays for analysis
    vertices = np.array(vertices)
    colors = np.array(colors)
    
    # Group vertices by unique colors (major colors only)
    major_colors = [color for color, count in sorted_colors if count > len(vertices) * 0.05]  # At least 5%
    
    for i, (r, g, b) in enumerate(major_colors):
        color_mask = (colors[:, 0] == r) & (colors[:, 1] == g) & (colors[:, 2] == b)
        color_vertices = vertices[color_mask]
        
        if len(color_vertices) > 0:
            # Compute bounding box for this color
            min_coords = np.min(color_vertices, axis=0)
            max_coords = np.max(color_vertices, axis=0)
            center = (min_coords + max_coords) / 2
            size = max_coords - min_coords
            
            color_name = color_names.get((r, g, b), f"RGB({r},{g},{b})")
            print(f"\n{color_name} ({len(color_vertices):,} points):")
            print(f"  Center: [{center[0]:.1f}, {center[1]:.1f}, {center[2]:.1f}]")
            print(f"  Size: [{size[0]:.1f}, {size[1]:.1f}, {size[2]:.1f}]")
            print(f"  Range X: [{min_coords[0]:.1f}, {max_coords[0]:.1f}]")
            print(f"  Range Y: [{min_coords[1]:.1f}, {max_coords[1]:.1f}]")
            print(f"  Range Z: [{min_coords[2]:.1f}, {max_coords[2]:.1f}]")
    
    # Check for spatial intersections between colors
    print(f"\n=== SPATIAL INTERSECTIONS BETWEEN COLORS ===")
    
    intersections_found = 0
    for i, color1 in enumerate(major_colors):
        for j, color2 in enumerate(major_colors[i+1:], i+1):
            
            # Get vertices for each color
            mask1 = (colors[:, 0] == color1[0]) & (colors[:, 1] == color1[1]) & (colors[:, 2] == color1[2])
            mask2 = (colors[:, 0] == color2[0]) & (colors[:, 1] == color2[1]) & (colors[:, 2] == color2[2])
            
            verts1 = vertices[mask1]
            verts2 = vertices[mask2]
            
            if len(verts1) > 0 and len(verts2) > 0:
                # Compute bounding boxes
                bbox1_min, bbox1_max = np.min(verts1, axis=0), np.max(verts1, axis=0)
                bbox2_min, bbox2_max = np.min(verts2, axis=0), np.max(verts2, axis=0)
                
                # Check intersection
                intersects = True
                for dim in range(3):
                    if bbox1_max[dim] < bbox2_min[dim] or bbox2_max[dim] < bbox1_min[dim]:
                        intersects = False
                        break
                
                if intersects:
                    # Compute intersection volume
                    intersect_min = np.maximum(bbox1_min, bbox2_min)
                    intersect_max = np.minimum(bbox1_max, bbox2_max)
                    intersect_size = np.maximum(0, intersect_max - intersect_min)
                    volume = np.prod(intersect_size)
                    
                    if volume > 0.1:  # Significant intersection
                        intersections_found += 1
                        name1 = color_names.get(color1, f"RGB{color1}")
                        name2 = color_names.get(color2, f"RGB{color2}")
                        print(f"🚨 INTERSECTION: {name1} ↔ {name2} (Volume: {volume:.1f})")
    
    if intersections_found == 0:
        print("✅ No significant color intersections found")
    else:
        print(f"🚨 Total color intersections: {intersections_found}")

if __name__ == "__main__":
    analyze_ply_colors("pot_a_complete_assembly_fixed.ply")