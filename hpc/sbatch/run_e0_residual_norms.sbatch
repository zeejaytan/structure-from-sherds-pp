#!/bin/bash
# cauchy-scales ticket 01 (2026-10-11): E-0 residual measurement.
# Log-only instrumentation: one file-local helper in class/reconstruction.cpp
# called immediately before the four live Solve sites (Icp iters 0+1,
# Registration 5-arg iters 0+1, Registration 6-arg iters 0+1,
# IcpIncGraphAxis iter 0), env-gated on SFS_E0_DUMP. Nothing else changed --
# no scale, weight, iteration or gate value is touched, so the default path
# (env unset) must be outcome-identical to the control tree at HEAD 0a1f71f.
#
# This arm runs the SAME freshly built binary TWICE in one job:
#   run A (default path): SFS_E0_DUMP unset -> compared against the
#        solver-01 control numbers (same node, same inputs, same cap tree).
#   run B (dump path): SFS_E0_DUMP set -> dump_e0_*.tsv per call site.
# Same binary, two runs, so any difference is the instrumentation, not the
# build.
#
# Self-contained batch arm: pull + marker verify + in-container rebuild
# of build_sif/Hierarchy-Clear + freshness check + two full Pot_A runs.
# Submit pinned:
#   sbatch --nodelist=spartan-bm065 hpc/sbatch/run_e0_residual_norms.sbatch
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_e0_residuals

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear"
DUMPDIR=/data/gpfs/projects/punim2657/sfs_main/e0_dumps

echo "=== E-0 RESIDUAL NORMS: log-only measurement arm ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- pull (ff-only; diverged stops the job, never resolves here):"
git pull --ff-only 2>&1 | tail -n 3 || { echo "ERROR: pull failed -- STOP, report, do not force"; exit 1; }
echo "    HEAD now: $(git rev-parse HEAD)"

echo "--- verify the E-0 hunk is in the tree (marker at the 4 logged sites):"
grep -q "cauchy-scales ticket 01" class/reconstruction.cpp || { echo "ERROR: E-0 comment marker absent -- stale checkout, aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals(' class/reconstruction.cpp)" -eq 5 ] || { echo "ERROR: expected 5 LogE0Residuals occurrences (1 definition + 4 calls) -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Icp", iter' class/reconstruction.cpp)" -eq 1 ] || { echo "ERROR: Icp call missing -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration5", iter' class/reconstruction.cpp)" -eq 1 ] || { echo "ERROR: Registration 5-arg call missing -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("Registration6", iter' class/reconstruction.cpp)" -eq 1 ] || { echo "ERROR: Registration 6-arg call missing -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'LogE0Residuals("IcpIncGraphAxis", iter, cauchy_dist, cauchy_norm' class/reconstruction.cpp)" -eq 1 ] || { echo "ERROR: IcpIncGraphAxis call missing (or not using live cauchy values) -- wrong tree, aborting"; exit 1; }
echo "    E-0 helper + 4 call sites present"

echo "--- verify the control invariants are untouched (this hunk changes no value):"
[ "$(grep -c 'options.initial_trust_region_radius = 100;' class/reconstruction.cpp)" -eq 5 ] || { echo "ERROR: solver-01 initial cap not at all 5 sites -- wrong tree, aborting"; exit 1; }
[ "$(grep -c 'options.max_trust_region_radius = 1000;' class/reconstruction.cpp)" -eq 5 ] || { echo "ERROR: solver-01 max cap not at all 5 sites -- wrong tree, aborting"; exit 1; }
grep -q "SOLVER-01 2026-10-10" class/reconstruction.cpp || { echo "ERROR: solver-01 marker absent -- stale checkout, aborting"; exit 1; }

echo "--- verify dataset selection is POT_A (compile-time):"
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    POT_A selected"

echo "--- rebuild build_sif/Hierarchy-Clear in-container:"
"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/e0_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "    build exit ${rc} (log: build_sif/e0_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/e0_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/e0_build_${SLURM_JOB_ID}.log" || true
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
echo "=== run A: default path (SFS_E0_DUMP unset) -- must match control ==="
time "${APPTAINER}" exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_pota_e0_default_${SLURM_JOB_ID}.log" 2>&1
rcA=$?
echo "run A exit ${rcA} (not trusted alone)"

echo ""
echo "=== run B: dump path (SFS_E0_DUMP=${DUMPDIR}) ==="
time "${APPTAINER}" exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    --bind "${DUMPDIR}:${DUMPDIR}" \
    --env "SFS_E0_DUMP=${DUMPDIR}" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_pota_e0_dump_${SLURM_JOB_ID}.log" 2>&1
rcB=$?
echo "run B exit ${rcB} (not trusted alone)"

echo ""
echo "=== RUN A (default path) outcome lines ==="
grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_pota_e0_default_${SLURM_JOB_ID}.log" | tail -n 8 || true
echo "--- run A gate lines:"
grep -cE "PROFILE" "e2e_pota_e0_default_${SLURM_JOB_ID}.log" || true
echo "--- run A diverged (kill-criterion counter):"
grep -cE "DIVERGED-SOLVE" "e2e_pota_e0_default_${SLURM_JOB_ID}.log" || true

echo ""
echo "=== RUN B (dump path) outcome lines ==="
grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_pota_e0_dump_${SLURM_JOB_ID}.log" | tail -n 8 || true
echo "--- run B gate lines:"
grep -cE "PROFILE" "e2e_pota_e0_dump_${SLURM_JOB_ID}.log" || true
echo "--- run B diverged (kill-criterion counter):"
grep -cE "DIVERGED-SOLVE" "e2e_pota_e0_dump_${SLURM_JOB_ID}.log" || true

echo ""
echo "=== default-vs-dump outcome comparison (must be identical) ==="
OUT_A=$(grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_pota_e0_default_${SLURM_JOB_ID}.log")
OUT_B=$(grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_pota_e0_dump_${SLURM_JOB_ID}.log")
if [ "${OUT_A}" = "${OUT_B}" ]; then
    echo "PASS: default path and dump path agree on every outcome line"
else
    echo "FAIL: outcome lines differ -- default path is NOT behavior-neutral, STOP"
    diff <(printf '%s\n' "${OUT_A}") <(printf '%s\n' "${OUT_B}") || true
fi

echo ""
echo "=== dump census (per-point labels must be present) ==="
NDUMP=$(ls "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | wc -l)
echo "    dump files: ${NDUMP}"
for f in "${DUMPDIR}"/dump_e0_*.tsv; do
    [ -e "${f}" ] || continue
    echo "    $(basename "${f}") $(wc -l < "${f}") lines"
done
echo "    class histogram:"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1{print $4}' | sort | uniq -c | sort -rn || true
echo "    site histogram:"
cat "${DUMPDIR}"/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' 'NR>1{print $1"\t"$2}' | sort | uniq -c || true

# Report the worse binary exit code; the outcome lines above are the real gate.
if [ "${rcA}" -ne 0 ]; then exit "${rcA}"; fi
exit "${rcB}"
