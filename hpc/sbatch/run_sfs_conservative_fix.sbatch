#!/bin/bash
#SBATCH --job-name=sfs_conservative_fix
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=results_%Y_%m_%d_%H%M/sfs_conservative_fix_%j.out
#SBATCH --error=results_%Y_%m_%d_%H%M/sfs_conservative_fix_%j.err

echo "=== SFS RECONSTRUCTION WITH CONSERVATIVE CURVATURE-ONLY FIX ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Dataset: NURBS with Fixed Curvature (Conservative Approach)"
echo "Strategy: Maintain valid reconstruction capability while reducing false positives"
echo "Expected: 8/8 pieces assembled with some remaining intersection artifacts to analyze"
echo

# Create timestamped results directory
RESULTS_DIR="results_$(date +%Y_%m_%d_%H%M)"
mkdir -p "$RESULTS_DIR"

cd /data/gpfs/projects/punim2657/sfs_main

# Use the corrected curvature dataset (proven to work)
DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp"

echo "Using conservative curvature-fix dataset: $DATASET_PATH"
echo "This approach maintains reconstruction capability while improving feature discrimination"
echo

echo "Running SFS reconstruction with conservative curvature fixes..."
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "$DATASET_PATH:/Dataset/SfS_pp" \
  sfspreproc.sif /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== CONSERVATIVE FIX RESULTS ==="
echo "End time: $(date)"

# Copy results to timestamped directory for preservation
if [ -d "visual_output" ]; then
    cp -r visual_output/* "$RESULTS_DIR/"
    echo "Results preserved in: $RESULTS_DIR/"
fi

echo "=== NEXT STEPS ==="
echo "If reconstruction successful:"
echo "1. Generate visualization to identify remaining intersection artifacts"
echo "2. Implement targeted spatial filtering for specific problematic piece pairs"
echo "3. Focus on Orange-Blue and Orange-Green distance-based filtering only"