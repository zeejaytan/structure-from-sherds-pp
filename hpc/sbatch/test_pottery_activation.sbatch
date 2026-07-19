#!/bin/bash
#SBATCH --job-name=pottery_activation_test
#SBATCH --time=00:10:00
#SBATCH --mem=4G
#SBATCH --cpus-per-task=2
#SBATCH --partition=sapphire

# =================================================================
# POTTERY VALIDATION ACTIVATION TEST
# =================================================================

echo "=== POTTERY VALIDATION ACTIVATION TEST ==="
echo "Testing pottery validation hooks are properly activated"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo

# Use fixed NURBS dataset with proper curvature computation
export DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Dataset location: $DATASET_PATH/"

# Enable pottery-aware validation
export ENABLE_POTTERY_VALIDATION=1
echo "🏺 POTTERY VALIDATION: ENABLED"

# Enable debug output
export POTTERY_DEBUG=1
echo "🔍 POTTERY DEBUG: ENABLED"

# Container setup
export CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

echo
echo "Running pottery validation activation test..."
echo "Command: Hierarchy-Clear 3 (3-piece test for faster execution)"
echo "Expected: Pottery validation debug messages should appear"
echo

# Run quick 3-piece test to verify pottery validation activation
cd /data/gpfs/projects/punim2657/sfs_main

time $CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind "${DATASET_PATH}/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && timeout 300 ./Hierarchy-Clear 3"

echo
echo "=== POTTERY ACTIVATION TEST RESULTS ==="
echo

# Check if pottery validation messages appeared
if grep -q "POTTERY VALIDATION.*ENABLED\|POTTERY VALIDATION.*Pieces" slurm-${SLURM_JOB_ID}.out; then
    echo "✅ SUCCESS: Pottery validation system activated!"
    echo
    echo "🎯 Pottery validation messages found:"
    grep "POTTERY VALIDATION" slurm-${SLURM_JOB_ID}.out | head -10

    echo
    echo "📊 Connection validation examples:"
    grep "pottery_valid\|ACCEPTED\|REJECTED" slurm-${SLURM_JOB_ID}.out | head -5

else
    echo "❌ FAILURE: Pottery validation not activated"
    echo "Expected to see: *** POTTERY VALIDATION *** ENABLED via environment variable"
    echo "And: *** POTTERY VALIDATION *** Pieces X-Y"
fi

echo
echo "🔍 Robust ICP activation check:"
if grep -q "ROBUST ICP" slurm-${SLURM_JOB_ID}.out; then
    echo "✅ Robust ICP functions are being called"
    grep "ROBUST ICP" slurm-${SLURM_JOB_ID}.out | head -5
else
    echo "⚠️  Robust ICP functions may not be reached in this test"
fi

echo
echo "📋 Test Summary:"
echo "Pottery activation test completed at: $(date)"
echo "Purpose: Verify pottery validation hooks replace normal threshold filtering"
echo "Next: If successful, run full 8-piece test with pottery validation"

echo
echo "=== Test Complete ==="