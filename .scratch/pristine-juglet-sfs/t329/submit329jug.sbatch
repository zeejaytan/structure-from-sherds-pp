#!/bin/bash
#SBATCH --job-name=t329jug
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=64G
#SBATCH --time=02:00:00
#SBATCH --output=logs/slurm-%j.out
#SBATCH --error=logs/slurm-%j.err
#
# Ticket 329 bar 5: Juglet E2E, four lanes, two repeats of the cured tree per assembler.
#   T329P_r1 / T329P_r2  = pristine image  (sfs_pristine_assembly.sif)   + pristine shim
#   T329X_r1 / T329X_r2  = ticket 329 image (sfs_pristine_assembly_329.sif) + the 329 batch shim
# Template = runs/20261110-2352_123f2ca_t328-e2e-dup8/submit328f.sbatch, same cured inputs
# (the t328-trim5 cured tree, GT-axes arm), same seam gate (FLOOR=20, NP=9, -DJUGLET_ORIG).
# Each assembler gets its OWN /panel: the 329 image compiles its own objects from the patched
# sources, so a shared build dir would silently reuse pristine objects.
set -uo pipefail

RUN=$SLURM_SUBMIT_DIR
CURED=${CURED:-/data/gpfs/projects/punim2657/SfSpp/runs/20261110-2325_123f2ca_t328-trim5/trees/juglet_after}
SHIPPED_LANES=/data/gpfs/projects/punim2657/SfSpp/runs/20261103-1332_fab76f5_t66-admitsign-default/outU_jug/geom
DS=/data/gpfs/projects/punim2657/sfs_preprocessing/Dataset
GT=$DS/Juglet_GT
CORP=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF_P=$MAIN/sfs_pristine_assembly.sif
SIF_X=$MAIN/sfs_pristine_assembly_329.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
SHIPPED_P=$MAIN/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp
SHIPPED_X=$RUN/main_pristine_batch_329.cpp
BLOCK=/data/gpfs/projects/punim2657/SfSpp/runs/20261101-1913_d8f716f_t58-floor-density-panel/work/juglet_orig_block.inc

WORK=$RUN/work
TREES=$RUN/trees
OUT=$RUN/out
SC=$RUN/score
FLOOR=20
NP=9
JOBID=${SLURM_JOB_ID:-na}

fail() { echo "RUN-FAILED: $*" | tee "$RUN/RUN-FAILED.txt"; exit "${2:-1}"; }

for v in $(compgen -e | grep -E "^(SFS_|POT_NAME$)"); do unset "$v"; done
cd "$RUN" || exit 1
mkdir -p "$WORK/inc_src/class" "$OUT" "$TREES" "$SC" logs

echo "[JOB] runid=$(basename "$RUN") job=$JOBID start=$(date -u +%FT%TZ) cured=$CURED"
test -s "$SHIPPED_X" || fail 3 "missing 329 shim $SHIPPED_X"
test -s "$SIF_X" || fail 3 "missing 329 image $SIF_X"

# corpus listing BEFORE (read-only proof)
{ echo "== DS $DS"; find "$DS" -maxdepth 3 -type f | LC_ALL=C sort | head -500 | xargs -r md5sum 2>/dev/null
  echo "== CORP $CORP"; find "$CORP" -maxdepth 3 -type f | LC_ALL=C sort | head -500 | xargs -r md5sum 2>/dev/null
} > "$RUN/list_corpus_before.txt" 2>&1

# ---- per-assembler panel: arm67.sh + the guarded base for THAT assembler
prep_panel() {
  local TAG=$1 SHP=$2
  rm -rf "$WORK/panel_$TAG"
  mkdir -p "$WORK/panel_$TAG/shim" "$WORK/panel_$TAG/logs" "$WORK/panel_$TAG/inc_jug/class" "$WORK/panel_$TAG/tools"
  cp -f "$WORK/arm67.sh" "$WORK/panel_$TAG/arm67.sh" || fail 4 "arm67.sh copy failed"
  cp -a "$WORK/inc_jug/." "$WORK/panel_$TAG/inc_jug/" || fail 4 "inc_jug copy failed"
  $PY "$WORK/patch_guard67.py" "$SHP" "$WORK/panel_$TAG/shim/G.cpp" > "logs/guard_$TAG.log" 2>&1 || fail 4 "patch_guard67 failed for $TAG"
  echo "[PANEL] $TAG shim md5=$(md5sum "$WORK/panel_$TAG/shim/G.cpp" | cut -d' ' -f1) shipped=$(md5sum "$SHP" | cut -d' ' -f1)"
}

# ---- juglet include block from the pristine image (data_path.h is identical in both images)
$APT exec "$SIF_P" cat /opt/sfs-pristine/class/data_path.h > "$WORK/inc_src/class/data_path.h" 2>>logs/provenance.log || fail 5 "cannot read pristine data_path.h"
mkdir -p "$WORK/inc_jug/class"
$PY "$WORK/mk_inc.py" "$WORK/inc_src/class/data_path.h" "$WORK/inc_jug/class/data_path.h" NONE --append "$BLOCK" > logs/mk_inc.log 2>&1 || fail 6 "mk_inc failed"

prep_panel p   "$SHIPPED_P"
prep_panel x   "$SHIPPED_X"

# ---- stage the cured lane, GT axes, two repeats
$PY "$WORK/stage67_inputs.py" --lanes "$CURED"        --dataset "$DS" --pota "$CORP" --out "$WORK/stage_cured"   > logs/stage_cured.log   2>&1 || fail 7 "stage cured failed"
$PY "$WORK/stage67_inputs.py" --lanes "$SHIPPED_LANES" --dataset "$DS" --pota "$CORP" --out "$WORK/stage_shipped" > logs/stage_shipped.log 2>&1 || fail 7 "stage shipped failed"
for t in cured shipped; do
  for i in $(seq 1 9); do
    cp -f "$GT/Juglet_Piece_${i}_Axis.xyz" "$WORK/stage_$t/juglet/Axes/Juglet_Piece_${i}_Axis.xyz" || fail 8 "GT axis copy failed"
  done
done
rm -rf "${TREES:?}"/juglet_*
cp -a "$WORK/stage_cured/juglet" "$TREES/juglet_cured_r1"
cp -a "$WORK/stage_cured/juglet" "$TREES/juglet_cured_r2"
for t in "$TREES"/juglet_*; do
  ( cd "$t" && find . -type f -o -type l | LC_ALL=C sort | xargs -r md5sum ) > "$(dirname "$t")/$(basename "$t").md5"
done

# ---- warm each assembler's objects inside its own image
for TAG in p x; do
  SIF=$SIF_P; [ "$TAG" = "x" ] && SIF=$SIF_X
  echo "[BUILD] $TAG objects from $( [ "$TAG" = x ] && echo patched || echo pristine ) sources"
  $APT exec --writable-tmpfs \
     --bind /data/gpfs:/data/gpfs --bind "$MAIN":/workspace --bind "$WORK/panel_$TAG":/panel \
     "$SIF" bash -c '
       mkdir -p /panel/build /panel/logs
       cd /panel/build
       cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release > /panel/logs/cmake.log 2>&1
       make -j4 > /panel/logs/make.log 2>&1
     ' || fail 9 "$TAG object build failed"
  $APT exec "$SIF" md5sum /opt/sfs-pristine/class/feature_matching.cpp > "logs/fm_md5_$TAG.txt" 2>&1
  cat "logs/fm_md5_$TAG.txt"
done

export OMP_NUM_THREADS=4
runarm() {
  local LABEL=$1 TAG=$2 TREE=$3
  local SIF=$SIF_P; [ "$TAG" = "x" ] && SIF=$SIF_X
  local T0=$SECONDS
  echo "##### LAUNCH $LABEL tag=$TAG tree=$TREE $(date -u +%FT%TZ)"
  $APT exec --writable-tmpfs \
     --bind /data/gpfs:/data/gpfs --bind "$MAIN":/workspace --bind "$WORK/panel_$TAG":/panel --bind "$OUT":/out --bind "$TREE":/Dataset/SfS_pp \
     "$SIF" bash /panel/arm67.sh "$LABEL" "$FLOOR" /panel/inc_jug "$NP" -DJUGLET_ORIG > "logs/run_${LABEL}.log" 2>&1
  local RC=$? DT=$((SECONDS - T0))
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "$LABEL" "$FLOOR" /panel/inc_jug "$TREE" "$NP" "$RC" "$DT" > "logs/panel_${LABEL}.row"
  echo "##### DONE $LABEL exit=$RC wall=${DT}s $(date -u +%FT%TZ)"
  # SCALE329 is printed by the binary; arm67.sh redirects that stdout to /panel/logs/binary_<LABEL>.log
  { grep -h "SCALE329" "$WORK/panel_$TAG/logs/binary_${LABEL}.log" 2>/dev/null || grep -h "SCALE329" "logs/run_${LABEL}.log" 2>/dev/null; } > "logs/scale329_${LABEL}.txt" || true
  cat "logs/scale329_${LABEL}.txt" 2>/dev/null || true
  if [ -d "$OUT/$LABEL" ]; then
    $PY "$WORK/acc305.py" "$OUT/$LABEL" "$LABEL" "$SC/acc_${LABEL}.row" >> "logs/acc_${LABEL}.log" 2>&1
    [ -f "$OUT/$LABEL/1. Acc.txt" ] && cp -f "$OUT/$LABEL/1. Acc.txt" "$SC/$LABEL.Acc.txt"
    $PY "$WORK/score58.py" --result "$OUT/$LABEL" --gt "$GT" \
        --mesh "$TREE/Mesh" --lanes "$TREE/Breaklines" \
        --out "$SC/$LABEL.json" --label "$LABEL" > "logs/score_${LABEL}.log" 2>&1
  fi
}

# control first, then the patched lanes, so a patched crash still leaves the control on disk
runarm "T329P_r1" p "$TREES/juglet_cured_r1"
runarm "T329P_r2" p "$TREES/juglet_cured_r2"
runarm "T329X_r1" x "$TREES/juglet_cured_r1"
runarm "T329X_r2" x "$TREES/juglet_cured_r2"

printf "label\ttime_s\tsherd\tedge\traw\n" > "$SC/acc.tsv"
printf "arm\tfloor\tinc\ttree\tnp\texit\twall_s\n" > logs/panel.tsv
for arm in T329P_r1 T329P_r2 T329X_r1 T329X_r2; do
  [ -f "logs/panel_${arm}.row" ] && cat "logs/panel_${arm}.row" >> logs/panel.tsv
  [ -f "$SC/acc_${arm}.row" ] && cat "$SC/acc_${arm}.row" >> "$SC/acc.tsv"
done
cat logs/panel.tsv
cat "$SC/acc.tsv"
for f in "$SC"/T329*.json; do [ -f "$f" ] && echo "== $f" && cat "$f"; done

# corpus listing AFTER
{ echo "== DS $DS"; find "$DS" -maxdepth 3 -type f | LC_ALL=C sort | head -500 | xargs -r md5sum 2>/dev/null
  echo "== CORP $CORP"; find "$CORP" -maxdepth 3 -type f | LC_ALL=C sort | head -500 | xargs -r md5sum 2>/dev/null
} > "$RUN/list_corpus_after.txt" 2>&1
diff "$RUN/list_corpus_before.txt" "$RUN/list_corpus_after.txt" > "$RUN/list_corpus.diff" 2>&1 && echo "corpus listing identical" || echo "CORPUS LISTING CHANGED (see list_corpus.diff)"

echo "[JOB] complete at $(date -u +%FT%TZ)"
