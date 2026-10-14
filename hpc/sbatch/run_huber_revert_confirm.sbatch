#!/bin/bash
# Ticket 07 HUBER-REVERT confirming run (2026-10-14): Huber→Cauchy at the
# three pairwise sites, scales frozen at solver-01 roots (4.0/1.8/1.8/1.8).
#
# Context: E-6 (Arm H, job 32756813) swapped Cauchy→Huber at the three
# pairwise solve sites (Icp, Registration 5-arg, Registration 6-arg) to
# test whether divergence is kernel-driven. Verdict (landed ticket 11 +
# S1): ranking NOT flipped → NOT kernel-driven, nothing adopted. But the
# HuberLoss hunk is still live in the tree — this ticket puts Cauchy back.
#
# One variable: kernel ONLY. Scales stay 4.0/1.8/1.8/1.8 (solver-01 root
# values, same numerals as the Huber live tree). LogE0Residuals dump calls
# stay (env-gated logging, not the kernel). IcpIncGraphAxis weights and
# everything else untouched.
#
# TRANSIENT in-job patch: HEAD still carries the Huber hunk (commit
# 34cf112); the laptop tree holds the identical revert uncommitted for
# lead review (no commits on this ticket). So this script pulls HEAD,
# applies the Huber→Cauchy patch in-job, rebuilds a SEPARATE binary
# (build_sif/Hierarchy-Clear-huberrevert), runs Pot_A, then restores
# class/reconstruction.cpp before exit — the checkout is left clean.
# Same pattern as run_row7_priority_off.sbatch. Submit pinned:
#   sbatch --nodelist=spartan-bm065 run_huber_revert_confirm.sbatch
# (rows 1-5, E-6 and row 7 all ran on bm065; same-node pairs comparable.)
#
# Must reproduce the settled baseline (post-row-7 / Eq.6-control on HEAD
# adba2d4): Sherd 0/8 + Edge 0/15, best ~260, DIVERGED 0, SURVIVED 29.
# If it does NOT reproduce, the tree differs from the settled one
# somewhere else — STOP and report, do not tune.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_huber_revert

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear-huberrevert"
LOCK="${ASM}/build_sif/.huberrevert_build.lock"
SRC="class/reconstruction.cpp"

echo "=== TICKET-07 HUBER REVERT (Huber->Cauchy, transient) ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- revert ticket-08 in-job JUGLET residue, then pull (ff-only):"
echo "    (retry loop: a parallel pull may hold the git index briefly)"
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

echo "--- verify the Huber live tree is what we expect (pre-patch):"
grep -q "cauchy-scales ticket 06 Arm H" "${SRC}" || { echo "ERROR: ticket-06 marker absent -- stale checkout, aborting"; exit 1; }
[ "$(grep -c 'new ceres::HuberLoss(4.0)' "${SRC}")" -eq 3 ] || { echo "ERROR: expected 3 HuberLoss(4.0) pre-patch -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'new ceres::HuberLoss(1.8)' "${SRC}")" -eq 9 ] || { echo "ERROR: expected 9 HuberLoss(1.8) pre-patch -- wrong tree, aborting"; exit 1; }
echo "    Huber live at 3 sites, root scales"

echo "--- verify the untouched sites are still untouched:"
[ "$(grep -c 'new ceres::CauchyLoss(cauchy_dist)' "${SRC}")" -eq 1 ] || { echo "ERROR: IcpIncGraphAxis no longer reads SFS_CAUCHY_DIST -- aborting"; exit 1; }
grep -q 'getenv("SFS_CAUCHY_DIST")' "${SRC}" || { echo "ERROR: SFS_CAUCHY_DIST gate missing -- aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(5.0)' "${SRC}")" -eq 1 ] || { echo "ERROR: dead IcpFine no longer has literal 5.0 -- aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Icp", iter, 4.0, 1.8, 1.8, 1.8' "${SRC}")" -eq 1 ] || { echo "ERROR: Icp dump call not passing (4.0, 1.8, 1.8, 1.8) -- aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration5", iter, 4.0, 1.8, 1.8, 1.8' "${SRC}")" -eq 1 ] || { echo "ERROR: Registration5 dump call not passing (4.0, 1.8, 1.8, 1.8) -- aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration6", iter, 4.0, 1.8, 1.8, 1.8' "${SRC}")" -eq 1 ] || { echo "ERROR: Registration6 dump call not passing (4.0, 1.8, 1.8, 1.8) -- aborting"; exit 1; }
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    control sites + dumps + POT_A selection intact"

echo "--- locked patch + rebuild (lock serialises source access):"
exec 9>"${LOCK}"
flock 9
cleanup() {
    echo "--- restoring reconstruction.cpp:"
    git -C "${ASM}" checkout -- "${SRC}"
    git -C "${ASM}" status --porcelain -- class/ | head -n 5 || true
    flock -u 9
    exec 9>&-
}
trap cleanup EXIT

sed -i 's/new ceres::HuberLoss(4\.0)/new ceres::CauchyLoss(4.0)/g; s/new ceres::HuberLoss(1\.8)/new ceres::CauchyLoss(1.8)/g' "${SRC}" || { echo "ERROR: sed patch failed -- aborting, tree restored by trap"; exit 1; }

echo "--- verify patch (kernel-only, scales identical):"
[ "$(grep -c 'HuberLoss' "${SRC}" || true)" = "0" ] || { echo "ERROR: HuberLoss still present after patch"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(4.0)' "${SRC}")" -eq 3 ] || { echo "ERROR: expected 3 CauchyLoss(4.0) post-patch -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(1.8)' "${SRC}")" -eq 9 ] || { echo "ERROR: expected 9 CauchyLoss(1.8) post-patch -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(cauchy_dist)' "${SRC}")" -eq 1 ] || { echo "ERROR: control site disturbed by patch"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(5.0)' "${SRC}")" -eq 1 ] || { echo "ERROR: dead IcpFine disturbed by patch"; exit 1; }
git diff --stat -- "${SRC}"
echo "    patch verified"

"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/huberrevert_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
cp -f "${ASM}/build_sif/Hierarchy-Clear" "${BIN}" || rc=1
echo "    build exit ${rc} (log: build_sif/huberrevert_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/huberrevert_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/huberrevert_build_${SLURM_JOB_ID}.log" || true
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
    "${BIN}" 8 > "e2e_pota_huberrevert_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

LOG="e2e_pota_huberrevert_${SLURM_JOB_ID}.log"
echo "--- settled baseline (post-row-7 / Eq.6-control on HEAD adba2d4):"
echo "    Sherd 0/8, Edge 0/15, best ~260, DIVERGED 0, SURVIVED 29"
echo "--- this run:"
grep -E "Sherd accuracy|Shard accuracy|Edge accuracy|Best assembly" "${LOG}" | tail -n 6 || true
echo "--- counts:"
echo "SURVIVED: $(grep -c SURVIVED "${LOG}" || true)"
echo "FEATURE_MATCHING: $(grep -c FEATURE_MATCHING "${LOG}" || true)"
echo "diverged: $(grep -ci diverged "${LOG}" || true)"
echo "DIVERGED-SOLVE: $(grep -cE 'DIVERGED-SOLVE' "${LOG}" || true)"
echo "veto: $(grep -c 'Opposing normals' "${LOG}" || true)"
echo "overlap: $(grep -c 'overlap=true' "${LOG}" || true)"
echo "profile-result: $(grep -c 'PROFILE VALIDATION.*RESULT' "${LOG}" || true)"
echo "profile-passed: $(grep -c 'PROFILE VALIDATION.*PASSED' "${LOG}" || true)"
echo "--- ground-truth pair survival lines:"
grep -E "^  (SURVIVED|FEATURE_MATCHING):" "${LOG}" | tail -n 6 || true
exit ${rc}
