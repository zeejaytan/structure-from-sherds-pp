#!/bin/bash
# E2E baseline 2026-10-01: full assembly on FRESH pipeline breaklines (Pot_A).
#
# Inputs: our current-code extraction (vote+guards+patches, convergence loop,
# live noise filter, 1.9mm resampling) staged at
#   /data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp/
# bound to /Dataset/SfS_pp, with the POT_A dataset block compiled in
# (data_path.h flip, E2E commit) and the build_sif/Hierarchy-Clear binary.
# Probe reads 15/15 on this exact bundle (e2e_fresh arm) — if the assembler
# finds nothing, the fault has moved downstream (ticket 07 handoff clause).
#
# Deliberately NO in-job rebuild: the binary is prebuilt and verified
# (strings-checked for the POT_A block). Rebuilding inside the batch job
# is how stale binaries reach results.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_e2e_pota_fresh

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear"

echo "=== SFS E2E BASELINE: fresh Pot_A breaklines, full assembly ==="
echo "Job ${SLURM_JOB_ID}, $(date)"
echo "--- input census (host side, trusted over container):"
for s in Breaklines Mesh Axes Surfaces "Ground Truth"; do
    echo "    $s: $(ls "${FRESH}/$s" | wc -l) files"
done
echo "--- binary:"
ls -lh "${BIN}"
if [ ! -x "${BIN}" ]; then echo "ERROR: binary missing/not executable"; exit 1; fi

cd /data/gpfs/projects/punim2657/sfs_main || exit 1

time "${APPTAINER}" exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_pota_fresh_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

echo "--- outcome lines:"
grep -E "Assembly Score|Pieces Assembled|Best State|accuracy" "e2e_pota_fresh_${SLURM_JOB_ID}.log" | tail -n 8 || true
exit ${rc}
