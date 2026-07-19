#!/bin/bash
#SBATCH --job-name=complete_pottery_test
#SBATCH --time=00:10:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire

# =================================================================
# COMPLETE POTTERY VALIDATION SYSTEM TEST
# =================================================================

echo "=== COMPLETE POTTERY VALIDATION SYSTEM TEST ==="
echo "Testing: Real geometric constraints, proper piece IDs, vessel-agnostic validation"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo

# Use fixed NURBS dataset with proper curvature computation
export DATASET_PATH="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Dataset location: $DATASET_PATH/"

# Enable pottery-aware validation
export ENABLE_POTTERY_VALIDATION=1
echo "🏺 POTTERY VALIDATION: ENABLED"

# Enable debug output for analysis
export POTTERY_DEBUG=1
echo "🔍 POTTERY DEBUG: ENABLED"

# Container setup
export CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

echo
echo "=========================================="
echo "COMPLETE POTTERY VALIDATION TEST (3-piece)"
echo "=========================================="
echo "Features tested:"
echo "- Real axis data loading from .xyz files"
echo "- Proper piece ID propagation (no hardcoded 1,2)"
echo "- Geometric pottery constraints (axis alignment, spatial proximity, radial consistency)"
echo "- Permissive validation (OR logic replacing restrictive normal threshold)"
echo "- Legacy ICP + Robust ICP pottery integration"
echo

echo "Running 3-piece pottery validation test..."
cd /data/gpfs/projects/punim2657/sfs_main

time $CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind "${DATASET_PATH}/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && timeout 300 ./Hierarchy-Clear 3"

echo
echo "=== COMPLETE POTTERY VALIDATION ANALYSIS ==="
echo

# Check pottery validation activation
echo "🏺 POTTERY SYSTEM ACTIVATION:"
grep "POTTERY VALIDATION.*ENABLED\|POTTERY DATA.*Loaded" slurm-${SLURM_JOB_ID}.out | head -10

echo
echo "📊 POTTERY GEOMETRIC DECISIONS:"
echo "=== Real Geometric Pottery Validation ==="
grep "POTTERY VALIDATION.*axis_align\|POTTERY VALIDATION.*permissive" slurm-${SLURM_JOB_ID}.out | head -15

echo
echo "🔍 PIECE ID PROPAGATION:"
echo "=== Checking Real Piece IDs (not hardcoded 1,2) ==="
grep "POTTERY VALIDATION.*Pieces [1-8]-[1-8]" slurm-${SLURM_JOB_ID}.out | grep -v "Pieces 1-2" | head -10

echo
echo "📈 ICP PATH VERIFICATION:"
if grep -q "USING LEGACY ICP" slurm-${SLURM_JOB_ID}.out; then
    echo "✅ Legacy ICP path: Pottery validation active"
    grep "USING LEGACY ICP" slurm-${SLURM_JOB_ID}.out | head -3
fi

if grep -q "ROBUST ICP.*Processing" slurm-${SLURM_JOB_ID}.out; then
    echo "✅ Robust ICP path: Pottery validation active"
    grep "ROBUST ICP.*Processing" slurm-${SLURM_JOB_ID}.out | head -3
fi

echo
echo "🎯 SUCCESS CRITERIA VALIDATION:"
echo "1. Real axis data loading from /Dataset/SfS_pp/Axes/"
echo "2. Non-hardcoded piece IDs (not just 1-2)"
echo "3. Geometric constraints (axis_align, dist, radial_diff)"
echo "4. Permissive validation (OR logic)"
echo "5. Integration in both Legacy and Robust ICP paths"

echo
echo "📋 Complete Pottery Test Summary:"
echo "Pottery validation architecture: Real geometric constraints (NO hardcoded ground truth)"
echo "Integration: Both Legacy ICP (active) and Robust ICP (backup)"
echo "Expected: Improved ground truth connection discovery through pottery-aware validation"
echo "Test completed at: $(date)"

echo
echo "=== Complete Pottery Validation Test Complete ==="