#!/bin/bash
#SBATCH --job-name=build_axis_0.5
#SBATCH --time=00:30:00
#SBATCH --mem=32G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire
#SBATCH --output=build_axis_threshold_%j.out
#SBATCH --error=build_axis_threshold_%j.err

echo "=== BUILDING WITH AXIS THRESHOLD 0.5 ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"

cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker

echo "Cleaning build directory..."
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile *.a Hierarchy-Clear

echo "Building inside container..."
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && cmake . && make -j8 Hierarchy-Clear"

echo
echo "Build complete at: $(date)"
if [ -f "/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear" ]; then
    echo "✅ Binary created successfully"
    ls -lh /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear
else
    echo "❌ Binary not found - build failed"
fi
