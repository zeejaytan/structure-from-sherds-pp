#!/bin/bash
# Bisect the segfault: build WITHOUT the smoother change, run identically.
# If BASETEST also segfaults, the crash is in the fresh build (config,
# toolchain drift since Oct 2025), not in the filter edit. If it completes,
# the filter edit is guilty and the SG work stops until understood.
#
# Method: the filter edit lives in exactly one commit (40321cc "SG smoother
# behind SFS_SMOOTHER switch"). Take the PRE-CHANGE versions from its parent,
# build, restore. No stash: a previous version of this script stashed
# already-committed changes, found nothing to stash, and mislabeled a
# WITH-changes build as "baseline". File copies with verification instead.
#
# Run inside a holder:  srun --jobid=<ID> --overlap bash build_baseline.sh

set -uo pipefail

ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
LOG=/tmp/baseline_build.log
PRE=40321cc^
BACKUP=/tmp/filter_mine

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "### confirm my changes are in the tree first"
grep -q "SFS-SG" class/filter.cpp || { echo "ERROR: my filter edit absent -- nothing to bisect against"; exit 1; }
echo "    present"

echo
echo "### save mine aside, check out pre-change versions"
rm -rf "${BACKUP}"; mkdir -p "${BACKUP}"
cp class/filter.cpp class/filter.h "${BACKUP}/" || exit 1
git show "${PRE}:class/filter.cpp" > class/filter.cpp || exit 1
git show "${PRE}:class/filter.h" > class/filter.h || exit 1
if grep -q "SFS-SG" class/filter.cpp; then
    echo "ERROR: pre-change file still contains the switch -- aborting"
    cp "${BACKUP}"/filter.cpp "${BACKUP}"/filter.h class/
    exit 1
fi
echo "    pre-change files in place (no SFS-SG)"

echo
echo "### build baseline (POT_A selection unchanged -- same dataset both arms)"
"${APPTAINER}" exec --bind /data:/data "${SIF}" /bin/bash -c \
    "cd '${ASM}/build' && cmake .. && make -j8 Hierarchy-Clear" \
    > "${LOG}" 2>&1
rc=$?
echo "BUILD-EXIT=${rc}"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error' "${LOG}" | head -n 10 || true
    cp "${BACKUP}"/filter.cpp "${BACKUP}"/filter.h class/
    echo "    (my files restored after failed build)"
    exit 1
fi
cp "${ASM}/build/Hierarchy-Clear" "${ASM}/Hierarchy-Clear-BASETEST"
echo "    real baseline aside as Hierarchy-Clear-BASETEST"

echo
echo "### restore mine and rebuild SGtest"
cp "${BACKUP}"/filter.cpp "${BACKUP}"/filter.h class/
grep -q "SFS-SG" class/filter.cpp || { echo "ERROR: restore failed -- fix by hand"; exit 1; }
"${APPTAINER}" exec --bind /data:/data "${SIF}" /bin/bash -c \
    "cd '${ASM}/build' && make -j8 Hierarchy-Clear" > "${LOG}.sg" 2>&1
echo "    rebuild exit $?"
cp "${ASM}/build/Hierarchy-Clear" "${ASM}/Hierarchy-Clear-SGtest"
echo "    SGtest restored; tree state:"
git status --short class/ | head -n 3
echo "    (clean above = only intended edits present)"
