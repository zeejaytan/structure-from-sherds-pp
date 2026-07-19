#!/bin/bash
#SBATCH --job-name=sfs_original_baseline
#SBATCH --time=01:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_original_baseline_%j.out
#SBATCH --error=sfs_original_baseline_%j.err

echo "=== SFS ORIGINAL NURBS BASELINE TEST ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing original NURBS sample data with original thresholds: axis_angle < 0.436 radians, score < 1.5"
echo

cd /data/gpfs/projects/punim2657/sfs_main

# Backup current TPS dataset
echo "Backing up current TPS dataset..."
mv sfspreproc-docker/Dataset/SfS_pp sfspreproc-docker/Dataset/SfS_pp_TPS_backup

# Copy original NURBS sample data
echo "Setting up original NURBS sample dataset..."
cp -r original_samples/SfS_pp sfspreproc-docker/Dataset/

# Revert to original strict thresholds in container and build
echo "Building SFS with original strict thresholds (0.436, 1.5)..."
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    sfspreproc.sif \
    /bin/bash -c "
    cd /workspace/sfspreproc-docker/class
    # Revert to original thresholds
    sed -i 's/axis_angle_ > 1.8/axis_angle_ > 0.436/g' feature_matching.cpp
    sed -i 's/> 1.8/> 0.436/g' feature_matching.cpp  
    sed -i 's/lowest_score > 5.0/lowest_score > 1.5/g' feature_matching.cpp
    sed -i 's/> 5.0/> 1.5/g' feature_matching.cpp
    cd /workspace/sfspreproc-docker
    make clean && make -j4
    "

echo "Running SFS with original NURBS data and original thresholds..."
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset:/Dataset" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo "Restoring TPS dataset..."
rm -rf sfspreproc-docker/Dataset/SfS_pp
mv sfspreproc-docker/Dataset/SfS_pp_TPS_backup sfspreproc-docker/Dataset/SfS_pp

echo
echo "Original NURBS baseline test completed at: $(date)"
echo "=== Job Finished ==="