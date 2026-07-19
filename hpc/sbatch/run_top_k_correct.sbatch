#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_top_k_correct

# TOP-K TRANSFORMATION TEST - CORRECT CONTAINER PATH

echo "=== SFS TOP-K TRANSFORMATION TEST (CORRECTED) ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Using correct container path: sfspreproc.sif"

cd /data/gpfs/projects/punim2657/sfs_main

# Verify NURBS dataset
DATASET_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Verifying NURBS dataset: $DATASET_DIR"
echo "Surfaces: $(find $DATASET_DIR/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find $DATASET_DIR/SfS_pp/Breaklines -name "*.pcd" | wc -l)"
echo "Axes: $(find $DATASET_DIR/SfS_pp/Axes -name "*.xyz" | wc -l)"

echo ""
echo "Building TOP-K enhanced binary inside container..."
echo "Configuration: K=2, Debug enabled, Extended beam search"

# Build inside correct container
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && cmake . && make clean && make -j 8"

if [ $? -ne 0 ]; then
    echo "❌ Build failed inside container"
    exit 1
fi

echo "✅ Build completed successfully!"
echo "Binary timestamp: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/main_headless_correct)"

echo ""
echo "Running SFS reconstruction with TOP-K transformations..."

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/main_headless_correct 8

echo ""
echo "=== TOP-K TRANSFORMATION RESULTS ANALYSIS ==="
echo "Job completed at: $(date)"

# Find latest results
LATEST_RESULTS=$(find /data/gpfs/projects/punim2657/sfs_main -name "results_*" -type d | sort | tail -1)

if [ -n "$LATEST_RESULTS" ] && [ -d "$LATEST_RESULTS" ]; then
    echo "✅ Results generated: $(basename $LATEST_RESULTS)"

    if [ -f "$LATEST_RESULTS/assembly_data.txt" ]; then
        echo ""
        echo "=== ASSEMBLY OUTCOME ==="
        grep -E "Assembly Score|Pieces Assembled|Best State" "$LATEST_RESULTS/assembly_data.txt"

        echo ""
        echo "=== GRAPH FRAGMENTATION ANALYSIS ==="
        unique_graphs=$(grep "Transformation found in graph" "$LATEST_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Number of separate graphs: $unique_graphs"

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯 BREAKTHROUGH: UNIFIED GRAPH ACHIEVED!"
            echo "TOP-K transformations resolved the spatial constraint issue!"
        elif [ "$unique_graphs" -eq 2 ]; then
            echo "⚡ SIGNIFICANT IMPROVEMENT: Reduced from 3 to $unique_graphs graphs"
            echo "TOP-K transformations are working!"
        else
            echo "📊 Current state: $unique_graphs separate graphs (was 3)"
        fi

        # Show which pieces are in which graphs
        echo ""
        echo "=== PIECE-GRAPH ASSIGNMENTS ==="
        grep -E "Piece [0-9]+.*graph [0-9]+" "$LATEST_RESULTS/assembly_data.txt" | sort -V

    fi

    echo ""
    echo "=== TOP-K DEBUG ANALYSIS ==="
    echo "TOP-K transformation generation events:"
    echo "$(grep -c "*** TOP-K DEBUG *** Generated.*transformation options" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0") piece pairs got TOP-K transformations"

    echo "TOP-K configuration assignments:"
    echo "$(grep -c "*** TOP-K DEBUG *** State.*using config" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0") beam search state configurations"

    echo "Spatial conflict failures:"
    echo "$(grep -c "There is no more new root" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0") graph merging failures"

    echo ""
    echo "Complete results: $LATEST_RESULTS"
else
    echo "❌ No results directory found"
fi

echo ""
echo "TOP-K transformation test completed."