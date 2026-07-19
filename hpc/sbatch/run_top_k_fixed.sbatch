#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_top_k_fixed

# TOP-K TRANSFORMATION TEST JOB - FIXED VERSION
# Build inside container where all dependencies are available

echo "=== SFS TOP-K TRANSFORMATION TEST JOB (FIXED) ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Building inside container to avoid dependency issues"

# Verify NURBS dataset exists
DATASET_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Using NURBS dataset: $DATASET_DIR"

if [ ! -d "$DATASET_DIR" ]; then
    echo "ERROR: NURBS dataset not found at $DATASET_DIR"
    exit 1
fi

# Create timestamped output directory
OUTPUT_DIR="/data/gpfs/projects/punim2657/sfs_main/results_top_k_$(date +%Y_%m_%d_%H%M)"
echo "Results will be saved to: $OUTPUT_DIR"

echo ""
echo "Building and running SFS reconstruction with TOP-K transformations..."
echo "Configuration: K=2, DEBUG=enabled, Extended beam search"

# Build and run inside container
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

# Build inside container
echo "Building TOP-K enabled binary inside container..."
$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/pcl_191_nurbs.sif \
    bash -c "cd /workspace/sfspreproc-docker && cmake . && make clean && make -j 8"

if [ $? -ne 0 ]; then
    echo "Build failed inside container"
    exit 1
fi

echo "Build completed successfully!"

# Run SFS reconstruction
echo "Running SFS reconstruction with TOP-K transformations..."
$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature:/Dataset/SfS_pp \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/pcl_191_nurbs.sif \
    /workspace/sfspreproc-docker/main_headless_correct 8

echo ""
echo "=== TOP-K TRANSFORMATION TEST RESULTS ==="
echo "Job completed at: $(date)"

# Check if results were generated
LATEST_RESULTS_DIR=$(find /data/gpfs/projects/punim2657/sfs_main -name "results_*" -type d | sort | tail -1)

if [ -n "$LATEST_RESULTS_DIR" ] && [ -d "$LATEST_RESULTS_DIR" ]; then
    echo "✅ Results generated successfully"
    echo "Results directory: $LATEST_RESULTS_DIR"

    if [ -f "$LATEST_RESULTS_DIR/assembly_data.txt" ]; then
        echo ""
        echo "=== ASSEMBLY RESULT SUMMARY ==="
        grep -E "Assembly Score|Pieces Assembled|Best State" "$LATEST_RESULTS_DIR/assembly_data.txt"

        echo ""
        echo "=== GRAPH FRAGMENTATION CHECK ==="
        unique_graphs=$(grep "Transformation found in graph" "$LATEST_RESULTS_DIR/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Unique graphs used: $unique_graphs"

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯 SUCCESS: UNIFIED GRAPH ACHIEVED!"
            echo "All pieces assembled in single graph - TOP-K transformations resolved spatial constraints!"
        elif [ "$unique_graphs" -eq 2 ]; then
            echo "⚠ IMPROVEMENT: Reduced to $unique_graphs graphs (was 3)"
            echo "TOP-K transformations showing improvement!"
        else
            echo "⚠ FRAGMENTED: $unique_graphs separate graphs"
        fi

        echo ""
        echo "=== TOP-K DEBUG SUMMARY ==="
        echo "Transformation generation events:"
        grep -c "*** TOP-K DEBUG *** Generated.*transformation options" "$SLURM_SUBMIT_DIR/slurm-$SLURM_JOB_ID.out" 2>/dev/null || echo "Check logs for TOP-K debug output"

        echo ""
        echo "Graph merging 'There is no more new root' failures:"
        grep -c "There is no more new root" "$SLURM_SUBMIT_DIR/slurm-$SLURM_JOB_ID.out" 2>/dev/null || echo "Check logs for merging failures"
    fi

    echo "Complete results available in: $LATEST_RESULTS_DIR"
else
    echo "❌ No output directory created - check logs for errors"
fi

echo "TOP-K transformation test job completed."