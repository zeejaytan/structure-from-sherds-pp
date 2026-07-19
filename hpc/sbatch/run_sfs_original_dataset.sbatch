#!/bin/bash
#SBATCH --job-name=sfs_original_test
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=results_%Y_%m_%d_%H%M/sfs_original_test_%j.out
#SBATCH --error=results_%Y_%m_%d_%H%M/sfs_original_test_%j.err

echo "=== SFS RECONSTRUCTION WITH ORIGINAL WORKING DATASET ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Dataset: Original NURBS sample dataset (proven to work)"
echo "Goal: Confirm we can restore working reconstruction, then identify problematic matches"
echo

# Create timestamped results directory
RESULTS_DIR="results_$(date +%Y_%m_%d_%H%M)"
mkdir -p "$RESULTS_DIR"

cd /data/gpfs/projects/punim2657/sfs_main

# Use the original working dataset
DATASET_PATH="/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp"

echo "Using original working dataset: $DATASET_PATH"
echo "Contents check:"
ls -la "$DATASET_PATH/Surfaces/" | head -5
ls -la "$DATASET_PATH/Breaklines/" | head -5 
ls -la "$DATASET_PATH/Axes/" | head -5
echo

echo "Running SFS reconstruction with original dataset..."
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "$DATASET_PATH:/Dataset/SfS_pp" \
  sfspreproc.sif /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== ORIGINAL DATASET TEST RESULTS ==="
echo "End time: $(date)"

# Copy results to timestamped directory for preservation
if [ -d "visual_output" ]; then
    cp -r visual_output/* "$RESULTS_DIR/"
    echo "Results preserved in: $RESULTS_DIR/"
fi

echo "=== ANALYSIS ==="
echo "Expected with original dataset: ~200+ matches → successful assembly"
echo "If successful: We can identify the exact Orange-Green and Blue-Orange matches"
echo "If failed: Dataset/algorithm compatibility issue needs investigation"