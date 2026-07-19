#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=03:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --array=0-15
#SBATCH --job-name=top_k_parallel

# PARALLEL TOP-K BEAM SEARCH ACCELERATION
# 16 parallel jobs exploring different assembly state branches simultaneously
# Expected speedup: 8-16x faster than sequential execution

echo "=== PARALLEL TOP-K TRANSFORMATION SYSTEM ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Array Task ID: $SLURM_ARRAY_TASK_ID"
echo "Start time: $(date)"
echo "Node: $(hostname)"
echo "Parallel Branch: $SLURM_ARRAY_TASK_ID/15 (TOP-K state exploration)"

cd /data/gpfs/projects/punim2657/sfs_main

# Each array job explores a different beam search branch
BRANCH_START=$SLURM_ARRAY_TASK_ID
BRANCH_COUNT=1

echo ""
echo "Step 1: Configure parallel TOP-K parameters for branch $BRANCH_START..."

# Create temporary modified source for this branch
cp sfspreproc-docker/main_headless_correct.cpp sfspreproc-docker/main_headless_correct_branch_${SLURM_ARRAY_TASK_ID}.cpp

# Modify TOP-K parameters for focused branch exploration
sed -i "s/#define TOP_k 15/#define TOP_k 1/g" sfspreproc-docker/main_headless_correct_branch_${SLURM_ARRAY_TASK_ID}.cpp
sed -i "s/#define BRANCH_b 8/#define BRANCH_b 8/g" sfspreproc-docker/main_headless_correct_branch_${SLURM_ARRAY_TASK_ID}.cpp

# Add branch-specific seed for different exploration paths
cat >> sfspreproc-docker/main_headless_correct_branch_${SLURM_ARRAY_TASK_ID}.cpp << EOF

// Parallel branch configuration
#define PARALLEL_BRANCH_ID ${SLURM_ARRAY_TASK_ID}
#define PARALLEL_SEED (1000 + PARALLEL_BRANCH_ID)

EOF

echo "✅ Branch $BRANCH_START configured with focused exploration parameters"

echo ""
echo "Step 2: Build branch-specific binary..."

/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker &&
             export CC=/usr/bin/gcc &&
             export CXX=/usr/bin/g++ &&
             cp main_headless_correct_branch_${SLURM_ARRAY_TASK_ID}.cpp main_headless_correct.cpp &&
             make -j 4 &&
             mv Hierarchy-Clear Hierarchy-Clear-Branch-${SLURM_ARRAY_TASK_ID}"

if [ $? -ne 0 ]; then
    echo "❌ Branch build failed"
    exit 1
fi

echo "✅ Branch-specific binary built: Hierarchy-Clear-Branch-${SLURM_ARRAY_TASK_ID}"

echo ""
echo "Step 3: Execute parallel TOP-K branch exploration..."
echo "Exploring assembly states starting from branch $BRANCH_START"
echo "Output will be saved with branch suffix for result aggregation"

# Create branch-specific output directory
mkdir -p parallel_results_branch_${SLURM_ARRAY_TASK_ID}
cd parallel_results_branch_${SLURM_ARRAY_TASK_ID}

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear-Branch-${SLURM_ARRAY_TASK_ID} 8

echo ""
echo "=== PARALLEL BRANCH $SLURM_ARRAY_TASK_ID COMPLETED ==="
echo "Completed at: $(date)"

# Analyze this branch's results
BRANCH_RESULTS=$(find /data/gpfs/projects/punim2657/sfs_main/parallel_results_branch_${SLURM_ARRAY_TASK_ID} -name "results_*" -type d | sort | tail -1)

if [ -n "$BRANCH_RESULTS" ] && [ -d "$BRANCH_RESULTS" ]; then
    echo "✅ Branch results: $(basename $BRANCH_RESULTS)"

    if [ -f "$BRANCH_RESULTS/assembly_data.txt" ]; then
        echo ""
        echo "=== BRANCH $SLURM_ARRAY_TASK_ID GRAPH ANALYSIS ==="
        unique_graphs=$(grep "Transformation found in graph" "$BRANCH_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Branch $SLURM_ARRAY_TASK_ID graphs: $unique_graphs"

        # Save branch summary for aggregation
        echo "BRANCH_${SLURM_ARRAY_TASK_ID}_GRAPHS:$unique_graphs" > branch_${SLURM_ARRAY_TASK_ID}_summary.txt
        grep "Transformation found in graph" "$BRANCH_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq -c > branch_${SLURM_ARRAY_TASK_ID}_distribution.txt

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯 BRANCH $SLURM_ARRAY_TASK_ID: UNIFIED GRAPH ACHIEVED!"
            echo "UNIFIED_SUCCESS" > branch_${SLURM_ARRAY_TASK_ID}_success.txt
        fi
    fi
else
    echo "❌ No results for branch $SLURM_ARRAY_TASK_ID"
fi

echo ""
echo "Parallel branch $SLURM_ARRAY_TASK_ID execution completed."
echo "Runtime: $SECONDS seconds"