#!/bin/bash
#SBATCH --job-name=sfs_strict_overlap
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_strict_overlap_%j.out
#SBATCH --error=sfs_strict_overlap_%j.err

echo "=== SFS RECONSTRUCTION WITH STRICT OVERLAP DETECTION ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing stricter overlap thresholds to prevent shard intersections"
echo "Thresholds reduced: 50→5, 50→10, 100→20"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Using STRICT overlap detection binary..."
echo "Binary timestamp: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear 2>/dev/null || echo 'Binary not found')"
echo

echo "Running SFS reconstruction with STRICT overlap detection..."
echo "Expected behavior: More feature matches rejected due to overlap violations"
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== STRICT Overlap Detection Results ==="
echo "Timestamped output directories created:"
ls -la /data/gpfs/projects/punim2657/sfs_main/results_* 2>/dev/null || echo "No timestamped result directories found"

echo
echo "Strict overlap detection SFS reconstruction completed at: $(date)"
echo "🔧 Overlap Thresholds: REDUCED (50→5, 50→10, 100→20)"
echo "🎯 Expected Result: Fewer overlapping sherds, potentially incomplete assembly"
echo "=== Job Finished ==="