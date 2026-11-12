#!/bin/bash
# 329 bar-3 first half (queue-free compile proof): apply the patch to the image's own pinned
# pristine tree and prove every patched translation unit compiles with the image's own flags.
set -uo pipefail
RUN=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0245_20b52bf_t329-compile
mkdir -p "$RUN"
PATCHSRC=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0230_20b52bf_t329-build/hpc/patches/329-scale-aware-seed-floor.patch
cp -f "$PATCHSRC" "$RUN/329-scale-aware-seed-floor.patch"
mkdir -p "$RUN/src"
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
SIF=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly.sif
cd ~ || exit 1
$APT exec --writable-tmpfs --bind "$RUN":/work "$SIF" bash -c '
set -uo pipefail
cd /work || exit 1
echo "=== image: $(cat /etc/os-release | head -1)"
echo "=== g++: $(g++ --version | head -1)"
# 1. the pinned pristine tree, straight out of the image (build/ excluded: we want sources)
rm -rf /work/src; mkdir -p /work/src
tar -C /opt/sfs-pristine --exclude=build -cf - . 2>/dev/null | tar -C /work/src -xf -
cd /work/src || exit 2
git config --global --add safe.directory /work/src
echo "=== pinned commit: $(git rev-parse HEAD)"
echo "=== tree clean before patch: $(git status --porcelain | wc -l) entries"
# 2. apply exactly as the image definition does
echo "=== git apply --check"
git apply --check --verbose /work/329-scale-aware-seed-floor.patch
echo "check_rc=$?"
echo "=== git apply"
git apply --verbose /work/329-scale-aware-seed-floor.patch
echo "apply_rc=$?"
echo "=== post-patch status"
git status --porcelain | tee /work/git_status_after_patch.txt
echo "changed_files=$(git status --porcelain | wc -l)"
echo "=== md5 of the six patched files"
md5sum main.cpp class/data_structure.h class/data_structure.cpp class/feature_matching.cpp class/ranking_system.cpp class/reconstruction.cpp | tee /work/patched_md5.txt
# 3. compile every patched translation unit with the flags the image itself built with
D=/opt/sfs-pristine/build/CMakeFiles/Hierarchy-Clear.dir
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v GOOGLE_GLOG_DLL_DECL | tr "\n" " ")
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2-)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2-)
echo "=== flags: $FLAGS"
echo "=== includes: $INCS"
rc_all=0
for f in class/data_structure.cpp class/feature_matching.cpp class/reconstruction.cpp class/ranking_system.cpp main.cpp; do
  echo "--- compile(-fsyntax-only) $f"
  g++ -fsyntax-only -I/work/src -I/work/src/class $DEFS $INCS $FLAGS /work/src/$f 2>&1 | tail -20
  rc=${PIPESTATUS[0]}
  echo "    rc=$rc"
  [ "$rc" -eq 0 ] || rc_all=1
done
echo "--- compile(-fsyntax-only) main_pristine_batch_329.cpp (the batch shim the image will run)"
g++ -fsyntax-only -I/work/src -I/work/src/class $DEFS $INCS $FLAGS /work/main_pristine_batch_329.cpp 2>&1 | tail -20
rc=${PIPESTATUS[0]}
echo "    rc=$rc"
[ "$rc" -eq 0 ] || rc_all=1
echo "=== marker counts in the patched tree"
echo "MinNumFloor()   data_structure.h=$(grep -c "int MinNumFloor();" /work/src/class/data_structure.h) data_structure.cpp=$(grep -c "int MinNumFloor()" /work/src/class/data_structure.cpp) feature_matching.cpp=$(grep -c "MinNumFloor()" /work/src/class/feature_matching.cpp) reconstruction.cpp=$(grep -c "MinNumFloor()" /work/src/class/reconstruction.cpp) ranking_system.cpp=$(grep -c "MinNumFloor()" /work/src/class/ranking_system.cpp) main.cpp=$(grep -c "MinNumFloor()" /work/src/main.cpp)"
echo "SCALE329 data_structure.cpp=$(grep -c SCALE329 /work/src/class/data_structure.cpp)"
echo "MeasureLengthScale main.cpp=$(grep -c "MeasureLengthScale(shard);" /work/src/main.cpp) feature_matching.cpp=$(grep -c "MeasureLengthScale(a);" /work/src/class/feature_matching.cpp)"
echo "MINIMUM_NUMBER left: $(grep -rc MINIMUM_NUMBER /work/src/class/*.cpp /work/src/class/*.h /work/src/main.cpp | grep -v ":0" | tr "\n" " ")"
exit $rc_all
'
echo "OUTER_RC=$?"
