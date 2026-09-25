#!/bin/bash
#SBATCH --job-name=juglet_rerun
#SBATCH --time=02:00:00
#SBATCH --mem=32G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire
#SBATCH --output=juglet_rerun_%j.out
#SBATCH --error=juglet_rerun_%j.err

# Rerun the identical Juglet assembly binary+inputs (ticket 04: reproducibility).
# NO rebuild, NO pull (frozen tree): any difference vs 30829588 is runtime
# nondeterminism, not code. Expects: best state 7 singletons, score 0.
cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker || exit 1

echo "Job ID: $SLURM_JOB_ID, start: $(date)"
echo "Tree: $(git log --oneline -1)"
echo "Binary: $(ls -la build/Hierarchy-Clear | awk '{print $6, $7, $8, $9}')"
grep -q "^#define JUGLET$" class/data_path.h || { echo "ERROR: JUGLET not active"; exit 1; }
[ -x build/Hierarchy-Clear ] || { echo "ERROR: binary missing"; exit 1; }
echo ""

APPTAINER="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
SIF="/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif"
ROOT="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker"

time ${APPTAINER} exec \
    --bind "/data:/data" \
    --env SFS_AXIS_ANGLE_MAX="${SFS_AXIS_ANGLE_MAX:-}" \
    --env SFS_PC_DIST="${SFS_PC_DIST:-}" \
    --env SFS_PC_NORM="${SFS_PC_NORM:-}" \
    --env SFS_AXIS_WEIGHT="${SFS_AXIS_WEIGHT:-}" \
    --env SFS_LCS_ONLY="${SFS_LCS_ONLY:-}" \
    --env SFS_REJECT_DIST="${SFS_REJECT_DIST:-}" \
    --env SFS_REJECT_NORM="${SFS_REJECT_NORM:-}" \
    --env SFS_NORMAL_ABS="${SFS_NORMAL_ABS:-}" \
    --env SFS_CAUCHY_DIST="${SFS_CAUCHY_DIST:-}" \
    --env SFS_CAUCHY_NORM="${SFS_CAUCHY_NORM:-}" \
    --env OMP_NUM_THREADS="${OMP_NUM_THREADS:-}" \
    "${SIF}" \
    bash -c "cd ${ROOT}/build && ./Hierarchy-Clear"
echo "Execution exit code: $?"
echo "Finished at: $(date)"
echo ""
R=$(ls -td ${ROOT}/build/results_* | head -n 1)
echo "Results dir: ${R}"
ls "${R}/Result" 2>/dev/null | head -n 20
ACC=$(ls "${R}/Result"/1.\ Acc.txt 2>/dev/null | head -n 1)
[ -n "${ACC}" ] && { echo "--- Accuracy ---"; cat "${ACC}"; }
echo ""
echo "--- Assembly states ---"
grep -aE "Best assembly|pieces\)|singletons|graphs - " juglet_rerun_${SLURM_JOB_ID}.out 2>/dev/null | tail -n 5 || true
echo "End: $(date)"
