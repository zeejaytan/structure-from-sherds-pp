#!/bin/bash
#SBATCH --job-name=t329-pot
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --mem=160G
#SBATCH --time=04:00:00
#SBATCH --output=logs/slurm-%j.out
#SBATCH --error=logs/slurm-%j.err
#
# Ticket 329 bar 6: ten-pot end-to-end with the PATCHED 329 assembler, same recorded bars as bar 1,
# both lanes (MO = authors' released Axes, K3 = our axes329f top-3), two repeats, one pot per job.
# Derived line-for-line from runs/20261110-2248_6925cb0_t326-potA/submit.sbatch with exactly three
# changes: SIF -> sfs_pristine_assembly_329.sif, SHIPPED -> the 329 batch shim, AX_318 -> axes329f;
# plus the SCALE329 capture/assert the bar requires (every pot lane must print s=1.000 M=6).
# Submit with POT/NAMED/NP in the environment, e.g.  POT=D NAMED=29 NP=29 sbatch submit329pot6.sbatch
set -uo pipefail

: "${POT:?set POT}" ; : "${NAMED:?set NAMED}" ; : "${NP:?set NP}"
FLOOR=50
NREP=2

LANES=/data/gpfs/projects/punim2657/SfSpp/runs/20261103-1332_fab76f5_t66-admitsign-default/outU_pota/geom
AX_MO=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Axes
AX_318=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0120_20b52bf_t329-axes/out/axes329f

RUN=$SLURM_SUBMIT_DIR
CORP=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly_329.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
SHIPPED=$RUN/main_pristine_batch_329.cpp
GT="/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Ground Truth"

WORK=$RUN/work
TREES=$RUN/trees
OUT=$RUN/out
SC=$RUN/score
JOBID=${SLURM_JOB_ID:-na}

for v in $(compgen -e | grep -E '^(SFS_|POT_NAME$)'); do unset "$v"; done
cd "$RUN" || exit 1

echo "[JOB] runid=$(basename "$RUN") job=$JOBID pot=$POT named=$NAMED np=$NP floor=$FLOOR start=$(date -u +%FT%TZ)"
echo "[JOB] image=$SIF shim=$SHIPPED md5=$(md5sum "$SHIPPED" | cut -d' ' -f1) axes=$AX_318"

fail() { local rc=$1; shift
  { echo "pot=$POT"; echo "jobid=$JOBID"; echo "rc=$rc"; echo "reason: $*"; } > "$RUN/RUN-FAILED.txt"
  echo "[JOB] FATAL rc=$rc $*"; exit "$rc"; }

mesh_of() { local s=$1; if [ -f "$CORP/Mesh/${s}_Mesh_DS.obj" ]; then echo "$CORP/Mesh/${s}_Mesh_DS.obj"; else echo "$CORP/Mesh/${s}_Mesh.obj"; fi; }
md5_inputs() { local out=$1 p stem
  { for p in $(seq 1 "$NAMED"); do
      stem=$(printf "Pot_%s_Piece_%02d" "$POT" "$p")
      for f in "$(mesh_of "$stem")" "$CORP/Axes/${stem}_Axis.xyz" \
               "$CORP/Breaklines/${stem}_Breakline_0.pcd" "$CORP/Surfaces/${stem}_Surface_0.xyz" \
               "$CORP/Surfaces/${stem}_Surface_1.xyz"; do
        [ -f "$f" ] && printf "%s\t%s\n" "$(md5sum "$f" | cut -d" " -f1)" "$f"
      done
      [ -f "$GT/Pot_${POT}_Piece_${p}_T.txt" ] && printf "%s\t%s\n" "$(md5sum "$GT/Pot_${POT}_Piece_${p}_T.txt" | cut -d" " -f1)" "$GT/Pot_${POT}_Piece_${p}_T.txt"
    done
  } | sort -k2 > "$out"; }

md5_inputs "$RUN/logs/corpus_inputs_before.md5"

# K<=3 axes: first three lines of each axes329f file
for f in "$AX_318"/Pot_${POT}_Piece_*_Axis.xyz; do
  head -n 3 "$f" > "$WORK/axes_k3/$(basename "$f")"
done

# Patch guard & per-pot include root
$PY "$WORK/patch_guard67.py" "$SHIPPED" "$WORK/shim/G.cpp" > logs/guard.log 2>&1 || fail 4 "guard patch failed"
$APT exec "$SIF" cat /opt/sfs-pristine/class/data_path.h > "$WORK/inc_src/class/data_path.h" 2>>logs/provenance.log
[ -s "$WORK/inc_src/class/data_path.h" ] || fail 6 "could not extract data_path.h"
mkdir -p "$WORK/inc_$POT/class"
$PY "$WORK/mk_inc.py" "$WORK/inc_src/class/data_path.h" "$WORK/inc_$POT/class/data_path.h" "$POT" > logs/mk_inc.log 2>&1 || fail 6 "mk_inc failed"
grep -c "^#define POT_" "$WORK/inc_$POT/class/data_path.h" > logs/inc_active.txt 2>&1 || true
# provenance: prove the image really carries the 329 patch
$APT exec "$SIF" md5sum /opt/sfs-pristine/class/feature_matching.cpp > logs/fm_md5_image.txt 2>&1
grep -q cb7b115045a6277aefd47305353589bc logs/fm_md5_image.txt || fail 5 "image is not the 329 patch: $(cat logs/fm_md5_image.txt)"

stage_arm() {
  local A=$1 MODE=$2 AXD=$3
  for r in $(seq 1 "$NREP"); do
    local TDIR="$TREES/${A}R${r}"
    mkdir -p "$TDIR"
    $PY "$WORK/stage305.py" --pot "$POT" --corpus "$CORP" --lanes "$LANES" --axes "$AXD" \
        --out "$TDIR" --arms "$MODE" --named "$NAMED" --np "$NP" > "logs/stage_${A}R${r}.log" 2>&1 \
        || fail 7 "stage ${A}R${r} failed"
  done
}

stage_arm "${POT}_MO" "MO" "$AX_MO"
stage_arm "${POT}_K3" "MM" "$WORK/axes_k3"

tree_path() {
  local L=$1
  if [[ "$L" == *_MO* ]]; then echo "$TREES/$L/$POT/MO"; else echo "$TREES/$L/$POT/MM"; fi
}

# Warm up /panel/build (patched objects, built once per job from the patched /opt/sfs-pristine)
$APT exec --writable-tmpfs \
   --bind "$MAIN":/workspace --bind "$WORK":/panel \
   "$SIF" bash -c '
     mkdir -p /panel/build /panel/logs
     cd /panel/build
     cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release > /panel/logs/cmake.log 2>&1
     make -j8 > /panel/logs/make.log 2>&1
   ' || fail 9 "cmake/make warm-up failed"

export OMP_NUM_THREADS=8

run_and_score() {
  local LABEL=$1
  local TREE
  TREE=$(tree_path "$LABEL")
  local T0=$SECONDS
  echo "##### LAUNCH $LABEL tree=$TREE $(date -u +%FT%TZ)"
  $APT exec --writable-tmpfs \
     --bind "$MAIN":/workspace --bind "$WORK":/panel --bind "$OUT":/out --bind "$TREE":/Dataset/SfS_pp \
     "$SIF" bash /panel/arm67.sh "$LABEL" "$FLOOR" "/panel/inc_$POT" "$NP" NONE > "logs/run_${LABEL}.log" 2>&1
  local RC=$? DT=$((SECONDS - T0))
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "$LABEL" "$FLOOR" "/panel/inc_$POT" "$TREE" "$NP" "$RC" "$DT" > "logs/panel_${LABEL}.row"
  echo "##### DONE $LABEL exit=$RC wall=${DT}s $(date -u +%FT%TZ)"
  # SCALE329 is printed by the binary; arm67.sh redirects that stdout to /panel/logs/binary_<LABEL>.log
  { grep -h "SCALE329" "$WORK/logs/binary_${LABEL}.log" 2>/dev/null || grep -h "SCALE329" "logs/run_${LABEL}.log" 2>/dev/null; } > "logs/scale329_${LABEL}.txt" || true
  cat "logs/scale329_${LABEL}.txt" 2>/dev/null || true
  if [ -d "$OUT/$LABEL" ]; then
    local PTS="$TREES/$LABEL/arm_pieces_$POT.tsv"
    $PY "$WORK/acc305.py" "$OUT/$LABEL" "$LABEL" "$SC/acc_${LABEL}.row" >> "logs/acc_${LABEL}.log" 2>&1
    [ -f "$OUT/$LABEL/1. Acc.txt" ] && cp -f "$OUT/$LABEL/1. Acc.txt" "$SC/$LABEL.Acc.txt"
    $PY "$WORK/score305.py" "$OUT/$LABEL" "$GT" "$POT" "$PTS" "$SC/$LABEL.tsv" "$LABEL" >> "logs/score_${LABEL}.log" 2>&1
    $PY "$WORK/score311_cpp.py" "$OUT/$LABEL" "$GT" "$POT" "$SC/${LABEL}_cpp.json" "$LABEL" >> "logs/score_${LABEL}.log" 2>&1
  fi
}

pids=()
for arm in "${POT}_MOR1" "${POT}_MOR2" "${POT}_K3R1" "${POT}_K3R2"; do
  run_and_score "$arm" &
  pids+=($!)
  sleep 2
done
for pid in "${pids[@]}"; do
  wait "$pid"
done

printf "arm\tfloor\tinc\ttree\tnp\texit\twall_s\n" > logs/panel.tsv
printf "label\ttime_s\tsherd\tedge\traw\n" > "$SC/acc.tsv"
for arm in "${POT}_MOR1" "${POT}_MOR2" "${POT}_K3R1" "${POT}_K3R2"; do
  [ -f "logs/panel_${arm}.row" ] && cat "logs/panel_${arm}.row" >> logs/panel.tsv
  [ -f "$SC/acc_${arm}.row" ] && cat "$SC/acc_${arm}.row" >> "$SC/acc.tsv"
done

# ---- the bar-6 SCALE329 requirement: every lane of every pot must print s=1.000 M=6
printf "lane\tn_scale329\ts_one\tM_six\tverdict\n" > logs/scale329_assert.tsv
scale_bad=0
for arm in "${POT}_MOR1" "${POT}_MOR2" "${POT}_K3R1" "${POT}_K3R2"; do
  f="logs/scale329_${arm}.txt"
  n=0; s1=NO; m6=NO; v=MISSING
  if [ -s "$f" ]; then
    n=$(grep -c SCALE329 "$f")
    grep -q "s=1.000" "$f" && s1=YES
    grep -q "M=6" "$f" && m6=YES
    if [ "$n" -ge 1 ] && [ "$s1" = YES ] && [ "$m6" = YES ]; then v=OK; else v=FAIL; scale_bad=1; fi
  else
    scale_bad=1
  fi
  printf "%s\t%s\t%s\t%s\t%s\n" "$arm" "$n" "$s1" "$m6" "$v" >> logs/scale329_assert.tsv
done
cat logs/scale329_assert.tsv

md5_inputs "$RUN/logs/corpus_inputs_after.md5"
if diff -q "$RUN/logs/corpus_inputs_before.md5" "$RUN/logs/corpus_inputs_after.md5" > /dev/null 2>&1; then
  echo "CORPUS-UNCHANGED"
else
  echo "CORPUS-CHANGED"; diff "$RUN/logs/corpus_inputs_before.md5" "$RUN/logs/corpus_inputs_after.md5" > logs/corpus_diff.txt 2>&1
fi

cat logs/panel.tsv
cat "$SC/acc.tsv"
if [ "$scale_bad" -ne 0 ]; then echo "[JOB] SCALE329-ASSERT-FAILED for pot $POT (clamp did not hold)"; fi
echo "[JOB] complete at $(date -u +%FT%TZ)"
exit "$scale_bad"
