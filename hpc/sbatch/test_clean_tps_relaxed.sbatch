#!/bin/bash
#SBATCH --job-name=sfs_clean_tps_relaxed
#SBATCH --time=01:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_clean_tps_relaxed_%j.out
#SBATCH --error=sfs_clean_tps_relaxed_%j.err

echo "=== SFS CLEAN TPS DATASET WITH RELAXED SETTINGS ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing clean TPS dataset from: /data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Verifying clean TPS dataset structure..."
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"
echo "Axes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp/Axes -name "*.xyz" | wc -l)"
echo "Surface_F: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp/Surfaces -name "*_Surface_F.pcd" | wc -l)"
echo

echo "Using pre-built SFS binary with segfault fixes..."
echo "Binary timestamp: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear)"

echo "Running SFS with clean TPS dataset (directly from preprocessing directory)..."
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "Clean TPS dataset test completed at: $(date)"
echo "=== Job Finished ==="