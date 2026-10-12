#!/bin/bash
# cauchy-scales ticket 06 (2026-10-12): E-6 Huber loss-shape diagnostic Arm H.
#
# DIAGNOSTIC ticket, not a fix: ONE kernel swap (Huber at solver-01 root
# scales), everything else frozen — tests whether divergence/seating is
# kernel-driven at all. No shape is adopted here whatever wins.
#
# Hunk: at exactly the three hardcoded pairwise sites (Icp, Registration
# 5-arg, Registration 6-arg) the four literals are RESTORED to the
# solver-01 root tree (dist 4.0 / norm 1.8 / axis 1.8 / rim 1.8, the E-0
# record — NOT the E-5 working tree) AND all four CauchyLoss are swapped
# to HuberLoss at those sites only. Kernel-only + restore in one hunk.
# IcpIncGraphAxis (env-tunable Cauchy) is the in-run control, untouched.
# multi_hypothesis_optimizer.cpp:887-890 untouched (dead by default).
# Dead IcpFine untouched. Weights, iteration counts, gates, solver
# options, caps, staging untouched (100 x5, 1000 x5).
#
# Rho'-comparability note: Huber has no Cauchy `a` with the same rho'
# curve — the `a` literal is kept numerically identical (4.0/1.8/1.8/1.8)
# so the swap is kernel-only, and the verdict compares rankings (DIVERGED
# count, accuracy, score-vs-accuracy split), never raw rho' across
# kernels. The LogE0 dump stays Cauchy-arithmetic; its numbers are
# scaffold health only, not cross-kernel evidence.
#
# Ticket 01 already established that the SFS_E0_DUMP instrumentation is
# outcome-neutral (its arm's run A vs run B agreed on every outcome line,
# job 32696452), so the dump can simply be enabled here.
#
# Self-contained batch arm: pull + marker verify + in-container rebuild of
# build_sif/Hierarchy-Clear + freshness check + one full Pot_A run with the
# dump enabled. Submit pinned:
#   sbatch --nodelist=spartan-bm065 hpc/sbatch/run_e6_huber.sbatch
# Node pinning is mandatory: every comparable number is bm065.
# Control cited, not re-run: solver-01 job 32624499, same node (per 02
# deviation; dump outcome-neutrality already proven, job 32696452 A vs B).
# Arm K (Tukey) is CONDITIONAL on Huber ranking surprising — do NOT run
# alongside or before Arm H.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_e6_huber

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear"
DUMPDIR=/data/gpfs/projects/punim2657/sfs_main/e6_dumps
SRC=class/reconstruction.cpp

echo "=== E-6 HUBER DIAGNOSTIC (Huber at solver-01 root 4.0/1.8/1.8/1.8) ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- pull (ff-only; diverged stops the job, never resolves here):"
git pull --ff-only 2>&1 | tail -n 3 || { echo "ERROR: pull failed -- STOP, report, do not force"; exit 1; }
echo "    HEAD now: $(git rev-parse HEAD)"

# ---------------------------------------------------------------- markers
# Huber at exactly the three pairwise sites with restored root scales.
echo "--- verify the Huber hunk is in the tree:"
grep -q "cauchy-scales ticket 06 Arm H" "${SRC}" || { echo "ERROR: ticket-06 marker absent -- stale checkout, aborting"; exit 1; }
[ "$(grep -c 'new ceres::HuberLoss(4.0)' "${SRC}")" -eq 3 ] || { echo "ERROR: expected 3 literal HuberLoss(4.0) (dist restored to root) -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'new ceres::HuberLoss(1.8)' "${SRC}")" -eq 9 ] || { echo "ERROR: expected 9 literal HuberLoss(1.8) (norm/axis/rim at three sites) -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(119.0)' "${SRC}")" -eq 0 ] || { echo "ERROR: CauchyLoss(119.0) still present at pairwise sites -- restore incomplete, aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(0.5)' "${SRC}")" -eq 0 ] || { echo "ERROR: CauchyLoss(0.5) still present at pairwise sites -- restore incomplete, aborting"; exit 1; }
echo "    three sites Huber at root scales intact"

echo "--- verify the two untouched sites are still untouched:"
[ "$(grep -c 'new ceres::CauchyLoss(cauchy_dist)' "${SRC}")" -eq 1 ] || { echo "ERROR: IcpIncGraphAxis no longer reads SFS_CAUCHY_DIST -- this arm must not touch that site"; exit 1; }
grep -q 'getenv("SFS_CAUCHY_DIST")' "${SRC}" || { echo "ERROR: SFS_CAUCHY_DIST gate missing -- IcpIncGraphAxis is supposed to stay the in-run control"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(5.0)' "${SRC}")" -eq 1 ] || { echo "ERROR: dead IcpFine no longer has its literal 5.0 -- nothing in this ticket may touch it"; exit 1; }
echo "    IcpIncGraphAxis still env-driven, dead IcpFine still 5.0"

echo "--- verify the dump calls pass the restored live values (4.0, 1.8, 1.8, 1.8):"
[ "$(grep -c 'LogE0Residuals("Icp", iter, 4.0, 1.8, 1.8, 1.8' "${SRC}")" -eq 1 ] || { echo "ERROR: Icp dump call not passing (4.0, 1.8, 1.8, 1.8) -- the dump's a column would lie"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration5", iter, 4.0, 1.8, 1.8, 1.8' "${SRC}")" -eq 1 ] || { echo "ERROR: Registration5 dump call not passing (4.0, 1.8, 1.8, 1.8) -- aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration6", iter, 4.0, 1.8, 1.8, 1.8' "${SRC}")" -eq 1 ] || { echo "ERROR: Registration6 dump call not passing (4.0, 1.8, 1.8, 1.8) -- aborting"; exit 1; }

echo "--- verify the control invariants are untouched (this hunk changes no value):"
[ "$(grep -c 'options.initial_trust_region_radius = 100;' "${SRC}")" -eq 5 ] || { echo "ERROR: solver-01 initial cap not at all 5 sites -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'options.max_trust_region_radius = 1000;' "${SRC}")" -eq 5 ] || { echo "ERROR: solver-01 max cap not at all 5 sites -- wrong tree, aborting"; exit 1; }
grep -q "SOLVER-01 2026-10-10" "${SRC}" || { echo "ERROR: solver-01 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "E3 REVERTED 2026-10-01" "${SRC}" || { echo "ERROR: weights marker absent -- this hunk must not move any weight"; exit 1; }
[ "$(grep -c 'max_iteration = 150, ceres_iteration = 100' "${SRC}")" -eq 3 ] || { echo "ERROR: iteration counts moved (expected 150/100 at the 3 live sites) -- wrong tree, aborting"; exit 1; }

echo "--- verify dataset selection is POT_A (compile-time):"
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    POT_A selected"

echo "--- rebuild build_sif/Hierarchy-Clear in-container:"
"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/e6_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "    build exit ${rc} (log: build_sif/e6_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/e6_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/e6_build_${SLURM_JOB_ID}.log" || true
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
mkdir -p "${DUMPDIR}" && rm -f "${DUMPDIR}"/dump_e0_*.tsv

echo ""
echo "=== E-6 run: Huber at root scales, SFS_E0_DUMP=${DUMPDIR} ==="
time "${APPTAINER}" exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    --bind "${DUMPDIR}:${DUMPDIR}" \
    --env "SFS_E0_DUMP=${DUMPDIR}" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_pota_e6_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "run exit ${rc} (not trusted alone)"

# ---------------------------------------------------------------- outcomes
# Kill criterion: Huber should show less far-init divergence but MORE
# false-positive convergence than Cauchy at the same `a`; Tukey the
# reverse. Predicted: 0/8 or worse with score at-or-above 262
# (false-positive convergence — the scorer's wrong-answer preference
# amplified), DIVERGED 0.
echo ""
echo "=== outcome lines (compare against the cited control) ==="
echo "    control, solver-01 job 32624499 same node:"
echo "    Sherd accuracy 2/8 (25.000%), Edge accuracy 1/15 (6.667%),"
echo "    best assembly score=262.000, PROFILE 783, DIVERGED-SOLVE 0,"
echo "    SURVIVED 14, FEATURE_MATCHING 1"
echo "--- this run:"
grep -E "Sherd accuracy|Shard accuracy|Edge accuracy|Best assembly" "e2e_pota_e6_${SLURM_JOB_ID}.log" | tail -n 6 || true
echo "--- PROFILE lines (control: 783):"
grep -cE "PROFILE" "e2e_pota_e6_${SLURM_JOB_ID}.log" || true
echo "--- DIVERGED-SOLVE (control: 0; read SEPARATELY from accuracy --"
echo "    this ticket's only divergence question: does Huber diverge where Cauchy held 0?):"
grep -cE "DIVERGED-SOLVE" "e2e_pota_e6_${SLURM_JOB_ID}.log" || true
echo "--- ground-truth pair survival (control: SURVIVED 14, FEATURE_MATCHING 1 -- must stay unchanged):"
grep -E "^  (SURVIVED|FEATURE_MATCHING):" "e2e_pota_e6_${SLURM_JOB_ID}.log" | tail -n 4 || true

# ---------------------------------------------------------------- dump read
# STOP (a) scaffold health: the dump's `a` column MUST show the restored
# values (dist 4.0 / norm 1.8 / axis 1.8 / rim 1.8) at every site/iter, or
# STOP. The LogE0 dump stays Cauchy-arithmetic: report its numbers as
# scaffold health only, NEVER as cross-kernel evidence, and never compare
# raw rho' across kernels. STOP (d): score vs accuracy read SEPARATELY.
echo ""
echo "=== dump census ==="
NDUMP=$(ls "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | wc -l)
echo "    dump files: ${NDUMP}"
echo "    site/iter histogram (iter 0 is the primary read):"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1{print $1"\t"$2}' | sort | uniq -c || true

echo ""
echo "=== SCAFFOLD HEALTH: a column must show restored values at iter 0 ==="
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1 && $2==0 {print $4"\t"$6}' | sort | uniq -c || true
echo "    (expect: dist 4.0, norm 1.8, axis 1.8, rim 1.8 -- anything else is STOP)"

exit ${rc}
