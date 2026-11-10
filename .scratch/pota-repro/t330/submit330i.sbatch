#!/bin/bash
# Ticket 330 variant I: the ASSEMBLY itself at two regimes of the SAME patched binary.
# Why: bar 1 asks the harness, rebuilt at pristine constants, to reproduce the t323 baseline on
# Pot E. The audit (variant H) settles the pruning stage; this settles the whole assembly.
#   S=60.00 -> Gate(x)=x exactly; if the 326 patch is scale-only, this must reproduce t323's 62/62
#   S=92.77 -> the ticket-326 condition; must reproduce t326's 46/62
# Both arms run the shipped scorer (main_pristine_batch.cpp + 2-line instrument) against the
# identical E_MOR1 tree, differing only in argv[2]. interactive partition: sapphire could not start
# a 5-minute 1-cpu job for two hours (fairshare 0.137, no backfill estimate); deviation recorded.
#SBATCH --job-name=t330-asm
#SBATCH --account=punim2657
#SBATCH --partition=interactive
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=02:00:00
#SBATCH --output=logs/slurm-%j.out
#SBATCH --error=logs/slurm-%j.err
set -uo pipefail
RUN=$SLURM_SUBMIT_DIR
SIF=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly_326.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
mkdir -p "$RUN/logs" "$RUN/out" "$RUN/bin"
echo "t330 assembly start $(date -u +%FT%TZ) job=$SLURM_JOB_ID node=$(hostname)"
echo "sif=$SIF md5=$(md5sum $SIF | cut -d' ' -f1)"

one() {  # one LABEL S TREE
  local LABEL=$1 S=$2 TREE=$3
  $APT exec --writable-tmpfs \
    --bind "$RUN:/panel" \
    --bind "$TREE:/Dataset/SfS_pp" \
    "$SIF" bash /panel/t330_run.sh "$LABEL" "$S" 31 > "logs/run_$LABEL.log" 2>&1
  echo "--- $LABEL exit=$?"
}

# One arm per job: the site silently redirects interactive jobs that ask for more than
# 8 cpus to sapphire (probed 05:21: a 16-cpu --wrap on -p interactive landed on sapphire,
# an 8-cpu one stayed on interactive), so each regime runs as its own 8-cpu job against
# its own tree copy.
LABEL=$1; [ -z "$LABEL" ] && LABEL=S60
S=$2; [ -z "$S" ] && S=60.00
TREE=$3; [ -z "$TREE" ] && TREE=$RUN/tree_60
echo "one arm: label=$LABEL S=$S tree=$TREE"
one "$LABEL" "$S" "$TREE"
echo "--- $LABEL summary"
grep -E "Sherd accuracy|Edge accuracy|Total number" "logs/run_$LABEL.log"
echo "t330 assembly done $(date -u +%FT%TZ)"