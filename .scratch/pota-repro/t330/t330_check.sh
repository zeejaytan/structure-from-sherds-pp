#!/bin/bash
# compile+link only, no run -- login-node sanity check for the audit tool
set -uo pipefail
B=/panel/build; D=$B/CMakeFiles/Hierarchy-Clear.dir; SRC=/panel/tools/t330_audit.cpp; INC=/panel/inc_E
BIN=/panel/bin/t330-DRYRUN
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
echo "DEFS=[$DEFS]"
cd $B || exit 2
g++ -I$INC -I/opt/sfs-pristine -I/opt/sfs-pristine/class $DEFS $INCS $FLAGS -c $SRC -o $D/t330_DRYRUN.o
echo "compile rc=$?"
sed -e "s|CMakeFiles/Hierarchy-Clear.dir/main.cpp.o|CMakeFiles/Hierarchy-Clear.dir/t330_DRYRUN.o|" -e "s|-o Hierarchy-Clear|-o $BIN|" $D/link.txt > /tmp/link_dry.sh
bash /tmp/link_dry.sh
echo "link rc=$?"
ls -la $BIN 2>/dev/null && echo BINARY_OK
