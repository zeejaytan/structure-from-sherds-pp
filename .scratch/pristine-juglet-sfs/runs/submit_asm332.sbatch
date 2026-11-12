#!/bin/bash
#SBATCH --job-name=t332-asm
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=64G
#SBATCH --time=04:00:00
#SBATCH --output=logs/slurm-asm-%j.out
#SBATCH --error=logs/slurm-asm-%j.err
#
# Ticket 332, Parts B and C: the LCS window switch and the three arms x two repeats.
#
# One job per CHAIN CELL (submit with  sbatch --export=ALL,CELL=gt,PFX=gt  submit_asm332.sbatch):
#   CELL=gt  lanes from the chain cell whose SFS_AXIS_FILE was Dataset/Juglet_GT  (the axis the
#            cured-tree baseline t328 gt_jug / t329 control 34710135 was assembled on)
#   CELL=ds  lanes from the chain cell whose SFS_AXIS_FILE was Dataset/Axes/Juglet (the standard
#            recipe: our shipped September PotSAC axes)
# In both jobs the assembler tree axes are the GT axes, so within a cell the three arms differ in
# nothing but the lanes and the window; between cells they differ in the axis the MESH STAGE was
# given, which is reported as a separate factor, not folded into the pre-registered reading.
#
# Arms, per cell, each twice (the assembler is not deterministic):
#   sparse  = chain arm 18 lanes (shipped 1.8 mm walk), SFS_LCS_WINDOW unset -> 6, today's behaviour
#   dense   = chain arm 09 lanes (0.9 mm walk),          SFS_LCS_WINDOW unset -> 6
#   dense12 = chain arm 09 lanes (0.9 mm walk),          SFS_LCS_WINDOW=12
# The Pot A neutrality runs (window unset) belong to the CELL=gt job only: the corpus tree does not
# depend on the juglet lanes.
# Derived from runs/20261110-1640_22151d0_t325-juglet/submit.sbatch.
set -uo pipefail

RUN=${SLURM_SUBMIT_DIR}
CELL=${CELL:-}
PFX=${PFX:-${CELL:-}}
DS=/data/gpfs/projects/punim2657/sfs_preprocessing/Dataset
GTAX=${DS}/Juglet_GT
CORP=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF=${MAIN}/sfs_pristine_assembly.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
BLOCK=/data/gpfs/projects/punim2657/SfSpp/runs/20261101-1913_d8f716f_t58-floor-density-panel/work/juglet_orig_block.inc
LANES18=${RUN}/out/${CELL}/geom18
LANES09=${RUN}/out/${CELL}/geom09

HELP=${RUN}/work
for f in patch_guard67.py mk_inc.py stage67_inputs.py acc305.py score58.py arm67.sh; do
  [ -f "$HELP/$f" ] || { echo "[JOB] FATAL helper missing $HELP/$f"; exit 2; }
done
WORK=${RUN}/work/${CELL}
TREES=${WORK}/trees
OUT=${RUN}/out
SC=${RUN}/score
FLOOR=20
NP=9
NPP=8
JOBID=${SLURM_JOB_ID:-na}
DOPOT=no
[ "$CELL" = gt ] && DOPOT=yes

for v in $(compgen -e | grep -E "^(SFS_|POT_NAME$)"); do unset "$v"; done
cd "$RUN" || exit 1
mkdir -p "$WORK/shim" "$WORK/asm" "$OUT" "$TREES" "$SC" logs
cp -f "$HELP/arm67.sh" "$WORK/arm67.sh"   # arm67.sh must sit at /panel inside the bind
[ -f "$WORK/arm67.sh" ] || { echo "[JOB] FATAL arm67.sh not staged"; exit 2; }
echo "[JOB] runid=$(basename "$RUN") cell=$CELL pfx=$PFX pot=$DOPOT job=$JOBID host=$(hostname) start=$(date -u +%FT%TZ)"
[ -d "$LANES18" ] || { echo "[JOB] FATAL no lanes $LANES18"; exit 2; }
[ -d "$LANES09" ] || { echo "[JOB] FATAL no lanes $LANES09"; exit 2; }

# ---------------------------------------------------------------- the patched assembler source
tar -xf src/asm_src.tar -C "$WORK/asm" || exit 2
grep -q "^+++ b/main.cpp" src/asm_patch.diff || { echo "[JOB] FATAL diff is not the two source files"; exit 3; }
( cd "$WORK/asm" && patch -p1 --batch < "$RUN/src/asm_patch.diff" ) > logs/patch_asm_${CELL}.log 2>&1 || { echo "[JOB] FATAL patch"; cat logs/patch_asm_${CELL}.log; exit 3; }
SHIM=${WORK}/asm/hpc/tools/main_pristine_batch.cpp
for f in "$SHIM" "$WORK/asm/main.cpp"; do
  grep -q 'LCS window: ' "$f" || { echo "[JOB] FATAL no window banner in $f"; exit 3; }
  grep -q 'SFS_LCS_WINDOW' "$f" || { echo "[JOB] FATAL no env read in $f"; exit 3; }
  grep -q 'FeatureComp(shard, LCS_out, 25, lcs_window, 0)' "$f" || { echo "[JOB] FATAL call not switched in $f"; exit 3; }
done
md5sum "$SHIM" "$WORK/asm/main.cpp" | tee logs/shim332_${CELL}.md5
grep -n "lcs_window\|LCS window: \|SFS_LCS_WINDOW" "$SHIM" | head -12

# guard (T67 markers) on top of the patched shim -- the arm helper's default BASE
$PY "$HELP/patch_guard67.py" "$SHIM" "$WORK/shim/G.cpp" > logs/guard_${CELL}.log 2>&1 || { echo "[JOB] FATAL guard"; cat logs/guard_${CELL}.log; exit 4; }
grep -c "T67-" "$WORK/shim/G.cpp"

# ------------------------------------------------------------------------------ the trees
mkdir -p "$WORK/inc_jug/class"
$APT exec "$SIF" cat /opt/sfs-pristine/class/data_path.h > "$WORK/inc_src_data_path.h" 2>logs/provenance.log
$PY "$HELP/mk_inc.py" "$WORK/inc_src_data_path.h" "$WORK/inc_jug/class/data_path.h" NONE --append "$BLOCK" > logs/mk_inc_${CELL}.log 2>&1 || { echo "[JOB] FATAL mk_inc"; cat logs/mk_inc_${CELL}.log; exit 6; }
grep -c '^#define POT_' "$WORK/inc_jug/class/data_path.h" > logs/inc_counts_${CELL}.txt 2>&1 || true
grep -n '^#define JUGLET_ORIG_BASE' "$WORK/inc_jug/class/data_path.h" | head -2

$PY "$HELP/stage67_inputs.py" --lanes "$LANES18" --dataset "$DS" --pota "$CORP" --out "$TREES/base18" > logs/stage18_${CELL}.log 2>&1 || { echo "[JOB] FATAL stage18"; tail -5 logs/stage18_${CELL}.log; exit 7; }
$PY "$HELP/stage67_inputs.py" --lanes "$LANES09" --dataset "$DS" --pota "$CORP" --out "$TREES/base09" > logs/stage09_${CELL}.log 2>&1 || { echo "[JOB] FATAL stage09"; tail -5 logs/stage09_${CELL}.log; exit 7; }
for i in 1 2 3 4 5 6 7 8 9; do
  P=Juglet_Piece_$i
  cp -f "$GTAX/${P}_Axis.xyz" "$TREES/base18/juglet/Axes/${P}_Axis.xyz"
  cp -f "$GTAX/${P}_Axis.xyz" "$TREES/base09/juglet/Axes/${P}_Axis.xyz"
done
diff -rq "$TREES/base18/juglet" "$TREES/base09/juglet" > logs/treediff_18_09_${CELL}.log 2>&1 || true
echo "[JOB] tree diff base18 vs base09 (expect Breaklines + Surfaces only):"; cat logs/treediff_18_09_${CELL}.log
for f in "$TREES"/base18/juglet/Breaklines/*.pcd; do
  echo "$(basename $f) md5=$(md5sum $f | cut -d' ' -f1) dots=$(awk 'f{print} /^DATA/{f=1}' $f | grep -c .)";
done > logs/lanes_${CELL}.txt
cat logs/lanes_${CELL}.txt

# one copy per repeat so an arm never writes into another arm's Result/
for r in 1 2; do
  rm -rf "$TREES/${PFX}18_r$r" "$TREES/${PFX}09_r$r" "$TREES/${PFX}09w12_r$r" "$TREES/potA_r$r"
  cp -a "$TREES/base18/juglet" "$TREES/${PFX}18_r$r"
  cp -a "$TREES/base09/juglet" "$TREES/${PFX}09_r$r"
  cp -a "$TREES/base09/juglet" "$TREES/${PFX}09w12_r$r"
  if [ "$DOPOT" = yes ]; then cp -a "$TREES/base18/pota" "$TREES/potA_r$r"; fi
done

# ------------------------------------------------------------------------ pristine objects
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

runarm() {   # LABEL TREE NP WINDOW(empty|12)
  local LABEL=$1 TREE=$2 NPX=$3 WIN=$4
  local T0=$SECONDS
  echo "##### LAUNCH $LABEL tree=$TREE np=$NPX window=[${WIN:-unset}] $(date -u +%FT%TZ)"
  if [ -n "$WIN" ]; then
    $APT exec --writable-tmpfs \
       --bind /data/gpfs:/data/gpfs --bind "$MAIN":/workspace --bind "$WORK":/panel --bind "$OUT":/out --bind "$TREE":/Dataset/SfS_pp \
       "$SIF" env "SFS_LCS_WINDOW=$WIN" bash /panel/arm67.sh "$LABEL" "$FLOOR" /panel/inc_jug "$NPX" -DJUGLET_ORIG > "logs/run_${LABEL}.log" 2>&1
  else
    $APT exec --writable-tmpfs \
       --bind /data/gpfs:/data/gpfs --bind "$MAIN":/workspace --bind "$WORK":/panel --bind "$OUT":/out --bind "$TREE":/Dataset/SfS_pp \
       "$SIF" bash /panel/arm67.sh "$LABEL" "$FLOOR" /panel/inc_jug "$NPX" -DJUGLET_ORIG > "logs/run_${LABEL}.log" 2>&1
  fi
  local RC=$? DT=$((SECONDS - T0))
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "$LABEL" "$FLOOR" /panel/inc_jug "$TREE" "$NPX" "${WIN:-unset}" "$RC" "$DT" > "logs/panel_${LABEL}.row"
  local WLINE
  WLINE=$(grep -m1 "LCS window: " "logs/binary_${LABEL}.log" 2>/dev/null | tr -d "\r")
  echo "##### DONE $LABEL exit=$RC wall=${DT}s  ${WLINE:-<no LCS window line>}"
  echo "${WLINE}" > "logs/window_${LABEL}.txt"
  if [ -d "$OUT/$LABEL" ]; then
    $PY "$HELP/acc305.py" "$OUT/$LABEL" "$LABEL" "$SC/acc_${LABEL}.row" >> "logs/acc_${LABEL}.log" 2>&1
    [ -f "$OUT/$LABEL/1. Acc.txt" ] && cp -f "$OUT/$LABEL/1. Acc.txt" "$SC/$LABEL.Acc.txt"
    if [ "$NPX" = "9" ]; then
      $PY "$HELP/score58.py" --result "$OUT/$LABEL" --gt "$DS/Juglet_GT" \
          --mesh "$TREE/Mesh" --lanes "$TREE/Breaklines" \
          --out "$SC/$LABEL.json" --label "$LABEL" > "logs/score_${LABEL}.log" 2>&1
    fi
  fi
}

for r in 1 2; do
  if [ "$DOPOT" = yes ]; then runarm "potA_r$r" "$TREES/potA_r$r" "$NPP" ""; fi
  runarm "${PFX}18_r$r"    "$TREES/${PFX}18_r$r"    "$NP" ""
  runarm "${PFX}09_r$r"    "$TREES/${PFX}09_r$r"    "$NP" ""
  runarm "${PFX}09w12_r$r" "$TREES/${PFX}09w12_r$r" "$NP" "12"
done

printf "label\tfloor\tinc\ttree\tnp\twindow\texit\twall_s\n" > logs/panel_${CELL}.tsv
for arm in $(ls logs/panel_*.row | sed 's|logs/panel_||; s|\.row$||'); do cat "logs/panel_${arm}.row" >> logs/panel_${CELL}.tsv; done
for arm in $(ls "$SC"/acc_*.row 2>/dev/null | sed 's|.*/acc_||; s|\.row$||'); do cat "$SC/acc_${arm}.row" >> "$SC/acc.tsv"; done
cat logs/panel_${CELL}.tsv
cat "$SC/acc.tsv"
for f in logs/window_*.txt; do echo "$f: $(cat "$f")"; done
echo "[JOB] complete at $(date -u +%FT%TZ)"