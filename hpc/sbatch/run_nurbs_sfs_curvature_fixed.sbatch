#!/bin/bash
#SBATCH --job-name=nurbs_sfs_curvature_fixed
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=results_%Y_%m_%d_%H%M/sfs_nurbs_curvature_fixed_%j.out
#SBATCH --error=results_%Y_%m_%d_%H%M/sfs_nurbs_curvature_fixed_%j.err

echo "=== NURBS SFS Reconstruction with Fixed Curvature Breaklines ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Dataset: NURBS_Dataset_20250907_ProperCurvature"
echo "Goal: Test elimination of false positive matches between Orange and Green pieces"
echo

# Create timestamped results directory
RESULTS_DIR="results_$(date +%Y_%m_%d_%H%M)"
mkdir -p "$RESULTS_DIR"

cd /data/gpfs/projects/punim2657/sfs_main

# Set dataset path to use the corrected curvature dataset
DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp"

echo "Using corrected curvature dataset: $DATASET_PATH"
echo "Contents check:"
ls -la "$DATASET_PATH/"
ls -la "$DATASET_PATH/Breaklines/" | head -10
echo

echo "Verifying curvature values in Green (03) and Orange (04) pieces:"
echo "Green piece curvature sample:"
tail -5 "$DATASET_PATH/Breaklines/Pot_A_Piece_03_Breakline_0.pcd" | awk '{print "  " $7}'
echo "Orange piece curvature sample:"  
tail -5 "$DATASET_PATH/Breaklines/Pot_A_Piece_04_Breakline_0.pcd" | awk '{print "  " $7}'
echo

echo "Running SFS reconstruction with corrected breaklines..."
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "$DATASET_PATH:/Dataset/SfS_pp" \
  sfspreproc.sif /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== SFS Reconstruction Completed ==="
echo "End time: $(date)"

# Copy results to timestamped directory for preservation
if [ -d "visual_output" ]; then
    cp -r visual_output/* "$RESULTS_DIR/"
    echo "Results preserved in: $RESULTS_DIR/"
fi

echo "=== Analysis Required ==="
echo "Check the output above for feature matching statistics:"
echo "1. Look for match counts between pieces 3 (Green) and 4 (Orange)"
echo "2. Previous corrupted result: 22 false matches between Orange-Green"
echo "3. Expected with proper curvature: Significantly fewer or zero matches"
echo "4. Assembly should show proper spatial separation without intersections"