#!/bin/bash
# Solver-01 (2026-10-10): trust-region cap at all 5 Solve sites
# (initial=100, max=1000; defaults 1e4/1e16). One variable: the cap only.
# Control is the row-4 tree (veto DISABLED, rows 1-3 adopted).
# Kill criterion: diverged falls AND sane byte-identical AND no pair lost.
#
# Self-contained batch arm: pull + marker verify + in-container rebuild
# of build_sif/Hierarchy-Clear + freshness check + full Pot_A assembly on
# pota_e2e_fresh. Submit pinned:
#   sbatch --nodelist=spartan-bm065 hpc/sbatch/run_solver01_trust_cap.sbatch
# (rows 1-4 ran on bm065; same-node pairs are comparable, cross-node
# numbers are draws -- ticket juglet-sfs/12).
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_solver01_cap

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear"

echo "=== SOLVER-01: trust-region cap arm ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- pull (ff-only; diverged stops the job, never resolves here):"
git pull --ff-only 2>&1 | tail -n 3 || { echo "ERROR: pull failed -- STOP, report, do not force"; exit 1; }
echo "    HEAD now: $(git rev-parse HEAD)"

echo "--- verify the solver-01 change is in the tree (marker at all 5 sites):"
grep -q "SOLVER-01 2026-10-10" class/reconstruction.cpp || { echo "ERROR: solver-01 marker absent -- stale checkout, aborting"; exit 1; }
[ "$(grep -c 'options.initial_trust_region_radius = 100;' class/reconstruction.cpp)" -eq 5 ] || { echo "ERROR: initial cap not at all 5 sites -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'options.max_trust_region_radius = 1000;' class/reconstruction.cpp)" -eq 5 ] || { echo "ERROR: max cap not at all 5 sites -- wrong tree, aborting"; exit 1; }
grep -q "rej_dist = 20, rej_norm = 0.7;" class/reconstruction.cpp || { echo "ERROR: row-3 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "lowest_score > 1.5" class/feature_matching.cpp || { echo "ERROR: row-2 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "axis_angle_ > 0.610" class/feature_matching.cpp || { echo "ERROR: row-1 kept value missing -- wrong tree, aborting"; exit 1; }
if grep -q "else if (CheckOpposingNormals" class/feature_matching.cpp; then echo "ERROR: row-4 veto still live -- control tree must have it DISABLED, aborting"; exit 1; fi
echo "    solver-01 (cap x5) + row-3 (20/0.7) + row-2 (1.5) + row-1 (0.610) + veto DISABLED all present"

echo "--- verify dataset selection is POT_A (compile-time):"
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    POT_A selected"

echo "--- rebuild build_sif/Hierarchy-Clear in-container:"
"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/solver01_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "    build exit ${rc} (log: build_sif/solver01_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/solver01_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/solver01_build_${SLURM_JOB_ID}.log" || true
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
    "${BIN}" 8 > "e2e_pota_solver01_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

echo "--- outcome lines:"
grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_pota_solver01_${SLURM_JOB_ID}.log" | tail -n 8 || true
echo "--- gate lines:"
grep -cE "PROFILE" "e2e_pota_solver01_${SLURM_JOB_ID}.log" || true
echo "--- diverged (kill-criterion counter):"
grep -cE "DIVERGED-SOLVE" "e2e_pota_solver01_${SLURM_JOB_ID}.log" || true
exit ${rc}
