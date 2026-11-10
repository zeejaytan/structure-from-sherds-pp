#!/bin/bash
# diag326.sh -- ticket 326 diagnosis: build and run hpc/tools/diag326_pairs.cpp INSIDE an
# assembly image, against the class objects in /panel/build (cmake of /opt/sfs-pristine).
# Binds: $WORK -> /panel (holds build/, tools/diag326_pairs.cpp), $TREE -> /Dataset/SfS_pp:ro
# Usage: bash /panel/diag326.sh LABEL INCDIR FLOOR EXTRA
set -uo pipefail
LABEL=$1; INC=$2; FLOOR=$3; EXTRA=$4
if [ "$EXTRA" = "NONE" ]; then EXTRA=""; fi
BUILD=/panel/build
D=$BUILD/CMakeFiles/Hierarchy-Clear.dir
SRC=/panel/tools/diag326_pairs.cpp
mkdir -p /panel/logs
cd $BUILD || exit 2
echo "########## diag $LABEL floor=$FLOOR inc=$INC extra=[$EXTRA] $(date -u +%FT%TZ)"
echo "--- harness md5: $(md5sum $SRC | cut -d' ' -f1)"
echo "--- feature_matching.cpp md5: $(md5sum /opt/sfs-pristine/class/feature_matching.cpp | cut -d' ' -f1)"
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
g++ -I$INC $EXTRA -DFLOOR=$FLOOR $DEFS $INCS $FLAGS -I/opt/sfs-pristine -I/opt/sfs-pristine/class -c $SRC -o $D/diag_$LABEL.o || { echo "compile failed"; exit 3; }
LINK=$(sed -e "s|CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/diag_$LABEL.o|" -e "s|-o Hierarchy-Clear|-o diag-$LABEL|" $D/link.txt)
eval "$LINK" || { echo "link failed"; exit 4; }
./diag-$LABEL
rc=$?
echo "########## diag $LABEL done rc=$rc"
exit $rc
