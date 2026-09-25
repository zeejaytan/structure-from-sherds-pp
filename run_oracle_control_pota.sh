#!/usr/bin/env bash
# Ticket 06 Test 2 -- Pot_A_ORIG control arm. Proves the oracle harness
# works on the sample the method DOES solve, so a null Juglet result
# cannot be blamed on the harness. Runs inside an existing holder.
#
# Usage: bash run_oracle_control_pota.sh <HOLDER_JOBID>
#   env: M_POTA_25  (12 numbers, from the Pot_A TAXIS harvest)
set -euo pipefail
JOBID="${1:?usage: run_oracle_control_pota.sh <HOLDER_JOBID>}"
ROOT=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
: "${M_POTA_25:?set M_POTA_25 (12 numbers from oracle_M_pota.py)}"

cd "${ROOT}"
git pull --ff-only

echo "=== STEP 1: switch dataset to POT_A_ORIG, rebuild, harvest TAXIS ==="
# POT_A_ORIG is a separate dataset block in class/data_path.h. Swap the
# define on the Spartan checkout only (never edit tracked files there);
# restore JUGLET in the trap so the next Juglet run is not poisoned.
sed -i 's/^#define JUGLET$/\/\/#define JUGLET/; s/^\/\/#define POT_A_ORIG$/#define POT_A_ORIG/' class/data_path.h
restore() { sed -i 's/^#define POT_A_ORIG$/\/\/#define POT_A_ORIG/; s/^\/\/#define JUGLET$/#define JUGLET/' class/data_path.h; }
trap restore EXIT
grep -n '^#define JUGLET\|^#define POT_A_ORIG' class/data_path.h

srun --jobid="${JOBID}" --overlap --cpus-per-task=8 bash -lc "
    ${APPTAINER} exec --bind /data:/data ${SIF} bash -c \
      'cd ${ROOT}/build && cmake .. >/dev/null && make -j8 Hierarchy-Clear 2>&1 | tail -n 1' \
    && ${APPTAINER} exec --bind /data:/data ${SIF} bash -c \
      'cd ${ROOT}/build && ./Hierarchy-Clear' > ${ROOT}/pota_harvest.out 2>&1"
echo "--- Pot_A TAXIS (first 3) ---"
grep -a 'TAXIS' "${ROOT}/pota_harvest.out" | head -n 3

echo "=== STEP 2: baseline (no oracle) -- does the sample still work? ==="
srun --jobid="${JOBID}" --overlap --cpus-per-task=8 bash -lc "
    ${APPTAINER} exec --bind /data:/data ${SIF} bash -c \
      'cd ${ROOT}/build && ./Hierarchy-Clear' > ${ROOT}/oracle_pota_baseline.out 2>&1"
grep -a -E 'Best assembly|Edge accuracy|Shard accuracy' \
    "${ROOT}/oracle_pota_baseline.out" | head -n 3 || true

echo "=== STEP 3: oracle arm 2-5 from GT ==="
srun --jobid="${JOBID}" --overlap --cpus-per-task=8 bash -lc "
    ${APPTAINER} exec --bind /data:/data \
      --env SFS_ORACLE_PAIR='2,5' --env SFS_ORACLE_M='${M_POTA_25}' \
      ${SIF} bash -c 'cd ${ROOT}/build && ./Hierarchy-Clear' \
      > ${ROOT}/oracle_pota_25.out 2>&1"
echo "--- oracle 2-5 verdict ---"
grep -a -E 'ORACLE|Best assembly|Edge accuracy|Shard accuracy' \
    "${ROOT}/oracle_pota_25.out" | head -n 8 || true
echo "=== POT_A CONTROL DONE (JUGLET define restored by trap) ==="
