#!/bin/bash
#SBATCH --job-name=t326-potE
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
# Ticket 323: pot end-to-end, MO control + axes323 (cost-ratio rule), two repeats; derived from /data/gpfs/projects/punim2657/SfSpp/runs/20261109-1426_ecd5bb8_t319-potE.
# Arms: <POT>_MO (authors' released Axes, control) and <POT>_K3 (axes318 top-3),
# 2 repeats each, all four assembler runs in THIS one job. Measurement only.
set -uo pipefail

POT=E
NAMED=31
NP=31
FLOOR=50
NREP=2

LANES=/data/gpfs/projects/punim2657/SfSpp/runs/20261105-0445_9c5eec2_t305-lane-e2/out/geom
AX_MO=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Axes
AX_318=/data/gpfs/projects/punim2657/SfSpp/runs/20261110-0740_d628fc0_t323-axes/out/axes323

RUN=$SLURM_SUBMIT_DIR
CORP=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly_326.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
SHIPPED=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp
GT="/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Ground Truth"

WORK=$RUN/work
TREES=$RUN/trees
OUT=$RUN/out
SC=$RUN/score
JOBID=${SLURM_JOB_ID:-na}

for v in $(compgen -e | grep -E '^(SFS_|POT_NAME$)'); do unset "$v"; done
cd "$RUN" || exit 1

echo "[JOB] runid=$(basename "$RUN") job=$JOBID pot=$POT named=$NAMED np=$NP floor=$FLOOR start=$(date -u +%FT%TZ)"

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

# K<=3 axes: first three lines of each axes318 file
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

# Warm up /panel/build (pristine objects, built once per job)
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

md5_inputs "$RUN/logs/corpus_inputs_after.md5"
if diff -q "$RUN/logs/corpus_inputs_before.md5" "$RUN/logs/corpus_inputs_after.md5" > /dev/null 2>&1; then
  echo "CORPUS-UNCHANGED"
else
  echo "CORPUS-CHANGED"; diff "$RUN/logs/corpus_inputs_before.md5" "$RUN/logs/corpus_inputs_after.md5" > logs/corpus_diff.txt 2>&1
fi

cat logs/panel.tsv
cat "$SC/acc.tsv"
echo "[JOB] complete at $(date -u +%FT%TZ)"
