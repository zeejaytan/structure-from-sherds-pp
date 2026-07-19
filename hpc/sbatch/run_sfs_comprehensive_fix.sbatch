#!/bin/bash
#SBATCH --job-name=sfs_comprehensive_fix
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=results_%Y_%m_%d_%H%M/sfs_comprehensive_fix_%j.out
#SBATCH --error=results_%Y_%m_%d_%H%M/sfs_comprehensive_fix_%j.err

echo "=== SFS RECONSTRUCTION WITH COMPREHENSIVE FEATURE MATCHING FIX ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Dataset: NURBS with Fixed Curvature + Comprehensive Feature Matching Fixes"
echo "Fixes Applied:"
echo "  1. Fixed curvature computation in breaklines (NURBS preprocessing)"
echo "  2. Fixed quantization bug in feature matching (j==3 accessing a(6,i))"
echo "  3. Enhanced thickness discrimination to prevent false matches"
echo "  4. Improved invalid thickness handling with unique values per piece"
echo "Goal: Eliminate ALL false positive matches (Orange-Blue, Orange-Green, Blue-Green)"
echo

# Create timestamped results directory
RESULTS_DIR="results_$(date +%Y_%m_%d_%H%M)"
mkdir -p "$RESULTS_DIR"

cd /data/gpfs/projects/punim2657/sfs_main

# Use the corrected curvature dataset
DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp"

echo "Using comprehensive-fix dataset: $DATASET_PATH"
echo "Verifying dataset integrity:"
ls -la "$DATASET_PATH/Surfaces/" | wc -l
ls -la "$DATASET_PATH/Breaklines/" | wc -l
ls -la "$DATASET_PATH/Axes/" | wc -l
echo

echo "Sample of fixed curvature values (should be geometric, not corrupted):"
echo "Green (Piece 3) sample:"
tail -3 "$DATASET_PATH/Breaklines/Pot_A_Piece_03_Breakline_0.pcd" | awk '{print "  Curvature: " $7}'
echo "Orange (Piece 4) sample:"  
tail -3 "$DATASET_PATH/Breaklines/Pot_A_Piece_04_Breakline_0.pcd" | awk '{print "  Curvature: " $7}'
echo "Blue (Piece 2) sample:"
tail -3 "$DATASET_PATH/Breaklines/Pot_A_Piece_02_Breakline_0.pcd" | awk '{print "  Curvature: " $7}'
echo

echo "Running SFS reconstruction with comprehensive feature matching fixes..."
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "$DATASET_PATH:/Dataset/SfS_pp" \
  sfspreproc.sif /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "=== COMPREHENSIVE FIX RESULTS ==="
echo "End time: $(date)"
echo "Expected improvements:"
echo "1. Orange-Blue matches: Should be 0-2 (was 10)"
echo "2. Orange-Green matches: Should be 0-2 (was 21)" 
echo "3. Overall feature matches: Significantly reduced"
echo "4. Pairwise pruning: More effective due to better feature discrimination"
echo "5. Assembly quality: No intersection artifacts"

# Copy results to timestamped directory for preservation
if [ -d "visual_output" ]; then
    cp -r visual_output/* "$RESULTS_DIR/"
    echo "Results preserved in: $RESULTS_DIR/"
fi

echo "=== ANALYSIS INSTRUCTIONS ==="
echo "Look for feature matching statistics in the output above:"
echo "- Initial matches before pruning"
echo "- Pairwise pruning effectiveness" 
echo "- Final assembly quality and piece count"
echo "- Specific Orange-Blue and Orange-Green match reduction"