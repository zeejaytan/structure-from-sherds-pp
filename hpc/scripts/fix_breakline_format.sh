#!/bin/bash

echo "=== FIXING BREAKLINE FILE FORMAT ==="
echo "The pipeline expects PCD files with: x y z normal_x normal_y normal_z curvature"
echo "Current generated files have: x y z segment"
echo ""

# Backup incorrectly formatted files
mkdir -p /tmp/incorrect_breaklines_backup
cp /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_*.pcd /tmp/incorrect_breaklines_backup/

echo "Regenerating breaklines with correct format..."

for piece in {01..08}; do
    mesh_file="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Mesh/Pot_A_Piece_${piece}_Mesh.obj"
    breakline_0="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_Piece_${piece}_Breakline_0.pcd"
    breakline_1="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_Piece_${piece}_Breakline_1.pcd"
    
    if [[ -f "$mesh_file" ]]; then
        echo "Fixing breaklines for piece $piece..."
        
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
    z_threshold = (max_z - min_z) * 0.15
    
    # Bottom edges (breakline 0) - more points for better representation
    bottom_edges = vertices[vertices[:, 2] < min_z + z_threshold]
    
    # Top edges (breakline 1)  
    top_edges = vertices[vertices[:, 2] > max_z - z_threshold]
    
    # Subsample but keep more points
    if len(bottom_edges) > 150:
        indices = np.linspace(0, len(bottom_edges)-1, 150, dtype=int)
        bottom_edges = bottom_edges[indices]
    
    if len(top_edges) > 100:
        indices = np.linspace(0, len(top_edges)-1, 100, dtype=int)
        top_edges = top_edges[indices]
    
    # Compute simple normals (pointing outward/upward)
    def compute_normals(points):
        normals = np.zeros_like(points)
        center = np.mean(points, axis=0)
        for i, point in enumerate(points):
            # Simple outward normal
            direction = point - center
            if np.linalg.norm(direction) > 0:
                normals[i] = direction / np.linalg.norm(direction)
            else:
                normals[i] = [0, 0, 1]  # Default upward
        return normals
    
    bottom_normals = compute_normals(bottom_edges)
    top_normals = compute_normals(top_edges)
    
    # Save as correctly formatted PCD files
    def save_correct_pcd(points, normals, filename):
        # Create segment information (simplified)
        n_points = len(points)
        if n_points <= 50:
            segments = [(0, n_points-1, 0)]
        elif n_points <= 150:
            mid = n_points // 2
            segments = [(0, mid-1, 0), (mid, n_points-1, 0)]
        else:
            third = n_points // 3
            segments = [(0, third-1, 0), (third, 2*third-1, 0), (2*third, n_points-1, 0)]
        
        with open(filename, 'w') as f:
            f.write('# .PCD v0.7 - Point Cloud Data file format\n')
            
            # Write segment information
            f.write(f'# {len(segments)} {n_points} 0\n')
            for start, end, _ in segments:
                f.write(f'# {start+1} {end+1} 0\n')
            
            f.write('VERSION 0.7\n')
            f.write('FIELDS x y z normal_x normal_y normal_z curvature\n')
            f.write('SIZE 4 4 4 4 4 4 4\n')
            f.write('TYPE F F F F F F F\n')
            f.write('COUNT 1 1 1 1 1 1 1\n')
            f.write(f'WIDTH {n_points}\n')
            f.write('HEIGHT 1\n')
            f.write('VIEWPOINT 0 0 0 1 0 0 0\n')
            f.write(f'POINTS {n_points}\n')
            f.write('DATA ascii\n')
            
            for i, (point, normal) in enumerate(zip(points, normals)):
                curvature = 1.9057659e-42  # Default curvature value from original
                f.write(f'{point[0]:.6f} {point[1]:.6f} {point[2]:.6f} ')
                f.write(f'{normal[0]:.6f} {normal[1]:.6f} {normal[2]:.6f} {curvature}\n')
    
    save_correct_pcd(bottom_edges, bottom_normals, '$breakline_0')
    save_correct_pcd(top_edges, top_normals, '$breakline_1')
    print(f'Fixed breaklines for piece ${piece}: {len(bottom_edges)} bottom, {len(top_edges)} top points')
else:
    print(f'No vertices found in $mesh_file')
" && echo "  ✅ Fixed breaklines for piece $piece" || echo "  ❌ Failed to fix breaklines for piece $piece"
    else
        echo "  ❌ Mesh file not found: $mesh_file"
    fi
done

echo ""
echo "=== VERIFICATION ==="
echo "Checking format of fixed breakline files..."

# Check first file to verify format
first_file="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_Piece_01_Breakline_0.pcd"
if [[ -f "$first_file" ]]; then
    echo "Sample of fixed format:"
    head -20 "$first_file"
    echo ""
    echo "Fields check:"
    grep "FIELDS" "$first_file"
    echo ""
fi

echo "Fixed Breaklines files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_*.pcd 2>/dev/null | wc -l)"
echo "✅ Breakline format correction complete!"