#!/bin/bash
#SBATCH --job-name=sfs_legacy_tray000
#SBATCH --time=04:00:00
#SBATCH --mem=32G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire
#SBATCH --output=sfs_legacy_tray000_%j.out
#SBATCH --error=sfs_legacy_tray000_%j.err

echo "========================================================================"
echo "        SFS++ LEGACY SYSTEM - TRAY-000 ASSEMBLY (40 pieces)           "
echo "========================================================================"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp

echo "=== Configuration ==="
echo "Dataset: Tray-000 (40 pieces)"
echo "Source: /data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/"
echo "Algorithm: Original SFS++ (beam search with TOP_k=5, BRANCH_b=3)"
echo "Data format: Millimeters (freshly converted)"
echo ""
echo "data_path.h configuration:"
echo "  #define TRAY_000"
echo "  SHARD_NUMBER = 40"
echo "  path = /Dataset/SfS_pp/"
echo ""

# Verify TRAY_000 is defined in data_path.h
echo "=== Verifying Configuration ==="
if grep -q "^#define TRAY_000" class/data_path.h; then
    echo "✓ TRAY_000 is defined in data_path.h"
else
    echo "ERROR: TRAY_000 not defined in data_path.h"
    echo "Current configuration:"
    grep "^#define POT\|^#define TRAY" class/data_path.h | head -5
    exit 1
fi
echo ""

# Verify Tray-000 dataset exists and has correct structure
echo "=== Verifying Tray-000 Dataset ==="
TRAY_DATA="/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp"
echo "Checking dataset at: ${TRAY_DATA}"

if [ ! -d "${TRAY_DATA}" ]; then
    echo "ERROR: Dataset directory not found: ${TRAY_DATA}"
    exit 1
fi

echo "Dataset structure:"
echo "  Surfaces: $(ls ${TRAY_DATA}/Surfaces/Tray-000_Piece_*_Surface_0.xyz 2>/dev/null | wc -l) files"
echo "  Breaklines: $(ls ${TRAY_DATA}/Breaklines/Tray-000_Piece_*_Breakline_0.pcd 2>/dev/null | wc -l) files"
echo "  Axes: $(ls ${TRAY_DATA}/Axes/Tray-000_Piece_*_Axis.xyz 2>/dev/null | wc -l) files"
echo "  Meshes: $(ls ${TRAY_DATA}/Mesh/Tray-000_Piece_*_Mesh.obj 2>/dev/null | wc -l) files"
echo ""

# Check coordinate units (should be millimeters)
echo "Sample coordinates (should be in millimeters):"
head -1 ${TRAY_DATA}/Surfaces/Tray-000_Piece_01_Surface_0.xyz
echo ""

echo "=== Building Legacy SFS++ Code for Tray-000 ==="
echo "Building with CMake inside container..."

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp:/workspace" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace && rm -rf build && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8"

BUILD_STATUS=$?
echo ""
if [ $BUILD_STATUS -ne 0 ]; then
    echo "ERROR: Build failed with exit code $BUILD_STATUS"
    exit 1
fi

echo "✓ Build complete!"
echo "Binary location: build/Hierarchy-Clear-headless"
ls -lh build/Hierarchy-Clear-headless 2>/dev/null || echo "ERROR: Headless binary not found!"
echo ""

echo "========================================================================"
echo "                  RUNNING SFS++ ON TRAY-000 DATASET                    "
echo "========================================================================"
echo "This will take approximately 1-2 hours for 40 pieces..."
echo "Algorithm will perform:"
echo "  1. Pairwise feature matching (40 × 39 / 2 = 780 pairs)"
echo "  2. ICP refinement for promising pairs"
echo "  3. Incremental beam search assembly"
echo "  4. Global structure optimization"
echo ""

# Create Result directory
mkdir -p /data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result

# Run the headless legacy code with Tray-000 dataset bound to /Dataset/
echo "Starting assembly at: $(date)"
echo ""

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021:/Dataset" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace/build && ./Hierarchy-Clear-headless"

RUN_STATUS=$?
echo ""
echo "Execution exit code: $RUN_STATUS"
echo "Finished at: $(date)"
echo ""

echo "========================================================================"
echo "                           RESULTS                                      "
echo "========================================================================"

RESULT_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result"

if [ -d "${RESULT_DIR}" ]; then
    echo "Results directory: ${RESULT_DIR}"
    echo ""

    # Count results
    SURFACE_COUNT=$(ls ${RESULT_DIR}/*surface.xyz 2>/dev/null | wc -l)
    EDGE_COUNT=$(ls ${RESULT_DIR}/*edge.xyz 2>/dev/null | wc -l)
    TRANSFORM_COUNT=$(ls ${RESULT_DIR}/T_*_to_*.txt 2>/dev/null | wc -l)

    echo "Generated files:"
    echo "  Surface assemblies: ${SURFACE_COUNT} files"
    echo "  Edge assemblies: ${EDGE_COUNT} files"
    echo "  Transformations: ${TRANSFORM_COUNT} files"
    echo ""

    if [ ${SURFACE_COUNT} -gt 0 ]; then
        echo "Assembly files:"
        ls -lh ${RESULT_DIR}/*surface.xyz 2>/dev/null | head -10
        echo ""
    fi

    if [ -f "${RESULT_DIR}/1. Acc.txt" ]; then
        echo "Accuracy report found: ${RESULT_DIR}/1. Acc.txt"
        echo "Content:"
        cat ${RESULT_DIR}/1.\ Acc.txt
        echo ""
    fi

    echo "Full results listing:"
    ls -lh ${RESULT_DIR}/ | head -50
    echo ""

else
    echo "ERROR: No results directory found at ${RESULT_DIR}"
    echo "Checking for error messages in output..."
    echo ""
fi

echo "========================================================================"
echo "                 GENERATING PLY VISUALIZATION FILES                    "
echo "========================================================================"
echo ""
# Generate PLY visualization files directly in Result directory
python3 /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/scripts/generate_assembly_ply.py ${RESULT_DIR}

PLY_STATUS=$?
echo ""
if [ $PLY_STATUS -eq 0 ]; then
    echo "✓ PLY visualization files generated successfully"
    echo "  Main file: ${RESULT_DIR}/assembly_all_pieces.ply"
else
    echo "⚠ PLY generation had issues (check output above)"
fi
echo ""

echo "========================================================================"
echo "                      VISUALIZATION GUIDE                               "
echo "========================================================================"
echo ""
echo "To visualize the assembled pieces:"
echo ""
echo "1. View combined assembly (RECOMMENDED):"
echo "   meshlab ${RESULT_DIR}/assembly_all_pieces.ply"
echo ""
echo "2. View individual pieces:"
echo "   meshlab ${RESULT_DIR}/piece_01_surface.ply"
echo ""
echo "3. Files generated in ${RESULT_DIR}:"
echo "   - assembly_all_pieces.ply (all pieces, colored)"
echo "   - piece_XX_surface.ply (individual surfaces)"
echo "   - piece_XX_edges.ply (individual edges, red)"
echo "   - PLY_VISUALIZATION_README.txt (guide)"
echo ""

echo "========================================================================"
echo "                          JOB COMPLETE                                  "
echo "========================================================================"
echo "End time: $(date)"
echo "Dataset: Tray-000 (40 pieces)"
echo "Algorithm: Original SFS++ legacy system"
echo "Results: ${RESULT_DIR}"
echo ""
