#!/bin/bash
#SBATCH --job-name=t08_lcs_ab
#SBATCH --time=06:00:00
#SBATCH --mem=32G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire
#SBATCH --output=t08_lcs_ab_%j.out
#SBATCH --error=t08_lcs_ab_%j.err

# =====================================================================
# Ticket 08 (juglet-sfs): attribute the run-to-run pairing instability.
# One job = one node, so all runs below are trivially same-node
# (ticket 12 standing rule satisfied without --nodelist).
#
# Acceptance:
#  1. Same input twice in ONE process (SFS_LCS_TWICE=1) -> IDENTICAL =>
#     seed/thread/address-space cause; fix there.
#  2. DIFFER in-process => order-sensitive/stateful; report as
#     distribution.
#  3. Cross-process byte-compare (md5sum dump_A vs dump_B) decides
#     whether the instability survives process boundaries.
#
# Builds Hierarchy-Clear once (commit e5dadfa+: sorted fixed-precision
# LCS dump to stdout + sfs_lcs_dump.txt), runs it 3x:
#   Run A (normal) -> dump_A.txt
#   Run B (normal) -> dump_B.txt + md5/diff vs A
#   Run C (SFS_LCS_TWICE=1) -> *** LCS-TWICE *** IDENTICAL|DIFFER
# =====================================================================

echo "========================================================================"
echo "        TICKET 08 LCS-DUMP A/B (same node, 3 runs)                    "
echo "========================================================================"
echo "Job ID: $SLURM_JOB_ID  Node: $(hostname -s)"
echo "Start time: $(date)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker || exit 1

echo "=== Syncing code ==="
git pull --ff-only || { echo "ERROR: pull failed (diverged?)"; exit 1; }
git log --oneline -1
echo ""

# Ticket 08 is Juglet-specific, but the checkout's committed dataset is
# POT_A (cauchy-scales E2E series owns that line). Flip to JUGLET in-job
# with sed AFTER pull so the repo file is untouched: the JUGLET block
# uses host-absolute JUGLET_BASE paths, so the --bind /data:/data below
# suffices (same as run_juglet.sbatch). POT_A would need a /Dataset
# bind and yields zero connections + segfault without it (job 32850374).
echo "=== Selecting JUGLET dataset (in-job only, repo untouched) ==="
sed -i 's/^#define POT_A/\/\/#define POT_A/' class/data_path.h
sed -i 's|^// #define JUGLET|#define JUGLET //|' class/data_path.h
grep -n "^#define POT_A\|^#define JUGLET\|^//#define POT_A\|^// #define JUGLET" class/data_path.h | head -n 5
echo ""

echo "=== Verifying Configuration ==="
if grep -q "^#define JUGLET" class/data_path.h; then
    echo "JUGLET is defined in class/data_path.h"
else
    echo "ERROR: JUGLET not defined in class/data_path.h"
    exit 1
fi
echo ""

# Verify bundle inputs (every missing file silently switches its shard OFF)
echo "=== Verifying Juglet Bundle ==="
BUNDLE="/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp"
for kind in "Breaklines/Juglet_Piece_*_Breakline_0.pcd:9" \
            "Mesh/Juglet_Piece_*_Mesh.obj:9" \
            "Axes/Juglet_Piece_*_Axis.xyz:9" \
            "Surfaces/Juglet_Piece_*_Surface_0.xyz:9"; do
    pat="${kind%%:*}"; exp="${kind##*:}"
    got=$(ls ${BUNDLE}/${pat} 2>/dev/null | wc -l)
    echo "  ${pat}: ${got} (expected ${exp})"
    [ "${got}" -ne "${exp}" ] && { echo "ERROR: bundle incomplete"; exit 1; }
done
echo ""

echo "=== Building SFS++ (JUGLET) in container ==="
APPTAINER="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
SIF="/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif"
ROOT="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker"
time ${APPTAINER} exec \
    --bind "/data:/data" \
    "${SIF}" \
    bash -c "cd ${ROOT} && rm -rf build && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 Hierarchy-Clear"
[ $? -ne 0 ] && { echo "ERROR: build failed"; exit 1; }
ls -lh build/Hierarchy-Clear || { echo "ERROR: binary missing"; exit 1; }
echo ""

run_once() {
    local tag="$1"; shift
    echo "========================================================================"
    echo "  ${tag} starting at: $(date)"
    echo "========================================================================"
    time ${APPTAINER} exec \
        --bind "/data:/data" \
        --env SFS_LCS_TWICE="${SFS_LCS_TWICE_RUN:-}" \
        "${SIF}" \
        bash -c "cd ${ROOT}/build && ./Hierarchy-Clear"
    echo "${tag} exit code: $?"
    if [ -f "${ROOT}/build/sfs_lcs_dump.txt" ]; then
        cp "${ROOT}/build/sfs_lcs_dump.txt" "${ROOT}/build/${tag}.txt"
        echo "${tag}: $(wc -l < "${ROOT}/build/${tag}.txt") pairs, md5=$(md5sum "${ROOT}/build/${tag}.txt" | cut -d' ' -f1)"
    else
        echo "${tag}: ERROR sfs_lcs_dump.txt missing"
    fi
    echo ""
}

SFS_LCS_TWICE_RUN="" run_once dump_A
SFS_LCS_TWICE_RUN="" run_once dump_B

echo "=== Cross-process byte-compare (A vs B) ==="
md5sum "${ROOT}/build/dump_A.txt" "${ROOT}/build/dump_B.txt"
if cmp -s "${ROOT}/build/dump_A.txt" "${ROOT}/build/dump_B.txt"; then
    echo "*** LCS-ACROSS *** IDENTICAL (cross-process byte-identical)"
else
    echo "*** LCS-ACROSS *** DIFFER (cross-process instability confirmed)"
    diff "${ROOT}/build/dump_A.txt" "${ROOT}/build/dump_B.txt" | head -n 40 || true
fi
echo ""

SFS_LCS_TWICE_RUN=1 run_once dump_C

echo "=== In-process verdict (C) ==="
LOG="t08_lcs_ab_${SLURM_JOB_ID}.out"
[ -f "${LOG}" ] || LOG=$(ls -t t08_lcs_ab_*.out 2>/dev/null | head -n 1)
grep -aE "LCS-TWICE|LCS-ACROSS|LCS-DUMP-BEGIN" "${LOG}" 2>/dev/null | tail -n 20 || true
echo ""
echo "End time: $(date)"
