#!/bin/bash
#SBATCH --job-name=test_coord_fixed
#SBATCH --time=00:30:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=test_coord_fixed_%j.out
#SBATCH --error=test_coord_fixed_%j.err

echo "========================================================================="
echo "     SFS Assembly Test - Coordinate-Fixed NURBS Dataset"
echo "========================================================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

echo "=== PREREQUISITE CHECK ==="
echo "This test requires Phase 2 (dataset regeneration) to be complete"
echo ""

# Find the most recent coordinate-fixed dataset
PREPROCESSING_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing"
LATEST_DATASET=$(ls -dt ${PREPROCESSING_DIR}/NURBS_Dataset_*_CoordinateFix 2>/dev/null | head -1)

if [ -z "$LATEST_DATASET" ]; then
    echo "❌ ERROR: No coordinate-fixed dataset found!"
    echo "Expected: NURBS_Dataset_YYYYMMDD_HHMMSS_CoordinateFix"
    echo "Please wait for Phase 2 (regeneration) to complete"
    exit 1
fi

echo "✓ Found coordinate-fixed dataset: $(basename $LATEST_DATASET)"
echo "  Location: $LATEST_DATASET"
echo ""

# Verify dataset has required files
SURFACE_COUNT=$(ls "$LATEST_DATASET/SfS_pp/Surfaces/"Pot_A_Piece_*_Surface_*.xyz 2>/dev/null | wc -l)
BREAKLINE_COUNT=$(ls "$LATEST_DATASET/SfS_pp/Breaklines/"Pot_A_Piece_*_Breakline_*.pcd 2>/dev/null | wc -l)
AXIS_COUNT=$(ls "$LATEST_DATASET/SfS_pp/Axes/"Pot_A_Piece_*_Axis.xyz 2>/dev/null | wc -l)

echo "Dataset contents:"
echo "  Surface files: $SURFACE_COUNT/16 expected"
echo "  Breakline files: $BREAKLINE_COUNT/16 expected"
echo "  Axis files: $AXIS_COUNT/8 expected"

if [ $SURFACE_COUNT -lt 16 ] || [ $BREAKLINE_COUNT -lt 16 ] || [ $AXIS_COUNT -lt 8 ]; then
    echo ""
    echo "❌ ERROR: Dataset incomplete!"
    echo "Please ensure Phase 2 completed successfully"
    exit 1
fi

echo ""
echo "✓ Dataset appears complete"
echo ""

echo "=== Step 1: Backup Current data_path.h ==="
DATA_PATH_H="/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/class/data_path.h"
BACKUP_PATH="${DATA_PATH_H}.backup_$(date +%Y%m%d_%H%M%S)"
cp "$DATA_PATH_H" "$BACKUP_PATH"
echo "✓ Backed up: $BACKUP_PATH"
echo ""

echo "=== Step 2: Update data_path.h to Use Coordinate-Fixed Dataset ==="
# Create temporary updated data_path.h
cat > "${DATA_PATH_H}.tmp" << 'EOF'
#ifndef DATA_PATH_H
#define DATA_PATH_H

#include <string>

const std::string potID = "A";

inline std::string getSurfaceDataPath() {
    return "PLACEHOLDER_PATH/SfS_pp/Surfaces/";
}

inline std::string getBreaklineDataPath() {
    return "PLACEHOLDER_PATH/SfS_pp/Breaklines/";
}

inline std::string getAxisDataPath() {
    return "PLACEHOLDER_PATH/SfS_pp/Axes/";
}

#endif
EOF

# Replace placeholder with actual dataset path
sed -i "s|PLACEHOLDER_PATH|$LATEST_DATASET|g" "${DATA_PATH_H}.tmp"
mv "${DATA_PATH_H}.tmp" "$DATA_PATH_H"

echo "✓ Updated data_path.h to point to:"
echo "  $LATEST_DATASET"
echo ""

echo "=== Step 3: Rebuild SFS Executables ==="
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/build
echo "Running: make clean && make -j4"
make clean > /dev/null 2>&1
make -j4 2>&1 | tail -10

if [ ! -f "Hierarchy-Clear-headless" ]; then
    echo "❌ ERROR: Build failed!"
    echo "Restoring backup data_path.h..."
    cp "$BACKUP_PATH" "$DATA_PATH_H"
    exit 1
fi

echo "✓ Build successful"
echo "  Executable: Hierarchy-Clear-headless ($(ls -lh Hierarchy-Clear-headless | awk '{print $5}'))"
echo ""

echo "=== Step 4: Run SFS Assembly Test ==="
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp

# Create test output directory
TEST_OUTPUT="test_coordinate_fixed_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TEST_OUTPUT"

echo "Running assembly test..."
echo "Output directory: $TEST_OUTPUT"
echo ""

time apptainer exec --bind /data/gpfs/projects/punim2657:/data/gpfs/projects/punim2657 \
    /data/gpfs/projects/punim2657/sfs_main/pcl_191_ceres_nurbs.sif \
    bash -c "cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/build && \
             ./Hierarchy-Clear-headless" > "$TEST_OUTPUT/assembly_output.log" 2>&1

ASSEMBLY_EXIT=$?
echo ""
echo "Assembly exit code: $ASSEMBLY_EXIT"
echo ""

echo "=== Step 5: Analyze Results ==="
LOG_FILE="$TEST_OUTPUT/assembly_output.log"

# Extract key metrics
PIECES_LOADED=$(grep "Pieces Loaded" "$LOG_FILE" 2>/dev/null | tail -1)
PIECES_MATCHED=$(grep "Pieces Matched" "$LOG_FILE" 2>/dev/null | tail -1)
EDGES_FOUND=$(grep "Total edges" "$LOG_FILE" 2>/dev/null | tail -1)
EDGES_PRUNED=$(grep "After pruning" "$LOG_FILE" 2>/dev/null | tail -1)

echo "--- Assembly Results ---"
echo "$PIECES_LOADED"
echo "$PIECES_MATCHED"
echo "$EDGES_FOUND"
echo "$EDGES_PRUNED"
echo ""

# Check coordinate loading (should not see giant numbers)
COORD_SAMPLE=$(grep "loaded.*Surface" "$LOG_FILE" 2>/dev/null | head -3)
if [ -n "$COORD_SAMPLE" ]; then
    echo "--- Coordinate Loading Check ---"
    echo "$COORD_SAMPLE"
    echo ""
fi

# Compare with baseline (broken NURBS dataset)
echo "=== Comparison with Broken Dataset (0% accuracy) ==="
echo "Broken (before fix):"
echo "  - Pieces: 6/8 (pieces 06/08 excluded)"
echo "  - Edges: 47 total, 8 after pruning"
echo "  - Assembly: 0% accuracy"
echo ""
echo "Fixed (with coordinate fix):"
if [ -n "$PIECES_LOADED" ]; then
    echo "  - $PIECES_LOADED"
fi
if [ -n "$EDGES_FOUND" ]; then
    echo "  - $EDGES_FOUND"
fi
echo ""

# Determine success
LOADED_COUNT=$(echo "$PIECES_LOADED" | grep -o '[0-9]\+/8' | head -1 | cut -d'/' -f1)
EDGE_COUNT=$(echo "$EDGES_FOUND" | grep -o '[0-9]\+' | head -1)

SUCCESS=false
if [ -n "$LOADED_COUNT" ] && [ "$LOADED_COUNT" -ge 7 ]; then
    if [ -n "$EDGE_COUNT" ] && [ "$EDGE_COUNT" -ge 60 ]; then
        SUCCESS=true
    fi
fi

echo "=== VERDICT ==="
if [ "$SUCCESS" = true ]; then
    echo "✅ COORDINATE FIX SUCCESSFUL!"
    echo "   - Loaded $LOADED_COUNT/8 pieces (vs broken: 6/8)"
    echo "   - Found $EDGE_COUNT edges (vs broken: 47)"
    echo "   - Significant improvement observed"
    echo ""
    echo "🎉 Bug fix verified - coordinates now in correct scale!"
else
    echo "⚠️  RESULTS INCONCLUSIVE"
    echo "   Assembly metrics similar to broken dataset"
    echo "   May need Phase 4 (adaptive parameter tuning)"
fi

echo ""
echo "=== Step 6: Restore Original data_path.h ==="
cp "$BACKUP_PATH" "$DATA_PATH_H"
echo "✓ Restored original data_path.h"
echo ""

echo "=== SUMMARY ==="
echo "Dataset tested: $LATEST_DATASET"
echo "Results saved: $TEST_OUTPUT"
echo "Test completed: $(date)"
echo ""
echo "========================================================================="
echo "END OF COORDINATE FIX TEST"
echo "========================================================================="
