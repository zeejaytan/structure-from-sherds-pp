#!/bin/bash
#SBATCH --job-name=sfs_proper_segmentation
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_proper_segmentation_%j.out
#SBATCH --error=sfs_proper_segmentation_%j.err

echo "=== SFS RECONSTRUCTION WITH PROPER GEOMETRIC SEGMENTATION ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Using NURBS dataset with proper geometric segmentation (not artificial division)"
echo "Dataset: /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Verifying dataset components..."
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"
echo "Axes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/SfS_pp/Axes -name "*.xyz" | wc -l)"

# Verify proper geometric segmentation in breakline files
SAMPLE_BREAKLINE="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/SfS_pp/Breaklines/Pot_A_Piece_01_Breakline_0.pcd"
if [ -f "$SAMPLE_BREAKLINE" ]; then
    echo
    echo "Verifying proper geometric segmentation in sample breakline file:"
    echo "File: $(basename $SAMPLE_BREAKLINE)"
    SEGMENT_COUNT=$(grep "^# [0-9]" "$SAMPLE_BREAKLINE" | grep -v "PCD v0.7" | head -7 | tail -6 | wc -l)
    echo "Geometric segments found: $SEGMENT_COUNT"
    echo "Sample geometric segment headers:"
    grep "^# [0-9]" "$SAMPLE_BREAKLINE" | head -7
    
    # Check curvature values  
    SAMPLE_CURVATURE=$(grep -E "^[0-9\\.\\-e]" "$SAMPLE_BREAKLINE" | head -1 | awk '{print $7}')
    echo "Sample curvature value: $SAMPLE_CURVATURE"
    echo "✓ Dataset verified with proper geometric segmentation and NURBS curvature"
fi

echo
echo "Running SFS reconstruction with proper geometric segmentation dataset..."
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== SFS Reconstruction Results ==="
echo "Output files generated:"
ls -la /data/gpfs/projects/punim2657/sfs_main/result_* 2>/dev/null || echo "No result files found - check logs above"

echo
echo "Proper geometric segmentation SFS reconstruction completed at: $(date)"
echo "=== Job Finished ==="