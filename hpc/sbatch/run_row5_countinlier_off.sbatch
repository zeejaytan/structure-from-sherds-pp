#!/bin/bash
# Ticket 12 ROW 5 OFF arm (2026-10-13): CountInlier adaptive segment
# contribution REVERTED to upstream uniform gate (pre-869e696 shape:
# `if (dummy_inlier > MINIMUM_NUMBER / 2)`), in overloads 1&2 only.
# Disable-vs-keep test vs run_row5_countinlier_keep.sbatch. The revert is
# TRANSIENT: applied via in-job patch after pull, restored before exit --
# the checkout is left clean, no commits. acos clamp + INLIER DEBUG prints
# stay (one variable: the gate only).
#
# Rows 1-4 values stay (0.610 / 1.5 / 20,0.7 / veto disabled).
#
# Same locked rebuild protocol as KEEP arm (shared lock serialises source
# access; separate binary build_sif/Hierarchy-Clear-row5off). Submit pinned:
#   sbatch --nodelist=spartan-bm065 run_row5_countinlier_off.sbatch
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=sfs_row5_off

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
FRESH=/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp
BIN="${ASM}/build_sif/Hierarchy-Clear-row5off"
LOCK="${ASM}/build_sif/.row5_build.lock"

echo "=== TICKET-12 ROW 5 OFF arm (adaptive reverted, transient) ==="
echo "Job ${SLURM_JOB_ID} on $(hostname), $(date)"

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "--- revert ticket-08 in-job JUGLET residue, then pull (ff-only):"
git checkout -- class/data_path.h || { echo "ERROR: checkout data_path.h failed"; exit 1; }
git pull --ff-only 2>&1 | tail -n 3 || { echo "ERROR: pull failed -- STOP, report, do not force"; exit 1; }
echo "    HEAD now: $(git rev-parse HEAD)"
test -z "$(git status --porcelain -- class/ | head -n 3)" || { echo "ERROR: tree dirty after pull:"; git status --porcelain -- class/ | head; exit 1; }

echo "--- verify rows 1-4 adopted values + adaptive present pre-patch:"
grep -q "ROBUSTNESS FIX: Adaptive segment contribution" class/feature_matching.cpp || { echo "ERROR: adaptive gate absent -- wrong tree, aborting"; exit 1; }
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
    echo "--- restoring feature_matching.cpp:"
    git checkout -- class/feature_matching.cpp
    git status --porcelain -- class/ | head -n 5 || true
    flock -u 9
    exec 9>&-
}
trap cleanup EXIT

python3 - <<'PYEOF'
import io
p = "class/feature_matching.cpp"
s = io.open(p, encoding="utf-8").read()

old1 = (
"\t\t// ROBUSTNESS FIX: Adaptive segment contribution based on geometric significance\n"
"\t\tbool segment_contributes = false;\n"
"\t\tint segment_size = COR[i].cor.size();\n"
"\n"
"\t\tif (segment_size >= 30) {\n"
"\t\t\t// Large segments: use original threshold (backward compatibility)\n"
"\t\t\tsegment_contributes = (dummy_inlier > MINIMUM_NUMBER / 2);\n"
"\t\t} else if (segment_size >= 15) {\n"
"\t\t\t// Medium segments: require at least 1 inlier\n"
"\t\t\tsegment_contributes = (dummy_inlier >= 1);\n"
"\t\t} else {\n"
"\t\t\t// Small segments (geometric features from NURBS): density-based\n"
"\t\t\tdouble inlier_density = (double)dummy_inlier / segment_size;\n"
"\t\t\tsegment_contributes = (inlier_density >= 0.08); // 8% coverage for small features\n"
"\t\t}\n"
"\n"
"\t\tif (segment_contributes) {\n"
"\t\t\tinlier += dummy_inlier;\n"
)
new1 = (
"\t\t// TICKET-12 ROW 5 OFF arm (transient): upstream uniform gate.\n"
"\t\tif (dummy_inlier > MINIMUM_NUMBER / 2)\n"
"\t\t{\n"
"\t\t\tinlier += dummy_inlier;\n"
)
assert s.count(old1) == 1, "overload-1 block found %d times, expected 1" % s.count(old1)
s = s.replace(old1, new1)

old2 = (
"\t\t// ROBUSTNESS FIX: Adaptive segment contribution (same logic as first CountInlier)\n"
"\t\tbool segment_contributes = false;\n"
"\t\tint segment_size = COR[i].cor.size();\n"
"\n"
"\t\tif (segment_size >= 30) {\n"
"\t\t\t// Large segments: use original threshold (backward compatibility)\n"
"\t\t\tsegment_contributes = (dummy_inlier > MINIMUM_NUMBER / 2);\n"
"\t\t} else if (segment_size >= 15) {\n"
"\t\t\t// Medium segments: require at least 1 inlier\n"
"\t\t\tsegment_contributes = (dummy_inlier >= 1);\n"
"\t\t} else {\n"
"\t\t\t// Small segments (geometric features from NURBS): density-based\n"
"\t\t\tdouble inlier_density = (double)dummy_inlier / segment_size;\n"
"\t\t\tsegment_contributes = (inlier_density >= 0.08); // 8% coverage for small features\n"
"\t\t}\n"
"\n"
"\t\tif (segment_contributes) {\n"
"\t\t\tshard_inlier[s_x] += dummy_inlier;\n"
)
new2 = (
"\t\t// TICKET-12 ROW 5 OFF arm (transient): upstream uniform gate.\n"
"\t\tif (dummy_inlier > MINIMUM_NUMBER / 2)\n"
"\t\t{\n"
"\t\t\tshard_inlier[s_x] += dummy_inlier;\n"
)
assert s.count(old2) == 1, "overload-2 block found %d times, expected 1" % s.count(old2)
s = s.replace(old2, new2)

io.open(p, "w", encoding="utf-8", newline="").write(s)
print("patch applied: both overloads reverted to uniform gate")
PYEOF
[ $? -eq 0 ] || { echo "ERROR: patch failed -- aborting, tree restored by trap"; exit 1; }

echo "--- verify patch (adaptive gone, uniform live):"
test "$(grep -c 'ROBUSTNESS FIX: Adaptive segment contribution' class/feature_matching.cpp || true)" = "0" || { echo "ERROR: adaptive marker still present"; exit 1; }
test "$(grep -c 'ROW 5 OFF arm' class/feature_matching.cpp || true)" = "2" || { echo "ERROR: expected 2 OFF markers"; exit 1; }
git diff --stat -- class/feature_matching.cpp
echo "    patch verified"

"${APPTAINER}" exec --bind /data:/data "${SIF}" \
    bash -c "cd '${ASM}/build_sif' && cmake .. >/dev/null && make -j8 Hierarchy-Clear" \
    > "${ASM}/build_sif/row5off_build_${SLURM_JOB_ID}.log" 2>&1
rc=$?
cp -f "${ASM}/build_sif/Hierarchy-Clear" "${BIN}" || rc=1
echo "    build exit ${rc} (log: build_sif/row5off_build_${SLURM_JOB_ID}.log)"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${ASM}/build_sif/row5off_build_${SLURM_JOB_ID}.log" | head -n 15 || true
    tail -n 10 "${ASM}/build_sif/row5off_build_${SLURM_JOB_ID}.log" || true
    exit 1
fi

echo "--- freshness: binary must be newer than every source it was built from:"
STALE=$(find "${ASM}/class" "${ASM}/main_headless_correct.cpp" -newer "${BIN}" 2>/dev/null | grep -v "feature_matching.cpp" | head -n 5 || true)
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
    "${BIN}" 8 > "e2e_pota_row5off_${SLURM_JOB_ID}.log" 2>&1
rc=$?
echo "binary exit ${rc} (not trusted alone)"

LOG="e2e_pota_row5off_${SLURM_JOB_ID}.log"
echo "--- outcome lines:"
grep -E "Sherd accuracy|Edge accuracy|Best assembly" "${LOG}" | tail -n 5 || true
echo "--- counts:"
echo "SURVIVED: $(grep -c SURVIVED "${LOG}" || true)"
echo "diverged: $(grep -ci diverged "${LOG}" || true)"
echo "veto: $(grep -c 'Opposing normals' "${LOG}" || true)"
echo "overlap: $(grep -c 'overlap=true' "${LOG}" || true)"
echo "profile-result: $(grep -c 'PROFILE VALIDATION.*RESULT' "${LOG}" || true)"
echo "profile-passed: $(grep -c 'PROFILE VALIDATION.*PASSED' "${LOG}" || true)"
exit ${rc}
