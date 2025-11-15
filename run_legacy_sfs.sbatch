#!/bin/bash
#SBATCH --job-name=sfs_legacy_original
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_legacy_%j.out
#SBATCH --error=sfs_legacy_%j.err

echo "=== SFS++ LEGACY SYSTEM (Original Author's Code) ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo

cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp

echo "=== Repository Information ==="
echo "Repository: https://github.com/SeongJong-Yoo/sfs_legacy_temp"
echo "Commit: $(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')"
echo "Date: $(git log -1 --format='%cd' --date=short 2>/dev/null || echo 'unknown')"
echo

echo "=== Building Legacy SFS++ Code ==="
echo "Building with CMake inside container..."

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp:/workspace" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4"

BUILD_STATUS=$?
echo
if [ $BUILD_STATUS -ne 0 ]; then
    echo "ERROR: Build failed with exit code $BUILD_STATUS"
    exit 1
fi

echo "Build complete!"
echo "Headless binary location: build/Hierarchy-Clear-headless"
ls -lh build/Hierarchy-Clear-headless 2>/dev/null || echo "WARNING: Headless binary not found!"
ls -lh build/Hierarchy-Clear 2>/dev/null && echo "Original binary also built (with visualization)"
echo

echo "=== Running Legacy SFS++ on Author's Sample Data ==="
echo "Dataset: /data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp"
echo "Configuration: POT_A (8 pieces)"
echo "Parameters from main_headless.cpp:"
echo "  TOP_k = 5 (beam search top-k)"
echo "  BRANCH_b = 3 (branching factor)"
echo "  NO_BASE_INFO defined"
echo
echo "Running headless version (no visualization, auto-save results)"
echo

# Run the headless legacy code
time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp:/Dataset/SfS_pp" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace/build && ./Hierarchy-Clear-headless"

RUN_STATUS=$?
echo
echo "Execution exit code: $RUN_STATUS"
echo

echo "=== Results ==="
if [ -d "/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result" ]; then
    echo "Results directory found:"
    ls -lh /data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/
else
    echo "No results directory found at /data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/"
fi
echo

echo "End time: $(date)"
echo "=== Job Finished ==="
