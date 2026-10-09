#!/bin/bash
# Ticket 12 ROW 3 (2026-10-10): correspondence RejectOutlier back to
# upstream (20, 0.7) at the three LIVE-path sites (was lineage 2.0/0.85).
#
# Sites: MakeSingleCorres :523, MakeMergeCorres :587, MakeMultiCorres
# env-default :654. Dead MakeCorWithSur (:733/:758, single caller dead
# IcpFine) deliberately untouched. SFS_REJECT_* env still overrides.
#
# Self-contained batch arm: pull + marker verify + in-container rebuild
# of build_sif/Hierarchy-Clear + freshness check + full Pot_A assembly on
# the SAME bundle as rows 1-2 (pota_e2e_fresh). Submit pinned:
#   sbatch --nodelist=spartan-bm065 hpc/sbatch/run_row3_reject_outlier.sbatch
# (rows 1-2 ran on bm065; same-node pairs are comparable, cross-node
# numbers are draws -- ticket juglet-sfs/12).
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_row3_reject2070

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear"

echo "=== TICKET-12 ROW 3: RejectOutlier (20, 0.7) arm ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- pull (ff-only; diverged stops the job, never resolves here):"
git pull --ff-only 2>&1 | tail -n 3 || { echo "ERROR: pull failed -- STOP, report, do not force"; exit 1; }
echo "    HEAD now: $(git rev-parse HEAD)"

echo "--- verify the row-3 change is in the tree (markers + values):"
grep -q "TICKET-12 ROW 3" class/reconstruction.cpp || { echo "ERROR: row-3 marker absent -- stale checkout, aborting"; exit 1; }
[ "$(grep -c 'RejectOutlier(cor_line, 20, 0.7);' class/reconstruction.cpp)" -ge 2 ] || { echo "ERROR: hardcoded sites not at 20/0.7 -- wrong tree, aborting"; exit 1; }
grep -q "rej_dist = 20, rej_norm = 0.7;" class/reconstruction.cpp || { echo "ERROR: env default not 20/0.7 -- wrong tree, aborting"; exit 1; }
grep -q "lowest_score > 1.5" class/feature_matching.cpp || { echo "ERROR: row-2 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "axis_angle_ > 0.610" class/feature_matching.cpp || { echo "ERROR: row-1 kept value missing -- wrong tree, aborting"; exit 1; }
echo "    row-3 (20/0.7 x3) + row-2 (1.5) + row-1 (0.610) all present"

echo "--- verify dataset selection is POT_A (compile-time):"
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    POT_A selected"

echo "--- rebuild build_sif/Hierarchy-Clear in-container:"
"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/row3_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "    build exit ${rc} (log: build_sif/row3_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/row3_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/row3_build_${SLURM_JOB_ID}.log" || true
    exit 1
fi

echo "--- freshness: binary must be newer than every source it was built from:"
STALE=$(find "${ASM}/class" "${ASM}/main_headless_correct.cpp" -newer "${BIN}" 2>/dev/null | head -n 5 || true)
if [ -n "${STALE}" ]; then
    echo "ERROR: binary older than sources (stale build):"
    echo "${STALE}"
    exit 1
fi
ls -lh "${BIN}"
[ -x "${BIN}" ] || { echo "ERROR: binary missing/not executable"; exit 1; }

echo "--- input census (host side, trusted over container):"
for s in Breaklines Mesh Axes Surfaces; do
    echo "    $s: $(ls "${FRESH}/$s" | wc -l) files"
done

cd /data/gpfs/projects/punim2657/sfs_main || exit 1

time "${APPTAINER}" exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_pota_row3_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

echo "--- outcome lines:"
grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_pota_row3_${SLURM_JOB_ID}.log" | tail -n 8 || true
echo "--- gate lines:"
grep -cE "PROFILE" "e2e_pota_row3_${SLURM_JOB_ID}.log" || true
echo "--- reject env:"
grep -E "\[GATE\] reject_dist" "e2e_pota_row3_${SLURM_JOB_ID}.log" | head -n 2 || true
exit ${rc}
