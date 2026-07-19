#!/bin/bash
#SBATCH --job-name=sfs_revert_analyze
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=results_%Y_%m_%d_%H%M/sfs_revert_analyze_%j.out
#SBATCH --error=results_%Y_%m_%d_%H%M/sfs_revert_analyze_%j.err

echo "=== REVERTED SFS RECONSTRUCTION FOR MATCH ANALYSIS ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Goal: Restore working reconstruction and pinpoint specific problematic matches"
echo "Strategy: Use working dataset to identify exact Green-Orange and Blue-Orange matches"
echo

# Create timestamped results directory
RESULTS_DIR="results_$(date +%Y_%m_%d_%H%M)"
mkdir -p "$RESULTS_DIR"

cd /data/gpfs/projects/punim2657/sfs_main

# Use the dataset that previously worked (corrected curvature but not overly aggressive)
DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp"

echo "Using working NURBS dataset: $DATASET_PATH"
echo "Expected: ~238 initial matches → ~38 final matches → 8 pieces assembled"
echo

echo "Running SFS reconstruction with detailed match analysis..."
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "$DATASET_PATH:/Dataset/SfS_pp" \
  sfspreproc.sif /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== MATCH ANALYSIS RESULTS ==="
echo "End time: $(date)"

# Copy results to timestamped directory for preservation
if [ -d "visual_output" ]; then
    cp -r visual_output/* "$RESULTS_DIR/"
    echo "Results preserved in: $RESULTS_DIR/"
fi

echo "=== NEXT: PINPOINT SPECIFIC PROBLEMATIC MATCHES ==="
echo "Look for in the output above:"
echo "1. Pieces 4-3 (Orange-Green): How many matches before/after pruning?"
echo "2. Pieces 4-2 (Orange-Blue): How many matches before/after pruning?"
echo "3. Assembly success: Are all 8 pieces assembled?"
echo "4. If successful, generate visualization to identify intersection locations"