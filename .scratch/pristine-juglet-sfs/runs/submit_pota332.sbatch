#!/bin/bash
#SBATCH --job-name=t332-pota
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=64G
#SBATCH --time=01:30:00
#SBATCH --output=/data/gpfs/projects/punim2657/SfSpp/runs/20261112-0245_30b80ae_t332-dense-window12/logs/slurm-pota-%j.out
#SBATCH --error=/data/gpfs/projects/punim2657/SfSpp/runs/20261112-0245_30b80ae_t332-dense-window12/logs/slurm-pota-%j.out
set -uo pipefail

RUN=${SLURM_SUBMIT_DIR}
DS=/data/gpfs/projects/punim2657/sfs_preprocessing/Dataset
CORP=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF=${MAIN}/sfs_pristine_assembly.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
BLOCK=/data/gpfs/projects/punim2657/SfSpp/runs/20261101-1913_d8f716f_t58-floor-density-panel/work/juglet_orig_block.inc
HELP=${RUN}/work
WORK=${RUN}/work/pota
TREES=${WORK}/trees
OUT=${RUN}/out
SC=${RUN}/score
FLOOR=20
NPP=8
JOBID=${SLURM_JOB_ID:-na}

for v in $(compgen -e | grep -E "^(SFS_|POT_NAME$)"); do unset "$v"; done
cd "$RUN" || exit 1
for f in patch_guard67.py mk_inc.py stage67_inputs.py acc305.py score58.py arm67.sh; do
  [ -f "$HELP/$f" ] || { echo "[JOB] FATAL helper missing $HELP/$f"; exit 2; }
done
mkdir -p "$WORK/shim" "$WORK/asm" "$OUT" "$TREES" "$SC" logs
cp -f "$HELP/arm67.sh" "$WORK/arm67.sh"
echo "[JOB] runid=$(basename "$RUN") lane=POT_A-only job=$JOBID host=$(hostname) start=$(date -u +%FT%TZ)"

# the same patched shim (window variable, default 6) that the juglet arms use
tar -xf src/asm_src.tar -C "$WORK/asm" || exit 2
( cd "$WORK/asm" && patch -p1 --batch < "$RUN/src/asm_patch.diff" ) > logs/patch_asm_pota.log 2>&1 || { echo "[JOB] FATAL patch"; cat logs/patch_asm_pota.log; exit 3; }
SHIM=${WORK}/asm/hpc/tools/main_pristine_batch.cpp
for f in "$SHIM" "$WORK/asm/main.cpp"; do
  grep -q 'LCS window: ' "$f" || { echo "[JOB] FATAL no window banner in $f"; exit 3; }
  grep -q 'SFS_LCS_WINDOW' "$f" || { echo "[JOB] FATAL no env read in $f"; exit 3; }
  grep -q 'FeatureComp(shard, LCS_out, 25, lcs_window, 0)' "$f" || { echo "[JOB] FATAL call not switched in $f"; exit 3; }
done
md5sum "$SHIM" "$WORK/asm/main.cpp" | tee logs/shim_pota.md5
$PY "$HELP/patch_guard67.py" "$SHIM" "$WORK/shim/G.cpp" > logs/guard_pota.log 2>&1 || { echo "[JOB] FATAL guard"; cat logs/guard_pota.log; exit 4; }
grep -c "T67-" "$WORK/shim/G.cpp"

# Pot A include root: activate exactly #define POT_A, append nothing
$APT exec "$SIF" cat /opt/sfs-pristine/class/data_path.h > "$WORK/inc_src_data_path.h" 2>logs/provenance_pota.log
mkdir -p "$WORK/inc_pota/class"
$PY "$HELP/mk_inc.py" "$WORK/inc_src_data_path.h" "$WORK/inc_pota/class/data_path.h" A > logs/mk_inc_pota.log 2>&1 || { echo "[JOB] FATAL mk_inc"; cat logs/mk_inc_pota.log; exit 6; }
cat logs/mk_inc_pota.log
grep -n '^#define POT_' "$WORK/inc_pota/class/data_path.h"

$PY "$HELP/stage67_inputs.py" --lanes "$RUN/out/gt/geom18" --dataset "$DS" --pota "$CORP" --out "$TREES/basepota" > logs/stage_pota.log 2>&1 || { echo "[JOB] FATAL stage"; tail -5 logs/stage_pota.log; exit 7; }
for r in 1 2; do
  rm -rf "$TREES/potA_r$r"
  cp -a "$TREES/basepota/pota" "$TREES/potA_r$r"
  ls "$TREES/potA_r$r" | tr '\n' ' '; echo
  ls "$TREES/potA_r$r/Axes" | head -3
done

$APT exec --writable-tmpfs \
   --bind /data/gpfs:/data/gpfs --bind "$MAIN":/workspace --bind "$WORK":/panel \
   "$SIF" bash -c '
     mkdir -p /panel/build /panel/logs
     cd /panel/build
     cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release > /panel/logs/cmake.log 2>&1
     make -j8 > /panel/logs/make.log 2>&1
     echo build_rc=$?
   ' || exit 9

export OMP_NUM_THREADS=4

runpot() {
  local LABEL=$1 TREE=$2 NPX=$3
  local T0=$SECONDS
  echo "##### LAUNCH $LABEL tree=$TREE np=$NPX window=[unset] $(date -u +%FT%TZ)"
  $APT exec --writable-tmpfs \
     --bind /data/gpfs:/data/gpfs --bind "$MAIN":/workspace --bind "$WORK":/panel --bind "$OUT":/out --bind "$TREE":/Dataset/SfS_pp \
     "$SIF" bash /panel/arm67.sh "$LABEL" "$FLOOR" /panel/inc_pota "$NPX" NONE > "logs/run_${LABEL}.log" 2>&1
  local RC=$? DT=$((SECONDS - T0))
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "$LABEL" "$FLOOR" /panel/inc_pota "$TREE" "$NPX" unset "$RC" "$DT" > "logs/panel_${LABEL}.row"
  local WLINE
  WLINE=$(grep -m1 "LCS window: " "$WORK/logs/binary_${LABEL}.log" 2>/dev/null | tr -d '\r')
  echo "##### DONE $LABEL exit=$RC wall=${DT}s  ${WLINE:-<no LCS window line>}"
  echo "${WLINE}" > "logs/window_${LABEL}.txt"
  if [ -d "$OUT/$LABEL" ]; then
    $PY "$HELP/acc305.py" "$OUT/$LABEL" "$LABEL" "$SC/acc_${LABEL}.row" >> "logs/acc_${LABEL}.log" 2>&1
    [ -f "$OUT/$LABEL/1. Acc.txt" ] && cp -f "$OUT/$LABEL/1. Acc.txt" "$SC/$LABEL.Acc.txt"
  fi
}

runpot potA_r1 "$TREES/potA_r1" "$NPP"
runpot potA_r2 "$TREES/potA_r2" "$NPP"

echo "[JOB] complete at $(date -u +%FT%TZ)"