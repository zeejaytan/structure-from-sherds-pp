#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_top_k_test

# TOP-K TRANSFORMATION TEST JOB
# Extended 6-hour time limit for TOP-K testing
# Debug output enabled

echo "=== SFS TOP-K TRANSFORMATION TEST JOB ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing TOP-K transformation generation with K=2"
echo "Expected: Resolve Orange/Purple spatial constraints, achieve unified graph assembly"
echo "Extended time limit: 6 hours for comprehensive testing"

# Load required modules
module load GCC/11.3.0
module load CMake/3.24.3-GCCcore-11.3.0

echo "Building SFS reconstruction binary with TOP-K transformations..."
cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker

# Build with TOP-K transformations enabled
echo "Compiling with TOP-K transformation support..."
/home/zhuojiat/.local/bin/cmake . || {
    echo "CMake configuration failed"
    exit 1
}

make clean
make -j 8 || {
    echo "Build failed"
    exit 1
}

echo "Build completed successfully!"
echo "Binary timestamp: $(stat -c %y main_headless_correct)"

# Verify NURBS dataset exists
DATASET_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature"
echo "Using NURBS dataset: $DATASET_DIR"

if [ ! -d "$DATASET_DIR" ]; then
    echo "ERROR: NURBS dataset not found at $DATASET_DIR"
    exit 1
fi

# Verify dataset structure
echo "Verifying NURBS dataset structure..."
echo "Surfaces: $(find $DATASET_DIR/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find $DATASET_DIR/SfS_pp/Breaklines -name "*.pcd" | wc -l)"
echo "Axes: $(find $DATASET_DIR/SfS_pp/Axes -name "*.xyz" | wc -l)"

# Create timestamped output directory
OUTPUT_DIR="/data/gpfs/projects/punim2657/sfs_main/results_top_k_$(date +%Y_%m_%d_%H%M)"
echo "Results will be saved to: $OUTPUT_DIR"

echo ""
echo "Running SFS reconstruction with TOP-K transformations..."
echo "Configuration: K=2, DEBUG=enabled, Extended beam search"
echo "Expected improvements:"
echo "  • Multiple transformation options per piece pair"
echo "  • Beam search tests different spatial configurations"
echo "  • Orange/Purple positioned to avoid blocking Red connections"
echo "  • Unified graph assembly instead of 3-fragment result"

# Run SFS reconstruction with container
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature:/Dataset/SfS_pp \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/pcl_191_nurbs.sif \
    /workspace/sfspreproc-docker/main_headless_correct 8

echo ""
echo "=== TOP-K TRANSFORMATION TEST RESULTS ==="
echo "Job completed at: $(date)"

# Check if results were generated
if [ -d "$OUTPUT_DIR" ]; then
    echo "✅ Results generated successfully"
    echo "Assembly data file: $OUTPUT_DIR/assembly_data.txt"

    if [ -f "$OUTPUT_DIR/assembly_data.txt" ]; then
        echo ""
        echo "=== ASSEMBLY RESULT SUMMARY ==="
        grep -E "Assembly Score|Pieces Assembled|Best State" "$OUTPUT_DIR/assembly_data.txt"

        echo ""
        echo "=== GRAPH FRAGMENTATION CHECK ==="
        echo "Checking for unified vs fragmented assembly..."
        graph_count=$(grep -c "Transformation found in graph" "$OUTPUT_DIR/assembly_data.txt" | head -1)
        unique_graphs=$(grep "Transformation found in graph" "$OUTPUT_DIR/assembly_data.txt" | sort | uniq | wc -l)
        echo "Total piece assignments: $graph_count"
        echo "Unique graphs used: $unique_graphs"

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯 SUCCESS: UNIFIED GRAPH ACHIEVED!"
            echo "All pieces assembled in single graph - TOP-K transformations resolved spatial constraints!"
        else
            echo "⚠ FRAGMENTED: $unique_graphs separate graphs"
            echo "Further TOP-K parameter tuning may be needed"
        fi
    fi

    echo "Complete results available in: $OUTPUT_DIR"
else
    echo "❌ No output directory created - check logs for errors"
fi

echo ""
echo "=== TOP-K TRANSFORMATION DEBUG SUMMARY ==="
echo "Search for debug output in job logs:"
echo "  • '*** TOP-K DEBUG ***' - Transformation generation and selection"
echo "  • 'Generated TOP-K transformations' - Confirmation of multi-config generation"
echo "  • 'There is no more new root' - Previous failure pattern (should be reduced)"

echo "TOP-K transformation test job completed."