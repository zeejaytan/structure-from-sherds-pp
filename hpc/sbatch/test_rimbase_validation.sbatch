#!/bin/bash
#SBATCH --job-name=rimbase_validation_test
#SBATCH --time=01:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=rimbase_validation_test_%j.out
#SBATCH --error=rimbase_validation_test_%j.err

echo "=== PRD_RIMBASE.MD PHASE 5: VALIDATION TEST ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing: Single-piece sufficiency detection with adaptive thresholds"
echo "Dataset: Pot A 8-piece NURBS dataset"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "=== FEATURE FLAGS ==="
echo "✓ ENABLE_RIMBASE_VALIDATION=1       (PRD Phase 1-4 implementation)"
echo "✓ ENABLE_AUTO_AGGLOMERATIVE=1       (Global optimization - REQUIRED for hierarchical)"
echo "✓ ENABLE_HIERARCHICAL_ASSEMBLY=1    (Hierarchical assembly - REQUIRED for PRD_RIMBASE)"
echo "✓ ENABLE_STRICT_CLOSURE=1           (Ring closure validation)"
echo "✓ ENABLE_GAP_TARGETING=1            (Progressive gap filling)"
echo "✓ ENABLE_HUB_PREVENTION=1           (Degree limit enforcement)"
echo "✓ ENABLE_TOPOLOGY_BONUSES=1         (Circular topology bonuses)"
echo "✓ ENABLE_POTTERY_VALIDATION=1       (Pottery-aware geometric validation)"
echo

echo "=== DATASET VERIFICATION ==="
echo "Dataset location: /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/"
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"
echo "Axes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Axes -name "*.xyz" 2>/dev/null | wc -l)"
echo

echo "=== EXPECTED RESULTS (PRD ACCEPTANCE CRITERIA) ==="
echo "1. Single-piece sufficiency: Piece 1 (base) should be detected as 80%+ complete"
echo "2. Adaptive thresholds: Gap relaxation for 1-2 piece rim/base (10mm → 50mm)"
echo "3. RMS strict: RMS threshold remains 5mm regardless of data volume"
echo "4. Pre-validation: Sufficiency check BEFORE ring assembly attempt"
echo "5. Performance: <5% runtime increase vs baseline"
echo

echo "=== RUNNING SFS RECONSTRUCTION WITH RIMBASE VALIDATION ==="
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo "Output: Results will include PRD_RIMBASE validation messages"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "export ENABLE_RIMBASE_VALIDATION=1 && \
             export ENABLE_AUTO_AGGLOMERATIVE=1 && \
             export ENABLE_HIERARCHICAL_ASSEMBLY=1 && \
             export ENABLE_STRICT_CLOSURE=1 && \
             export ENABLE_GAP_TARGETING=1 && \
             export ENABLE_HUB_PREVENTION=1 && \
             export ENABLE_TOPOLOGY_BONUSES=1 && \
             export ENABLE_POTTERY_VALIDATION=1 && \
             echo '✓ All feature flags enabled (AUTO_AGGLOMERATIVE + HIERARCHICAL for PRD_RIMBASE)' && \
             /workspace/sfspreproc-docker/Hierarchy-Clear 8"

echo
echo "=== RESULTS ANALYSIS ==="
echo "End time: $(date)"
echo

# Extract key validation messages from output
echo "=== SEARCHING FOR PRD_RIMBASE VALIDATION MESSAGES ==="
echo

# Check for adaptive threshold messages
if grep -q "ADAPTIVE THRESHOLDS" rimbase_validation_test_${SLURM_JOB_ID}.out 2>/dev/null; then
    echo "✓ Adaptive thresholds detected:"
    grep "ADAPTIVE THRESHOLDS" rimbase_validation_test_${SLURM_JOB_ID}.out | head -10
else
    echo "⚠ No adaptive threshold messages found"
fi
echo

# Check for single-piece sufficiency detection
if grep -q "SINGLE-PIECE.*SUFFICIENCY DETECTED" rimbase_validation_test_${SLURM_JOB_ID}.out 2>/dev/null; then
    echo "✓ Single-piece sufficiency detected:"
    grep -A 5 "SINGLE-PIECE.*SUFFICIENCY DETECTED" rimbase_validation_test_${SLURM_JOB_ID}.out | head -20
else
    echo "⚠ No single-piece sufficiency detected (expected for rim, may detect for base)"
fi
echo

# Check for rim/base pre-validation
if grep -q "PRD_RIMBASE.*SUFFICIENCY CHECK" rimbase_validation_test_${SLURM_JOB_ID}.out 2>/dev/null; then
    echo "✓ Pre-validation stages found:"
    grep "PRD_RIMBASE.*SUFFICIENCY CHECK" rimbase_validation_test_${SLURM_JOB_ID}.out
else
    echo "⚠ No pre-validation messages found"
fi
echo

# Assembly results
echo "=== FINAL ASSEMBLY RESULTS ==="
if [ -d "/data/gpfs/projects/punim2657/sfs_main/results_$(date +%Y_%m_%d)*" ]; then
    echo "✓ Timestamped result directories created:"
    ls -la /data/gpfs/projects/punim2657/sfs_main/results_$(date +%Y_%m_%d)* 2>/dev/null | tail -3
else
    echo "⚠ No timestamped result directories found"
fi
echo

echo "=== PRD ACCEPTANCE CRITERIA VALIDATION ==="
echo "Review output above for:"
echo "  1. [ADAPTIVE THRESHOLDS] messages showing gap relaxation"
echo "  2. ✓ SINGLE-PIECE SUFFICIENCY DETECTED messages"
echo "  3. RMS threshold maintenance at 5mm"
echo "  4. Pre-validation BEFORE ring assembly"
echo "  5. Successful assembly completion"
echo

echo "=== PHASE 5 TEST COMPLETE ==="
echo "Full output available in: rimbase_validation_test_${SLURM_JOB_ID}.out"
echo "Errors (if any) in: rimbase_validation_test_${SLURM_JOB_ID}.err"
