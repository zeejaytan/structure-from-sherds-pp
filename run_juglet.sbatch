#!/bin/bash
#SBATCH --job-name=juglet_sfs
#SBATCH --time=06:00:00
#SBATCH --mem=32G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire
#SBATCH --output=juglet_sfs_%j.out
#SBATCH --error=juglet_sfs_%j.err

# =====================================================================
# SFS++ Juglet assembly: 9 sherds, scan-table poses, millimetres.
# Ticket .scratch/juglet-sfs/issues/03-assemble.md (Answers: S1).
# Dataset block: JUGLET in class/data_path.h (SHARD_NUMBER 9, absolute
# bundle paths, REAL GT). Binary: Hierarchy-Clear (NOT -headless; the
# Tray script's suffix is stale).
# Container sfspreproc.sif does NOT take :/workspace binds -- /workspace is a
# site GPFS mount inside it, shadowing any bind (verified 2026-09-20, same
# failure mode as the preprocessing image). Bind /data:/data instead.
# =====================================================================

echo "========================================================================"
echo "        SFS++ JUGLET ASSEMBLY (9 pieces)                              "
echo "========================================================================"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker || exit 1

echo "=== Syncing code ==="
git pull --ff-only || { echo "ERROR: pull failed (diverged?)"; exit 1; }
echo ""

# Verify JUGLET is the active dataset block
echo "=== Verifying Configuration ==="
if grep -q "^#define JUGLET" class/data_path.h; then
    echo "JUGLET is defined in class/data_path.h"
else
    echo "ERROR: JUGLET not defined in class/data_path.h"
    grep "^#define POT\|^#define TRAY\|^#define JUGLET" class/data_path.h | head -5
    exit 1
fi
echo ""

# Verify bundle inputs (every missing file silently switches its shard OFF,
# so count everything before building)
echo "=== Verifying Juglet Bundle ==="
BUNDLE="/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp"
echo "Bundle: ${BUNDLE}"
for kind in "Surfaces/*_Surface_0.xyz:9" "Surfaces/*_Surface_1.xyz:9" \
            "Breaklines/*_Breakline_0.pcd:9" "Axes/*_Axis.xyz:9" \
            "Mesh/*.obj:9"; do
    pat="${kind%%:*}"; exp="${kind##*:}"
    got=$(ls ${BUNDLE}/${pat} 2>/dev/null | wc -l)
    echo "  ${pat}: ${got} (expected ${exp})"
    [ "${got}" -ne "${exp}" ] && { echo "ERROR: bundle incomplete"; exit 1; }
done
# Ground Truth lives under a dir with a space: count with find, not ls+glob.
got=$(find "${BUNDLE}/Ground Truth" -maxdepth 1 -name "*_T.txt" 2>/dev/null | wc -l)
echo "  Ground Truth/*_T.txt: ${got} (expected 9)"
[ "${got}" -ne 9 ] && { echo "ERROR: bundle incomplete"; exit 1; }
[ -f "${BUNDLE}/Ground Truth/Juglet_simple_graph.txt" ] || { echo "ERROR: GT graph missing"; exit 1; }
echo "  GT graph: present"
echo ""
echo "Sample coords (mm expected, vessel ~65 mm, scan-table offset ~500):"
head -n 1 ${BUNDLE}/Surfaces/Juglet_Piece_1_Surface_0.xyz
echo "Breakline points per piece (need >= 50 or the shard is skipped):"
for p in 1 2 3 4 5 6 7 8 9; do
    n=$(grep -c "^[0-9-]" ${BUNDLE}/Breaklines/Juglet_Piece_${p}_Breakline_0.pcd 2>/dev/null || echo 0)
    echo "  piece ${p}: ${n} data lines"
done
echo ""

echo "=== Building SFS++ (JUGLET) in container ==="
# NOTE (verified 2026-09-20): do NOT bind anything to /workspace -- inside
# this image it is already a site GPFS mount ("project"), so such binds are
# shadowed (same failure as the preprocessing image, bit 30694326). Bind
# /data:/data and address the checkout by its real path instead. The junk
# in the site /workspace (stale Jul-2025 CMakeCache etc.) is left alone.
APPTAINER="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
SIF="/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif"
ROOT="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker"
time ${APPTAINER} exec \
    --bind "/data:/data" \
    "${SIF}" \
    bash -c "cd ${ROOT} && rm -rf build && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 Hierarchy-Clear"
[ $? -ne 0 ] && { echo "ERROR: build failed"; exit 1; }
ls -lh build/Hierarchy-Clear || { echo "ERROR: binary missing"; exit 1; }
echo ""

echo "========================================================================"
echo "                  RUNNING SFS++ ON JUGLET DATASET                     "
echo "========================================================================"
echo "9 pieces: 36 pairs for feature matching, then beam-search assembly."
echo "Starting at: $(date)"
echo ""

time ${APPTAINER} exec \
    --bind "/data:/data" \
    --env SFS_AXIS_ANGLE_MAX="${SFS_AXIS_ANGLE_MAX:-}" \
    --env SFS_PC_DIST="${SFS_PC_DIST:-}" \
    --env SFS_PC_NORM="${SFS_PC_NORM:-}" \
    --env SFS_AXIS_WEIGHT="${SFS_AXIS_WEIGHT:-}" \
    --env SFS_LCS_ONLY="${SFS_LCS_ONLY:-}" \
    --env SFS_REJECT_DIST="${SFS_REJECT_DIST:-}" \
    --env SFS_REJECT_NORM="${SFS_REJECT_NORM:-}" \
    --env SFS_CAUCHY_DIST="${SFS_CAUCHY_DIST:-}" \
    --env SFS_CAUCHY_NORM="${SFS_CAUCHY_NORM:-}" \
    "${SIF}" \
    bash -c "cd ${ROOT}/build && ./Hierarchy-Clear"
RUN_STATUS=$?
echo ""
echo "Execution exit code: ${RUN_STATUS}"
echo "Finished at: $(date)"
echo ""

echo "========================================================================"
echo "                           RESULTS                                      "
echo "========================================================================"
RESULT_BASE=$(ls -td ${ROOT}/build/results_* 2>/dev/null | head -n 1)
echo "Latest results dir: ${RESULT_BASE}"
if [ -n "${RESULT_BASE}" ]; then
    echo "Generated files: $(ls "${RESULT_BASE}/Result" 2>/dev/null | wc -l)"
    ls "${RESULT_BASE}/Result" 2>/dev/null | head -n 30
    echo ""
    ACC=$(ls "${RESULT_BASE}/Result"/1.\ Acc.txt 2>/dev/null | head -n 1)
    if [ -n "${ACC}" ]; then
        echo "--- Accuracy report ---"
        cat "${ACC}"
        echo ""
    else
        echo "(no accuracy report found)"
    fi
else
    echo "ERROR: no results_* directory produced"
fi
echo ""
echo "--- Connection summary from log ---"
LOG="juglet_sfs_${SLURM_JOB_ID}.out"
[ -f "${LOG}" ] || LOG=$(ls -t juglet_sfs_*.out 2>/dev/null | head -n 1)
grep -aE "connections with inliers|INPUT DATA|Shard accuracy|Edge accuracy" "${LOG}" 2>/dev/null | tail -n 20 || true
echo ""
echo "End time: $(date)"
