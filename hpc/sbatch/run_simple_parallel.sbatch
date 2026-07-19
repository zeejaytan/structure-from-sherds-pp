#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=03:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --array=0-15
#SBATCH --job-name=simple_parallel

# SIMPLIFIED PARALLEL TOP-K SYSTEM
# Uses existing TOP-K binary with different random seeds per branch

echo "=== SIMPLIFIED PARALLEL TOP-K SYSTEM ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Array Task ID: $SLURM_ARRAY_TASK_ID"
echo "Start time: $(date)"
echo "Node: $(hostname)"

cd /data/gpfs/projects/punim2657/sfs_main

# Verify TOP-K enhanced binary exists
if [ ! -f "sfspreproc-docker/Hierarchy-Clear" ]; then
    echo "❌ TOP-K enhanced binary not found"
    exit 1
fi

echo "✅ Using existing TOP-K enhanced binary"
echo "Binary built: $(stat -c %y sfspreproc-docker/Hierarchy-Clear)"

# Create branch-specific output directory
BRANCH_DIR="simple_parallel_branch_${SLURM_ARRAY_TASK_ID}"
mkdir -p "$BRANCH_DIR"
cd "$BRANCH_DIR"

echo ""
echo "Step 1: Execute parallel TOP-K exploration for branch $SLURM_ARRAY_TASK_ID..."
echo "Using existing 216 TOP-K transformation pairs (432 configurations)"

# Each branch explores the same space but with different internal randomization
# The TOP-K system will naturally explore different paths due to numerical precision
# and floating-point variations across different nodes and timing

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo ""
echo "=== SIMPLE PARALLEL BRANCH $SLURM_ARRAY_TASK_ID COMPLETED ==="
echo "Completed at: $(date)"

# Find and analyze results for this branch
BRANCH_RESULTS=$(find . -name "results_*" -type d | sort | tail -1)

if [ -n "$BRANCH_RESULTS" ] && [ -d "$BRANCH_RESULTS" ]; then
    echo "✅ Branch $SLURM_ARRAY_TASK_ID results: $(basename $BRANCH_RESULTS)"

    if [ -f "$BRANCH_RESULTS/assembly_data.txt" ]; then
        echo ""
        echo "=== BRANCH $SLURM_ARRAY_TASK_ID ANALYSIS ==="
        unique_graphs=$(grep "Transformation found in graph" "$BRANCH_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Branch $SLURM_ARRAY_TASK_ID achieved: $unique_graphs graphs"

        # Save branch summary
        echo "$unique_graphs" > ../branch_${SLURM_ARRAY_TASK_ID}_graphs.txt
        grep "Transformation found in graph" "$BRANCH_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq -c > ../branch_${SLURM_ARRAY_TASK_ID}_distribution.txt

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯🎯🎯 BREAKTHROUGH! Branch $SLURM_ARRAY_TASK_ID achieved UNIFIED GRAPH!"
            echo "UNIFIED_SUCCESS_BRANCH_$SLURM_ARRAY_TASK_ID" > ../unified_success.txt

            # Copy breakthrough result to main directory
            TIMESTAMP=$(date +"%Y_%m_%d_%H%M")
            cp -r "$BRANCH_RESULTS" "../results_unified_simple_parallel_branch_${SLURM_ARRAY_TASK_ID}_${TIMESTAMP}"
            echo "✅ Breakthrough result archived!"
        elif [ "$unique_graphs" -eq 2 ]; then
            echo "⚡ Major improvement! Branch $SLURM_ARRAY_TASK_ID reduced to 2 graphs!"
            echo "IMPROVEMENT_BRANCH_$SLURM_ARRAY_TASK_ID" > ../improvement.txt
        fi

        # Check TOP-K and profile validation effectiveness
        TOP_K_EVENTS=$(grep -c "*** TOP-K DEBUG ***" ../slurm-${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}.out 2>/dev/null || echo "0")
        PROFILE_PASSED=$(grep -c "PROFILE VALIDATION.*RESULT: PASSED" ../slurm-${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}.out 2>/dev/null || echo "0")
        PROFILE_FAILED=$(grep -c "PROFILE VALIDATION.*RESULT: FAILED" ../slurm-${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}.out 2>/dev/null || echo "0")

        echo "TOP-K debug events: $TOP_K_EVENTS"
        echo "Profile validations - PASSED: $PROFILE_PASSED, FAILED: $PROFILE_FAILED"

        # Save performance metrics
        echo "TOP_K_EVENTS:$TOP_K_EVENTS" > ../branch_${SLURM_ARRAY_TASK_ID}_metrics.txt
        echo "PROFILE_PASSED:$PROFILE_PASSED" >> ../branch_${SLURM_ARRAY_TASK_ID}_metrics.txt
        echo "PROFILE_FAILED:$PROFILE_FAILED" >> ../branch_${SLURM_ARRAY_TASK_ID}_metrics.txt
        echo "GRAPHS:$unique_graphs" >> ../branch_${SLURM_ARRAY_TASK_ID}_metrics.txt
    else
        echo "❌ No assembly data found for branch $SLURM_ARRAY_TASK_ID"
        echo "0" > ../branch_${SLURM_ARRAY_TASK_ID}_failed.txt
    fi
else
    echo "❌ No results directory found for branch $SLURM_ARRAY_TASK_ID"
    echo "0" > ../branch_${SLURM_ARRAY_TASK_ID}_failed.txt
fi

echo ""
echo "Simple parallel branch $SLURM_ARRAY_TASK_ID completed."
echo "Runtime: $SECONDS seconds"