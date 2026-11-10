#!/bin/bash
# setup330i.sh -- prepare the assembly experiment dir on the login node (no solver work here).
set -euo pipefail
B=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips
R=${B}-i
T326=/data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE
SHIPPED=/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp
mkdir -p "$R/logs" "$R/out" "$R/bin" "$R/shim" "$R/tools"
[ -d "$R/build" ] || cp -a "${B}-c/build" "$R/build"
[ -d "$R/inc_E" ] || cp -a "${B}-c/inc_E" "$R/inc_E"
[ -d "$R/tree_60" ] || cp -a "$T326/trees/E_MOR1/E/MO" "$R/tree_60"
[ -d "$R/tree_9277" ] || cp -a "$T326/trees/E_MOR1/E/MO" "$R/tree_9277"
awk -f "$R/tools/mk_base.awk" "$SHIPPED" > "$R/shim/base.cpp"
echo "--- inserted lines:"; grep -n "T330_S\|extern double g_len_scale" "$R/shim/base.cpp"
echo "--- shipped vs base (expect exactly the 3 insertions):"
diff "$SHIPPED" "$R/shim/base.cpp" || true
chmod +x "$R/t330_run.sh" 2>/dev/null || true
echo "--- md5s:"; md5sum "$SHIPPED" "$R/shim/base.cpp" "$R/t330_run.sh" "$R/tools/submit330i.sbatch"
du -sh "$R/tree_60" "$R/tree_9277" "$R/build"
