#!/bin/bash
# arm67.sh -- ticket 67: ONE arm, executed INSIDE sfs_pristine_assembly.sif.
# t58's work/arm.sh with one change of source: every variant is sed-ed from the GUARDED base
# /panel/shim/G.cpp (shipped shim + patch_guard67.py), not from the shipped shim itself.
# Binds (set by submit67.sbatch):
#   $MAIN -> /workspace   $WORK -> /panel   $OUT -> /out   $TREE -> /Dataset/SfS_pp
# Usage: bash /panel/arm67.sh ARM FLOOR INCDIR NP EXTRA [BASE]
#   BASE  source to sed the floor into (default /panel/shim/G.cpp; job 2 passes the instrumented GA.cpp)
set -uo pipefail
ARM=$1; FLOOR=$2; INC=$3; NP=$4; EXTRA=$5
if [ "$EXTRA" = "NONE" ]; then EXTRA=""; fi

BUILD=/panel/build
D=$BUILD/CMakeFiles/Hierarchy-Clear.dir
SHIPPED=/workspace/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp
BASE=${6:-/panel/shim/G.cpp}
mkdir -p /panel/logs /panel/shim /out/$ARM

echo "########## arm $ARM floor=$FLOOR np=$NP extra=[$EXTRA] $(date -u +%FT%TZ)"
echo "--- hostname: $(hostname)"
echo "--- shipped shim md5: $(md5sum $SHIPPED | cut -d" " -f1)"
echo "--- guarded base md5: $(md5sum $BASE | cut -d" " -f1)"
echo "--- include root: $INC"
grep -n "^#define POT_A\|^#define POT_A_INFRA\|^#define JUGLET_ORIG_BASE" $INC/class/data_path.h || true
echo "--- lane files this arm will read (md5 + dots):"
for f in /Dataset/SfS_pp/Breaklines/*.pcd; do
  n=$(awk "f{print} /^DATA/{f=1}" "$f" | grep -c . || true)
  echo "    $(basename $f) md5=$(md5sum $f | cut -d" " -f1) dots=$n"
done

if [ ! -f $BUILD/CMakeCache.txt ]; then
  echo "--- pristine objects: cold build"
  mkdir -p $BUILD
  cd $BUILD
  cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release > /panel/logs/cmake.log 2>&1
  make -j8 > /panel/logs/make.log 2>&1
  echo "--- pristine objects built rc=$?"
else
  echo "--- pristine objects: reusing $BUILD"
fi
cd $BUILD
cat $D/flags.make > /panel/logs/flags_$ARM.log

VAR=/panel/shim/$ARM.cpp
sed "s/point_\.cols() < 50/point_.cols() < $FLOOR/" $BASE > $VAR
if ! grep -q "point_.cols() < $FLOOR" $VAR; then
  echo "!!!! SED DID NOT APPLY for floor $FLOOR"; exit 3
fi
echo "--- variant md5: $(md5sum $VAR | cut -d" " -f1)"
diff -u $BASE $VAR > /panel/logs/diff_$ARM.txt || true
diff -u $SHIPPED $VAR > /panel/logs/diff_shipped_$ARM.txt || true
echo "--- diff vs guarded base (expect exactly the floor literal, empty at 50):"
cat /panel/logs/diff_$ARM.txt

DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
g++ -I$INC $EXTRA $DEFS $INCS $FLAGS -I/opt/sfs-pristine -I/opt/sfs-pristine/class -c $VAR -o $D/shim_$ARM.o
echo "--- compile rc=$?"

cat $D/link.txt > /panel/logs/link_raw_$ARM.txt
LINK=$(sed -e "s|CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/shim_$ARM.o|" -e "s|/opt/sfs-pristine/build/CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/shim_$ARM.o|" -e "s|-o Hierarchy-Clear|-o Hierarchy-$ARM|" /panel/logs/link_raw_$ARM.txt)
echo "$LINK" > /panel/logs/link_$ARM.txt
eval "$LINK"
echo "--- link rc=$?"
echo "--- binary md5: $(md5sum Hierarchy-$ARM | cut -d" " -f1)  Hierarchy-$ARM"

rm -rf /Dataset/SfS_pp/Result
mkdir -p /Dataset/SfS_pp/Result
t0=$SECONDS
./Hierarchy-$ARM $NP > /panel/logs/binary_$ARM.log 2>&1
rc=$?
dt=$((SECONDS - t0))
echo "--- batch exit=$rc wall=${dt}s"
grep -E "T67-|Total number|Sherd accuracy|Edge accuracy|Result save finish|co-first|End condition|Node Sequence|segmentation" /panel/logs/binary_$ARM.log | tail -14 || true
ls -la --time-style=full-iso /Dataset/SfS_pp/Result/ > /panel/logs/result_ls_$ARM.log || true
cp -pr /Dataset/SfS_pp/Result/. /out/$ARM/ 2>/dev/null || true
rm -rf /Dataset/SfS_pp/Result
echo "########## arm $ARM done rc=$rc wall=${dt}s"
exit $rc
