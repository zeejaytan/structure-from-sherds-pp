#!/bin/bash
#SBATCH --job-name=hierarchical_rimbase
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=hierarchical_rimbase_%j.out
#SBATCH --error=hierarchical_rimbase_%j.err

echo "=========================================="
echo "HIERARCHICAL RIM/BASE ASSEMBLY TEST"
echo "Testing geometric rim/base detection with height validation removed"
echo "=========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main

echo "*** FEATURE FLAGS ***"
echo "  ENABLE_HIERARCHICAL_ASSEMBLY=1  (Rim/Base/Body phased assembly)"
echo "  ENABLE_POTTERY_VALIDATION=1     (Pottery-aware geometric validation)"
echo ""
echo "*** EXPECTED RESULTS ***"
echo "  - Geometric rim detection finds pieces 3,5,6,7,8 (360° coverage)"
echo "  - Geometric base detection finds pieces with planar bottom edges"
echo "  - NO height validation penalties (removed chicken-and-egg paradox)"
echo "  - All 8/8 pieces assembled (previously 6/8 due to height blocking)"
echo ""

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "export ENABLE_HIERARCHICAL_ASSEMBLY=1 && export ENABLE_POTTERY_VALIDATION=1 && echo 'Container ENV: HIERARCHICAL_RIMBASE_TEST' && /workspace/sfspreproc-docker/Hierarchy-Clear 8"

echo ""
echo "=========================================="
echo "TEST COMPLETE - Check output above for:"
echo "  1. GEOMETRY-BASED RIM/BASE IDENTIFICATION section"
echo "  2. RIM/BASE pieces found (geometric)"
echo "  3. FINAL CLASSIFICATION (RIM/BASE/BODY counts)"
echo "  4. Assembly completeness (should be 8/8 pieces)"
echo "=========================================="
echo "End time: $(date)"
