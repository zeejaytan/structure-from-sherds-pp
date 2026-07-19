#!/bin/bash

echo "=== GENERATING SURFACE_F WITH OPTIMIZED TPS NORMALS ==="
echo "Using subsampled TPS computation for efficiency"
echo ""

for piece in {01..08}; do
    surface_0="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_0.xyz"
    surface_f_pcd="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_F.pcd"
    surface_f_xyz="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_F.xyz"
    
    echo "Processing Piece $piece (optimized)..."
    
    if [[ -f "$surface_0" ]]; then
        # Create subsampled surface file for faster TPS fitting
        temp_surface="/tmp/surface_${piece}_subsampled.xyz"
        
        # Subsample every 10th point for TPS fitting (840 points instead of 8400)
        python3 -c "
import numpy as np
surface = np.loadtxt('$surface_0')
subsampled = surface[::10]  # Every 10th point
np.savetxt('$temp_surface', subsampled, fmt='%.6f')
print(f'Subsampled from {len(surface)} to {len(subsampled)} points for TPS fitting')
" 2>/dev/null
        
        echo "  Computing TPS normals from subsampled surface..."
        
        # Use timeout to prevent hanging
        timeout 60 ./compute_tps_normals "$temp_surface" "$surface_f_pcd" "$surface_f_xyz"
        
        if [[ $? -eq 0 ]]; then
            echo "  ✅ Generated Surface_F with TPS normals for piece $piece"
        else
            echo "  ⚠️ TPS computation failed/timeout, using simplified approach..."
            
            # Fallback: Use the existing approach but with proper 7-field format
            python3 -c "
import numpy as np

# Load surface points
points = np.loadtxt('$surface_0')

# Subsample to reasonable size for Surface_F (fracture surface)
if len(points) > 2000:
    indices = np.linspace(0, len(points)-1, 2000, dtype=int)
    points = points[indices]

# Compute simple normals (better than random but not full TPS)
center = np.mean(points, axis=0)
normals = np.zeros_like(points)

for i, point in enumerate(points):
    direction = point - center
    if np.linalg.norm(direction) > 0:
        normals[i] = direction / np.linalg.norm(direction)
    else:
        normals[i] = [0, 0, 1]  # Default upward

# Save XYZ
np.savetxt('$surface_f_xyz', points, fmt='%.6f')

# Save PCD with 7-field format
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
        curvature = 0.0
        f.write(f'{point[0]:.6f} {point[1]:.6f} {point[2]:.6f} ')
        f.write(f'{normal[0]:.6f} {normal[1]:.6f} {normal[2]:.6f} {curvature}\n')

print(f'Generated Surface_F for piece ${piece} with {len(points)} points (fallback method)')
" 2>/dev/null && echo "  ✅ Generated Surface_F with fallback normals for piece $piece" || echo "  ❌ Fallback failed for piece $piece"
        fi
        
        # Clean up temp file
        rm -f "$temp_surface"
    else
        echo "  ❌ Surface_0 not found for piece $piece"
    fi
    
    echo ""
done

echo "=== VERIFICATION ==="
first_file="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_01_Surface_F.pcd"
if [[ -f "$first_file" ]]; then
    echo "Surface_F format check:"
    grep "FIELDS" "$first_file"
    grep "POINTS" "$first_file" | grep -v "WIDTH"
    echo "Sample data:"
    tail -n +12 "$first_file" | head -2
    echo ""
fi

echo "Generated Surface_F files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_F* 2>/dev/null | wc -l)"
echo "✅ Optimized Surface_F generation complete!"