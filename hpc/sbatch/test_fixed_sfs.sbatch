#!/bin/bash
#SBATCH --job-name=sfs_fixed_test
#SBATCH --partition=compute
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --output=sfs_fixed_test_%j.out
#SBATCH --error=sfs_fixed_test_%j.err

echo "=== TESTING FIXED SFS ALGORITHM ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Node: $SLURMD_NODENAME"

# Load required modules
module load Apptainer/1.3.3-GCC-11.3.0

# Set container path
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

# Change to working directory
cd /data/gpfs/projects/punim2657/sfs_main

# Backup previous results
if [ -d "visual_output" ]; then
    echo "Backing up previous results..."
    mv visual_output visual_output_original_$(date +%Y%m%d_%H%M%S)
fi

# Create fresh output directory
mkdir -p visual_output

echo ""
echo "=== ALGORITHM FIXES APPLIED ==="
echo "1. Outlier rejection: 20mm -> 2mm (10x stricter)"
echo "2. Correspondence requirements: 3 -> 8 points (2.7x more evidence)"
echo "3. Convergence criteria: 1mm -> 0.2mm (5x more precise)"
echo "4. Constraint weights: All increased for better optimization"
echo "5. Loss functions: 5x stricter penalties for misalignment"
echo "6. Optimization tolerance: 1000x tighter (1e-3 -> 1e-6)"
echo ""

echo "=== RUNNING FIXED SFS RECONSTRUCTION ==="
echo "Expected improvements:"
echo "• Blue, Green, Cyan pieces should connect to main assembly"
echo "• Yellow, Magenta, Purple overlaps should be resolved"
echo "• Overall assembly should be geometrically valid"
echo ""

# Run the fixed SFS algorithm
$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif /workspace/sfspreproc-docker/Hierarchy-Clear 8

# Check if reconstruction succeeded
if [ $? -eq 0 ]; then
    echo ""
    echo "=== FIXED SFS RECONSTRUCTION COMPLETED ==="
    echo "End time: $(date)"
    
    # Generate visualization immediately
    cd visual_output
    
    echo ""
    echo "=== CREATING VISUALIZATION FILES ==="
    
    # Create enhanced visualization
    python3 << 'EOF'
import numpy as np
import os

def create_fixed_visualization():
    """Create visualization for the fixed SFS assembly"""
    
    output_dir = "/data/gpfs/projects/punim2657/sfs_main/visual_output"
    
    # Color mapping for pieces
    color_map = {
        1: {'name': 'RED', 'color': [255, 0, 0]},
        2: {'name': 'GREEN', 'color': [0, 255, 0]},
        3: {'name': 'BLUE', 'color': [0, 0, 255]},
        4: {'name': 'YELLOW', 'color': [255, 255, 0]},
        5: {'name': 'MAGENTA', 'color': [255, 0, 255]},
        6: {'name': 'CYAN', 'color': [0, 255, 255]},
        7: {'name': 'ORANGE', 'color': [255, 128, 0]},
        8: {'name': 'PURPLE', 'color': [128, 0, 255]}
    }
    
    # Load and combine all piece surfaces
    all_points = []
    all_colors = []
    piece_info = []
    
    for piece_id in range(1, 9):
        surface_file = f"{output_dir}/piece_{piece_id}_surface.xyz"
        
        if os.path.exists(surface_file):
            try:
                data = np.loadtxt(surface_file)
                if data.shape[1] >= 3:
                    points = data[:, :3]
                    center = np.mean(points, axis=0)
                    
                    color_info = color_map[piece_id]
                    color = color_info['color']
                    
                    all_points.append(points)
                    all_colors.extend([color] * len(points))
                    
                    piece_info.append({
                        'id': piece_id,
                        'name': color_info['name'],
                        'center': center,
                        'points': len(points)
                    })
                    
                    print(f"  {color_info['name']}: {len(points):,} points at ({center[0]:.1f}, {center[1]:.1f}, {center[2]:.1f})")
            except:
                print(f"  Warning: Could not load piece {piece_id}")
    
    if all_points:
        # Combine all points
        combined_points = np.vstack(all_points)
        combined_colors = np.array(all_colors)
        total_points = len(combined_points)
        
        # Create PLY file
        ply_file = f"{output_dir}/sfs_fixed_assembly.ply"
        
        with open(ply_file, 'w') as f:
            f.write("ply\n")
            f.write("format ascii 1.0\n")
            f.write(f"element vertex {total_points}\n")
            f.write("property float x\n")
            f.write("property float y\n")
            f.write("property float z\n")
            f.write("property uchar red\n")
            f.write("property uchar green\n")
            f.write("property uchar blue\n")
            f.write("end_header\n")
            
            for i in range(total_points):
                point = combined_points[i]
                color = combined_colors[i]
                f.write(f"{point[0]:.6f} {point[1]:.6f} {point[2]:.6f} {int(color[0])} {int(color[1])} {int(color[2])}\n")
        
        print(f"\n✅ Fixed assembly visualization: {ply_file}")
        print(f"Total points: {total_points:,}")
        
        # Quick assembly analysis
        print(f"\n=== FIXED ASSEMBLY ANALYSIS ===")
        
        # Calculate pairwise distances between piece centers
        distances = []
        for i, piece1 in enumerate(piece_info):
            for j, piece2 in enumerate(piece_info):
                if i < j:
                    center1 = piece1['center']
                    center2 = piece2['center']
                    dist = np.linalg.norm(center1 - center2)
                    distances.append(dist)
                    
                    if dist < 30:  # Potentially connected pieces
                        print(f"  {piece1['name']} ↔ {piece2['name']}: {dist:.1f}mm")
        
        avg_distance = np.mean(distances)
        print(f"\nAverage piece separation: {avg_distance:.1f}mm")
        
        if avg_distance < 50:
            print("✅ Assembly appears more compact - potential improvement!")
        else:
            print("⚠️  Assembly still widely spaced - may need further tuning")
    
    else:
        print("❌ No surface files found - reconstruction may have failed")

if __name__ == "__main__":
    create_fixed_visualization()
EOF
    
    echo ""
    echo "=== FINAL RESULTS ==="
    ls -la sfs_fixed_assembly.ply 2>/dev/null && echo "✅ Fixed assembly PLY created" || echo "❌ Visualization failed"
    
    echo ""
    echo "=== TO VIEW RESULTS ==="
    echo "meshlab sfs_fixed_assembly.ply"
    echo ""
    echo "Compare with previous assembly to see improvements:"
    echo "• Blue, Green, Cyan should be closer to main assembly"
    echo "• Yellow, Magenta, Purple should have proper separation"
    echo "• Overall assembly should look more realistic"
    
else
    echo ""
    echo "❌ FIXED SFS RECONSTRUCTION FAILED"
    echo "Check error log: sfs_fixed_test_${SLURM_JOB_ID}.err"
    echo "The algorithm fixes may be too restrictive for this dataset"
    echo ""
    echo "Troubleshooting options:"
    echo "1. Revert to original parameters (see documentation)"
    echo "2. Adjust parameters to intermediate values"
    echo "3. Check input data quality"
fi

echo ""
echo "=== JOB COMPLETED ==="
echo "Job ID: $SLURM_JOB_ID"
echo "End time: $(date)"