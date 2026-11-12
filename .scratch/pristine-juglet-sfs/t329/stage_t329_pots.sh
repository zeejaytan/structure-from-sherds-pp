#!/bin/bash
set -euo pipefail
RUNS=/data/gpfs/projects/punim2657/SfSpp/runs
AXES=$RUNS/20261111-0120_20b52bf_t329-axes/out/axes329f
TS=$(date +%Y%m%d-%H%M)
echo "STAMP=$TS"
for p in A B C D E F G H I J; do
  SRC=$RUNS/20261110-0740_d628fc0_t323-pot$p
  DST=$RUNS/${TS}_20b52bf_t329-pot$p
  if [ -d "$DST" ]; then echo "EXISTS $DST"; exit 1; fi
  mkdir -p "$DST"/logs "$DST"/out "$DST"/score "$DST"/trees "$DST"/work/axes_k3 "$DST"/work/shim "$DST"/work/inc_src "$DST"/work/logs
  cp -a "$SRC"/work/*.py "$SRC"/work/*.sh "$DST"/work/
  cp -a "$SRC"/work/shim/G.cpp "$DST"/work/shim/
  cp -a "$SRC"/work/build "$DST"/work/build
  cp -a "$SRC"/work/inc_src/. "$DST"/work/inc_src/
  sed -e "s|^#SBATCH --job-name=t323-pot$p|#SBATCH --job-name=t329-pot$p|" \
      -e "s|^AX_318=.*|AX_318=$AXES|" \
      -e "s|^# Ticket 323:.*|# Ticket 329 bar-1 step2: ten-pot E2E, axes = regenerated axes329f (byte-gate vs axes323). From t323-pot$p submit.sbatch; only job-name and AX_318 differ.|" \
      "$SRC/submit.sbatch" > "$DST/submit.sbatch"
  cat > "$DST/run.json" <<EOF
{"runid": "${TS}_20b52bf_t329-pot$p", "ticket": 329, "pot": "$p", "tag": "t329-pot$p", "commit": "20b52bf",
 "axes": "$AXES", "arms": ["${p}_MOR1", "${p}_MOR2", "${p}_K3R1", "${p}_K3R2"], "template": "$SRC",
 "bar": "1 step2", "sif": "sfs_pristine_assembly.sif 63a532b9732f84e59e7b1349065f1323"}
EOF
  bash -n "$DST/submit.sbatch"
  echo "staged $DST"
done
echo "ALL-STAGED $TS"
