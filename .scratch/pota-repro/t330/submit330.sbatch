#!/bin/bash
# Ticket 330: gate-flip audit on Pot E lane MOR1 (and K3R1 as a cross-check).
# Two regimes of the SAME patched library, selected by g_len_scale at runtime:
#   S=60.00  -> Gate(x)=x   (pristine literals exactly; MeasureLengthScale no-ops)
#   S=92.77  -> Gate(x)=1.546x (the ticket-326 condition measured on Pot E)
# No source changes, no threshold tuning: the library's own PairwisePruning decides.
#SBATCH --job-name=t330-audit
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=02:00:00
#SBATCH --output=logs/slurm-%j.out
#SBATCH --error=logs/slurm-%j.err
set -uo pipefail

RUN=$SLURM_SUBMIT_DIR
T326=/data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE
SIF=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly_326.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
mkdir -p "$RUN/logs" "$RUN/out" "$RUN/bin"
echo "t330 audit start $(date -u +%FT%TZ) job=$SLURM_JOB_ID node=$(hostname)"
echo "sif=$SIF md5=$(md5sum $SIF | cut -d' ' -f1)"

if [ ! -f "$RUN/build/CMakeCache.txt" ]; then
  echo "--- cold build"
  $APT exec --writable-tmpfs --bind "$RUN:/panel" "$SIF" bash -c '
    mkdir -p /panel/build /panel/logs
    cd /panel/build
    cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release > /panel/logs/cmake.log 2>&1
    make -j8 > /panel/logs/make.log 2>&1
    echo "build rc=$?"
  ' || { echo "BUILD FAILED"; touch "$RUN/RUN-FAILED.txt"; exit 5; }
fi
grep -c "" "$RUN/logs/make.log" | sed 's/^/--- make.log lines: /'

one() {  # one LABEL S TREE
  local LABEL=$1 S=$2 TREE=$3
  $APT exec --writable-tmpfs \
    --bind "$RUN:/panel" \
    --bind "$TREE:/Dataset/SfS_pp:ro" \
    "$SIF" bash /panel/t330_audit.sh "$S" "$LABEL" > "logs/run_$LABEL.log" 2>&1
  echo "--- $LABEL exit=$?"
}

MOS=$T326/trees/E_MOR1/E/MO
K3S=$T326/trees/E_K3R1/E/MM

one MOR1_S60   60.00  "$MOS" &
one MOR1_S9277 92.77  "$MOS" &
wait
one K3R1_S60   60.00  "$K3S" &
one K3R1_S9277 92.77  "$K3S" &
wait

for f in MOR1_S60 MOR1_S9277 K3R1_S60 K3R1_S9277; do
  printf '%s\t' "$f"
  grep -E "^TOTAL|^PRUNED|^DONE" "out/audit_$f.txt" 2>/dev/null | tr '\n' ' '
  echo
done
echo "t330 audit done $(date -u +%FT%TZ)"
