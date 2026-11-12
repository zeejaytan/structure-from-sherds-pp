#!/bin/bash
# diag334.sh -- ticket 334: build and run hpc/tools/diag334_pairs.cpp INSIDE the assembly image,
# against the class objects in /panel/build (cmake of /panel/psrc = /opt/sfs-pristine + gate333),
# and dump the whole pair stage for one include root.
# Binds (set by submit.sbatch): $RUN -> /panel, $TREE -> /Dataset/SfS_pp (read-only).
# Usage: bash /panel/work/diag334.sh LABEL INCDIR FLOOR EXTRA
set -uo pipefail
LABEL=$1; INC=$2; FLOOR=$3; EXTRA=$4
if [ "$EXTRA" = "NONE" ]; then EXTRA=""; fi
BUILD=/panel/build
D=$BUILD/CMakeFiles/Hierarchy-Clear.dir
SRC=/panel/work/diag334_pairs.cpp
OUT=diag334-$LABEL
mkdir -p /panel/logs
cd $BUILD || exit 2
echo "########## diag334 $LABEL floor=$FLOOR inc=$INC extra=[$EXTRA] $(date -u +%FT%TZ)"
echo "--- harness md5: $(md5sum $SRC | cut -d' ' -f1)"
echo "--- feature_matching.cpp md5: $(md5sum /panel/psrc/class/feature_matching.cpp | cut -d' ' -f1)"
echo "--- reconstruction.cpp md5: $(md5sum /panel/psrc/class/reconstruction.cpp | cut -d' ' -f1)"
echo "--- include root: $INC"
grep -n "^#define POT_A\|^#define JUGLET_ORIG_BASE\|^#define SHARD_NUMBER\|^#define NUM_MIXED_SHERD" $INC/class/data_path.h || true
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
g++ -I$INC $EXTRA -DFLOOR=$FLOOR $DEFS $INCS $FLAGS -I/panel/psrc -I/panel/psrc/class -c $SRC -o $D/diag_$LABEL.o || { echo "compile failed"; exit 3; }
LINK=$(sed -e "s|CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/diag_$LABEL.o|" -e "s|-o Hierarchy-Clear|-o $OUT|" $D/link.txt)
eval "$LINK" || { echo "link failed"; exit 4; }
echo "--- binary md5: $(md5sum $OUT | cut -d' ' -f1)"
./$OUT
rc=$?
echo "########## diag334 $LABEL done rc=$rc"
exit $rc
