#!/bin/bash
# Ticket 12 MALLOC_PERTURB pair (2026-10-03): heap-garbage determinism test.
#
# Same binary + same data as the standard E2E job, but MALLOC_PERTURB_=169
# fixes heap-fill values: uninitialized heap reads become DETERMINISTIC
# (same garbage every run) while threading behavior is untouched.
# Identical twice under perturb => uninitialized heap reads are (a)
# contributor; still different => threading or stack (not heap garbage).
# No code change; env only. Results dirs timestamped per minute as usual;
# submit the pair SEQUENTIALLY (second after first completes) to avoid
# results-dir collision, same as the standard pair protocol.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_e2e_perturb

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear"

echo "=== SFS E2E MALLOC_PERTURB RUN (ticket 12) ==="
echo "Job ${SLURM_JOB_ID}, $(date)"
[ -x "${BIN}" ] || { echo "ERROR: binary missing"; exit 1; }

cd /data/gpfs/projects/punim2657/sfs_main || exit 1

time "${APPTAINER}" exec \
    --env MALLOC_PERTURB_=169 \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_perturb_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"
grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_perturb_${SLURM_JOB_ID}.log" | tail -n 4 || true
exit ${rc}
