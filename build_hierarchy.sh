#!/bin/bash
# Build Hierarchy-Clear (the binary the sbatch matchers run). In a file:
# inline nested quoting breaks.
#
# VERIFY before relying on it: the JUGLET/POT_A selection in
# class/data_path.h is COMPILE-TIME. A binary built for one dataset
# silently reads the other dataset's paths if rebuilt after a switch, and
# nothing in the log names the dataset. Check the selection first --
# this script prints it and aborts on ambiguity.
#
# Run inside a holder:  srun --jobid=<ID> --overlap bash build_hierarchy.sh

set -uo pipefail

ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_preprocessing/pcl_191_nurbs.sif
LOG=/tmp/hierarchy_build.log

cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }
git pull --ff-only 2>&1 | tail -n 2

echo "### dataset selection in this build:"
SEL=$(grep -E "^#define (POT_A|JUGLET|POT_A_ORIG|TRAY_000|POT_B|BB_)" class/data_path.h || true)
echo "    ${SEL:-NONE FOUND -- aborting}"
echo "${SEL}" | wc -l | grep -q "^1$" || { echo "ERROR: not exactly one dataset selected"; exit 1; }

echo
echo "### SFS_SMOOTHER switch present in filter.cpp:"
grep -c "SFS-SG" class/filter.cpp || { echo "ERROR: smoother switch missing"; exit 1; }

echo
echo "### build"
"${APPTAINER}" exec --bind /data:/data "${SIF}" /bin/bash -c \
    "cd '${ASM}/build' && cmake .. && make -j8 Hierarchy-Clear" \
    > "${LOG}" 2>&1
rc=$?
echo "BUILD-EXIT=${rc}"
if [ "${rc}" -ne 0 ]; then
    grep -E 'error:|Error [0-9]|CMake Error|No rule' "${LOG}" | head -n 15 || true
    tail -n 10 "${LOG}"
    exit 1
fi
ls -lh "${ASM}/Hierarchy-Clear" | awk '{print "  built:", $5, $6, $7, $8, $9}'
echo "    dataset: ${SEL}"
