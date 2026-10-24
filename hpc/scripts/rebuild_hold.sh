#!/usr/bin/env bash
# Rebuild Hierarchy-Clear inside a held allocation and print real
# compiler errors (no pipe to tail: that masks the exit code).
# Usage: bash rebuild_hold.sh <JOBID>
set -euo pipefail
JOBID="${1:?usage: rebuild_hold.sh <HOLDER_JOBID>}"
ROOT=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif

srun --jobid="${JOBID}" --overlap --cpus-per-task=8 bash -lc "
set -o pipefail
${APPTAINER} exec --bind /data:/data ${SIF} bash -c 'cd ${ROOT}/build && cmake .. >/dev/null && make -j8 Hierarchy-Clear' \
  > ${ROOT}/rebuild_hold.log 2>&1"
rc=$?
echo "REBUILD-EXIT=${rc}"
if [ "${rc}" -ne 0 ]; then
    echo "--- errors ---"
    grep -E 'error:|Error [0-9]' "${ROOT}/rebuild_hold.log" | head -n 20 || true
fi
exit "${rc}"
