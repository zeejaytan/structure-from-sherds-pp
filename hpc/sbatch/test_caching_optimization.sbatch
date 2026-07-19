#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=02:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --job-name=test_caching

# TEST CACHING OPTIMIZATION: Should reduce redundant intersection analyses by 80%

echo "=== TESTING INTERSECTION CACHING OPTIMIZATION ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Node: $(hostname)"

cd /data/gpfs/projects/punim2657/sfs_main

echo ""
echo "Testing optimized binary with intersection caching..."

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo ""
echo "=== CACHING TEST COMPLETED ==="
echo "Completed at: $(date)"
echo "Runtime: $SECONDS seconds"