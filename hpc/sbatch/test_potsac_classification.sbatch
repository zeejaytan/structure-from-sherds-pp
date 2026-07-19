#!/bin/bash
#SBATCH --job-name=potsac_classification
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=potsac_classification_%j.out
#SBATCH --error=potsac_classification_%j.err

echo "=========================================="
echo "PotSAC-BASED RIM/BASE/BODY CLASSIFICATION TEST"
echo "Testing adaptive height-based classification with hierarchical assembly"
echo "=========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main

echo "*** FEATURE FLAGS ***"
echo "  ENABLE_AUTO_AGGLOMERATIVE=1     (Required to trigger global optimization)"
echo "  ENABLE_HIERARCHICAL_ASSEMBLY=1  (Hierarchical rim→base→body assembly)"
echo "  ENABLE_POTTERY_VALIDATION=1     (Pottery-aware geometric validation)"
echo ""
echo "*** KEY INNOVATION ***"
echo "  PotSAC axis heights used for rim/base/body classification"
echo "  Solves chicken-and-egg paradox: heights computed BEFORE assembly"
echo ""
echo "*** EXPECTED PotSAC CLASSIFICATION (Pot A) ***"
echo "  RIM pieces (top 33%): [5, 6, 3, 7] (4 pieces)"
echo "    Piece 5: 278.19mm, Piece 6: 317.73mm"
echo "    Piece 3: 345.93mm, Piece 7: 356.65mm"
echo ""
echo "  BASE pieces (bottom 33%): [1, 4, 2] (3 pieces)"
echo "    Piece 1: 59.65mm, Piece 4: 87.51mm"
echo "    Piece 2: 88.04mm"
echo ""
echo "  BODY pieces (middle 33%): [8] (1 piece)"
echo "    Piece 8: 195.43mm"
echo ""
echo "*** EXPECTED RESULTS ***"
echo "  - '=== PotSAC-BASED PIECE CLASSIFICATION ===' section"
echo "  - Correct rim/base/body classification matching above"
echo "  - '=== HYBRID PotSAC + GEOMETRIC CLASSIFICATION ===' section"
echo "  - Improved assembly completeness (target: 8/8 pieces)"
echo "  - NO height validation paradox errors"
echo ""

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "export ENABLE_AUTO_AGGLOMERATIVE=1 && export ENABLE_HIERARCHICAL_ASSEMBLY=1 && export ENABLE_POTTERY_VALIDATION=1 && echo 'Container ENV: POTSAC_CLASSIFICATION_TEST' && /workspace/sfspreproc-docker/Hierarchy-Clear 8"

echo ""
echo "=========================================="
echo "TEST COMPLETE - Key sections to check:"
echo "  1. '=== PotSAC-BASED PIECE CLASSIFICATION ==='"
echo "  2. PotSAC height distribution and thresholds"
echo "  3. '=== CLASSIFICATION SUMMARY ===' (should match expected above)"
echo "  4. '=== HYBRID PotSAC + GEOMETRIC CLASSIFICATION ==='"
echo "  5. Final piece classification with PotSAC ✓/✗ and Geometry ✓/✗"
echo "  6. Assembly completeness (target: 8/8 pieces)"
echo "  7. No 'height validation' or 'chicken-and-egg' errors"
echo "=========================================="
echo "End time: $(date)"
