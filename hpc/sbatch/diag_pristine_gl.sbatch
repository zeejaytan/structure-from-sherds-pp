#!/bin/bash
# Diagnose pristine viewer-loop crash under Xvfb (01-pristine-pot-a-run, O4).
# Three GL env variants in ONE job; success = "Feature matching" marker
# (proves the axis-alignment render loop survived). No source changes.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --job-name=pristine_gl_diag

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF="${MAIN}/sfs_pristine_assembly.sif"
DATASET="${MAIN}/original_samples/SfS_pp"
RUNDIR="${MAIN}/pristine_gldiag_${SLURM_JOB_ID}"
mkdir -p "${RUNDIR}"

echo "=== GL DIAG ==="
echo "Job ${SLURM_JOB_ID}, $(date), node $(hostname)"

"${APPTAINER}" exec \
    --bind "${MAIN}:/workspace" \
    --bind "${DATASET}:/Dataset/SfS_pp" \
    "${SIF}" \
    bash -c '
set -u
RUNDIR="/workspace/pristine_gldiag_'"${SLURM_JOB_ID}"'"
export DISPLAY=:99
mkdir -p /tmp/.X11-unix && chmod 1777 /tmp/.X11-unix
Xvfb :99 -screen 0 1400x900x24 >"${RUNDIR}/xvfb.log" 2>&1 &
sleep 5
xdpyinfo -display :99 >"${RUNDIR}/xdpyinfo.log" 2>&1 || { echo "Xvfb unhealthy"; exit 1; }
try_variant() {
    VNAME="$1"; VLOG="${RUNDIR}/variant_${VNAME}.log"
    echo "--- variant ${VNAME}: ${EXTRA_ENV:-<default>} ---"
    (
        export LIBGL_ALWAYS_SOFTWARE=1 ${EXTRA_ENV:-}
        /opt/sfs-pristine/build/Hierarchy-Clear 8 >"${VLOG}" 2>&1 &
        PID=$!
        sleep 150
        if kill -0 ${PID} 2>/dev/null; then
            if grep -q "Feature matching" "${VLOG}"; then echo "variant ${VNAME}: ALIVE + marker PRESENT"; else echo "variant ${VNAME}: ALIVE, marker absent"; fi
            kill ${PID}; sleep 3; kill -9 ${PID} 2>/dev/null || true
        else
            wait ${PID}; echo "variant ${VNAME}: DIED exit $?"
        fi
        grep -c "was deprecated" "${VLOG}" || true
        tail -n 3 "${VLOG}" || true
    )
}
EXTRA_ENV="" try_variant baseline
EXTRA_ENV="LIBGL_ALWAYS_INDIRECT=1" try_variant indirect
EXTRA_ENV="GALLIUM_DRIVER=llvmpipe XLIB_SKIP_ARGB_VISUALS=1" try_variant llvmpipe
echo "diag done"
' > "${RUNDIR}/sbatch_outer.log" 2>&1
rc=$?
echo "diag exit ${rc}"
ls -la "${RUNDIR}"
exit ${rc}
