#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=64G
#SBATCH --time=04:00:00
#SBATCH --output=logs/slurm-%j.out
#
# submit.sbatch -- ticket 334, one job. Instrument only: no gate or constant in the shipped path is
# changed; the class sources are /opt/sfs-pristine plus ticket 333's gate333.patch (SFS_INLIER_MM
# unset = the shipped inlier gate), i.e. the same binary source that produced ticket 333's numbers.
#
# Part order: (1) copy 333's admitted lists as the reference, (2) build, (3) Pot A control with the
# SHIPPED binary, (4) the diagnostic pair-stage dump on the 3x juglet tree, (5) the same dump on
# Pot A, (6) the fidelity comparison of (4) against (1). Any failed stage aborts the job.
set -uo pipefail

RUN=/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1735_8560cae_t334-upsidedown
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF=$MAIN/sfs_pristine_assembly.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
R333=/data/gpfs/projects/punim2657/SfSpp/runs/20261112-1238_55473e9_t333-juglet-large
FLOOR=20
BIND="--bind /data/gpfs:/data/gpfs --bind $MAIN:/workspace --bind $RUN:/panel"

mkdir -p "$RUN"/logs "$RUN"/out "$RUN"/work "$RUN"/score "$RUN"/shim

echo "=== job $SLURM_JOB_ID on $(hostname) at $(date -u +%FT%TZ)"
echo "=== run dir $RUN ; git7 8560cae ; floor $FLOOR"

for v in SFS_INLIER_MM SFS_LCS_WINDOW POT_NAME JUGLET_ORIG; do unset "$v" 2>/dev/null || true; done

# ---------------------------------------------------------------- 0. the reference (read-only)
grep '^\[T333-PAIR\]' "$R333"/logs/binary_arm1_r1.log | sed 's/^\[T333-PAIR\] //' > "$RUN"/work/ref333_arm1.txt
grep '^\[T333-PAIR\]' "$R333"/logs/binary_arm1_r2.log | sed 's/^\[T333-PAIR\] //' > "$RUN"/work/ref333_arm1_r2.txt
grep '^\[T333-PAIR\]' "$R333"/logs/binary_potA_r1.log | sed 's/^\[T333-PAIR\] //' > "$RUN"/work/ref333_pota.txt
grep '^\[T333-CLUSTER' "$R333"/logs/binary_arm1_r1.log > "$RUN"/work/ref333_clusters_arm1.txt
( cd "$R333"/work/trees/px3 && md5sum juglet/*/* ) > "$RUN"/logs/tree_juglet_before.md5 2>/dev/null
echo "=== tree files pinned: $(wc -l < "$RUN"/logs/tree_juglet_before.md5)"
echo "=== reference lines: arm1_r1 $(wc -l < "$RUN"/work/ref333_arm1.txt) arm1_r2 $(wc -l < "$RUN"/work/ref333_arm1_r2.txt) potA_r1 $(wc -l < "$RUN"/work/ref333_pota.txt)"

# ---------------------------------------------------------------- 1. shipped source, unpatched gates
rm -rf "$RUN"/psrc "$RUN"/build "$RUN"/asm "$RUN"/inc_jug "$RUN"/inc_pota
mkdir -p "$RUN"/psrc "$RUN"/inc_jug/class "$RUN"/inc_pota/class "$RUN"/asm/hpc/tools
$APT exec --writable-tmpfs $BIND "$SIF" /bin/bash -c 'cd /opt/sfs-pristine && cp -r class CMakeLists.txt *.cpp /panel/psrc/ && echo SRC_COPIED' || { echo "FAIL copy pristine source"; exit 3; }
( cd "$RUN"/psrc && patch -p1 < "$RUN"/work/gate333.patch ) || { echo "FAIL gate333 patch"; exit 3; }
# include roots and the shim are ticket 333's own files (same trees, same code path)
cp "$R333"/inc_jug/class/data_path.h "$RUN"/inc_jug/class/data_path.h || exit 3
cp "$R333"/inc_pota/class/data_path.h "$RUN"/inc_pota/class/data_path.h || exit 3
cp "$R333"/asm/hpc/tools/main_pristine_batch.cpp "$RUN"/asm/hpc/tools/main_pristine_batch.cpp || exit 3
echo "=== md5 of what we build:"
md5sum "$RUN"/psrc/class/reconstruction.cpp "$RUN"/psrc/class/feature_matching.cpp "$RUN"/asm/hpc/tools/main_pristine_batch.cpp "$RUN"/inc_jug/class/data_path.h "$RUN"/inc_pota/class/data_path.h
echo "=== gates in this source (each must read the shipped literal):"
grep -n "0.436\|score_ > 1.5\|isSimilar(0.175, 20)\|MINIMUM_NUMBER 6" "$RUN"/psrc/class/feature_matching.cpp "$RUN"/psrc/class/reconstruction.h | head -20

# ---------------------------------------------------------------- 2. build
$APT exec --writable-tmpfs $BIND "$SIF" /bin/bash -c 'mkdir -p /panel/build && cd /panel/build && cmake /panel/psrc -DCMAKE_BUILD_TYPE=Release > /panel/logs/cmake.log 2>&1 && make -j8 > /panel/logs/make.log 2>&1'
RC_B=$?
echo "=== build rc=$RC_B"
[ $RC_B -eq 0 ] || { echo "FAIL build"; tail -30 "$RUN"/logs/make.log; exit 3; }

# ---------------------------------------------------------------- 3. CONTROL: shipped binary on Pot A
rm -rf "$RUN"/work/trees/pota_ctl; mkdir -p "$RUN"/work/trees
cp -a "$R333"/work/trees/px3/pota "$RUN"/work/trees/pota_ctl || exit 3
mkdir -p "$RUN"/out/potA_ctl
T0=$SECONDS
$APT exec --writable-tmpfs $BIND --bind "$RUN"/out/potA_ctl:/out --bind "$RUN"/work/trees/pota_ctl:/Dataset/SfS_pp "$SIF" /bin/bash -c "export OMP_NUM_THREADS=4; /bin/bash /panel/work/arm334.sh potA_ctl $FLOOR /panel/inc_pota 8 NONE" > "$RUN"/logs/run_potA_ctl.log 2>&1
RC_C=$?
C=$($PY "$RUN"/work/checkacc333.py "$RUN"/out/potA_ctl/"1. Acc.txt"); K=$?
echo "=== CONTROL potA_ctl: $C (rc=$K, run rc=$RC_C, wall $((SECONDS-T0))s)"
if [ $K -ne 0 ] || [ $RC_C -ne 0 ]; then
  echo "=== CONTROL FAILED: run is VOID, no diagnostic number is reported"
  echo "VOID control $C" > "$RUN"/score/VOID.txt
  exit 4
fi

# ---------------------------------------------------------------- 4. diag on the 3x juglet tree (read-only)
T0=$SECONDS
$APT exec --writable-tmpfs $BIND --bind "$R333"/work/trees/px3/juglet:/Dataset/SfS_pp:ro "$SIF" /bin/bash -c "bash /panel/work/diag334.sh juglet /panel/inc_jug $FLOOR -DJUGLET_ORIG" > "$RUN"/logs/diag_juglet.log 2>&1
RC_J=$?
echo "=== diag juglet rc=$RC_J wall $((SECONDS-T0))s"
grep -c '^CAND' "$RUN"/logs/diag_juglet.log || true
[ $RC_J -eq 0 ] || { echo "FAIL diag juglet"; tail -30 "$RUN"/logs/diag_juglet.log; exit 5; }

# ---------------------------------------------------------------- 5. diag on Pot A (read-only)
T0=$SECONDS
$APT exec --writable-tmpfs $BIND --bind "$R333"/work/trees/px3/pota:/Dataset/SfS_pp:ro "$SIF" /bin/bash -c "bash /panel/work/diag334.sh pota /panel/inc_pota $FLOOR NONE" > "$RUN"/logs/diag_pota.log 2>&1
RC_P=$?
echo "=== diag pota rc=$RC_P wall $((SECONDS-T0))s"
[ $RC_P -eq 0 ] || { echo "FAIL diag pota"; tail -30 "$RUN"/logs/diag_pota.log; exit 5; }

# ---------------------------------------------------------------- 6. fidelity
$PY "$RUN"/work/fidelity334.py --ref "$RUN"/work/ref333_arm1.txt --log "$RUN"/logs/diag_juglet.log --out "$RUN"/logs/fidelity_juglet.txt
RC_F=$?
$PY "$RUN"/work/fidelity334.py --ref "$RUN"/work/ref333_pota.txt --log "$RUN"/logs/diag_pota.log --out "$RUN"/logs/fidelity_pota.txt
RC_F2=$?
echo "=== fidelity juglet rc=$RC_F pota rc=$RC_F2"
cat "$RUN"/logs/fidelity_juglet.txt
cat "$RUN"/logs/fidelity_pota.txt

( cd "$R333"/work/trees/px3 && md5sum juglet/*/* ) > "$RUN"/logs/tree_juglet_after.md5 2>/dev/null
if diff -q "$RUN"/logs/tree_juglet_before.md5 "$RUN"/logs/tree_juglet_after.md5 > /dev/null; then
  echo "=== tree unchanged by the job: yes"
else
  echo "=== TREE CHANGED BY THE JOB (read-only bind failed):"
  diff "$RUN"/logs/tree_juglet_before.md5 "$RUN"/logs/tree_juglet_after.md5 | head -20
fi
$PY "$RUN"/work/summarize334.py --run "$RUN" > "$RUN"/logs/summary334.txt 2>&1 || echo "!!! summarize failed"
echo "=== summary:"
cat "$RUN"/logs/summary334.txt
echo "=== job done at $(date -u +%FT%TZ)"