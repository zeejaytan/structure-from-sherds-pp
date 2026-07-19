#!/bin/bash
#SBATCH --job-name=sfs_proper_curvature
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_proper_curvature_%j.out
#SBATCH --error=sfs_proper_curvature_%j.err

echo "=== SFS RECONSTRUCTION WITH PROPER CURVATURE DATASET ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Using NURBS dataset with proper curvature values (1.9e-42)"
echo "Dataset location: /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250904_ProperCurvature/"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Verifying proper curvature dataset structure..."
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250904_ProperCurvature/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250904_ProperCurvature/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"

# Sample curvature verification
SAMPLE_BREAKLINE=$(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250904_ProperCurvature/SfS_pp/Breaklines -name "*_Breakline_0.pcd" | head -1)
if [ ! -z "$SAMPLE_BREAKLINE" ]; then
    echo "Verifying curvature values in: $(basename $SAMPLE_BREAKLINE)"
    SAMPLE_CURVATURE=$(grep -E "^[0-9\.\-e]" "$SAMPLE_BREAKLINE" | head -1 | awk '{print $7}')
    echo "Sample curvature value: $SAMPLE_CURVATURE"
    if [[ $SAMPLE_CURVATURE == "1.9057659e-42" ]]; then
        echo "✓ Curvature values match sample data exactly"
    else
        echo "⚠ Curvature values differ from sample data"
    fi
fi

echo
echo "Running SFS reconstruction with proper curvature dataset..."
echo "Bind mount: Proper curvature dataset → /Dataset/SfS_pp"
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250904_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== SFS Reconstruction Results ==="
echo "Output files generated:"
ls -la /data/gpfs/projects/punim2657/sfs_main/result_* 2>/dev/null || echo "No result files found - check logs above"

echo
echo "Proper curvature SFS reconstruction completed at: $(date)"
echo "=== Job Finished ==="