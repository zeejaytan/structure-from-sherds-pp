#!/bin/bash
# cauchy-scales ticket 04 (2026-10-12): E-4 tight normal scale Arm T.
#
# BEHAVIOUR-CHANGE ticket. The normal term's scale is now the fixed literal
# 0.5 (tight arm) at the three hardcoded pairwise sites, weights frozen, so
# the DEFAULT path IS the experimental path: no env gate, no second run.
# Distance stays at the E-3 fixed literal 119.0 at all three sites; axis/rim
# keep 1.8. Ticket 01 already established that the SFS_E0_DUMP
# instrumentation is outcome-neutral (its arm's run A vs run B agreed on
# every outcome line, job 32696452), so the dump can simply be enabled here.
#
# Applied at the three hardcoded normal sites only: Icp, Registration
# 5-arg, Registration 6-arg. NOT applied at IcpIncGraphAxis (env-tunable
# cauchy_norm, kept as in-run control) or the dead IcpFine (no callers).
# multi_hypothesis_optimizer.cpp:888-890 untouched (dead by default).
#
# Self-contained batch arm: pull + marker verify + in-container rebuild of
# build_sif/Hierarchy-Clear + freshness check + one full Pot_A run with the
# dump enabled. Submit pinned:
#   sbatch --nodelist=spartan-bm065 hpc/sbatch/run_e4_norm_tight.sbatch
# Node pinning is mandatory: every comparable number is bm065.
# Control cited, not re-run: solver-01 job 32624499, same node (per 02
# deviation; dump outcome-neutrality already proven, job 32696452 A vs B).
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_e4_norm_tight

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear"
DUMPDIR=/data/gpfs/projects/punim2657/sfs_main/e4_dumps
SRC=class/reconstruction.cpp

echo "=== E-4 TIGHT NORMAL (a_norm = 0.5): behaviour-change arm ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- pull (ff-only; diverged stops the job, never resolves here):"
git pull --ff-only 2>&1 | tail -n 3 || { echo "ERROR: pull failed -- STOP, report, do not force"; exit 1; }
echo "    HEAD now: $(git rev-parse HEAD)"

# ---------------------------------------------------------------- markers
# The tight 0.5 must be present at exactly the three norm sites; dist stays
# fixed at 119.0 from E-3; axis/rim keep 1.8 (6 = 2 classes x 3 sites).
echo "--- verify the tight normal scale is in the tree:"
grep -q "cauchy-scales ticket 04 Arm T" "${SRC}" || { echo "ERROR: ticket-04 marker absent -- stale checkout, aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(0.5)' "${SRC}")" -eq 3 ] || { echo "ERROR: expected 3 literal CauchyLoss(0.5) (one norm site each) -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(119.0)' "${SRC}")" -eq 3 ] || { echo "ERROR: expected 3 literal CauchyLoss(119.0) (dist fixed from E-3) -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(1.8)' "${SRC}")" -eq 6 ] || { echo "ERROR: expected 6 literal CauchyLoss(1.8) (axis/rim x3 sites) -- wrong tree, aborting"; exit 1; }
echo "    three norm sites at 0.5, dist still 119.0, axis/rim 6 x 1.8 intact"

echo "--- verify the two untouched sites are still untouched:"
[ "$(grep -c 'new ceres::CauchyLoss(cauchy_dist)' "${SRC}")" -eq 1 ] || { echo "ERROR: IcpIncGraphAxis no longer reads SFS_CAUCHY_DIST -- this arm must not touch that site"; exit 1; }
grep -q 'getenv("SFS_CAUCHY_DIST")' "${SRC}" || { echo "ERROR: SFS_CAUCHY_DIST gate missing -- IcpIncGraphAxis is supposed to stay the in-run control"; exit 1; }
[ "$(grep -c 'new ceres::CauchyLoss(5.0)' "${SRC}")" -eq 1 ] || { echo "ERROR: dead IcpFine no longer has its literal 5.0 -- nothing in this ticket may touch it"; exit 1; }
echo "    IcpIncGraphAxis still env-driven, dead IcpFine still 5.0"

echo "--- verify the dump calls pass the live values (dist 119.0, norm 0.5):"
[ "$(grep -c 'LogE0Residuals("Icp", iter, 119.0, 0.5,' "${SRC}")" -eq 1 ] || { echo "ERROR: Icp dump call not passing (119.0, 0.5) -- the dump's a column would lie"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration5", iter, 119.0, 0.5,' "${SRC}")" -eq 1 ] || { echo "ERROR: Registration5 dump call not passing (119.0, 0.5) -- aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration6", iter, 119.0, 0.5,' "${SRC}")" -eq 1 ] || { echo "ERROR: Registration6 dump call not passing (119.0, 0.5) -- aborting"; exit 1; }

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
    > "${ASM}/build_sif/e4_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "    build exit ${rc} (log: build_sif/e4_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/e4_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/e4_build_${SLURM_JOB_ID}.log" || true
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
echo "=== E-4 run: tight 0.5 in place, SFS_E0_DUMP=${DUMPDIR} ==="
time "${APPTAINER}" exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    --bind "${DUMPDIR}:${DUMPDIR}" \
    --env "SFS_E0_DUMP=${DUMPDIR}" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_pota_e4_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "run exit ${rc} (not trusted alone)"

# ---------------------------------------------------------------- outcomes
echo ""
echo "=== outcome lines (compare against the cited control) ==="
echo "    control, solver-01 job 32624499 same node:"
echo "    Sherd accuracy 2/8 (25.000%), Edge accuracy 1/15 (6.667%),"
echo "    PROFILE 783, DIVERGED-SOLVE 0, best assembly 7/8 pieces score=262.000"
echo "--- this run:"
grep -E "Sherd accuracy|Shard accuracy|Edge accuracy|Best assembly" "e2e_pota_e4_${SLURM_JOB_ID}.log" | tail -n 6 || true
echo "--- PROFILE lines (control: 783):"
grep -cE "PROFILE" "e2e_pota_e4_${SLURM_JOB_ID}.log" || true
echo "--- DIVERGED-SOLVE (control: 0; anything above 0 is a STOP):"
grep -cE "DIVERGED-SOLVE" "e2e_pota_e4_${SLURM_JOB_ID}.log" || true
echo "--- ground-truth pair survival (control: SURVIVED 14, FEATURE_MATCHING 1):"
grep -E "^  (SURVIVED|FEATURE_MATCHING):" "e2e_pota_e4_${SLURM_JOB_ID}.log" | tail -n 4 || true
echo "--- pair 1-4 still solves (control: 1-4 SURVIVED feature matching):"
grep -E "Pieces 1-4 @" "e2e_pota_e4_${SLURM_JOB_ID}.log" | tail -n 2 || true

# ---------------------------------------------------------------- dump read
# Primary read: the norm class' s/a^2 and rho' at iteration 0. E-0 measured
# median entry s ~ 15.2 (iter-0 s/a^2 p50 4.07-5.40 at a=1.8). At a=0.5 the
# arithmetic prediction is s/a^2 p50 ~ 61 and rho' p50 ~ 0.016 -- normals
# ~fully suppressed at entry, solve proceeds on axis/P2L/rim. Landing far
# off prediction means STOP, report, do not tune.
echo ""
echo "=== dump census ==="
NDUMP=$(ls "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | wc -l)
echo "    dump files: ${NDUMP}"
echo "    site/iter histogram (iter 0 is the primary read):"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1{print $1"\t"$2}' | sort | uniq -c || true

echo ""
echo "=== PRIMARY READ: norm class, iteration 0, per site ==="
echo "    a actually used (every site at iter 0 MUST be 0.5 for norm):"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1 && $2==0 && $4=="norm"{print $1"\t"$6}' | sort | uniq -c | sort -k2 || true
echo "    norm s/a^2 percentiles at iteration 0 (arithmetic prediction at a=0.5: p50 ~61):"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' '
  NR>1 && $2==0 && $4=="norm"{
    n[$1]++; v[$1]=v[$1]" "$8
  }
  END{
    for (s in n) { split(v[s], a, " "); asort(a); delete a[1];
      k=int((n[s]-1)*0.5)+1; k90=int((n[s]-1)*0.9)+1;
      printf "    %-16s n=%-7d p50=%-11.4g p90=%-11.4g max=%-11.4g\n", s, n[s], a[k], a[k90], a[n[s]] }
  }' || true
echo "    norm rho-prime percentiles at iteration 0 (arithmetic prediction at a=0.5: p50 ~0.016):"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' '
  NR>1 && $2==0 && $4=="norm"{
    n[$1]++; v[$1]=v[$1]" "$9
  }
  END{
    for (s in n) { split(v[s], a, " "); asort(a); delete a[1];
      k=int((n[s]-1)*0.5)+1; k90=int((n[s]-1)*0.9)+1;
      printf "    %-16s n=%-7d p50=%-11.4g p90=%-11.4g max=%-11.4g\n", s, n[s], a[k], a[k90], a[n[s]] }
  }' || true

echo ""
echo "=== secondary: dist/axis/rim at iteration 0 (dist keeps 119.0, axis/rim keep 1.8) ==="
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1 && $2==0 {print $4"\t"$6}' | sort | uniq -c || true

echo ""
echo "=== optional diagnostic (non-gating): max raw_norm for norm class at iter 0 ==="
echo "    (stored PCD normals are read without normalizing; non-unit normals may feed the tail)"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1 && $2==0 && $4=="norm"{if($11+0>m[$1])m[$1]=$11+0}END{for(s in m)printf "    %-16s max_raw_norm=%-11.4g\n", s, m[s]}' || true

exit ${rc}
