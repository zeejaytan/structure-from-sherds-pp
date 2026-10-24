#!/usr/bin/env bash
# Ticket 06 Test 2 -- oracle-init proof runs, all inside one held
# allocation (no queue between arms). Usage (from project root on
# Spartan): bash run_oracle_proof.sh <JOBID>
#
# Arms (single variable each, everything else default):
#   juglet67  : SFS_ORACLE_PAIR=6,7  SFS_ORACLE_M=<M_6,7>  (default gates)
#   juglet29  : SFS_ORACLE_PAIR=2,9  SFS_ORACLE_M=<M_2,9>  (default gates)
#   pota      : POT_A_ORIG build, SFS_ORACLE_PAIR=2,5  SFS_ORACLE_M=<M_2,5>
# Arm order is deliberate: prove the harness on the control FIRST so a
# null Juglet result cannot be blamed on the harness.
set -euo pipefail
JOBID="${1:?usage: run_oracle_proof.sh <HOLDER_JOBID>}"
ROOT=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif

# M values are injected by the caller (exported), computed offline from
# the TAXIS harvest with artifacts/juglet_run1/oracle_M.py.
: "${M_JUGLET_67:?set M_JUGLET_67 (12 numbers)}"
: "${M_JUGLET_29:?set M_JUGLET_29 (12 numbers)}"

run_arm() {   # $1=tag $2=pair $3=M
    local tag="$1" pair="$2" m="$3"
    echo "=== ARM ${tag}: oracle pair ${pair} ==="
    srun --jobid="${JOBID}" --overlap --cpus-per-task=8 bash -lc "
        ${APPTAINER} exec --bind /data:/data \
            --env SFS_ORACLE_PAIR='${pair}' --env SFS_ORACLE_M='${m}' \
            --env SFS_ORACLE_INJECT=1 \
            ${SIF} bash -c 'cd ${ROOT}/build && ./Hierarchy-Clear' \
            > ${ROOT}/oracle_${tag}.out 2>&1"
    echo "--- ${tag} verdict ---"
    # ORACLE-count 0 means the arm is VOID (override never fired) and the
    # run says nothing about the method. Never report a null from these.
    local fired
    fired=$(grep -a -c 'ORACLE \*\*\* pair' "${ROOT}/oracle_${tag}.out" || true)
    echo "ORACLE fired ${fired} time(s)$([ "${fired}" = "0" ] && echo '  <-- VOID ARM')"
    grep -a -E 'ORACLE|MERGETABLE|PLAUSIBILITY|Best assembly|Edge accuracy|Shard accuracy' \
        "${ROOT}/oracle_${tag}.out" | head -n 14 || true
}

run_arm juglet67 "${ORACLE_PAIR_67:-6,7}" "${M_JUGLET_67}"
run_arm juglet29 "${ORACLE_PAIR_29:-2,9}" "${M_JUGLET_29}"
echo "=== TEST 2 JUGLET ARMS DONE ==="
