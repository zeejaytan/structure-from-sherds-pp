#!/bin/bash
#SBATCH --job-name=pottery_validation_full
#SBATCH --time=01:30:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire

# =================================================================
# FULL 8-PIECE POTTERY ASSEMBLY WITH POTTERY VALIDATION
# =================================================================

echo "=== FULL 8-PIECE POTTERY VALIDATION TEST ==="
echo "Testing ground truth connection discovery improvement"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo

# Use fixed NURBS dataset with proper curvature computation
export DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Dataset location: $DATASET_PATH/"

# Enable pottery-aware validation
export ENABLE_POTTERY_VALIDATION=1
echo "🏺 POTTERY VALIDATION: ENABLED"

# Enable debug output for detailed analysis
export POTTERY_DEBUG=1
echo "🔍 POTTERY DEBUG: ENABLED"

# Container setup
export CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

echo
echo "=========================================="
echo "FULL 8-PIECE POTTERY ASSEMBLY TEST"
echo "=========================================="
echo "Expected improvements:"
echo "- Ground truth connection discovery: 33% → 60-80%"
echo "- Connections 3↔5 (88 inliers): Should be ACCEPTED"
echo "- Connections 3↔6 (perfect pottery): Should be ACCEPTED"
echo "- Connection rejection based on pottery structure, not rigid normals"
echo

echo "Running full 8-piece assembly with pottery validation..."
cd /data/gpfs/projects/punim2657/sfs_main

time $CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind "${DATASET_PATH}/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && ./Hierarchy-Clear 8"

echo
echo "=== POTTERY VALIDATION RESULTS ANALYSIS ==="
echo

# Analyze pottery validation activation
echo "🏺 POTTERY VALIDATION ACTIVATION:"
grep "POTTERY VALIDATION.*ENABLED" slurm-${SLURM_JOB_ID}.out | head -3

echo
echo "📊 POTTERY CONNECTION DECISIONS:"
echo "=== Pottery Validation Decisions ==="
grep "POTTERY VALIDATION.*Pieces" slurm-${SLURM_JOB_ID}.out | head -20

echo
echo "📈 CONNECTION INLIER COUNTS:"
echo "=== Inlier Counts per Connection ==="
grep -E "inlier.*[0-9]+|Inlier.*[0-9]+|Final.*inlier" slurm-${SLURM_JOB_ID}.out | head -20

echo
echo "🎯 GROUND TRUTH COMPARISON:"
echo "=== Expected Ground Truth Connections ==="
echo "Expected: 1-2, 1-3, 1-4, 1-5, 1-6, 1-7"
echo "Expected: 2-1, 2-4, 2-5, 2-8"
echo "Expected: 3-1, 3-5, 3-6"
echo "Expected: 4-1, 4-2, 4-6, 4-7, 4-8"
echo "Expected: 5-1, 5-2, 5-3"
echo "Expected: 6-1, 6-3, 6-4, 6-7"
echo "Expected: 7-1, 7-4, 7-6"
echo "Expected: 8-2, 8-4"

echo
echo "🔍 ACTUAL CONNECTIONS FOUND:"
grep -E "Connected|Final.*piece|Assembly.*piece" slurm-${SLURM_JOB_ID}.out | head -15

echo
echo "🧮 PERFORMANCE METRICS:"
echo "Total runtime: $(date)"
echo "Memory usage: Available in job statistics"

echo
echo "🎯 SUCCESS CRITERIA:"
echo "1. ✅ Pottery validation messages appear"
echo "2. 🔍 Ground truth connections 3↔5, 3↔6 discovered"
echo "3. 📈 Overall ground truth recall improvement"
echo "4. 🏺 Assembly maintains pottery vessel structure"

echo
echo "=== FULL POTTERY VALIDATION TEST COMPLETE ==="