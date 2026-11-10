#!/bin/bash
#SBATCH --job-name=t326-build
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=04:00:00
#SBATCH --output=logs/slurm-%j.out
#SBATCH --error=logs/slurm-%j.err
#
# Ticket 326 bar 2/3: build the scale-free assembly image.
# Copy of hpc/sbatch/build_pristine_assembly.sbatch pointed at the ticket-326 definition.
# sfs_pristine_assembly.sif is read-only here: its md5 is recorded before and after and the
# job fails if it moved. Everything the build needs is staged into this run directory first,
# so the %files source (a bare filename) is unambiguous.
#
# Usage: sbatch submit.sbatch            (STAGE_SRC defaults to the canonical checkout)
#        STAGE_SRC=<run>/hpc sbatch ...  (this ticket: the branch is not pushed, so the def
#                                         and the patch were copied into <run>/hpc/{containers,patches})
set -uo pipefail

RUN=$SLURM_SUBMIT_DIR
MAIN=/data/gpfs/projects/punim2657/sfs_main
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
STAGE_SRC=${STAGE_SRC:-$MAIN/sfspreproc-docker/hpc}
DEF_SRC=$STAGE_SRC/containers/container-pristine-assembly-326.def
PATCH_SRC=$STAGE_SRC/patches/326-scale-free-matching.patch
DEF=$RUN/container-pristine-assembly-326.def
PATCH=$RUN/326-scale-free-matching.patch
SIF=$MAIN/sfs_pristine_assembly_326.sif
PRISTINE=$MAIN/sfs_pristine_assembly.sif
EXPECT_FM_MD5=2471f1ecde47d8e41e5fa31a2502837a

cd "$RUN" || exit 1
mkdir -p logs

echo "[JOB] $(basename "$RUN") job=${SLURM_JOB_ID:-na} start=$(date -u +%FT%TZ)"
echo "[HOST] $(hostname) nproc=$(nproc) stage_src=$STAGE_SRC"

test -s "$DEF_SRC"   || { echo "[FATAL] missing def $DEF_SRC";   exit 2; }
test -s "$PATCH_SRC" || { echo "[FATAL] missing patch $PATCH_SRC"; exit 2; }
cp -f "$DEF_SRC" "$DEF"
cp -f "$PATCH_SRC" "$PATCH"
echo "[STAGE] def md5   $(md5sum "$DEF" | cut -d' ' -f1)"
echo "[STAGE] patch md5 $(md5sum "$PATCH" | cut -d' ' -f1)"

# --- the pristine image must not move -------------------------------------
PRISTINE_BEFORE=$(md5sum "$PRISTINE" | cut -d' ' -f1)
echo "[PRISTINE] md5 before: $PRISTINE_BEFORE"
test -z "${PRISTINE_BEFORE:-}" && { echo "[FATAL] cannot read $PRISTINE"; exit 3; }
if [ -f "$SIF" ]; then
  echo "[SIF] previous 326 image present, md5 $(md5sum "$SIF" | cut -d' ' -f1); removing"
  rm -f "$SIF" || exit 3
fi

# --- build ----------------------------------------------------------------
echo "[BUILD] apptainer build --fakeroot $SIF $DEF"
$APT build --fakeroot "$SIF" "$DEF" > "logs/build_${SLURM_JOB_ID:-na}.log" 2>&1
RC=$?
echo "[BUILD] rc=$RC  log=logs/build_${SLURM_JOB_ID:-na}.log"
tail -n 40 "logs/build_${SLURM_JOB_ID:-na}.log"

if [ "$RC" -ne 0 ]; then
  echo "[FATAL] image build failed rc=$RC"
  exit 4
fi
test -s "$SIF" || { echo "[FATAL] no image at $SIF"; exit 4; }

# --- provenance -----------------------------------------------------------
echo "[SIF] 326 image md5: $(md5sum "$SIF" | cut -d' ' -f1)"
PRISTINE_AFTER=$(md5sum "$PRISTINE" | cut -d' ' -f1)
echo "[PRISTINE] md5 after:  $PRISTINE_AFTER"
if [ "$PRISTINE_BEFORE" != "$PRISTINE_AFTER" ]; then
  echo "[FATAL] sfs_pristine_assembly.sif changed during the build"; exit 5
fi
FAIL=0
$APT exec "$SIF" cat /opt/sfs-pristine/PATCH326-BUILD.txt || FAIL=1
FM=$($APT exec "$SIF" md5sum /opt/sfs-pristine/class/feature_matching.cpp | cut -d' ' -f1)
echo "[PROOF] patched feature_matching.cpp md5 = $FM (expected $EXPECT_FM_MD5)"
[ "$FM" = "$EXPECT_FM_MD5" ] || { echo "[FATAL] patched source md5 mismatch"; FAIL=1; }
N=$($APT exec "$SIF" sh -c "grep -ac SCALE326 /opt/sfs-pristine/build/Hierarchy-Clear")
echo "[PROOF] marker strings in /opt/sfs-pristine/build/Hierarchy-Clear: $N"
[ "${N:-0}" -ge 1 ] || { echo "[FATAL] SCALE326 marker not compiled into the binary"; FAIL=1; }
$APT exec "$SIF" sh -c 'ls -lh /opt/sfs-pristine/build/Hierarchy-Clear'
$APT test "$SIF" > "logs/test_${SLURM_JOB_ID:-na}.log" 2>&1
echo "[TEST] apptainer test rc=$?"
tail -n 20 "logs/test_${SLURM_JOB_ID:-na}.log"

echo "[JOB] complete rc_fail=$FAIL at $(date -u +%FT%TZ)"
exit $FAIL
