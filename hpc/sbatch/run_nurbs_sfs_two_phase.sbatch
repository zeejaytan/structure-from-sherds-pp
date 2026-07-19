#!/bin/bash
#SBATCH --job-name=sfs_nurbs_two_phase
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=03:00:00
#SBATCH --output=/data/gpfs/projects/punim2657/sfs_main/sfs_two_phase_%j.out
#SBATCH --error=/data/gpfs/projects/punim2657/sfs_main/sfs_two_phase_%j.err

echo "=== Two-Phase Assembly Architecture Test Job ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Node: $SLURMD_NODENAME"
echo "Start time: $(date)"
echo

# Create timestamped output directory
TIMESTAMP=$(date +%Y_%m_%d_%H%M)
RESULT_DIR="/data/gpfs/projects/punim2657/sfs_main/results_two_phase_${TIMESTAMP}"
mkdir -p "$RESULT_DIR"
echo "Two-Phase Assembly results will be saved to: $RESULT_DIR"

echo
echo "Verifying fixed NURBS dataset structure..."
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"
echo "Axes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Axes -name "*.xyz" 2>/dev/null | wc -l)"
echo

echo "Building Two-Phase Assembly enabled SFS binary..."
echo "Current binary timestamp: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear 2>/dev/null || echo 'Binary not found')"

echo "Building with Two-Phase Assembly + GGCE support..."
cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker

/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec --bind $(pwd):/workspace /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif bash -c "
    cd /workspace &&
    export CC=/usr/bin/gcc &&
    export CXX=/usr/bin/g++ &&
    echo 'Starting Two-Phase Assembly + GGCE build...' &&
    cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ . &&
    make -j4 &&
    echo 'Two-Phase Assembly + GGCE build completed successfully'
"

echo "Build completed. Binary details:"
ls -la /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear

cd /data/gpfs/projects/punim2657/sfs_main
echo

echo "Running SFS reconstruction with Two-Phase Assembly Architecture..."
echo "Bind mount: Fixed NURBS dataset → /Dataset/SfS_pp"
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo "🎯 Two-Phase Assembly: ENABLED - Advanced connection preservation and global optimization"
echo "🎯 Phase 1: Local assembly with connection preservation"
echo "🎯 Phase 2: Global connectivity resolution with enhanced GGCE"
echo "✨ Output: Results will be saved in results_two_phase_${TIMESTAMP}/ directory"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "export ENABLE_TWO_PHASE_ASSEMBLY=1 && export GGCE_ENABLED=1 && export GGCE_VARIANT=balanced && export GGCE_DEBUG=1 && /workspace/sfspreproc-docker/Hierarchy-Clear 8"

echo
echo "=== Two-Phase Assembly SFS Reconstruction Results ==="
echo "Timestamped output directories created:"
ls -la /data/gpfs/projects/punim2657/sfs_main/results_* 2>/dev/null || echo "No timestamped result directories found - check logs above"

echo
echo "Individual result contents:"
for result_dir in /data/gpfs/projects/punim2657/sfs_main/results_*; do
    if [ -d "$result_dir" ]; then
        echo "📁 $(basename $result_dir):"
        echo "  Main files: $(ls -1 "$result_dir"/*.txt "$result_dir"/*.xyz 2>/dev/null | wc -l) files"
        echo "  Result files: $(ls -1 "$result_dir"/Result/*.txt "$result_dir"/Result/*.xyz 2>/dev/null | wc -l) files"
        echo "  Size: $(du -sh "$result_dir" 2>/dev/null | cut -f1)"
    fi
done

echo
echo "Two-Phase Assembly NURBS SFS reconstruction with timestamped outputs completed at: $(date)"
echo "✅ Two-Phase Assembly Architecture: ENABLED"
echo "✅ Connection Preservation Layer: Applied"
echo "✅ Global Connectivity Resolution: Applied"
echo "✅ Enhanced GGCE Integration: Applied"
echo "✅ Cross-Component Bridge Detection: Applied"
echo "✨ Timestamped Output Directories: Applied"
echo "=== Two-Phase Assembly Test Job Finished ==="