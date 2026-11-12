#!/bin/bash
# 329 bar-3 first half, part 2: syntax-check the patched batch shim with the image's own flags.
set -uo pipefail
RUN=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0245_20b52bf_t329-compile
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly.sif
cd ~ || exit 1
$APT exec --bind "$RUN":/work "$SIF" bash -c '
cd /work/src || exit 2
D=/opt/sfs-pristine/build/CMakeFiles/Hierarchy-Clear.dir
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
echo "=== md5 shim: $(md5sum /work/main_pristine_batch_329.cpp)"
echo "=== the three edited lines:"
grep -n "MeasureLengthScale(shard);\|MinNumFloor()\|FeatureComp(shard, LCS_out" /work/main_pristine_batch_329.cpp
echo "--- compile(-fsyntax-only) main_pristine_batch_329.cpp"
g++ -fsyntax-only -I/work/src -I/work/src/class $DEFS $INCS $FLAGS /work/main_pristine_batch_329.cpp
echo "shim_rc=$?"
'
