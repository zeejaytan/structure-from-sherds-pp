#!/bin/bash
# Ticket 02 Eq.6 CONTROL arm (2026-10-13): cap-only 01-tree lineage at HEAD
# (rows 1-7 + 09 landed, currently 6a6fb8d). No source patch: the global
# pregraph-freeze loop in IcpIncGraphAxis (class/reconstruction.cpp
# ~:2186-2193, SetParameterBlockConstant(s[i])/(trans[i])) stays AS-IS.
# Pairs with run_eq6_freeall_eq6.sbatch (transient free-all hunk); the only
# variable between arms is freeze vs free-all at the global step.
# Pairwise sites + cap (100/1000) + guard untouched in both arms.
#
# Self-contained batch arm: revert ticket-08 JUGLET residue + pull +
# marker verify + locked in-container rebuild of a SEPARATE binary
# (build_sif/Hierarchy-Clear-eq6ctl, shared binary untouched) +
# freshness check + full Pot_A assembly on the SAME bundle as rows 1-7
# (pota_e2e_fresh). Submit pinned:
#   sbatch --nodelist=spartan-bm065 run_eq6_freeall_control.sbatch
# (rows 1-7 ran on bm065; same-node pairs are comparable.)
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_eq6_ctl

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear-eq6ctl"
LOCK="${ASM}/build_sif/.eq6_build.lock"

echo "=== TICKET-02 EQ.6 CONTROL arm (freeze as-is, no patch) ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- revert ticket-08 in-job JUGLET residue, then pull (ff-only):"
echo "    (retry loop: EQ6 arm may hold the git index briefly at start)"
for i in $(seq 1 30); do
  git checkout -- class/data_path.h 2>/dev/null \
    && git pull --ff-only 2>&1 | tail -n 3 \
    && break
  echo "    git busy (try $i/30), waiting 20s..."
  sleep 20
  [ "$i" = "30" ] && { echo "ERROR: git still locked -- STOP, report, do not force"; exit 1; }
done
echo "    HEAD now: $(git rev-parse HEAD)"
test -z "$(git status --porcelain -- class/ | head -n 3)" || { echo "ERROR: tree dirty after pull:"; git status --porcelain -- class/ | head; exit 1; }

echo "--- verify rows 1-7 + 09 adopted values + Eq.6 CONTROL precondition:"
grep -q "TICKET-12 ROW 5" class/feature_matching.cpp || { echo "ERROR: row-5 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "TICKET-12 ROW 4" class/feature_matching.cpp || { echo "ERROR: row-4 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "rej_dist = 20, rej_norm = 0.7;" class/reconstruction.cpp || { echo "ERROR: row-3 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "lowest_score > 1.5" class/feature_matching.cpp || { echo "ERROR: row-2 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "axis_angle_ > 0.610" class/feature_matching.cpp || { echo "ERROR: row-1 kept value missing -- wrong tree, aborting"; exit 1; }
grep -q "TICKET-12 ROW 7" class/ranking_system.cpp || { echo "ERROR: row-7 marker absent -- stale checkout, aborting"; exit 1; }
test "$(grep -c 'combined_score = 0.7' class/ranking_system.cpp || true)" = "0" || { echo "ERROR: row-7 adopted delete missing (scorer still present) -- wrong tree, aborting"; exit 1; }
test "$(grep -c 'problem.SetParameterBlockConstant(s\[i\]);' class/reconstruction.cpp || true)" = "1" || { echo "ERROR: global freeze loop absent -- wrong tree, aborting"; exit 1; }
test "$(grep -c 'SetParameterBlockVariable' class/reconstruction.cpp || true)" = "0" || { echo "ERROR: unexpected free-all residue -- dirty tree, aborting"; exit 1; }
echo "    rows 1-7 + 09 present, freeze loop present, no free-all residue"

echo "--- verify dataset selection is POT_A (compile-time):"
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    POT_A selected"

echo "--- locked rebuild (serialises with EQ6 arm sharing this tree):"
exec 9>"${LOCK}"
flock 9
"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/eq6ctl_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
cp -f "${ASM}/build_sif/Hierarchy-Clear" "${BIN}" || rc=1
flock -u 9
exec 9>&-
echo "    build exit ${rc} (log: build_sif/eq6ctl_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/eq6ctl_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/eq6ctl_build_${SLURM_JOB_ID}.log" || true
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
    "${BIN}" 8 > "e2e_pota_eq6ctl_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

LOG="e2e_pota_eq6ctl_${SLURM_JOB_ID}.log"
echo "--- outcome lines:"
grep -E "Sherd accuracy|Edge accuracy|Best assembly" "${LOG}" | tail -n 5 || true
echo "--- counts:"
echo "SURVIVED: $(grep -c SURVIVED "${LOG}" || true)"
echo "diverged: $(grep -ci diverged "${LOG}" || true)"
echo "veto: $(grep -c 'Opposing normals' "${LOG}" || true)"
echo "overlap: $(grep -c 'overlap=true' "${LOG}" || true)"
echo "profile-result: $(grep -c 'PROFILE VALIDATION.*RESULT' "${LOG}" || true)"
echo "profile-passed: $(grep -c 'PROFILE VALIDATION.*PASSED' "${LOG}" || true)"
echo "pair-1-4-lines: $(grep -c '1-4' "${LOG}" || true)"
echo "GT: $(grep -E 'GT|ground.?truth' "${LOG}" | tail -n 5 || true)"
exit ${rc}
