#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=03:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --job-name=sfs_simple_test

# SIMPLE TEST - USE EXISTING BINARY FIRST

echo "=== SFS SIMPLE TEST - EXISTING BINARY ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Using existing Hierarchy-Clear binary built at: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear)"

cd /data/gpfs/projects/punim2657/sfs_main

# Check if binary has TOP-K features by looking at the source
echo "Checking if TOP-K features are compiled in..."
grep -q "ENABLE_TOP_K_TRANSFORMS" sfspreproc-docker/main_headless_correct.cpp && echo "✅ TOP-K source modifications found" || echo "❌ No TOP-K modifications in source"

# Run with existing binary
echo ""
echo "Running SFS reconstruction with existing binary..."

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo ""
echo "=== SIMPLE TEST RESULTS ==="
echo "Job completed at: $(date)"

# Find results
LATEST_RESULTS=$(find /data/gpfs/projects/punim2657/sfs_main -name "results_*" -type d | sort | tail -1)

if [ -n "$LATEST_RESULTS" ] && [ -d "$LATEST_RESULTS" ]; then
    echo "✅ Results: $(basename $LATEST_RESULTS)"

    if [ -f "$LATEST_RESULTS/assembly_data.txt" ]; then
        echo ""
        echo "=== GRAPH FRAGMENTATION CHECK ==="
        unique_graphs=$(grep "Transformation found in graph" "$LATEST_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Number of graphs: $unique_graphs"

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯 SUCCESS: UNIFIED GRAPH!"
        elif [ "$unique_graphs" -le 2 ]; then
            echo "⚡ IMPROVEMENT: $unique_graphs graphs (was 3)"
        else
            echo "📊 Current: $unique_graphs graphs"
        fi
    fi
else
    echo "❌ No results found"
fi

echo "Simple test completed."