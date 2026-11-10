#!/bin/bash
# t330_run.sh -- ONE assembly arm inside the ticket-326 image, with g_len_scale forced from argv.
# Instrument only: base.cpp is the shipped batch shim (main_pristine_batch.cpp) with TWO inserted
# statements at the top of main(): an extern declaration of g_len_scale and
#   if (argc > 2 && argv[2] != NULL) g_len_scale = std::atof(argv[2]);
# MeasureLengthScale() returns early when g_len_scale > 0 (patch 326, data_structure.cpp), so the
# value passed here IS the regime: 60.00 -> Gate(x) == x (pristine literals), 92.77 -> 1.546x.
# No algorithm, threshold, weight, solver or order change of any kind.
# Usage: bash /panel/t330_run.sh ARM S NP
set -uo pipefail
ARM=$1; S=$2; NP=$3
BUILD=/panel/build
D=$BUILD/CMakeFiles/Hierarchy-Clear.dir
BASE=/panel/shim/base.cpp
VAR=/panel/shim/$ARM.cpp
mkdir -p /panel/logs /panel/bin /out/$ARM
echo "########## run $ARM S=$S np=$NP $(date -u +%FT%TZ) on $(hostname)"
cp "$BASE" "$VAR"
echo "--- shim md5: $(md5sum $VAR | cut -d' ' -f1)"
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
g++ -I/panel/inc_E $DEFS $INCS $FLAGS -I/opt/sfs-pristine -I/opt/sfs-pristine/class -c "$VAR" -o $D/shim_$ARM.o
if [ $? -ne 0 ]; then echo "!!!! COMPILE FAILED"; exit 3; fi
echo "--- compile rc=0"
cd "$BUILD" || { echo "!!!! cd BUILD failed"; exit 4; }
LINK=$(sed -e "s|CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/shim_$ARM.o|" -e "s|-o Hierarchy-Clear|-o /panel/bin/Hierarchy-$ARM|" $D/link.txt)
eval "$LINK"
if [ $? -ne 0 ]; then echo "!!!! LINK FAILED"; exit 4; fi
echo "--- link rc=0 binary md5: $(md5sum /panel/bin/Hierarchy-$ARM | cut -d' ' -f1)"
export OMP_NUM_THREADS=8
cd /panel/build
rm -rf /Dataset/SfS_pp/Result
mkdir -p /Dataset/SfS_pp/Result
t0=$SECONDS
/panel/bin/Hierarchy-$ARM $NP $S > /panel/logs/binary_$ARM.log 2>&1
rc=$?
dt=$((SECONDS - t0))
echo "--- batch exit=$rc wall=${dt}s"
grep -E "T330_S|Total number|Sherd accuracy|Edge accuracy|Result save finish|End condition|Node Sequence" /panel/logs/binary_$ARM.log | tail -12 || true
cp -pr /Dataset/SfS_pp/Result/. /out/$ARM/ 2>/dev/null || true
rm -rf /Dataset/SfS_pp/Result
echo "########## run $ARM done rc=$rc wall=${dt}s"
exit $rc