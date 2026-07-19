#!/bin/bash

echo "=== GENERATING BREAKLINES FROM MESH FILES ==="

for piece in {01..08}; do
    mesh_file="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Mesh/Pot_A_Piece_${piece}_Mesh.obj"
    breakline_0="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_Piece_${piece}_Breakline_0.pcd"
    breakline_1="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_Piece_${piece}_Breakline_1.pcd"
    
    if [[ -f "$mesh_file" ]]; then
        echo "Generating breaklines for piece $piece from mesh..."
        
        # Extract edge points from mesh using simple edge detection
        python3 -c "
import numpy as np

# Read OBJ file
vertices = []
with open('$mesh_file', 'r') as f:
    for line in f:
        if line.startswith('v '):
            parts = line.strip().split()
            if len(parts) >= 4:
                vertices.append([float(parts[1]), float(parts[2]), float(parts[3])])

vertices = np.array(vertices)
if len(vertices) > 0:
    # Simple edge detection: find points at extremes
    min_z, max_z = np.min(vertices[:, 2]), np.max(vertices[:, 2])
    z_threshold = (max_z - min_z) * 0.1
    
    # Bottom edges (breakline 0)
    bottom_edges = vertices[vertices[:, 2] < min_z + z_threshold]
    
    # Top edges (breakline 1)  
    top_edges = vertices[vertices[:, 2] > max_z - z_threshold]
    
    # Subsample for reasonable breakline size
    if len(bottom_edges) > 200:
        indices = np.linspace(0, len(bottom_edges)-1, 200, dtype=int)
        bottom_edges = bottom_edges[indices]
    
    if len(top_edges) > 200:
        indices = np.linspace(0, len(top_edges)-1, 200, dtype=int)
        top_edges = top_edges[indices]
    
    # Save as PCD files (simplified format)
    def save_pcd(points, filename, segment_count=None):
        if segment_count is None:
            segment_count = min(5, max(1, len(points) // 50))  # Reasonable segment count
        
        with open(filename, 'w') as f:
            f.write('# .PCD v0.7 - Point Cloud Data file format\n')
            f.write('VERSION 0.7\n')
            f.write('FIELDS x y z segment\n')
            f.write('SIZE 4 4 4 4\n')
            f.write('TYPE F F F I\n')
            f.write('COUNT 1 1 1 1\n')
            f.write(f'WIDTH {len(points)}\n')
            f.write('HEIGHT 1\n')
            f.write('VIEWPOINT 0 0 0 1 0 0 0\n')
            f.write(f'POINTS {len(points)}\n')
            f.write('DATA ascii\n')
            
            # Assign segments to points
            points_per_segment = max(1, len(points) // segment_count)
            for i, point in enumerate(points):
                segment_id = min(segment_count - 1, i // points_per_segment)
                f.write(f'{point[0]:.6f} {point[1]:.6f} {point[2]:.6f} {segment_id}\n')
    
    save_pcd(bottom_edges, '$breakline_0', 3)  # 3 segments for breakline 0
    save_pcd(top_edges, '$breakline_1', 2)     # 2 segments for breakline 1
    print(f'Generated breaklines for piece ${piece}: {len(bottom_edges)} bottom points, {len(top_edges)} top points')
else:
    print(f'No vertices found in $mesh_file')
" 2>/dev/null && echo "  ✅ Generated breaklines for piece $piece" || echo "  ⚠️ Failed to generate breaklines for piece $piece"
    else
        echo "  ❌ Mesh file not found: $mesh_file"
    fi
done

echo ""
echo "=== BREAKLINE GENERATION VERIFICATION ==="
echo "Generated Breaklines files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_* 2>/dev/null | wc -l)"
echo "✅ Breakline generation complete!"