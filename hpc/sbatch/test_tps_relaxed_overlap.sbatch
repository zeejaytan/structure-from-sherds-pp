#!/bin/bash
#SBATCH --job-name=sfs_tps_relaxed_overlap
#SBATCH --time=01:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_tps_relaxed_overlap_%j.out
#SBATCH --error=sfs_tps_relaxed_overlap_%j.err

echo "=== SFS TPS RELAXED OVERLAP DETECTION TEST ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing TPS surfaces with relaxed overlap detection: distance < 2, area < 150"
echo "With moderate thresholds: axis_angle < 1.2, score < 3.0"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Verifying TPS dataset is loaded..."
ls -la sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_01_Surface_0.xyz

echo "Building SFS with relaxed overlap detection for TPS..."
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    sfspreproc.sif \
    /bin/bash -c "cd /workspace/sfspreproc-docker && make clean && make -j4"

echo "Running SFS with TPS data and relaxed overlap detection..."
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset:/Dataset" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo
echo "TPS relaxed overlap test completed at: $(date)"
echo "=== Job Finished ==="