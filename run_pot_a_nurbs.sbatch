#!/bin/bash
#SBATCH --job-name=pot_a_nurbs
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=pot_a_nurbs_%j.out
#SBATCH --error=pot_a_nurbs_%j.err

echo "========================================================================="
echo "      SFS++ LEGACY - POT A (NURBS DATASET COMPARISON)"
echo "========================================================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

echo "=== Configuration ==="
echo "Dataset: Pot A (8 pieces) - NURBS Preprocessed"
echo "Location: /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp"
echo ""
echo "Algorithm Parameters (ORIGINAL GITHUB):"
echo "  TOP_k = 5 (beam search)"
echo "  BRANCH_b = 3 (branching factor)"
echo "  RejectOutlier thresholds: (20, 0.7), (10, 0.8), (10, 0.7)"
echo "  Merge similarity: (0.175, 20)"
echo "  Axis angle threshold: 0.436 rad (25°)"
echo "  Weighting: pow(ratio, 2)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp

echo "=== Executable Information ==="
ls -lh build/Hierarchy-Clear-headless
echo ""

echo "=== Running Legacy SFS++ on NURBS Pot A Dataset ==="
echo "Running headless version (auto-save first result)"
echo ""

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed:/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace/build && ./Hierarchy-Clear-headless"

RUN_STATUS=$?
echo ""
echo "========================================================================="
echo "Execution exit code: $RUN_STATUS"
echo ""

echo "=== Results ==="
RESULT_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp/Result"
if [ -d "$RESULT_DIR" ]; then
    echo "Results directory found:"
    ls -lht "$RESULT_DIR" | head -20
    echo ""
    echo "Latest result files:"
    find "$RESULT_DIR" -type f -mmin -120 -ls | head -10
else
    echo "No results directory found at $RESULT_DIR"
fi
echo ""

echo "End time: $(date)"
echo "========================================================================="
