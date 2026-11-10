#!/bin/bash
# t330_audit.sh -- ONE audit run, executed INSIDE sfs_pristine_assembly_326.sif.
# Binds (set by submit330.sbatch): $RUN -> /panel ; $TREE -> /Dataset/SfS_pp (ro)
# Builds /panel/bin/t330-$LABEL from /panel/tools/t330_audit.cpp against the image's
# /opt/sfs-pristine objects (the ticket-326 patched build), then runs it with scale $S.
# Usage: bash /panel/t330_audit.sh S LABEL
set -uo pipefail
S=$1; LABEL=$2
BUILD=/panel/build
D=$BUILD/CMakeFiles/Hierarchy-Clear.dir
SRC=/panel/tools/t330_audit.cpp
INC=/panel/inc_E
BIN=/panel/bin/t330-$LABEL
mkdir -p /panel/logs /panel/bin /panel/out
cd $BUILD || exit 2

echo "########## audit $LABEL S=$S host=$(hostname) $(date -u +%FT%TZ)"
echo "--- source md5: $(md5sum $SRC | cut -d' ' -f1)"
echo "--- include root: $INC"
grep -n "^#define POT_E" $INC/class/data_path.h | head -2

DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
g++ -I$INC -I/opt/sfs-pristine -I/opt/sfs-pristine/class $DEFS $INCS $FLAGS -c $SRC -o $D/t330_$LABEL.o 2> /panel/logs/compile_$LABEL.err
echo "--- compile rc=$?"
grep -c "" /panel/logs/compile_$LABEL.err | sed 's/^/--- compile stderr lines: /'

sed -e "s|CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/t330_$LABEL.o|" \
    -e "s|/opt/sfs-pristine/build/CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/t330_$LABEL.o|" \
    -e "s|-o Hierarchy-Clear|-o $BIN|" $D/link.txt > /panel/logs/link_$LABEL.txt
eval "$(cat /panel/logs/link_$LABEL.txt)"
if [ -x "$BIN" ]; then echo "--- link rc=$? binary=yes"; else echo "--- LINK FAILED"; exit 3; fi

export OMP_NUM_THREADS=2
t0=$SECONDS
$BIN $S > /panel/out/audit_$LABEL.txt 2> /panel/logs/audit_$LABEL.err
rc=$?
echo "--- audit exit=$rc wall=$((SECONDS-t0))s"
head -3 /panel/out/audit_$LABEL.txt
grep -E "^TOTAL|^PRUNED|^DONE" /panel/out/audit_$LABEL.txt | sed 's/^/    /'
tail -3 /panel/logs/audit_$LABEL.err | sed 's/^/    ERR: /'
echo "########## audit $LABEL done rc=$rc"
exit $rc
