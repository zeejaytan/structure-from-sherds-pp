#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --array=0-15
#SBATCH --job-name=parallel_build_run

# PARALLEL TOP-K WITH INDIVIDUAL BUILDS
# Each job builds its own TOP-K binary and runs

echo "=== PARALLEL TOP-K WITH BUILD - BRANCH $SLURM_ARRAY_TASK_ID ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Array Task ID: $SLURM_ARRAY_TASK_ID"
echo "Start time: $(date)"
echo "Node: $(hostname)"

cd /data/gpfs/projects/punim2657/sfs_main

# Create branch-specific workspace
BRANCH_DIR="parallel_branch_${SLURM_ARRAY_TASK_ID}"
mkdir -p "$BRANCH_DIR"
cp -r sfspreproc-docker "$BRANCH_DIR/"
cd "$BRANCH_DIR"

echo ""
echo "Step 1: Build TOP-K enhanced binary for branch $SLURM_ARRAY_TASK_ID..."

# Build inside container
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/$BRANCH_DIR:/workspace" \
    ../sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker &&
             export CC=/usr/bin/gcc &&
             export CXX=/usr/bin/g++ &&
             cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ . &&
             make clean &&
             make -j 4"

if [ $? -ne 0 ]; then
    echo "❌ Build failed for branch $SLURM_ARRAY_TASK_ID"
    exit 1
fi

echo "✅ Binary built successfully for branch $SLURM_ARRAY_TASK_ID"
ls -la sfspreproc-docker/Hierarchy-Clear

echo ""
echo "Step 2: Execute TOP-K exploration for branch $SLURM_ARRAY_TASK_ID..."

# Run the built binary
time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/$BRANCH_DIR:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    ../sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo ""
echo "=== PARALLEL BRANCH $SLURM_ARRAY_TASK_ID COMPLETED ==="
echo "Completed at: $(date)"

# Analyze results
BRANCH_RESULTS=$(find . -name "results_*" -type d | sort | tail -1)

if [ -n "$BRANCH_RESULTS" ] && [ -d "$BRANCH_RESULTS" ]; then
    echo "✅ Branch $SLURM_ARRAY_TASK_ID results: $(basename $BRANCH_RESULTS)"

    if [ -f "$BRANCH_RESULTS/assembly_data.txt" ]; then
        unique_graphs=$(grep "Transformation found in graph" "$BRANCH_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Branch $SLURM_ARRAY_TASK_ID achieved: $unique_graphs graphs"

        # Save results to main directory
        echo "$unique_graphs" > ../parallel_branch_${SLURM_ARRAY_TASK_ID}_graphs.txt

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯🎯🎯 BREAKTHROUGH! Branch $SLURM_ARRAY_TASK_ID achieved UNIFIED GRAPH!"
            TIMESTAMP=$(date +"%Y_%m_%d_%H%M")
            cp -r "$BRANCH_RESULTS" "../results_unified_parallel_branch_${SLURM_ARRAY_TASK_ID}_${TIMESTAMP}"
        fi
    fi
else
    echo "❌ No results found for branch $SLURM_ARRAY_TASK_ID"
fi

echo "Parallel branch $SLURM_ARRAY_TASK_ID runtime: $SECONDS seconds"