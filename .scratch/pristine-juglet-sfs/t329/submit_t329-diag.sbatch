#!/bin/bash
#SBATCH --job-name=t329d
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
# Ticket 329 bar 2: seed-path diagnosis on the CURED trees with the PRISTINE binary.
# Same recipe as the t328-trim5 diag phase; harness extended (STAGE table, all 36 pairs,
# killing gate named as file:line on the measured batch path).
set -uo pipefail
: "${RUNID:?}"
RUNS=/data/gpfs/projects/punim2657/SfSpp/runs
RUN=$RUNS/$RUNID
T325=$RUNS/20261110-1640_22151d0_t325-juglet
T326=$RUNS/20261110-1844_6925cb0_t326-diag
TREES=$RUNS/20261110-2325_123f2ca_t328-trim5/trees/juglet_after
SIFA=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
cd "$RUN" || exit 1
mkdir -p logs out work/diag329/tools work/diag329/logs
cp -f "$RUN/diag329_pairs.cpp" "$RUN/work/diag329/tools/diag329_pairs.cpp"
cp -f "$RUN/diag329.sh" "$RUN/work/diag329/diag329.sh"
md5sum "$RUN/work/diag329/tools/diag329_pairs.cpp" "$RUN/work/diag329/diag329.sh" > "$RUN/out/harness.md5"
cp -a "$T325/work/inc_jug" "$RUN/inc_jug"
cp -al "$T326/work/build" "$RUN/work/diag329/build"
echo "[JOB] diag329 bar2 runid=$RUNID job=${SLURM_JOB_ID:-na} start=$(date -u +%FT%TZ)"
$APT exec --writable-tmpfs --bind "$RUN/work/diag329":/panel --bind "$RUN/inc_jug":/inc:ro --bind "$TREES":/Dataset/SfS_pp:ro \
   "$SIFA" bash /panel/diag329.sh 329_bar2 /inc 20 -DJUGLET_ORIG > "$RUN/out/diag_329_bar2.txt" 2>&1
RC=$?
echo "[JOB] diag329 bar2 rc=$RC $(date -u +%FT%TZ)"
tail -5 "$RUN/out/diag_329_bar2.txt" || true
exit $RC
