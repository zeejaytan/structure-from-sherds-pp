#!/bin/bash
#SBATCH --job-name=sfs_relaxed_12hr
#SBATCH --time=12:00:00
#SBATCH --mem=32G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire
#SBATCH --output=sfs_relaxed_12hr_%j.out
#SBATCH --error=sfs_relaxed_12hr_%j.err

echo "========================================================================"
echo "   SFS++ LEGACY - RELAXED THRESHOLDS TEST (Tray-000, 40 pieces)      "
echo "========================================================================"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

echo "MODIFICATIONS:"
echo "  - Distance threshold: 20mm → 30mm"
echo "  - Normal threshold: 0.7 (46°) → 0.3 (73°)"
echo "  - Rejection reason logging enabled"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp

# Verify configuration
echo "=== Verifying Configuration ==="
if grep -q "^#define TRAY_000" class/data_path.h; then
    echo "✓ TRAY_000 is defined in data_path.h"
else
    echo "ERROR: TRAY_000 not defined in data_path.h"
    exit 1
fi

# Verify dataset
TRAY_DATA="/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp"
echo "Dataset: ${TRAY_DATA}"
echo "  Surfaces: $(ls ${TRAY_DATA}/Surfaces/Tray-000_Piece_*_Surface_0.xyz 2>/dev/null | wc -l)"
echo "  Breaklines: $(ls ${TRAY_DATA}/Breaklines/Tray-000_Piece_*_Breakline_0.pcd 2>/dev/null | wc -l)"
echo "  Axes: $(ls ${TRAY_DATA}/Axes/Tray-000_Piece_*_Axis.xyz 2>/dev/null | wc -l)"
echo ""

# Clean previous outputs
RESULT_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result"
echo "=== Cleaning previous results ==="
cd ${RESULT_DIR}
rm -f *.xyz *.txt 2>/dev/null
echo "✓ Cleaned result directory"
echo ""

echo "=== Running SFS++ with Relaxed Thresholds ==="
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp:/Dataset/SfS_pp" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    /workspace/build/Hierarchy-Clear-headless

ASSEMBLY_STATUS=$?

echo ""
echo "========================================================================"
if [ $ASSEMBLY_STATUS -eq 0 ]; then
    echo "                    ASSEMBLY COMPLETED"
    echo "========================================================================"
    echo ""

    # Count results
    echo "=== Assembly Results ==="
    SURFACES=$(ls ${RESULT_DIR}/*surface.xyz 2>/dev/null | wc -l)
    TRANSFORMS=$(ls ${RESULT_DIR}/T_*.txt 2>/dev/null | wc -l)
    PAIRWISE=$(ls ${RESULT_DIR}/T_*to_*.txt 2>/dev/null | grep -v "to_$(echo {} | sed 's/.*T_\([0-9]*\)to_\([0-9]*\).txt/\1/')" | wc -l)

    echo "  Assembled pieces: $SURFACES / 40"
    echo "  Total transformations: $TRANSFORMS"
    echo "  Pairwise connections: Looking for T_Xto_Y files..."

    # Count pairwise properly
    PAIRWISE=0
    for f in ${RESULT_DIR}/T_*.txt; do
        if [[ $f =~ T_([0-9]+)to_([0-9]+).txt ]]; then
            FROM="${BASH_REMATCH[1]}"
            TO="${BASH_REMATCH[2]}"
            if [ "$FROM" != "$TO" ]; then
                PAIRWISE=$((PAIRWISE + 1))
            fi
        fi
    done
    echo "  Pairwise connections (T_Xto_Y): $PAIRWISE"

    echo ""
    echo "=== Rejection Statistics Summary ==="
    echo "Analyzing [REJECT] messages for rejection causes..."
    grep "\[REJECT\]" sfs_relaxed_12hr_${SLURM_JOB_ID}.out | head -20 || echo "No rejection logs found"

else
    echo "                    ASSEMBLY FAILED"
    echo "========================================================================"
    exit 1
fi

echo ""
echo "End time: $(date)"
echo "========================================================================"
