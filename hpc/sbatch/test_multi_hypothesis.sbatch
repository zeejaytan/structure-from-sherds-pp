#!/bin/bash
#SBATCH --job-name=test_multi_hypothesis
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=test_multi_hypothesis_%j.out
#SBATCH --error=test_multi_hypothesis_%j.err

echo "=== TESTING MULTI-HYPOTHESIS GLOBAL OPTIMIZATION ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "🎯 Testing new Multi-Hypothesis Global Optimization system"
echo "📝 Using pseudocode from global ideas.txt"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Running SFS reconstruction with Multi-Hypothesis Global Optimization..."
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo "🚀 Multi-Hypothesis: ENABLED - K=5 hypotheses per edge pair"
echo "🎯 Cycle Consistency: ENABLED - Global compatibility pruning"
echo "🏆 Binary Switch Optimization: ENABLED - Alternating pose-switch updates"
echo "✨ Output: Results will be saved in results_YYYY_MM_DD_HHMM/ directory"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "export ENABLE_MULTI_HYPOTHESIS=1 && echo 'Container ENV: ENABLE_MULTI_HYPOTHESIS='\$ENABLE_MULTI_HYPOTHESIS && /workspace/sfspreproc-docker/Hierarchy-Clear 8"

echo
echo "=== Multi-Hypothesis Optimization Results ==="
echo "Timestamped output directories created:"
ls -la /data/gpfs/projects/punim2657/sfs_main/results_* 2>/dev/null | tail -3

echo
echo "Latest result contents:"
LATEST_DIR=$(ls -td /data/gpfs/projects/punim2657/sfs_main/results_* 2>/dev/null | head -1)
if [ -d "$LATEST_DIR" ]; then
    echo "📁 $(basename $LATEST_DIR):"
    echo "  Main files: $(ls -1 "$LATEST_DIR"/*.txt "$LATEST_DIR"/*.xyz 2>/dev/null | wc -l) files"
    echo "  Result files: $(ls -1 "$LATEST_DIR"/Result/*.txt "$LATEST_DIR"/Result/*.xyz 2>/dev/null | wc -l) files"
    echo "  Size: $(du -sh "$LATEST_DIR" 2>/dev/null | cut -f1)"
fi

echo
echo "=== Multi-Hypothesis Test Complete ==="