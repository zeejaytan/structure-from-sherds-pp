#!/bin/bash
# Build the assembly repo to verify the dropped-shard logging compiles.
#
# The change is small C++ (cout, std::vector<int>, push_back) and both
# headers are already included, so this is a check, not a fix. But
# "obviously fine" is exactly how an unbuilt change reaches a result.
#
# Run inside a holder:  srun --jobid=<ID> --overlap bash build_assembly.sh

set -uo pipefail

ASM=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
cd "${ASM}" || { echo "ERROR: ${ASM} missing"; exit 1; }

# Pick up the laptop's commit. --ff-only: never resolve divergence here.
echo "### pull"
git pull --ff-only 2>&1 | tail -n 3
rc=${PIPESTATUS[0]}
if [ "${rc}" -ne 0 ]; then
    echo "ERROR: pull failed (rc=${rc}). If it says diverged, STOP and report --"
    echo "       do not force, and do not edit tracked files on the cluster."
    exit 1
fi

echo
echo "### confirm the change is present"
if ! grep -q "DROPPED" main_headless.cpp; then
    echo "ERROR: the logging is not in main_headless.cpp on the cluster."
    exit 1
fi
grep -n "DROPPED\|shard accounting" main_headless.cpp | head -n 6

echo
echo "### build"
LOG=/tmp/asm_build.log
if [ -x "${ASM}/build_at_container.sh" ]; then
    echo "    using build_at_container.sh"
    bash "${ASM}/build_at_container.sh" > "${LOG}" 2>&1
    rc=$?
else
    APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
    SIF=$(ls /data/gpfs/projects/punim2657/*/*.sif 2>/dev/null | head -n 1)
    if [ -z "${SIF}" ]; then echo "ERROR: no container found"; exit 1; fi
    echo "    container: ${SIF}"
    "${APPTAINER}" exec --bind /data:/data "${SIF}" \
        bash -c "cd '${ASM}' && mkdir -p build && cd build && cmake .. && make -j8" \
        > "${LOG}" 2>&1
    rc=$?
fi

echo "    exit ${rc}  (not trusted on its own -- see the file check below)"
if [ "${rc}" -ne 0 ]; then
    echo "--- errors ---"
    grep -E 'error:|Error [0-9]|CMake Error' "${LOG}" | head -n 20 || true
    echo "--- last 15 lines ---"
    tail -n 15 "${LOG}"
    exit 1
fi

# The build can succeed and still not have relinked the binary that runs.
echo
echo "### verify the binary was relinked and carries the change"
BIN=$(find "${ASM}/build" -name "*eadless*" -type f -newer "${ASM}/main_headless.cpp" 2>/dev/null | head -n 3)
if [ -z "${BIN}" ]; then
    echo "** no headless binary newer than main_headless.cpp -- the edit did"
    echo "   not reach a built artefact. Treat the build as NOT done."
    exit 1
fi
for b in ${BIN}; do
    echo "  $(ls -lh "$b" | awk '{print $5, $6, $7, $8}')  $b"
done
echo
echo "If the strings tool is available, confirm the message is IN the binary:"
for b in ${BIN}; do
    if command -v strings > /dev/null 2>&1; then
        if strings "$b" 2>/dev/null | grep -q "THIS RUN IS USING A SUBSET"; then
            echo "  OK: '$b' contains the new message"
        else
            echo "  MISSING: '$b' does not contain the new message"
        fi
    fi
done
