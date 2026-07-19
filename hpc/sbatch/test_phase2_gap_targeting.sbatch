#!/bin/bash
#SBATCH --job-name=phase2_gaps
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --time=0:30:00
#SBATCH --mem=8G
#SBATCH --output=phase2_gaps_%j.out

# PHASE 2 TEST: Gap-Targeting Scheduler
# - Live gap tracking and prioritization
# - Gap reduction validation (≥20% requirement)
# - Interleaved best-overall strategy (80% gap / 20% quality)
# - Comprehensive gap reduction logging

echo "=========================================="
echo "PHASE 2: GAP-TARGETING SCHEDULER TEST"
echo "=========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

# Phase 1 + Phase 2 feature flags
export ENABLE_STRICT_CLOSURE=1
export ENABLE_GAP_TARGETING=1
export ENABLE_HIERARCHICAL_ASSEMBLY=1
export ENABLE_AUTO_AGGLOMERATIVE=1
export ENABLE_MULTI_HYPOTHESIS=1

echo "*** FEATURE FLAGS ENABLED ***"
echo "  ENABLE_STRICT_CLOSURE=$ENABLE_STRICT_CLOSURE"
echo "  ENABLE_GAP_TARGETING=$ENABLE_GAP_TARGETING"
echo "  ENABLE_HIERARCHICAL_ASSEMBLY=$ENABLE_HIERARCHICAL_ASSEMBLY"
echo "  ENABLE_AUTO_AGGLOMERATIVE=$ENABLE_AUTO_AGGLOMERATIVE"
echo "  ENABLE_MULTI_HYPOTHESIS=$ENABLE_MULTI_HYPOTHESIS"
echo ""

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
