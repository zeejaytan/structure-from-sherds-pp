#!/bin/bash
# Smoother A/B: run Pot_A matching with control (Lanczos) vs SG smoother.
# Mirrors run_nurbs_sfs.sbatch (binds, binary invocation) but with the
# aside-built Hierarchy-Clear-SGtest and an isolated working directory, so
# result files cannot mix with production runs or with each other.
#
# Usage:  srun --jobid=<ID> --overlap bash run_match_sg.sh control
#         srun --jobid=<ID> --overlap bash run_match_sg.sh sg
# Each run must print which smoother is active ([SFS-SG] line iff sg);
# a run without the expected marker is void, not a result.
#
# Decisive outcomes, stated before running:
#   same proposed joins both arms -> smoother does not move matching;
#     close the ticket as cosmetic (descriptors differ, matches don't)
#   different joins -> the smoother is load-bearing; judge which arm is
#     right against ground truth per pair, not by count alone

set -uo pipefail

TAG="${1:?usage: run_match_sg.sh <control|sg>}"
[ "${TAG}" = "control" ] || [ "${TAG}" = "sg" ] || { echo "ERROR: tag must be control or sg"; exit 1; }

M=/data/gpfs/projects/punim2657/sfs_main
ASM="${M}/sfspreproc-docker"
BIN="${ASM}/Hierarchy-Clear-SGtest"
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF="${M}/sfspreproc.sif"
DS=/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20251103/SfS_pp
WORK="${M}/diag_sgmatch_${TAG}"
LOG="/tmp/sgmatch_${TAG}.log"

[ -x "${BIN}" ] || { echo "ERROR: ${BIN} missing -- build first"; exit 1; }
[ -d "${DS}" ] || { echo "ERROR: dataset ${DS} missing"; exit 1; }
rm -rf "${WORK}"; mkdir -p "${WORK}"

echo "### matching run: ${TAG}"
if [ "${TAG}" = "sg" ]; then
    ENVSTR='env SFS_SMOOTHER=sg'
else
    ENVSTR='env -u SFS_SMOOTHER'
fi
( cd "${WORK}" && ${APPTAINER} exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${DS}:/Dataset/SfS_pp" \
    "${SIF}" /bin/bash -c "${ENVSTR} /workspace/sfspreproc-docker/Hierarchy-Clear-SGtest 8" ) > "${WORK}/stdout.log" 2>&1
rc=$?
echo "    exit ${rc} (not trusted; full stdout in ${WORK}/stdout.log)"
echo "--- smoother marker:"
if [ "${TAG}" = "sg" ]; then
    grep -c "SFS-SG" "${LOG}" 2>/dev/null | sed 's/^/    SFS-SG lines: /'
else
    grep -c "SFS-SG" "${LOG}" 2>/dev/null | sed 's/^/    SFS-SG lines (expect 0): /'
fi
echo "--- tail:"
tail -n 15 "${LOG}"
echo
echo "--- result files in ${WORK}:"
ls "${WORK}" | head -n 15
