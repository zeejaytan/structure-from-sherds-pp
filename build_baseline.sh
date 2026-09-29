#!/bin/bash
# Bisect the segfault: build WITHOUT the smoother change, run identically.
# If BASETEST also segfaults, the crash is in the fresh build (config,
# toolchain drift since Oct 2025), not in the filter edit. If it completes,
# the filter edit is guilty and the SG work stops until understood.
#
# Uses git stash so the working tree is restored exactly. Aborts loudly if
# the stash/pop fails at any point rather than leaving the tree dirty.
#
# Run inside a holder:  srun --jobid=<ID> --overlap bash build_baseline.sh

set -uo pipefail

ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
LOG=/tmp/baseline_build.log

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

echo "### stash filter changes (tracked edits only)"
git stash push -q class/filter.cpp class/filter.h || { echo "ERROR: stash failed"; exit 1; }
git status --short class/ | head -n 3
echo "    stashed; tree clean for class/: $(git status --short class/ | wc -l) modified"

echo
echo "### build baseline (POT_A selection stays -- same dataset both arms)"
"${APPTAINER}" exec --bind /data:/data "${SIF}" /bin/bash -c \
    "cd '${ASM}/build' && cmake .. && make -j8 Hierarchy-Clear" \
    > "${LOG}" 2>&1
rc=$?
echo "BUILD-EXIT=${rc}"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error' "${LOG}" | head -n 10 || true
    git stash pop -q || echo "ERROR: stash pop failed after build failure -- TREE DIRTY, fix by hand"
    exit 1
fi
cp "${ASM}/build/Hierarchy-Clear" "${ASM}/Hierarchy-Clear-BASETEST"
echo "    baseline aside as Hierarchy-Clear-BASETEST"

echo
echo "### restore my changes"
git stash pop -q || { echo "ERROR: stash pop failed -- TREE DIRTY, fix by hand"; exit 1; }
git status --short class/ | head -n 5
echo "### rebuild SGtest so the tree state matches the binary set"
"${APPTAINER}" exec --bind /data:/data "${SIF}" /bin/bash -c \
    "cd '${ASM}/build' && make -j8 Hierarchy-Clear" > "${LOG}.sg" 2>&1
echo "    rebuild exit $?"
cp "${ASM}/build/Hierarchy-Clear" "${ASM}/Hierarchy-Clear-SGtest"
echo "    SGtest restored"
