#!/bin/bash
# Bisect run: Hierarchy-Clear-BASETEST (no smoother change) on the same
# dataset, same binds, isolated workdir. If it segfaults identically, the
# crash is the fresh build, not the filter edit. If it completes, the edit
# is guilty.
#
# Run inside a holder:  srun --jobid=<ID> --overlap bash run_match_base.sh

set -uo pipefail

M=/data/gpfs/projects/punim2657/sfs_main
ASM="${M}/sfspreproc-docker"
BIN="${ASM}/Hierarchy-Clear-BASETEST"
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF="${M}/sfspreproc.sif"
DS=/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20251103/SfS_pp
WORK="${M}/diag_sgmatch_base"
LOG="/tmp/sgmatch_base.log"

[ -x "${BIN}" ] || { echo "ERROR: ${BIN} missing"; exit 1; }
if strings "${BIN}" 2>/dev/null | grep -q "SFS-SG"; then
    echo "ERROR: BASETEST contains the switch -- mislabeled build, aborting"
    exit 1
fi
echo "BASETEST confirmed free of the switch"
rm -rf "${WORK}"; mkdir -p "${WORK}"

( cd "${WORK}" && ${APPTAINER} exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${DS}:/Dataset/SfS_pp" \
    "${SIF}" /bin/bash -c "/workspace/sfspreproc-docker/Hierarchy-Clear-BASETEST 8" ) > "${LOG}" 2>&1
rc=$?
echo "    exit ${rc}"
echo "--- tail:"
tail -n 8 "${LOG}"
echo "--- result files:"
ls "${WORK}" | head -n 10
