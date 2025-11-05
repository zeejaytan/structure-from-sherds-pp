#!/bin/bash
#SBATCH --job-name=rebuild_legacy
#SBATCH --partition=sapphire
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=16G
#SBATCH --time=0:30:00
#SBATCH --output=rebuild_legacy_%j.out
#SBATCH --error=rebuild_legacy_%j.err

# Rebuild legacy SFS++ with relaxed thresholds and rejection logging

# Load required modules
module load GCC/11.3.0 OpenMPI/4.1.4

CONTAINER_SIF="/data/gpfs/projects/punim2657/sfs_preprocessing/pcl_191_nurbs.sif"
SOURCE_DIR="/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp"
BUILD_DIR="${SOURCE_DIR}/build"

echo "========================================================================="
echo "                 REBUILDING LEGACY SFS++ WITH RELAXED THRESHOLDS"
echo "========================================================================="
echo "Start time: $(date)"
echo ""

echo "=== Cleaning build directory ==="
cd "$BUILD_DIR"
rm -rf *

echo ""
echo "=== Building inside container (DEBUG MODE) ==="
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "${SOURCE_DIR}:/workspace" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace/build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j8"

BUILD_STATUS=$?

echo ""
echo "========================================================================="
if [ $BUILD_STATUS -eq 0 ]; then
    echo "                         BUILD SUCCESSFUL"
    echo "========================================================================="
    echo ""
    ls -lh "${BUILD_DIR}/ReconstructionHeadless"
else
    echo "                         BUILD FAILED"
    echo "========================================================================="
    exit 1
fi

echo ""
echo "End time: $(date)"
echo "========================================================================="
