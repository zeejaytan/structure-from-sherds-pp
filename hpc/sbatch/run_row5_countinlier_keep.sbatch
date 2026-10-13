#!/bin/bash
# Ticket 12 ROW 5 KEEP arm (2026-10-13): CountInlier adaptive segment
# contribution AS-IS (lineage "ROBUSTNESS FIX", feature_matching.cpp
# CountInlier overloads 1&2). Disable-vs-keep test vs
# run_row5_countinlier_off.sbatch: the adaptive gate is suspected DEAD
# (live path uses overload 3, which has no gate) -- if the pair is
# identical, the adaptive code goes (adopt upstream uniform, Row-2 rule).
#
# One variable: nothing changed in-tree for this arm (HEAD 26db0e5).
# Rows 1-4 values stay (0.610 / 1.5 / 20,0.7 / veto disabled).
#
# Self-contained batch arm: revert ticket-08 JUGLET residue + pull +
# marker verify + locked in-container rebuild of a SEPARATE binary
# (build_sif/Hierarchy-Clear-row5keep, shared binary untouched) +
# freshness check + full Pot_A assembly on the SAME bundle as rows 1-4
# (pota_e2e_fresh). Submit pinned:
#   sbatch --nodelist=spartan-bm065 run_row5_countinlier_keep.sbatch
# (rows 1-4 ran on bm065; same-node pairs are comparable.)
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_row5_keep

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear-row5keep"
LOCK="${ASM}/build_sif/.row5_build.lock"

echo "=== TICKET-12 ROW 5 KEEP arm (adaptive as-is) ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- revert ticket-08 in-job JUGLET residue, then pull (ff-only):"
git checkout -- class/data_path.h || { echo "ERROR: checkout data_path.h failed"; exit 1; }
git pull --ff-only 2>&1 | tail -n 3 || { echo "ERROR: pull failed -- STOP, report, do not force"; exit 1; }
echo "    HEAD now: $(git rev-parse HEAD)"

echo "--- verify rows 1-4 adopted values + row-5 KEEP precondition:"
grep -q "ROBUSTNESS FIX: Adaptive segment contribution" class/feature_matching.cpp || { echo "ERROR: adaptive gate absent -- wrong tree, aborting"; exit 1; }
grep -q "TICKET-12 ROW 4" class/feature_matching.cpp || { echo "ERROR: row-4 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "rej_dist = 20, rej_norm = 0.7;" class/reconstruction.cpp || { echo "ERROR: row-3 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "lowest_score > 1.5" class/feature_matching.cpp || { echo "ERROR: row-2 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "axis_angle_ > 0.610" class/feature_matching.cpp || { echo "ERROR: row-1 kept value missing -- wrong tree, aborting"; exit 1; }
echo "    adaptive present + rows 1-4 values present"

echo "--- verify dataset selection is POT_A (compile-time):"
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    POT_A selected"

echo "--- locked rebuild (serialises with OFF arm sharing this tree):"
exec 9>"${LOCK}"
flock 9
"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/row5keep_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
cp -f "${ASM}/build_sif/Hierarchy-Clear" "${BIN}" || rc=1
flock -u 9
exec 9>&-
echo "    build exit ${rc} (log: build_sif/row5keep_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/row5keep_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/row5keep_build_${SLURM_JOB_ID}.log" || true
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
    "${BIN}" 8 > "e2e_pota_row5keep_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

LOG="e2e_pota_row5keep_${SLURM_JOB_ID}.log"
echo "--- outcome lines:"
grep -E "Sherd accuracy|Edge accuracy|Best assembly" "${LOG}" | tail -n 5 || true
echo "--- counts:"
echo "SURVIVED: $(grep -c SURVIVED "${LOG}" || true)"
echo "diverged: $(grep -ci diverged "${LOG}" || true)"
echo "veto: $(grep -c 'Opposing normals' "${LOG}" || true)"
echo "overlap: $(grep -c 'overlap=true' "${LOG}" || true)"
echo "profile-result: $(grep -c 'PROFILE VALIDATION.*RESULT' "${LOG}" || true)"
echo "profile-passed: $(grep -c 'PROFILE VALIDATION.*PASSED' "${LOG}" || true)"
exit ${rc}
