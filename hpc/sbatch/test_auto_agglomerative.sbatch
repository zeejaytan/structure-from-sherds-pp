#!/bin/bash
#SBATCH --job-name=test_auto_agglomerative
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=test_auto_agglomerative_%j.out
#SBATCH --error=test_auto_agglomerative_%j.err

echo "==============================================================================="
echo "🚀 AUTO-AGGLOMERATIVE ASSEMBLY SYSTEM TEST"
echo "==============================================================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing: PuzzleFusion++ inspired 6-DoF diffusion denoising"
echo "Expected: Single unified assembly with global optimization"
echo "==============================================================================="
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Verifying Auto-Agglomerative Assembly binary..."
echo "Binary: $(ls -la /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear)"
echo "Build time: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear 2>/dev/null)"
echo

echo "Verifying dataset structure..."
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"
echo

echo "==============================================================================="
echo "🎯 RUNNING AUTO-AGGLOMERATIVE ASSEMBLY SYSTEM"
echo "==============================================================================="
echo "Environment: ENABLE_AUTO_AGGLOMERATIVE=1"
echo "Algorithm: PuzzleFusion++ inspired three-phase optimization"
echo "  Phase 1: DENOISER - 6-DoF parameter diffusion denoising"
echo "  Phase 2: VERIFIER - Multi-criteria geometric compatibility"
echo "  Phase 3: AGGLOMERATIVE - Progressive cluster merging"
echo "Expected Result: Single unified graph with 8/8 pieces"
echo "==============================================================================="
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "export ENABLE_AUTO_AGGLOMERATIVE=1 && echo '🎯 Auto-Agglomerative Assembly ENABLED' && /workspace/sfspreproc-docker/Hierarchy-Clear 8"

echo
echo "==============================================================================="
echo "📊 AUTO-AGGLOMERATIVE ASSEMBLY TEST RESULTS"
echo "==============================================================================="
echo "Results saved to timestamped directory:"
ls -la /data/gpfs/projects/punim2657/sfs_main/results_* | tail -1

echo
echo "Test completed at: $(date)"
echo "Job duration: $SECONDS seconds"
echo "==============================================================================="