#!/bin/bash

echo "=== FIXING SURFACE_F PCD FORMAT ==="
echo "Converting 3-field format (x y z) to 7-field format (x y z normal_x normal_y normal_z curvature)"
echo ""

for piece in {01..08}; do
    surface_f_pcd="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_F.pcd"
    surface_f_xyz="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_F.xyz"
    
    if [[ -f "$surface_f_xyz" && -f "$surface_f_pcd" ]]; then
        echo "Fixing Surface_F format for piece $piece..."
        
        # Backup the incorrectly formatted file
        cp "$surface_f_pcd" "${surface_f_pcd}.backup_wrong_format"
        
        python3 -c "
import numpy as np

# Load the XYZ points
points = np.loadtxt('$surface_f_xyz')

if len(points) > 0:
    # Compute simple normals (outward pointing)
    center = np.mean(points, axis=0)
    normals = np.zeros_like(points)
    
    for i, point in enumerate(points):
        direction = point - center
        if np.linalg.norm(direction) > 0:
            normals[i] = direction / np.linalg.norm(direction)
        else:
            normals[i] = [0, 0, 1]  # Default upward
    
    # Create properly formatted PCD file
    with open('$surface_f_pcd', 'w') as f:
        f.write('# .PCD v0.7 - Point Cloud Data file format\n')
        f.write('VERSION 0.7\n')
        f.write('FIELDS x y z normal_x normal_y normal_z curvature\n')
        f.write('SIZE 4 4 4 4 4 4 4\n')
        f.write('TYPE F F F F F F F\n')
        f.write('COUNT 1 1 1 1 1 1 1\n')
        f.write(f'WIDTH {len(points)}\n')
        f.write('HEIGHT 1\n')
        f.write('VIEWPOINT 0 0 0 1 0 0 0\n')
        f.write(f'POINTS {len(points)}\n')
        f.write('DATA ascii\n')
        
        for point, normal in zip(points, normals):
            curvature = 0  # Default curvature like original
            f.write(f'{point[0]:.6f} {point[1]:.6f} {point[2]:.6f} ')
            f.write(f'{normal[0]:.6f} {normal[1]:.6f} {normal[2]:.6f} {curvature}\n')
    
    print(f'Fixed Surface_F for piece ${piece}: {len(points)} fracture points with 7-field format')
else:
    print(f'No points found in $surface_f_xyz')
" && echo "  ✅ Fixed Surface_F format for piece $piece" || echo "  ❌ Failed to fix Surface_F for piece $piece"
    else
        echo "  ❌ Surface_F files not found for piece $piece"
    fi
done

echo ""
echo "=== VERIFICATION ==="
echo "Checking fixed Surface_F format..."

first_file="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_01_Surface_F.pcd"
if [[ -f "$first_file" ]]; then
    echo "Sample of fixed Surface_F format:"
    head -15 "$first_file"
    echo ""
    echo "Fields check:"
    grep "FIELDS" "$first_file"
    echo "Points count:"
    grep "POINTS" "$first_file"
    echo ""
fi

echo "Fixed Surface_F PCD files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_F.pcd 2>/dev/null | wc -l)"
echo "✅ Surface_F format correction complete!"