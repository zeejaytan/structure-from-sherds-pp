#!/bin/bash
# Ticket 12 ROW 7 OFF arm (2026-10-13): priority-list scoring REVERTED to
# upstream shape -- NO combined score (local-only ordering). The transient
# in-job patch drops from live MakeHierarchyPriorityList
# (class/ranking_system.cpp): the ConnectivityOptimizer instance, both
# CalculateGlobalConnectivityScore calls, both 0.7/0.3 combined_score
# assignments, and the GLOBAL CONNECTIVITY debug print. What stays:
# CalculateMatchingScore (local inlier) + emplace_back in both loops, and
# the final InlierCompare sort -- with combined_score 0.0 everywhere its
# primary ties and the secondary inlier/node/i_edge comparison IS the
# upstream local-only ordering. The unused #include stays (harmless).
# Disable-vs-keep test vs run_row7_priority_keep.sbatch. The revert is
# TRANSIENT: applied via in-job patch after pull, restored before exit --
# the checkout is left clean, no commits.
#
# Rows 1-5 values stay (0.610 / 1.5 / 20,0.7 / veto disabled /
# CountInlier upstream-uniform).
#
# Same locked rebuild protocol as KEEP arm (shared lock serialises source
# access; separate binary build_sif/Hierarchy-Clear-row7off). Submit pinned:
#   sbatch --nodelist=spartan-bm065 run_row7_priority_off.sbatch
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_row7_off

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear-row7off"
LOCK="${ASM}/build_sif/.row7_build.lock"

echo "=== TICKET-12 ROW 7 OFF arm (combined score reverted, transient) ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- revert ticket-08 in-job JUGLET residue, then pull (ff-only):"
echo "    (retry loop: KEEP arm may hold the git index briefly at start)"
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

echo "--- verify rows 1-5 adopted values + scorer present pre-patch:"
test "$(grep -c 'combined_score = 0.7' class/ranking_system.cpp || true)" = "2" || { echo "ERROR: scorer shape changed -- wrong tree, aborting"; exit 1; }
test "$(grep -c 'CalculateGlobalConnectivityScore' class/ranking_system.cpp || true)" = "2" || { echo "ERROR: optimizer calls changed -- wrong tree, aborting"; exit 1; }
grep -q "TICKET-12 ROW 5" class/feature_matching.cpp || { echo "ERROR: row-5 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "TICKET-12 ROW 4" class/feature_matching.cpp || { echo "ERROR: row-4 marker absent -- stale checkout, aborting"; exit 1; }
grep -q "rej_dist = 20, rej_norm = 0.7;" class/reconstruction.cpp || { echo "ERROR: row-3 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "lowest_score > 1.5" class/feature_matching.cpp || { echo "ERROR: row-2 adopted value missing -- wrong tree, aborting"; exit 1; }
grep -q "axis_angle_ > 0.610" class/feature_matching.cpp || { echo "ERROR: row-1 kept value missing -- wrong tree, aborting"; exit 1; }
grep -q "^#define POT_A " class/data_path.h || { echo "ERROR: POT_A block not selected"; exit 1; }
echo "    preconditions present"

echo "--- locked patch + rebuild (lock serialises with KEEP arm):"
exec 9>"${LOCK}"
flock 9
cleanup() {
    echo "--- restoring ranking_system.cpp:"
    git -C "${ASM}" checkout -- class/ranking_system.cpp
    git -C "${ASM}" status --porcelain -- class/ | head -n 5 || true
    flock -u 9
    exec 9>&-
}
trap cleanup EXIT

python3 - <<'PYEOF'
import io
p = "class/ranking_system.cpp"
s = io.open(p, encoding="utf-8").read()

old_init = (
    "\t// Initialize ConnectivityOptimizer for global connectivity analysis\n"
    "\tConnectivityOptimizer connectivity_optimizer;\n"
)
assert s.count(old_init) == 1, "init block found %d times, expected 1" % s.count(old_init)
s = s.replace(old_init,
    "\t// TICKET-12 ROW 7 OFF arm (transient): upstream has no combined score --\n"
    "\t// local-only ordering (CalculateMatchingScore + inlier sort). Optimizer unused.\n")

lines = s.splitlines(True)
starts = [i for i, l in enumerate(lines)
          if l.strip() == "// Calculate global connectivity score"]
assert len(starts) == 2, "scorer anchors found %d times, expected 2" % len(starts)
for st in sorted(starts, reverse=True):
    i = st
    while "priority_list_.emplace_back" not in lines[i]:
        i += 1
        assert i - st < 40, "emplace not found near anchor at line %d" % (st + 1)
    j = i - 1
    while lines[j].strip() == "":
        j -= 1
    del lines[st:j + 1]
s = "".join(lines)

io.open(p, "w", encoding="utf-8", newline="").write(s)
print("patch applied: optimizer instance + both scorer regions removed")
PYEOF
[ $? -eq 0 ] || { echo "ERROR: patch failed -- aborting, tree restored by trap"; exit 1; }

echo "--- verify patch (scorer gone, local path live):"
test "$(grep -c 'CalculateGlobalConnectivityScore' class/ranking_system.cpp || true)" = "0" || { echo "ERROR: optimizer call still present"; exit 1; }
test "$(grep -c 'combined_score = 0.7' class/ranking_system.cpp || true)" = "0" || { echo "ERROR: combined assignment still present"; exit 1; }
test "$(grep -c 'ROW 7 OFF arm' class/ranking_system.cpp || true)" = "1" || { echo "ERROR: expected 1 OFF marker"; exit 1; }
test "$(grep -cF 'CalculateMatchingScore(*iter' class/ranking_system.cpp || true)" = "2" || { echo "ERROR: local scoring disturbed"; exit 1; }
git diff --stat -- class/ranking_system.cpp
echo "    patch verified"

"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/row7off_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
cp -f "${ASM}/build_sif/Hierarchy-Clear" "${BIN}" || rc=1
echo "    build exit ${rc} (log: build_sif/row7off_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/row7off_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/row7off_build_${SLURM_JOB_ID}.log" || true
    exit 1
fi

echo "--- freshness: binary must be newer than every source it was built from:"
STALE=$(find "${ASM}/class" "${ASM}/main_headless_correct.cpp" -newer "${BIN}" 2>/dev/null | grep -v "ranking_system.cpp" | head -n 5 || true)
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
    "${BIN}" 8 > "e2e_pota_row7off_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

LOG="e2e_pota_row7off_${SLURM_JOB_ID}.log"
echo "--- outcome lines:"
grep -E "Sherd accuracy|Edge accuracy|Best assembly" "${LOG}" | tail -n 5 || true
echo "--- counts:"
echo "SURVIVED: $(grep -c SURVIVED "${LOG}" || true)"
echo "diverged: $(grep -ci diverged "${LOG}" || true)"
echo "veto: $(grep -c 'Opposing normals' "${LOG}" || true)"
echo "overlap: $(grep -c 'overlap=true' "${LOG}" || true)"
echo "profile-result: $(grep -c 'PROFILE VALIDATION.*RESULT' "${LOG}" || true)"
echo "profile-passed: $(grep -c 'PROFILE VALIDATION.*PASSED' "${LOG}" || true)"
echo "global-connectivity-prints: $(grep -c 'GLOBAL CONNECTIVITY' "${LOG}" || true)"
echo "pair-1-4-lines: $(grep -c '1-4' "${LOG}" || true)"
echo "GT: $(grep -E 'GT|ground.?truth' "${LOG}" | tail -n 5 || true)"
exit ${rc}
