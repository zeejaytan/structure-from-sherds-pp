#!/bin/bash
#SBATCH --job-name=pottery_integration_test
#SBATCH --time=00:05:00
#SBATCH --mem=4G
#SBATCH --cpus-per-task=2
#SBATCH --partition=sapphire

# =================================================================
# POTTERY VALIDATION INTEGRATION TEST (LEGACY ICP PATH)
# =================================================================

echo "=== POTTERY VALIDATION INTEGRATION TEST ==="
echo "Testing pottery validation in Legacy ICP path"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo

# Use fixed NURBS dataset
export DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Dataset location: $DATASET_PATH/"

# Enable pottery-aware validation in Legacy ICP
export ENABLE_POTTERY_VALIDATION=1
echo "🏺 POTTERY VALIDATION: ENABLED (Legacy ICP path)"

# Container setup
export CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

echo
echo "Running Legacy ICP pottery validation test..."
echo "Command: Hierarchy-Clear 3 (3-piece test for quick validation)"
echo "Expected: *** POTTERY VALIDATION *** ENABLED messages"
echo "Expected: *** POTTERY VALIDATION *** Pieces X-Y ACCEPTED/REJECTED messages"
echo

# Run 3-piece test to verify pottery validation in Legacy ICP
cd /data/gpfs/projects/punim2657/sfs_main

time $CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind "${DATASET_PATH}/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && timeout 180 ./Hierarchy-Clear 3"

echo
echo "=== POTTERY INTEGRATION TEST RESULTS ==="
echo

# Check if pottery validation messages appeared
if grep -q "POTTERY VALIDATION.*ENABLED\|POTTERY VALIDATION.*Pieces" slurm-${SLURM_JOB_ID}.out; then
    echo "✅ SUCCESS: Pottery validation activated in Legacy ICP path!"
    echo
    echo "🎯 Pottery validation activation messages:"
    grep "POTTERY VALIDATION.*ENABLED" slurm-${SLURM_JOB_ID}.out | head -5

    echo
    echo "📊 Pottery connection validation examples:"
    grep "POTTERY VALIDATION.*Pieces" slurm-${SLURM_JOB_ID}.out | head -10

    echo
    echo "🔍 Legacy ICP confirmation:"
    grep "USING LEGACY ICP" slurm-${SLURM_JOB_ID}.out | head -3

else
    echo "❌ FAILURE: Pottery validation not activated in Legacy ICP"
    echo "Expected: *** POTTERY VALIDATION *** ENABLED via environment variable"
    echo "Expected: *** POTTERY VALIDATION *** Pieces X-Y ACCEPTED/REJECTED"

    echo
    echo "🔍 Debugging - what ICP path was used:"
    grep "USING LEGACY ICP\|ROBUST ICP" slurm-${SLURM_JOB_ID}.out | head -5
fi

echo
echo "📋 Integration Test Summary:"
echo "Pottery validation integrated into: Legacy ICP CountInlier function"
echo "Location: feature_matching.cpp:1183-1200 (angle threshold replacement)"
echo "Activation: ENABLE_POTTERY_VALIDATION=1 environment variable"
echo "Test completed at: $(date)"

echo
echo "🎯 Next step: If successful, run full 8-piece assembly test"
echo "=== Integration Test Complete ==="