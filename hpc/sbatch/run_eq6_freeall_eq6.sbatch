#!/bin/bash
# Ticket 02 Eq.6 FREE-ALL arm (2026-10-13): paper Eq.6 global joint
# refinement (sfspp-2502.13986v1.md:300-302, frees ALL of Phi=Phi_old+Phi_new)
# vs run_eq6_freeall_control.sbatch (freeze as-is). The transient in-job
# patch SKIPS the pregraph-freeze loop in IcpIncGraphAxis
# (class/reconstruction.cpp ~:2186-2193,
# problem.SetParameterBlockConstant(s[i])/(trans[i]) for
# pregraph.back().node_[i]) and replaces it with explicit
# SetParameterBlockVariable on ALL sherd blocks present in the problem
# before ceres::Solve (~:2212; HasParameterBlock-guarded — blocks absent
# from the problem are already unconstrained, and setting those varying
# (~:2212). Pairwise sites (Icp ~:1541, Registration5 ~:1787,
# Registration6 ~:2010, dead IcpFine ~:2412) + cap (100/1000) + guard
# UNTOUCHED. R_rim/H_rim stay Constant. One variable: freeze vs free-all
# at the global step only. The patch is TRANSIENT: applied via in-job
# patch after pull, restored before exit -- the checkout is left clean,
# no commits.
#
# Rows 1-7 + 09 values stay (0.610 / 1.5 / 20,0.7 / veto disabled /
# CountInlier upstream-uniform / scorer deleted / overlap 50).
#
# Same locked rebuild protocol as CONTROL arm (shared lock serialises
# source access; separate binary build_sif/Hierarchy-Clear-eq6free).
# Submit pinned:
#   sbatch --nodelist=spartan-bm065 run_eq6_freeall_eq6.sbatch
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_eq6_free

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear-eq6free"
LOCK="${ASM}/build_sif/.eq6_build.lock"

echo "=== TICKET-02 EQ.6 FREE-ALL arm (global freeze removed, transient) ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- revert ticket-08 in-job JUGLET residue, then pull (ff-only):"
echo "    (retry loop: CONTROL arm may hold the git index briefly at start)"
for i in $(seq 1 30); do
  git checkout -- class/data_path.h 2>/dev/null \
    && git pull --ff-only 2>&1 | tail -n 3 \
    && break
  echo "    git busy (try $i/30), waiting 20s..."
  sleep 20
  [ "$i" = "30" ] && { echo "ERROR: git still locked -- STOP, report, do not force"; exit 1; }
done
echo "    HEAD now: $(git rev-parse HEAD)"
test -z "$(git status --porcelain -- class/ | head -n 3)" || { echo "ERROR: tree dirty after pull:"; git status --porcelain -- class/ | head; exit 1; }

echo "--- verify rows 1-7 + 09 adopted values + freeze loop present pre-patch:"
grep -q "TICKET-12 ROW 5" class/feature_matching.cpp || { echo "ERROR: row-5 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "TICKET-12 ROW 4" class/feature_matching.cpp || { echo "ERROR: row-4 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "rej_dist = 20, rej_norm = 0.7;" class/reconstruction.cpp || { echo "ERROR: row-3 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "lowest_score > 1.5" class/feature_matching.cpp || { echo "ERROR: row-2 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "axis_angle_ > 0.610" class/feature_matching.cpp || { echo "ERROR: row-1 kept value missing -- wrong tree, aborting"; exit 1; }
grep -q "TICKET-12 ROW 7" class/ranking_system.cpp || { echo "ERROR: row-7 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    preconditions present"

echo "--- locked patch + rebuild (lock serialises with CONTROL arm):"
exec 9>"${LOCK}"
flock 9
cleanup() {
    echo "--- restoring reconstruction.cpp:"
    git -C "${ASM}" checkout -- class/reconstruction.cpp
    git -C "${ASM}" status --porcelain -- class/ | head -n 5 || true
    flock -u 9
    exec 9>&-
}
trap cleanup EXIT

python3 - <<'PYEOF'
import io
p = "class/reconstruction.cpp"
s = io.open(p, encoding="utf-8").read()

old = (
    "\t\tif (pregraph.size() != 0) {\n"
    "\t\t\t//cout << \"Constant ceres parameter \" << endl;\n"
    "\t\t\tfor (int i = 0; i < num_shard; i++) {\n"
    "\t\t\t\tif (pregraph.back().node_[i]) {\n"
    "\t\t\t\t\tproblem.SetParameterBlockConstant(s[i]);\n"
    "\t\t\t\t\tproblem.SetParameterBlockConstant(trans[i]);\n"
    "\t\t\t\t}\n"
    "\t\t\t}\n"
    "\t\t}\n"
)
assert s.count(old) == 1, "freeze block found %d times, expected 1" % s.count(old)
new = (
    "\t\t// TICKET-02 EQ.6 FREE-ALL (transient): global joint refinement --\n"
    "\t\t// all sherd pose blocks VARIABLE at the global solve (no Constant\n"
    "\t\t// on any sherd block; R_rim/H_rim stay Constant as now).\n"
    "\t\t// presence guard: sherds with no residuals in this\n"
    "\t\t// iteration own no block in the problem (already unconstrained);\n"
    "\t\t// setting those varying trips a Ceres CHECK (job 32905463).\n"
    "\t\tfor (int i = 0; i < num_shard; i++) {\n"
    "\t\t\tif (problem.HasParameterBlock(s[i]))\n"
    "\t\t\t\tproblem.SetParameterBlockVariable(s[i]);\n"
    "\t\t\tif (problem.HasParameterBlock(trans[i]))\n"
    "\t\t\t\tproblem.SetParameterBlockVariable(trans[i]);\n"
    "\t\t}\n"
)
s = s.replace(old, new)
io.open(p, "w", encoding="utf-8", newline="").write(s)
print("patch applied: pregraph-freeze loop replaced with free-all")
PYEOF
[ $? -eq 0 ] || { echo "ERROR: patch failed -- aborting, tree restored by trap"; exit 1; }

echo "--- verify patch (freeze gone, free-all live, rim pins untouched):"
test "$(grep -c 'problem.SetParameterBlockConstant(s\[i\]);' class/reconstruction.cpp || true)" = "0" || { echo "ERROR: sherd freeze still present"; exit 1; }
test "$(grep -c 'SetParameterBlockVariable' class/reconstruction.cpp || true)" = "2" || { echo "ERROR: expected 2 free-all lines"; exit 1; }
test "$(grep -c 'HasParameterBlock' class/reconstruction.cpp || true)" = "2" || { echo "ERROR: expected 2 block guards"; exit 1; }
test "$(grep -c 'EQ.6 FREE-ALL' class/reconstruction.cpp || true)" = "1" || { echo "ERROR: expected 1 EQ.6 marker"; exit 1; }
test "$(grep -c 'SetParameterBlockConstant(&R_rim);' class/reconstruction.cpp || true)" = "2" || { echo "ERROR: R_rim pins disturbed"; exit 1; }
test "$(grep -c 'SetParameterBlockConstant(&H_rim);' class/reconstruction.cpp || true)" = "2" || { echo "ERROR: H_rim pins disturbed"; exit 1; }
git diff --stat -- class/reconstruction.cpp
echo "    patch verified"

"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/eq6free_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
cp -f "${ASM}/build_sif/Hierarchy-Clear" "${BIN}" || rc=1
echo "    build exit ${rc} (log: build_sif/eq6free_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/eq6free_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/eq6free_build_${SLURM_JOB_ID}.log" || true
    exit 1
fi

echo "--- freshness: binary must be newer than every source it was built from:"
STALE=$(find "${ASM}/class" "${ASM}/main_headless_correct.cpp" -newer "${BIN}" 2>/dev/null | grep -v "reconstruction.cpp" | head -n 5 || true)
if [ -n "${STALE}" ]; then
    echo "ERROR: binary older than sources (stale build):"
    echo "${STALE}"
    exit 1
fi
ls -lh "${BIN}"
[ -x "${BIN}" ] || { echo "ERROR: binary missing/not executable"; exit 1; }

echo "--- input census (host side, trusted over container):"
for s in Breaklines Mesh Axes Surfaces; do
    echo "    $s: $(ls "${FRESH}/$s" | wc -l) files"
done

cd /data/gpfs/projects/punim2657/sfs_main || exit 1

time "${APPTAINER}" exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "${FRESH}:/Dataset/SfS_pp" \
    "${SIF}" \
    "${BIN}" 8 > "e2e_pota_eq6free_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

LOG="e2e_pota_eq6free_${SLURM_JOB_ID}.log"
echo "--- outcome lines:"
grep -E "Sherd accuracy|Edge accuracy|Best assembly" "${LOG}" | tail -n 5 || true
echo "--- counts:"
echo "SURVIVED: $(grep -c SURVIVED "${LOG}" || true)"
echo "diverged: $(grep -ci diverged "${LOG}" || true)"
echo "veto: $(grep -c 'Opposing normals' "${LOG}" || true)"
echo "overlap: $(grep -c 'overlap=true' "${LOG}" || true)"
echo "profile-result: $(grep -c 'PROFILE VALIDATION.*RESULT' "${LOG}" || true)"
echo "profile-passed: $(grep -c 'PROFILE VALIDATION.*PASSED' "${LOG}" || true)"
echo "pair-1-4-lines: $(grep -c '1-4' "${LOG}" || true)"
echo "GT: $(grep -E 'GT|ground.?truth' "${LOG}" | tail -n 5 || true)"
exit ${rc}
