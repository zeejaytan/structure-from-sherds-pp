#!/usr/bin/env python3

import numpy as np
import os
import sys

def load_surface_data(filename):
    """Load surface data from XYZ file"""
    try:
        data = np.loadtxt(filename)
        if data.shape[1] >= 6:
            points = data[:, :3]  # X, Y, Z coordinates
            normals = data[:, 3:6]  # Normal vectors if available
            return points, normals
        else:
            points = data[:, :3]
            return points, None
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None, None

def create_high_contrast_colors():
    """Create high contrast color palette for 8 pieces"""
    colors = [
        [1.0, 0.0, 0.0],    # Bright Red - Piece 1
        [0.0, 1.0, 0.0],    # Bright Green - Piece 2  
        [0.0, 0.0, 1.0],    # Bright Blue - Piece 3
        [1.0, 1.0, 0.0],    # Bright Yellow - Piece 4
        [1.0, 0.0, 1.0],    # Bright Magenta - Piece 5
        [0.0, 1.0, 1.0],    # Bright Cyan - Piece 6
        [1.0, 0.5, 0.0],    # Bright Orange - Piece 7
        [0.5, 0.0, 1.0],    # Bright Purple - Piece 8
    ]
    return colors

def create_high_contrast_ply():
    """Create high contrast PLY visualization"""
    
    print("Creating high contrast SFS reconstruction visualization...")
    
    # High contrast colors for maximum piece distinction
    colors = create_high_contrast_colors()
    
    output_file = "visual_output/sfs_high_contrast_assembly.ply"
    
    all_vertices = []
    all_colors = []
    vertex_count = 0
    
    # Load surface data for each piece
    for piece_num in range(1, 9):
        surface_file = f"visual_output/piece_{piece_num}_surface.xyz"
        
        if os.path.exists(surface_file):
            points, normals = load_surface_data(surface_file)
            
            if points is not None:
                num_points = len(points)
                print(f"Piece {piece_num}: {num_points:,} points")
                
                # Add vertices
                all_vertices.extend(points)
                
                # Add high contrast colors for this piece
                piece_color = colors[piece_num - 1]
                piece_colors = [piece_color] * num_points
                all_colors.extend(piece_colors)
                
                vertex_count += num_points
            else:
                print(f"Warning: Could not load piece {piece_num}")
        else:
            print(f"Warning: Surface file for piece {piece_num} not found")
    
    if vertex_count == 0:
        print("Error: No surface data found!")
        return
    
    print(f"\nTotal vertices: {vertex_count:,}")
    print(f"Creating high contrast PLY file: {output_file}")
    
    # Write PLY file with high contrast colors
    with open(output_file, 'w') as f:
        # PLY header
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {vertex_count}\n")
        f.write("property float x\n")
        f.write("property float y\n") 
        f.write("property float z\n")
        f.write("property uchar red\n")
        f.write("property uchar green\n")
        f.write("property uchar blue\n")
        f.write("end_header\n")
        
        # Write vertex data with colors
        for i in range(vertex_count):
            vertex = all_vertices[i]
            color = all_colors[i]
            
            # Convert color from [0,1] to [0,255]
            r = int(color[0] * 255)
            g = int(color[1] * 255) 
            b = int(color[2] * 255)
            
            f.write(f"{vertex[0]:.6f} {vertex[1]:.6f} {vertex[2]:.6f} {r} {g} {b}\n")
    
    print(f"✅ High contrast visualization created: {output_file}")
    print(f"File size: {os.path.getsize(output_file) / 1024 / 1024:.1f} MB")
    
    # Create color legend
    legend_file = "visual_output/HIGH_CONTRAST_COLOR_LEGEND.txt"
    with open(legend_file, 'w') as f:
        f.write("HIGH CONTRAST SFS RECONSTRUCTION - COLOR LEGEND\n")
        f.write("=" * 50 + "\n\n")
        
        color_names = ["Red", "Green", "Blue", "Yellow", "Magenta", "Cyan", "Orange", "Purple"]
        
        for i in range(8):
            rgb = colors[i]
            rgb_255 = [int(c * 255) for c in rgb]
            f.write(f"Piece {i+1}: {color_names[i]:<8} RGB({rgb_255[0]:3}, {rgb_255[1]:3}, {rgb_255[2]:3})\n")
        
        f.write(f"\nTotal pieces visualized: 8\n")
        f.write(f"Total surface points: {vertex_count:,}\n")
        f.write(f"Visualization optimized for maximum piece contrast\n")
    
    print(f"✅ Color legend created: {legend_file}")

if __name__ == "__main__":
    create_high_contrast_ply()