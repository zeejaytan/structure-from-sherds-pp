#!/bin/bash
#SBATCH --job-name=test_fixed_nurbs
#SBATCH --time=00:30:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --partition=cascade

# Load required modules
module load GCC/11.3.0 OpenMPI/4.1.4

echo "========================================================================="
echo "         TESTING FIXED NURBS DATASET (Nov 3, 2025)"
echo "========================================================================="
echo "Start time: $(date)"
echo ""
echo "Dataset: NURBS_Dataset_20251103 (NURBS normals - CORRECT)"
echo "Executable: Hierarchy-Clear-headless (relaxed thresholds)"
echo "Expected: 7-8/8 pieces, 12-15/15 edges (85-100% accuracy)"
echo ""
echo "========================================================================="
echo ""

cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp

CONTAINER_SIF="/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif"
SOURCE_DIR="/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp"

# Create output directory with timestamp
OUTPUT_DIR="test_results_fixed_nurbs_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

echo "Running assembly with fixed NURBS dataset..."
echo ""

# Run the headless executable inside container with output redirection
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "${SOURCE_DIR}:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing:/data/gpfs/projects/punim2657/sfs_preprocessing" \
    "$CONTAINER_SIF" \
    /workspace/build/Hierarchy-Clear-headless > "$OUTPUT_DIR/assembly_output.log" 2>&1

echo ""
echo "========================================================================="
echo "                         TEST COMPLETED"
echo "========================================================================="
echo ""
echo "End time: $(date)"
echo ""

# Check for output files
echo "Output files generated:"
if [ -f "assembly.ply" ]; then
    mv assembly.ply "$OUTPUT_DIR/"
    echo "  ✓ assembly.ply"
fi

if [ -f "edges.ply" ]; then
    mv edges.ply "$OUTPUT_DIR/"
    echo "  ✓ edges.ply"
fi

if [ -f "$OUTPUT_DIR/assembly_output.log" ]; then
    echo "  ✓ assembly_output.log"
fi

echo ""
echo "Results directory: $OUTPUT_DIR"
echo ""

# Extract key metrics from log
echo "========================================================================="
echo "                         ASSEMBLY METRICS"
echo "========================================================================="
echo ""

if [ -f "$OUTPUT_DIR/assembly_output.log" ]; then
    echo "Searching for assembly results..."

    # Look for normal rejections
    NORMAL_REJECTS=$(grep -i "reject.*normal" "$OUTPUT_DIR/assembly_output.log" | wc -l)
    echo "  Normal rejections found: $NORMAL_REJECTS lines"

    # Look for piece matches
    PIECES_MATCHED=$(grep -i "piece.*match" "$OUTPUT_DIR/assembly_output.log" | tail -5)
    if [ -n "$PIECES_MATCHED" ]; then
        echo ""
        echo "Piece matching results:"
        echo "$PIECES_MATCHED"
    fi

    # Look for edge count
    EDGE_COUNT=$(grep -i "edge" "$OUTPUT_DIR/assembly_output.log" | grep -i "total\|found\|discover" | tail -3)
    if [ -n "$EDGE_COUNT" ]; then
        echo ""
        echo "Edge discovery results:"
        echo "$EDGE_COUNT"
    fi
fi

echo ""
echo "========================================================================="
echo ""
echo "Full log available at: $OUTPUT_DIR/assembly_output.log"
echo ""
