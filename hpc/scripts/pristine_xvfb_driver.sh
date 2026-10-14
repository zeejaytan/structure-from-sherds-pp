#!/bin/bash
# Pristine Pot A control (01-pristine-pot-a-run): drive the GUI-only upstream
# main.cpp headless under Xvfb with scripted keypresses. ZERO source change
# (O4 Rank 1): space = show state 0, f = fine registration + CountResult +
# SaveAcc, s = SaveResult, g = save transforms. Results land in the code's
# hardcoded /Dataset/SfS_pp/Result/ and are snapshotted to $RUNDIR.
#
# Usage (inside the pristine container, one apptainer exec):
#   pristine_xvfb_driver.sh <binary-log> <rundir>
set -u

LOG="${1:?binary log path}"
RUNDIR="${2:?run dir}"
BIN=/opt/sfs-pristine/build/Hierarchy-Clear
WIN_NAME="Pot reconstruction"

mkdir -p "${RUNDIR}"
export DISPLAY=:99
export LIBGL_ALWAYS_SOFTWARE=1

cleanup() {
    kill ${BINPID:-} 2>/dev/null || true
    sleep 3
    kill -9 ${BINPID:-} 2>/dev/null || true
    kill ${XVFB_PID:-} 2>/dev/null || true
    sleep 2
    kill -9 ${XVFB_PID:-} 2>/dev/null || true
}
trap cleanup EXIT

echo "=== driver start $(date) node $(hostname) ===" | tee -a "${RUNDIR}/driver.log"
ls -l "${BIN}" | tee -a "${RUNDIR}/driver.log"

# Xvfb as non-root refuses to create /tmp/.X11-unix itself: pre-create it.
mkdir -p /tmp/.X11-unix && chmod 1777 /tmp/.X11-unix
Xvfb :99 -screen 0 1400x900x24 >"${RUNDIR}/xvfb.log" 2>&1 &
XVFB_PID=$!
sleep 5
if ! xdpyinfo -display :99 >"${RUNDIR}/xdpyinfo.log" 2>&1; then
    echo "driver: Xvfb NOT healthy, abort" | tee -a "${RUNDIR}/driver.log"
    cat "${RUNDIR}/xvfb.log" | tee -a "${RUNDIR}/driver.log"
    exit 1
fi
echo "driver: Xvfb healthy (pid ${XVFB_PID})" | tee -a "${RUNDIR}/driver.log"

"${BIN}" 8 >"${LOG}" 2>&1 &
BINPID=$!
echo "driver: binary pid ${BINPID}" | tee -a "${RUNDIR}/driver.log"

READY=0
WAITED=0
for i in $(seq 1 300); do
    if grep -q "Time after data load" "${LOG}" 2>/dev/null; then READY=1; WAITED=$i; break; fi
    if ! kill -0 ${BINPID} 2>/dev/null; then echo "driver: binary exited early" | tee -a "${RUNDIR}/driver.log"; break; fi
    sleep 60
done
echo "driver: READY=${READY} waited ${WAITED}min" | tee -a "${RUNDIR}/driver.log"
if [ "${READY}" != "1" ]; then
    tail -n 30 "${LOG}" | tee -a "${RUNDIR}/driver.log" || true
    exit 1
fi
sleep 20

xdotool search --name "${WIN_NAME}" >"${RUNDIR}/xwin.log" 2>&1 || echo "driver: window NOT found" | tee -a "${RUNDIR}/driver.log"
sendkey() {
    if xdotool search --name "${WIN_NAME}" windowactivate --sync key "$1" >>"${RUNDIR}/xwin.log" 2>&1; then
        echo "driver: key $1 sent" | tee -a "${RUNDIR}/driver.log"
    else
        echo "driver: key $1 FAILED" | tee -a "${RUNDIR}/driver.log"
    fi
    sleep "$2"
}
sendkey space 15
sendkey f 180
sendkey s 20
sendkey g 20

echo "--- driver: outcome lines ---" | tee -a "${RUNDIR}/driver.log"
grep -E "Total number|Assembly|score|Score|accuracy|Acc|Result save|Saving transformation|There is co-first" "${LOG}" | tail -n 20 | tee -a "${RUNDIR}/driver.log" || true

ls -la --time-style=full-iso /Dataset/SfS_pp/Result/ >"${RUNDIR}/result_ls.log" 2>&1 || true
mkdir -p "${RUNDIR}/Result_snapshot"
cp -pr /Dataset/SfS_pp/Result/. "${RUNDIR}/Result_snapshot/" 2>/dev/null || true

if grep -q "Sherd accuracy" "${LOG}"; then
    echo "driver: SCORED, done" | tee -a "${RUNDIR}/driver.log"
    exit 0
else
    echo "driver: NO score lines in binary log" | tee -a "${RUNDIR}/driver.log"
    exit 1
fi
