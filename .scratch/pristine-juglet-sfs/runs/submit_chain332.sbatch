#!/bin/bash
#SBATCH --job-name=t332-chain
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=64G
#SBATCH --time=04:00:00
#SBATCH --output=logs/slurm-chain-%j.out
#SBATCH --error=logs/slurm-chain-%j.err
#
# Ticket 332, Part A: the standard preprocessing chain on the nine Juglet pieces, cured tree
# (preprocessing main b1e1580, code identical to the cured merge 676def2), with the doubled
# edgeline density switched by ENVIRONMENT ONLY (SFS_SPHERE_RADIUS_MM=0.9 SFS_RESAMPLE_D_MM=0.95).
# Own source copy and own build under this run dir; the canonical preprocessing checkout
# belongs to ticket 331 and is read only here.
#
# Runner: the repo's dual-arm cell script (ticket 69). ONE mesh stage per piece feeds TWO edge
# stages from one snapshot, so the 1.8 mm and 0.9 mm breaklines are a PAIRED measurement on the
# same surfaces -- the mesh stage draws 30 unseeded ray-cast points, so two separate chain runs
# would not give the same surfaces.
#
# Two cells, both carrying the same dense/sparse pair:
#   gt  SFS_AXIS_FILE = Dataset/Juglet_GT/<Piece>_Axis.xyz   (the axis the cured-tree baseline,
#       t328 run 20261110-2325_123f2ca_t328-trim5 cell gt_jug, was assembled on)
#   ds  SFS_AXIS_FILE = Dataset/Axes/Juglet/<Piece>_Axis.xyz  (our shipped September PotSAC output)
set -uo pipefail

RUN=${SLURM_SUBMIT_DIR}
CANON=/data/gpfs/projects/punim2657/sfs_preprocessing
DS=${CANON}/Dataset
GT=${DS}/Juglet_GT
CORP=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp
SIF=${CANON}/pcl_191_nurbs.sif
APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
PP=${RUN}/src/pp
INSTR=${PP}/.scratch/juglet-breaklines/stage69/run_cell69.sh
STAGER=${PP}/.scratch/juglet-breaklines/stage55/instruments/stage_synth_p40.sh
JUG="Juglet_Piece_1 Juglet_Piece_2 Juglet_Piece_3 Juglet_Piece_4 Juglet_Piece_5 Juglet_Piece_6 Juglet_Piece_7 Juglet_Piece_8 Juglet_Piece_9"
P=${SLURM_CPUS_PER_TASK:-16}
JOBID=${SLURM_JOB_ID:-na}
for v in $(compgen -e | grep -E '^(SFS_.*|POT_NAME)$'); do unset "$v"; done
cd "$RUN" || exit 1
mkdir -p logs out work src

echo "[JOB] t332 chain runid=$(basename "$RUN") job=$JOBID host=$(hostname) start=$(date -u +%FT%TZ)"

[ -f "$INSTR" ] || { echo "[JOB] FATAL missing $INSTR"; exit 3; }
[ -f "$STAGER" ] || { echo "[JOB] FATAL missing $STAGER"; exit 3; }
grep -q "SFS_SPHERE_RADIUS_MM" "$PP/edgeline_extraction_headless.cpp" || { echo "[JOB] FATAL density switch absent from source"; exit 4; }
grep -n "RESAMPLE_D_MM" "$PP/edgeline_extraction_headless.cpp" | head -5

echo "[JOB] building in $PP/build inside $SIF"
mkdir -p "$PP/build"
( cd "$PP/build" \
  && "$APT" exec --bind /data:/data "$SIF" cmake -DCMAKE_BUILD_TYPE=Release .. \
  && "$APT" exec --bind /data:/data "$SIF" make -j"$P" MeshPreprocessingHeadless EdgeLineExtractionHeadless
) > logs/build.log 2>&1
BRC=$?
if [ "$BRC" -ne 0 ] || [ ! -x "$PP/build/MeshPreprocessingHeadless" ] || [ ! -x "$PP/build/EdgeLineExtractionHeadless" ]; then
  echo "[JOB] FATAL build rc=$BRC"; grep -n -i "error" logs/build.log | head -20; exit 5
fi
cp -f "$PP/build/MeshPreprocessingHeadless" "$RUN/bin-MeshPreprocessingHeadless"
cp -f "$PP/build/EdgeLineExtractionHeadless" "$RUN/bin-EdgeLineExtractionHeadless"
{ echo "source	$PP (preprocessing main b1e1580, cured tree)"
  echo "MeshPreprocessingHeadless	$(md5sum "$RUN/bin-MeshPreprocessingHeadless" | cut -d' ' -f1)"
  echo "EdgeLineExtractionHeadless	$(md5sum "$RUN/bin-EdgeLineExtractionHeadless" | cut -d' ' -f1)"
} > "$RUN/binaries.md5"
cat "$RUN/binaries.md5"

run_cell() {
  local name=$1 adir=$2
  echo "[JOB] cell $name axes=$adir start=$(date -u +%FT%TZ)"
  env POT_NAME=Juglet PIECES="$JUG" WORNESS=0.7 \
      MESH_ENV="SFS_NORMAL_MODE=radius SFS_NORMAL_RADIUS_MM=1.0" \
      OUT="$RUN/out/$name" CLOUD_DIR="$DS/Point/Juglet" MESH_DIR="$DS/Mesh/Juglet" AXIS_DIR="$adir" \
      MANIFEST="$DS/Point/Juglet/MANIFEST_JUGLET_CC.tsv" MAN_MD5_COL=pcd_md5 MAN_PTS_COL=kept_points \
      MESH_BIN="$RUN/bin-MeshPreprocessingHeadless" EDGE_BIN="$RUN/bin-EdgeLineExtractionHeadless" \
      APT="$APT" SIF="$SIF" STAGER="$STAGER" PY="$PY" DS="$DS" CORP="$CORP" RUN="$RUN" JOBID="$JOBID" \
      bash "$INSTR" > "logs/cell_${name}.log" 2>&1
  local rc=$?
  echo "[JOB] cell $name rc=$rc end=$(date -u +%FT%TZ)"
  tail -8 "logs/cell_${name}.log" || true
  [ -f "$RUN/out/$name/pieces69.tsv" ] && cat "$RUN/out/$name/pieces69.tsv"
  return "$rc"
}

RC=0
run_cell gt "$GT" || RC=1
run_cell ds "$DS/Axes/Juglet" || RC=1

for CELL in gt ds; do
  for ARM in 18 09; do
    grep -ah "SPHERE-69\|RESAMPLE-69" "$RUN/out/$CELL"/pieces/*/edge${ARM}.log 2>/dev/null | tr -d "\r" | sort -u > "$RUN/out/${CELL}_banners_${ARM}.txt" || true
    echo "[JOB] $CELL arm $ARM banners:"; cat "$RUN/out/${CELL}_banners_${ARM}.txt" 2>/dev/null || true
  done
done

echo "[JOB] done rc=$RC at $(date -u +%FT%TZ)"
exit "$RC"
