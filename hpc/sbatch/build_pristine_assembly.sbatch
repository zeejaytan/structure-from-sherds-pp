#!/bin/bash
# Build the pristine assembly image (01-pristine-pot-a-run, O1).
# Upstream @ 9195c01, X11/Xvfb included, never sfspreproc.sif.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=04:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=pristine_build

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
MAIN=/data/gpfs/projects/punim2657/sfs_main
DEF="${MAIN}/sfspreproc-docker/hpc/containers/container-pristine-assembly.def"
SIF="${MAIN}/sfs_pristine_assembly.sif"

echo "=== PRISTINE BUILD ==="
echo "Job ${SLURM_JOB_ID}, $(date), node $(hostname)"
ls -l "${DEF}"
rm -f "${SIF}"

time "${APPTAINER}" build --fakeroot "${SIF}" "${DEF}" > "pristine_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "apptainer build exit ${rc}"
tail -n 15 "pristine_build_${SLURM_JOB_ID}.log" || true
ls -lh "${SIF}" || true
exit ${rc}
