#!/bin/bash
#SBATCH --job-name=sfs_nurbs_dataset
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_nurbs_dataset_%j.out
#SBATCH --error=sfs_nurbs_dataset_%j.err

echo "=== SFS NURBS DATASET RECONSTRUCTION ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Using NURBS dataset from: /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250903/"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Verifying NURBS dataset structure..."
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250903/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250903/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"
echo "Axes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250903/SfS_pp/Axes -name "*.xyz" | wc -l)"
echo "Meshes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250903/SfS_pp/Mesh -name "*.ply" | wc -l)"
echo

echo "Dataset verification complete. Expected: 16 surfaces, 16 breaklines, 8 axes, 8 meshes"
echo

echo "Using pre-built SFS binary..."
echo "Binary timestamp: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear)"
echo

echo "Running SFS reconstruction with NURBS dataset..."
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250903/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "NURBS SFS reconstruction completed at: $(date)"
echo "=== Job Finished ==="