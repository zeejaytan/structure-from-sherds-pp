#!/bin/bash
#SBATCH --job-name=pottery_validation_test
#SBATCH --time=00:15:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire

# =================================================================
# POTTERY-AWARE GEOMETRIC VALIDATION TEST
# =================================================================

echo "=== POTTERY-AWARE GEOMETRIC VALIDATION TEST ==="
echo "Testing pottery validation against ground truth connections"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo

# Use fixed NURBS dataset with proper curvature computation
export DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Dataset location: $DATASET_PATH/"

# Enable pottery-aware validation (this is our test)
export ENABLE_POTTERY_VALIDATION=1
echo "🏺 POTTERY VALIDATION: ENABLED (Testing new geometric validation)"

# Enable comprehensive debug output to compare with baseline
export POTTERY_DEBUG=1
echo "🔍 POTTERY DEBUG: ENABLED"

# Container setup
export CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

# Create timestamped output directory
TIMESTAMP=$(date +"%Y_%m_%d_%H%M")
OUTPUT_DIR="pottery_test_results_${TIMESTAMP}"
echo "📁 Results will be saved in: ${OUTPUT_DIR}/"

echo
echo "Running pottery-aware SFS reconstruction..."
echo "Container: sfspreproc.sif"
echo "Binary: Hierarchy-Clear (with pottery validation)"
echo "Command: Hierarchy-Clear 8 (8-piece assembly)"
echo

# Run SFS reconstruction with pottery validation
cd /data/gpfs/projects/punim2657/sfs_main

time $CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind "${DATASET_PATH}/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && ./Hierarchy-Clear 8"

echo
echo "=== POTTERY VALIDATION TEST RESULTS ==="
echo

# Analyze the results
echo "📊 Connection Analysis:"
echo "Looking for pottery validation debug output..."

# Check if pottery validation messages appeared
if grep -q "POTTERY VALIDATION" slurm-${SLURM_JOB_ID}.out; then
    echo "✅ Pottery validation system activated"

    # Extract pottery validation results
    echo
    echo "🏺 Pottery Validation Results:"
    grep "POTTERY VALIDATION\|POTTERY SUCCESS\|POTTERY REJECTION" slurm-${SLURM_JOB_ID}.out

else
    echo "⚠️  Pottery validation debug output not found"
    echo "   This suggests pottery validation may not be fully integrated yet"
    echo "   Continuing with standard connection analysis..."
fi

echo
echo "🔗 Connection Results:"
grep "LEGACY-STYLE CONNECTION\|HYBRID SUCCESS\|PUZZLEFUSION" slurm-${SLURM_JOB_ID}.out | head -10

echo
echo "📈 Assembly Summary:"
grep "pieces assembled\|final.*result\|Global optimization result" slurm-${SLURM_JOB_ID}.out

echo
echo "🎯 Ground Truth Connection Test:"
echo "Checking for specific ground truth connections that should be found..."

# Check for the specific connections we identified as problems
echo
echo "Connection 3↔5 (should have ~88 inliers):"
grep "Pieces.*3.*5\|Pieces.*5.*3" slurm-${SLURM_JOB_ID}.out | grep "inlier" | head -3

echo
echo "Connection 3↔6 (should have perfect pottery geometry):"
grep "Pieces.*3.*6\|Pieces.*6.*3" slurm-${SLURM_JOB_ID}.out | grep "inlier" | head -3

echo
echo "Connection 5↔6 (non-GT, should compete with GT connections):"
grep "Pieces.*5.*6\|Pieces.*6.*5" slurm-${SLURM_JOB_ID}.out | grep "inlier" | head -3

echo
echo "📋 Test Completion Summary:"
echo "Pottery validation test completed at: $(date)"
echo "Output files location: Current directory"
echo "Log file: slurm-${SLURM_JOB_ID}.out"

echo
echo "🔬 Next Steps for Analysis:"
echo "1. Compare connection patterns with previous baseline (33% GT recall)"
echo "2. Check if pottery-valid connections were accepted"
echo "3. Verify ground truth connections 3↔5, 3↔6 were found"
echo "4. Analyze pottery geometric validation scores"

echo
echo "=== Test Script Complete ==="