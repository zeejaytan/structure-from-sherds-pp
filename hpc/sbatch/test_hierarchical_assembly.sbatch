#!/bin/bash
#SBATCH --job-name=hierarchical_test
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=hierarchical_test_%j.out
#SBATCH --error=hierarchical_test_%j.err

echo "=========================================="
echo "HIERARCHICAL ARCHAEOLOGICAL ASSEMBLY TEST"
echo "=========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main

# Enable hierarchical assembly mode
export ENABLE_HIERARCHICAL_ASSEMBLY=1
export ENABLE_AUTO_AGGLOMERATIVE=1
export ENABLE_MULTI_HYPOTHESIS=1

echo "*** FEATURE FLAGS ENABLED ***"
echo "  ENABLE_HIERARCHICAL_ASSEMBLY=1"
echo "  ENABLE_AUTO_AGGLOMERATIVE=1"
echo "  ENABLE_MULTI_HYPOTHESIS=1"
echo ""

# Run the assembly
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear \
    --input_dir /Dataset/SfS_pp \
    --dataset_name Pot_A

echo ""
echo "=========================================="
echo "TEST COMPLETE"
echo "End time: $(date)"
echo "=========================================="
