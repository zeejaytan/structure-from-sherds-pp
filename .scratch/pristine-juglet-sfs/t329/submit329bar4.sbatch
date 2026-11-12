#!/bin/bash
#SBATCH --job-name=t329d4
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=64G
#SBATCH --time=01:00:00
#SBATCH --output=logs/slurm-%j.out
#SBATCH --error=logs/slurm-%j.err
#
# Ticket 329 bar 4: the SAME seed-path diagnostic as bar 2, run with the PATCHED 329 binary on the
# SAME cured tree. Harness = diag329_pairs_329.cpp (takes its windowsize from MinNumFloor(), started
# by MeasureLengthScale(shard)), compiled against objects built from the patched /opt/sfs-pristine
# INSIDE sfs_pristine_assembly_329.sif -- never against the t326 pristine objects.
set -uo pipefail
: "${RUNID:?}"
RUNS=/data/gpfs/projects/punim2657/SfSpp/runs
RUN=$RUNS/$RUNID
T325=$RUNS/20261110-1640_22151d0_t325-juglet
TREES=$RUNS/20261110-2325_123f2ca_t328-trim5/trees/juglet_after
SIFX=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly_329.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
cd "$RUN" || exit 1
mkdir -p logs out work/diag329/tools work/diag329/logs
wait_for_image() { local n=0; while [ ! -s "$SIFX" ] && [ $n -lt 60 ]; do sleep 20; n=$((n+1)); done; [ -s "$SIFX" ]; }
wait_for_image || { echo "missing $SIFX"; exit 9; }
# diag329.sh hardcodes /panel/tools/diag329_pairs.cpp, so the M-aware harness is staged under that name
cp -f "$RUN/diag329_pairs_329.cpp" "$RUN/work/diag329/tools/diag329_pairs.cpp"
cp -f "$RUN/diag329.sh" "$RUN/work/diag329/diag329.sh"
md5sum "$RUN/diag329_pairs_329.cpp" "$RUN/work/diag329/tools/diag329_pairs.cpp" "$RUN/work/diag329/diag329.sh" > "$RUN/out/harness.md5"
cp -a "$T325/work/inc_jug" "$RUN/inc_jug"
echo "[JOB] diag329 bar4 runid=$RUNID job=${SLURM_JOB_ID:-na} start=$(date -u +%FT%TZ) image=$SIFX"
# patched objects: cmake the PATCHED tree inside the 329 image (cold, no reuse of t326 build)
$APT exec --writable-tmpfs --bind "$RUN/work/diag329":/panel --bind /data/gpfs:/data/gpfs \
   "$SIFX" bash -c '
     mkdir -p /panel/build /panel/logs
     cd /panel/build
     cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release > /panel/logs/cmake.log 2>&1
     make -j8 > /panel/logs/make.log 2>&1
     md5sum /opt/sfs-pristine/class/feature_matching.cpp /opt/sfs-pristine/class/data_structure.cpp
   ' | tee "$RUN/out/patched_sources.md5"
grep -q cb7b115045a6277aefd47305353589bc "$RUN/out/patched_sources.md5" || { echo "image is not the 329 patch (feature_matching.cpp md5 wrong)"; exit 8; }
$APT exec --writable-tmpfs --bind "$RUN/work/diag329":/panel --bind "$RUN/inc_jug":/inc:ro --bind "$TREES":/Dataset/SfS_pp:ro \
   "$SIFX" bash /panel/diag329.sh 329_bar4 /inc 20 -DJUGLET_ORIG > "$RUN/out/diag_329_bar4.txt" 2>&1
RC=$?
echo "[JOB] diag329 bar4 rc=$RC $(date -u +%FT%TZ)"
grep -c 'SCALE329' "$RUN/out/diag_329_bar4.txt" || true
grep 'SCALE329' "$RUN/out/diag_329_bar4.txt" | head -3 || true
tail -5 "$RUN/out/diag_329_bar4.txt" || true
exit $RC
