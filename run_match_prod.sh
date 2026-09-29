#!/bin/bash
# Control-for-the-crash: does the PRODUCTION Hierarchy-Clear (Oct 2025,
# untouched by any ticket-16 work) segfault identically on this dataset?
# If yes: the crash predates my change and the A/B needs a working baseline
# first (build_fixed_hierarchy.sh exists for exactly this). If no: my change
# or the fresh build config broke it, and the SG work stops until that is
# understood.
#
# Run inside a holder:  srun --jobid=<ID> --overlap bash run_match_prod.sh

set -uo pipefail

M=/data/gpfs/projects/punim2657/sfs_main
ASM="${M}/sfspreproc-docker"
BIN="${ASM}/Hierarchy-Clear"
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF="${M}/sfspreproc.sif"
DS=/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20251103/SfS_pp
WORK="${M}/diag_sgmatch_prod"
LOG="/tmp/sgmatch_prod.log"

[ -x "${BIN}" ] || { echo "ERROR: production binary missing"; exit 1; }
md5sum "${BIN}" "${ASM}/Hierarchy-Clear-SGtest" | sed 's/^/    /'
rm -rf "${WORK}"; mkdir -p "${WORK}"

echo "### production binary, same dataset, same binds, isolated workdir"
( cd "${WORK}" && ${APPTAINER} exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${DS}:/Dataset/SfS_pp" \
    "${SIF}" /bin/bash -c "/workspace/sfspreproc-docker/Hierarchy-Clear 8" ) > "${LOG}" 2>&1
rc=$?
echo "    exit ${rc}"
echo "--- tail:"
tail -n 8 "${LOG}"
echo "--- result files:"
ls "${WORK}" | head -n 10
