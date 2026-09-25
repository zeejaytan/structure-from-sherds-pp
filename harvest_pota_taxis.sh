#!/usr/bin/env bash
# Pot_A_ORIG TAXIS harvest only (no oracle yet): swap the dataset define
# on the Spartan checkout, rebuild, run, restore the define. Prints the
# alignment matrices needed to build the control oracle offline.
# Usage: bash harvest_pota_taxis.sh <HOLDER_JOBID>
set -euo pipefail
JOBID="${1:?usage: harvest_pota_taxis.sh <HOLDER_JOBID>}"
ROOT=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif

cd "${ROOT}"
restore() {
    sed -i 's/^#define POT_A_ORIG$/\/\/#define POT_A_ORIG/; s/^\/\/#define JUGLET$/#define JUGLET/' class/data_path.h
    echo "--- restored defines ---"
    grep -n '^#define JUGLET\|^#define POT_A_ORIG' class/data_path.h || true
}
trap restore EXIT

sed -i 's/^#define JUGLET$/\/\/#define JUGLET/; s/^\/\/#define POT_A_ORIG$/#define POT_A_ORIG/' class/data_path.h
echo "--- active defines ---"
grep -n '^#define JUGLET\|^#define POT_A_ORIG' class/data_path.h || true

srun --jobid="${JOBID}" --overlap --cpus-per-task=8 bash -lc "
set -o pipefail
${APPTAINER} exec --bind /data:/data ${SIF} bash -c \
  'cd ${ROOT}/build && cmake .. >/dev/null && make -j8 Hierarchy-Clear' \
  > ${ROOT}/pota_rebuild.log 2>&1
rc=\$?
if [ \$rc -ne 0 ]; then
  grep -E 'error:' ${ROOT}/pota_rebuild.log | head -n 10
  exit \$rc
fi
${APPTAINER} exec --bind /data:/data ${SIF} bash -c \
  'cd ${ROOT}/build && ./Hierarchy-Clear' > ${ROOT}/pota_harvest.out 2>&1
echo \"POTA-HARVEST-EXIT=\$?\""

echo "--- Pot_A TAXIS ---"
grep -a 'TAXIS' "${ROOT}/pota_harvest.out" | head -n 8
echo "--- Pot_A baseline (no oracle) ---"
grep -a -E 'Best assembly|Edge accuracy|Shard accuracy' "${ROOT}/pota_harvest.out" | head -n 3 || true
